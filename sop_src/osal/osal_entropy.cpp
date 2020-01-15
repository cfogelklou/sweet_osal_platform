/**
* COPYRIGHT    (c)	Applicaudia 2020
* @file        osal_entropy.cpp
*              Some entropy sources for use by the 
*              entropy poller in mbedtls.
*              These sources can be fed by events in the 
*              system, and will always produce enough entropy as they
*              use a PRNG.
*/

#include "osal_entropy.hpp"
#include "osal/cs_task_locker.hpp"
#include "utils/helper_macros.h"
#include "utils/simple_string.hpp"
#include "mbedtls/sha512.h"
#include <string.h>



// ////////////////////////////////////////////////////////////////////////////
OsalShaEntropySrc::OsalShaEntropySrc()
  : OsalEntropySrc()
  , mHashPrev()
  , mHashedKey()
  , mIncomingEntropyArr()
  , mIncomingEntropyQ(mIncomingEntropyArr, ARRSZ(mIncomingEntropyArr))
  , mUniqueEntropyReceived(false)
{
  HashMoreText((uint8_t *)"OsalAes", 7);
}

// ////////////////////////////////////////////////////////////////////////////
OsalShaEntropySrc::~OsalShaEntropySrc() {
}

// ////////////////////////////////////////////////////////////////////////////
void OsalShaEntropySrc::WriteText(const uint8_t * const pText, const int len) {
  CSTaskLocker lock;
  int remaining = len;
  int written = 0;
  while (remaining > 0) {
    int bytesToWrite = len - written;
    bytesToWrite = MIN(bytesToWrite, remaining);
    const int bytesWritten = mIncomingEntropyQ.Write(&pText[written], bytesToWrite);
    remaining -= bytesWritten;
    written += bytesWritten;
    if ((!mUniqueEntropyReceived) || (bytesWritten < bytesToWrite)) {
      ConsumeTextEntropy();
    }
  }
}

// ////////////////////////////////////////////////////////////////////////////
void OsalShaEntropySrc::HashMoreText(const uint8_t * const pText, const int len) {
  mbedtls_sha512_context ctx;
  mbedtls_sha512_init(&ctx);
  mbedtls_sha512_starts_ret(&ctx, 0);
  mbedtls_sha512_update_ret(&ctx, mHashedKey, sizeof(mHashPrev));
  mbedtls_sha512_update_ret(&ctx, mHashPrev, sizeof(mHashPrev));
  if ((pText) && (len > 0)) {
    mbedtls_sha512_update_ret(&ctx, pText, len);
  }
  mbedtls_sha512_finish_ret(&ctx, mHashPrev);
  mbedtls_sha512_free(&ctx);
}

// ////////////////////////////////////////////////////////////////////////////
void OsalShaEntropySrc::ForceReseed(const uint8_t * const pText, const int len) {
  CSTaskLocker lock;
  if ((pText) && (len > 0) && (!mUniqueEntropyReceived)) {
    mUniqueEntropyReceived = true;
    // We should get initial reseed by deserialization, not basic entropy.
    LOG_VERBOSE(("Initial reseed by ForceReseed().  Good!\r\n"));
    mbedtls_sha512(pText, len, mHashedKey, 0);
  }
  HashMoreText(pText, len);
}

// ////////////////////////////////////////////////////////////////////////////
void OsalShaEntropySrc::ConsumeTextEntropy() {
  const int rdy = mIncomingEntropyQ.GetReadReady();
  if (rdy > 0) {
    sstring tmp;
    uint8_t *pBuf = tmp.u8DataPtr(rdy, rdy);
    LOG_ASSERT(pBuf);
    mIncomingEntropyQ.Read(pBuf, rdy);

    HashMoreText(pBuf, rdy);

    if (!mUniqueEntropyReceived) {
      mUniqueEntropyReceived = true;
      // We should get initial reseed by deserialization, not basic entropy.
      LOG_WARNING(("WARNING: Initial reseed by incoming entropy.\r\n"));
      mbedtls_sha512(pBuf, rdy, mHashedKey, 0);
    }
  }
}

// ////////////////////////////////////////////////////////////////////////////
int OsalShaEntropySrc::GetEntropy(uint8_t *pbuf, const int entropyLen)
{
  int remaining = entropyLen;
  int bytesCreated = 0;

  while (remaining > 0) {
    CSTaskLocker lock;
    if (!mUniqueEntropyReceived) {
      LOG_ASSERT_WARN(mUniqueEntropyReceived);
      mUniqueEntropyReceived = true;
    }
    const int bytesToCreate = MIN(remaining, 64);
    HashMoreText((uint8_t *)"01234567890", 10);
    memcpy(&pbuf[bytesCreated], mHashPrev, bytesToCreate);
    remaining -= bytesToCreate;
    bytesCreated += bytesToCreate;
  }

  return entropyLen;
}


