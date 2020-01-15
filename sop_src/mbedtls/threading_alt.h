/**
* COPYRIGHT	(c)	Applicaudia 2017
* @file     threading_alt.h
* @brief    
*           Supports MBEDTLS memory allocation and 
*           deallocation with critical sections.
*/
#ifndef MBEDTLS_THREADING_ALT_H
#define MBEDTLS_THREADING_ALT_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_THREADING_ALT)

/* You should define the mbedtls_threading_mutex_t type in your header */
#include "osal/osal.h"
#include "utils/dl_list.h"
// Define the mutex type used by mbedtls to lock the mutex.
typedef struct mbedtls_threading_mutex_data_tag {
  DLLNode listNode;
  bool    isAValidMutex;
} mbedtls_threading_mutex_data_t;

typedef mbedtls_threading_mutex_data_t * mbedtls_threading_mutex_t;

#endif // #if defined(MBEDTLS_THREADING_ALT)

#endif // #ifndef MBEDTLS_THREADING_ALT_H
