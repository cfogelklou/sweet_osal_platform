/*
 * osal_posix.cpp
 *
 *  Created on: Jan 24, 2020
 *      Author: chris
 */

/*
 * osal.cpp
 *
 *  Created on: Sep 22, 2020
 *      Author: Chris
 */

#include "osal/osal.h"
#include "utils/platform_log.h"
#include "utils/helper_utils.h"
#include "utils/helper_macros.h"
#include "osal/cs_task_locker.hpp"
#include <map>
#include <stdint.h>
#include <string.h>
#include <thread>
#include <mutex>

#ifndef EOK
#define EOK 0
#endif

#if defined(__linux__) || defined(__APPLE__)

LOG_MODNAME("osal.cpp");

#if defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
//#define PRIO_IN_THREAD
#include <errno.h>
#include <cstdlib>

#if defined(__APPLE__)
#define GCDSEM
#endif

#ifdef GCDSEM
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

#include "mbedtls/entropy.h"

#define POSIX_OBJHDR 0x5432abcdu

class posix_OsalBase {
protected:
  uint32_t mHdr;

public:
  bool check(void) {
    const bool rval = (POSIX_OBJHDR == mHdr);
    LOG_ASSERT(rval);
    return rval;
  }

  posix_OsalBase() : mHdr(POSIX_OBJHDR) {}

  virtual ~posix_OsalBase() {
    check();
    mHdr = ~POSIX_OBJHDR;
  }
};

// //////////////////////////////////////////////
// Get milliseconds without any critical sections.
static uint32_t getMS(void) {
  static uint32_t firstMs = 0;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint32_t rval = ((uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000)) - firstMs;
  if (0 == firstMs){
    firstMs = rval;
    rval = 0;
  }
  return rval;
}


// //////////////////////////////////////////////
class posix_OsalMutex : public posix_OsalBase {
protected:
  std::recursive_mutex mMutex;

public:
  posix_OsalMutex()
  : posix_OsalBase()
  , mMutex(){
  }

  virtual ~posix_OsalMutex() {
    check();
    if (!lock(10000)) {
      fprintf(stderr, "Unable to get lock in 10 seconds, destroying anyway\r\n");
    }
    else {
      unlock();
    }
    
  }

  bool lock(const uint32_t timeoutMs) {
    check();
    bool rval = false;
    if (timeoutMs == OSAL_WAIT_INFINITE) {
      // If no timeout specified, then just do a normal wait
      mMutex.lock();
      rval = true;
    } else {
      bool gotSem = false;
      uint32_t currentTime = getMS();
      const uint32_t timeOut = currentTime + (0x7fffffff & timeoutMs);
      int32_t timeRemaining = timeOut - currentTime;
      while ((!gotSem) && (timeRemaining > 0)) {
        const bool gotIt = mMutex.try_lock();
        gotSem = gotIt;
        if (!gotSem) {
          usleep(2000);
          currentTime = getMS();
          timeRemaining = timeOut - currentTime;
        }
      }
      rval = gotSem;
    }
    return rval;
  }

  bool unlock() {
    check();
    mMutex.unlock();
    return true;
  }
};

static uint32_t mInitializedTag = 0;

class OSAL {
public:
  static OSAL &inst() {
    static OSAL inst;
    return inst;
  }
  
#ifndef ccs
  void EnterCritical() {
    if (0x99999999 == mInitializedTag) {
      if (!mMutex.lock(OSAL_WAIT_INFINITE)) {
        LOG_ASSERT(false);
        exit(-1);
      }
#ifdef CHECK_CS
      uint64_t tid;
      pthread_threadid_np(NULL, &tid);
      LOG_ASSERT((mOwnerThreadId == 0) || (tid == mOwnerThreadId));
      mOwnerThreadId = tid;
      mRecurse++;
#endif
    }
    else {
      mMissedCriticals++;
      //LOG_WARNING(("***Mutex not initialized %d times!\r\n", mMissedCriticals));
    }
  }

