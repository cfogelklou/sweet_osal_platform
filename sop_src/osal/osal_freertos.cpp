#if defined(__FREERTOS__)

extern "C" {
// FreeRTOS
#include "portmacro.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "portable.h"
#include "semphr.h"
#include "task.h"
}

#include "platform/osal.h"

#include "mbedtls/myconfig.h"
// embedtls for entropy
#include "mbedtls/entropy.h"
#include "mempools.h"
#include "utils/dl_list.h"
#include "utils/platform_log.h"
#include "utils/helper_utils.h"
#include "utils/helper_macros.h"
#include "utils/buf_alloc.hpp"
#include "platform/cs_locker.hpp"
#include "utils/sl_list.hpp"
#include "libcrypto_al/libcrypto_al_impl.hpp"

#include <stdint.h>
#include <string.h>

typedef uintptr_t  addr_t;

LOG_MODNAME("osal_freertos.cpp");

typedef struct SemOrMutexTag {
  void *pSemOrMutex;
} SemOrMutexT;


extern "C" {

typedef struct {
  mbedtls_entropy_context entropy;
  DLL osalTaskList;
  bool stackGrowthUp;
} OsalT;

static OsalT osal;

static void osal_getStackGrowthDir2(int **pp2){
  int a = 2;
  *pp2 = &a;
}

static bool osal_getStackGrowthDir(){
  int * p2 = NULL;
  int b = 1;
  int *p1 = &b;
  osal_getStackGrowthDir2(&p2);
  ASSERT_AT_COMPILE_TIME(sizeof(addr_t) == sizeof(void *));
  addr_t a1 = (addr_t)p1;
  addr_t a2 = (addr_t)p2;
  bool up = (a2>a1) ? true : false;
  return up;

}


extern void MemPoolsInitialize();

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
static const char freertos[] = "osal_freertos";
void OSALInit(void) {

  static bool init = false;
  if (!init) {
    init = true;
    MemPoolsInitialize();
    LCAL_Init();
    DLL_Init( &osal.osalTaskList );
    osal.stackGrowthUp = osal_getStackGrowthDir();
    //OSALRandomInit(freertos, strlen(freertos));
  }
}


// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALEnterCritical(void) {
  portENTER_CRITICAL();
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALExitCritical(void) {
  portEXIT_CRITICAL();
}


// ////////////////////////////////////////////////////////////////////////////
// Disable critical section (disable task scheduler)
// void OSALEnterTaskCritical(void)
void OSALEnterTaskCritical(void) {
  vTaskSuspendAll();
}

// ////////////////////////////////////////////////////////////////////////////
// Disable critical section (disable task scheduler)
// void OSALExitTaskCritical(void)
void OSALExitTaskCritical(void) {
  xTaskResumeAll();
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t OSALGetMS(void) { return xTaskGetTickCount() * portTICK_RATE_MS; }

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALSleep(const uint32_t ms) {
  if (ms == 0){
    vTaskDelay(0);
  }
  else {
    uint32_t currTime = OSALGetMS();
    const uint32_t expiryTime = currTime + ms;
    int32_t remaining = expiryTime - currTime;
    while (remaining > 0){
      portTickType ticksToSleep = (portTickType)(MAX((int)1, (int)remaining / (int)portTICK_RATE_MS));
      vTaskDelay(ticksToSleep);
      currTime = OSALGetMS();
      remaining = expiryTime - currTime;
    }
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void *OSALCreateMutex() {
  SemOrMutexT *pMutex = (SemOrMutexT *)pvPortMalloc(sizeof(SemOrMutexT));

  LOG_ASSERT(pMutex);

  pMutex->pSemOrMutex = xSemaphoreCreateRecursiveMutex();

  LOG_ASSERT(pMutex->pSemOrMutex);

  return (OSALSemaphorePtrT)pMutex;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALDeleteMutex(void **ppPortMutex) {
  LOG_ASSERT(ppPortMutex);
  LOG_ASSERT(*ppPortMutex);
  SemOrMutexT **ppMutex = (SemOrMutexT **)ppPortMutex;
  SemOrMutexT *pMutex = *ppMutex;

  LOG_ASSERT(pMutex != NULL);
  if (pMutex != NULL) {
    vSemaphoreDelete(pMutex->pSemOrMutex);
    // vPortFree(pMutex);
    vPortFree(pMutex);
    *ppMutex = NULL;
  }

}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALLockMutex(void *const pPortMutex, const uint32_t timeoutMs) {
  SemOrMutexT *pMutex = (SemOrMutexT *)pPortMutex;
  LOG_ASSERT(pPortMutex);
  bool stat = false;
  if (timeoutMs != OSAL_WAIT_INFINITE) {
    stat = (pdTRUE == xSemaphoreTakeRecursive(pMutex->pSemOrMutex,
      timeoutMs / portTICK_RATE_MS));
  }
  else {    
    stat = (pdTRUE == xSemaphoreTakeRecursive(pMutex->pSemOrMutex,
      30000 / portTICK_RATE_MS));
    LOG_ASSERT(stat);
  }
  return stat;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALUnlockMutex(void *const pPortMutex) {
  SemOrMutexT *pMutex = (SemOrMutexT *)pPortMutex;
  LOG_ASSERT(pPortMutex);
  return pdTRUE == xSemaphoreGiveRecursive(pMutex->pSemOrMutex);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
OSALSemaphorePtrT OSALSemaphoreCreate(const uint32_t initialValue,
                                      const uint32_t maxValue) {
  SemOrMutexT *pSemaphore = NULL;

  if (pSemaphore == NULL) {
    pSemaphore = (SemOrMutexT *)pvPortMalloc(sizeof(SemOrMutexT));

    LOG_ASSERT(pSemaphore);

    pSemaphore->pSemOrMutex = xSemaphoreCreateCounting(maxValue, initialValue);
  }

  LOG_ASSERT(pSemaphore->pSemOrMutex);

  return (OSALSemaphorePtrT)pSemaphore;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALSemaphoreDelete(OSALSemaphorePtrT *const ppSemaphore) {
  bool rval = false;
  SemOrMutexT **const ppSem = (SemOrMutexT **)ppSemaphore;
  if (ppSem) {
    LOG_ASSERT(ppSem);
    SemOrMutexT *const pSem = *ppSem;
    LOG_ASSERT(pSem);
    if (pSem) {
      vSemaphoreDelete(pSem->pSemOrMutex);
      vPortFree(pSem);
      rval = true;
      *ppSem = NULL;
    }
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALSemaphoreWait(OSALSemaphorePtrT const pPortSem,
                       const uint32_t timeoutMs) {
  bool gotSem = false;
  SemOrMutexT *const pSem = (SemOrMutexT *)pPortSem;
  LOG_ASSERT(pSem);
  if (timeoutMs == OSAL_WAIT_INFINITE) {
    bool gotFail = false;
    while ((!gotSem) && (!gotFail)) {
      const portBASE_TYPE stat = xSemaphoreTake(
          pSem->pSemOrMutex, OSAL_WAIT_INFINITE / portTICK_RATE_MS);
      gotSem = (pdTRUE == stat);
      gotFail = (!gotSem) && (pdFALSE != stat);
      LOG_ASSERT_WARN(!gotFail);
    }
  } else {
    gotSem = (pdTRUE ==
              xSemaphoreTake(pSem->pSemOrMutex, timeoutMs / portTICK_RATE_MS));
  }
  return gotSem;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALSemaphoreSignal(OSALSemaphorePtrT const pPortSem,
                         const uint32_t count) {
  SemOrMutexT *const pSem = (SemOrMutexT *)pPortSem;
  LOG_ASSERT(pSem);
  bool rval = true;
  for (uint32_t i = 0; (rval) && (i < count); i++) {
    rval &= (pdTRUE == xSemaphoreGive(pSem->pSemOrMutex));
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
// Signal a counting semaphore from the ISR
bool OSALSemaphoreSignalFromIsr(OSALSemaphorePtrT const pPortSem, const uint32_t count) {
  SemOrMutexT *const pSem = (SemOrMutexT *)pPortSem;
  LOG_ASSERT(pSem);
  bool rval = true;
  for (uint32_t i = 0; (rval) && (i < count); i++) {
    rval &= (pdTRUE == xSemaphoreGiveFromISR(pSem->pSemOrMutex, NULL));
  }
  return rval;
}




typedef struct osalTaskTag { 
  xTaskHandle vCreatedtask; 
  StaticTask_t vTcb;
} osalTaskT;

// Prio map
static const portBASE_TYPE osalPrioMap[] = {
    0,
    10, // OSAL_PRIO_CRITICAL
    8, // OSAL_PRIO_HIGH
    6, // OSAL_PRIO_IA
    4, // OSAL_PRIO_MEDIUM
    2,  // OSAL_PRIO_LOW
    1   // OSAL_PRIO_BACKGND
};



typedef struct osal_NewTaskDataTag {
  DLLNode  listNode;
  osalTaskT *pTask;
  OSALTaskFuncPtrT pTaskFunc;
  void * pParam;
  uint32_t taskId;
  portSTACK_TYPE *pStack;
  uint32_t stackSize;
} osal_NewTaskDataT;

// ////////////////////////////////////////////////////////////////////////////////////////////////
static void OSALTaskCreateTaskCb(void *p) {
  osal_NewTaskDataT local;
  
  {
    // Copy and free the pointer.
    osal_NewTaskDataT * const pCbData = (osal_NewTaskDataT *)p;
    local = *pCbData;
    OSALFREE(pCbData);
  }
#ifdef WIN32
  // Fake the WIN32 static stack since in Windows, stack can't be specified.
  // We record the address of LOCAL and assume it's at the beginning of the stack.
  uint8_t * pEndOfStack8 = (uint8_t *)&local;
  pEndOfStack8 = &pEndOfStack8[sizeof(local)];
  local.pStack = (size_t *)&pEndOfStack8[-local.stackSize];
#endif

  DLL_NodeInit( &local.listNode );
  OSALEnterCritical();
  DLL_PushBack( &osal.osalTaskList, &local.listNode );
  OSALExitCritical();

  LOG_ASSERT( local.taskId == OSALGetCurrentTaskID() );

  // Call the function.
  local.pTaskFunc(local.pParam);

  OSALEnterCritical();
  DLL_NodeUnlist(&local.listNode);
  OSALExitCritical();

  // A task has returned - unexpected in an embedded system.
  LOG_ASSERT_WARN(false);

  {
    OSALTaskPtrT pTask = local.pTask;
    OSALTaskDelete(&pTask);
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t OSALGetCurrentTaskID(void) {
  osal_NewTaskDataT *pTask = NULL;
  OSALEnterCritical();
  DLLNode *pIter = DLL_Begin(&osal.osalTaskList);
  DLLNode * const pEnd = DLL_End(&osal.osalTaskList);
  void * pMem = &pIter; // Get a pointer to a variable on the stack.
  const addr_t mem = (addr_t)pMem;

  while ((pIter != pEnd) && (NULL == pTask)){
    osal_NewTaskDataT * const pLocal = (osal_NewTaskDataT *)pIter;
    const addr_t stackLow = (addr_t) pLocal->pStack;
    const addr_t stackHigh = (addr_t) &pLocal->pStack[pLocal->stackSize];
    if ((mem >= stackLow) && (mem < stackHigh)){
      pTask = pLocal;
    }
    else {
      pIter = pIter->pNext;
    }
  }
  OSALExitCritical();
  return (pTask) ? pTask->taskId : 0;
}

#define OSAL_EXTRA_STACK_BYTES_JUST_IN_CASE 16

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
OSALTaskPtrT OSALTaskCreate(OSALTaskFuncPtrT pTaskFunc, void *const pParam,
                            const OSALPrioT prio,
                            const OSALTaskStructT *const pTaskParams) {
  LOG_ASSERT(pTaskParams);
  osalTaskT *const pTask = (osalTaskT *)pvPortMalloc(sizeof(osalTaskT));
  LOG_ASSERT(NULL != pTask);

  memset(pTask, 0, sizeof(*pTask));
  LOG_ASSERT( (prio >= 1) && (prio < ARRSZN(osalPrioMap) ));
  const portBASE_TYPE rtprio = osalPrioMap[prio];
  StackType_t * pStackWords = (StackType_t *)pTaskParams->pStack;

  const uint32_t stackSizeWords =
      (pTaskParams->stackSize / sizeof(StackType_t)) - OSAL_EXTRA_STACK_BYTES_JUST_IN_CASE;


  // Allocate a temporary buffer to use to pass some information into the new task.
  osal_NewTaskDataT * const pCbData = 
    (osal_NewTaskDataT *)OSALMALLOC(sizeof(osal_NewTaskDataT));

  LOG_ASSERT(pCbData);
  pCbData->pParam = pParam;
  pCbData->pTaskFunc = pTaskFunc;
  pCbData->taskId = pTaskParams->taskId;
  pCbData->pTask = pTask;
  pCbData->pStack = pStackWords;
  pCbData->stackSize = stackSizeWords;

  pTask->vCreatedtask =
      xTaskCreateStatic(
          OSALTaskCreateTaskCb, // TaskFunction_t
          "",                   // const char*
          stackSizeWords,       // uint32_t
          pCbData,              // void*
          rtprio,               // UBaseType_t
          pStackWords,          // StackType_t*
          &pTask->vTcb          // StaticTask_t*
      );

  LOG_ASSERT(pTask->vCreatedtask);

  return (void *)pTask;
}

#ifndef WIN32
typedef struct StackMonTag {
  uint32_t taskId;
  portSTACK_TYPE *pStack;
  uint32_t stackSize;
} StackMonT;


#include "utils/crc.h"
#define MAX_STACKS 8
static StackMonT tasksCopy[MAX_STACKS] = {0};
static float stackUsagePct[MAX_STACKS] = {0};
static uint16_t stackUsageChecksum = 0;
#endif
void OSALCheckStacks(void){

#ifndef WIN32
  // Make a copy of all task stacks
  OSALEnterCritical();
  DLLNode * pIter = DLL_Begin( &osal.osalTaskList );
  DLLNode * const pEnd = DLL_End( &osal.osalTaskList );
  int   stackIdx = 0;
  while (pIter != pEnd){
    osal_NewTaskDataT *pTask = (osal_NewTaskDataT *)pIter;
    StackMonT &stk = tasksCopy[stackIdx];
    LOG_ASSERT(stackIdx < MAX_STACKS);
    stk.taskId = pTask->taskId;
    stk.pStack = pTask->pStack;
    stk.stackSize = pTask->stackSize;
    stackIdx++;
    pIter = pIter->pNext;
  }
  OSALExitCritical();

  if (log_TraceEnabled) {
    for (int i = 0; i < stackIdx; i++) {
      StackMonT& stk = tasksCopy[i];
      int unusedWords = 0;
      int w = 0;
      while ((w < (int)stk.stackSize) && (stk.pStack[w] == 0xa5a5a5a5)) {
        unusedWords++;
        w++;
      }
      const int usedWords = stk.stackSize - unusedWords;
      const float usedRatio = (float)usedWords / (float)stk.stackSize;
      stackUsagePct[i] = 100.0f * usedRatio;
    }

    const uint16_t newCrc = CRC_16((uint8_t*)stackUsagePct, sizeof(stackUsagePct));
    if (newCrc != stackUsageChecksum) {
      stackUsageChecksum = newCrc;
      for (int i = 0; i < stackIdx; i++) {
        StackMonT& stk = tasksCopy[i];
        LOG_TRACE(("Stack %d used %u percent of %u bytes\r\n", i, (int)stackUsagePct[i], ((stk.stackSize + OSAL_EXTRA_STACK_BYTES_JUST_IN_CASE) * sizeof(uint32_t))));
      }
    }

    MemPoolsPrintUsage();
  }

#endif

}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
bool OSALTaskDelete(OSALTaskPtrT *const ppTaskPtr) {
  osalTaskT **const ppTask = (osalTaskT **)ppTaskPtr;
  LOG_ASSERT(ppTask);
  osalTaskT *const pTask = *ppTask;
  LOG_ASSERT(pTask);
#if (INCLUDE_vTaskDelete > 0)
  vTaskDelete(pTask->vCreatedtask);
#else
  LOG_ASSERT(false);
#endif

  vPortFree(pTask);

  *ppTask = NULL;

  return true;
}






// ////////////////////////////////////////////////////////////////////////////////////////////////
// START of static memory for use by freertos startup.
// ////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
//#define DBG_MEMALLOC
#endif
#ifdef DBG_MEMALLOC
}
#include <fstream>
#include <map>
using namespace std;
typedef map<uintptr_t, bool> DbgMapT;
DbgMapT dbgMap;
static fstream dbgf("osalmemdbg.txt", ios_base::out);
extern "C" {
#endif

// Used for startup of the OS.
static uint64_t mp_startupMem[512 + 256];
static BufAlloc mp_Buf(mp_startupMem, ARRSZ(mp_startupMem));
static bool mempoolsStarted = false;


// ////////////////////////////////////////////////////////////////////////////////////////////////
// During startup, we don't want to use memory pools for allocation, as none of the startup
// allocations are ever freed (they are like static memory.)  However, AFTER startup, items MAY be
// freed, so we switch to mempools after startup.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void OSALFreertosSwitchToMempools(const bool pakmkey_PowerIsEnabled){
  if (pakmkey_PowerIsEnabled != mempoolsStarted){
    mempoolsStarted = pakmkey_PowerIsEnabled;
    if (mempoolsStarted){
      LOG_TRACE(("OSAL: %u of %u vPortFree words in startup memory.\r\n", mp_Buf.AvailWords(), ARRSZ(mp_startupMem) ));
    }
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// This is called by freertos during startup
// ////////////////////////////////////////////////////////////////////////////////////////////////
void *pvPortMalloc( size_t xSize ) {
  void *pMem = NULL;
  if (mempoolsStarted){
    pMem = OSALMALLOC(xSize);
  }
  else {
    CSLocker lock;
    pMem = mp_Buf.Malloc( xSize );
    //pMem = malloc( xSize );
  }
#ifdef DBG_MEMALLOC
  dbgf << "alloc: " << hex << (uintptr_t)pMem << " size:" << dec << xSize << endl;
  LOG_ASSERT(pMem);
  {
    CSLocker lock;
    LOG_ASSERT(dbgMap.end() == dbgMap.find((uintptr_t)pMem));
    dbgMap[(uintptr_t)pMem] = true;
  }
#endif
  return pMem;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// This should only be called after mempools start.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void vPortFree( void *pv ) {
#ifdef DBG_MEMALLOC
  dbgf << "free: " << hex << (uintptr_t)pv << endl;
  {
    CSLocker lock;
    if (pv){
      LOG_ASSERT(dbgMap.find( (uintptr_t)pv ) != dbgMap.end());
      dbgMap.erase((uintptr_t)pv );  
    }
    LOG_ASSERT(dbgMap.find( (uintptr_t)pv ) == dbgMap.end());
  }
#endif
  if(mempoolsStarted){
    OSALFREE(pv);
  }
  else {
#ifndef RUN_GTEST
    // We cannot vPortFree!
    LOG_ASSERT_WARN( false );
    //free( pv );
#endif
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void vPortInitialiseBlocks( void ) {
  mempoolsStarted = false;
#ifdef DBG_MEMALLOC
  dbgf << "reset: "<< endl;
#endif
  mp_Buf.Reset();
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Description - see the header file.
// ////////////////////////////////////////////////////////////////////////////////////////////////
size_t xPortGetFreeHeapSize( void ) {
  CSLocker lock;
  return mp_Buf.AvailBytes();
}

#ifdef __EMBEDDED_MCU_BE__

#include <sys/time.h>
#ifndef _TIMEVAL_DEFINED
#define _TIMEVAL_DEFINED
struct timeval {
  time_t      tv_sec;
  suseconds_t tv_usec;
};
#endif

// ////////////////////////////////////////////////////////////////////////////////////////////////
// END of static memory for use by freertos startup.
// ////////////////////////////////////////////////////////////////////////////////////////////////
// The gettimeofday() function shall obtain the current time,
// expressed as seconds and microseconds since the Epoch, and
// store it in the timeval structure pointed to by tp.The
// resolution of the system clock is unspecified
int gettimeofday(struct timeval *tv, void *tz){
  (void)tz;
  if (!tv) return 0;
  const LibCryptoAL *pal = LCAL_GetAppLcal();
  if (!pal) {
    tv->tv_sec = OSALGetMS() / 1000;
    tv->tv_usec = 0;
  }
  else {
    lcal_time_t ts;
    pal->time(&ts);
    LOG_ASSERT(ts >= 1544561218ull);
    LOG_ASSERT(ts <= (2ull*1544561218ull));
    tv->tv_sec = ts;
    tv->tv_usec = 0;
  }

  return 0;
}

#endif

} // extern "C"

#else
#if defined(WIN32)
#include <cstddef>

// Stubs so the program links.
extern "C" {
void *pvPortMalloc(size_t sz){
  return (void *)0;
}

void vPortFree(void *p){
  (void)p;
}

void vApplicationStackOverflowHook( void * pxTask, signed char *pcTaskName ){
  (void)pxTask;
  (void)pcTaskName;
}

void vApplicationIdleHook( void ){
}
}
#endif // WIN32

#endif

