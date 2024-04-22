/**
 * COPYRIGHT	(c)	Applicaudia 2020
 * @file     task_sched.h
 * @brief    This is a task scheduler that can be used to
 * schedule run-to-completion state-machine tasks from a single thread or from multiple threads.
 * It can even schedule RTC tasks triggered from an interrupt.
 */
#ifndef TASKSCHED_H__
#define TASKSCHED_H__

#include "osal/osal.h"
#include "utils/dl_list.h"
#include "utils/platform_log.h"

#include <stdbool.h>
#include <stdint.h>

#if defined(OSAL_SINGLE_TASK)
#define TASKSCHED_SINGLETASK
#endif

#if defined(DEBUG) && \
  (defined(WIN32) || defined(__linux__) || defined(TARGET_OS_OSX))
#define TASK_SCHED_DBG
#endif

#ifdef TASK_SCHED_DBG
#define TSCHED_F_OCH_L __FILE__, __LINE__
#else
#define TSCHED_F_OCH_L nullptr, -1
#endif

#define TASK_SCHED_CHECK 0x56892356
#define TASKCHED_BASE_TASK_ID 0x07a50000

// Task Priorities
typedef enum {
// TS_PRIO_HIGH,
#ifdef TASKSCHED_SINGLETASK
  TS_PRIO_APP_EVENTS = 0,
  TS_PRIO_NPI        = 0,
  TS_PRIO_APP        = 0,
  TS_PRIO_BACKGROUND = 0,
  TS_PRIO_IDLE_TASK = 0, /// < Will only run when the idle task is active!
#else
  TS_PRIO_APP_EVENTS = 0, /// < ONLY for non-blocking event handling, like callbacks.
  TS_PRIO_NPI, /// < TI's NPI task
  TS_PRIO_APP, /// < The main application - expected to block occasionally.
  TS_PRIO_BACKGROUND, /// < For slow things like flash reading, etc.
  TS_PRIO_IDLE_TASK, /// < Will only run when the idle task is active!
#endif
  TS_NUM_PRIORITIES ///< do not use this one.  It is not a priority.
} TaskSchedPriority;

// /////////////////////////////////////////////////////////////////////////////
// Schedulables contain this key function callback.
// \func RunnableFnPtr()
// \param pCallbackData: Pointer to data defined by the
// application \param timeOrTicks: Will be "ticks" if the
// iterations scheduler is used, otherwise will be time.
// /////////////////////////////////////////////////////////////////////////////
typedef void (*RunnableFnPtr)(void* pCallbackData, uint32_t timeOrTicks);

struct TaskSchedulableTag;

#ifdef __cplusplus
extern "C" {
#endif

/*
Add a timer function - a function that will execute periodically.
This function uses the hardware timer as the scheduling clock.
*/
#ifndef TASK_SCHED_DBG
void TaskSchedAddTimerFn(
  const TaskSchedPriority prio, struct TaskSchedulableTag* const pSchedulable,
  const uint32_t periodMs, const uint32_t timeOffsetMs);
#else
void _TaskSchedAddTimerFn(
  const TaskSchedPriority prio, struct TaskSchedulableTag* const pSchedulable,
  const uint32_t periodMs, const uint32_t timeOffsetMs,
  const char* const pFile, const int line);
#define TaskSchedAddTimerFn(prio, psched, periodms, timeoffset) \
  _TaskSchedAddTimerFn(prio, psched, periodms, timeoffset, __FILE__, __LINE__)
#endif

#ifdef __cplusplus
}
#endif

