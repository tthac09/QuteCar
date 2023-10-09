/*
 *  FIPS-197 compliant AES implementation
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
/*
 *  The AES block cipher was designed by Vincent Rijmen and Joan Daemen.
 *
 *  http://csrc.nist.gov/encryption/aes/rijndael/Rijndael.pdf
 *  http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_AES_C)

#include <string.h>
#include "los_base.h"
#include "mbedtls/aes.h"
#include "mbedtls/platform.h"
#include "mbedtls/platform_util.h"
#if defined(MBEDTLS_PADLOCK_C)
#include "mbedtls/padlock.h"
#endif
#if defined(MBEDTLS_AESNI_C)
#include "mbedtls/aesni.h"
#endif

#if defined(MBEDTLS_SELF_TEST)
#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#define mbedtls_printf printf
#endif /* MBEDTLS_PLATFORM_C */
#endif /* MBEDTLS_SELF_TEST */

#if defined(MBEDTLS_AES_ALT)

#define AES_ALT_BLOCK_SIZE  16
#define AES_ALT_ALIGNSIZE   4
/* Parameter validation macros based on platform_util.h */
#define AES_VALIDATE_RET( cond )    \
    MBEDTLS_INTERNAL_VALIDATE_RET( cond, MBEDTLS_ERR_AES_BAD_INPUT_DATA )
#define AES_VALIDATE( cond )        \
    MBEDTLS_INTERNAL_VALIDATE( cond )

void mbedtls_aes_init(mbedtls_aes_context *ctx)
{
    AES_VALIDATE(ctx != NULL);

    memset(ctx, 0, sizeof(mbedtls_aes_context));
}

void mbedtls_aes_free(mbedtls_aes_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_aes_context));
}

#if defined(MBEDTLS_CIPHER_MODE_XTS)
void mbedtls_aes_xts_init(mbedtls_aes_xts_context *ctx)
{
    AES_VALIDATE(ctx != NULL);

    memset(ctx, 0, sizeof(mbedtls_aes_xts_context));
}

void mbedtls_aes_xts_free(mbedtls_aes_xts_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_aes_xts_context));
}
#endif /* MBEDTLS_CIPHER_MODE_XTS */

/*
 * AES key schedule (encryption)
 */
int mbedtls_aes_setkey_enc(mbedtls_aes_context *ctx, const unsigned char *key,
                    unsigned int keybits)
{
    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(key != NULL);

    switch(keybits) {
        case 128: ctx->key_len = HI_CIPHER_AES_KEY_LENGTH_128BIT; break;
        case 192: ctx->key_len = HI_CIPHER_AES_KEY_LENGTH_192BIT; break;
        case 256: ctx->key_len = HI_CIPHER_AES_KEY_LENGTH_256BIT; break;
        default : return( MBEDTLS_ERR_AES_INVALID_KEY_LENGTH );
    }
    ctx->random_en = HI_FALSE;
    ctx->key_from  = HI_CIPHER_AES_KEY_FROM_CPU;
    ctx->work_mode = HI_CIPHER_AES_WORK_MODE_ECB;
    ctx->ccm = HI_NULL;
    (void)memcpy_s(ctx->key, sizeof(ctx->key), key, (keybits/8));
    return 0;
}

/*
 * AES key schedule (decryption)
 */
int mbedtls_aes_setkey_dec(mbedtls_aes_context *ctx, const unsigned char *key,
                    unsigned int keybits)
{
    return mbedtls_aes_setkey_enc(ctx, key, keybits);
}

