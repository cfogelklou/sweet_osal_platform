#include "task_rescheduler.hpp"
#include "osal/cs_task_locker.hpp"
#include "utils/platform_log.h"
#include "utils/convert_utils.h"
#include <string.h>


LOG_MODNAME("task_resched.cpp")

// ////////////////////////////////////////////////////////////////////////////
TaskRescheduler::TaskRescheduler(const char* const pFile, const int line, RunnableFnPtr pAppFn,
                                 void* const pAppParam, const TaskSchedPriority prio)
  : mChk(0x44445555)
  , mpAppFn(pAppFn)
  , mpAppParam(pAppParam)
  , mNextExec(0)
  , mCObj(this)
  , flags({ true, false, 0, prio })
{
  TaskSchedInitSched(&mCObj.sched, OnSchedCallbackC, &mCObj);
#ifdef TASK_SCHED_DBG
  mCObj.sched.pFile = CNV_stripSlash(pFile);
  mCObj.sched.line = line;
#else
  (void)pFile;
  (void)line;
#endif
  LOG_ASSERT((mCObj.sched.listNode.pNext == NULL) ||
             (mCObj.sched.listNode.pNext == &mCObj.sched.listNode));
}

// ////////////////////////////////////////////////////////////////////////////
TaskRescheduler::~TaskRescheduler()
{
  mChk = 0;
  flags.mIsEnabled = false;
  CSTaskLocker locker;

  LOG_ASSERT(1 == ++flags.mLocks);
  flags.mIsEnabled = false;
  if ((mCObj.sched.pTaskFn) && (mCObj.sched.pTaskFn != OnDummyCallback)) {
    if (flags.mIsScheduled) {
      LOG_TRACE(("%s::Note: Deleting a scheduled task!\r\n", dbgModId));
      TaskSchedCancelScheduledTask(&mCObj.sched);
      flags.mIsScheduled = false;
    }
  }
  mCObj.sched.pTaskFn = OnDummyCallback;
  LOG_ASSERT(0 == --flags.mLocks);
}

///////////////////////////////////////////////////////////////////////////////
void
TaskRescheduler::OnSchedCallbackC(void* p, uint32_t timeOrTicks)
{
  CObj& obj = *(CObj*)p;
  if ((obj.pThis) && (obj.pThis->mChk == 0x44445555)) {
    obj.pThis->OnSchedCallback(timeOrTicks);
  }
  else {
    LOG_WARNING(("Could not execute schedulable. No this pointer.\r\n"));
  }
}

///////////////////////////////////////////////////////////////////////////////
// TODO: Reschedulables can be FORCE rescheduled even though they are on the
// way to RUN (the task has been rescheduled between CS sections in Task Scheduler)
void
TaskRescheduler::OnSchedCallback(uint32_t timeOrTicks)
{
  RunnableFnPtr pAppFn = NULL;
  void* pAppParam = NULL;
  {
    CSTaskLocker locker;
    LOG_ASSERT(1 == ++flags.mLocks);
    if ((flags.mIsScheduled) && (mCObj.sched.listNode.pNext == NULL)) {
      flags.mIsScheduled = false;
      LOG_ASSERT(mCObj.sched.listNode.pNext == NULL);
      if ((flags.mIsEnabled) && (mpAppFn)) {
        pAppFn = mpAppFn;
        pAppParam = mpAppParam;
      }
    }
    LOG_ASSERT(0 == --flags.mLocks);
  }
  if (pAppFn) {
    pAppFn(pAppParam, timeOrTicks);
  }
}

// ////////////////////////////////////////////////////////////////////////////
void
TaskRescheduler::doReschedule(const uint32_t delay)
{

  ASSERT_AT_COMPILE_TIME(sizeof(this->flags) < 5);

  CSTaskLocker locker;
  LOG_ASSERT(1 == ++flags.mLocks);
  if (flags.mIsEnabled) {
    bool doSchedule = false;
    const uint32_t nextExec = OSALGetMS() + delay;
    if (!flags.mIsScheduled) {
      doSchedule = true;
      LOG_ASSERT((mCObj.sched.listNode.pNext == NULL) ||
                 (mCObj.sched.listNode.pNext == &mCObj.sched.listNode));
    } else {
      const int32_t timeDiff = nextExec - mNextExec;
      doSchedule = (timeDiff < 0);
      if (doSchedule) {
        TaskSchedCancelScheduledTask(&mCObj.sched);
        LOG_ASSERT((mCObj.sched.listNode.pNext == NULL) ||
                   (mCObj.sched.listNode.pNext == &mCObj.sched.listNode));
      }
    }
    if (doSchedule) {
      mNextExec = nextExec;
      flags.mIsScheduled = true;
      LOG_ASSERT((mCObj.sched.listNode.pNext == NULL) ||
                 (mCObj.sched.listNode.pNext == &mCObj.sched.listNode));
#ifdef TASK_SCHED_DBG
      _TaskSchedAddTimerFn(flags.mTaskPrio, &mCObj.sched, 0, delay, mCObj.sched.pFile,
                           mCObj.sched.line);
#else
      TaskSchedAddTimerFn(flags.mTaskPrio, &mCObj.sched, 0, delay);
#endif
    }
  }
  LOG_ASSERT(0 == --flags.mLocks);
}

// ////////////////////////////////////////////////////////////////////////////
void
TaskRescheduler::forceReschedule(const uint32_t delay)
{

  CSTaskLocker locker;

  LOG_ASSERT(1 == ++flags.mLocks);
  if (flags.mIsEnabled) {
    const uint32_t nextExec = OSALGetMS() + delay;
    if (flags.mIsScheduled) {
      TaskSchedCancelScheduledTask(&mCObj.sched);
    } else if (mCObj.sched.listNode.pNext) {
      LOG_WARNING(("TaskRescheduler::Cancelling a task that shouldn't be scheduled!\r\n"));
      TaskSchedCancelScheduledTask(&mCObj.sched);
    }
    mNextExec = nextExec;
    flags.mIsScheduled = true;
#ifdef TASK_SCHED_DBG
    _TaskSchedAddTimerFn(flags.mTaskPrio, &mCObj.sched, 0, delay, mCObj.sched.pFile, mCObj.sched.line);
#else
    TaskSchedAddTimerFn(flags.mTaskPrio, &mCObj.sched, 0, delay);
#endif
  }
  LOG_ASSERT(0 == --flags.mLocks);
}

// ////////////////////////////////////////////////////////////////////////////
void
TaskRescheduler::disable()
{
  CSTaskLocker locker;

  LOG_ASSERT(1 == ++flags.mLocks);
  flags.mIsEnabled = false;
  if (flags.mIsScheduled) {
    flags.mIsScheduled = false;
    TaskSchedCancelScheduledTask(&mCObj.sched);
  }
  LOG_ASSERT(0 == --flags.mLocks);
}

// ////////////////////////////////////////////////////////////////////////////
void
TaskRescheduler::enable()
{
  CSTaskLocker locker;

  LOG_ASSERT(1 == ++flags.mLocks);
  flags.mIsEnabled = true;
  LOG_ASSERT(0 == --flags.mLocks);
}

// ////////////////////////////////////////////////////////////////////////////
void
TaskRescheduler::cancel()
{
  CSTaskLocker locker;

  LOG_ASSERT(1 == ++flags.mLocks);
  if (flags.mIsScheduled) {
    flags.mIsScheduled = false;
    TaskSchedCancelScheduledTask(&mCObj.sched);
  }
  LOG_ASSERT(0 == --flags.mLocks);
}
