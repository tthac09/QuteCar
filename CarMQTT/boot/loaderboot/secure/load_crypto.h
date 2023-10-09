/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Flash encryption and decryption feature src file.
 * Author: hisilicon
 * Create: 2020-03-16
 */

#ifndef __LOAD_CRYPTO_H__
#define __LOAD_CRYPTO_H__
#ifdef CONFIG_FLASH_ENCRYPT_SUPPORT
#include <hi_boot_rom.h>

#define crypto_mem_free(sz)               \
    do {                                  \
        if ((sz) != HI_NULL) {            \
            boot_free(sz);                \
        }                                 \
        (sz) = HI_NULL;                   \
    } while (0)

#define IV_BYTE_LENGTH          16
#define ROOTKEY_IV_BYTE_LENGTH  32

#define DIE_ID_BYTE_LENGTH      24

#define KEY_BYTE_LENGTH         32

#define SHA_256_LENGTH          32

#define ROOT_SALT_LENGTH        32

#define CRYPTO_CNT_NUM          6

#define CRYPTO_KERNEL_LENGTH  4096

#define KERNEL_RAM_ADDR       0xD8200

#define KDF_ITERATION_CNT       1024

#define MIN_CRYPTO_BLOCK_SIZE   16

#define HI_NV_FTM_STARTUP_CFG_ID          0x3
#define HI_NV_FTM_KERNELA_WORK_ID         0x4
#define HI_NV_FTM_BACKUP_KERNELA_WORK_ID  0x5

#ifdef CONFIG_FLASH_ENCRYPT_NOT_USE_EFUSE
#define HI_NV_FLASH_CRYPT_CNT_ID      0x8
#endif

typedef enum {
    CRYPTO_WORKKEY_KERNEL_A = 0x1,
    CRYPTO_WORKKEY_KERNEL_A_BACKUP = 0x2,
    CRYPTO_WORKKEY_KERNEL_A_BOTH = 0x3,
} crypto_workkey_partition;

typedef struct {
    uintptr_t addr_start; /* boot start address */
    hi_u16 mode;          /* upgrade mode */
    hi_u8 file_type;      /* file type:boot or code+nv */
    hi_u8 refresh_nv;     /* refresh nv when the flag bit 0x55 is read */
    hi_u8 reset_cnt;      /* number of restarts in upgrade mode */
    hi_u8 cnt_max;        /* the maximum number of restarts (default value : 3) */
    hi_u16 reserved1;
    uintptr_t addr_write; /* write kernel upgrade file address */
    hi_u32 reserved2; /* 2: reserved bytes */
} hi_nv_ftm_startup_cfg;

typedef struct {
    hi_u8 root_salt[ROOT_SALT_LENGTH];
    hi_u8 iv_nv[IV_BYTE_LENGTH];         /* 根密钥加密工作密钥的初始向量值，在NV中以明文存储 */
    hi_u8 iv_content[IV_BYTE_LENGTH];    /* 工作密钥加密加Flash的初始向量值密文 */
    hi_u8 work_text[KEY_BYTE_LENGTH];    /* 工作密钥密文 */
    hi_u8 content_sh256[SHA_256_LENGTH]; /* 上述3个数据哈希计算结果的密文 */
} hi_flash_crypto_content;


#ifdef CONFIG_FLASH_ENCRYPT_NOT_USE_EFUSE
typedef struct {
    hi_u32 flash_crypt_cnt;
} hi_flash_crypto_cnt;
#endif

hi_u32 crypto_check_encrypt(hi_void);
hi_u32 crypto_encrypt_factory_image(uintptr_t file_addr);

#endif
#endif