  void ExitCritical() {
    if (0x99999999 == mInitializedTag) {
#ifdef CHECK_CS
      uint64_t tid;
      pthread_threadid_np(NULL, &tid);
      LOG_ASSERT(mOwnerThreadId == tid);
      LOG_ASSERT(mRecurse > 0);
      if (--mRecurse == 0){
        mOwnerThreadId = 0;
      }
      LOG_ASSERT(mRecurse >= 0);
#endif
      mMutex.unlock();
    } else {
      mMissedCriticals++;
      //LOG_WARNING(("***Mutex not initialized %d times!!\r\n", mMissedCriticals));
    }
    
  }
#endif


  static int mMissedCriticals;

  typedef std::map<pthread_t, uint32_t> IdMapT;
  IdMapT *psosal_IdMap;
  IdMapT &osal_IdMap;
#ifdef CHECK_CS
  uint64_t mOwnerThreadId;
  int mRecurse;
#endif
  
private:
  posix_OsalMutex mMutex;
  
  OSAL()
  : psosal_IdMap(new IdMapT)
  , osal_IdMap(*psosal_IdMap)
#ifdef CHECK_CS
  , mOwnerThreadId(0)
  , mRecurse(0)
#endif
  , mMutex()
  {
    LOG_ASSERT(mInitializedTag == 0);
    mInitializedTag = 0x99999999;
  }
  ~OSAL() {
    //LOG_ASSERT(0x99999999 == mInitializedTag);
    mInitializedTag = 1;
  }
};

int OSAL::mMissedCriticals = 0;


extern "C" {
void OSALEnterCritical(void) { OSAL::inst().EnterCritical(); }
void OSALExitCritical(void) { OSAL::inst().ExitCritical(); }
void OSALEnterTaskCritical(void) { OSAL::inst().EnterCritical(); }
void OSALExitTaskCritical(void) { OSAL::inst().ExitCritical(); }
}
#endif



