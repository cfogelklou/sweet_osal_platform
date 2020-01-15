#include "mbedtls/config.h"
#include "mbedtls/sha256.h"
#include "sha256_alt.h"
#include "mbedtls/sha512.h"
#include "sha512_alt.h"
#include "utils/platform_log.h"
#include <string.h>

LOG_MODNAME("sha_alt.cpp");

// ////////////////////////////////////////////////////////////////////////////
// Implement SHA256
#ifdef MBEDTLS_SHA256_ALT


extern "C" {

#ifdef LIBCRYPTO_EXTERNAL
#include "src/ecu/pak_spc564b_crypto/libsodium_struct.h"

  void mbedtls_sha256_init(mbedtls_sha256_context *ctx) {
    libSodiumApi.pSha256->mbedtls_sha256_init(ctx);
  }
  void mbedtls_sha256_free(mbedtls_sha256_context *ctx) {
    libSodiumApi.pSha256->mbedtls_sha256_free(ctx);
  }
  void mbedtls_sha256_clone(mbedtls_sha256_context *dst,
    const mbedtls_sha256_context *src) {
    libSodiumApi.pSha256->mbedtls_sha256_clone(dst, src);
  }
  int mbedtls_sha256_starts_ret(mbedtls_sha256_context *ctx, int is224) {
    return libSodiumApi.pSha256->mbedtls_sha256_starts_ret(ctx, is224);
  }
  int mbedtls_sha256_update_ret(mbedtls_sha256_context *ctx,
    const unsigned char *input,
    size_t ilen) {
    return libSodiumApi.pSha256->mbedtls_sha256_update_ret(ctx, input, ilen);
  }
  int mbedtls_sha256_finish_ret(mbedtls_sha256_context *ctx,
    unsigned char output[32]) {
    return libSodiumApi.pSha256->mbedtls_sha256_finish_ret(ctx, output);
  }
  int mbedtls_internal_sha256_process(mbedtls_sha256_context *ctx,
    const unsigned char data[64])
  {
    return libSodiumApi.pSha256->mbedtls_internal_sha256_process(ctx, data);
  }

#else // LIBCRYPTO_EXTERNAL

/**
 * \brief          Initialize SHA-256 context
 *
 * \param ctx      SHA-256 context to be initialized
 */
void mbedtls_sha256_init( mbedtls_sha256_context *ctx ){
  LOG_ASSERT(ctx);
  if (ctx){
    memset( ctx, 0, sizeof(*ctx) );
    crypto_hash_sha256_init( &ctx->state );
  }
}

/**
 * \brief          Clear SHA-256 context
 *
 * \param ctx      SHA-256 context to be cleared
 */
void mbedtls_sha256_free( mbedtls_sha256_context *ctx ){
  LOG_ASSERT(ctx);
}


/**
 * \brief          Clone (the state of) a SHA-256 context
 *
 * \param dst      The destination context
 * \param src      The context to be cloned
 */
void mbedtls_sha256_clone( mbedtls_sha256_context *dst,
                           const mbedtls_sha256_context *src )
{
  LOG_ASSERT(dst);
  LOG_ASSERT(src);
  if ((NULL != src) && (NULL != dst)) {
    *dst = *src;
  }
}


/**
 * \brief          SHA-256 context setup
 *
 * \param ctx      context to be initialized
 * \param is224    0 = use SHA256, 1 = use SHA224
 */
int mbedtls_sha256_starts_ret( mbedtls_sha256_context *ctx, int is224 ){
  int rval = -1;
  LOG_ASSERT(ctx);
  if (ctx){
    rval = 0;
    ctx->is224 = (is224) ? 1 : 0;
    if (is224) {
      /* SHA-224 */
      ctx->state.state[0] = 0xC1059ED8;
      ctx->state.state[1] = 0x367CD507;
      ctx->state.state[2] = 0x3070DD17;
      ctx->state.state[3] = 0xF70E5939;
      ctx->state.state[4] = 0xFFC00B31;
      ctx->state.state[5] = 0x68581511;
      ctx->state.state[6] = 0x64F98FA7;
      ctx->state.state[7] = 0xBEFA4FA4;
    }
  }
  return rval;
}



/**
 * \brief          SHA-256 process buffer
 *
 * \param ctx      SHA-256 context
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 */
int mbedtls_sha256_update_ret( mbedtls_sha256_context *ctx, const unsigned char *input,
                    size_t ilen ){
  int rval = -1;
  LOG_ASSERT(ctx);
  if (ctx){
    rval = crypto_hash_sha256_update( &ctx->state, input, ilen );
  }
  return rval;
}



/**
 * \brief          SHA-256 final digest
 *
 * \param ctx      SHA-256 context
 * \param output   SHA-224/256 checksum result
 */
int mbedtls_sha256_finish_ret( mbedtls_sha256_context *ctx, unsigned char output[32] ){
  int rval = -1;
  LOG_ASSERT(ctx);
  if (ctx){
    if (ctx->is224) {
      uint8_t tmp[32];
      rval = crypto_hash_sha256_final(&ctx->state, tmp);
      memcpy(output, tmp, 224 / 8);
    }
    else {
      LOG_ASSERT(0 == ctx->is224);
      rval = crypto_hash_sha256_final(&ctx->state, output);
    }
  }
  return rval;
}



/* Internal use */
int mbedtls_internal_sha256_process( mbedtls_sha256_context *ctx, const unsigned char data[64] ){
  (void)ctx;
  (void)data;
  LOG_ASSERT(false); // should not be called.
  return -1;
}





#endif // LIBCRYPTO_EXTERNAL

void mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224) {
  (void)mbedtls_sha256_starts_ret(ctx, is224);
}

