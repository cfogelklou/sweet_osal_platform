/*
 * osal.cpp
 *
 *  Created on: Sep 22, 2020
 *      Author: Chris
 */

#if defined(WIN32) && !defined(__FREERTOS__)

#include "osal/osal.h"
#include "utils/platform_log.h"
#include "utils/ble_utils.h"
#include "utils/helper_macros.h"
#include "osal/cs_locker.hpp"
#include <map>
#include <stdint.h>
#include <string.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>


#ifndef EOK
#define EOK 0
#endif

LOG_MODNAME("osal_win32.cpp");

class OSAL {
public:
  static OSAL &inst() {
    static OSAL theInst;
    return theInst;
  }

  void EnterCritical() {
    const DWORD currId = GetCurrentThreadId();
    if (currId != m_CSThreadId) {
      EnterCriticalSection(&m_CS);
      LOG_ASSERT_FN(1 == ++m_CSRecursion);
      LOG_ASSERT(m_CSThreadId == (DWORD)-1);
      m_CSThreadId = currId;
    }
    else {
      // Recursive call, we already own it.
      ++m_CSRecursion;
    }
  }

  void ExitCritical() {
    LOG_ASSERT(m_CSRecursion >= 1);
    if (0 == --m_CSRecursion) {
      const DWORD currId = GetCurrentThreadId();
      const DWORD owner = m_CSThreadId;
      m_CSThreadId = (DWORD)-1;
      LeaveCriticalSection(&m_CS);
      if (owner != (DWORD)-1) {
        LOG_ASSERT(owner == currId);
      }
      else {
        LOG_ASSERT(false);
      }
    }
  }

  uint32_t GetStartMS(void) { return m_startMs; }

  static bool mInitialized;
private:
  CRITICAL_SECTION m_CS;
public:
  DWORD m_CSThreadId;
private:
  int   m_CSRecursion;
  uint32_t m_startMs;

  OSAL() 
    :  m_CS()
    , m_CSThreadId((DWORD)-1)
    , m_startMs(0)
    , m_CSRecursion(0)
  {
    InitializeCriticalSection(&m_CS);
    m_startMs = OSALGetMS();
    mInitialized = true;
  }
  ~OSAL() {
	  //OSALDeleteMutex(&m_pMutex); 
    mInitialized = false;
  }
};

bool OSAL::mInitialized = false;

extern "C" {

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALEnterCritical(void) { OSAL::inst().EnterCritical(); }

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALExitCritical(void) { OSAL::inst().ExitCritical(); }

int osal_criticalTaskCount = 0;
// ////////////////////////////////////////////////////////////////////////////
// Disable critical section (disable task scheduler)
//void OSALEnterTaskCritical(void)
void OSALEnterTaskCritical(void) {
  OSAL::inst().EnterCritical();
  LOG_ASSERT_FN(++osal_criticalTaskCount < 100);
}

// ////////////////////////////////////////////////////////////////////////////
// Disable critical section (disable task scheduler)
//void OSALExitTaskCritical(void)
void OSALExitTaskCritical(void) {
  LOG_ASSERT_FN(--osal_criticalTaskCount >= 0);
  OSAL::inst().ExitCritical();

}

} // extern "C"