#if defined(MBEDTLS_CIPHER_MODE_XTS)
static int mbedtls_aes_xts_decode_keys(const unsigned char *key,
                                        unsigned int keybits,
                                        const unsigned char **key1,
                                        unsigned int *key1bits,
                                        const unsigned char **key2,
                                        unsigned int *key2bits)
{
    const unsigned int half_keybits = keybits / 2;
    const unsigned int half_keybytes = half_keybits / 8;

    switch(keybits) {
        case 256: break;
        case 512: break;
        default : return( MBEDTLS_ERR_AES_INVALID_KEY_LENGTH );
    }

    *key1bits = half_keybits;
    *key2bits = half_keybits;
    *key1 = &key[0];
    *key2 = &key[half_keybytes];

    return 0;
}

int mbedtls_aes_xts_setkey_enc(mbedtls_aes_xts_context *ctx,
                                const unsigned char *key,
                                unsigned int keybits)
{
    int ret;
    const unsigned char *key1, *key2;
    unsigned int key1bits, key2bits;

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(key != NULL);

    ret = mbedtls_aes_xts_decode_keys(key, keybits, &key1, &key1bits,
                                       &key2, &key2bits);
    if (ret != 0) {
        return(ret);
    }

    /* Set the tweak key. Always set tweak key for the encryption mode. */
    ret = mbedtls_aes_setkey_enc(&ctx->tweak, key2, key2bits);
    if (ret != 0) {
        return(ret);
    }

    /* Set crypt key for encryption. */
    return mbedtls_aes_setkey_enc(&ctx->crypt, key1, key1bits);
}

int mbedtls_aes_xts_setkey_dec(mbedtls_aes_xts_context *ctx,
                                const unsigned char *key,
                                unsigned int keybits)
{
    int ret;
    const unsigned char *key1, *key2;
    unsigned int key1bits, key2bits;

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(key != NULL);

    ret = mbedtls_aes_xts_decode_keys(key, keybits, &key1, &key1bits,
                                       &key2, &key2bits);
    if (ret != 0) {
        return(ret);
    }

    /* Set the tweak key. Always set tweak key for encryption. */
    ret = mbedtls_aes_setkey_enc(&ctx->tweak, key2, key2bits);
    if (ret != 0) {
        return(ret);
    }

    /* Set crypt key for decryption. */
    return mbedtls_aes_setkey_dec(&ctx->crypt, key1, key1bits);
}
#endif /* MBEDTLS_CIPHER_MODE_XTS */

int mbedtls_internal_aes_option(mbedtls_aes_context *ctx,
                                 const unsigned char input[16],
                                 unsigned char output[16],
                                 bool type)
{
    int ret;
    uintptr_t alignedInBuf = (uintptr_t)input;
    uintptr_t alignedOutBuf = (uintptr_t)output;
    unsigned char in[16];
    unsigned char out[16];
    if ((uintptr_t)input % AES_ALT_ALIGNSIZE) {
        (VOID)memcpy(in, input, 16);
        alignedInBuf = (uintptr_t)in;
    }
    if ((uintptr_t)output % AES_ALT_ALIGNSIZE) {
        alignedOutBuf = (uintptr_t)out;
    }
    ret = hi_cipher_aes_config(ctx);
    if (ret != 0) {
        return ret;
    }
    ret = hi_cipher_aes_crypto(alignedInBuf, alignedOutBuf, AES_ALT_BLOCK_SIZE, type);
    if (ret == 0) {
        if (alignedOutBuf != (uintptr_t)output) {
            (VOID)memcpy(output, out, 16);
        }
    }
    (hi_void)hi_cipher_aes_destroy_config();
    return ret;
}

/*
 * AES-ECB block encryption
 */
