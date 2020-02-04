//
//  helper_utils.h
//  opensesame
//
//  Created by Chris Fogelklou on 10/09/15.
//  Copyright © 2015 Applicaudia. All rights reserved.
//

#ifndef HELPER_UTILS_H
#define HELPER_UTILS_H
#include "osal/platform_type.h"
#include "utils/platform_log.h"
#include "utils/simple_string.hpp"
#if (PLATFORM_FULL_OS > 0)

#ifdef __cplusplus
#include <string>

// Takes incoming ASCII HEX characters and converts to binary hexadecimal.
int HELPER_StrAsciiToHex( const std::string &asc, std::string &binStr );

// Takes incoming BINARY HEX characters and converts to ascii hexadecimal.
int HELPER_BinHexToAsciiHex(const std::string &binStr, std::string &ascStr);

void HELPER_GetFileContentsAsAscii( std::string &filename, std::string &outAsciiHex );

void HELPER_BinToAsciiHexString(const std::string &str, std::string &outAsciiHex);

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

// Takes incoming ASCII characters and converts to hexadecimal.
int HELPER_AsciiToHexBin( const uint8_t * const pAsciiIn, const int numChars, uint8_t *pBinaryOut, const int binaryOutSize );

void HELPER_TraceFileContents( const char *szFname );

#ifdef __cplusplus
}
#endif

#endif // #if (PLATFORM_FULL_OS > 0)

#endif /* helper_utils */
