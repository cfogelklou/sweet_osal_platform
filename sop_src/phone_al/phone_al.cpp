#include "phone_al.hpp"
#include "osal/osal.h"
#include "utils/helper_utils.h"
#include "task_sched/task_sched.h"

// Set in JNI or .m file to an object that can run stuff.
PhoneAL* phone_al_global_ptr = nullptr;

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

static PakSchedulable pakSchedTask;


// //////////////////////////////////////////////////////////////////////////////////
void PakSchedDoSchedule(const uint32_t delay);

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

// //////////////////////////////////////////////////////////////////////////////////
bool PakSchedCancel(
    PhoneAL * const pHal,
    PakSchedulable * const pSched) {
  OSALEnterCritical();
  if (pSched->pObjectReturnedFromLastCall) {
    if (pHal) {
      pHal->CancelRunnable(pSched->pObjectReturnedFromLastCall);
    }
    pSched->pObjectReturnedFromLastCall = NULL;
  }
  OSALExitCritical();
  return false;
}

// //////////////////////////////////////////////////////////////////////////////////
void PakSchedInitSched(
    PhoneAL * const pHal,
    PakSchedulable * const pSched,
    pakp_RunnableFnPtr   const pTaskFn,
    void           * const /*@unused@*/ pUserData) {
  OSALEnterCritical();
  if (pSched->pObjectReturnedFromLastCall) {
    PakSchedCancel(pHal, pSched);
  }
  pSched->pFnRunnable = pTaskFn;
  pSched->pCallbackData = pUserData;
  pSched->pObjectReturnedFromLastCall = NULL;
  OSALExitCritical();
}

// //////////////////////////////////////////////////////////////////////////////////
void PakSchedInit(PhoneAL * const pHal){
  phone_al_global_ptr = pHal;
  PakSchedInitSched(pHal, &pakSchedTask, paksched_TaskSchedPoll, nullptr);
}

// //////////////////////////////////////////////////////////////////////////////////
void pakp_tasksched_runnableCb(void *pObj) {
  PakSchedulable * const pSched = (PakSchedulable *)pObj;
  LOG_ASSERT(pSched);
  LOG_ASSERT(pSched->pFnRunnable);
  if ((pSched) && (pSched->pFnRunnable)) {
    PakSchedulable tmp;
    OSALEnterCritical();
    tmp = *pSched;
    pSched->pObjectReturnedFromLastCall = NULL;
    OSALExitCritical();
    tmp.pFnRunnable(tmp.pCallbackData, OSALGetMS());
  }
}

// //////////////////////////////////////////////////////////////////////////////////
void PakSchedAddTimerFn(
    PhoneAL * const pHal,
    PakSchedulable * const pSched,
    const uint32_t timeOffsetMs) {
  LOG_ASSERT(pSched != NULL);
  LOG_ASSERT(pHal != NULL);
  LOG_ASSERT(pSched->pFnRunnable);
  OSALEnterCritical();
  if (pSched->pObjectReturnedFromLastCall) {
    PakSchedCancel(pHal, pSched);
  }
  if (pHal) {
    pSched->pObjectReturnedFromLastCall = pHal->RunOnUiThread(pakp_tasksched_runnableCb, pSched, timeOffsetMs);
  }
  OSALExitCritical();
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

// //////////////////////////////////////////////////////////////////////////////////
bool PakSchedIsScheduled(
    const PakSchedulable * const pSched) {
  return PakSchedIsListed(pSched);
}

// ////////////////////////////////////////////////////////////////////////////
void PakSchedDoSchedule(const uint32_t delay){
  if (phone_al_global_ptr) {
    PakSchedCancel(phone_al_global_ptr, &pakSchedTask);
    PakSchedAddTimerFn(phone_al_global_ptr, &pakSchedTask, delay);
  }
}

// ////////////////////////////////////////////////////////////////////////////
extern "C" {
void PAKP_ScheduleTaskSched(uint32_t delay) {
  PakSchedDoSchedule(delay);
}
}
