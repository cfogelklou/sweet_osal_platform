#include "mbedtls/config.h"

#include "utils/platform_log.h"
#include "utils/util_vsnprintf.h"
#include "osal/osal.h"

#if (PLATFORM_FULL_OS > 0)
#endif

LOG_MODNAME("mbedtls_port.c")


extern "C" {


#ifdef MBEDTLS_PLATFORM_STD_FPRINTF
// ////////////////////////////////////////////////////////////////////////////
int pak_mbedtls_fprintf( FILE *stream, const char *format, ... ){
  // Do nada.
  (void)stream;
  (void)format;
  return 0;  
}
#endif

#ifdef MBEDTLS_PLATFORM_STD_PRINTF
// ////////////////////////////////////////////////////////////////////////////
int pak_mbedtls_printf( const char * sfmt, ... ){
  va_list va;
  va_start(va, sfmt);
  char tmp[60];
  const int rval = util_vsnprintf( tmp, sizeof(tmp), sfmt, va );
  LOG_Log("mbed:%s", tmp);
  va_end(va);
  return rval;
}
#endif

#ifdef MBEDTLS_PLATFORM_STD_SNPRINTF
// ////////////////////////////////////////////////////////////////////////////
int pak_mbedtls_snprintf( char * s, size_t n, const char * sfmt, ... ) {
  va_list va;
  va_start(va, sfmt);
  const int rval = util_vsnprintf( s, n, sfmt, va );
  va_end(va);
  return rval;
}
#endif

}


