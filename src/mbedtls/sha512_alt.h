#ifndef MY_SHA512_ALT
#define MY_SHA512_ALT
#include "mbedtls/config.h"
#ifdef MBEDTLS_SHA512_ALT

#include "crypto_hash_sha512.h"

typedef struct
{
    crypto_hash_sha512_state    state;
    int is384;
}
mbedtls_sha512_context;

#ifdef __cplusplus
extern "C" {
#endif


/**
 * \brief          Initialize SHA-512 context
 *
 * \param ctx      SHA-512 context to be initialized
 */
void mbedtls_sha512_init( mbedtls_sha512_context *ctx );

/**
 * \brief          Clear SHA-512 context
 *
 * \param ctx      SHA-512 context to be cleared
 */
void mbedtls_sha512_free( mbedtls_sha512_context *ctx );

/**
 * \brief          Clone (the state of) a SHA-512 context
 *
 * \param dst      The destination context
 * \param src      The context to be cloned
 */
void mbedtls_sha512_clone( mbedtls_sha512_context *dst,
                           const mbedtls_sha512_context *src );

/**
 * \brief          SHA-512 context setup
 *
 * \param ctx      context to be initialized
 * \param is384    0 = use SHA512, 1 = use SHA384
 */
void mbedtls_sha512_starts( mbedtls_sha512_context *ctx, int is384 );

/**
 * \brief          SHA-512 process buffer
 *
 * \param ctx      SHA-512 context
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 */
void mbedtls_sha512_update( mbedtls_sha512_context *ctx, const unsigned char *input,
                    size_t ilen );

/**
 * \brief          SHA-512 final digest
 *
 * \param ctx      SHA-512 context
 * \param output   SHA-384/512 checksum result
 */
void mbedtls_sha512_finish( mbedtls_sha512_context *ctx, unsigned char output[64] );

#ifdef __cplusplus
}
#endif

#endif // MBEDTLS_SHA512_ALT

#endif