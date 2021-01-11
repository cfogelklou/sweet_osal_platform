#include "convert_utils.h"
#include "utils/helper_macros.h"
#include "utils/util_vsnprintf.h"
#include "utils/simple_string.hpp"
#include <string.h>
#include <ctype.h>

const char CNV_hexChars[] = "0123456789ABCDEF";
#include "utils/platform_log.h"

LOG_MODNAME("convert_utils.cpp")

extern "C" {

// ////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////
void CNV_BinByteToAscHexByte(const uint8_t bin, char *pMsn, char *pLsn) {
  const uint8_t msnybble = ((bin >> 4) & 0x0f);
  const uint8_t lsnybble = (bin & 0x0f);
  if (pMsn) {
    *pMsn = CNV_hexChars[msnybble];
  }
  if (pLsn) {
    *pLsn = CNV_hexChars[lsnybble];
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Converts the hexadecimal nybble in asc to a hex character
// ////////////////////////////////////////////////////////////////////////////////////////////////
int CNV_AscHexByteToBinNybble(const uint8_t asc) {
  int rval = -1;
  if ((asc >= '0') && (asc <= '9')) {
    rval = (asc - '0') + 0;
  } else if ((asc >= 'a') && (asc <= 'f')) {
    rval = (asc - 'a') + 10;
  } else if ((asc >= 'A') && (asc <= 'F')) {
    rval = (asc - 'A') + 10;
  }
  return (rval >= 0) ? (rval & 0x0f) : -1;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////
char *CNV_BinToHex(const uint8_t *const pBin, const size_t inBinLen,
                   char *const str, const size_t inStrLen) {
  const size_t binLen = MIN((inStrLen - 1) / 2, inBinLen);
  const size_t strLen = MIN(binLen * 2 + 1, inStrLen);
  const size_t sLenWithNull = strLen - 1;

  // Check that there is enough room for the whole string plus a \0 character.
  LOG_ASSERT_WARN((binLen * 2 + 1) <= strLen);

  memset(str, 0, sizeof(uint8_t) * strLen);

  size_t b = 0;
  size_t s = 0;
  while ((b < binLen) && (s < sLenWithNull)) {
    const uint8_t inByte = pBin[b++];
    str[s++] = CNV_hexChars[0x0f & (inByte >> 4)];
    if (s < sLenWithNull) {
      str[s++] = CNV_hexChars[0x0f & (inByte >> 0)];
    }
  }

  return &str[s];
}
}
// ////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////
const char *CNV_BinToHexStr(const uint8_t *const pBin, const size_t binLen,
                            sstring &hex, const bool toUpper) {
  const size_t strLen = binLen * 2 + 1;
  CNV_BinToHex(pBin, binLen, hex.c8DataPtr(strLen, strLen), strLen);
  if (!toUpper) {
    char *pc = hex.c8DataPtr();
    for (size_t i = 0; i < strLen; i++) {
      pc[i] = tolower(pc[i]);
    }
  }
  return hex.c_str();
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////
const char *CNV_BinToHexDelimStr(
  const uint8_t *const pBin, 
  const size_t binLen,
  const char *delimPre, 
  sstring &hex, 
  const char *delimPost){
  const size_t delimPreSize = (delimPre) ? strlen(delimPre) : 0;
  const size_t delimPostSize = (delimPost) ? strlen(delimPost) : 0;
  const size_t hexChars = (binLen * 2);
  const size_t numPreDelims = binLen;
  const size_t numPostDelims = (size_t)(MAX(0, (int)(binLen - 1)));
  const size_t strLen = hexChars + numPreDelims*delimPreSize + numPostDelims*delimPostSize + 1;
  CNV_BinToHexDelim(pBin, binLen, delimPre, hex.c8DataPtr(strLen, strLen), strLen, delimPost);
  hex.trim_null();
  hex.push_back(0);
  return hex.c_str();
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
static char isHex(const char c) {
  const char C = toupper(c);
  return ((C >= '0') && (C <= '9')) || ((C >= 'A') && (C <= 'F')) ? C : 0;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
int nibblify(const uint8_t ascUppercase) {
  return ((ascUppercase >= '0') && (ascUppercase <= '9'))
    ? (ascUppercase - '0') + 0
    : (ascUppercase - 'A') + 10;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////
const uint8_t *CNV_AsciiHexToBinStr(
  const char *const pAsciiHex,
  const size_t ascHexLenOrZero,
  sstring &bin) {
  bin.clear();
  size_t ascHexLen = (ascHexLenOrZero <= 0) ? strlen(pAsciiHex) : ascHexLenOrZero;
  sstring nybblesReverse;
  for (int idx = (int)ascHexLen-1; idx >= 0; idx--) {
    const char C = isHex(pAsciiHex[idx]);
    if (C) {
      const uint8_t n = nibblify(C);
      nybblesReverse.push_back(n);
    }    
  }
  int numNybbles = nybblesReverse.nlength();
  if (numNybbles & 1) {
    nybblesReverse.push_back(0);
    numNybbles += 1;
  }
  
  const uint8_t * const pNybbles = nybblesReverse.u_str();
  int wrIdx = numNybbles / 2;
  uint8_t *pWr = bin.u8DataPtr(wrIdx, wrIdx);
  
  for (int i = 0; i < numNybbles; i += 2) {
    uint8_t n = pNybbles[i + 1];
    n = (n << 4) | pNybbles[i + 0];
    pWr[--wrIdx] = n;
  }
  return bin.u_str();

}

// Removes any non-hex characters from the input string.
void CNV_CleanupAsciiHex(const sstring &in, sstring &out){
  out.clear();
  const char * buf = in.c_str();
  size_t len = strlen(in.c_str());
  len = MIN(len, (size_t)in.length());
  for (size_t i = 0; i < len; i++){
    const char c = isHex(buf[i]);
    if (c) {
      out.push_char(c);
    }
  }
  out.trim_null();
}

// 1b5c 1b44 fbde 11e8 8eb2 f280 1f1b 9fd1
// 1b5c1b44-fbde-11e8-8eb2-f2801f1b9fd1
void CNV_Uuidify(const char *inHex, sstring &out) {
  sstring tmp;
  out.clear();
  CNV_CleanupAsciiHex(inHex, tmp);
  const char *t = tmp.c_str();
  if (32 ==  tmp.length()){
    out.appendc(&t[ 0 ], 8);
    out.push_char('-');
    out.appendc(&t[ 8 ], 4);
    out.push_char('-');
    out.appendc(&t[ 12 ], 4);
    out.push_char('-');
    out.appendc(&t[ 16 ], 4);
    out.push_char('-');
    out.appendc(&t[ 20 ], 12);
  }
  out.push_back(0);
  out.trim_null();
}

extern "C" {
// ////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////
char *CNV_BinToHexDelim(const uint8_t *const pBin, const size_t binLen,
                        const char *delimPre, char *const str,
                        const size_t strLen, const char *delimPost) {
  size_t b = 0;
  size_t s = 0;
  const size_t sLenWithNull = strLen - 1;
  const size_t delimPreSize = (delimPre) ? strlen(delimPre) : 0;
  const size_t delimPostSize = (delimPost) ? strlen(delimPost) : 0;
  const size_t hexChars = (binLen * 2);
  const size_t numPreDelims = binLen;
  const size_t numPostDelims = (size_t)(MAX(0, (int)(binLen - 1)));
  const size_t strLenCmp = hexChars + numPreDelims*delimPreSize + numPostDelims*delimPostSize + 1;

  // Check that there is enough room for the whole string plus a \0 character.
  LOG_ASSERT_WARN(strLenCmp <= strLen);

  memset(str, 0, sizeof(uint8_t) * strLen);

  while ((b < binLen) && (s < sLenWithNull)) {
    if ((delimPreSize) && (s < (sLenWithNull - delimPreSize))) {
      memcpy(&str[s], delimPre, delimPreSize);
      s += delimPreSize;
    }
    const uint8_t inByte = pBin[b++];
    if (s <= (sLenWithNull - 2)) {
      str[s++] = CNV_hexChars[0x0f & (inByte >> 4)];
      if (s < sLenWithNull) {
        str[s++] = CNV_hexChars[0x0f & (inByte >> 0)];
      }
    }
    if ((delimPostSize) && (s < (sLenWithNull - delimPostSize))) {
      memcpy(&str[s], delimPost, delimPostSize);
      s += delimPostSize;
    }
  }

  return &str[s];
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////
char *CNV_convertArrayToHexWithColons(const uint8_t *const src,
                                      const int srcLen, uint8_t *const dst,
                                      const int dst_len) {

  uint8_t *pStr = dst;
  const uint8_t *pSrc = src;
  uint8_t avail = dst_len - 1;
  int src_len = srcLen;

  memset(dst, 0, avail);

  while (src_len && avail > 3) {
    if (avail < dst_len - 1) {
      *pStr++ = ':';
      avail -= 1;
    };
    *pStr++ = CNV_hexChars[*pSrc >> 4];
    *pStr++ = CNV_hexChars[*pSrc++ & 0x0F];
    avail -= 2;
    src_len--;
  }

  if (src_len && avail) {
    *pStr++ = ':'; // Indicate that we ran out of space.
  }

  return (char *)dst;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////
void CNV_convertBdAddr2Str(char *const strOut, const int strLen,
                           const uint8_t *const pAddr6) {
  uint8_t charCnt;
  const uint8_t *pAddr = pAddr6;
  LOG_ASSERT(strLen >= (CNV_BD_ADDR_LEN * 2));
  LOG_ASSERT(strOut);

  char *str = strOut;
  char *const strEnd = strOut + strLen;

  // Start from end of addr
  pAddr += CNV_BD_ADDR_LEN;

  for (charCnt = CNV_BD_ADDR_LEN; (str != strEnd) && (charCnt > 0); charCnt--) {
    *str++ = CNV_hexChars[*--pAddr >> 4];
    if (str == strEnd)
      break;
    *str++ = CNV_hexChars[*pAddr & 0x0F];
  }

  return;
}

const char *CNV_convertBdAddr2StrDbg(const uint8_t *const pAddr6) {
  static char out[CNV_BD_ADDR_LEN * 2 + 1] = {0};
  CNV_convertBdAddr2Str(out, CNV_BD_ADDR_LEN * 2 + 1, pAddr6);
  return out;
}


uint32_t CNV_atoi(const char * const pStr) {
  uint32_t rval = 0;
  const char *pC = pStr;
  while (*pC) {
    const char a = *pC;
    if ((a >= '0') && (a <= '9')) {
      const int num = a - '0';
      rval = (rval * 10) + num;
    }
    pC++;
  }
  return rval;
}

uint32_t CNV_htoi(const char * const pStr) {
  int rval = 0;
  const char *pC = pStr;
  while (*pC) {
    const char a = *pC;
    if ((a >= '0') && (a <= '9')) {
      const int num = a - '0';
      rval = (rval * 16) + num;
    }
    else if ((a >= 'a') && (a <= 'f')) {
      const int num = a - 'a' + 10;
      rval = (rval * 16) + num;
    }
    else if ((a >= 'A') && (a <= 'F')) {
      const int num = a - 'A' + 10;
      rval = (rval * 16) + num;
    }
    pC++;
  }
  return rval;
}

// Strip leading slashes
const char *CNV_stripSlash(const char * const fname) {
  const char *p = fname;
  if (fname) {
    const char *pLastSlash = nullptr;
    while (*p) {
      if ((*p == '\\') || (*p == '/')) {
        pLastSlash = p;
      }
      p++;
    }
    p = (pLastSlash) ? &pLastSlash[1] : fname;
  }
  return p;
}


// Precalculate some stuff.
static const int numDaysPerLeapCycle = (1 * 366) + (3 * 365);
static const int secondsPerDay = 60u * 60u * 24u;
static const int numSecondsPerLeapCycle = (numDaysPerLeapCycle * secondsPerDay);

// ////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////
void CNV_EpochTimestampToDateStruct(const uint64_t ts2,
                                    TimeAndDateT *const pTm) {
  LOG_ASSERT(pTm);
  if (NULL == pTm)
    return;

  // This code will only work on 32-bit or larger processors.
  ASSERT_AT_COMPILE_TIME(sizeof(int) >= 4);

#if 1
  // No timestamp offset.
  const int STARTING_YEAR = 1970;
  const int STARTING_DAY_OF_WEEK = E_Thursday;
  const int STARTING_TS = 0;
#else
  // Timestamp offset for Jan 1, 2012.
  const int STARTING_YEAR = 2012;
  const int STARTING_DAY_OF_WEEK = E_Sunday;
  const int STARTING_TS = 1325376000;
#endif

  const uint64_t secondsSinceStartingTs = ts2 - STARTING_TS;

  // Ensure that we aren't going backwards.
  // TODO: This will warn after year 2038.
  LOG_ASSERT_WARN(((int64_t)secondsSinceStartingTs) >= 0);

  // Calculate how many seconds disappear into entire leap cycles, and
  // compress that into years.
  const int numDaysSinceStartYear = (int)(secondsSinceStartingTs / secondsPerDay);
  pTm->weekday =
      (WeekdaysE)((STARTING_DAY_OF_WEEK + numDaysSinceStartYear) % 7);

  const int numLeapCyclesSinceStartYear =
      numDaysSinceStartYear / numDaysPerLeapCycle;

  pTm->year = STARTING_YEAR + numLeapCyclesSinceStartYear * 4;

  int64_t secondsRemaining = (int64_t)(secondsSinceStartingTs -
                         ((int64_t)numLeapCyclesSinceStartYear * (int64_t)numSecondsPerLeapCycle));
  LOG_ASSERT((secondsRemaining >= 0) &&
             (secondsRemaining < numSecondsPerLeapCycle));

  // Calculate now based on how many days since last leap cycle...
  const int daysSinceLastLeapCycle = (int)(secondsRemaining / secondsPerDay);
  int daysRemaining = daysSinceLastLeapCycle;

  // Remove daysRemaining from secondsRemaining.
  secondsRemaining -= (int64_t)daysRemaining * (int64_t)secondsPerDay;

  {
    // Decrement daysRemaining while iterating years, to figure out how many
    // years have passed since last leap cycle
    // TODO: Fix this before year 2100. This code will fail year 2100 since that is not a leap year.
    bool foundYear = false;
    const int yearsUntilNextLeapYear = (4 - (pTm->year % 4)) % 4;
    for (int y = 0; (!foundYear) && (y < 4); y++) {
      const int daysThisYear = (y == yearsUntilNextLeapYear) ? 366 : 365;
      daysRemaining -= daysThisYear;
      if (daysRemaining < 0) {
        daysRemaining += daysThisYear; // Restore the last subtraction
        foundYear = true;
      } else {
        pTm->year++;
      }
    }

    LOG_ASSERT((daysRemaining >= 0) && (daysRemaining < 366));
  }

  // Now figure out how many months into the year we are...
  const int daysInFebruary = (0 == (pTm->year % 4)) ? 29 : 28;
  const int daysPerMonth[] = {
      31, daysInFebruary, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  // Subtract days and increase months until we find our month.
  int month = -1;
  for (int m = 0; (month < 0) && (m < 12); m++) {
    daysRemaining -= daysPerMonth[m];
    if (daysRemaining < 0) {
      daysRemaining += daysPerMonth[m]; // Restore the last subtraction
      month = m;
    }
  }

  LOG_ASSERT((month >= 0) && (month < 12));
  pTm->month = (MonthsE)month; // 0..11

  LOG_ASSERT((daysRemaining >= 0) && (daysRemaining < 31));
  pTm->day = daysRemaining; // 0..30

  LOG_ASSERT(secondsRemaining < secondsPerDay);
  pTm->hour = (int)(secondsRemaining / (60 * 60));
  LOG_ASSERT((pTm->hour >= 0) && (pTm->hour < 24));

  secondsRemaining -= (int64_t)pTm->hour * (60 * 60);
  pTm->min = (int)(secondsRemaining / 60);
  LOG_ASSERT((pTm->min >= 0) && (pTm->min < 60));

  secondsRemaining -= (int64_t)pTm->min * 60;
  pTm->sec = (int)(secondsRemaining);
  LOG_ASSERT((pTm->sec >= 0) && (pTm->sec < 60));
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////
void CNV_EpochTimestampToString(const uint32_t ts, char *const pC,
                                const int strLen) {
  TimeAndDateT date;
  CNV_EpochTimestampToDateStruct(ts, &date);
  util_snprintf(pC, strLen, "%d/%d%d/%d%d %d%d:%d%d:%d%d", date.year,
                (date.month + 1) / 10, (date.month + 1) % 10,
                (date.day + 1) / 10, (date.day + 1) % 10, date.hour / 10,
                date.hour % 10, date.min / 10, date.min % 10, date.sec / 10,
                date.sec % 10);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Converts timestamp since Jan1 1970 to Generalized (ASN.1) time.
// YYYYMMDDHH[MM[SS[.fff]]]Z
// ////////////////////////////////////////////////////////////////////////////////////////////////
static void cnv_EpochTimestampToUTCGeneralizedTimeString(
  const uint32_t ts,
  char *const pC,
  const int strLen,
  const bool generalizedTime = true
  ) 
{
  TimeAndDateT date;
  CNV_EpochTimestampToDateStruct(ts, &date);
  const int year = (generalizedTime) ? date.year : date.year - 2000;
  util_snprintf(pC, strLen, "%d%d%d%d%d%d%d%d%d%d%dZ",
    year,
    (date.month + 1) / 10, (date.month + 1) % 10,
    (date.day + 1) / 10, (date.day + 1) % 10,
    date.hour / 10, date.hour % 10,
    date.min / 10, date.min % 10,
    date.sec / 10, date.sec % 10);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Converts timestamp since Jan1 1970 to Generalized UTC (ASN.1) time. (No milliseconds .fff)
// YYMMDDHH[MM[SS[.fff]]]Z
// ////////////////////////////////////////////////////////////////////////////////////////////////
void CNV_EpochTimestampToUTCGeneralizedTimeString(
  const uint32_t ts, 
  char *const pC,
  const int strLen) {
  cnv_EpochTimestampToUTCGeneralizedTimeString(ts, pC, strLen, true);
}

} // extern "C" {

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Converts timestamp since Jan1 1970 to UTC (ASN.1) time.
// YYMMDDhhmm[ss]Z
void CNV_EpochTimestampToUTCTimeString(
  const uint32_t ts,
  char *const pC,
  const int strLen,
  const bool generalizedTime) {
  cnv_EpochTimestampToUTCGeneralizedTimeString(ts, pC, strLen, generalizedTime);
}

extern "C" {
// ////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////
uint64_t CNV_EpochDateStructToTimestamp(TimeAndDateT *const pTm) {
  LOG_ASSERT(pTm);
  if (NULL == pTm)
    return 0;

  int year = 1970;
  int daysFrom1970ToNow = 0;
  const int yearsSince1970 = pTm->year - year;
  const int leapCycles = yearsSince1970 / 4;
  daysFrom1970ToNow += (leapCycles * numDaysPerLeapCycle);
  year += leapCycles * 4;
  while (year < pTm->year) {
    daysFrom1970ToNow += ((0 == (year % 4)) ? 366 : 365);
    year++;
  }
  
  const int daysInFebruary = (0 == (pTm->year % 4)) ? 29 : 28;
  const int daysPerMonth[] = {
    31, daysInFebruary, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  
  if ((int)pTm->month >= 12) {
    const int monthN = (int)pTm->month;
    LOG_ASSERT_WARN(monthN < 12);
    pTm->month = (MonthsE)(monthN % 12);
  }
  for (int m = 0; m < pTm->month; m++) {
    daysFrom1970ToNow += daysPerMonth[m];
  }

  daysFrom1970ToNow += pTm->day;
  uint64_t secondsFrom1970ToNow = ((uint64_t)daysFrom1970ToNow * (uint64_t)secondsPerDay);
  secondsFrom1970ToNow += (uint64_t)pTm->hour * 60ULL * 60ULL;
  secondsFrom1970ToNow += (uint64_t)pTm->min * 60ULL;
  secondsFrom1970ToNow += (uint64_t)pTm->sec;

  return secondsFrom1970ToNow;
}

// ////////////////////////////////////////////////////////////////////////////
// Decodes the following two formats, but ignores ".fff" if it exists.
// YYYYMMDDHH[MM[SS[.fff]]]Z
// YYMMDDHH[MM[SS[.fff]]]Z
uint32_t CNV_UTCTimeStringToTimestamp(const char * pC) {
  uint32_t ts = 0;
  sstring utc(pC);
  const int len = utc.nlength();
  const unsigned int end0 = (unsigned int)utc.index_of('Z');
  const unsigned int end1 = (unsigned int)utc.index_of('.');
  unsigned int end = MIN(end0, end1);
  end = MIN(end, (unsigned int)len);
  const char * const pBuf = utc.c_str();
  const char *c = &pBuf[end];
  TimeAndDateT date = { 0 };
  c -= 2;
  if (c >= pBuf) {
    sstring tmp(c, 2);
    date.sec = CNV_atoi(tmp.c_str());
    c -= 2;
  }
  if (c >= pBuf) {
    sstring tmp(c, 2);
    date.min = CNV_atoi(tmp.c_str());
    c -= 2;
  }
  if (c >= pBuf) {
    sstring tmp(c, 2);
    date.hour = CNV_atoi(tmp.c_str());
    c -= 2;
  }
  if (c >= pBuf) {
    sstring tmp(c, 2);
    date.day = CNV_atoi(tmp.c_str()) - 1;
    c -= 2;
  }
  if (c >= pBuf) {
    sstring tmp(c, 2);
    date.month = (MonthsE)(CNV_atoi(tmp.c_str()) - 1);
    c -= 2;
  }
  if (c >= pBuf) {
    sstring tmp(c, 2);
    date.year = CNV_atoi(tmp.c_str());
    c -= 2;
  }
  if (c >= pBuf) {
    sstring tmp(c, 2);
    date.year += 100 * CNV_atoi(tmp.c_str());
  }
  else {
    date.year += 2000;
  }
  ts = (uint32_t)(CNV_EpochDateStructToTimestamp(&date));
  return ts;
}

} // extern "C"


// ////////////////////////////////////////////////////////////////////////////
const char * CNV_EpochTimestampToSString(
  const uint32_t ts, 
  sstring &str,
  const bool generalizedTime
){
  if (!generalizedTime) {
    CNV_EpochTimestampToString(ts, str.c8DataPtr(32, 32), 32);    
  }
  else {
    CNV_EpochTimestampToUTCGeneralizedTimeString(ts, str.c8DataPtr(32, 32), 32);
  }
  str.trim_null();
  return str.c_str();
}
