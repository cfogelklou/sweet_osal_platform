#ifndef TASK_RESCHEDULER_HPP
#define TASK_RESCHEDULER_HPP

#include "task_sched/task_sched.h"

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4103)
#endif

#include "utils/pack_push.h"
#include "utils/packed.h"
typedef struct PACKED1 TaskSchedFlagsTag
{
  unsigned int mIsEnabled : 1;
  unsigned int mIsScheduled : 1;
  unsigned int mLocks : 4;
  TaskSchedPriority mTaskPrio : 4;
} PACKED2 TaskSchedFlagsT;
#include "utils/pack_pop.h"
#ifdef WIN32
#pragma warning(pop)
#endif


// ////////////////////////////////////////////////////////////////////////////
// This helper class allows tasks to safely be rescheduled only once.
// If the task is already scheduled, it will not be scheduled again.
class TaskRescheduler
{
private:
  typedef struct CObjTag
  {
    TaskSchedulable sched;
    TaskRescheduler* const pThis;
    CObjTag(TaskRescheduler* pT)
      : sched()
      , pThis(pT) {
    }

    //}
  private:
    CObjTag()
    : pThis(nullptr)
    {}
  } CObj;

public:
  TaskRescheduler(const char* const pFile, const int line, RunnableFnPtr pAppFn,
                  void* const pAppParam = NULL, const TaskSchedPriority prio = TS_PRIO_APP_EVENTS);

  // Destructor - try to avoid this, and use queueDeletion() instead.
  ~TaskRescheduler();

  // Reschedules the task IFF it isn't already scheduled.
  void doReschedule(const uint32_t delay = 0);

  // Reschedules the task, even if it is already scheduled.
  void forceReschedule(const uint32_t delay = 0);

  void disable();
  void enable();
  void cancel();
  inline bool isEnabled() const { return (flags.mIsEnabled) ? true : false; }

  inline bool isScheduled() const { return flags.mIsScheduled ? true : false; }

private:
  static void OnSchedCallbackC(void* p, uint32_t timeOrTicks);
  void OnSchedCallback(uint32_t timeOrTicks);
  static void OnDummyCallback(void*, uint32_t) {}

private:
  uint32_t mChk;
  const RunnableFnPtr mpAppFn;
  void* const mpAppParam;
  uint32_t mNextExec;
  CObj mCObj;
  TaskSchedFlagsT flags;
};


#endif // !TASK_RESCHEDULER_HPP