///////////////////////////////////////////////////////////////////////////////
/// Schedulable, to schedule tasks at run time in the scheduler.
///////////////////////////////////////////////////////////////////////////////
typedef struct TaskSchedulableTag {
  DLLNode listNode;

  // This must match a known value, TASK_SCHED_CHECK, otherwise schedulable will NOT be run.
  uint32_t chk;

  // Private, next ms or tick to run.  You do not need to set this.
  uint32_t nextExecutionTime;

  // Private, period.  You do not need to set this.
  uint32_t executionPeriod;

  // Public, function to call.  Set this parameter before
  // passing to the scheduler.
  RunnableFnPtr pTaskFn;

  // Public, data to pass to the callback function.  Set
  // this before passing to the scheduler.
  void* pUserData;

#ifdef TASK_SCHED_DBG
  const char* pFile;
  int line;
#endif


#ifdef __cplusplus
  // Default c++ constructor
  TaskSchedulableTag()
    : listNode()
    , chk(TASK_SCHED_CHECK)
    , nextExecutionTime(0)
    , executionPeriod(0)
    , pTaskFn(NULL)
    , pUserData(NULL)
#ifdef TASK_SCHED_DBG
    , pFile("tasksched.h")
    , line(0)
#endif // TASK_SCHED_DBG
  {
  }

  // Constructor taking a runnable function pT.
  // if <to> != -1, will also schedule the schedulable
  TaskSchedulableTag(
    RunnableFnPtr pT,
    void* const pU,
    const TaskSchedPriority tspr = TS_PRIO_APP_EVENTS,
    const uint32_t to            = (uint32_t)(-1),
    const uint32_t per           = 0,
    const bool scheduleNow       = false);

#endif // #ifdef __cplusplus

} TaskSchedulable;

#ifdef __cplusplus
extern "C" {
#endif

/*
Adds a function, but does memory allocation using mempools.
*/
#if (MEMPOOLS_DEBUG_FILETRACE > 0) || defined(TASK_SCHED_DBG)
// This define tracks the memory allocation inside _TaskSchedScheduleFn.
extern bool _TaskSchedScheduleFn(
  const TaskSchedPriority prio, RunnableFnPtr const pTaskFn,
  void* const pUserData, const uint32_t timeOffsetMs,
  const char* const pFile, const int line);
#define TaskSchedScheduleFn(prio, pTaskFn, pUserData, timeOffsetMs) \
  _TaskSchedScheduleFn((prio), (pTaskFn), (pUserData), (timeOffsetMs), __FILE__, __LINE__)
#else
bool TaskSchedScheduleFn(
  const TaskSchedPriority prio,
  RunnableFnPtr const pTaskFn,
  void* const pUserData,
  const uint32_t timeOffsetMs);
#endif

/*
  Forces the singleton to be created.
*/
void TaskSchedInit(void);

void TaskSchedQuit(void);

void TaskSchedDisablePrio(const TaskSchedPriority prio);

void TaskSchedEnablePrio(const TaskSchedPriority prio);

/*
  Add a schedulable that will trigger based on number of iterations of the
  idle task.
  Use TaskSchedAddTimerFn() to add a one-shot.
*/
void TaskSchedAddIdlePollerFn(TaskSchedulable* const pSchedulable, const uint32_t iterationsBetweenPolls);

/*
  Cancel a schedulable.
  NOTE: Not thread safe, so can ONLY be called from the same
  context as the schedulable executes in.
*/
bool TaskSchedCancel(TaskSchedulable* const pSchedulable);

/*
  A FAST unscheduler, but the TASK must have been
  initialized properly for this to work!
  Use only when you KNOW that a task is scheduled/initialized OK.
*/
bool TaskSchedCancelScheduledTask(TaskSchedulable* const pSchedulable);

/*
  Returns TRUE if the task is scheduled (a fast function
  that just checks the node pointer.)
*/
bool TaskSchedIsListed(const TaskSchedulable* const pSchedulable);

/*
  Returns TRUE if the task is scheduled.
*/
bool TaskSchedIsScheduled(const TaskSchedulable* const pSchedulable);

/*
  Initializes a schedulable with a pointer to the function
  to call, and the data to pass to that function.
*/
void TaskSchedInitSched(
  TaskSchedulable* const pSched,
  RunnableFnPtr const pTaskFn, void* const pUserData);


/*
  Your platform shall call TaskSchedPollIdle() from the idle (main) loop.
*/
int32_t TaskSchedPollIdle(void);

/*
  Returns the currently running priority, or TS_PRIO_IDLE_TASK if not recognized by the OSAL.
*/
TaskSchedPriority TaskSched_GetCurrentPriority(void);

/*
  Gets an mutex that can be used by external tasks. Note that this mutex is NOT used internally.
*/
OSALMutexPtrT TaskSched_GetMutex(const TaskSchedPriority prio);

/*
 * Return type for TaskSchedAddEventFn()
 * Events may be triggered safely from within an ISR.
 */
typedef uint32_t TaskSchedEventTrigger;


/**
Add a schedulable that can be awoken based on an event/interrupt.
These are not listed, but added to an internal array.
@param prio: which priority the event should be registered in.
@param pSchedulableFn: The function to schedule.
@param pUserData: Data to be passed back to the function.
@return TaskSchedEventTrigger: This is used later to trigger the event.
If 0 is returned, then the event registration has FAILED.
*/
#ifndef TASK_SCHED_DBG
TaskSchedEventTrigger TaskSchedAddEventFn(
  const TaskSchedPriority prio,
  RunnableFnPtr const pSchedulableFn,
  void* const pUserData,
  TaskSchedEventTrigger trigToOverwrite);
#else
TaskSchedEventTrigger _TaskSchedAddEventFn(
  const TaskSchedPriority prio,
  RunnableFnPtr const pSchedulableFn,
  void* const pUserData,
  TaskSchedEventTrigger trigToOverwrite,
  const char* const pFile,
  const int line);
#define TaskSchedAddEventFn(prio, psf, pud, tto) \
  _TaskSchedAddEventFn((prio), (psf), (pud), (tto), __FILE__, __LINE__)
#endif

/**
Trigger the event from another task.
*/
void TaskSchedTriggerEvent(const TaskSchedEventTrigger evtTrugger);

/**
Trigger the event from another task.
Trigger the event from an ISR.
*/
void TaskSchedTriggerEventFromIsr(const TaskSchedEventTrigger evtTrugger);


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// ////////////////////////////////////////////////////////////////////////////
//
// Use this to remove some boilerplate in calling back to a class's
// member functions from the task scheduler.
// Ensure that the "this" pointer is set as the callback data, otherwise you will
// see hard crashes!
#define TASKSCHED_CALLBACK_DECLARATION(ClassName, MemberFunction) \
  static void MemberFunction##C(void* p, uint32_t ts) {           \
    ((ClassName*)p)->MemberFunction(ts);                          \
  }                                                               \
  void MemberFunction(uint32_t ts)

