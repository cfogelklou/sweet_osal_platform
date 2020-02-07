/**
* COPYRIGHT    (c)	Applicaudia 2020
* @file        osal_random.cpp
* @brief       Random number generation.
*/
#include "mbedtls/myconfig.h"
#include "osal/osal_random.hpp"
#include "osal/osal.h"
#include "osal/cs_task_locker.hpp"

#if (OSALRANDOM_LIBSODIUM > 0)
#include "crypto_stream.h"
#else
#include "mbedtls/sha512.h"
#include "mbedtls/sha256.h"
#include "mbedtls/aes.h"
#endif
#include "utils/simple_string.hpp"
#include "utils/helper_macros.h"

#ifdef PAK_ECU
#include "params/param_mgr.hpp"
#endif
#include <string.h>

LOG_MODNAME("osal_random.cpp");

SINGLETON_INSTANTIATIONS(OsalRandom);

const int OsalRandom::CACHED_ENTROPY_BYTES; // definition

extern void MbedInitThreadingAlt();

// ////////////////////////////////////////////////////////////////////////////
OsalRandom::OsalRandom()
#ifdef __EMSCRIPTEN__
  : mpEntropyCtx(new mbedtls_entropy_context)
  , mEntropyCtx(*mpEntropyCtx)
#else
  : mEntropyCtx()
#endif
  , mDefaultEntropy()
  , mListOfEntropySrcs()
  , mResched(TSCHED_F_OCH_L,
    [](void *p, uint32_t t) {((OsalRandom *)p)->OsalRegenEntropyTask(t); }, 
    this, 
    TS_PRIO_APP)
  , mEntropyCacheQ(mCachedEntropyArr, CACHED_ENTROPY_BYTES)
  , mCachedEntropyArr()
{
  MbedInitThreadingAlt();
  EntropySrcEntry * const pEntry = &mDefaultEntropy.GetListEntry();
  LOG_ASSERT(pEntry->pSrc);
  mListOfEntropySrcs.push_back(&pEntry->listNode);
  mbedtls_entropy_init(&mEntropyCtx);
  LOG_ASSERT(mEntropyCtx.source_count > 0);
}

// ////////////////////////////////////////////////////////////////////////////
OsalRandom::~OsalRandom() {
  mbedtls_entropy_free(&mEntropyCtx);
#ifdef __EMSCRIPTEN__
  delete mpEntropyCtx;
  mpEntropyCtx = NULL;
#endif
}

// ////////////////////////////////////////////////////////////////////////////
void OsalRandom::Deserialize(const sstring &in) {
  CSTaskLocker lock;
  const int reseedSize = mDefaultEntropy.GetReseedSize();
  const int sz = MIN(reseedSize, in.nlength());
  mDefaultEntropy.ForceReseed(in.u_str(), sz);

  // Ensure all future reads use new data.
  mEntropyCacheQ.Flush();

  // Start caching some entropy
  mResched.doReschedule(1);

}

// ////////////////////////////////////////////////////////////////////////////
void OsalRandom::Serialize(sstring &out) {
  CSTaskLocker lock;
  const int reseedSize = mDefaultEntropy.GetReseedSize();
  uint8_t *ptr = out.u8DataPtr(reseedSize, reseedSize);
  LOG_ASSERT(ptr);
  OSALRandom(ptr, reseedSize);
}

// ////////////////////////////////////////////////////////////////////////////
void OsalRandom::OsalRegenEntropyTask(uint32_t) {
#ifndef __EMSCRIPTEN__
  CSTaskLocker lock;
  const int remaining = OsalRegenEntropyNoCs();
  if (remaining > 0) {
    mResched.doReschedule(10);
  }
#endif
}

