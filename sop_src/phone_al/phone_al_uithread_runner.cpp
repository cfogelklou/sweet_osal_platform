#include "phone_al.hpp"
#include "osal/osal.h"
#include "utils/helper_utils.h"
#include "task_sched/task_sched.h"

LOG_MODNAME("pak_sched")

// //////////////////////////////////////////////////////////////////////////////////
void pakp_tasksched_runnableCb(void *pObj);

typedef void(*pakp_RunnableFnPtr)(void *pCallbackData, uint32_t timeOrTicks);

struct PakSchedulableTag;

// //////////////////////////////////////////////////////////////////////////////////
typedef struct PakSchedulableTag {
  void *pObjectReturnedFromLastCall = nullptr;
  pakp_RunnableFnPtr pFnRunnable = nullptr;
  void *pCallbackData = nullptr;
} PakSchedulable;

// We can only schedule one task at a time.
// //////////////////////////////////////////////////////////////////////////////////
void PakSchedDoSchedule(const uint32_t delay);
void PakSchedInitSched(
    PakSchedulable * const pSched,
    pakp_RunnableFnPtr   const pTaskFn,
    void           * const /*@unused@*/ pUserData);

class PakSched {
public:
  static PakSched &inst(){
    static PakSched theInst;
    return theInst;
  }

  // //////////////////////////////////////////////////////////////////////////////////
  static void paksched_TaskSchedPoll(void *, uint32_t timeOrTicks) {
    bool runAgain = true;
    while (runAgain){
      runAgain = false;
      int delay = TaskSchedPollIdle();
      if (delay == 0) {
        runAgain = true;
      }
      else if (delay > 0){
        PakSchedDoSchedule(delay);
      }
    }
  }

  PakSchedulable &getSched(){
    return pakSchedTask;
  }
private:

  PakSched()
  : pakSchedTask()
  {
    PakSchedInitSched(&pakSchedTask, paksched_TaskSchedPoll, nullptr);
  }
  PakSchedulable pakSchedTask;
};


// //////////////////////////////////////////////////////////////////////////////////
bool PakSchedCancel() {
  auto &sched = PakSched::inst().getSched();
  OSALEnterCritical();
  if (sched.pObjectReturnedFromLastCall) {
    PhoneAL::inst().CancelRunnable(sched.pObjectReturnedFromLastCall);
    sched.pObjectReturnedFromLastCall = nullptr;
  }
  OSALExitCritical();
  return false;
}

// //////////////////////////////////////////////////////////////////////////////////
void PakSchedInitSched(
    pakp_RunnableFnPtr   const pTaskFn,
    void           * const /*@unused@*/ pUserData) {
  PakSchedulable &sched = PakSched::inst().getSched();
  OSALEnterCritical();
  if (sched.pObjectReturnedFromLastCall) {
    PakSchedCancel();
  }
  sched.pFnRunnable = pTaskFn;
  sched.pCallbackData = pUserData;
  sched.pObjectReturnedFromLastCall = nullptr;
  OSALExitCritical();
}

// //////////////////////////////////////////////////////////////////////////////////
void pakp_tasksched_runnableCb(void *pObj) {
  PakSchedulable &sched = PakSched::inst().getSched();
  LOG_ASSERT(sched.pFnRunnable);
  if (sched.pFnRunnable) {
    PakSchedulable tmp;
    OSALEnterCritical();
    tmp = sched;
    sched.pObjectReturnedFromLastCall = nullptr;
    OSALExitCritical();
    tmp.pFnRunnable(tmp.pCallbackData, OSALGetMS());
  }
}

// //////////////////////////////////////////////////////////////////////////////////
bool PakSchedIsListed(
    const PakSchedulable * const pSched) {
  bool rval = false;
  LOG_ASSERT(pSched);
  if (pSched) {
    rval = (pSched->pObjectReturnedFromLastCall) ? true : false;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
void PakSchedDoSchedule(const uint32_t delay){
  PakSchedCancel();
  PakSchedulable &sched = PakSched::inst().getSched();
  LOG_ASSERT(sched.pFnRunnable);
  OSALEnterCritical();
  if (sched.pObjectReturnedFromLastCall) {
    PakSchedCancel();
  }

  // Run the next task on the thread
  sched.pObjectReturnedFromLastCall =
      PhoneAL::inst().RunOnUiThread(pakp_tasksched_runnableCb, &sched, delay);
  OSALExitCritical();
}

