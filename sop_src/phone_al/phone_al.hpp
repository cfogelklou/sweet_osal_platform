#ifndef PHONE_AL_HPP
#define PHONE_AL_HPP

#ifdef __cplusplus
#include <cstdint>

// This abstract class shall be implemented by Android/iOS/Windows
class PhoneAL {
public:

  typedef void (*onMonoAudioCbPtr)(
    void *pCbData, const int fs, const float * const pMonoArr, const int numSamps);

public:
  PhoneAL()
  : mpOnMonoAudioFn(nullptr)
  , mpOnMonoAudioFnData(nullptr)
  {

  }

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

  virtual void StartAudioInputSrc(
    const int fs = 48000,
    onMonoAudioCbPtr cb = nullptr,
    void *pCbData = nullptr
  ){
    mFs = fs;
    mpOnMonoAudioFn = cb;
    mpOnMonoAudioFnData = pCbData;
  }

  virtual void StopAudioInputSrc(){

  }

  // Implement in Android/iOS/Windows/etc.
  static PhoneAL &inst();

protected:
  onMonoAudioCbPtr mpOnMonoAudioFn;
  void *mpOnMonoAudioFnData;
  int mFs;

};


#endif // __cplusplus

#endif
