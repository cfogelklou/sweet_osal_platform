#ifndef UTIL_VPRINTF
#define UTIL_VPRINTF

#include <stddef.h>
#include <stdarg.h>
#include "osal/platform_type.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef USE_UTIL_SPRINTF
int util_snprintf( char * dst, const size_t dstSize, const char * sfmt, ...);
int util_vsnprintf( char * dst, const size_t dstSize, const char * sfmt, va_list va );
#else
#include <stdio.h>
// MARBEN needed real string support, which negates the need for our
// mini versions.
#define util_snprintf snprintf
#define util_vsnprintf vsnprintf
#endif
void util_reverse(char s[]);
#ifdef __cplusplus
int util_itoa(const int theInt, char *const s, const int sLen = 20);
int util_uitoa(const unsigned int theInt, char *const s, const int sLen = 20);
#else
int util_itoa(const int theInt, char *const s, const int sLen);
int util_uitoa(const unsigned int theInt, char *const s, const int sLen);
#endif

#ifdef __cplusplus
}
#endif


#endif
