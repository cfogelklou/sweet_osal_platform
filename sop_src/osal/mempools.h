#ifndef MEMPOOLS_H__
#define MEMPOOLS_H__
/**
 * COPYRIGHT	(c)	Applicaudia 2020
 * @file        mempools.h
 * @brief       This used to contain pools of memory, but this functionality has
 *              been remapped to mbedtls's buffer allocator.
 *
 *              This module now serves to initialize the mbedtls buffer allocator
 *              and to provide some debug functionality when MEMPOOLS_DEBUG is enabled.
 */

#include "osal/platform_type.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#if ((TARGET_OS_IOS > 0) || defined(EMSCRIPTEN))
#undef NO_MEMPOOLS
#define NO_MEMPOOLS 1
#else
#undef NO_MEMPOOLS
#define NO_MEMPOOLS 0
#endif

#if (NO_MEMPOOLS > 0)
#undef MEMPOOLS_DEBUG
#define MEMPOOLS_DEBUG 0
#undef MEMPOOLS_DEBUG_FILETRACE
#define MEMPOOLS_DEBUG_FILETRACE 0

#define MemPoolsMalloc(size) calloc(1, (size))
#define _MemPoolsMalloc(size, u1, u2) calloc(1, (size))
#define _MemPoolsMallocWithId(sz, id, ...) calloc(1, sz)

#define MemPoolsMallocWithId(sz, id, ...) calloc(1, sz)
#define MemPoolsFree(pVoid) free(pVoid)
#define _MemPoolsFree(pVoid) free(pVoid)
#define MemPoolsHeapMalloc(sz) calloc(1, (sz))
#define MemPoolsPrintAllocatedMemory() \
  do {                                 \
    ;                                  \
  } while (0)
#define MemPoolsPrintUsage() \
  do {                       \
    ;                        \
  } while (0)
#define MemPoolsEnableNewOverride(ovr) (true)
#define MemPoolsDeInitNewOverride() \
  do {                              \
    ;                               \
  } while (0)
#define MemPoolsForEachWithAllocId(a, b, c) \
  do {                                      \
    ;                                       \
  } while (0)

#else // #if (NO_MEMPOOLS > 0)

#ifndef MEMPOOLS_DEBUG
#if (PLATFORM_FULL_OS > 0)
#define MEMPOOLS_DEBUG 1
#else // #if (PLATFORM_FULL_OS > 0)
#define MEMPOOLS_DEBUG 0
#endif // #if (PLATFORM_FULL_OS > 0)
#endif // #if (NO_MEMPOOLS > 0)

#ifndef MEMPOOLS_DEBUG_FILETRACE
#if ((PLATFORM_FULL_OS > 0) && (MEMPOOLS_DEBUG > 0))
#define MEMPOOLS_DEBUG_FILETRACE 1
#else // #if ((PLATFORM_FULL_OS > 0) && (MEMPOOLS_DEBUG > 0))
#define MEMPOOLS_DEBUG_FILETRACE 0
#endif // #if ((PLATFORM_FULL_OS > 0) && (MEMPOOLS_DEBUG > 0))
#endif // #ifndef MEMPOOLS_DEBUG_FILETRACE

#ifndef __cplusplus
#define MP_DEFAULT(x)
#else
#define MP_DEFAULT(x) = (x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if (MEMPOOLS_DEBUG_FILETRACE > 0)
void* _MemPoolsMallocWithId(
  const size_t sz,
  const uint8_t id MP_DEFAULT(0),
  const bool useHeap MP_DEFAULT(false),
  const char* const MP_DEFAULT(""),
  const int line MP_DEFAULT(-1));

#define _MemPoolsMalloc(sz, pf, line) \
  _MemPoolsMallocWithId((sz), 0, false, (pf), (line))

// Debug default malloc from system pools.
#define MemPoolsMalloc(sz) \
  _MemPoolsMallocWithId((sz), 0, false, dbgModId, __LINE__)

#define MemPoolsMallocWithId(sz, id) \
  _MemPoolsMallocWithId((sz), (id), false, dbgModId, __LINE__)

#define MemPoolsHeapMalloc(sz) \
  _MemPoolsMallocWithId((sz), 0, true, dbgModId, __LINE__)

void _MemPoolsFree(
  void* pVoid,
  const char* const MP_DEFAULT(""),
  const int line MP_DEFAULT(-1));

#define MemPoolsFree(ptr) \
  _MemPoolsFree((ptr), dbgModId, __LINE__)

#else // #if (MEMPOOLS_DEBUG_FILETRACE > 0)

void* MemPoolsMallocWithId(
  const size_t sz,
  const uint8_t id MP_DEFAULT(0),
  const bool useHeap MP_DEFAULT(false));

// Debug default malloc from system pools.
#define MemPoolsMalloc(sz) \
  MemPoolsMallocWithId((sz), 0, false)

#define MemPoolsHeapMalloc(sz) \
  MemPoolsMallocWithId((sz), 0, true)

// Free the memory allocated in pVoid.
void MemPoolsFree(void* pVoid);

#endif // #ifdef MEMPOOLS_DEBUG_FILETRACE

// The MALLOC and FREE are different depending on where they are called from.
#if (MEMPOOLS_DEBUG > 0)

// With debugging enabled, checks that the memory hasn't been corrupted.
bool MemPoolsCheckMem(void* MemPtr);

typedef void (*MemPoolsForEachWithAllocIdFnT)(void* pUserData, void* const pMem);

void MemPoolsForEachWithAllocId(const uint8_t id, MemPoolsForEachWithAllocIdFnT pFn, void* pData);

int MemPoolsGetAllocationsWithAllocId(const uint8_t id);

#else // #if (MEMPOOLS_DEBUG > 0)

// With debugging enabled, checks that the memory hasn't been corrupted.
#define MemPoolsCheckMem(M) (true)
#define MemPoolsForEachWithAllocId(id)
#define MemPoolsGetAllocationsWithAllocId(id)

#endif // #if (MEMPOOLS_DEBUG > 0)

// Print all allocated memory.  Use for debugging memory leaks.
void MemPoolsPrintAllocatedMemory(void);

// Prints the current memory usage. Works even if MEMPOOLS_DEBUG is off.
void MemPoolsPrintUsage(void);

// Gets current usage numbers.
void MemPoolsGetUsage(
  size_t* pcur_used MP_DEFAULT(nullptr),
  size_t* pcur_blocks MP_DEFAULT(nullptr),
  size_t* pmax_used MP_DEFAULT(nullptr),
  size_t* pmax_blocks MP_DEFAULT(nullptr));

// Override new/delete
// Enable this after platform startup on an embedded system
// so that strings, etc, will use mempools instead of the
// heap. Returns the old value.
bool MemPoolsEnableNewOverride(const bool enable);

// Completely disable new/delete overrides.
void MemPoolsDeInitNewOverride(void);

#ifdef __cplusplus
}
#endif

#endif // #if (NO_MEMPOOLS > 0)

#endif
