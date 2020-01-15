#include "cs_mutex_locker.hpp"
#include "utils/platform_log.h"


// //////////////////////////////////////////////////////////////////////////////////
//  Never forget to unlock the OSAL mutex again!

CSMutexLocker::CSMutexLocker() 
  : mpMutex(NULL)
{
}

CSMutexLocker::CSMutexLocker(OSALMutexPtrT pMutex) 
  : mpMutex(pMutex) {
  if (mpMutex) {
    OSALLockMutex(mpMutex, OSAL_WAIT_INFINITE);
  }
}

CSMutexLocker::~CSMutexLocker() {
  if (mpMutex) {
    OSALUnlockMutex(mpMutex);
  }
}
