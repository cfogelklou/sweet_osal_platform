
///////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Includes
///////////////////////////////////////////////////////////////////////////////////////////////////
#include "mempools.h"
#include "utils/platform_log.h"

#if (!NO_MEMPOOLS)
#include "osal.h"
#include "mbedtls/myconfig.h"
#include "mbedtls/platform.h"
#include "utils/helper_macros.h"
#include "utils/dl_list.hpp"
#include "mbedtls/memory_buffer_alloc.h"
#include "osal/cs_task_locker.hpp"
#include "utils/convert_utils.h"

#include <stdlib.h> // for malloc, free
#include <string.h> // for memset, memcpy

LOG_MODNAME("mempools.cpp");

static bool mInitialized = false;
static bool mNewDeleteOverride = false;

extern void MbedInitThreadingAlt();

#ifndef MEMPOOLS_HEAP_BYTES
#if (PLATFORM_EMBEDDED > 0)
#ifdef EVTLOG_TRACE
#define EVTLOG_TRACE_MEM 16384
#else // #ifdef EVTLOG_TRACE
#define EVTLOG_TRACE_MEM 0
#endif // #ifdef EVTLOG_TRACE

#define MEMPOOLS_HEAP_BYTES 64*1024 - EVTLOG_TRACE_MEM
#else // #if (PLATFORM_EMBEDDED > 0)
#define MEMPOOLS_HEAP_BYTES 10000000
#endif // #if (PLATFORM_EMBEDDED > 0)
#endif

#ifdef ENVIRONMENT64
#define MEMPOOLS_ALIGN_SIZE 8
typedef unsigned char memword_t;
typedef uint64_t align_mask_t;
static align_mask_t const MEMPOOLS_MAGIC = 0xBB81337B00B59955;
#else
// ENVIRONMENT32
#define MEMPOOLS_ALIGN_SIZE 4
typedef unsigned char memword_t;
typedef uint32_t align_mask_t;
static align_mask_t const MEMPOOLS_MAGIC = 0xBB81337B;
#endif

static uint8_t mempools_buf[MEMPOOLS_HEAP_BYTES];
static align_mask_t mempools_buf_start =
  (align_mask_t)&mempools_buf[0];
static align_mask_t mempools_buf_end =
  (align_mask_t)&mempools_buf[MEMPOOLS_HEAP_BYTES];

#if (MEMPOOLS_DEBUG > 0)
dll::list mempools_allocatedList;
#endif


#if (MEMPOOLS_DEBUG > 0)
// We use a magic number to ensure that
typedef struct MemChkHdrTag {

  DLLNode       listNode;
  align_mask_t  id;
  align_mask_t  magic;
#if (MEMPOOLS_DEBUG_FILETRACE > 0)
  const char  * pFile;
  align_mask_t  line;
  uint32_t      ts;
#endif
} MemChkHdr;

typedef struct MemTag {
  MemChkHdr hdr;
  uint8_t payload[8];
} Mem;
#else
#define Mem void
#endif

