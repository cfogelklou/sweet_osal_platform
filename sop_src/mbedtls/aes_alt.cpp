#include "mbedtls/myconfig.h"
#if defined(MBEDTLS_AES_ALT) || defined(MBEDTLS_AES_ENCRYPT_ALT) || defined(MBEDTLS_AES_DECRYPT_ALT)

#include "mbedtls/aes.h"
#include "utils/platform_log.h"

#ifndef __EMBEDDED_MCU_BE__
#include "crypto/crypto_aes_cifra.hpp"
#else
#include "crypto/crypto_aes_mbedtls.hpp"
#endif
#include "extern_libs/cifra/src/modes.h"
#include <string.h>

LOG_MODNAME("aes_alt.cpp");
extern "C" {


#ifdef MBEDTLS_AES_ALT


  static void cf_prp_encrypt_block(void *ctx, const uint8_t *in, uint8_t *out) {
    mbedtls_aes_context *pThis = (mbedtls_aes_context *)ctx;
    CryptoAes &enc = *(CryptoAes *)pThis->pEnc;
    enc.EncryptBlock(in, out);
  }

  static void cf_prp_decrypt_block(void *ctx, const uint8_t *in, uint8_t *out) {
    mbedtls_aes_context *pThis = (mbedtls_aes_context *)ctx;
    CryptoAes &dec = *(CryptoAes *)pThis->pDec;
    dec.DecryptBlock(in, out);
  }

/**
 * \brief          Initialize AES context
 *
 * \param ctx      AES context to be initialized
 */
void mbedtls_aes_init( mbedtls_aes_context *ctx ){
#ifndef __EMBEDDED_MCU_BE__
  ASSERT_AT_COMPILE_TIME(sizeof(CryptoAesBG) < sizeof(ctx->enclassMem));
  ASSERT_AT_COMPILE_TIME(sizeof(CryptoAesBG) < sizeof(ctx->declassMem));
  LOG_ASSERT(sizeof(CryptoAesBG) < sizeof(ctx->enclassMem));
  LOG_ASSERT(sizeof(CryptoAesBG) < sizeof(ctx->declassMem));
  ctx->pEnc = new (ctx->enclassMem) CryptoAesBG(true);
  ctx->pDec = new (ctx->declassMem) CryptoAesBG(false);
#else
  ASSERT_AT_COMPILE_TIME(sizeof(CryptoAesMbedTls) < sizeof(ctx->enclassMem));
  ASSERT_AT_COMPILE_TIME(sizeof(CryptoAesMbedTls) < sizeof(ctx->declassMem));
  LOG_ASSERT(sizeof(CryptoAesMbedTls) < sizeof(ctx->enclassMem));
  LOG_ASSERT(sizeof(CryptoAesMbedTls) < sizeof(ctx->declassMem));
  ctx->pEnc = new (ctx->enclassMem) CryptoAesMbedTls();
  ctx->pDec = new (ctx->declassMem) CryptoAesMbedTls();

#endif

  ctx->prp.encrypt = cf_prp_encrypt_block;
  ctx->prp.decrypt = cf_prp_decrypt_block;
  ctx->prp.blocksz = 16;

}

/**
 * \brief          Clear AES context
 *
 * \param ctx      AES context to be cleared
 */
void mbedtls_aes_free( mbedtls_aes_context *ctx ){
  if( ctx == NULL )
      return;
  memset( ctx, 0, sizeof( mbedtls_aes_context ) );
}

/**
 * \brief          AES key schedule (encryption)
 *
 * \param ctx      AES context to be initialized
 * \param key      encryption key
 * \param keybits  must be 128, 192 or 256
 *
 * \return         0 if successful, or MBEDTLS_ERR_AES_INVALID_KEY_LENGTH
 */
int mbedtls_aes_setkey_enc( mbedtls_aes_context *ctx, const unsigned char *key,
                    unsigned int keybits ){
  CryptoAes &enc = *(CryptoAes *)ctx->pEnc;
  const bool ok = enc.SetKey(key, keybits);
  return (ok) ? 0 : -1;
}

/**
 * \brief          AES key schedule (decryption)
 *
 * \param ctx      AES context to be initialized
 * \param key      decryption key
 * \param keybits  must be 128, 192 or 256
 *
 * \return         0 if successful, or MBEDTLS_ERR_AES_INVALID_KEY_LENGTH
 */
int mbedtls_aes_setkey_dec( mbedtls_aes_context *ctx, const unsigned char *key,
                    unsigned int keybits ){
  CryptoAes &dec = *(CryptoAes *)ctx->pDec;
  const bool ok = dec.SetKey(key, keybits);
  return (ok) ? 0 : -1;
}

/**
 * \brief          AES-ECB block encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     MBEDTLS_AES_ENCRYPT or MBEDTLS_AES_DECRYPT
 * \param input    16-byte input block
 * \param output   16-byte output block
 *
 * \return         0 if successful
 */
int mbedtls_aes_crypt_ecb( mbedtls_aes_context *ctx,
                    int mode,
                    const unsigned char input[16],
                    unsigned char output[16] ){
  if (mode == MBEDTLS_AES_ENCRYPT) {
    CryptoAes &enc = *(CryptoAes *)ctx->pEnc;
    enc.EcbEncrypt(input, output, 16);
  }
  else {
    CryptoAes &dec = *(CryptoAes *)ctx->pDec;
    dec.EcbDecrypt(input, output, 16);
  }
  return 0;
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/**
 * \brief          AES-CBC buffer encryption/decryption
 *                 Length should be a multiple of the block
 *                 size (16 bytes)
 *
 * \note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * \param ctx      AES context
 * \param mode     MBEDTLS_AES_ENCRYPT or MBEDTLS_AES_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 *
 * \return         0 if successful, or MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH
 */
int mbedtls_aes_crypt_cbc( mbedtls_aes_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output ){
  if (mode == MBEDTLS_AES_ENCRYPT) {
    CryptoAes &enc = *(CryptoAes *)ctx->pEnc;
    enc.CbcEncrypt(input, output, length, iv);
  }
  else {
    CryptoAes &dec = *(CryptoAes *)ctx->pDec;
    dec.CbcDecrypt(input, output, length, iv);
  }
  return 0;
}
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_CFB)
/**
 * \brief          AES-CFB128 buffer encryption/decryption.
 *
 * Note: Due to the nature of CFB you should use the same key schedule for
 * both encryption and decryption. So a context initialized with
 * mbedtls_aes_setkey_enc() for both MBEDTLS_AES_ENCRYPT and MBEDTLS_AES_DECRYPT.
 *
 * \note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * \param ctx      AES context
 * \param mode     MBEDTLS_AES_ENCRYPT or MBEDTLS_AES_DECRYPT
 * \param length   length of the input data
 * \param iv_off   offset in IV (updated after use)
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 *
 * \return         0 if successful
 */
int mbedtls_aes_crypt_cfb128( mbedtls_aes_context *ctx,
                       int mode,
                       size_t length,
                       size_t *iv_off,
                       unsigned char iv[16],
                       const unsigned char *input,
                       unsigned char *output ){
  int c;
  size_t n = *iv_off;

  if (mode == MBEDTLS_AES_DECRYPT)
  {
    while (length--)
    {
      if (n == 0)
        mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);

      c = *input++;
      *output++ = (unsigned char)(c ^ iv[n]);
      iv[n] = (unsigned char)c;

      n = (n + 1) & 0x0F;
    }
  }
  else
  {
    while (length--)
    {
      if (n == 0)
        mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);

      iv[n] = *output++ = (unsigned char)(iv[n] ^ *input++);

      n = (n + 1) & 0x0F;
    }
  }

  *iv_off = n;

  return(0);
}

