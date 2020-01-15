//
//  BLEUtils.h
//  opensesame
//
//  Created by Chris Fogelklou on 10/09/15.
//  Copyright © 2020 Acorn Technology. All rights reserved.
//

#ifndef BLE_UTILS_H
#define BLE_UTILS_H
#include "osal/platform_type.h"
#include "utils/platform_log.h"
#include "utils/simple_string.hpp"
#if (PLATFORM_FULL_OS > 0)

#ifdef __cplusplus
#include <string>
// Takes incoming ASCII HEX characters and converts to binary hexadecimal.
int BLEU_StrAsciiToHex( const std::string &asc, std::string &binStr );

// Takes incoming BINARY HEX characters and converts to ascii hexadecimal.
int BLEU_BinHexToAsciiHex(const std::string &binStr, std::string &ascStr);

void BLEU_GetFileContentsAsAscii( std::string &filename, std::string &outAsciiHex );

void BLEU_BinToAsciiHexString(const std::string &str, std::string &outAsciiHex);

#endif // __cplusplus

#define BLEU_UUID_LEN 16

#ifdef __cplusplus
extern "C" {
#endif

// Utility to get the length of a file.
int BLEU_GetFileLen( const char *szFileName );
    
// Returns the registration number
void BLEU_PubKeyToUUID( const sstring &pubkey, sstring &UUID );

// Takes incoming ASCII characters and converts to hexadecimal.
int BLEU_AsciiToHexBin( const uint8_t * const pAsciiIn, const int numChars, uint8_t *pBinaryOut, const int binaryOutSize );

void BLEU_TraceFileContents( const char *szFname );

#ifdef __cplusplus
}
#endif

#endif // #if (PLATFORM_FULL_OS > 0)

#endif /* BLEUtils_h */
