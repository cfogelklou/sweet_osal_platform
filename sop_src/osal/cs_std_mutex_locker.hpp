#ifndef CS_STDMUTEXLOCKER_HPP_
#define CS_STDMUTEXLOCKER_HPP_

#ifdef __cplusplus

#include <mutex>
// //////////////////////////////////////////////////////////////////////////////////
//  Never forget to unlock the OSAL mutex.
class CSStdMutexLocker {
private:
  std::mutex &mMutex;

public:
  explicit CSStdMutexLocker(std::mutex& m) 
    : mMutex(m)
  {
    mMutex.lock();
  }

  ~CSStdMutexLocker() {
    mMutex.unlock();
  }
};

#endif

#endif // CS_MUTEXLOCKER_HPP_
