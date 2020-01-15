/*
 * osal.cpp
 *
 *  Created on: Sep 22, 2020
 *      Author: Chris
 */

#if defined(TARGET_IS_MSP432P4XX)

#include "platform/osal.h"
#include "utils/platform_log.h"
#include "utils/ble_utils.h"
#include "utils/helper_macros.h"
#include <stdint.h>
#include "mempools.h"
#include <string.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Error.h>

// LOG_MODNAME("osal_tirtos.cpp");

extern "C" {

#define MAX_NESTED_DISABLES 8
static volatile xdc_UInt hwiDisableKeys[MAX_NESTED_DISABLES];
static volatile int hwiDisableCnt = 0;

#define TIRTOS_OBJHDR 0x5432abcd

class msp_OsalBase {
protected:
    uint32_t mHdr;
public:
    bool check(void){
        const bool rval = (TIRTOS_OBJHDR == mHdr);
        LOG_ASSERT(rval);
        return rval;
    }

    msp_OsalBase(): mHdr(TIRTOS_OBJHDR){

    }
    virtual ~msp_OsalBase(){
        check();
        mHdr = ~TIRTOS_OBJHDR;
    }
};


static int entropy_gen( void *data, unsigned char *x, size_t xlen );

OSALEntropyFuncPtrT osal_getEntropyFunc(void) {
    return entropy_gen;
}
void * osal_getEntropyData(void){
    return NULL;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
static const char tirtos[] = "osal_tirtos";
void OSALInit(void){
    static bool init = false;
    if (!init){
        init = true;
        MemPoolsInit();
        //OSALRandomInit(tirtos, strlen(tirtos) );
    }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALEnterCritical(void) {
  LOG_ASSERT(hwiDisableCnt < MAX_NESTED_DISABLES);
  hwiDisableKeys[hwiDisableCnt] = Hwi_disable();
  // Interrupt is disabled, so changing it is safe to do.
  hwiDisableCnt++;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALExitCritical(void) {
  // Interrupt is disabled, so changing it is safe to do.
  hwiDisableCnt -= 1;
  const int disableCnt = hwiDisableCnt;
  Hwi_restore(hwiDisableKeys[hwiDisableCnt]);
  LOG_ASSERT(disableCnt >= 0);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t OSALGetMS(void) { return Clock_getTicks(); }

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALSleep(const uint32_t ms) { Task_sleep(ms); }

// //////////////////////
// We need a special class for the mutex because
// tirtos doesn't support recursive mutexes by default.
class osal_Mutex : public msp_OsalBase {
private:
  int               mCnt;
  Error_Block       mEb;
  Task_Handle       mCurrentOwner;
  bool              mIsOwned;
  Semaphore_Handle  mHandle;

public:
  // //////////////////////
  // Constructor.
  osal_Mutex() : msp_OsalBase(), mCnt(0), mCurrentOwner(NULL), mIsOwned(false) {
    Error_init(&mEb);
    
    Semaphore_Params mutexParams;
    Semaphore_Params_init(&mutexParams);
    mutexParams.mode = ti_sysbios_knl_Semaphore_Mode_BINARY;

    mHandle = Semaphore_create(1, &mutexParams, &mEb);

    check();
  }

  // //////////////////////
  // Destructor
  ~osal_Mutex() {
    check();
    Semaphore_delete(&mHandle);
  }


  // //////////////////////
  // Lock the mutex
  bool lock(const uint32_t timeoutMs) {
    check();
    xdc_Bool iOwnIt = false;
    Task_Handle me = Task_self();
    if (mIsOwned) {
      iOwnIt = (me == mCurrentOwner) ? true : false;
    }
    if (!iOwnIt) {
      if (timeoutMs == OSAL_WAIT_INFINITE) {
        iOwnIt = Semaphore_pend(mHandle, BIOS_WAIT_FOREVER);
      } else {
        iOwnIt = Semaphore_pend(mHandle, timeoutMs);
      }
    }
    if (iOwnIt) {
      mIsOwned = true;
      mCurrentOwner = me;
      mCnt++;
    }
    return iOwnIt;
  }

  // //////////////////////
  // Unlock the mutex.  Note: The calling function must actually OWN the mutex!!
  bool unlock() {
    bool rval = false;
    check();
    LOG_ASSERT(mIsOwned);
    Task_Handle me = Task_self();
    LOG_ASSERT(me == mCurrentOwner);
    if (me == mCurrentOwner) {
      --mCnt;
      LOG_ASSERT(mCnt >= 0);
      if (mCnt == 0) {
        mIsOwned = false;
        mCurrentOwner = NULL;
        Semaphore_post(mHandle);
      }
      rval = true;
    }
    return rval;
  }
};

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void *OSALCreateMutex() {
  osal_Mutex *pMutex = new osal_Mutex();
  return (void *)pMutex;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALDeleteMutex(void **ppPortMutex) {
  LOG_ASSERT((ppPortMutex) && (*ppPortMutex));
  osal_Mutex **ppMutex = (osal_Mutex **)ppPortMutex;
  osal_Mutex *pMutex = *ppMutex;
  delete pMutex;
  *ppPortMutex = NULL;
} // Acorn!

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// Note, in TIRTOS, recursive mutexes are supported by "gates"
// See
// http://downloads.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/bios/sysbios/6_46_00_23/exports/bios_6_46_00_23/docs/Bios_User_Guide.pdf
// See https://e2e.ti.com/support/embedded/tirtos/f/355/t/542513
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALLockMutex(void *const pPortMutex, const uint32_t timeoutMs) {
  osal_Mutex *pMutex = (osal_Mutex *)pPortMutex;
  LOG_ASSERT((pPortMutex));
  return pMutex->lock(timeoutMs);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALUnlockMutex(void *const pPortMutex) {
  osal_Mutex *pMutex = (osal_Mutex *)pPortMutex;
  LOG_ASSERT((pPortMutex));
  return pMutex->unlock();
}

// //////////////////////
// We need a special class for the mutex because
// tirtos doesn't support recursive mutexes by default.
class osal_CntSem : public msp_OsalBase {
private:
  Error_Block       mEb;
  Semaphore_Handle  mHandle;
  int32_t mMaxCnt;

public:
  // //////////////////////
  // Constructor.
  osal_CntSem(const int32_t initValue, const int32_t maxValue)
    : msp_OsalBase()
    , mMaxCnt(maxValue)
{
    Error_init(&mEb);

    Semaphore_Params cntSemParams;
    Semaphore_Params_init(&cntSemParams);
    cntSemParams.mode = ti_sysbios_knl_Semaphore_Mode_COUNTING;

    mHandle = Semaphore_create(initValue, &cntSemParams, &mEb);
    check();
  }

  // //////////////////////
  // Destructor
  ~osal_CntSem() {
    check();
    Semaphore_delete(&mHandle);
  }


  // //////////////////////
  // Wiat on the semaphore
  bool wait(const uint32_t timeoutMs) {
    xdc_Bool iOwnIt = false;
    check();
    if (timeoutMs == OSAL_WAIT_INFINITE) {
      iOwnIt = Semaphore_pend(mHandle, BIOS_WAIT_FOREVER);
    } else {
      iOwnIt = Semaphore_pend(mHandle, timeoutMs);
    }
    return iOwnIt;
  }

  // //////////////////////
  // Signal the counting semaphore.
  bool signal(const int inc) {
    bool rval = check();
    if (rval){
        if (mMaxCnt != OSAL_COUNT_INFINITE){
            OSALEnterCritical();
            int cnt = Semaphore_getCount(mHandle);
            const int finalCnt = cnt + inc;
            const int endCnt = MIN(finalCnt, mMaxCnt);
            OSALExitCritical();
            for (; cnt < endCnt; cnt++) {
                Semaphore_post(mHandle);
            }
            rval = (endCnt == finalCnt) ? true : false;
        }
        else {
            for (int i = 0; i < inc; i++){
                Semaphore_post(mHandle);
            }
            rval = true;
        }
    }
    return rval;
  }
};

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
OSALSemaphorePtrT OSALSemaphoreCreate(const uint32_t initialValue,
                                      const uint32_t maxValue) {
  osal_CntSem *pSem = new osal_CntSem(initialValue, maxValue);
  return (OSALSemaphorePtrT)pSem;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALSemaphoreDelete(OSALSemaphorePtrT *const ppSemaphore) {
  bool rval = false;
  LOG_ASSERT((ppSemaphore) && (*ppSemaphore));
  if ((ppSemaphore) && (*ppSemaphore)){
      osal_CntSem **ppSem = (osal_CntSem **)ppSemaphore;
      osal_CntSem *pSem = *ppSem;
      delete pSem;
      *ppSemaphore = NULL;
      rval = true;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALSemaphoreWait(OSALSemaphorePtrT const pPortSem,
                       const uint32_t timeoutMs) {
  osal_CntSem *pSem = (osal_CntSem *)pPortSem;
  LOG_ASSERT((pPortSem));
  return pSem->wait(timeoutMs);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALSemaphoreSignal(OSALSemaphorePtrT const pPortSem,
                         const uint32_t count) {
  osal_CntSem * const pSem = (osal_CntSem *)pPortSem;
  LOG_ASSERT((pSem));
  return pSem->signal(count);
}

// ////////////////////////////////////////////////////////////////////////////
// Signal a counting semaphore from the ISR
bool OSALSemaphoreSignalFromIsr(OSALSemaphorePtrT const pPortSem, const uint32_t count) {
  return OSALSemaphoreSignal(pPortSem, count);
}


// Prio map
static const int osalPrioMap[] = {
    0,
    5, // OSAL_PRIO_CRITICAL
    4, // OSAL_PRIO_HIGH
    3, // OSAL_PRIO_IA
    2, // OSAL_PRIO_MEDIUM
    1, // OSAL_PRIO_LOW
    1  // OSAL_PRIO_BACKGND
};

class osal_Task : public msp_OsalBase{
  Task_Handle       mHandle;
  Error_Block       mEb;

  // ////////////////////////////////////////////////////////////////////////////////////////////////
  // Internal function just calls the application's task function
  static void osalTaskFxn(UArg a0, UArg a1) {
    osal_Task *pTask = (osal_Task *)(void *)a0;
    pTask->check();
    pTask->mFnPtr(pTask->mParamPtr);
  }


public:

  OSALTaskFuncPtrT  mFnPtr;
  void             *mParamPtr;
    
  // //////////////////////
  osal_Task(
      OSALTaskFuncPtrT pFn, 
      void *pParam,
      const OSALTaskStructT *const pOsalTaskParams,
      const OSALPrioT prio)
      : msp_OsalBase(), mFnPtr(pFn), mParamPtr(pParam) {
    
    Task_Params task_params;
    Task_Params_init(&task_params);

    Error_init(&mEb);

    COMPILE_TIME_ASSERT(sizeof(task_params.arg0) == sizeof(void *));
    task_params.arg0 = (uintptr_t)(void *)this;
    task_params.stack = pOsalTaskParams->pStack;
    task_params.stackSize = pOsalTaskParams->stackSize;
    task_params.priority = osalPrioMap[prio];

    mHandle = ti_sysbios_knl_Task_create(
        osalTaskFxn, &task_params, &mEb);
  }


  // //////////////////////
  ~osal_Task() {
    check();
    Task_delete(&mHandle);

  }
};



// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
OSALTaskPtrT OSALTaskCreate(OSALTaskFuncPtrT pTaskFunc, void *const pParam,
                            const OSALPrioT prio,
                            const OSALTaskStructT *const pTaskParams) {
  LOG_ASSERT(pTaskParams);
  LOG_ASSERT(pTaskParams->pStack);
  LOG_ASSERT(pTaskParams->stackSize);
  osal_Task *pOsalTask = new osal_Task(pTaskFunc, pParam, pTaskParams, prio);
  return pOsalTask;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALTaskDelete(OSALTaskPtrT *const ppTaskPtr) {
  osal_Task **ppTaskSlot = (osal_Task **)ppTaskPtr;
  LOG_ASSERT(ppTaskSlot);
  osal_Task *pTaskSlot = *ppTaskSlot;
  LOG_ASSERT(pTaskSlot);
  pTaskSlot->check();

  delete pTaskSlot;

  *ppTaskPtr = NULL;

  return true;
}

#ifndef PAK_ECU_HAS_NO_SAP
// This uses the CC2650 to generate real random numbers using the built in HW RNG.
#include <ti/sysbios/knl/Task.h>
#include <ti/sap/sap.h>

#else

#include <stdlib.h>
// THis should almost never be used.
// For now, fake it till we make it.  This really sucks, but we'll replace it SOON!
uint32_t SAP_getRand(void){
    const uint32_t rand1 = 0xffff & rand();
    const uint32_t rand2 = 0xffff & rand();
    return (rand1 << 16) | (rand2);
}
#endif
static int entropy_gen( void *data, unsigned char *x, size_t xlen ){
    unsigned char *px = x;
    uint32_t const bytes32 = (uint32_t)xlen;
    const uint32_t words = bytes32 / sizeof(uint32_t);
    for (uint32_t w = 0; w < words; w++) {
        const uint32_t rnd32 = SAP_getRand();
        px[0] = (rnd32 >> 24) & 0xff;
        px[1] = (rnd32 >> 16) & 0xff;
        px[2] = (rnd32 >> 8) & 0xff;
        px[3] = (rnd32 >> 0) & 0xff;
        px += 4;
    }

    uint32_t bytesRemaining = bytes32 - (words*4);
    if (bytesRemaining > 0) {
        LOG_ASSERT(bytesRemaining <= 3);
        int shifts = 24;
        const uint32_t rnd32 = SAP_getRand();
        while (bytesRemaining > 0) {
            *px = (rnd32 >> shifts) & 0xff;
            px++;
            shifts -= 8;
            bytesRemaining--;
        }
    }
    return 0;
}

} // Extern "C"

#endif

#if defined(USE_ICALL)
// ICALL is the abstraction layer used when the TI-RTOS is in a "library" that
// resides in a parallel section of FLASH, and the Application is like a slave
// process in that library.  Used on CC2650.

#include "ICall.h"
#include "utils/ble_utils.h"
#include "utils/helper_macros.h"
#include <ti/sysbios/hal/Hwi.h>
#include "platform/osal.h"

extern "C" {
// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void * OSALCreateMutex() {
    ICall_Semaphore const pSem = ICall_createSemaphore(
            ICALL_SEMAPHORE_MODE_BINARY, 1);

    return (void *)pSem;
}



// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALDeleteMutex( void **ppPortMutex ) {
    // Looks like semaphores cannot be deleted... Strange!
    LOG_ASSERT((ppPortMutex) && (*ppPortMutex));
    *ppPortMutex = NULL;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALLockMutex( void * const pPortMutex, const uint32_t timeoutMs ) {
    LOG_ASSERT((pPortMutex));
    ICall_Semaphore const pSem = (ICall_Semaphore *)pPortMutex;
    ICall_Errno err = ICall_waitSemaphore(pSem, ICALL_TIMEOUT_FOREVER);
    LOG_ASSERT(err == ICALL_ERRNO_SUCCESS);
    return (err == ICALL_ERRNO_SUCCESS);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALUnlockMutex( void * const pPortMutex ) {
    ICall_Semaphore const pSem = (ICall_Semaphore *)pPortMutex;
    ICall_Errno err = ICall_signal(pSem);
    LOG_ASSERT(err == ICALL_ERRNO_SUCCESS);
    return (err == ICALL_ERRNO_SUCCESS);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t OSALGetMS(void) {
#if 0
    const uint32_t usPerTick = ICall_getTickPeriod();
    const uint32_t ticks = ICall_getTicks();

    const double fUsPerTick = (double)usPerTick;
    const double fTicks = (double)ticks;
    double fMs = fUsPerTick * fTicks / 1000.0;
    return (uint32_t)fMs;
#else
    const uint64_t usPerTick = ICall_getTickPeriod();
    const uint64_t ticks = ICall_getTicks();
    const uint64_t ms = (usPerTick * ticks)/1000;
    return (uint32_t)ms;
#endif
}


// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALSleep(const uint32_t ms) {
}


static int cstateIdx = 0;
static unsigned int csstateStack[8] = {0};

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALEnterCritical( void ) {
    unsigned int state = Hwi_disable();
    csstateStack[cstateIdx++] = state;
    LOG_ASSERT( cstateIdx != ARRSZ(csstateStack));
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALExitCritical( void ) {
    LOG_ASSERT(cstateIdx > 0);
    unsigned int state = csstateStack[--cstateIdx];
    Hwi_restore(state);
}

// ////////////////////////////////////////////////////////////////////////////
// Enter critical section (disable task scheduler)
void OSALEnterTaskCritical(void) { OSALEnterCritical() }

// ////////////////////////////////////////////////////////////////////////////
// Enable critical section (disable task scheduler AND interrupts.)
void OSALExitTaskCritical(void) { OSALExitCritical() }

typedef void * OSALSemaphorePtrT;
// ////////////////////////////////////////////////////////////////////////////
// Create a counting semaphore.
OSALSemaphorePtrT OSALSemaphoreCreate(const uint32_t initialValue,
                                      const uint32_t maxValue) {
    return NULL;
}

// ////////////////////////////////////////////////////////////////////////////
// Delete a counting semaphore.
bool OSALSemaphoreDelete(OSALSemaphorePtrT *const ppSemaphore) {
    return false;
}

// ////////////////////////////////////////////////////////////////////////////
// Signal a counting semaphore.
bool OSALSemaphoreSignal(OSALSemaphorePtrT const pPortSem,
                         const uint32_t count) {
    return false;
}

// ////////////////////////////////////////////////////////////////////////////
// Signal a counting semaphore from the ISR
bool OSALSemaphoreSignalFromIsr(OSALSemaphorePtrT const pPortSem, const uint32_t count) {
  return OSALSemaphoreSignal(pPortSem, count);
}


// ////////////////////////////////////////////////////////////////////////////
// Wait for a counting semaphore.
bool OSALSemaphoreWait(OSALSemaphorePtrT const pPortSem,
                       const uint32_t timeoutMs){
    return false;
}


typedef void * OSALTaskPtrT;

OSALTaskPtrT OSALTaskCreate(OSALTaskFuncPtrT pTaskFunc, void *const pParam,
                            const OSALPrioT prio,
                            const OSALTaskStructT *const pPlatform){
    return NULL;
}

bool OSALTaskDelete(OSALTaskPtrT *const ppTaskPtr){
    return false;
}


} // extern "C" {


#endif

