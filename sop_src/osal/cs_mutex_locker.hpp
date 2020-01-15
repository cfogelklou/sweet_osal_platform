#ifndef CS_MUTEXLOCKER_HPP_
#define CS_MUTEXLOCKER_HPP_
#include "osal/osal.h"
#ifdef __cplusplus
// //////////////////////////////////////////////////////////////////////////////////
//  Never forget to unlock the OSAL mutex.
class CSMutexLocker {
private:
  CSMutexLocker();
  const OSALMutexPtrT mpMutex;

public:
  explicit CSMutexLocker(OSALMutexPtrT pMutex);

  ~CSMutexLocker();
};

#endif

#endif // CS_MUTEXLOCKER_HPP_