extern "C" {

// ////////////////////////////////////////////////////////////////////////////
void MemPoolsInitialize(void) {
  if (!mInitialized) {
    mInitialized = true;
    MbedInitThreadingAlt();
    mbedtls_memory_buffer_alloc_init(mempools_buf, sizeof(mempools_buf));
    (void)MEMPOOLS_MAGIC;
  }
}

// ////////////////////////////////////////////////////////////////////////////
static void mempools_DynamicInit(){
  LOG_WARNING(("Your test case has forgotten to initialize osal. I am doing it for you.\r\n"));
  OSALInit();
  if (!mInitialized){
    LOG_WARNING(("Your OSAL has forgotten to initialize mempools. I am doing it for you.\r\n"));
    MemPoolsInitialize();
  }
}

#define MP_INIT() if (!mInitialized) do {mempools_DynamicInit();} while(0)

// ////////////////////////////////////////////////////////////////////////////
// Free the memory allocated in pVoid.
static bool mempools_IsWithinPool(void *pVoid) {
  bool isWithinPool = false;
  if (pVoid) {
    const align_mask_t mem_addr = (align_mask_t)pVoid;
    LOG_ASSERT(mempools_buf_start < mempools_buf_end);
    if ((mem_addr >= mempools_buf_start) && (mem_addr < mempools_buf_end))
    {
      isWithinPool = true;
    }
  }
  return isWithinPool;
}

#if (MEMPOOLS_DEBUG > 0)
// ////////////////////////////////////////////////////////////////////////////
// Free the memory allocated in pVoid.
static Mem * mempools_GetHdr(void *pVoid) {
  Mem * pRval = nullptr;
  if (mempools_IsWithinPool(pVoid)) {
    uint8_t * const pMem8 = (uint8_t *)pVoid;
    pRval = (Mem *)(pMem8 - sizeof(MemChkHdr));
    LOG_ASSERT_WARN(pRval->hdr.magic == MEMPOOLS_MAGIC);
  }
  return pRval;
}
#endif

// ////////////////////////////////////////////////////////////////////////////
#if (!MEMPOOLS_DEBUG)
// Debug disabled.
void *MemPoolsMallocWithId(
  const size_t sz,
  const uint8_t id,
  const bool useHeap)
{
  MP_INIT();
  (void)useHeap;
  (void)id;
  // Cannot assign ID if MEMPOOLS_DEBUG is disabled.
  void * const pMem = mbedtls_calloc(1, sz);
  if (pMem) {
    return pMem;
  }
  else {
    static int heapAllocs = 0;
    if (1 == (++heapAllocs & 0xff)) {
      LOG_WARNING(("%d heap allocations!\r\n", heapAllocs));
    }
    return calloc(1, sz);
  }
}
#endif

#if (MEMPOOLS_DEBUG > 0)
// Debug enabled
// ////////////////////////////////////////////////////////////////////////////
#if (MEMPOOLS_DEBUG_FILETRACE > 0)
void *_MemPoolsMallocWithId(
#else // #if (MEMPOOLS_DEBUG_FILETRACE > 0)
void *MemPoolsMallocWithId(
#endif
  const size_t sz,
  const uint8_t id,
  const bool useHeap
#if (MEMPOOLS_DEBUG_FILETRACE > 0)
  ,const char * const pF,
  const int line
#endif
)
{
  MP_INIT();
  void *pRval = 0;
  if (!useHeap) {
    const size_t szWithMagic = sz + sizeof(MemChkHdr);
    Mem * const pMem = (Mem *)
      mbedtls_calloc(1, szWithMagic);
    if (pMem) {
      LOG_ASSERT(mempools_IsWithinPool(pMem));
      memcpy(&pMem->hdr.magic, &MEMPOOLS_MAGIC, sizeof(pMem->hdr.magic));
      DLL_NodeInit(&pMem->hdr.listNode);
      pMem->hdr.id = id;
#if (MEMPOOLS_DEBUG_FILETRACE > 0)
      pMem->hdr.pFile = pF;
      pMem->hdr.line = line;
      pMem->hdr.ts = OSALGetMS();
#endif
      {
        CSTaskLocker cs;
        mempools_allocatedList.push_back(&pMem->hdr.listNode);
      }
      pRval = pMem->payload;
    }
    if (nullptr == pMem) {
      static int heapAllocs = 0;
      if (1 == (++heapAllocs & 0xff)) {
        LOG_WARNING(("%d heap allocations!\r\n", heapAllocs));
      }
    }
  }
  if (nullptr == pRval) {
    // Cannot assign ID to heap objects.
    LOG_ASSERT(0 == id);
    pRval = calloc(1, sz);
  }
  LOG_ASSERT(pRval);
  return pRval;
}
#endif // MEMPOOLS_DEBUG

#if (MEMPOOLS_DEBUG > 0)
// ////////////////////////////////////////////////////////////////////////////
// With debugging enabled, checks that the memory hasn't been corrupted.
bool MemPoolsCheckMem(void *MemPtr) {
  MP_INIT();
  bool rval = false;
  if (mempools_GetHdr(MemPtr)) {
    rval = true;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
void MemPoolsForEachWithAllocId(
  const uint8_t id,
  MemPoolsForEachWithAllocIdFnT pFn,
  void *pData)
{
  MP_INIT();
  if (pFn) {
    CSTaskLocker cs;
    DLLNode *pIter = mempools_allocatedList.begin();
    DLLNode * const pEnd = mempools_allocatedList.end();
    while (pIter != pEnd) {
      DLLNode * const pNext = pIter->pNext;
      Mem * pMem = (Mem *)pIter;
      pIter = pNext;
      if (pMem->hdr.id == id) {
        pFn(pData, pMem->payload);
      }
    }
  }
}

// ////////////////////////////////////////////////////////////////////////////
void * MemPoolsHeapMallocWithId(const size_t sz, const uint8_t id) {
  MP_INIT();
  void *pRval = nullptr;
  void *pTmp = MemPoolsMalloc(sz);
  if (pTmp) {
    Mem * pMem = mempools_GetHdr(pTmp);
    if (pMem) {
      pRval = pTmp;
      pMem->hdr.id = id;
    }
    else {
      MemPoolsFree(pTmp);
    }
  }
  return pRval;
}
#endif

// ////////////////////////////////////////////////////////////////////////////
// Free the memory allocated in pVoid.
#if (MEMPOOLS_DEBUG_FILETRACE > 0)
void _MemPoolsFree(
  void *pVoid,
  const char * pFile,
  const int line
) {
#else
void MemPoolsFree(void *pVoid) {
#endif
  MP_INIT();
  if (mempools_IsWithinPool(pVoid)){
#if (!MEMPOOLS_DEBUG)
    mbedtls_free(pVoid);
#else
    Mem * const pMem = mempools_GetHdr(pVoid);
    if (pMem){
      if (MEMPOOLS_MAGIC != pMem->hdr.magic){
#if (MEMPOOLS_DEBUG_FILETRACE > 0)
        if (~MEMPOOLS_MAGIC == pMem->hdr.magic) {
          LOG_TRACE(("\r\n*****mempools.cpp::Memory was already freed at %s(%d)!\r\n", pMem->hdr.pFile, pMem->hdr.line));
        }
        else {
          LOG_TRACE(("\r\n*****mempools.cpp::Cannot free this memory!\r\n"));
        }
#else
      LOG_TRACE(("\r\n*****mempools.cpp::Cannot free this memory!\r\n"));
      LOG_ASSERT(false);
#endif
      }
      else {
        {
          CSTaskLocker cs;
          DLL_NodeUnlist(&pMem->hdr.listNode);
          pMem->hdr.magic = ~MEMPOOLS_MAGIC;
#if (MEMPOOLS_DEBUG_FILETRACE > 0)
          pMem->hdr.pFile = pFile; // Track where the block was freed now that it's freed.
          pMem->hdr.line = line;
          pMem->hdr.ts = OSALGetMS();
#endif
        }
        mbedtls_free(pMem);
      }
    }
#endif
  }
  else {
    free(pVoid);
  }
}

// ////////////////////////////////////////////////////////////////////////////
// Print all allocated memory.  Use for debugging memory leaks.
void MemPoolsPrintAllocatedMemory(void) {

#if (MEMPOOLS_DEBUG_FILETRACE > 0)
  {
    CSTaskLocker cs;
    DLLNode* pIter = mempools_allocatedList.begin();
    DLLNode* const pEnd = mempools_allocatedList.end();
    while (pIter != pEnd) {
      DLLNode* const pNext = pIter->pNext;
      Mem* pMem = (Mem*)pIter;
      pIter = pNext;
      LOG_TRACE(("\t%s(%d) with id:%d at ts %u\r\n", pMem->hdr.pFile, pMem->hdr.line, pMem->hdr.id, pMem->hdr.ts));
    }
  }
#elif defined(MBEDTLS_MEMORY_DEBUG)
  mbedtls_memory_buffer_alloc_status();
#endif
}

#if (MEMPOOLS_DEBUG > 0)
// ////////////////////////////////////////////////////////////////////////////
int MemPoolsGetAllocationsWithAllocId(const uint8_t id)
{
  int cnt = 0;

  auto fn = [](void *pUserData, void * const) {
    int &cnt = *(int *)pUserData;
    cnt++;
  };
  MemPoolsForEachWithAllocId(id, fn, &cnt);
  return cnt;
}
#endif

// ////////////////////////////////////////////////////////////////////////////
// Prints the current memory usage. Works even if MEMPOOLS_DEBUG is off.
void MemPoolsPrintUsage(void) {
#if defined(MBEDTLS_MEMORY_DEBUG)
  if (log_TraceEnabled) {
    size_t cur_used, cur_blocks;
    CSTaskLocker cs;
    mbedtls_memory_buffer_alloc_cur_get(&cur_used, &cur_blocks);
    LOG_TRACE(("MemPools: Current: %u/%u (%u pct), Blocks: %u\r\n",
      cur_used,
      MEMPOOLS_HEAP_BYTES,
      (int)(0.5f + (100.0f * cur_used) / ((float)MEMPOOLS_HEAP_BYTES)),
      cur_blocks));

    mbedtls_memory_buffer_alloc_max_get(&cur_used, &cur_blocks);
    LOG_TRACE(("MemPools: Maximum: %u/%u (%u pct), Blocks: %u\r\n",
      cur_used,
      MEMPOOLS_HEAP_BYTES,
      (int)(0.5f + (100.0f * cur_used) / ((float)MEMPOOLS_HEAP_BYTES)),
      cur_blocks));
  }
#endif
}

// ////////////////////////////////////////////////////////////////////////////
void MemPoolsGetUsage(
  size_t *pcur_used,
  size_t *pcur_blocks,
  size_t *pmax_used,
  size_t *pmax_blocks){
  size_t cur_used = 0;
  size_t cur_blocks = 0;
  size_t max_used = 0;
  size_t max_blocks = 0;
#if defined(MBEDTLS_MEMORY_DEBUG)
  CSTaskLocker cs;
  mbedtls_memory_buffer_alloc_cur_get(&cur_used, &cur_blocks);
  mbedtls_memory_buffer_alloc_max_get(&max_used, &max_blocks);
#endif
  if (pcur_used) *pcur_used = cur_used;
  if (pcur_blocks) *pcur_blocks = cur_blocks;
  if (pmax_used) *pmax_used = max_used;
  if (pmax_blocks) *pmax_blocks = max_blocks;
}

// ////////////////////////////////////////////////////////////////////////////
// Override new/delete
// Enable this after platform startup on an embedded system so that strings, etc, will use mempools
// instead of the heap.
// Returns the old value.
bool MemPoolsEnableNewOverride(const bool enable) {
  MP_INIT();
  const bool rval = mNewDeleteOverride;
  mNewDeleteOverride = enable;
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
// Completely disable new/delete overrides.
void MemPoolsDeInitNewOverride(void) {

  mNewDeleteOverride = false;
}

} // extern "C" {


  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // Override new and delete in the embedded platform, or any non-apple platform
#if (PLATFORM_EMBEDDED > 0) || (!defined(APPLE) && !defined(__APPLE__) && !defined(ANDROID))
void* operator new     (size_t size) {
  if (mNewDeleteOverride) {
    return MemPoolsMalloc(size);
  }
  else if (mInitialized) {
    return MemPoolsHeapMalloc(size);
  }
  else {
    void * const p = calloc(1, size);
    LOG_ASSERT(p);
    return p;
  }
}

  ///////////////////////////////////////////////////////////////////////////////////////////////////
void* operator new[](size_t size) {
  if (mNewDeleteOverride) {
    return MemPoolsMalloc(size);
  }
  else if (mInitialized) {
    return MemPoolsHeapMalloc(size);
  }
  else {
    void * const p = calloc(1, size);
    LOG_ASSERT(p);
    return p;
  }
}

  ///////////////////////////////////////////////////////////////////////////////////////////////////
void  operator delete (void* ptr)
{
  if (mInitialized) {
    MemPoolsFree(ptr);
  }
  else {
    free(ptr);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void  operator delete[](void* ptr) {
  if (mInitialized) {
    MemPoolsFree(ptr);
  }
  else {
    free(ptr);
  }
}

#endif // #if (PLATFORM_EMBEDDED > 0) || (!defined(APPLE) && !defined(__APPLE__))


#else // #if (!NO_MEMPOOLS)
extern "C" {
void MemPoolsInitialize() {
}

}

#endif // #if (!NO_MEMPOOLS)


