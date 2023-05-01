/**
 * COPYRIGHT	(c)	Applicaudia 2018
 * @file        buf_io.cpp
 * @brief       See header file.
 */

#include "buf_io.hpp"

#include "osal/osal.h"
#include "utils/platform_log.h"
#include "utils/simple_string.hpp"
LOG_MODNAME("buf_io.cpp")

///////////////////////////////////////////////////////////////////////////////
//
BufIO::BufIO()
  : thisRef({ this })
  , pBleCApi(NULL) {
}

///////////////////////////////////////////////////////////////////////////////
//
BufIO::~BufIO() {
}

///////////////////////////////////////////////////////////////////////////////
//
int BufIO::Write(const uint8_t* const pBytes, const int numBytes) {
  (void)pBytes;
  (void)numBytes;
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
int BufIO::GetWriteReady() const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
int BufIO::Write(const sstring& buf) {
  return Write(buf.u_str(), (int)buf.length());
}

///////////////////////////////////////////////////////////////////////////////
//
int BufIO::Read(uint8_t* const pBytes, const int numBytes) {
  (void)pBytes;
  (void)numBytes;
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
int BufIO::GetReadReady() const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
int BufIO::Read(sstring& buf) {
  int rval            = 0;
  int readReady       = GetReadReady();
  uint8_t* const pBuf = buf.u8DataPtr(readReady, readReady);
  if (pBuf) {
    rval = Read(pBuf, readReady);
  }
  return rval;
}

///////////////////////////////////////////////////////////////////////////////
//
BufIOCRefT* BufIO::GetCRef() {
  return &this->thisRef;
}

///////////////////////////////////////////////////////////////////////////////
//
BufIO* BufIO::GetCppRef(const BufIOCRefT* const pCRef) {
  BufIO* prval = NULL;
  LOG_ASSERT(pCRef);
  if (pCRef) {
    LOG_ASSERT(pCRef->pThis);
    if (pCRef->pThis) {
      prval = static_cast<BufIO*>(pCRef->pThis);
    }
  }
  return prval;
}


///////////////////////////////////////////////////////////////////////////////
struct BLEIOBaseTag* BufIO::GetCBleCApi() {
  return this->pBleCApi;
}

///////////////////////////////////////////////////////////////////////////////
void BufIO::SetCBleCApi(struct BLEIOBaseTag* const pCApi) {
  LOG_ASSERT(NULL == this->pBleCApi);
  this->pBleCApi = pCApi;
}

extern "C" {
///////////////////////////////////////////////////////////////////////////////
//
int BufIOWrite(const BufIOCRefT* const pBaseIo, const uint8_t* const pBytes, const int numBytes) {
  BufIO* const pBleIo = static_cast<BufIO*>(pBaseIo->pThis);
  return pBleIo->Write(pBytes, numBytes);
}

///////////////////////////////////////////////////////////////////////////////
//
int BufIOGetWriteReady(const BufIOCRefT* const pBaseIo) {
  BufIO* const pBleIo = static_cast<BufIO*>(pBaseIo->pThis);
  return pBleIo->GetWriteReady();
}

///////////////////////////////////////////////////////////////////////////////
//
int BufIORead(const BufIOCRefT* const pBaseIo, uint8_t* const pBytes, const int numBytes) {
  BufIO* const pBleIo = static_cast<BufIO*>(pBaseIo->pThis);
  return pBleIo->Read(pBytes, numBytes);
}

///////////////////////////////////////////////////////////////////////////////
//
int BufIOGetReadReady(const BufIOCRefT* const pBaseIo) {
  BufIO* const pBleIo = static_cast<BufIO*>(pBaseIo->pThis);
  return pBleIo->GetReadReady();
}
}
