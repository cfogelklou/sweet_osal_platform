#if !defined(TASK_TRIGGER_HPP) && defined(__cplusplus)
#define TASK_TRIGGER_HPP

#include "task_sched/task_sched.h"

/**
Helper class which allows for triggering callbacks with built-in Trigger()
functions.
*/
class TaskTrigger
{
public:
  // Constructor
  TaskTrigger(const TaskSchedPriority prio, RunnableFnPtr const pSchedulableFn,
              void* const pUserData)
    : mEnabled(true)
    , mTrig(0)
  {
    mTrig = TaskSchedAddEventFn(prio, pSchedulableFn, pUserData, mTrig);
  }

  // Trigger the task
  void Trigger()
  {
    if (mEnabled) {
      TaskSchedTriggerEvent(mTrig);
    }
  }

  // Trigger the task from an ISR. Doesn't enter critical sections.
  void TriggerFromIsr()
  {
    if (mEnabled) {
      TaskSchedTriggerEventFromIsr(mTrig);
    }
  }

  // Enables the trigger
  bool Enable()
  {
    const bool oldVal = mEnabled;
    mEnabled = true;
    return oldVal;
  }

  // Disables the trigger.
  bool Disable()
  {
    const bool oldVal = mEnabled;
    mEnabled = false;
    return oldVal;
  }

private:
  TaskTrigger()
    : mEnabled(true)
    , mTrig()  
  {}

private:
  bool mEnabled;
  TaskSchedEventTrigger mTrig;
};

#endif
