/**
* COPYRIGHT    (c)	Applicaudia 2020
* @file        osal_random.hpp
* @brief       Secure random numbers for PAK.
*/

#ifndef OSAL_RANDOM_HPP__
#define OSAL_RANDOM_HPP__

/* 
  RNG Module.

  Includes a psueudo-random-number-generator based on Salsa20.

  Entropy should be added by the application as it appears.
*/ 

#ifdef __cplusplus
#include "mbedtls/myconfig.h"
#include "mbedtls/entropy.h"
#include "osal_entropy.hpp"
#include "osal/singleton_defs.hpp"
#include "task_sched/task_rescheduler.hpp"
#include <stddef.h>
#include <stdint.h>


class sstring;
class OsalRandomParam;

#define OSALRANDOM_LIBSODIUM 0

class OsalRandom {
  static const int CACHED_ENTROPY_BYTES = MBEDTLS_ENTROPY_BLOCK_SIZE*8;
public:

  SINGLETON_DECLARATIONS(OsalRandom);

  // Reads some pre-generated random numbers.
  int Read(uint8_t *const pBytes, const int numBytes) ;

  // Gets the amount of entropy that can be read.
  int GetReadReady() ;

  // Add some entropy text.
  void QueueEntropyText(const uint8_t *const pBytes, const int numBytes);

  // Get a string representing the random buffer.
  void Serialize(sstring &out);

  // Deserialize the string.
  void Deserialize(const sstring &in);

  // Utilizes the built-in PRNG to generate some PRNG data.
  int mbedtls_hardware_poll(unsigned char *output, size_t len, size_t *olen);

private:

  friend class OsalRandomParam;

  // Constructor.  
  OsalRandom();
  ~OsalRandom();

  int OsalRegenEntropyNoCs();

  //TASKSCHED_CALLBACK_DECLARATION(OsalRandom, OsalRegenEntropyTask);
  void OsalRegenEntropyTask(uint32_t t);



private:
#ifdef __EMSCRIPTEN__
  mbedtls_entropy_context  * mpEntropyCtx;
  mbedtls_entropy_context  &mEntropyCtx;
#else
  mbedtls_entropy_context  mEntropyCtx;
#endif

  // A default entropy source to use together with mEntropyCtx.
  //OsalAesEntropySrc mDefaultEntropy;
  OsalShaEntropySrc mDefaultEntropy;

  sll::list mListOfEntropySrcs;

  TaskRescheduler mResched;
  ByteQ mEntropyCacheQ;     // Internal buffer of entropy bytes.
  
  uint8_t mCachedEntropyArr[CACHED_ENTROPY_BYTES];

};

#endif

#endif