extern "C" {

  extern void MemPoolsInitialize();
  
// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALInit(void) {
  static bool init = false;
  
  if (!init) {
#if defined(_POSIX_PRIORITY_SCHEDULING) && !defined(ANDROID) && !defined(OSAL_SINGLE_TASK)
    struct sched_param param;
    param.sched_priority = 10;
    // pthread_attr_t attr;
    // LOG_ASSERT_FN(EOK == pthread_attr_init(&attr));
    // LOG_ASSERT_WARN_FN(EOK == pthread_attr_setinheritsched(&attr,
    // PTHREAD_EXPLICIT_SCHED) );
    // pid_t pid = getpid();
    LOG_ASSERT_WARN_FN(
        EOK == pthread_setschedparam(pthread_self(), SCHED_FIFO, &param));
#endif
    init = true;
    MemPoolsInitialize();
    //LCAL_Init();
    (void)OSAL::inst();
    //OSALRandomInit("posix", 5);
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void *OSALCreateMutex() {
  posix_OsalMutex *const pMutex = new posix_OsalMutex();
  LOG_ASSERT(pMutex);
  return (void *)pMutex;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALDeleteMutex(void **ppPortMutex) {
  posix_OsalMutex **ppMutex = (posix_OsalMutex **)ppPortMutex;
  posix_OsalMutex *const pMutex = *ppMutex;

  LOG_ASSERT(pMutex != NULL);
  if (pMutex != NULL) {
    delete pMutex;
    *ppPortMutex = NULL;
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALLockMutex(void *const pPortMutex, const uint32_t timeoutMs) {
  posix_OsalMutex *const pMutex = (posix_OsalMutex *)pPortMutex;
  if (pMutex){
    return pMutex->lock(timeoutMs);
  }
  else {
    return false;
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALUnlockMutex(void *const pPortMutex) {
  posix_OsalMutex *const pMutex = (posix_OsalMutex *)pPortMutex;
  if (pMutex){
    return pMutex->unlock();
  }
  else {
    return false;
  }
}

  
// ///////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ///////////////////////////////////////////////////////////////////////////////
uint32_t OSALGetMS(void) {
#if (TARGET_OS_OSX > 0)
  CSTaskLocker lock;
#endif
  const uint32_t ms = getMS();
  return ms;
}

} // extern "C" {

class posix_OsalCntSem : public posix_OsalBase {
protected:
  int mMaxCnt;
  int mCnt;
#ifdef GCDSEM
  dispatch_semaphore_t mSem;
#else
  sem_t mSem;
#endif
public:
  posix_OsalCntSem(const int initValue, const int maxValue)
      : posix_OsalBase(), mMaxCnt(maxValue), mCnt(initValue) {

#ifdef GCDSEM
    mSem = dispatch_semaphore_create(initValue);
    LOG_ASSERT(mSem);
#else
    LOG_ASSERT_FN(EOK == sem_init(&mSem, false, initValue));
#endif
  }

  virtual ~posix_OsalCntSem() {
    check();
#ifdef GCDSEM
// dispatch_semaphore_destroy( mSem );
#else
    LOG_ASSERT_FN(EOK == sem_destroy(&mSem));
#endif
  }

  bool wait(const uint32_t timeoutMs) {
    check();
    bool rval = false;
#ifdef GCDSEM
    if (OSAL_WAIT_INFINITE == timeoutMs) {
      rval = (EOK == dispatch_semaphore_wait(mSem, DISPATCH_TIME_FOREVER));
    } else {
      // LOG_TRACE(("Setting sem timeout to %u ms", timeoutMs));
      const int64_t timeoutns = (int64_t)(NSEC_PER_MSEC) * (int64_t)(timeoutMs);
      const dispatch_time_t timeoutTime =
          dispatch_time(DISPATCH_TIME_NOW, timeoutns);
      rval = (EOK == dispatch_semaphore_wait(mSem, timeoutTime));
    }
#else
    if (timeoutMs == OSAL_WAIT_INFINITE) {
      // If no timeout specified, then just do a normal wait
      LOG_ASSERT_FN(EOK == sem_wait(&mSem));
      rval = true;
    } else {
      bool gotSem = false;
      uint32_t currentTime = OSALGetMS();
      const uint32_t timeOut = currentTime + (0x7fffffff & timeoutMs);
      while ((!gotSem) && (currentTime < timeOut)) {
        const int status = sem_trywait(&mSem);
        gotSem = (EOK == status);
        if (!gotSem) {
#ifdef EBUSY
          // LOG_TRACE(("Could not get sem due to code %d\r\n", status));
          LOG_ASSERT(
              (-1 == status) || (EAGAIN == status) ||
              (EBUSY == status)); // Check that the wait failed due to business.
#endif
          usleep(2000);
          currentTime = OSALGetMS();
        }
      }
      rval = gotSem;
    }
#endif // GCDSEM
    if (rval) {
      OSALEnterTaskCritical();
      const bool isGteZero = (--mCnt >= 0);
      OSALExitTaskCritical();
      LOG_ASSERT_FN(isGteZero);
    }
    return rval;
  }

  bool signal(const int inc) {
    check();
    OSALEnterTaskCritical();
    const int finalCnt = mCnt + inc;
    const int endCnt = MIN(finalCnt, mMaxCnt);
#ifdef GCDSEM
    for (; mCnt < endCnt; mCnt++) {
      dispatch_semaphore_signal(mSem);
    }
    OSALExitTaskCritical();
    return (endCnt == finalCnt);
#else
    int status = EOK;
    for (; (status == EOK) && (mCnt < endCnt); mCnt++) {
      status = sem_post(&mSem);
    }
    OSALExitTaskCritical();
    return ((endCnt == finalCnt) && (EOK == status));
#endif
  }
};

extern "C" {
// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
OSALSemaphorePtrT OSALSemaphoreCreate(const uint32_t initialValue,
                                      const uint32_t maxValue) {

  posix_OsalCntSem *const pSem = new posix_OsalCntSem(initialValue, maxValue);
  LOG_ASSERT(pSem);

  return (OSALSemaphorePtrT)pSem;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALSemaphoreDelete(OSALSemaphorePtrT *const ppSemaphore) {
  bool rval = false;
  posix_OsalCntSem **ppSem = (posix_OsalCntSem **)ppSemaphore;
  LOG_ASSERT(ppSem);
  posix_OsalCntSem *const pSem = *ppSem;
  LOG_ASSERT(pSem);

  pSem->check();

  if (pSem != NULL) {
    delete pSem;
    rval = true;
  }
  *ppSem = NULL;
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALSemaphoreSignal(OSALSemaphorePtrT const pPortSem,
                         const uint32_t count) {
  bool rval = false;
  posix_OsalCntSem *const pSem = (posix_OsalCntSem *)pPortSem;
  LOG_ASSERT(pSem);
  if (pSem){
    pSem->check();
    rval = pSem->signal(count);
  }
  else {
    rval = false;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
// Signal a counting semaphore from the ISR
bool OSALSemaphoreSignalFromIsr(OSALSemaphorePtrT const pPortSem, const uint32_t count) {
  return OSALSemaphoreSignal(pPortSem, count);
}


// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALSemaphoreWait(OSALSemaphorePtrT const pPortSem,
                       const uint32_t timeoutMs) {
  posix_OsalCntSem *const pSem = (posix_OsalCntSem *)pPortSem;
  LOG_ASSERT(pSem);
  if (pSem){
    LOG_ASSERT(pSem->check());
    return pSem->wait(timeoutMs);
  }
  else {
    return false;
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALSleep(const uint32_t ms) {
#if 1 // ndef OSAL_SINGLE_TASK
  const uint32_t startTime = OSALGetMS();
  uint32_t now = startTime;
  uint32_t elapsed = (now - startTime);
  while (elapsed < ms) {
    uint32_t remaining = ms - elapsed;
    usleep((uint32_t)((uint64_t)remaining * (uint64_t)1000u));
    elapsed = (now - OSALGetMS());
  }
#else
  (void)ms;
  LOG_VERBOSE(("Attempted to sleep %d ms from a single task!\r\n", ms));
#endif
}

} // extern "C" {

#ifndef OSAL_SINGLE_TASK

// Prio map
static const int osalPrioMap[] = {
    0,
    17, // OSAL_PRIO_CRITICAL
    16, // OSAL_PRIO_HIGH
    14, // OSAL_PRIO_IA
    11, // OSAL_PRIO_MEDIUM
    4,  // OSAL_PRIO_LOW
    2   // OSAL_PRIO_BACKGND
};

class OSALTask : public posix_OsalBase {
public:

private:

  // ////////////////////////////////////////////////////////////////////////////////////////////////
  // Internal function just calls the application's task function
  static void *osalTaskFxn(void *a0) {
    OSALTask *pTask = (OSALTask *)a0;
    if (!pTask->check()) {
      LOG_WARNING(("OSAL has been destructed.\r\n"));
      return nullptr;
    };

    pthread_t me = pthread_self();

#ifdef PRIO_IN_THREAD
    {
      int policy;
      struct sched_param param;

      LOG_ASSERT_FN(EOK == pthread_getschedparam(me, &policy, &param));
      LOG_ASSERT_FN(EOK == pthread_setschedparam(me, SCHED_FIFO, &param));
      /* Set the scheduling priority for TARGET_THREAD.  */
      LOG_ASSERT_FN(EOK == pthread_setschedprio(me, pTask->mPriority));
    }
#endif

    {
      OSALEnterTaskCritical();
      auto &idmap = OSAL::inst().osal_IdMap;
      const OSAL::IdMapT::iterator f = idmap.find(me);
      LOG_ASSERT(f == idmap.end());
      idmap[me] = pTask->mTaskId;
      OSALExitTaskCritical();
    }

    pTask->mFnPtr(pTask->mParamPtr);
    return NULL;
  }

  static bool mSafeMode;

private:
  pthread_t mhThread;
  pthread_attr_t mAttr;
  int mPriority;
  uint32_t mTaskId;
  
public:
  OSALTaskFuncPtrT mFnPtr;
  void *mParamPtr;

  void initAttr(bool safe) {
    struct sched_param param;
    LOG_ASSERT_FN(EOK == pthread_attr_init(&mAttr));
    LOG_ASSERT_FN(EOK == pthread_attr_getschedparam(&mAttr, &param));

#ifndef PRIO_IN_THREAD

    if (!safe) {
#ifndef ANDROID
      LOG_ASSERT_WARN_FN(
          EOK == pthread_attr_setinheritsched(&mAttr, PTHREAD_EXPLICIT_SCHED));
#endif
      LOG_ASSERT_WARN_FN(EOK ==
                         pthread_attr_setschedpolicy(&mAttr, SCHED_FIFO));
      param.sched_priority = mPriority;
      const int prio_stat = pthread_attr_setschedparam(&mAttr, &param);
      LOG_ASSERT_WARN(EOK == prio_stat);
      if (EOK != prio_stat) {
        LOG_TRACE(("Got %d when setting priority.\r\n", prio_stat));
      }
    }
#endif
  }

  // //////////////////////
  OSALTask(OSALTaskFuncPtrT pFn, void *pParam,
            const OSALTaskStructT *const pTaskStruct, 
            const OSALPrioT prio)
    : posix_OsalBase()
    , mhThread()
    , mAttr()
    , mPriority(0)
    , mTaskId(pTaskStruct->taskId)
    , mFnPtr(pFn)
    , mParamPtr(pParam)
  {

    mPriority = osalPrioMap[prio];

    initAttr(mSafeMode);

#ifdef __QNX__
    LOG_ASSERT_FN(EOK ==
                  pthread_attr_setstackaddr(&mAttr, pTaskStruct->pStack));
    LOG_ASSERT_FN(EOK ==
                  pthread_attr_setstacksize(&mAttr, pTaskStruct->stackSize));
#endif // #ifdef __QNX__

    const int threadStatus =
        pthread_create(&mhThread, &mAttr, &osalTaskFxn, (void *)this);
    if (EOK != threadStatus) {
      if (EPERM == threadStatus) {
        mSafeMode = true;
        LOG_TRACE(
            ("**Got permission error when setting thread priority.\r\n**Trying "
             "without priority (next time, run with SUDO!).\r\n"));
        initAttr(mSafeMode);
        LOG_ASSERT_FN(EOK == pthread_create(&mhThread, &mAttr, &osalTaskFxn,
                                            (void *)this));
      } else {
        LOG_TRACE(("Got %d when creating thread.\r\n", threadStatus));
        LOG_ASSERT(EOK == threadStatus);
      }
    }

    LOG_ASSERT(&mhThread != NULL);
  }

  virtual ~OSALTask() {

    check();
    LOG_ASSERT_FN(EOK == pthread_join(mhThread, nullptr));
#ifndef ANDROID
    LOG_ASSERT_WARN_FN(EOK == pthread_cancel(mhThread));
#endif
    LOG_ASSERT_FN(EOK == pthread_attr_destroy(&mAttr));
  }
};

bool OSALTask::mSafeMode = false;

extern "C" {
// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
OSALTaskPtrT OSALTaskCreate(OSALTaskFuncPtrT pTaskFunc, void *const pParam,
                            const OSALPrioT prio,
                            const OSALTaskStructT *const pTaskParams) {

  LOG_ASSERT(pTaskParams);
  OSALTask *pOsalTask = new OSALTask(pTaskFunc, pParam, pTaskParams, prio);
  return pOsalTask;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALTaskDelete(OSALTaskPtrT *const ppTaskPtr) {
  OSALTask **ppTaskSlot = (OSALTask **)ppTaskPtr;
  LOG_ASSERT(ppTaskSlot);
  OSALTask *pTaskSlot = *ppTaskSlot;
  LOG_ASSERT(pTaskSlot);
  pTaskSlot->check();

  delete pTaskSlot;

  *ppTaskPtr = NULL;

  return true;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t OSALGetCurrentTaskID(void) {
  uint32_t rval = 0;
  if (0x99999999 == mInitializedTag) {
    pthread_t pt = pthread_self();
    auto &idmap = OSAL::inst().osal_IdMap;
    const OSAL::IdMapT::iterator f = idmap.find(pt);
    if (f != idmap.end()) {
      rval = f->second;
    }
  }
  return rval;

}

} // Extern "C"
#endif // ANDROID

#endif
