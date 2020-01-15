/**
* COPYRIGHT (c)	Applicaudia 2017
* @file     cnv_utils.cpp
* @brief    Functions for converting to/from BASE64.
*/

#include "mbedtls/base64.h"
#include "utils/cnv_utils.hpp"
#include "utils/platform_log.h"

LOG_MODNAME("cnv_utils.cpp");

void CNV_Base64Enc(const sstring &bin, sstring &asc){
  size_t olen = 0;
  asc.clear();
  mbedtls_base64_encode( NULL, 0, &olen, bin.u_str(), bin.length() );
  if (olen > 0){
    mbedtls_base64_encode( asc.u8DataPtr(olen, olen), olen, &olen, bin.u_str(), bin.length() );
  }
}

void CNV_Base64EncBin(const uint8_t *pBin, const int binLen, sstring &asc) {
  sstring tmp;
  tmp.assign_static((uint8_t *)pBin, binLen);
  CNV_Base64Enc(tmp, asc);
}

void CNV_Base64Dec(const sstring &asc, sstring &bin){
  size_t olen = 0;

  sstring tmp;
  tmp.assign_static((uint8_t *)asc.u_str(), asc.length());
  tmp.trim_null();

  bin.clear();
  mbedtls_base64_decode( NULL, 0, &olen, tmp.u_str(), tmp.length() );
  if (olen > 0){
    LOG_ASSERT(0 == mbedtls_base64_decode( bin.u8DataPtr(olen, olen), olen, &olen, tmp.u_str(), tmp.length() ));
  }
}

void CNV_Base64DecAsc(const char *pAsc, const int ascLen, sstring &bin) {
  sstring tmp;
  tmp.assign_static((uint8_t *)pAsc, ascLen);
  tmp.trim_null();
  CNV_Base64Dec(tmp, bin);
}
