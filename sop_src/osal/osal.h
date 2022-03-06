/*
 * osal.h
 *
 *  Created on: Sep 23, 2020
 *      Author: Chris
 */

#ifndef APP_COMMON_SRC_MINICRYPTO_UTILS_SRC_OSAL_H_
#define APP_COMMON_SRC_MINICRYPTO_UTILS_SRC_OSAL_H_

#include "osal/mempools.h"
#include "osal/platform_type.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef OSAL_SINGLE_TASK
#if defined(CC26XXWARE) || defined(ANDROID) || \
  defined(__EMSCRIPTEN__) // || defined(TARGET_OS_IOS)
#define OSAL_SINGLE_TASK 1
#endif
#endif

#define OSALMALLOC(sz) MemPoolsMalloc(sz)
#define OSALFREE(p) MemPoolsFree(p)

#ifdef __cplusplus
extern "C" {
#endif

typedef void* OSALMutexPtrT;
typedef void* OSALSemaphorePtrT;
typedef void* OSALTaskPtrT;

typedef enum OSALPrioTag {
  OSAL_PRIO_CRITICAL = 1, ///< Use only for hardware interfaces (uart, etc.)
  OSAL_PRIO_HIGH, ///< High priority interface (audio, etc.)
  OSAL_PRIO_MEDIUM_PLUS, ///< iAnywhere stack priority
  OSAL_PRIO_MEDIUM,      ///< Medium priority (eventq)
  OSAL_PRIO_LOW,         ///< Low priority (worker)
  OSAL_PRIO_BACKGND      ///< Idle priority

} OSALPrioT;

#define OSAL_WAIT_INFINITE 0xffffffff
#define OSAL_COUNT_INFINITE 0x7fffffff

// ////////////////////////////////////////////////////////////////////////////
// Initialize the OSAL
void OSALInit(void);

// ////////////////////////////////////////////////////////////////////////////
// Enter critical section (disable task scheduler AND interrupts.)
void OSALEnterCritical(void);

// ////////////////////////////////////////////////////////////////////////////
// Disable critical section (disable task scheduler AND interrupts.)
void OSALExitCritical(void);

// ////////////////////////////////////////////////////////////////////////////
// Disable critical section (disable task scheduler)
// void OSALEnterTaskCritical(void)
void OSALEnterTaskCritical(void);

// ////////////////////////////////////////////////////////////////////////////
// Disable critical section (disable task scheduler)
// void OSALExitTaskCritical(void)
void OSALExitTaskCritical(void);

#if !defined(__FREERTOS__) && !defined(ccs)
#else

// Startup of FREERTOS occurs in a small static buffer. After startup, this switches to mempools.
extern void OSALFreertosSwitchToMempools(const bool pakmkey_PowerIsEnabled);

#endif

// ////////////////////////////////////////////////////////////////////////////
// Get the millisecond counter.
uint32_t OSALGetMS(void);

// ////////////////////////////////////////////////////////////////////////////
// Sleep for ms milliseconds.
void OSALSleep(const uint32_t ms);

// ////////////////////////////////////////////////////////////////////////////
// For testbenches, unhooking from hardware will make the OSAL go into simulated mode.
void OSALMSHookToHardware(const bool hookToHardware);

// ////////////////////////////////////////////////////////////////////////////
// Create a mutex (with lock/unlock capability)
OSALMutexPtrT OSALCreateMutex(void);

// ////////////////////////////////////////////////////////////////////////////
// Delete a mutex (with lock/unlock capability)
void OSALDeleteMutex(OSALMutexPtrT* ppPortMutex);

// ////////////////////////////////////////////////////////////////////////////
// Lock a mutex.  Returns false if locking failed,
#ifdef __cplusplus
bool OSALLockMutex(OSALMutexPtrT const pPortMutex, const uint32_t timeoutMs = OSAL_WAIT_INFINITE);
#else
bool OSALLockMutex(OSALMutexPtrT const pPortMutex, const uint32_t timeoutMs);
#endif

// ////////////////////////////////////////////////////////////////////////////
// Unlock a mutex.
bool OSALUnlockMutex(OSALMutexPtrT const pPortMutex);

// ////////////////////////////////////////////////////////////////////////////
// Create a counting semaphore.
// initialValue and maxValue <= OSAL_COUNT_INFINITE
OSALSemaphorePtrT
  OSALSemaphoreCreate(const uint32_t initialValue, const uint32_t maxValue);

// ////////////////////////////////////////////////////////////////////////////
// Delete a counting semaphore.
bool OSALSemaphoreDelete(OSALSemaphorePtrT* const ppSemaphore);

// ////////////////////////////////////////////////////////////////////////////
// Signal a counting semaphore.
bool OSALSemaphoreSignal(OSALSemaphorePtrT const pPortSem, const uint32_t count);

// ////////////////////////////////////////////////////////////////////////////
// Signal a counting semaphore from the ISR
bool OSALSemaphoreSignalFromIsr(OSALSemaphorePtrT const pPortSem, const uint32_t count);

// ////////////////////////////////////////////////////////////////////////////
// Wait for a counting semaphore.
bool OSALSemaphoreWait(OSALSemaphorePtrT const pPortSem, const uint32_t timeoutMs);


// ////////////////////////////////////////////////////////////////////////////
void OSALRandomInit(const char* const szName, const int len);

// ////////////////////////////////////////////////////////////////////////////
// Generate random data.
void OSALRandom(uint8_t* const pBytes, const uint32_t len);

// ////////////////////////////////////////////////////////////////////////////
// Sets an initial seed from command line.
void OSALRandomSeed(const uint8_t* const pEntropy, const uint32_t len);

// ////////////////////////////////////////////////////////////////////////////
// Adds some entropy.
// Can be a basic string, because the OSAL will create entropy from this source.
void OSALRandomWriteEntropyText(const void* const pEntropy, const uint32_t len);

typedef struct OSALTaskStructTag {
  char* pStack; /* Unused for WIN32 */
  uint32_t stackSize;
  uint32_t taskId;
} OSALTaskStructT;

// ////////////////////////////////////////////////////////////////////////////
// For statically declaring a stack.  Use for real application code!
#define OSAL_INSTANTIATE_STACK(name, size, taskId) \
  static char name##_stack[ size ];                \
  static const OSALTaskStructT name = { name##_stack, (size), taskId }

// ////////////////////////////////////////////////////////////////////////////
// For dynamically allocating a stack from within a function
// For use by test case code only!
#define OSAL_ALLOC_STACK(name, size, taskId)            \
  char* name##_stack         = (char*)OSALMALLOC(size); \
  const OSALTaskStructT name = { name##_stack, size, taskId }

// ////////////////////////////////////////////////////////////////////////////
// For dynamically freeing a stack from within a function.
// For use by test case code only!
#define OSAL_FREE_STACK(name) OSALFREE(name##_stack)

// ////////////////////////////////////////////////////////////////////////////
// Function type for OSAL tasks.
typedef void (*OSALTaskFuncPtrT)(void* const pParam);

// ////////////////////////////////////////////////////////////////////////////
// Creates a new task
OSALTaskPtrT OSALTaskCreate(
  OSALTaskFuncPtrT pTaskFunc,
  void* const pParam,
  const OSALPrioT prio,
  const OSALTaskStructT* const pPlatform);

// ////////////////////////////////////////////////////////////////////////////
// Deletes a task.
bool OSALTaskDelete(OSALTaskPtrT* const ppTaskPtr);

// ////////////////////////////////////////////////////////////////////////////
// Gets the current task ID.  (Task ID is set in OSALTaskStructT)
uint32_t OSALGetCurrentTaskID(void);

// ////////////////////////////////////////////////////////////////////////////
// Returns TRUE if the boolean was FALSE and is now TRUE.
bool OSALTestAndSet(volatile bool* const pBoolToTestAndSet);

// ////////////////////////////////////////////////////////////////////////////
void OSALClearFlag(bool* const pBoolToClear);

#if (PLATFORM_EMBEDDED > 0)
// ////////////////////////////////////////////////////////////////////////////
// Sets or clears the first test point on the hardware.
void OSALTP0(const int state);

// ////////////////////////////////////////////////////////////////////////////
// Sets or clears the second test point on the hardware.
void OSALTP1(const int state);

// ////////////////////////////////////////////////////////////////////////////
// Resets the CPU
void OSALSoftReset(void);

#endif

#ifdef __cplusplus
}
#endif


#endif /* APP_COMMON_SRC_MINICRYPTO_UTILS_SRC_OSAL_H_ */