const uint8_t OsalDrbgEntropySrc::zeroes[128] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

// ////////////////////////////////////////////////////////////////////////////
OsalDrbgEntropySrc::OsalDrbgEntropySrc() 
  : OsalShaEntropySrc()
  , mpSeedQ(NULL)
  , mCtrDrbg()
  , mpMdInfo(mbedtls_md_info_from_type(MBEDTLS_MD_SHA512))
{
  mbedtls_drbg_init(&mCtrDrbg);

  // Note that this object is useless until Deserialize is called.
  ByteQ seedQ((uint8_t *)zeroes, ARRSZ(zeroes));
  seedQ.CommitWrite(ARRSZ(zeroes));
  this->mpSeedQ = &seedQ;
  mbedtls_drbg_seed( &mCtrDrbg,
#ifndef OSALENTROPY_DRBG_CTR
    mpMdInfo,
#endif
    MbedCtrDrbgReseedC,
    this,
    (uint8_t *)"OsalAes", 7);

  this->mpSeedQ = NULL;

}

// ////////////////////////////////////////////////////////////////////////////
OsalDrbgEntropySrc::~OsalDrbgEntropySrc() {
  mbedtls_drbg_free(&mCtrDrbg);
}

// ////////////////////////////////////////////////////////////////////////////
void OsalDrbgEntropySrc::ForceReseed(const uint8_t * const pText, const int len) {
  CSTaskLocker lock;
  if ((pText) && (len > 0)) {
    ByteQ seedQ((uint8_t *)pText, len);
    seedQ.CommitWrite(len);
    this->mpSeedQ = &seedQ;
    if (!mUniqueEntropyReceived) {
      mUniqueEntropyReceived = true;
      // We should get initial reseed by deserialization, not basic entropy.
      LOG_VERBOSE(("Initial reseed by ForceReseed().  Good!\r\n"));
    }
    mbedtls_drbg_reseed(&mCtrDrbg, pText, len);
    this->mpSeedQ = NULL;
  }
  else {
    HashMoreText(pText, len);
  }
}

// ////////////////////////////////////////////////////////////////////////////
void OsalDrbgEntropySrc::ConsumeTextEntropy() {
  const bool noUniqueEntropy = (!mUniqueEntropyReceived);
  OsalShaEntropySrc::ConsumeTextEntropy();
  if (noUniqueEntropy) {
    mbedtls_drbg_reseed(&mCtrDrbg, NULL, 0);

  }
}

// ////////////////////////////////////////////////////////////////////////////
int OsalDrbgEntropySrc::GetEntropy(uint8_t *pbuf, const int entropyLen)
{
  int remaining = entropyLen;
  int bytesCreated = 0;

  while (remaining > 0) {
    CSTaskLocker lock;
    if (!mUniqueEntropyReceived) {
      LOG_ASSERT_WARN(mUniqueEntropyReceived);
      mUniqueEntropyReceived = true;
    }
    const int bytesToCreate = MIN(remaining, MBETLS_DRBG_MAX_REQUEST);
    mbedtls_drbg_random(&mCtrDrbg, &pbuf[bytesCreated], bytesToCreate);
    remaining -= bytesToCreate;
    bytesCreated += bytesToCreate;
  }

  return entropyLen;
}

// ////////////////////////////////////////////////////////////////////////////
// Called by mbedtls when the drbg generator needs new text.
int OsalDrbgEntropySrc::MbedCtrDrbgReseed(unsigned char *pData, size_t bytesToRead) {
  int rval = 0;

  int bytes = (int)bytesToRead;
  if (mpSeedQ) {

    rval = mpSeedQ->Read(pData, bytes);
    pData += rval;
    bytes -= rval;
  }

  if (bytes > 0) {
    ConsumeTextEntropy();

    int remaining = bytes;
    int iter = 0;
    while (remaining > 0) {
      HashMoreText();
      const int bytesToCopy = MIN(64, remaining);
      memcpy(&pData[iter], mHashPrev, bytesToCopy);
      remaining -= bytesToCopy;
      iter += bytesToCopy;
    }

    rval = (int)bytesToRead;
  }
    
  return (rval == (int)bytesToRead) ? 0 : -1;
}