void mbedtls_sha256_update(mbedtls_sha256_context *ctx, const unsigned char *input,
  size_t ilen) {
  (void)mbedtls_sha256_update_ret(ctx, input, ilen);
}

void mbedtls_sha256_finish(mbedtls_sha256_context *ctx, unsigned char output[32]) {
  (void)mbedtls_sha256_finish_ret(ctx, output);
}
void mbedtls_sha256_process(mbedtls_sha256_context *ctx, const unsigned char data[64]) {
  (void)ctx;
  (void)data;
  LOG_ASSERT(false); // should not be called.
}

} // Extern "C" 
#endif // #ifdef MBEDTLS_SHA256_ALT


#ifdef MBEDTLS_SHA512_ALT
extern "C" {

#ifdef LIBCRYPTO_EXTERNAL

  void mbedtls_sha512_init(mbedtls_sha512_context *ctx) {
    libSodiumApi.pSha512->mbedtls_sha512_init(ctx);
  }
  void mbedtls_sha512_free(mbedtls_sha512_context *ctx) {
    libSodiumApi.pSha512->mbedtls_sha512_free(ctx);
  }
  void mbedtls_sha512_clone(mbedtls_sha512_context *dst,
    const mbedtls_sha512_context *src) {
    libSodiumApi.pSha512->mbedtls_sha512_clone(dst, src);
  }
  int mbedtls_sha512_starts_ret(mbedtls_sha512_context *ctx, int is384) {
    return libSodiumApi.pSha512->mbedtls_sha512_starts_ret(ctx, is384);
  }
  int mbedtls_sha512_update_ret(mbedtls_sha512_context *ctx,
    const unsigned char *input,
    size_t ilen) {
    return libSodiumApi.pSha512->mbedtls_sha512_update_ret(ctx, input, ilen);
  }
  int mbedtls_sha512_finish_ret(mbedtls_sha512_context *ctx,
    unsigned char output[64]) {
    return libSodiumApi.pSha512->mbedtls_sha512_finish_ret(ctx, output);
  }
  int mbedtls_internal_sha512_process(mbedtls_sha512_context *ctx,
    const unsigned char data[128])
  {
    return libSodiumApi.pSha512->mbedtls_internal_sha512_process(ctx, data);
  }

#else // LIBCRYPTO_EXTERNAL
// ////////////////////////////////////////////////////////////////////////////
// Implement SHA512



/**
 * \brief          Initialize SHA-512 context
 *
 * \param ctx      SHA-512 context to be initialized
 */
void mbedtls_sha512_init( mbedtls_sha512_context *ctx ){
  LOG_ASSERT(ctx);
  if (ctx){
    memset( ctx, 0, sizeof(*ctx) );
    crypto_hash_sha512_init( &ctx->state );
  }
}

/**
 * \brief          Clear SHA-512 context
 *
 * \param ctx      SHA-512 context to be cleared
 */
void mbedtls_sha512_free( mbedtls_sha512_context *ctx ){
  LOG_ASSERT(ctx);
  if (ctx){
  }
}

/**
 * \brief          Clone (the state of) a SHA-512 context
 *
 * \param dst      The destination context
 * \param src      The context to be cloned
 */
void mbedtls_sha512_clone( mbedtls_sha512_context *dst,
                           const mbedtls_sha512_context *src ){
  LOG_ASSERT(dst);
  LOG_ASSERT(src);
  if ((NULL != src) && (NULL != dst)) {
    *dst = *src;
  }
}

#if defined(_MSC_VER) || defined(__WATCOMC__)
#define UL64(x) x##ui64
#else
#define UL64(x) x##ULL
#endif

/**
 * \brief          SHA-512 context setup
 *
 * \param ctx      context to be initialized
 * \param is384    0 = use SHA512, 1 = use SHA384
 */
int mbedtls_sha512_starts_ret( mbedtls_sha512_context *ctx, int is384 ){
  int rval = -1;
  LOG_ASSERT(ctx);
  if (ctx){
    rval = 0;
    ctx->is384 = (is384) ? 1 : 0;
    if (is384) {
      /* SHA-384 */
      ctx->state.state[0] = UL64(0xCBBB9D5DC1059ED8);
      ctx->state.state[1] = UL64(0x629A292A367CD507);
      ctx->state.state[2] = UL64(0x9159015A3070DD17);
      ctx->state.state[3] = UL64(0x152FECD8F70E5939);
      ctx->state.state[4] = UL64(0x67332667FFC00B31);
      ctx->state.state[5] = UL64(0x8EB44A8768581511);
      ctx->state.state[6] = UL64(0xDB0C2E0D64F98FA7);
      ctx->state.state[7] = UL64(0x47B5481DBEFA4FA4);
    }
  }
  return rval;

}



/**
 * \brief          SHA-512 process buffer
 *
 * \param ctx      SHA-512 context
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 */
int mbedtls_sha512_update_ret( mbedtls_sha512_context *ctx, const unsigned char *input,
                    size_t ilen ){
  int rval = -1;
  LOG_ASSERT(ctx);
  if (ctx){
    rval = crypto_hash_sha512_update( &ctx->state, input, ilen );
  }
  return rval;
}



/**
 * \brief          SHA-512 final digest
 *
 * \param ctx      SHA-512 context
 * \param output   SHA-384/512 checksum result
 */
int mbedtls_sha512_finish_ret( mbedtls_sha512_context *ctx, unsigned char output[64] ){
  int rval = -1;
  LOG_ASSERT(ctx);
  if (ctx) {
    if (ctx->is384) {
      uint8_t tmp[512 / 8];
      rval = crypto_hash_sha512_final(&ctx->state, tmp);
      memcpy(output, tmp, 384 / 8);
    }
    else {
      rval = crypto_hash_sha512_final(&ctx->state, output);
    }
  }
  return rval;
}

int mbedtls_internal_sha512_process(mbedtls_sha512_context *ctx, const unsigned char data[128]) {
  (void)ctx;
  (void)data;
  LOG_ASSERT(false); // should not be called.
  return -1;
}



#endif // #else // LIBCRYPTO_EXTERNAL

void mbedtls_sha512_starts(mbedtls_sha512_context *ctx, int is384) {
  (void)mbedtls_sha512_starts_ret(ctx, is384);
}

void mbedtls_sha512_update(mbedtls_sha512_context *ctx, const unsigned char *input,
  size_t ilen) {
  (void)mbedtls_sha512_update_ret(ctx, input, ilen);
}

void mbedtls_sha512_finish(mbedtls_sha512_context *ctx, unsigned char output[64]) {
  (void)mbedtls_sha512_finish_ret(ctx, output);
}

/* Internal use */
void mbedtls_sha512_process(mbedtls_sha512_context *ctx, const unsigned char data[128]) {
  (void)ctx;
  (void)data;
  LOG_ASSERT(false); // should not be called.
}
} // Extern "C" 

#endif // #ifdef MBEDTLS_SHA512_ALT