extern "C" {

#define HDRMUTEX 0x28a22a22
class OSALMutex {
public:
  uint32_t hdr;
  HANDLE hMutex;
  OSALMutex() 
    : hdr(HDRMUTEX)
    , hMutex(CreateMutex(NULL, FALSE, NULL))
  {
    hdr = HDRMUTEX;
    check();
  }

  bool check() {
    LOG_ASSERT(hdr == HDRMUTEX);
    return (hdr == HDRMUTEX);
  }

  ~OSALMutex() {
    check();
    hdr = ~HDRMUTEX;
  }
};

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void *OSALCreateMutex() {
  OSALMutex *pMutex = new OSALMutex();
  LOG_ASSERT(pMutex != NULL);
  if (pMutex != NULL) {
    LOG_ASSERT(pMutex->hMutex != INVALID_HANDLE_VALUE);
    if (pMutex->hMutex == INVALID_HANDLE_VALUE) {
      delete pMutex;
      pMutex = NULL;
    }
  }
  return pMutex;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALDeleteMutex(void **ppPortMutex) {

  OSALMutex **ppMutex = (OSALMutex **)ppPortMutex;
  OSALMutex *pMutex = *ppMutex;
  LOG_ASSERT(pMutex != NULL);

  if (pMutex != NULL) {
    LOG_ASSERT(pMutex->check());
    const HANDLE hMutex = pMutex->hMutex;
    delete pMutex;
    LOG_ASSERT_WARN_FN(CloseHandle(hMutex));
    *ppMutex = NULL;
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALLockMutex(void *const pPortMutex, const uint32_t timeoutMs) {
  bool rval = false;
  OSALMutex *pMutex = (OSALMutex * const)pPortMutex;

  // Do not lock from within a CS!
  LOG_ASSERT(GetCurrentThreadId() != OSAL::inst().m_CSThreadId);

  LOG_ASSERT(pMutex != NULL);
  LOG_ASSERT(pMutex->check());

  if (timeoutMs == OSAL_WAIT_INFINITE) {
    // If no timeout specified, then just do a normal wait
    const DWORD stat = WaitForSingleObject(pMutex->hMutex, INFINITE);
    rval = ((stat == WAIT_OBJECT_0) || (stat == WAIT_ABANDONED));
  } else {
    const DWORD stat = WaitForSingleObject(pMutex->hMutex, timeoutMs);
    rval = ((stat == WAIT_OBJECT_0) || (stat == WAIT_ABANDONED));
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALUnlockMutex(void *const pPortMutex) {
  OSALMutex *const pMutex = (OSALMutex * const)pPortMutex;
  LOG_ASSERT(pMutex != NULL);
  LOG_ASSERT(pMutex->check());
  return (TRUE == ReleaseMutex(pMutex->hMutex));
}

#define WINSEMHDR 0x97386452
class OSALWinSem {
public:
  uint32_t hdr;
  HANDLE hSem;

  OSALWinSem(
    const uint32_t initialValue,
    const uint32_t maxValue)
    : hdr(WINSEMHDR)
    , hSem(CreateSemaphore(NULL, initialValue, maxValue, NULL))
  { }

  void check() { LOG_ASSERT(WINSEMHDR == hdr); }

  ~OSALWinSem() { LOG_ASSERT(WINSEMHDR == hdr); }

private:
  OSALWinSem()
    : hdr(WINSEMHDR)
    , hSem(CreateSemaphore(NULL, 0, 0, NULL))
  {}
};

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
OSALSemaphorePtrT OSALSemaphoreCreate(const uint32_t initialValue,
                                      const uint32_t maxValue) {
  void *pRval = NULL;

  OSALWinSem *const pSem = new OSALWinSem(initialValue, maxValue);
  pSem->check();

  // initialize semaphore
  if (pSem->hSem == INVALID_HANDLE_VALUE) {
    // Failed, free memory and exit.
    LOG_ASSERT(0);
    delete pSem;
  } else {
    pRval = pSem;
    LOG_ASSERT_WARN(pRval);
  }

  return (OSALSemaphorePtrT)pRval;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALSemaphoreDelete(OSALSemaphorePtrT *const ppSemaphore) {
  bool rval = false;
  OSALWinSem **ppSem = (OSALWinSem **)ppSemaphore;
  LOG_ASSERT(ppSem);
  OSALWinSem *const pSem = *ppSem;
  LOG_ASSERT(pSem);

  pSem->check();

  if (pSem != NULL) {
    const HANDLE hSem = pSem->hSem;
    delete pSem;
    rval = (TRUE == CloseHandle(hSem));
    LOG_ASSERT(rval);
  }

  *ppSem = NULL;
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALSemaphoreSignal(OSALSemaphorePtrT const pPortSem,
                         const uint32_t count) {
  bool rval = FALSE;
  OSALWinSem *const pSem = (OSALWinSem *)pPortSem;

  pSem->check();

  if (pSem != NULL) {
    // Failure to release could mean the max count is reached
    rval = (TRUE == ReleaseSemaphore(pSem->hSem, count, NULL));
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
  bool rval = FALSE;
  OSALWinSem *const pSem = (OSALWinSem *)pPortSem;

  pSem->check();

  if (pSem != NULL) {
    DWORD stat = 0;
    if (timeoutMs == OSAL_WAIT_INFINITE) {
      // If no timeout specified, then just do a normal wait
      stat = WaitForSingleObject(pSem->hSem, INFINITE);
    } else {
      stat = WaitForSingleObject(pSem->hSem, timeoutMs);
    }
    if ((stat == WAIT_OBJECT_0) || (stat == WAIT_ABANDONED)) {
      rval = TRUE;
    }
  }
  return rval;
}

class TicksGetter{
public:
  
  static TicksGetter &inst() {
    static TicksGetter inst;
    return inst;
  }

  uint64_t getTicks() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart - mStartTime;
  }

  uint64_t getMs(const bool ignoreHookedToHardware = false) {
    if ((ignoreHookedToHardware) || (!mSimulated)) {
      return (1000 * getTicks()) / mTicksPerSec;
    }
    else {
      return mEmulatedHwTicks;
    }
  }

  void DoHookToHardware(bool hookToHardware) {
    const bool simulated = !hookToHardware;
    if (simulated != mSimulated) {

      mSimulated = simulated;
      mThreadId++;

      if (mSimulated) {
        mEmulatedHwTicks = getMs(true);
        mhFakeTicksThread = CreateThread(
          NULL, 32000,
          ticksTaskC, (void *)(uintptr_t)mThreadId, 0,
          &mDwFakeTicksThread);
      }
    }
  }

private:

  void ticksTask(const uint32_t myId) {
    uint32_t ms = getMs(true);
    while (myId == mThreadId) {
      Sleep(10);
      uint32_t ms2 = getMs(true);
      int diff = ms2 - ms;
      if (diff >= 10) {
        mEmulatedHwTicks += 10;
      }
      ms = ms2;
    }
  }

  static DWORD WINAPI ticksTaskC(LPVOID p) {
    const uint32_t myCode = (uint32_t)(uintptr_t)p;
    TicksGetter::inst().ticksTask(myCode);
    return 0;
  }

  TicksGetter() 
    : mSimulated(false)
    , mhFakeTicksThread( INVALID_HANDLE_VALUE )
    , mThreadId(0)
    , mDwFakeTicksThread(0)
    , mEmulatedHwTicks(0)
  {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    mTicksPerSec = freq.QuadPart;

    QueryPerformanceCounter(&freq);
    mStartTime = freq.QuadPart;

  }

  uint64_t mTicksPerSec;
  uint64_t mStartTime;
  volatile bool     mSimulated;
  HANDLE   mhFakeTicksThread;
  DWORD    mDwFakeTicksThread;
  volatile uint32_t mThreadId;
  volatile uint32_t mEmulatedHwTicks;

};


// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALMSHookToHardware(const bool hookToHardware) {
  TicksGetter::inst().DoHookToHardware(hookToHardware);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t OSALGetMS(void) { 
  return (uint32_t)TicksGetter::inst().getMs();
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALSleep(const uint32_t ms) {

  uint32_t currTime = OSALGetMS();
  const uint32_t expiryTime = currTime + ms;
  int32_t remaining = expiryTime - currTime;
  // Do loop so a call to OSALSleep always results in at least one sleep
  do {
    const auto t = MAX(1, remaining); // Don't call sleep(0) ever.
    Sleep(t);
    remaining = expiryTime - OSALGetMS();
  } while (remaining > 0);
}

#define OSALTASKHDR 0x93716AAA

} // extern C

class OSALTask {
public:
  typedef std::map<DWORD, uint32_t> IdMapT;

  uint32_t hdr;
  void *pParam;
  OSALTaskFuncPtrT pFn;
  HANDLE hThread;
  DWORD dwThread;
  uint32_t taskId;
  static IdMapT osal_IdMap;

  OSALTask(OSALTaskFuncPtrT pFn, void *pParam, const uint32_t taskId)
    : hdr(OSALTASKHDR)
    , pParam(pParam)
    , pFn(pFn)
    , hThread(INVALID_HANDLE_VALUE)
    , dwThread(0)
    , taskId(taskId)
  {
    check();
  }
  
  bool check() {
    LOG_ASSERT(hdr == OSALTASKHDR);
    return (hdr == OSALTASKHDR);
  }

  ~OSALTask() {
    LOG_ASSERT(hdr == OSALTASKHDR);
    hdr = ~OSALTASKHDR;
  }
  
  static DWORD WINAPI osal_ThreadTask(LPVOID p) {
    OSALTask *pTaskSlot = (OSALTask *)p;
    LOG_ASSERT((pTaskSlot != NULL) && (pTaskSlot->check()));
    const DWORD dw = GetCurrentThreadId();
    {
      CSLocker lock;
      const OSALTask::IdMapT::iterator f = OSALTask::osal_IdMap.find(dw);
      LOG_ASSERT(f == OSALTask::osal_IdMap.end());
      osal_IdMap[dw] = pTaskSlot->taskId;
    }
    
    pTaskSlot->pFn(pTaskSlot->pParam);

    return 0;
  }
};

OSALTask::IdMapT OSALTask::osal_IdMap;

// Prio map
static const int osalPrioMap[] = {
    0,
    THREAD_PRIORITY_TIME_CRITICAL, // OSAL_PRIO_CRITICAL
    THREAD_PRIORITY_HIGHEST,       // OSAL_PRIO_HIGH
    THREAD_PRIORITY_ABOVE_NORMAL,  // OSAL_PRIO_IA
    THREAD_PRIORITY_ABOVE_NORMAL,  // OSAL_PRIO_MEDIUM
    THREAD_PRIORITY_NORMAL,        // OSAL_PRIO_LOW
    THREAD_PRIORITY_IDLE           // OSAL_PRIO_BACKGND
};


extern "C" {

extern void MemPoolsInitialize();

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALInit(void) {
  static bool init = false;
  if (!init) {
    init = true;
    // Constructor
    MemPoolsInitialize();
    OSAL::inst();
    std::string str = "WIN32";
    //OSALRandomInit(str.c_str(), str.length());
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
OSALTaskPtrT OSALTaskCreate(OSALTaskFuncPtrT pTaskFunc, void *const pParam,
                            const OSALPrioT prio,
                            const OSALTaskStructT *const pPlatform) {
  OSALTask *pTaskSlot = new OSALTask(pTaskFunc, pParam, pPlatform->taskId);
  if (pTaskSlot) {

    // Start the thread...
    pTaskSlot->hThread = CreateThread(
      NULL, pPlatform->stackSize,
      OSALTask::osal_ThreadTask, pTaskSlot, 0,
      &pTaskSlot->dwThread);

    LOG_ASSERT(pTaskSlot->hThread != INVALID_HANDLE_VALUE);
    LOG_ASSERT(pTaskSlot->hThread != 0);
    if (pTaskSlot->hThread != 0) {
      SetThreadPriority(pTaskSlot->hThread, osalPrioMap[prio]);
    }
  }
  return pTaskSlot;
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

  HANDLE hThread = pTaskSlot->hThread;

  TerminateThread(hThread, ERROR_SUCCESS);
  CloseHandle(hThread);

  *ppTaskPtr = NULL;
  return true;
}


// ////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t OSALGetCurrentTaskID(void) {
  uint32_t rval = 0;
  if (OSAL::mInitialized) {
    const DWORD dw = GetCurrentThreadId();
    const OSALTask::IdMapT::iterator f = OSALTask::osal_IdMap.find(dw);
    if (f != OSALTask::osal_IdMap.end()) {
      rval = f->second;
    }
  }
  return rval;
}
} // extern "C"

#endif
