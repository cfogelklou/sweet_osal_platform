#include "phone_al.hpp"
#include "osal/osal.h"
#include "utils/helper_utils.h"
#include "task_sched/task_sched.h"
#include "phone_al_uithread_runner.hpp"

LOG_MODNAME("uit_sched")

class UiTSched {
public:
  static UiTSched &inst(){
    static UiTSched theInst;
    return theInst;
  }

  // //////////////////////////////////////////////////////////////////////////////////
  bool Cancel() {
    OSALEnterCritical();
    if (pObjectReturnedFromLastCall) {
      PhoneAL::inst().CancelRunnable(pObjectReturnedFromLastCall);
      pObjectReturnedFromLastCall = nullptr;
    }
    OSALExitCritical();
    return false;
  }

  // ////////////////////////////////////////////////////////////////////////////
  void DoSchedule(const uint32_t delay){
    Cancel();

    OSALEnterCritical();
    if (pObjectReturnedFromLastCall) {
      Cancel();
    }

    // Run the next task on the thread
    pObjectReturnedFromLastCall = PhoneAL::inst().RunOnUiThread(uitp_tasksched_runnableCb, this, delay);

    OSALExitCritical();
  }
private:

  UiTSched()
  : pObjectReturnedFromLastCall(nullptr)
  {
  }

  // //////////////////////////////////////////////////////////////////////////////////
  static void uitp_tasksched_runnableCb(void *pObj) {
    UiTSched *pThis = (UiTSched *)pObj;
    OSALEnterCritical();
    pThis->pObjectReturnedFromLastCall = nullptr;
    OSALExitCritical();
    bool runAgain = true;
    while (runAgain){
      runAgain = false;
      int delay = TaskSchedPollIdle();
      if (delay == 0) {
        runAgain = true;
      }
      else if (delay > 0){
        pThis->DoSchedule(delay);
      }
    }
  }

private:

  // When non-null, indicates that there is a callback pending.
  void *pObjectReturnedFromLastCall;

};


// ////////////////////////////////////////////////////////////////////////////
// Externally accessible
void UiTSchedDoSchedule(const uint32_t delay){
  UiTSched::inst().DoSchedule(delay);
}

