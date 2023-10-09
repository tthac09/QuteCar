/*
 *  FIPS-180-2 compliant SHA-256 implementation
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
 *  The SHA-256 Secure Hash Standard was published by NIST in 2002.
 *
 *  http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_SHA256_C)

#include "mbedtls/sha256.h"
#include "mbedtls/platform_util.h"

#include <string.h>

#if defined(MBEDTLS_SHA256_ALT)

#include <stdlib.h>

#define SHA256_VALIDATE_RET(cond)                           \
    MBEDTLS_INTERNAL_VALIDATE_RET( cond, MBEDTLS_ERR_SHA256_BAD_INPUT_DATA )
#define SHA256_VALIDATE(cond)  MBEDTLS_INTERNAL_VALIDATE( cond )

#define SHA256_ALT_ALIGNSIZE   4

void mbedtls_sha256_init(mbedtls_sha256_context *ctx)
{
    SHA256_VALIDATE( ctx != NULL );
    ctx->sha_type = HI_CIPHER_HASH_TYPE_SHA256;
}

void mbedtls_sha256_free(mbedtls_sha256_context *ctx)
{
    if( ctx == NULL )
            return;

    mbedtls_platform_zeroize( ctx, sizeof( mbedtls_sha256_context ) );
}

void mbedtls_sha256_clone(mbedtls_sha256_context *dst, const mbedtls_sha256_context *src)
{
    SHA256_VALIDATE( dst != NULL );
    SHA256_VALIDATE( src != NULL );

    *dst = *src;
}

int mbedtls_sha256_starts_ret(mbedtls_sha256_context *ctx, int is224)
{
    if (is224) {
        return MBEDTLS_ERR_SHA256_BAD_INPUT_DATA;
    }
    return hi_cipher_hash_start(ctx);
}

int mbedtls_sha256_update_ret(mbedtls_sha256_context *ctx,
                               const unsigned char *input, size_t ilen)
{
    (void)ctx;
    unsigned int ret;
    if (ilen == 0) {
        return 0;
    }
    uintptr_t alignedBuf = (uintptr_t)input;
    if ((uintptr_t)input % SHA256_ALT_ALIGNSIZE) {
        alignedBuf = (uintptr_t)malloc(ilen);
        if (alignedBuf == NULL) {
            return -1;
        }
        (void)memcpy((void *)alignedBuf, input, ilen);
    }
    ret = hi_cipher_hash_update(alignedBuf, ilen);
    if (alignedBuf != (uintptr_t)input) {
        free((void *)alignedBuf);
    }
    return ret;
}

int mbedtls_sha256_finish_ret(mbedtls_sha256_context *ctx, unsigned char output[32])
{
    (void)ctx;
    unsigned int ret;
    uintptr_t alignedBuf = (uintptr_t)output;
    if ((uintptr_t)output % SHA256_ALT_ALIGNSIZE) {
        alignedBuf = (uintptr_t)malloc(32);
        if (alignedBuf == NULL) {
            return -1;
        }
    }
    ret = hi_cipher_hash_final((unsigned char *)alignedBuf, 32);
    if (alignedBuf != (uintptr_t)output) {
        (void)memcpy(output, (void *)alignedBuf, 32);
        free((void *)alignedBuf);
    }
    return ret;
}

int mbedtls_internal_sha256_process(mbedtls_sha256_context *ctx, const unsigned char data[64])
{
    (void)ctx;
    unsigned int ret;
    uintptr_t alignedBuf = (uintptr_t)data;
    if ((uintptr_t)data % SHA256_ALT_ALIGNSIZE) {
        alignedBuf = (uintptr_t)malloc(64);
        if (alignedBuf == NULL) {
            return -1;
        }
        (void)memcpy((void *)alignedBuf, data, 64);
    }
    ret = hi_cipher_hash_update(alignedBuf, 64);
    if (alignedBuf != (uintptr_t)data) {
        free((void *)alignedBuf);
    }
    return ret;
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha256_update( mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen )
{
    mbedtls_sha256_update_ret( ctx, input, ilen );
}

void mbedtls_sha256_finish( mbedtls_sha256_context *ctx, unsigned char output[32] )
{
    mbedtls_sha256_finish_ret( ctx, output );
}
#endif

#endif /* MBEDTLS_SHA256_ALT */

#endif /* MBEDTLS_SHA256_C */