int mbedtls_internal_aes_encrypt(mbedtls_aes_context *ctx,
                                  const unsigned char input[16],
                                  unsigned char output[16])
{
    return mbedtls_internal_aes_option(ctx, input, output, HI_TRUE);
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_aes_encrypt(mbedtls_aes_context *ctx,
                          const unsigned char input[16],
                          unsigned char output[16])
{
    mbedtls_internal_aes_encrypt(ctx, input, output);
}
#endif /* !MBEDTLS_DEPRECATED_REMOVED */

/*
 * AES-ECB block decryption
 */
int mbedtls_internal_aes_decrypt(mbedtls_aes_context *ctx,
                                  const unsigned char input[16],
                                  unsigned char output[16])
{
    return mbedtls_internal_aes_option(ctx, input, output, HI_FALSE);
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_aes_decrypt(mbedtls_aes_context *ctx,
                          const unsigned char input[16],
                          unsigned char output[16])
{
    mbedtls_internal_aes_decrypt(ctx, input, output);
}
#endif /* !MBEDTLS_DEPRECATED_REMOVED */

/*
 * AES-ECB block encryption/decryption
 */
int mbedtls_aes_crypt_ecb(mbedtls_aes_context *ctx,
                           int mode,
                           const unsigned char input[16],
                           unsigned char output[16])
{
    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);
    AES_VALIDATE_RET(mode == MBEDTLS_AES_ENCRYPT ||
                     mode == MBEDTLS_AES_DECRYPT);

    if (mode == MBEDTLS_AES_ENCRYPT) {
        return(mbedtls_internal_aes_encrypt(ctx, input, output));
    } else {
        return(mbedtls_internal_aes_decrypt(ctx, input, output));
    }
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/*
 * AES-CBC buffer encryption/decryption
 */
static int mbedtls_aesalt_buf_align(const unsigned char *input,
                                    const unsigned char *output,
                                    unsigned char **p_temp_input,
                                    unsigned char **p_temp_output,
                                    size_t length,
                                    int temp_length)
{
    if ((uintptr_t)input % AES_ALT_ALIGNSIZE) {
        *p_temp_input = (unsigned char *)malloc(temp_length);
        if (*p_temp_input == NULL) {
            return -1;
        }
        (VOID)memset(*p_temp_input, 0, temp_length);
        (VOID)memcpy(*p_temp_input, input, length);
    } else {
        *p_temp_input = (unsigned char *)input;
    }
    if ((uintptr_t)output % AES_ALT_ALIGNSIZE) {
        *p_temp_output = (unsigned char *)malloc(temp_length);
        if (*p_temp_output == NULL) {
            return -1;
        }
    } else {
        *p_temp_output = (unsigned char *)output;
    }
    return 0;
}

int mbedtls_aes_crypt_cbc(mbedtls_aes_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output)
{
    int ret, temp_length;
    unsigned char *temp_input = NULL;
    unsigned char *temp_output = NULL;
    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET((mode == MBEDTLS_AES_ENCRYPT) || (mode == MBEDTLS_AES_DECRYPT));
    AES_VALIDATE_RET(!(length % AES_ALT_BLOCK_SIZE));
    AES_VALIDATE_RET(iv != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);
    (VOID)memcpy((unsigned char *)ctx->iv, iv, AES_ALT_BLOCK_SIZE);
    ctx->work_mode = HI_CIPHER_AES_WORK_MODE_CBC;
    ret = hi_cipher_aes_config(ctx);
    if (ret != 0) {
        return ret;
    }
    temp_length = LOS_Align(length, AES_ALT_ALIGNSIZE);
    ret = mbedtls_aesalt_buf_align(input, output, &temp_input, &temp_output, length, temp_length);
    if (ret != 0) {
        goto exit;
    }
    if (mode == MBEDTLS_AES_DECRYPT) {
        (VOID)memcpy(iv, temp_input + temp_length - AES_ALT_BLOCK_SIZE, AES_ALT_BLOCK_SIZE);
    }

    ret = hi_cipher_aes_crypto((uintptr_t)temp_input, (uintptr_t)temp_output, length, (mode == MBEDTLS_AES_ENCRYPT));
    if (ret != 0) {
        goto exit;
    }

    if (mode == MBEDTLS_AES_ENCRYPT) {
        (VOID)memcpy(iv, temp_output + temp_length - AES_ALT_BLOCK_SIZE, AES_ALT_BLOCK_SIZE);
    }

    if (temp_output != output) {
        (VOID)memcpy(output, temp_output, temp_length);
    }

exit:
    (hi_void)hi_cipher_aes_destroy_config();
    if ((temp_input != NULL) && (temp_input != input)) {
        free(temp_input);
    }
    if ((temp_output != NULL) && (temp_output != output)) {
        free(temp_output);
    }
    return 0;
}

#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_XTS)
/*
 * AES-XTS buffer encryption/decryption
 */
/* Endianess with 64 bits values */
#ifndef GET_UINT64_LE
#define GET_UINT64_LE(n,b,i)                          \
{                                                     \
    (n) = ((uint64_t) (b)[(i) + 7] << 56)             \
        | ((uint64_t) (b)[(i) + 6] << 48)             \
        | ((uint64_t) (b)[(i) + 5] << 40)             \
        | ((uint64_t) (b)[(i) + 4] << 32)             \
        | ((uint64_t) (b)[(i) + 3] << 24)             \
        | ((uint64_t) (b)[(i) + 2] << 16)             \
        | ((uint64_t) (b)[(i) + 1] <<  8)             \
        | ((uint64_t) (b)[(i)    ]      );            \
}
#endif

#ifndef PUT_UINT64_LE
#define PUT_UINT64_LE(n,b,i)                         \
{                                                    \
    (b)[(i) + 7] = (unsigned char)((n) >> 56);       \
    (b)[(i) + 6] = (unsigned char)((n) >> 48);       \
    (b)[(i) + 5] = (unsigned char)((n) >> 40);       \
    (b)[(i) + 4] = (unsigned char)((n) >> 32);       \
    (b)[(i) + 3] = (unsigned char)((n) >> 24);       \
    (b)[(i) + 2] = (unsigned char)((n) >> 16);       \
    (b)[(i) + 1] = (unsigned char)((n) >>  8);       \
    (b)[(i)    ] = (unsigned char)((n)      );       \
}
#endif

typedef unsigned char mbedtls_be128[AES_ALT_BLOCK_SIZE];

/*
 * GF(2^128) multiplication function
 *
 * This function multiplies a field element by x in the polynomial field
 * representation. It uses 64-bit word operations to gain speed but compensates
 * for machine endianess and hence works correctly on both big and little
 * endian machines.
 */
static void mbedtls_gf128mul_x_ble(unsigned char r[16],
                                    const unsigned char x[16])
{
    uint64_t a, b, ra, rb;

    GET_UINT64_LE(a, x, 0);
    GET_UINT64_LE(b, x, 8);

    ra = (a << 1)  ^ 0x0087 >> (8 - ((b >> 63) << 3));
    rb = (a >> 63) | (b << 1);

    PUT_UINT64_LE(ra, r, 0);
    PUT_UINT64_LE(rb, r, 8);
}

/*
 * AES-XTS buffer encryption/decryption
 */
int mbedtls_aes_crypt_xts(mbedtls_aes_xts_context *ctx,
                          int mode,
                          size_t length,
                          const unsigned char data_unit[16],
                          const unsigned char *input,
                          unsigned char *output)
{
    int ret;
    size_t blocks = length / AES_ALT_BLOCK_SIZE;
    size_t leftover = length % AES_ALT_BLOCK_SIZE;
    unsigned char tweak[AES_ALT_BLOCK_SIZE];
    unsigned char prev_tweak[AES_ALT_BLOCK_SIZE];
    unsigned char tmp[AES_ALT_BLOCK_SIZE];

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(mode == MBEDTLS_AES_ENCRYPT ||
                     mode == MBEDTLS_AES_DECRYPT);
    AES_VALIDATE_RET(data_unit != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);

    /* Data units must be at least 16 bytes long. */
    if (length < AES_ALT_BLOCK_SIZE) {
        return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }

    /* NIST SP 800-38E disallows data units larger than 2**20 blocks. */
    if (length > ( 1 << 20 ) * AES_ALT_BLOCK_SIZE) {
        return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }

    /* Compute the tweak. */
    ret = mbedtls_aes_crypt_ecb(&ctx->tweak, MBEDTLS_AES_ENCRYPT,
                                 data_unit, tweak);
    if (ret != 0) {
        return(ret);
    }

    while (blocks--) {
        size_t i;

        if (leftover && (mode == MBEDTLS_AES_DECRYPT) && blocks == 0) {
            /* We are on the last block in a decrypt operation that has
             * leftover bytes, so we need to use the next tweak for this block,
             * and this tweak for the lefover bytes. Save the current tweak for
             * the leftovers and then update the current tweak for use on this,
             * the last full block. */
            memcpy(prev_tweak, tweak, sizeof(tweak));
            mbedtls_gf128mul_x_ble(tweak, tweak);
        }

        for (i = 0; i < AES_ALT_BLOCK_SIZE; i++) {
            tmp[i] = input[i] ^ tweak[i];
        }

        ret = mbedtls_aes_crypt_ecb(&ctx->crypt, mode, tmp, tmp);
        if (ret != 0) {
            return(ret);
        }

        for (i = 0; i < AES_ALT_BLOCK_SIZE; i++) {
            output[i] = tmp[i] ^ tweak[i];
        }

        /* Update the tweak for the next block. */
        mbedtls_gf128mul_x_ble(tweak, tweak);

        output += AES_ALT_BLOCK_SIZE;
        input += AES_ALT_BLOCK_SIZE;
    }

    if (leftover) {
        /* If we are on the leftover bytes in a decrypt operation, we need to
         * use the previous tweak for these bytes (as saved in prev_tweak). */
        unsigned char *t = mode == MBEDTLS_AES_DECRYPT ? prev_tweak : tweak;

        /* We are now on the final part of the data unit, which doesn't divide
         * evenly by 16. It's time for ciphertext stealing. */
        size_t i;
        unsigned char *prev_output = output - AES_ALT_BLOCK_SIZE;

        /* Copy ciphertext bytes from the previous block to our output for each
         * byte of cyphertext we won't steal. At the same time, copy the
         * remainder of the input for this final round (since the loop bounds
         * are the same). */
        for (i = 0; i < leftover; i++) {
            output[i] = prev_output[i];
            tmp[i] = input[i] ^ t[i];
        }

        /* Copy ciphertext bytes from the previous block for input in this
         * round. */
        for (; i < AES_ALT_BLOCK_SIZE; i++) {
            tmp[i] = prev_output[i] ^ t[i];
        }

        ret = mbedtls_aes_crypt_ecb(&ctx->crypt, mode, tmp, tmp);
        if (ret != 0) {
            return ret;
        }

        /* Write the result back to the previous block, overriding the previous
         * output we copied. */
        for (i = 0; i < AES_ALT_BLOCK_SIZE; i++) {
            prev_output[i] = tmp[i] ^ t[i];
        }
    }

    return( 0 );
}

#endif /* MBEDTLS_CIPHER_MODE_XTS */

#if defined(MBEDTLS_CIPHER_MODE_CFB)
/*
 * AES-CFB128 buffer encryption/decryption
 */
int mbedtls_aes_crypt_cfb128(mbedtls_aes_context *ctx,
                       int mode,
                       size_t length,
                       size_t *iv_off,
                       unsigned char iv[16],
                       const unsigned char *input,
                       unsigned char *output)
{
    int c;
    size_t n;

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(mode == MBEDTLS_AES_ENCRYPT ||
                     mode == MBEDTLS_AES_DECRYPT);
    AES_VALIDATE_RET(iv_off != NULL);
    AES_VALIDATE_RET(iv != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);

    n = *iv_off;

    if (n > 15) {
        return(MBEDTLS_ERR_AES_BAD_INPUT_DATA);
    }

    if (mode == MBEDTLS_AES_DECRYPT) {
        while (length--) {
            if (n == 0) {
                mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);
            }

            c = *input++;
            *output++ = (unsigned char)(c ^ iv[n]);
            iv[n] = (unsigned char)c;

            n = (n + 1) & 0x0F;
        }
    } else {
        while (length--) {
            if (n == 0) {
                mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);
            }
            iv[n] = *output++ = (unsigned char)(iv[n] ^ *input++);
            n = (n + 1) & 0x0F;
        }
    }

    *iv_off = n;

    return(0);

}

