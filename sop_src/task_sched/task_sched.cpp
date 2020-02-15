/* *************************************************************************************************
 ***************************************************************************************************
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Includes
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "osal/osal.h"
#include "osal/singleton_defs.hpp"
#include "task_sched/task_sched.h"
#include "utils/platform_log.h"
#include "utils/helper_macros.h"
#include "osal/cs_task_locker.hpp"
#include "phone_al/phone_al_uithread_runner.hpp"
//#include "evt_log/evt_log_c_api.h"
#include "utils/convert_utils.h"

LOG_MODNAME("task_sched")


#define NUM_ISR_EVENTS 16


#ifdef TASK_SCHED_DBG
#if (PLATFORM_FULL_OS > 0)
// Uncomment this line in order to check which module has corrupted its installed callback.
//#define HARDCODE_TASK_DEBUG 1
#endif
#else
#define _TaskSchedAddTimerFn(a,b,c,d,e,f) TaskSchedAddTimerFn((a),(b),(c),(d))
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Types and Definitions
///////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_TASKS_PER_EXECUTION 12

#if (TARGET_OS_ANDROID > 0) || (TARGET_OS_IOS > 0)
extern "C" {
void PAKP_ScheduleTaskSched(uint32_t delay);
};
#endif

TaskSchedulableTag::TaskSchedulableTag(
  RunnableFnPtr pT,
  void * const pU,
  const TaskSchedPriority tspr,
  const uint32_t to,
  const uint32_t per,
  const bool scheduleNow
)
: listNode()
, chk(TASK_SCHED_CHECK)
, nextExecutionTime(to)
, executionPeriod(per)
, pTaskFn(pT)
, pUserData(pU)
#ifdef TASK_SCHED_DBG
, pFile("tasksched.h")
, line(0)
#endif
{
  if (scheduleNow) {
    _TaskSchedAddTimerFn(tspr, this, per, to, pFile, line);
  }
}

#if (HARDCODE_TASK_DEBUG > 0)
#include <map>
static std::map<uintptr_t, const char *> ts_hcdbg_ptrmap;
static std::map<uintptr_t, int> ts_hcdbg_linemap;
static void ts_hcdbg_addToMap(const char *file, const int line, void *pts){

  CSTaskLocker cs;
  uintptr_t t = (uintptr_t)pts;
  ts_hcdbg_ptrmap[t] = file;
  ts_hcdbg_linemap[t] = line;
}

#endif

///////////////////////////////////////////////////////////////////////////////
// The TaskSchedQueue is a small class used to keep track of a small amount of
// tasks that will run.
class TaskSchedQueue {
public:

// Only this data is saved when temporarily queuing tasks
  typedef struct ElementTag {
    RunnableFnPtr pTaskFn;
    void *pUserData;
    bool isPeriodic;
#ifdef TASK_SCHED_DBG
    const char *pFile;
    int line;
#endif
  } Element;

private:
  Element arr[MAX_TASKS_PER_EXECUTION];
  int count;

public:
  TaskSchedQueue() {
    memset(arr, 0, sizeof(arr));
    count = 0;
  }

  void push_back(const TaskSchedulable * const pSchedulable) {
    if (count >= MAX_TASKS_PER_EXECUTION) {
      return;
    }
    Element * const pNextElem = &arr[count];
    pNextElem->pTaskFn = pSchedulable->pTaskFn;
    pNextElem->pUserData = pSchedulable->pUserData;
    pNextElem->isPeriodic = (pSchedulable->executionPeriod != 0);
#ifdef TASK_SCHED_DBG
    pNextElem->pFile = pSchedulable->pFile;
    pNextElem->line = pSchedulable->line;
#endif
    count++;
    LOG_ASSERT(count <= MAX_TASKS_PER_EXECUTION);
  }

  int size() { return count; }

  void clear() { count = 0; }

  Element *get(const int i) {
    LOG_ASSERT(i < count);
    return &arr[i];
  }
};

///////////////////////////////////////////////////////////////////////////////
// The task scheduling manager.
class TaskSchedPrio {
private:
  TaskSchedPrio();

  void PollTimed(DLL *const pList, const uint32_t compareTimer,
                 const uint32_t currentTime);

  static void PollTask(void *const pParam);

public:
  TaskSchedPrio(const TaskSchedPriority prio, const char *name);

  void dtor();

  ~TaskSchedPrio();

  static int NodeCompareCb(void *const p, const DLLNode * const pNode0,
                           const DLLNode * const pNode1);

  int32_t DoPoll();

  void    DoPollEvents();

  void Enable();
  void Disable();

  const TaskSchedPriority mPriority;

  uint32_t      mChk;

  // One shots will only execute once, and then will be descheduled.
  DLL mOneShotsList;

  // Timer schedulables run from the free-running hardware timer.
  DLL mTimerBasedList;

  // The iterations based list runs with a resolution of less than the timer.
  // Each scheduler DoPoll is like a timer tick.
  DLL mIterationsBasedList;

  // Last time the iterations counter was executed.
  uint32_t mIterationsCounter;

  // Last time the mTimerBasedList was executed.
  uint32_t mLastTimerProcessTime;

  // Current timer time.
  uint32_t mCurrentTime;

  // These are used by the hardware ISR to awaken tasks from the interrupt.
  // These only run when "awakened" by an trigger from the ISR.
  int             mInterruptEventsIdx;
  TaskSchedQueue::Element mInterruptEventsAry[NUM_ISR_EVENTS];
  uint16_t        mInterruptEventsPendingMask;

  // "working" queue of tasks to run at the end of a single execution.
  TaskSchedQueue mQtl;

#ifndef  TASKSCHED_SINGLETASK
  // Wakes up the thread.
  OSALSemaphorePtrT mpWakeyWakeySem;

  OSALMutexPtrT     mpMutex;

  // The thread itself.
  OSALTaskPtrT mpPollTask;
#endif

  bool mEnabled;

  int mContexts;

  const char * const mName;

  TaskSchedQueue::Element *mpTaskExecuting;

  #ifdef TASK_SCHED_DBG
  uint32_t         mTaskExecStart;
  uint32_t         mTaskExecEnd;
  uint32_t         mWorstCaseExecTime;
  const char *     mWorstCaseExecFile;
  int              mWorstCaseExecLine;
#endif

private:

  void Start();
  friend class TaskScheduler;
  static TaskSchedPrio m_inst;
};

#ifdef TASK_SCHED_DBG
class TaskScheduler;
TaskScheduler *taskSchedPtrDbg = NULL;
#endif
///////////////////////////////////////////////////////////////////////////////
// The task scheduling manager.
class TaskScheduler {

public:
  static TaskScheduler &inst() {
    if (NULL == mInstPtr) {
      OSALEnterTaskCritical();
      if (NULL == mInstPtr) {
        mInstPtr = new (mInstMemAry) TaskScheduler();
#ifdef TASK_SCHED_DBG
        taskSchedPtrDbg = mInstPtr;
#endif
      }
      OSALExitTaskCritical();
    }
    return *mInstPtr;
  }

  ~TaskScheduler();

  void dtor();

  TaskSchedPrio &getScheduler(const TaskSchedPriority prio) {
    LOG_ASSERT(prio < TS_NUM_PRIORITIES);
    return *mpSchedulers[prio];
  }

private:
  uint32_t mChk;
  TaskSchedPrio mIdle;

#ifndef  TASKSCHED_SINGLETASK
  TaskSchedPrio mBack;
  TaskSchedPrio mApp;
  TaskSchedPrio mNpi;
  TaskSchedPrio mEvt;
#endif

  TaskSchedPrio *mpSchedulers[TS_NUM_PRIORITIES];

private:

  TaskScheduler();

  static TaskScheduler *mInstPtr;
  static char mInstMemAry[];
};



///////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Variables
///////////////////////////////////////////////////////////////////////////////////////////////////

// Singleton
TaskScheduler *TaskScheduler::mInstPtr;

// The memory to use for the singleton.
char TaskScheduler::mInstMemAry[sizeof(TaskScheduler)];

#ifndef  TASKSCHED_SINGLETASK

OSAL_INSTANTIATE_STACK(ts_stack1, 3100, TASKCHED_BASE_TASK_ID + TS_PRIO_APP_EVENTS);
OSAL_INSTANTIATE_STACK(ts_stack2, 8500, TASKCHED_BASE_TASK_ID + TS_PRIO_NPI);
OSAL_INSTANTIATE_STACK(ts_stack3, 8500, TASKCHED_BASE_TASK_ID + TS_PRIO_APP);
OSAL_INSTANTIATE_STACK(ts_stack4, 8500, TASKCHED_BASE_TASK_ID + TS_PRIO_BACKGROUND);

///////////////////////////////////////////////////////////////////////////////
// Task Stacks
static const OSALTaskStructT *stacks[TS_NUM_PRIORITIES-1] = {
  &ts_stack1,
  &ts_stack2,
  &ts_stack3,
  &ts_stack4,
};

static const OSALPrioT taskPriorities[TS_NUM_PRIORITIES-1]  = {
  OSAL_PRIO_HIGH,           ///< High priority interface (audio, etc.)
  OSAL_PRIO_MEDIUM_PLUS,    ///< Low priority (worker)
  OSAL_PRIO_MEDIUM,         ///< App & TLS priority
  OSAL_PRIO_LOW             ///< Low priority (worker)
};
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef TASK_SCHED_DBG
volatile bool taskSchedRunMisbehavingFunctionAgain = false;
#endif

///////////////////////////////////////////////////////////////////////////////
//
int32_t TaskSchedPrio::DoPoll() {
  LOG_ASSERT( ++mContexts == 1 ); // Check that this is only called from one thread.
  mCurrentTime = OSALGetMS();
  mQtl.clear();

  mIterationsCounter++;

  // Process the one-shots list.
  // is being accessed or if it needs to be run from the "top" scheduler
  OSALEnterTaskCritical();
  DLLNode *pIter = DLL_BeginFast(&mOneShotsList);
  DLLNode *const pEnd = DLL_EndFast(&mOneShotsList);
  while (pIter != pEnd) {
    DLLNode *const pNext = pIter->pNext;
    TaskSchedulable *const sPtr = (TaskSchedulable *)pIter;
    LOG_ASSERT(pNext != pNext->pNext);

    // Unlist it if it is a one-shot
    if (sPtr->executionPeriod == 0) {
      DLL_NodeUnlist(pIter);
    }

    // Queue for execution
    mQtl.push_back(sPtr);

    // End the loop now and execute.  Remaining tasks will be run next time.
    if (mQtl.size() == MAX_TASKS_PER_EXECUTION) {
      pIter = pEnd;
      break;
    } else {
      // get the next one to process
      pIter = pNext;
    }
  }

  // Process the iterations-based list
  if ((mPriority == TS_PRIO_IDLE_TASK) &&
      (mQtl.size() < MAX_TASKS_PER_EXECUTION) &&
      (!DLL_IsEmptyFast(&mIterationsBasedList))) {
    LOG_ASSERT(TS_PRIO_IDLE_TASK == mPriority);
    PollTimed(&mIterationsBasedList, mIterationsCounter, mCurrentTime);
  }

  // Process the timer-based list
  if ((mQtl.size() < MAX_TASKS_PER_EXECUTION) &&
      (!DLL_IsEmptyFast(&mTimerBasedList))) {
    if (mLastTimerProcessTime != mCurrentTime) {
      mLastTimerProcessTime = mCurrentTime;
      PollTimed(&mTimerBasedList, mCurrentTime, mCurrentTime);
    }
  }
  OSALExitTaskCritical();
  LOG_ASSERT( mContexts == 1 ); // Check that this is only called from one thread.

  // Execute all of the queued tasks on this execution.
  const int numTasks = mQtl.size();
  for (int t = 0; t < numTasks; t++) {
    mpTaskExecuting = mQtl.get(t);
    LOG_ASSERT((mpTaskExecuting) && (mpTaskExecuting->pTaskFn));
#ifndef TASK_SCHED_DBG
    mpTaskExecuting->pTaskFn(mpTaskExecuting->pUserData, mCurrentTime);
#else
    mTaskExecStart = OSALGetMS();
    mpTaskExecuting->pTaskFn(mpTaskExecuting->pUserData, mCurrentTime);
    mTaskExecEnd = OSALGetMS();
    const uint32_t execTime = mTaskExecEnd - mTaskExecStart;

    if (execTime > mWorstCaseExecTime) {
      mWorstCaseExecTime = execTime;
      mWorstCaseExecFile = mpTaskExecuting->pFile;
      mWorstCaseExecLine = mpTaskExecuting->line;
      volatile bool *pRunAgain = &taskSchedRunMisbehavingFunctionAgain;
      if (*pRunAgain) {
        mpTaskExecuting->pTaskFn(mpTaskExecuting->pUserData, mCurrentTime);
      }
    }
#endif
    mpTaskExecuting = nullptr;
  }
  LOG_ASSERT( mContexts == 1 ); // Check that this is only called from one thread.
  mQtl.clear();

  // Return the time to next execution.
  int32_t timeToNextRun = -1;
  CSTaskLocker cs;
  if (!DLL_IsEmptyFast(&mOneShotsList)) {
    timeToNextRun = 0;
  } else if (!DLL_IsEmptyFast(&mIterationsBasedList)) {
    timeToNextRun = 0;
  } else if (!DLL_IsEmptyFast(&mTimerBasedList)) {
    TaskSchedulable * const pNext =
        (TaskSchedulable *)DLL_BeginFast(&mTimerBasedList);
    const int32_t t = pNext->nextExecutionTime - OSALGetMS();
    //t = MIN((((int32_t)(1u << 31) - 1)), t);
    timeToNextRun = MAX(0, t);
  }
  LOG_ASSERT( --mContexts == 0 ); // Check that this is only called from one thread.

  return timeToNextRun;
}

///////////////////////////////////////////////////////////////////////////////
//
void TaskSchedPrio::DoPollEvents() {
  // Poll ISR events.
  uint16_t eventsMask = 0;
  OSALEnterTaskCritical(); //ISR critical.
  eventsMask = mInterruptEventsPendingMask;
  mInterruptEventsPendingMask = 0;
  OSALExitTaskCritical(); //ISR critical.

  if (0 != eventsMask) {
    const uint32_t timestamp = OSALGetMS();
    for (size_t i = 0; i < ARRSZ(mInterruptEventsAry); i++) {
      const uint16_t bitMask = (1u << i);
      if (0 != (bitMask & eventsMask)) {
        TaskSchedQueue::Element &ts = mInterruptEventsAry[i];
        LOG_ASSERT(ts.pTaskFn);
        ts.pTaskFn(ts.pUserData, timestamp);
      }
    }
  }
}


///////////////////////////////////////////////////////////////////////////////
//  tasksched_NodeCompareCb:
//     Used for list sorting
//
//  \return:
//      >   0 if (Node0Ptr > Node1Ptr)
//      ==  0 if (Node0Ptr == Node1Ptr)
//      <   0 if (Node0Ptr < Node1Ptr)
int TaskSchedPrio::NodeCompareCb(void * const p, const DLLNode * const pNode0,
                                 const DLLNode * const pNode1) {
  (void)p;
  const TaskSchedulable * const p0 = (const TaskSchedulable *)pNode0;
  const TaskSchedulable * const p1 = (const TaskSchedulable *)pNode1;
  return (p0->nextExecutionTime - p1->nextExecutionTime);
}

///////////////////////////////////////////////////////////////////////////////
//  Run timed schedulables that need to run.
void TaskSchedPrio::PollTimed(DLL *const pList,
                              const uint32_t compareTimer,
                              const uint32_t currentTime) {
  (void)currentTime;
  // Process the iterations-based list
  DLLNode *pIter = DLL_BeginFast(pList);
  DLLNode *const pEnd = DLL_EndFast(pList);

  while (pIter != pEnd) {
    TaskSchedulable *const sPtr = (TaskSchedulable *)pIter;
    const int32_t timeDiff = sPtr->nextExecutionTime - compareTimer;

    if (timeDiff > 0) {
      pIter = pEnd; // Break out
    } else {
      DLLNode *const pNext = pIter->pNext;
#if (HARDCODE_TASK_DEBUG > 0)
      if (!pIter->pNext){
        uintptr_t t = (uintptr_t)pIter;
        const char *fname = ts_hcdbg_ptrmap[t];
        const int fline = ts_hcdbg_linemap[t];
        if (fname){
          LOG_TRACE(("Misbehaving schedulable was installed by %s:%d\r\n", fname, fline));
        }
      }
#endif
      LOG_ASSERT(pIter->pNext);
      LOG_ASSERT(pNext != pNext->pNext);

      // We need to remove and re-add the schedulable so it's in the right
      // order.
      DLL_NodeUnlist(pIter);

      // Put it back in the list (sorted.)
      if (sPtr->executionPeriod != 0) {
        LOG_ASSERT(((int)sPtr->executionPeriod) > 0);

        // Ensure we always schedule in the future
        if ((-timeDiff) >= (int32_t)sPtr->executionPeriod) {
          // Schedule for next run
          sPtr->nextExecutionTime = compareTimer + 1;
#ifdef TASKSCHED_PROFILING
          if ((pSched->mLatencyOverruns & 0x0f) == 0) {
            TestPoint_SetProfilingParam(enTPTaskSchedLatencyOverruns,
                                        pSched->mLatencyOverruns);
          }
#endif
        } else {
          // Reschedule, compensating for latency.
          sPtr->nextExecutionTime =
              compareTimer + sPtr->executionPeriod + timeDiff;
        }

        // Reschedule the task.
        DLL_SortedInsert(pList, &sPtr->listNode,
                               TaskSchedPrio::NodeCompareCb, this);

        // Time to execute the function
      }
      LOG_ASSERT(TASK_SCHED_CHECK == sPtr->chk);
      LOG_ASSERT(NULL != sPtr->pTaskFn);

      // Queue for execution
      mQtl.push_back(sPtr);

      if (mQtl.size() == MAX_TASKS_PER_EXECUTION) {
        pIter = pEnd;
      } else {
        pIter = pNext;
      }
    }
  }
}

#ifndef  TASKSCHED_SINGLETASK
///////////////////////////////////////////////////////////////////////////////
void TaskSchedPrio::PollTask(void* const pParam)
{
  TaskSchedPrio * const pThis = (TaskSchedPrio *)pParam;
  while(TASK_SCHED_CHECK == pThis->mChk){
    if (pThis->mEnabled) {
      pThis->DoPollEvents();

      // Poll Timer events.
      const int32_t nextWaitTime = pThis->DoPoll();

      const uint32_t wait = (nextWaitTime < 0) ? OSAL_WAIT_INFINITE : nextWaitTime;
      OSALSemaphoreWait( pThis->mpWakeyWakeySem, wait );
    } else {
      OSALSemaphoreWait( pThis->mpWakeyWakeySem, OSAL_WAIT_INFINITE);
    }

  }
}
#endif

///////////////////////////////////////////////////////////////////////////////
void TaskSchedPrio::Enable() {
  if (TASK_SCHED_CHECK != mChk) return;
  mEnabled = true;
#ifndef  TASKSCHED_SINGLETASK
  OSALSemaphoreSignal(mpWakeyWakeySem, 1);
#endif
}

///////////////////////////////////////////////////////////////////////////////
void TaskSchedPrio::Disable() {
  if (TASK_SCHED_CHECK != mChk) return;
  mEnabled = false;
}

///////////////////////////////////////////////////////////////////////////////
// Constructor
TaskSchedPrio::TaskSchedPrio(
    const TaskSchedPriority prio,
    const char * const name)
  : mPriority(prio)
  , mChk( TASK_SCHED_CHECK )
  , mIterationsCounter(0)
  , mLastTimerProcessTime(0)
  , mCurrentTime(0)
  , mInterruptEventsIdx(0)
  , mInterruptEventsAry()
  , mInterruptEventsPendingMask(false)
  , mQtl()
#ifndef  TASKSCHED_SINGLETASK
  , mpWakeyWakeySem(NULL)
  , mpMutex(NULL)
  , mpPollTask(NULL)
#endif
  , mEnabled(true)
  , mContexts( 0 )
  , mName( name )
  , mpTaskExecuting(nullptr)
#ifdef TASK_SCHED_DBG
  , mTaskExecStart(0)
  , mTaskExecEnd(0)
  , mWorstCaseExecTime(0)
  , mWorstCaseExecFile(nullptr)
  , mWorstCaseExecLine(0)
#endif
{
  memset(mInterruptEventsAry, 0, sizeof(mInterruptEventsAry));
  DLL_Init(&mOneShotsList);
  DLL_Init(&mTimerBasedList);
  DLL_Init(&mIterationsBasedList);

}

///////////////////////////////////////////////////////////////////////////////
// Destructor
void TaskSchedPrio::Start(){
#ifndef  TASKSCHED_SINGLETASK
  // If it's one of the tasks, then create the semaphore and thread.
  if ((mPriority < TS_PRIO_IDLE_TASK) && (NULL == mpWakeyWakeySem)) {
    const int taskIdx = (int)mPriority;
    mpWakeyWakeySem = OSALSemaphoreCreate(0, 1);
    mpMutex = OSALCreateMutex();
    mpPollTask = OSALTaskCreate(TaskSchedPrio::PollTask, this,
                                      taskPriorities[taskIdx],
                                      stacks[taskIdx]);
  }
#endif
}

///////////////////////////////////////////////////////////////////////////////
void TaskSchedPrio::dtor() {
  if (TASK_SCHED_CHECK == mChk) {
    mChk = 0;
#ifndef  TASKSCHED_SINGLETASK
    if (mpWakeyWakeySem) {
      OSALSemaphoreSignal(mpWakeyWakeySem, 1);
    }
#endif
  }
}

///////////////////////////////////////////////////////////////////////////////
// Destructor
TaskSchedPrio::~TaskSchedPrio() {
  dtor();
#ifndef  TASKSCHED_SINGLETASK
  if (mpPollTask){
    OSALTaskDelete(&mpPollTask);
  }
  if (mpWakeyWakeySem){
    OSALSemaphoreDelete(&mpWakeyWakeySem);
  }
  if (mpMutex) {
    OSALDeleteMutex(&mpMutex);
  }
#endif
}

///////////////////////////////////////////////////////////////////////////////
TaskScheduler::TaskScheduler()
    : mChk(TASK_SCHED_CHECK)
    , mIdle(TS_PRIO_IDLE_TASK, "idle")
#ifndef  TASKSCHED_SINGLETASK
    , mBack(TS_PRIO_BACKGROUND, "background")
    , mApp(TS_PRIO_APP, "app")
    , mNpi(TS_PRIO_NPI, "npi")
    , mEvt(TS_PRIO_APP_EVENTS, "events")
    , mpSchedulers()

#endif
{
  LOG_ASSERT(NULL == mInstPtr);
  mInstPtr = this;
#ifndef  TASKSCHED_SINGLETASK
  mpSchedulers[TS_PRIO_APP] = &mApp;
  mpSchedulers[TS_PRIO_APP_EVENTS] = &mEvt;
  mpSchedulers[TS_PRIO_BACKGROUND] = &mBack;
  mpSchedulers[TS_PRIO_NPI] = &mNpi;
  mApp.Start();
  mNpi.Start();
  mEvt.Start();
  mBack.Start();
#endif
  mpSchedulers[TS_PRIO_IDLE_TASK] = &mIdle;
  mIdle.Start();

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void TaskScheduler::dtor() {
  if (TASK_SCHED_CHECK == mChk) {
    mChk = 0;
    for (int i = 0; i < TS_NUM_PRIORITIES; i++) {
      TaskSchedPrio *p = mpSchedulers[i];
      if (p) {
        p->dtor();
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TaskScheduler::~TaskScheduler() {
  dtor();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  Global Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" {


  ///////////////////////////////////////////////////////////////////////////////
  //  Add a timer function - a function that will execute periodically.
#ifndef TASK_SCHED_DBG
  void TaskSchedAddTimerFn(const TaskSchedPriority prio,
    TaskSchedulable *const pSchedulable,
    const uint32_t periodMs, const uint32_t timeOffsetMs) {
#else
  void _TaskSchedAddTimerFn(const TaskSchedPriority prio,
    TaskSchedulable *const pSchedulable,
    const uint32_t periodMs,
    const uint32_t timeOffsetMs, const char * const pFile, const int line) {
#if (HARDCODE_TASK_DEBUG > 0)
    ts_hcdbg_addToMap(pFile, line, pSchedulable);
#endif
#endif // TASK_SCHED_DBG

    TaskSchedPrio &sched = TaskScheduler::inst().getScheduler(prio);

    LOG_ASSERT((NULL != pSchedulable) && (NULL != pSchedulable->pTaskFn));
    LOG_ASSERT((pSchedulable->listNode.pNext == NULL) || (pSchedulable->listNode.pNext == &pSchedulable->listNode));

    if ((NULL != pSchedulable)) {
#ifdef TASK_SCHED_DBG
      pSchedulable->pFile = CNV_stripSlash(pFile);
      pSchedulable->line = line;
#endif

      // If no time or period specified, execute immediately using oneshot list.
      // If time or period specified, then use timer list.
      if ((periodMs == 0) && (timeOffsetMs == 0)) {

        // Execute on next scheduler pass
        pSchedulable->executionPeriod = 0;
        OSALEnterTaskCritical();
        DLL_PushBack(&sched.mOneShotsList, &pSchedulable->listNode);
        LOG_ASSERT(pSchedulable->listNode.pNext != &pSchedulable->listNode);

#ifndef  TASKSCHED_SINGLETASK
        if (sched.mpWakeyWakeySem) {
          if (&pSchedulable->listNode == DLL_GetFront(&sched.mOneShotsList)) {
            OSALSemaphoreSignal(sched.mpWakeyWakeySem, 1);
          }
        }
#elif (TARGET_OS_ANDROID > 0) || (TARGET_OS_IOS > 0)
        UiTSchedDoSchedule(0);
#endif
        OSALExitTaskCritical();

      }
      else {
        // Set period and next execution time, then insert on list.
        LOG_ASSERT(((int)periodMs) >= 0);

        pSchedulable->executionPeriod = periodMs;

        pSchedulable->nextExecutionTime = OSALGetMS() + timeOffsetMs;

        if (true){
          CSTaskLocker lock;
          const bool inserted = DLL_SortedInsert(
                                                 &sched.mTimerBasedList, &pSchedulable->listNode,
                                                 TaskSchedPrio::NodeCompareCb, &sched);
          if (!inserted) {
#if (HARDCODE_TASK_DEBUG > 0)
            DLLNode *pIter = DLL_BeginFast(&sched.mTimerBasedList);
            DLLNode *const pEnd = DLL_EndFast(&sched.mTimerBasedList);
            while (pIter != pEnd) {
              if (nullptr == pIter->pNext) {
                uintptr_t t = (uintptr_t)pIter;
                const char *fname = ts_hcdbg_ptrmap[t];
                const int fline = ts_hcdbg_linemap[t];
                if (fname) {
                  LOG_TRACE(("Misbehaving schedulable was installed by %s:%d\r\n", fname, fline));
                }
              }
              else {
                pIter = pIter->pNext;
              }
            }
            LOG_ASSERT(false);
#endif // (HARDCODE_TASK_DEBUG > 0)
          }
          LOG_ASSERT(pSchedulable->listNode.pNext != &pSchedulable->listNode);

#ifndef  TASKSCHED_SINGLETASK
        if (sched.mpWakeyWakeySem) {
          if (&pSchedulable->listNode == DLL_GetFront(&sched.mTimerBasedList)) {
            OSALSemaphoreSignal(sched.mpWakeyWakeySem, 1);
          }
        }
#elif (TARGET_OS_ANDROID > 0) || (TARGET_OS_IOS > 0)
          UiTSchedDoSchedule(0);
#endif

      }
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Structure used to save a one-shot timer function.
  typedef struct tasksched_OneShotTag {
    TaskSchedulable sched;
    RunnableFnPtr pTaskFn;
    void *pUserData;
  } tasksched_OneShotT;

  ///////////////////////////////////////////////////////////////////////////////
  // Callback used by TaskSchedScheduleFn
  static void tasksched_ScheduleFnCb(void *pCallbackData, uint32_t timeOrTicks) {
    tasksched_OneShotT *const pData = (tasksched_OneShotT *)pCallbackData;
    LOG_ASSERT((pData) && (pData->pTaskFn));
    pData->pTaskFn(pData->pUserData, timeOrTicks);
    OSALFREE(pData);
  }

  ///////////////////////////////////////////////////////////////////////////////
  //
#if (MEMPOOLS_DEBUG_FILETRACE > 0) || defined(TASK_SCHED_DBG)
  bool _TaskSchedScheduleFn(
#else
  bool TaskSchedScheduleFn(
#endif
    const TaskSchedPriority prio,
    RunnableFnPtr const pTaskFn,
    void *const pUserData,
    const uint32_t timeOffsetMs
#if (MEMPOOLS_DEBUG_FILETRACE > 0) || defined(TASK_SCHED_DBG)
    ,const char * const pFile,
    const int line
#endif
  ) {
    bool rval = false;
    tasksched_OneShotT *const pOneShot = (tasksched_OneShotT *)
#if (MEMPOOLS_DEBUG_FILETRACE > 0)
      _MemPoolsMalloc(sizeof(tasksched_OneShotT), pFile, line);
      //ts_addToMap(pFile, pSchedulable);
#else
      OSALMALLOC(sizeof(tasksched_OneShotT));
#endif
    if (NULL != pOneShot) {
#ifdef TASK_SCHED_DBG
      pOneShot->sched.pFile = pFile;
      pOneShot->sched.line = line;
#endif
      pOneShot->pTaskFn = pTaskFn;
      pOneShot->pUserData = pUserData;
      TaskSchedInitSched(&pOneShot->sched, tasksched_ScheduleFnCb, pOneShot);
      _TaskSchedAddTimerFn(prio, &pOneShot->sched, 0, timeOffsetMs, pFile, line);
      rval = true;
    }
    return rval;
  }

} // extern "C" {

///////////////////////////////////////////////////////////////////////////////
void TaskSchedScheduleLambda(
  const char * const pFile,
  const int line,
  CapturelessLambda lambda,
  const uint32_t timeOffsetMs,
  const TaskSchedPriority prio,
  const uint32_t periodMs,
  TaskSchedulable * const pSchedToUse
)
{
  (void)pFile;
  (void)line;
  if (pSchedToUse){
    auto thisLambda = [](void *p, uint32_t ts) {
      CapturelessLambda pFn = (CapturelessLambda)p;
      pFn(ts);
    };
    TaskSchedInitSched(pSchedToUse, thisLambda, (void *)lambda);
    _TaskSchedAddTimerFn(prio, pSchedToUse, periodMs, timeOffsetMs, pFile, line);
  }
  else {
    LOG_ASSERT(periodMs == 0);
    if (0 == periodMs) {
      typedef struct LambdaDataTag
      {
        TaskSchedulable sched;
        CapturelessLambda pFn;
        LambdaDataTag(CapturelessLambda fn)
          : sched()
          , pFn(fn)
        {
        }
      } LambdaData;


      LambdaData *pLambda = new LambdaData(lambda);

      auto thisLambdaDelete = [](void *p, uint32_t ts) {
        LambdaData *pFn = (LambdaData *)p;
        pFn->pFn(ts);
        delete pFn;
      };
      TaskSchedInitSched(&pLambda->sched, thisLambdaDelete, pLambda);
      _TaskSchedAddTimerFn(prio, &pLambda->sched, periodMs, timeOffsetMs, pFile, line);
    }
  }
}

extern "C" {
///////////////////////////////////////////////////////////////////////////////
int32_t TaskSchedPollIdle(void){
  TaskSchedPrio &sched = TaskScheduler::inst().getScheduler(TS_PRIO_IDLE_TASK);
  return sched.DoPoll();
}

///////////////////////////////////////////////////////////////////////////////
// Returns the currently running priority, or TS_PRIO_IDLE_TASK if not recognized by the OSAL.
TaskSchedPriority TaskSched_GetCurrentPriority(void) {
  TaskSchedPriority rval = TS_PRIO_IDLE_TASK;
#ifdef TASKSCHED_SINGLETASK
#else
  const uint32_t taskId = OSALGetCurrentTaskID();
  const int32_t id = taskId - TASKCHED_BASE_TASK_ID;
  if ((id >= 0) && (id < TS_PRIO_IDLE_TASK)) {
    rval = (TaskSchedPriority)id;
  }
  else {
    rval = TS_PRIO_IDLE_TASK;
  }
#endif
  return rval;
}


///////////////////////////////////////////////////////////////////////////////
// Gets the mutex associated with the task scheduler.
OSALMutexPtrT TaskSched_GetMutex(const TaskSchedPriority prio) {
#ifndef  TASKSCHED_SINGLETASK
  TaskSchedPrio &sched = TaskScheduler::inst().getScheduler(prio);
  LOG_ASSERT(sched.mpMutex);
  return sched.mpMutex;
#else
  return NULL;
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Add a schedulable that will trigger based on number of iterations of the
// scheduler
void TaskSchedAddIdlePollerFn(TaskSchedulable *const pSchedulable,
                              const uint32_t iterationsBetweenPolls) {
  // Choose the correct list depending on this function can be run while
  // USB/tfat
  // is being accessed or if it needs to be run from the "top" scheduler
  TaskSchedPrio &sched = TaskScheduler::inst().getScheduler(TS_PRIO_IDLE_TASK);
  LOG_ASSERT((NULL != pSchedulable) && (NULL != pSchedulable->pTaskFn));
  LOG_ASSERT(pSchedulable->listNode.pNext == NULL);

  if ((NULL != pSchedulable)) {
    
    LOG_ASSERT(pSchedulable->chk == TASK_SCHED_CHECK);

    // If Iterations between polls is 1, then we execute every time (use the
    // one-shots list since it's more efficient)
    LOG_ASSERT(((int)iterationsBetweenPolls) > 0);
    if (iterationsBetweenPolls <= 1) {
      pSchedulable->executionPeriod = 1;
      OSALEnterTaskCritical();
      DLL_PushBack(&sched.mOneShotsList, &pSchedulable->listNode);
      OSALExitTaskCritical();
    } else {

      // Otherwise insert on the iterations list.
      pSchedulable->executionPeriod = iterationsBetweenPolls;

      pSchedulable->nextExecutionTime =
          sched.mIterationsCounter + iterationsBetweenPolls;

      OSALEnterTaskCritical();
      DLL_SortedInsert(&sched.mIterationsBasedList,
                             &pSchedulable->listNode,
                             TaskSchedPrio::NodeCompareCb, &sched);
      LOG_ASSERT(pSchedulable->listNode.pNext != &pSchedulable->listNode);

      OSALExitTaskCritical();
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
#ifndef TASK_SCHED_DBG
TaskSchedEventTrigger TaskSchedAddEventFn(
#else
TaskSchedEventTrigger _TaskSchedAddEventFn(
#endif
  const TaskSchedPriority prio,
  RunnableFnPtr const pSchedulableFn,
  void *const pUserData,
  TaskSchedEventTrigger trigToOverwrite
#ifdef TASK_SCHED_DBG
  , const char * const pFile, const int line
#endif
) {

  TaskSchedPrio &sched = TaskScheduler::inst().getScheduler(prio);
  TaskSchedEventTrigger evtTrigger = (((uint32_t)prio) << 16) | 0x80000000u;

  OSALEnterTaskCritical(); // ISR Critical!

  if (sched.mInterruptEventsIdx < NUM_ISR_EVENTS) {
    TaskSchedQueue::Element *pTs = NULL;

    if (trigToOverwrite) {
      // Calling function would like to re-install another callback.
      evtTrigger = trigToOverwrite;
      const TaskSchedPriority prioNew = (TaskSchedPriority)((evtTrigger >> 16) & 0x7fff);
      LOG_ASSERT(prioNew == prio);
      const uint16_t evtIdx = (evtTrigger >> 0) & 0xffff;
      pTs = &sched.mInterruptEventsAry[evtIdx];
    }
    else {
      // Calling function wants to register a new callback.
      evtTrigger |= sched.mInterruptEventsIdx;
      pTs = &sched.mInterruptEventsAry[sched.mInterruptEventsIdx++];
      LOG_ASSERT_WARN(pTs->pTaskFn == NULL);
    }

    LOG_ASSERT(pTs);

    pTs->pTaskFn = pSchedulableFn;
    pTs->pUserData = pUserData;
#ifdef TASK_SCHED_DBG
    pTs->pFile = pFile;
    pTs->line = line;
#endif

    LOG_ASSERT(sched.mInterruptEventsIdx <= NUM_ISR_EVENTS);
  }
  else {
    evtTrigger = 0;
  }
  OSALExitTaskCritical(); // ISR Critical!

  return evtTrigger;
}

///////////////////////////////////////////////////////////////////////////////
//
void TaskSchedTriggerEvent(const TaskSchedEventTrigger evtTrigger) {
  TaskScheduler &inst = TaskScheduler::inst();
  const TaskSchedPriority prio = (TaskSchedPriority)((evtTrigger >> 16) & 0x7fff);
  const uint16_t evtIdx = (evtTrigger >> 0) & 0xffff;
  TaskSchedPrio &sched = inst.getScheduler(prio);
  //evtlog_AllocEvent(inst.mpIsrFact, "evt trigger", "non-isr", 0, prio);
#ifndef  TASKSCHED_SINGLETASK
  OSALEnterTaskCritical();
  sched.mInterruptEventsPendingMask |= (1u << evtIdx);
  OSALSemaphoreSignal(sched.mpWakeyWakeySem, 1);
  OSALExitTaskCritical();
#else
  sched.mInterruptEventsPendingMask |= (1u << evtIdx);
  sched.mInterruptEventsAry[evtIdx].pTaskFn(sched.mInterruptEventsAry[evtIdx].pUserData, OSALGetMS());
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
void TaskSchedTriggerEventFromIsr(const TaskSchedEventTrigger evtTrigger) {
#if (PLATFORM_EMBEDDED > 0)
  const TaskSchedPriority prio = (TaskSchedPriority)((evtTrigger >> 16) & 0x7fff);
  const uint16_t evtIdx = (evtTrigger >> 0) & 0xffff;
  TaskSchedPrio &sched = TaskScheduler::inst().getScheduler(prio);
  sched.mInterruptEventsPendingMask |= (1u << evtIdx);
#ifndef  TASKSCHED_SINGLETASK
  OSALSemaphoreSignalFromIsr(sched.mpWakeyWakeySem, 1);
#endif
  //evtlog_AllocEventFromIsr(inst.mpIsrFact, "isr trigger", 0, prio);
#else
  TaskSchedTriggerEvent(evtTrigger);
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
bool TaskSchedCancel(TaskSchedulable *const pSchedulable) {
  OSALEnterTaskCritical();
  const bool isInAList = TaskSchedIsScheduled(pSchedulable);
  if (isInAList) {
    DLL_NodeUnlist(&pSchedulable->listNode);
  }
  LOG_ASSERT(pSchedulable->listNode.pNext == NULL);
  OSALExitTaskCritical();

  return isInAList;
}

///////////////////////////////////////////////////////////////////////////////
//
bool TaskSchedCancelScheduledTask(TaskSchedulable *const pSchedulable) {
  bool rval = false;
  if((pSchedulable) && (pSchedulable->listNode.pPrev) &&
             (pSchedulable->listNode.pNext)){
    OSALEnterTaskCritical();
    if (pSchedulable->listNode.pPrev) {
      DLL_NodeUnlist(&pSchedulable->listNode);
      rval = true;
    }
    LOG_ASSERT((pSchedulable->listNode.pNext == NULL) || (pSchedulable->listNode.pNext == &pSchedulable->listNode));
    OSALExitTaskCritical();
  }
  return rval;
}

///////////////////////////////////////////////////////////////////////////////
//  Returns TRUE if the task is scheduled somewhere.
bool TaskSchedIsScheduled(const TaskSchedulable *const pSchedulable) {
  bool isScheduled = false;
  if (pSchedulable != NULL) {
    TaskScheduler &ts = TaskScheduler::inst();
    OSALEnterTaskCritical();
    for (int i = 0; (!isScheduled) && (i < TS_NUM_PRIORITIES); i++) {
      const TaskSchedPriority prio = (TaskSchedPriority)i;
      TaskSchedPrio &sched = ts.getScheduler(prio);
      isScheduled =
          DLL_IsListed(&sched.mTimerBasedList, &pSchedulable->listNode);
      if (!isScheduled) {
        isScheduled =
            DLL_IsListed(&sched.mOneShotsList, &pSchedulable->listNode);
      }
      if (!isScheduled) {
        isScheduled |= DLL_IsListed(&sched.mIterationsBasedList,
                                          &pSchedulable->listNode);
      }
    }
    OSALExitTaskCritical();
  }
  return isScheduled;
}

///////////////////////////////////////////////////////////////////////////////
// Returns TRUE if the task is scheduled (a fast function that just checks the
// node pointer.)
bool TaskSchedIsListed(const TaskSchedulable *const pSchedulable) {
  bool isScheduled = false;
  if (pSchedulable != NULL) {
    isScheduled = (pSchedulable->listNode.pNext != NULL);
  }
  return isScheduled;
}

///////////////////////////////////////////////////////////////////////////////
//
void TaskSchedInitSched(TaskSchedulable *const pSched,
                        RunnableFnPtr const pTaskFn, void *const pUserData) {
  LOG_ASSERT(pSched);
  DLL_NodeInit(&pSched->listNode);
  pSched->chk = TASK_SCHED_CHECK;
  pSched->pTaskFn = pTaskFn;
  pSched->pUserData = pUserData;
}

///////////////////////////////////////////////////////////////////////////////
void TaskSchedInit(void){
  (void)TaskScheduler::inst();
}

///////////////////////////////////////////////////////////////////////////////
void TaskSchedQuit(void){
  TaskScheduler::inst().dtor();
}
                                          
///////////////////////////////////////////////////////////////////////////////
void TaskSchedDisablePrio(const TaskSchedPriority prio){
  TaskSchedPrio &sched = TaskScheduler::inst().getScheduler(prio);
  sched.Disable();
}

///////////////////////////////////////////////////////////////////////////////
void TaskSchedEnablePrio(const TaskSchedPriority prio){
  TaskSchedPrio &sched = TaskScheduler::inst().getScheduler(prio);
  sched.Enable();

}

} // extern "C" {
