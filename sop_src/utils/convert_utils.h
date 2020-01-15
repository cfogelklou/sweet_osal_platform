/**
* COPYRIGHT    (c)	Applicaudia 2020
* @file        convert_utils.h
* @brief       A simple string class for building up PaK messages without too many lines
*              of code.  Uses mempools instead of heap, so grows by powers of 2 to match
*              standard pool sizes.
*/
#ifndef CONVERT_UTILS_H__
#define CONVERT_UTILS_H__

#include <stddef.h>
#include <stdint.h>

extern const char CNV_hexChars[];

#ifdef __cplusplus
extern "C" {
#endif

// Converts the ascii byte in asc to a binary nybble.  
// Example: 'a' -> 0x0a, '0' -> 0, 'F' -> 0x0f
int CNV_AscHexByteToBinNybble(const uint8_t asc);

// Converts the binary byte in bin to an MSNybble and LSNybble
// Example: 0xa5 --> *pMsn = 'A', *pLsn = '5'
void CNV_BinByteToAscHexByte(const uint8_t bin, char *pMsn, char *pLsn);

// Converts the the binary data in src into hexadecimal ASCII, with colons separating
// the members.
// Example: {0xa5, 0x5a} --> "a5:5a"
char *CNV_convertArrayToHexWithColons(const uint8_t *const src,
                                      const int srcLen, uint8_t *const dst,
                                      const int dst_len);

// Converts the the binary data in pBin into hexadecimal with no delimiters
// Example: {0xa5, 0x5a} --> "a55a"
char *CNV_BinToHex(const uint8_t *const pBin, const size_t binLen,
                   char *const str, const size_t strLen);

// Converts the the binary data in pBin into hexadecimal with the delimiter specified in delim
// Example: (delim = " : ") {0xa5, 0x5a} --> "a5 : 5a"
char *CNV_BinToHexDelim(const uint8_t *const pBin, const size_t binLen,
                        const char *delim, char *const str,
                        const size_t strLen, const char *delimPost );


#define CNV_BD_ADDR_LEN 6
// Converts the the binary data in pAddr6 into a printable BD Address
// The assumption is that pAddr6 is be 6 bytes long.
void CNV_convertBdAddr2Str(char *const strOut, const int strLen,
                           const uint8_t *const pAddr6);

// Does CNV_convertBdAddr2Str(), but returns a static char *, for use directly in print statements.
// Obviously not thread safe.
const char *CNV_convertBdAddr2StrDbg(const uint8_t *const pAddr6);

// Atoi
uint32_t CNV_atoi(const char * const pStr);

// Htoi
uint32_t CNV_htoi(const char * const pStr);

// Strip leading slashes
const char *CNV_stripSlash(const char * const fname);

// Months of the year
typedef enum MonthsETag {
  E_January = 0,
  E_February,
  E_March,
  E_April,
  E_May,
  E_June,
  E_July,
  E_August,
  E_September,
  E_October,
  E_November,
  E_December
} MonthsE;

// Days of the week
typedef enum WeekdaysETag {
  E_Sunday = 0,
  E_Monday,
  E_Tuesday,
  E_Wednesday,
  E_Thursday,
  E_Friday,
  E_Saturday,
} WeekdaysE;

// Allows for conversion from epoch timestamp to time and date.
typedef struct TimeAndDateTag {
  int       year;    // 1970..now
  MonthsE   month;   // 0..11
  int       day;     // 0..30
  WeekdaysE weekday; // 0..6
  int       hour;    // 0..23
  int       min;     // 0..59
  int       sec;     // 0..59
} TimeAndDateT;

// Converts timestamp since Jan1 1970 to a struct with the real date.
void CNV_EpochTimestampToDateStruct(const uint64_t ts, TimeAndDateT *const pTm);

// Converts timestamp since Jan1 1970 to a date string.
void CNV_EpochTimestampToString(
  const uint32_t ts, 
  char *const pC,
  const int strLen);

// Converts timestamp since Jan1 1970 to GeneralizedTime (UTC) (ASN.1).
void CNV_EpochTimestampToUTCGeneralizedTimeString(
  const uint32_t ts, 
  char *const pC,
  const int strLen);

// Converts a UTC timestamp (YYMMDDHHmmssZ) to 32-bit epoch seconds.
uint32_t CNV_UTCTimeStringToTimestamp(const char * pC);

// Converts date struct back to an epoch timestamp in seconds since Jan1, 1970.
uint64_t CNV_EpochDateStructToTimestamp(TimeAndDateT *const pTm);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
class sstring; // Forward declaration

// Converts the binary data in pBin into hexadecimal, and puts the result in hex
// Example: {0xa5, 0x5a} --> "a55a"
const char *CNV_BinToHexStr(const uint8_t *const pBin, const size_t binLen,
                            sstring &hex, const bool toUpper = true);

// Converts the binary data in pBin into delimited hexadecimal, and puts the result in hex
// Example: (delim = " : ") {0xa5, 0x5a} --> "a5 : 5a"
const char *CNV_BinToHexDelimStr(const uint8_t *const pBin, const size_t binLen,
                                 const char *delimPre, sstring &hex, const char *delimPost = NULL);

// Converts incoming ASCII hex (without any delimiters) into binary
const uint8_t *CNV_AsciiHexToBinStr(const char *const pAsciiHex,
                                    const size_t ascHexLen, sstring &bin);

// Removes any non-hex characters from the input string.
void CNV_CleanupAsciiHex(const sstring &in, sstring &out);

void CNV_Uuidify(const char *inHex, sstring &out);

// Converts timestamp since Jan1 1970 to a date string.
const char * CNV_EpochTimestampToSString(
  const uint32_t ts, 
  sstring &str,
  const bool generalizedTime = false);

// Converts timestamp since Jan1 1970 to GeneralizedTime (UTC) (ASN.1).
void CNV_EpochTimestampToUTCTimeString(
  const uint32_t ts,
  char *const pC,
  const int strLen,
  const bool generalizedTime = false
);

#endif

#endif // #define CONVERT_UTILS_H__
