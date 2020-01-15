#include "osal/osal.h"

#ifdef WIN32
//#define EM_WIN32
#endif


#if defined(__EMSCRIPTEN__)
#include "utils/platform_log.h"
#ifndef EM_WIN32
#include <emscripten.h>
#else
#include <windows.h>
#endif
#include <string.h>
static uint32_t dummyUInt = 0;

extern "C" {

// ////////////////////////////////////////////////////////////////////////////
// Initialize the OSAL
void OSALInit(void){
}

// ////////////////////////////////////////////////////////////////////////////
// Enter critical section (disable task scheduler AND interrupts.)
void OSALEnterCritical(void){
}

// ////////////////////////////////////////////////////////////////////////////
// Disable critical section (disable task scheduler AND interrupts.)
void OSALExitCritical(void){
}

// ////////////////////////////////////////////////////////////////////////////
// Enter critical section (disable task scheduler)
void OSALEnterTaskCritical(void) {  }

// ////////////////////////////////////////////////////////////////////////////
// Enable critical section (disable task scheduler AND interrupts.)
void OSALExitTaskCritical(void) {  }

// ////////////////////////////////////////////////////////////////////////////
// Get the millisecond counter.
uint32_t OSALGetMS(void){
#ifndef EM_WIN32
	const int x = EM_ASM_INT({
	  return Date.now();
	}, 0);
	return (uint32_t)x;
#else
  return GetTickCount();
#endif
}

// ////////////////////////////////////////////////////////////////////////////
// Sleep for ms milliseconds.
void OSALSleep(const uint32_t ms){
}

// ////////////////////////////////////////////////////////////////////////////
// Create a mutex (with lock/unlock capability)
OSALMutexPtrT OSALCreateMutex(void){
  return (void *)&dummyUInt;
}

// ////////////////////////////////////////////////////////////////////////////
// Delete a mutex (with lock/unlock capability)
void OSALDeleteMutex(OSALMutexPtrT *ppPortMutex){
  if (*ppPortMutex){
    *ppPortMutex = (void *)0;
  }
}

// ////////////////////////////////////////////////////////////////////////////
// Lock a mutex.  Returns false if locking failed,
bool OSALLockMutex(OSALMutexPtrT const pPortMutex, const uint32_t timeoutMs){
  return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Unlock a mutex.
bool OSALUnlockMutex(OSALMutexPtrT const pPortMutex){
  return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Create a counting semaphore.
OSALSemaphorePtrT OSALSemaphoreCreate(const uint32_t initialValue,
                                      const uint32_t maxValue){

  return (void *)&dummyUInt;
}

// ////////////////////////////////////////////////////////////////////////////
// Delete a counting semaphore.
bool OSALSemaphoreDelete(OSALSemaphorePtrT *const ppSemaphore){
  if (ppSemaphore){
    *ppSemaphore = (void *)NULL;
  }
  return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Signal a counting semaphore.
bool OSALSemaphoreSignal(OSALSemaphorePtrT const pPortSem,
                         const uint32_t count){
  return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Wait for a counting semaphore.
bool OSALSemaphoreWait(OSALSemaphorePtrT const pPortSem,
                       const uint32_t timeoutMs){
  return true;
}

OSALTaskPtrT OSALTaskCreate(OSALTaskFuncPtrT pTaskFunc, void *const pParam,
                            const OSALPrioT prio,
                            const OSALTaskStructT *const pPlatform){
  return NULL;
}

bool OSALTaskDelete(OSALTaskPtrT *const ppTaskPtr){
  return false;
}
}
#endif
