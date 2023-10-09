/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: Flash encryption and decryption feature head file.
 * Author: wangjian
 * Create: 2019-05-12
 */
#ifndef __CRYPTO_H__
#define __CRYPTO_H__
#ifdef CONFIG_FLASH_ENCRYPT_SUPPORT
#include <hi_flashboot.h>
#include <hi_types.h>
#include <hi_upg_file.h>

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

#define HI_NV_FTM_KERNELA_WORK_ID         0x4
#define HI_NV_FTM_BACKUP_KERNELA_WORK_ID  0x5
#define HI_NV_FTM_KERNELB_WORK_ID         0x6
#define HI_NV_FTM_BACKUP_KERNELB_WORK_ID  0x7

#define LZMA_HEAD_SIZE          13
#define DATA_MEDIUM_NOT_INIT    0
#define DATA_MEDIUM_RAM         1
#define DATA_MEDIUM_FLASH       2

#ifdef CONFIG_FLASH_ENCRYPT_NOT_USE_EFUSE
#define HI_NV_FLASH_CRYPT_CNT_ID      0x8
#endif

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

#ifdef CONFIG_FLASH_ENCRYPT_NOT_USE_EFUSE
typedef struct {
    hi_u32 flash_crypt_cnt;
} hi_flash_crypto_cnt;
#endif

typedef struct {
    uintptr_t kernel_addr;
    uintptr_t crypto_start_addr;
    uintptr_t crypto_end_addr;
    hi_u16 crypto_total_size;
    hi_u16 cryptoed_size;
    hi_u8 *buf;
    hi_u8 upg_iv[IV_BYTE_LENGTH];
    hi_u8 upg_salt[IV_BYTE_LENGTH];
    hi_bool is_verify_byte;
    hi_u8 data_medium;
    hi_u16 ram_offset;
    hi_bool is_crypto_section;
    hi_bool para_is_init;
} boot_crypto_ctx;

boot_crypto_ctx *boot_crypto_get_ctx(hi_void);
boot_crypto_ctx *boot_decrypt_get_ctx(hi_void);
hi_u32 crypto_decrypt(hi_u32 ram_addr, hi_u32 ram_size);
hi_u32 crypto_load_flash_raw(uintptr_t ram_addr, hi_u32 ram_size);
hi_void crypto_check_decrypt(hi_void);
hi_u32 crypto_kernel_write(hi_u32 start, hi_u32 offset, hi_u8 *buffer, hi_u32 size);
hi_u32 crypto_kernel_read(hi_u32 start, hi_u32 offset, hi_u8 *buf, hi_u32 buf_len);

hi_u32 boot_decrypt_upg_file(hi_u32 addr_write, const hi_upg_section_head *section_head);
hi_void boot_decrypt_free_memory(hi_void);

#endif
#endif