// ////////////////////////////////////////////////////////////////////////////
int OsalRandom::OsalRegenEntropyNoCs() {
  // Should be within a task critical section here.
  int remaining = mEntropyCacheQ.GetWriteReady();
  if (remaining > 0) {
    remaining = MIN(remaining, CACHED_ENTROPY_BYTES);
    uint8_t tmpBuf[MBEDTLS_ENTROPY_BLOCK_SIZE];
    const int bytesToGenerate = MIN(remaining, ARRSZN(tmpBuf));
    LOG_ASSERT(mEntropyCtx.source_count > 0);
    const int rval = mbedtls_entropy_func(&mEntropyCtx, tmpBuf, bytesToGenerate);
    LOG_ASSERT_WARN(0 == rval);
    if (0 != rval){
      LOG_TRACE(("mbedtls_entropy_func() returned %d\r\n", rval));
    }
    mEntropyCacheQ.ForceWrite(tmpBuf, bytesToGenerate);
    remaining -= bytesToGenerate;
  }
  return remaining;
}

// ////////////////////////////////////////////////////////////////////////////
// Reads some entropy from the internal queue.
int OsalRandom::Read(uint8_t *const pBytes, const int numBytes) {
  if ((NULL == pBytes) || (numBytes <= 0)) return 0;
  int remaining = numBytes;
  int iter = 0;
  while (remaining > 0) {
    CSTaskLocker cs;
    const int bytesRead = mEntropyCacheQ.Read(&pBytes[iter], remaining);
    if (bytesRead < remaining) {
      OsalRegenEntropyNoCs();
    }
    iter += bytesRead;
    remaining -= bytesRead;
  }

// TODO fix: chfo should fix OSALRandom()
#ifndef APPLICAUDIA_PROD_APP
  mResched.doReschedule(10);
#endif

  return numBytes;
}

// ////////////////////////////////////////////////////////////////////////////
// Gets the amount of entropy that can be read.
int OsalRandom::GetReadReady() {
  return mEntropyCacheQ.GetReadReady();
}

// ////////////////////////////////////////////////////////////////////////////
// Adds some entropy text that will be used to generate binary entropy when the buffer
// is full or entropy is empty.
void OsalRandom::QueueEntropyText(const uint8_t *const pBytes, const int numBytes) {
  mDefaultEntropy.WriteText(pBytes, numBytes); 
}

// ////////////////////////////////////////////////////////////////////////////
// Polls the entropy context.
// The entropy context will fetch entropy from hardware.
int OsalRandom::mbedtls_hardware_poll(unsigned char *output, size_t len, size_t *olen) {
  SLLNode *pIter = mListOfEntropySrcs.begin();
  SLLNode * const pEnd = mListOfEntropySrcs.end();
  while (pIter != pEnd) {
    EntropySrcEntry * const pEntry = (EntropySrcEntry *)pIter;
    pIter = pIter->pNext;
    LOG_ASSERT((pEntry) && (pEntry->pSrc));
    const size_t read = pEntry->pSrc->GetEntropy(output, (int)len);
    LOG_ASSERT(read == len);
  }

  *olen = len;
  return 0;
}


extern "C" {

#if 0

#if defined(MBEDTLS_TIMING_C)
  // ////////////////////////////////////////////////////////////////////////////
  int mbedtls_hardclock_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
    (void)data;
    *olen = 0;

    uint32_t t1 = OSALGetMS();
    uint64_t t = rtcm().GetSecondsSinceJan11970();
    uint32_t t2 = OSALGetMS();
    if ((t2 - t1) < 0) {
      t++;
    }
    t = (t * 1000) + (t2 % 1000);

    if (len < sizeof(t)) return(0);

    memcpy(output, &t, sizeof(t));
    *olen = sizeof(t);

    return(0);
  }
#endif
#endif

#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT) && (!OSALRANDOM_LIBSODIUM)
  // ////////////////////////////////////////////////////////////////////////////
  // Our own RNG + the entropy service provide extra entropy to mbed_tls.
  int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
    (void)data;
    return OsalRandom::inst().mbedtls_hardware_poll((uint8_t *)output, len, olen);
  }
#endif

#if (PLATFORM_EMBEDDED > 0)
#if !defined(MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES)
#if !defined(MBEDTLS_NO_PLATFORM_ENTROPY)

  // ////////////////////////////////////////////////////////////////////////////
  // Called by mbed_tls to fetch entropy.
  int mbedtls_platform_entropy_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
    (void)data;
    OSALRandom((uint8_t *)output, len);
    *olen = len;
    return 0;
  }
#endif
#endif
#endif

} // extern "C"
