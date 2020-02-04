#include "osal.h"

#include "mempools.h"
#include "utils/platform_log.h"
#include "osal_random.hpp"
#include "utils/simple_string.hpp"
//#include "rtc/rtc.h"
#include "mbedtls/config.h"

LOG_MODNAME("osal.cpp")


extern "C" {

// ////////////////////////////////////////////////////////////////////////
void OSALRandomInit(const char *const szName, const int len) {
  (void)szName;(void)len;
  (void)OsalRandom::inst();
}

// ////////////////////////////////////////////////////////////////////////
void OSALRandom(uint8_t *const pBytes, const uint32_t len) {
  OsalRandom::inst().Read(pBytes, len);
}

// ////////////////////////////////////////////////////////////////////////////
void OSALRandomWriteEntropyText(const void * const pEntropy, const uint32_t len){ 
  uint32_t t = OSALGetMS();
  OsalRandom::inst().QueueEntropyText((uint8_t *)&t, sizeof(t));
  if (pEntropy && len > 0) {
    OsalRandom::inst().QueueEntropyText((const uint8_t *)pEntropy, len);
  }
}

// ////////////////////////////////////////////////////////////////////////////
void OSALRandomSeed(const uint8_t * const pEntropy, const uint32_t len) {
  sstring tmp;
  tmp.assign_static((uint8_t *)pEntropy, len);
  OsalRandom::inst().Deserialize(tmp);
}


// ////////////////////////////////////////////////////////////////////////////
// Returns TRUE if the boolean was FALSE and is now TRUE.
bool OSALTestAndSet(volatile bool * const pBoolToTestAndSet) {
  bool rval = false;
  if (pBoolToTestAndSet) {
    if (false == *pBoolToTestAndSet){
      OSALEnterCritical();
      if (false == *pBoolToTestAndSet) {
        *pBoolToTestAndSet = rval = true;
      }
      OSALExitCritical();
    }
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
void OSALClearFlag(bool * const pBoolToClear){
  if (pBoolToClear) {
    OSALEnterCritical();
    *pBoolToClear = false;
    OSALExitCritical();
  }
}

#if (PLATFORM_EMBEDDED > 0) && defined(__EMBEDDED_MCU_BE__)
#define STUB_RANDOMBYTES
#endif

#ifdef STUB_RANDOMBYTES 
void randombytes(unsigned char *const x, const unsigned long long xlen) {
  LOG_ASSERT(xlen <= 0xffffffff);
  OSALRandom((uint8_t *)x, (uint32_t)xlen);
}

void randombytes_buf(void * const buf, const size_t size) {
  randombytes((unsigned char *)buf, (unsigned long long)size);
}

void randombytes_stir(void) {
  uint32_t ms = OSALGetMS();
  OSALRandomWriteEntropyText(&ms, sizeof(ms));
}
#endif



}