/**
 * \brief          AES-CFB8 buffer encryption/decryption.
 *
 * Note: Due to the nature of CFB you should use the same key schedule for
 * both encryption and decryption. So a context initialized with
 * mbedtls_aes_setkey_enc() for both MBEDTLS_AES_ENCRYPT and MBEDTLS_AES_DECRYPT.
 *
 * \note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * \param ctx      AES context
 * \param mode     MBEDTLS_AES_ENCRYPT or MBEDTLS_AES_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 *
 * \return         0 if successful
 */
int mbedtls_aes_crypt_cfb8( mbedtls_aes_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output ){
  unsigned char c;
  unsigned char ov[17];

  while (length--)
  {
    memcpy(ov, iv, 16);
    mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);

    if (mode == MBEDTLS_AES_DECRYPT)
      ov[16] = *input;

    c = *output++ = (unsigned char)(iv[0] ^ *input++);

    if (mode == MBEDTLS_AES_ENCRYPT)
      ov[16] = c;

    memcpy(iv, ov + 1, 16);
  }

  return(0);
}
#endif /*MBEDTLS_CIPHER_MODE_CFB */

#if defined(MBEDTLS_CIPHER_MODE_CTR)
/**
 * \brief               AES-CTR buffer encryption/decryption
 *
 * Warning: You have to keep the maximum use of your counter in mind!
 *
 * Note: Due to the nature of CTR you should use the same key schedule for
 * both encryption and decryption. So a context initialized with
 * mbedtls_aes_setkey_enc() for both MBEDTLS_AES_ENCRYPT and MBEDTLS_AES_DECRYPT.
 *
 * \param ctx           AES context
 * \param length        The length of the data
 * \param nc_off        The offset in the current stream_block (for resuming
 *                      within current cipher stream). The offset pointer to
 *                      should be 0 at the start of a stream.
 * \param nonce_counter The 128-bit nonce and counter.
 * \param stream_block  The saved stream-block for resuming. Is overwritten
 *                      by the function.
 * \param input         The input data stream
 * \param output        The output data stream
 *
 * \return         0 if successful
 */
