#include "simple_string.hpp"
#include "util_vsnprintf.h"
#include "convert_utils.h"
#include "osal/endian_convert.h"
#include "utils/util_vsnprintf.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>


#include "platform_log.h"

LOG_MODNAME("util_printf.cpp");

// double stddev(int count, ...)

// vsnprintf((pb), (sz), (fmt), (va))
extern "C" {



#ifdef USE_UTIL_SPRINTF
// MARBEN needed real string support, which negates the need for our
// mini versions.
// ////////////////////////////////////////////////////////////////////////////
static void copyStrToDst(char *const dst, int &d, const int dEnd,
                         sstring &tmp) {
  tmp.trim_null();
  int i = 0;

  while ((i < tmp.length()) && (d < dEnd)) {
    dst[d++] = tmp[i++];
  }
}

// ////////////////////////////////////////////////////////////////////////////
int util_vsnprintf(char *dst, const size_t dstSize, const char *src, va_list va) {
  if ((NULL == src) || (NULL == dst)){
    return 0;
  }
  const int sEnd = strlen(src);
  const int dEnd = dstSize - 1;
  int s = 0;
  int d = 0;
  while ((s < sEnd) && (d < dEnd)) {
    switch (src[s]) {
    case '\\': {
      if (d < dEnd) {
        dst[d++] = src[s++];
      }
      if (d < dEnd) {
        dst[d++] = src[s++];
      }
    } break;

    case '%': {
      s++;
      const char specifier = src[s++];
      switch (specifier) {
      case 'f': {
        const double f = va_arg(va, double);
        sstring tmp;
        util_itoa((int)f, tmp.c8DataPtr(32, 32));
        copyStrToDst(dst, d, dEnd, tmp);
      } break;
      case 'd': {
        const int num = va_arg(va, int);
        sstring tmp;
        util_itoa(num, tmp.c8DataPtr(32, 32));
        copyStrToDst(dst, d, dEnd, tmp);
      } break;
      case 'u': {
        const unsigned int num = va_arg(va, unsigned int);
        sstring tmp;
        util_itoa(num, tmp.c8DataPtr(32, 32));
        copyStrToDst(dst, d, dEnd, tmp);
      } break;
      case 's': {
        const char *p = va_arg(va, char *);
        if (NULL == p){
          p = "NULL";
        }
        const int len = strlen(p);
        int i = 0;
        while ((i < len) && (d < dEnd)) {
          dst[d++] = p[i++];
        }
      } break;
      case 'c': {
        const char p = va_arg(va, int);
        dst[d++] = p;
      } break;
      case 'x': {
        const unsigned int num = va_arg(va, unsigned int);
        sstring tmp;
        const uint32_t be = HOSTTOBE32(num);
        CNV_BinToHexStr( (uint8_t *)&be, sizeof(be), tmp );
        copyStrToDst(dst, d, dEnd, tmp);
      } break;
      case '%': {
        if (d < dEnd) {
          dst[d++] = '%';
        }
      } break;
      default: {
        if (0 == memcmp("zu", &src[s - 1], 2)) {
          const size_t num = va_arg(va, size_t);
          s += (2-1);
          sstring tmp;
          util_itoa(num, tmp.c8DataPtr(32, 32));
          copyStrToDst(dst, d, dEnd, tmp);
        }
        else if (0 == memcmp("10zu", &src[s-1], 4)) {
          const size_t num = va_arg(va, size_t);
          s += (4 - 1);
          sstring tmp;
          util_itoa(num, tmp.c8DataPtr(32, 32));
          copyStrToDst(dst, d, dEnd, tmp);
        }
        else {
          const unsigned int num = va_arg(va, unsigned int);
          (void)num;
        }
      } break;
      };
    } break; // case '%'

    default: { dst[d++] = src[s++]; } break;
    } // switch(src[s])
  }   // while

  LOG_ASSERT(d < (int)dstSize);
  dst[d] = '\0';

  return d;
}

// ////////////////////////////////////////////////////////////////////////////
int util_snprintf( char * dst, const size_t dstSize, const char * sfmt, ...){
  va_list va;
  va_start(va, sfmt);
  const int rval = util_vsnprintf( dst, dstSize, sfmt, va );
  va_end(va);
  return rval;
}

#endif

/* reverse:  reverse string s in place */
// ////////////////////////////////////////////////////////////////////////////
void util_reverse(char s[])
{
  const int theLen = strlen(s);
  int i = 0;
  int j = theLen-1;
  while( i < j ) {
    const char c = s[i];
    s[i++] = s[j];
    s[j--] = c;
  }
}


// ////////////////////////////////////////////////////////////////////////////
static int util_uitoa_intern(const unsigned int theInt, char *const s, const int maxLen) {
  unsigned int n = theInt;
  int i = 0;
  do {
    s[i++] = n % 10 + '0'; 
    n /= 10;
  } while ((n > 0) && (i < maxLen));
  
  return i;
}

// ////////////////////////////////////////////////////////////////////////////
int util_uitoa(const unsigned int theInt, char *const s, const int sLen)
{
  int rval = 0;
  const int maxLen = sLen-1;
  if ((maxLen > 0) || (NULL != s)) {
    unsigned int n = theInt;
    int i = util_uitoa_intern( n, s, maxLen );
  
    if (i <= sLen-1) {
      s[i] = '\0';
    }
    
    util_reverse(s);

    rval = i;
  }
  else {
    if(s){
      s[0] = '\0';
    }
  }
  return rval;
}


// ////////////////////////////////////////////////////////////////////////////
int util_itoa(const int theInt, char *const s, const int sLen)
{
  int rval = 0;
  const int maxLen = sLen-2;
  if ((maxLen > 0) || (NULL != s)) {
  
    const int sign = (theInt >= 0) ? 1 : -1;
    int n = (sign < 0) ? -theInt : theInt;
    int i = util_uitoa_intern( n, s, maxLen );

    if ((i <= sLen-2) && (sign < 0)) {
      s[i++] = '-';
    }
    if (i <= sLen-1) {
      s[i] = '\0';
    }
    
    util_reverse(s);

    rval = i;

  }
  else {
    if(s){
      s[0] = '\0';
    }
  }
  return rval;
}

} // extern "C"
