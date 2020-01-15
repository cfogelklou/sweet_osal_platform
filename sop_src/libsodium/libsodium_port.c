/**
* 
* @file     libsodium_port.c
* @brief    Newer libsodium branches use sodium_misuse() as an assertion handler.
*/

#include "utils/platform_log.h"
#include "osal/platform_type.h"


LOG_MODNAME("sodium_misuse");

#if (PLATFORM_EMBEDDED > 0)
void sodium_misuse(void){
  LOG_ASSERT(0);
}
#endif