/*
 * AES-CFB8 buffer encryption/decryption
 */
int mbedtls_aes_crypt_cfb8(mbedtls_aes_context *ctx,
                            int mode,
                            size_t length,
                            unsigned char iv[16],
                            const unsigned char *input,
                            unsigned char *output)
{
    unsigned char c;
    unsigned char ov[17];

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(mode == MBEDTLS_AES_ENCRYPT ||
                     mode == MBEDTLS_AES_DECRYPT);
    AES_VALIDATE_RET(iv != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);
    while (length--) {
        memcpy(ov, iv, AES_ALT_BLOCK_SIZE);
        mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);

        if (mode == MBEDTLS_AES_DECRYPT) {
            ov[AES_ALT_BLOCK_SIZE] = *input;
        }

        c = *output++ = (unsigned char)(iv[0] ^ *input++);

        if (mode == MBEDTLS_AES_ENCRYPT) {
            ov[AES_ALT_BLOCK_SIZE] = c;
        }

        memcpy(iv, ov + 1, AES_ALT_BLOCK_SIZE);
    }
    return(0);
}
#endif /* MBEDTLS_CIPHER_MODE_CFB */

#if defined(MBEDTLS_CIPHER_MODE_OFB)
/*
 * AES-OFB (Output Feedback Mode) buffer encryption/decryption
 */
