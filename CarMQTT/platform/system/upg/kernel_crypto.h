/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: Flash encryption and decryption feature header file.
 * Author: hisilicon
 * Create: 2020-03-16
 */

#ifndef __ENCRYPT_UPG_DUAL_PARTITION_H__
#define __ENCRYPT_UPG_DUAL_PARTITION_H__

#if defined(CONFIG_FLASH_ENCRYPT_SUPPORT)
#include <hi_config.h>
#include <hi_mem.h>

#ifdef CRYPTO_DEBUG
#include <hi_early_debug.h>
#define crypto_print(ftm...) \
    do {                  \
        printf(ftm);      \
    } while (0);
#else
#define crypto_print(ftm...)
#endif

#define crypto_mem_free(sz)                  \
    do {                                     \
        if ((sz) != HI_NULL) {               \
            hi_free(HI_MOD_ID_CRYPTO, (sz)); \
        }                                    \
        (sz) = HI_NULL;                      \
    } while (0)

#define IV_BYTE_LENGTH          16
#define ROOTKEY_IV_BYTE_LENGTH  32

#define DIE_ID_BYTE_LENGTH      24

#define KEY_BYTE_LENGTH         32

#define SHA_256_LENGTH          32

#define ROOT_SALT_LENGTH        32

#define CRYPTO_CNT_NUM          6

#define CRYPTO_KERNEL_LENGTH  4096

#define ENCRYPT_KDF_ITERATION_CNT       1024

#define MIN_CRYPTO_BLOCK_SIZE   16

#define HI_NV_FTM_KERNELA_WORK_ID         0x4
#define HI_NV_FTM_BACKUP_KERNELA_WORK_ID  0x5
#define HI_NV_FTM_KERNELB_WORK_ID         0x6
#define HI_NV_FTM_BACKUP_KERNELB_WORK_ID  0x7

typedef enum {
    CRYPTO_WORKKEY_KERNEL_A = 0x1,
    CRYPTO_WORKKEY_KERNEL_A_BACKUP = 0x2,
    CRYPTO_WORKKEY_KERNEL_A_BOTH = 0x3,
    CRYPTO_WORKKEY_KERNEL_B = 0x4,
    CRYPTO_WORKKEY_KERNEL_B_BACKUP = 0x8,
    CRYPTO_WORKKEY_KERNEL_B_BOTH = 0xC,
} crypto_workkey_partition;

typedef struct {
    hi_u8 root_salt[ROOT_SALT_LENGTH];
    hi_u8 iv_nv[IV_BYTE_LENGTH];         /* 根密钥加密工作密钥的初始向量值，在NV中以明文存储 */
    hi_u8 iv_content[IV_BYTE_LENGTH];    /* 工作密钥加密加Flash的初始向量值密文 */
    hi_u8 work_text[KEY_BYTE_LENGTH];    /* 工作密钥密文 */
    hi_u8 content_sh256[SHA_256_LENGTH]; /* 上述3个数据哈希计算结果的密文 */
} hi_flash_crypto_content;

typedef struct {
    uintptr_t kernel_start;
    uintptr_t encrypt_offset;
    hi_u32 encrypt_size;
    hi_u8 upg_iv[IV_BYTE_LENGTH];
    hi_u8 upg_salt[IV_BYTE_LENGTH];
    hi_u8 *raw_buf;
} encrypt_ctx;

encrypt_ctx *encrypt_get_ctx(hi_void);

hi_u32 crypto_decrypt(encrypt_ctx *para);

hi_u32 encrypt_upg_data(encrypt_ctx *para);

hi_u32 crypto_encrypt_data_to_flash(uintptr_t kernel_offset);

#endif

#endif