// ////////////////////////////////////////////////////////////////////////////
// Use this from C++ code to directly call a member function
// from within the task_resched function call.
#define TS_CLASS_FN(ClassName, MemberFunction) \
  [](void* p, uint32_t t) {                    \
    ((ClassName*)p)->MemberFunction(t);        \
  }

// Execute a lambda function with the current timestamp.
// Note: The lambda cannot capture any context.
typedef void (*CapturelessLambda)(uint32_t ts);
void TaskSchedScheduleLambda(
  const char* const pFile,
  const int line,
  CapturelessLambda lambda,
  const uint32_t timeOffsetMs        = 0,
  const TaskSchedPriority prio       = TS_PRIO_APP_EVENTS,
  const uint32_t periodMs            = 0,
  TaskSchedulable* const pSchedToUse = NULL);


// Calls templated lambda functions
// Beware this will cause a bit of extra binary code bloat.
template <typename F>
void TaskSchedScheduleCaptureLambda(
  F& lambda,
  const uint32_t timeOffsetMs  = 0,
  const TaskSchedPriority prio = TS_PRIO_APP_EVENTS) {
  class LambdaData {
  public:
    LambdaData(
      F& lambda,
      const uint32_t timeOffsetMs  = 0,
      const TaskSchedPriority prio = TS_PRIO_APP_EVENTS,
      const uint32_t periodMs      = 0)
      : lambda(lambda)
      , sched(lambdaCallerC, this, prio, timeOffsetMs, periodMs) {
    }

  private:
    static void lambdaCallerC(void* p, uint32_t ts) {
      LambdaData* pThis = (LambdaData*)p;
      pThis->lambda(ts);
      delete pThis;
    };

    F lambda;
    TaskSchedulable sched;
  };
  auto p = new LambdaData(lambda, timeOffsetMs, prio, 0);
  LOG_ASSERT_HPP(p);
}


#endif // #ifdef __cplusplus

#endif
