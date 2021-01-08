/**
* COPYRIGHT    (c)	Applicaudia 2020
* @file        osal_entropy.hpp
* @brief       Defines a basic OSAL entropy primitive to be used with mbedtls.
*/

#ifndef OSAL_ENTROPY_HPP
#define OSAL_ENTROPY_HPP

#include <stdint.h>
#include "utils/sl_list.hpp"
#include "utils/platform_log.h"

#ifdef __cplusplus

class OsalEntropySrc;

// Allows additional sources of entropy to be hooked into OSALRandom.
typedef struct EntropySrcEntryTag {
  SLLNode                listNode;
  OsalEntropySrc *       pSrc;
} EntropySrcEntry;

// Additional sources of entropy should use this API to allow OSALRandom
// to utilize them.
class OsalEntropySrc {
protected:

#if (PLATFORM_EMBEDDED > 0) && !defined(EMSCRIPTEN)
  const char * const dbgModId = "OsalEntropySrc";
#endif

public:
  OsalEntropySrc()
    : mEntropySrcEntry({ {}, this })
  {
    SLL_NodeInit(&mEntropySrcEntry.listNode);
  }
  virtual ~OsalEntropySrc() {}

  // Fetches entropy from the source. Return the amount of entropy fetched.
  virtual int GetEntropy(uint8_t *pbuf, const int entropyLen) {
    (void)pbuf; (void)entropyLen;
    return 0;
  }

  // Gets the internally-generated entropy object.
  virtual EntropySrcEntry &GetListEntry() {
    return mEntropySrcEntry;
  }

protected:
  EntropySrcEntry mEntropySrcEntry;
};


#include "utils/byteq.hpp"
#include <stddef.h> // for NULL
// A simplified entropy src using SHA as a PRNG
class OsalShaEntropySrc : public OsalEntropySrc {
protected:
  static const int reseedSize = 256;
public:

  OsalShaEntropySrc();

  virtual ~OsalShaEntropySrc();

  virtual int GetEntropy(uint8_t *pbuf, const int entropyLen) override;

  virtual void WriteText(
    const uint8_t * const pText,
    const int len);

  virtual void ForceReseed(
    const uint8_t * const pText = NULL,
    const int len = 0);

  virtual int GetReseedSize() {
    return reseedSize;
  }

protected:


  virtual void ConsumeTextEntropy();

  virtual void HashMoreText(
    const uint8_t * const pText = NULL,
    const int len = 0);

protected:

  // Used only for initial IV and key
  uint8_t mHashPrev[64];
  uint8_t mHashedKey[64];
  uint8_t mIncomingEntropyArr[128];

  ByteQ mIncomingEntropyQ;

  bool mUniqueEntropyReceived;

};

#include "mbedtls/myconfig.h"
#ifdef MBEDTLS_CTR_DRBG_C
//#define OSALENTROPY_DRBG_CTR
#endif

#ifdef OSALENTROPY_DRBG_CTR
#include "mbedtls/ctr_drbg.h"
#define mbedtls_drbg_reseed mbedtls_ctr_drbg_reseed
#define mbedtls_drbg_random mbedtls_ctr_drbg_random
#define mbedtls_drbg_init mbedtls_ctr_drbg_init
#define mbedtls_drbg_free mbedtls_ctr_drbg_free
#define mbedtls_drbg_seed mbedtls_ctr_drbg_seed
#define MBETLS_DRBG_MAX_REQUEST MBEDTLS_CTR_DRBG_MAX_REQUEST
#else
#include "mbedtls/hmac_drbg.h"
#define mbedtls_drbg_reseed mbedtls_hmac_drbg_reseed
#define mbedtls_drbg_random mbedtls_hmac_drbg_random
#define mbedtls_drbg_init mbedtls_hmac_drbg_init
#define mbedtls_drbg_free mbedtls_hmac_drbg_free
#define mbedtls_drbg_seed mbedtls_hmac_drbg_seed
#define MBETLS_DRBG_MAX_REQUEST MBEDTLS_HMAC_DRBG_MAX_REQUEST
#endif


// A simplified entropy src using AES as a PRNG
class OsalDrbgEntropySrc : public OsalShaEntropySrc {
protected:
#ifdef OSALENTROPY_DRBG_CTR
  static const int reseedSize = MBEDTLS_CTR_DRBG_MAX_SEED_INPUT - MBEDTLS_CTR_DRBG_ENTROPY_LEN;
#else
  static const int reseedSize = MBEDTLS_HMAC_DRBG_MAX_SEED_INPUT;
#endif

public:

  OsalDrbgEntropySrc();

  virtual ~OsalDrbgEntropySrc();

  virtual int GetEntropy(uint8_t *pbuf, const int entropyLen) override;

  virtual void ForceReseed(
    const uint8_t * const pText = NULL,
    const int len = 0) override;

  virtual void ConsumeTextEntropy() override;

  virtual int GetReseedSize() override {
    return reseedSize;
  }

  static const uint8_t zeroes[128];

private:

  static int MbedCtrDrbgReseedC(void *p, unsigned char *data, size_t len) {
    return ((OsalDrbgEntropySrc *)p)->MbedCtrDrbgReseed(data, len);
  }
  int MbedCtrDrbgReseed(unsigned char *data, size_t len);

private:

  ByteQ * mpSeedQ;
#ifdef OSALENTROPY_DRBG_CTR
  mbedtls_ctr_drbg_context mCtrDrbg;
#else
  mbedtls_hmac_drbg_context mCtrDrbg;
  const mbedtls_md_info_t * const mpMdInfo;
#endif


};

#endif // __cplusplus

#endif //#ifndef OSAL_ENTROPY_HPP
