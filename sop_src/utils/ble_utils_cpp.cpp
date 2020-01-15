//
//  BLEUtils.cpp
//  opensesame
//
//  Created by Chris Fogelklou on 10/09/15.
//  Copyright © 2020 Acorn Technology. All rights reserved.
//
#include "utils/ble_utils.h"
#if (PLATFORM_FULL_OS > 0)

#include "utils/convert_utils.h"
#include "utils/helper_macros.h"
#include "utils/simple_string.hpp"
#include <ctype.h> // for toupper()
#include <deque>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "osal/osal.h"
LOG_MODNAME("ble_utils_cpp")

using namespace std;

extern "C" {

// ////////////////////////////////////////////////////////////////////////////
int BLEU_GetFileLen(const char *szFileName) {
  ifstream is;
  is.open(szFileName, ios::binary);

  // get length of file:
  is.seekg(0, ios::end);
  std::streamoff length = is.tellg();
  is.seekg(0, ios::beg);

  return (int)length;
}


// ////////////////////////////////////////////////////////////////////////////
void BLEU_PubKeyToUUID(const sstring &pubkey, sstring &uuid) {
  LOG_ASSERT(pubkey.length() >= 6);
  uuid.clear();
  uuid.append("HONEASAKEY");
  uuid.trim_null();
  sstring pubkeystr;
  CNV_BinToHexStr(pubkey.u_str(), pubkey.length(), pubkeystr);
  uuid.append(pubkeystr.u_str(), 6);
  uuid.trim_null();
  LOG_ASSERT(uuid.length() == BLEU_UUID_LEN);

}

// ////////////////////////////////////////////////////////////////////////////
// Takes incoming ASCII characters and converts to hexadecimal.
int BLEU_AsciiToHexBin(const uint8_t *const pAsciiIn, const int numChars,
                       uint8_t *pBinaryOut, const int binaryOutSize) {
  deque<uint8_t> nybbles;

  // Push all valid nybbles onto a list of nybbles.
  for (int i = 0; i < numChars; i++) {
    int bin = CNV_AscHexByteToBinNybble(pAsciiIn[i]);
    if (bin >= 0) {
      nybbles.push_back((uint8_t)bin);
    }
  }

  const size_t validNybbles = nybbles.size();
  int nybblePos = (validNybbles % 2) == 0 ? 0 : 1;
  uint8_t theByte = 0;
  int outIdx = 0;
  while (!nybbles.empty() && (outIdx < binaryOutSize)) {
    theByte |= nybbles.front();
    nybbles.pop_front();
    if ((++nybblePos & 0x01) == 0) {
      // Saved the LSNybble, so continue to next one.
      pBinaryOut[outIdx++] = (theByte);
      theByte = 0;
    } else {
      theByte <<= 4;
    }
  }

  // We should never end on an odd position.
  LOG_ASSERT(!((nybblePos & 0x01) && (outIdx < binaryOutSize)));

  return outIdx;
}
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Takes incoming ASCII characters and converts to hexadecimal.
int BLEU_StrAsciiToHex(const std::string &asc, std::string &binStr) {
  deque<uint8_t> nybbles;
  const size_t numChars = asc.length();
  const char *const pAscStr = asc.c_str();
  // Push all valid nybbles onto a list of nybbles.
  for (size_t i = 0; i < numChars; i++) {
    int bin = CNV_AscHexByteToBinNybble(pAscStr[i]);
    if (bin >= 0) {
      nybbles.push_back((uint8_t)bin);
    }
  }

  const size_t validNybbles = nybbles.size();
  int nybblePos = (validNybbles % 2) == 0 ? 0 : 1;
  uint8_t theByte = 0;
  int outIdx = 0;
  while (!nybbles.empty()) {
    theByte |= nybbles.front();
    nybbles.pop_front();
    if ((++nybblePos & 0x01) == 0) {
      // Saved the LSNybble, so continue to next one.
      binStr += (char)theByte;
      theByte = 0;
    } else {
      theByte <<= 4;
    }
  }

  // We should never end on an odd position.
  LOG_ASSERT(!(nybblePos & 0x01));

  return outIdx;
}

// ////////////////////////////////////////////////////////////////////////////
int BLEU_BinHexToAsciiHex(const std::string &binStr, std::string &ascStr) {
  ascStr.clear();
  const char *pc = binStr.data();
  const int len = (int)binStr.length();
  for (int i = 0; i < len; i++) {
    if ((pc[i] >= 0) && (pc[i] <= 9)) {
      ascStr.push_back('0' + pc[i]);
    } else if ((pc[i] >= 10) && (pc[i] <= 15)) {
      ascStr.push_back('a' + pc[i] - 0x0a);
    }
  }
  return len;
}

// ////////////////////////////////////////////////////////////////////////////
void BLEU_BinToAsciiHexString(const string &str, string &outAsciiHex) {
  outAsciiHex.clear();
  const char *pc = str.data();
  const int len = (int)str.length();
  for (int i = 0; i < len; i++) {
    char msn, lsn;
    CNV_BinByteToAscHexByte(pc[i], &msn, &lsn);
    outAsciiHex.push_back(msn);
    outAsciiHex.push_back(lsn);
  }
}

// ////////////////////////////////////////////////////////////////////////////
void BLEU_GetFileContentsAsAscii(string &filename, string &outAsciiHex) {
  fstream f(filename.c_str(), ios_base::binary | ios_base::in);
  outAsciiHex = "";
  if (f.is_open()) {
    char c;
    char msn, lsn;
    while (!f.eof()) {
      f.read(&c, 1);
      CNV_BinByteToAscHexByte(c, &msn, &lsn);
      outAsciiHex.push_back(msn);
      outAsciiHex.push_back(lsn);
    }
    f.close();
  }
}

extern "C" {
void BLEU_TraceFileContents(const char *szFname) {
  std::string contents;
  std::string filename(szFname);
  BLEU_GetFileContentsAsAscii(filename, contents);
  LOG_TRACE((contents.c_str()));
}
}
#endif