int mbedtls_aes_crypt_ctr(mbedtls_aes_context *ctx,
  size_t length,
  size_t *nc_off,
  unsigned char nonce_counter[16],
  unsigned char stream_block[16],
  const unsigned char *input,
  unsigned char *output)
{
  int c, i;
  size_t n = *nc_off;

  while (length--)
  {
    if (n == 0) {
      mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, nonce_counter, stream_block);

      for (i = 16; i > 0; i--)
        if (++nonce_counter[i - 1] != 0)
          break;
    }
    c = *input++;
    *output++ = (unsigned char)(c ^ stream_block[n]);

    n = (n + 1) & 0x0F;
  }

  *nc_off = n;

  return(0);
}
#endif /* MBEDTLS_CIPHER_MODE_CTR */

#endif // MBEDTLS_AES_ALT

#ifdef MBEDTLS_AES_ENCRYPT_ALT

/**
 * \brief           Internal AES block encryption function
 *                  (Only exposed to allow overriding it,
 *                  see MBEDTLS_AES_ENCRYPT_ALT)
 *
 * \param ctx       AES context
 * \param input     Plaintext block
 * \param output    Output (ciphertext) block
 *
 * \return          0 if successful
 */
int mbedtls_internal_aes_encrypt( mbedtls_aes_context *ctx,
                                  const unsigned char input[16],
                                  unsigned char output[16] ){
  CryptoAes &enc = *(CryptoAes *)ctx->pEnc;
  enc.EncryptBlock(input, output);
  return 0;
}

#endif

#ifdef MBEDTLS_AES_DECRYPT_ALT
/**
 * \brief           Internal AES block decryption function
 *                  (Only exposed to allow overriding it,
 *                  see MBEDTLS_AES_DECRYPT_ALT)
 *
 * \param ctx       AES context
 * \param input     Ciphertext block
 * \param output    Output (plaintext) block
 *
 * \return          0 if successful
 */
int mbedtls_internal_aes_decrypt( mbedtls_aes_context *ctx,
                                  const unsigned char input[16],
                                  unsigned char output[16] ){
  CryptoAes &dec = *(CryptoAes *)ctx->pDec;
  dec.DecryptBlock(input, output);
  return 0;
}
#endif

int mbedtls_aes_encrypt_ext(mbedtls_aes_context *ctx,
  const unsigned char input[16],
  unsigned char output[16]) {
  return mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, input, output);
}


int mbedtls_aes_decrypt_ext(mbedtls_aes_context *ctx,
  const unsigned char input[16],
  unsigned char output[16]) {
  return mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_DECRYPT, input, output);
}


#if !defined(MBEDTLS_DEPRECATED_REMOVED)
#if defined(MBEDTLS_DEPRECATED_WARNING)
#define MBEDTLS_DEPRECATED      __attribute__((deprecated))
#else
#define MBEDTLS_DEPRECATED
#endif
/**
 * \brief           Deprecated internal AES block encryption function
 *                  without return value.
 *
 * \deprecated      Superseded by mbedtls_aes_encrypt_ext() in 2.5.0
 *
 * \param ctx       AES context
 * \param input     Plaintext block
 * \param output    Output (ciphertext) block
 */
MBEDTLS_DEPRECATED void mbedtls_aes_encrypt( mbedtls_aes_context *ctx,
                                             const unsigned char input[16],
                                             unsigned char output[16] ){
  (void)mbedtls_aes_encrypt_ext(ctx, input, output);
}

/**
 * \brief           Deprecated internal AES block decryption function
 *                  without return value.
 *
 * \deprecated      Superseded by mbedtls_aes_decrypt_ext() in 2.5.0
 *
 * \param ctx       AES context
 * \param input     Ciphertext block
 * \param output    Output (plaintext) block
 */
MBEDTLS_DEPRECATED void mbedtls_aes_decrypt( mbedtls_aes_context *ctx,
                                             const unsigned char input[16],
                                             unsigned char output[16] ){
  (void)mbedtls_aes_decrypt_ext(ctx, input, output);

}

#endif

}
#endif