int mbedtls_aes_crypt_ofb(mbedtls_aes_context *ctx,
                           size_t length,
                           size_t *iv_off,
                           unsigned char iv[16],
                           const unsigned char *input,
                           unsigned char *output)
{
    int ret = 0;
    size_t n;

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(iv_off != NULL);
    AES_VALIDATE_RET(iv != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);

    n = *iv_off;

    if (n > 15) {
        return(MBEDTLS_ERR_AES_BAD_INPUT_DATA);
    }

    while (length--) {
        if (n == 0) {
            ret = mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);
            if (ret != 0) {
                goto exit;
            }
        }
        *output++ =  *input++ ^ iv[n];
        n = (n + 1) & 0x0F;
    }
    *iv_off = n;

exit:
    return(ret);
}
#endif /* MBEDTLS_CIPHER_MODE_OFB */

#if defined(MBEDTLS_CIPHER_MODE_CTR)
/*
 * AES-CTR buffer encryption/decryption
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
    size_t n;

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(nc_off != NULL);
    AES_VALIDATE_RET(nonce_counter != NULL);
    AES_VALIDATE_RET(stream_block != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);

    n = *nc_off;

    if (n > 0x0F) {
        return(MBEDTLS_ERR_AES_BAD_INPUT_DATA);
    }

    while (length--) {
        if (n == 0) {
            mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, nonce_counter, stream_block);

            for (i = AES_ALT_BLOCK_SIZE; i > 0; i--) {
                if(++nonce_counter[i - 1] != 0) {
                    break;
                }
            }
        }
        c = *input++;
        *output++ = (unsigned char)(c ^ stream_block[n]);

        n = (n + 1) & 0x0F;
    }

    *nc_off = n;

    return(0);
}
#endif /* MBEDTLS_CIPHER_MODE_CTR */

#endif /* !MBEDTLS_AES_ALT */

#endif /* MBEDTLS_AES_C */
