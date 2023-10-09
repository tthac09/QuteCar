/*
 * Wrapper functions for crypto libraries
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 */
/****************************************************************************
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations,
 * which might include those applicable to Huawei LiteOS of U.S. and the country in
 * which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in
 * compliance with such applicable export control laws and regulations.
 ****************************************************************************/

#ifndef CRYPTO_MBEDTLS_H
#define CRYPTO_MBEDTLS_H

#include "mbedtls/ecp.h"
#include "mbedtls/bignum.h"

#define crypto_ec mbedtls_ecp_group
#define crypto_bignum mbedtls_mpi
#define crypto_ec_point mbedtls_ecp_point
#define AES_128_ALT_BLOCK_SIZE 16
#define AES_256_ALT_BLOCK_SIZE 32
#define AES_128_CRYPTO_LEN 128
#define DHM_PARM_MEM 2048
#define DHM_PARM_G_LEN 2

#endif
