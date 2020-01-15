#ifndef CNV_UTILS_HPP
#define CNV_UTILS_HPP
/**
* COPYRIGHT (c)	Applicaudia 2017
* @file     cnv_utils.cpp
* @brief    Functions for converting to/from BASE64.
*  Utility functions that are dependent on the "big" dependencies such as
*  mbedtls or libsodium.
*/

#ifdef __cplusplus
#include "utils/simple_string.hpp"

void CNV_Base64EncBin(const uint8_t *pBin, const int binLen, sstring &asc);
void CNV_Base64Enc(const sstring &bin, sstring &asc);
void CNV_Base64DecAsc(const char *pAsc, const int ascLen, sstring &bin);
void CNV_Base64Dec(const sstring &asc, sstring &bin);

#endif


#endif