#ifndef PHONE_AL_HPP
#define PHONE_AL_HPP

#ifdef __cplusplus
#include <cstdint>

class PhoneAL {
public:
  virtual ~PhoneAL() {}

  typedef void (*PAKAL_RunnableCb)(void *pObj);

  // Executes a runnable from the "UI" thread.  Passes back a pointer that can
  // be used to cancel it.
  virtual void *RunOnUiThread(
      PAKAL_RunnableCb cb,
      void *const pObj,
      const int delayMs)
  {

    (void)cb;
    (void)pObj;
    (void)delayMs;
    return nullptr;
  }

  // Cancels a runnable.  Uses the pointer passed back from RunOnUiThread.
  virtual void CancelRunnable(void *pObjReturnedFromRunCall) {
    (void)pObjReturnedFromRunCall;
  }

};

// Called by the .mm or ndk implementation of the phone AL.
void PakSchedInit(PhoneAL* const pHal);

#endif // __cplusplus

#endif
