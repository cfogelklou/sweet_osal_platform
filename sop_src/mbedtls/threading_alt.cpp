/**
* COPYRIGHT	(c)	Applicaudia 2018
* @file     threading_alt.cpp
* @brief    
*           See header file.
*/
#include "mbedtls/myconfig.h"
#include "threading_alt.h"
#include "utils/platform_log.h"

LOG_MODNAME("threading_alt.cpp")

#if defined(MBEDTLS_THREADING_ALT)
// ///////////////////////////////////////////////////////////////////////////////////
// Define basic CS lockers so that mbedtls memory allocation routines can stop

#include "utils/dl_list.hpp"
#include "utils/helper_macros.h"
#include "mbedtls/threading.h"
#include "mbedtls/platform.h"
#include "osal/cs_task_locker.hpp"

static mbedtls_threading_mutex_data_t mbedtls_mutexes[16] = { { { 0 } } };
static dll::list mbedtls_freeMutexes;
static dll::list mbedtls_usedMutexes;

#if defined(MBEDTLS_FS_IO)
// Two globals that aren't actually used by mbedtls, but defined anyway.
mbedtls_threading_mutex_t mbedtls_threading_readdir_mutex = nullptr;
mbedtls_threading_mutex_t mbedtls_threading_gmtime_mutex = nullptr;
#endif

// ///////////////////////////////////////////////////////////////////////////////////
static void mbedtls_mutex_init_fn(mbedtls_threading_mutex_t *mutex) {
  if (mutex) {
    CSTaskLocker cs;
    mbedtls_threading_mutex_data_t *pnode =
      (mbedtls_threading_mutex_data_t *)mbedtls_freeMutexes.pop_front();
    LOG_ASSERT(pnode);
    if (pnode) {
      pnode->isAValidMutex = true;
      *mutex = pnode;
    }
  }
}

// ///////////////////////////////////////////////////////////////////////////////////
static void mbedtls_mutex_free_fn(mbedtls_threading_mutex_t *mutex) {
  if (mutex) {
    mbedtls_threading_mutex_data_t *pnode = *mutex;
    if (pnode) {
      CSTaskLocker cs;
      DLL_NodeUnlist(&pnode->listNode);
      mbedtls_freeMutexes.push_back(&pnode->listNode);
    }
    *mutex = nullptr;
  }
}

// ///////////////////////////////////////////////////////////////////////////////////
static int  mbedtls_mutex_lock_fn(mbedtls_threading_mutex_t *mutex) {
  if (mutex) {
    mbedtls_threading_mutex_data_t *pnode = *mutex;
    if (pnode) {
      if (pnode->isAValidMutex) {
        OSALEnterTaskCritical();
      }
    }
  }
  return 0;
}

// ///////////////////////////////////////////////////////////////////////////////////
static int  mbedtls_mutex_unlock_fn(mbedtls_threading_mutex_t *mutex) {
  if (mutex) {
    mbedtls_threading_mutex_data_t *pnode = *mutex;
    if (pnode) {
      if (pnode->isAValidMutex) {
        OSALExitTaskCritical();
      }
    }
  }
  return 0;
}

// ///////////////////////////////////////////////////////////////////////////////////
static void mempools_initMbedMutexes() {
  if (mbedtls_mutex_init != mbedtls_mutex_init_fn) {

    // Initialize API functions used by mbedtls.
    mbedtls_mutex_init = mbedtls_mutex_init_fn;
    mbedtls_mutex_free = mbedtls_mutex_free_fn;
    mbedtls_mutex_lock = mbedtls_mutex_lock_fn;
    mbedtls_mutex_unlock = mbedtls_mutex_unlock_fn;

    // Initialize list of free mutexes
    for (int i = 0; i < ARRSZN(mbedtls_mutexes); i++) {
      mbedtls_threading_mutex_data_t *pnode = &mbedtls_mutexes[i];
      DLL_NodeInit(&pnode->listNode);
      mbedtls_freeMutexes.push_back(&pnode->listNode);
    }
  }
}

// ///////////////////////////////////////////////////////////////////////////////////
void MbedInitThreadingAlt() {
  static bool initialized = false;
  if (initialized) return;
  initialized = true;
  if (mbedtls_mutex_init != mbedtls_mutex_init_fn) {
    mempools_initMbedMutexes();
  }

#if defined(MBEDTLS_FS_IO)
  mbedtls_threading_mutex_data_t *pnode
    = (mbedtls_threading_mutex_data_t *)mbedtls_freeMutexes.pop_front();
  // Set isValid to FALSE so that any attempts to lock won't actually block processing.
  pnode->isAValidMutex = false;
  mbedtls_usedMutexes.push_back(&pnode->listNode);

  mbedtls_threading_readdir_mutex = pnode;
  mbedtls_threading_gmtime_mutex = pnode;
#endif
}

void OSALDeInitMbedMutexes() {
  mbedtls_threading_free_alt();
}

#else

void MbedInitThreadingAlt() {
}

void OSALDeInitMbedMutexes() {
}

#endif
