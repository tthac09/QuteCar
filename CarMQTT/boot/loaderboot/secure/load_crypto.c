/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Flash encryption and decryption feature src file.
 * Author: hisilicon
 * Create: 2020-03-16
 */
#ifdef CONFIG_FLASH_ENCRYPT_SUPPORT
#include <hi_nvm.h>
#include <load_partition_table.h>

#include "load_crypto.h"
#include "burn_file.h"

#define ENCPT_CFG_FLAG 1

static hi_efuse_idx g_efuse_id[CRYPTO_CNT_NUM] = {
    HI_EFUSE_FLASH_ENCPY_CNT0_RW_ID,
    HI_EFUSE_FLASH_ENCPY_CNT1_RW_ID,
    HI_EFUSE_FLASH_ENCPY_CNT2_RW_ID,
    HI_EFUSE_FLASH_ENCPY_CNT3_RW_ID,
    HI_EFUSE_FLASH_ENCPY_CNT4_RW_ID,
    HI_EFUSE_FLASH_ENCPY_CNT5_RW_ID
};

static hi_efuse_idx g_writeable_efuse = HI_EFUSE_IDX_MAX;

#ifdef CONFIG_FLASH_ENCRYPT_NOT_USE_EFUSE
hi_flash_crypto_cnt g_crypto_nv_cfg;

hi_flash_crypto_cnt *boot_crypto_get_cfg(hi_void)
{
    return &g_crypto_nv_cfg;
}

hi_u32 boot_crypto_save_cfg_to_nv(hi_void)
{
    hi_flash_crypto_cnt *cfg = boot_crypto_get_cfg();
    hi_u32 ret = hi_factory_nv_write(HI_NV_FLASH_CRYPT_CNT_ID, cfg, sizeof(hi_flash_crypto_cnt), 0);
    if (ret != HI_ERR_SUCCESS) {
        boot_msg1("[boot crypto save nv]nv write fail,ret ", ret);
        return ret;
    }
    return ret;
}

hi_void boot_crypto_load_cfg_from_nv(hi_void)
{
    hi_u32 cs;
    hi_flash_crypto_cnt nv_cfg = { 0 };
    hi_flash_crypto_cnt *cfg = boot_crypto_get_cfg();
    hi_u32 ret = hi_factory_nv_read(HI_NV_FLASH_CRYPT_CNT_ID, &nv_cfg, sizeof(hi_flash_crypto_cnt), 0);
    if ((ret != HI_ERR_SUCCESS) ||
        ((nv_cfg.flash_crypt_cnt != 0) && (nv_cfg.flash_crypt_cnt != 1))) {
        cfg->flash_crypt_cnt = 0;
        ret = boot_crypto_save_cfg_to_nv();
        if (ret != HI_ERR_SUCCESS) {
            return;
        }
    } else {
        cs = (uintptr_t)cfg ^ sizeof(hi_flash_crypto_cnt) ^ ((uintptr_t)&nv_cfg) ^ sizeof(hi_flash_crypto_cnt);
        memcpy_s(cfg, sizeof(hi_flash_crypto_cnt), &nv_cfg, sizeof(hi_flash_crypto_cnt), cs);
    }
}

hi_u32 boot_set_crypto_finish_flag(hi_void)
{
    hi_u32 ret;
    hi_flash_crypto_cnt *cfg = boot_crypto_get_cfg();
    cfg->flash_crypt_cnt = 0x1;
    ret = boot_crypto_save_cfg_to_nv();
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    return HI_ERR_SUCCESS;
}
#endif

hi_void loader_clear_content(hi_u8 *content, hi_u32 content_len)
{
    if ((content == HI_NULL) || (content_len == 0)) {
        return;
    }

    hi_u32 cs = (uintptr_t)content ^ content_len ^ 0x0 ^ content_len;
    (hi_void)memset_s(content, content_len, 0x0, content_len, cs);
}

static hi_void crpto_set_aes_ctrl_default_value(hi_cipher_aes_ctrl *aes_ctrl)
{
    if (aes_ctrl == HI_NULL) {
        return;
    }
    aes_ctrl->random_en = HI_FALSE;
    aes_ctrl->key_from = HI_CIPHER_AES_KEY_FROM_CPU;
    aes_ctrl->work_mode = HI_CIPHER_AES_WORK_MODE_CBC;
    aes_ctrl->key_len = HI_CIPHER_AES_KEY_LENGTH_256BIT;
}

static hi_u32 crypto_destory(hi_void)
{
    return hi_cipher_deinit();
}

static hi_u32 crypto_load_salt(crypto_workkey_partition part, hi_flash_crypto_content *key_content)
{
    hi_u32 ret = HI_ERR_SUCCESS;
    hi_u8 salt_e[ROOT_SALT_LENGTH] = { 0 };

    hi_u32 cs = (uintptr_t)salt_e ^ (hi_u32)sizeof(salt_e) ^ 0x0 ^ ROOT_SALT_LENGTH;
    (hi_void) memset_s(salt_e, sizeof(salt_e), 0x0, ROOT_SALT_LENGTH, cs);

    if (part == CRYPTO_WORKKEY_KERNEL_A) {
        ret = hi_factory_nv_read(HI_NV_FTM_KERNELA_WORK_ID, key_content, sizeof(hi_flash_crypto_content), 0);
        if (ret != HI_ERR_SUCCESS) {
            goto fail;
        }
    } else if (part == CRYPTO_WORKKEY_KERNEL_A_BACKUP) {
        ret = hi_factory_nv_read(HI_NV_FTM_BACKUP_KERNELA_WORK_ID, key_content, sizeof(hi_flash_crypto_content), 0);
        if (ret != HI_ERR_SUCCESS) {
            goto fail;
        }
    }

    if (memcmp(key_content->root_salt, salt_e, ROOT_SALT_LENGTH) == HI_ERR_SUCCESS) {
        ret = HI_PRINT_ERRNO_CRYPTO_SALT_EMPTY_ERR;
        goto fail;
    }
fail:
    return ret;
}

static hi_u32 crypto_get_root_salt(hi_flash_crypto_content *key_content)
{
    hi_u32 ret = crypto_load_salt(CRYPTO_WORKKEY_KERNEL_A, key_content);
    if (ret != HI_ERR_SUCCESS) {
        ret = crypto_load_salt(CRYPTO_WORKKEY_KERNEL_A_BACKUP, key_content);
        if (ret == HI_PRINT_ERRNO_CRYPTO_SALT_EMPTY_ERR) {
            ret = hi_cipher_trng_get_random_bytes(key_content->root_salt, ROOT_SALT_LENGTH);
            if (ret != HI_ERR_SUCCESS) {
                return ret;
            }
        }
    }

    return ret;
}

static hi_u32 crypto_prepare(hi_flash_crypto_content *save_content)
{
    hi_u32 ret;
    hi_u8 rootkey_iv[ROOTKEY_IV_BYTE_LENGTH];
    hi_cipher_kdf_ctrl ctrl;
    hi_u32 cs;

    ret = hi_cipher_init();
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    ret = crypto_get_root_salt(save_content);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    cs = (uintptr_t)rootkey_iv ^ sizeof(rootkey_iv) ^ ((uintptr_t)save_content->root_salt) ^ ROOT_SALT_LENGTH;
    ret = memcpy_s(rootkey_iv, sizeof(rootkey_iv), save_content->root_salt, ROOT_SALT_LENGTH, cs);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    ctrl.salt = rootkey_iv;
    ctrl.salt_len = sizeof(rootkey_iv);
    ctrl.kdf_cnt = KDF_ITERATION_CNT;
    ctrl.kdf_mode = HI_CIPHER_SSS_KDF_KEY_STORAGE; /* 自动生成HUK值的方式，硬件直接从EFUSE中获取HUK，生成根密钥固定 */
    return hi_cipher_kdf_key_derive(&ctrl);
}

hi_u8 get_flash_crypto_cfg(hi_void)
{
    hi_u8 flash_encpt_cfg;
    if (hi_efuse_read(HI_EFUSE_FLASH_ENCPY_CFG_RW_ID, &flash_encpt_cfg, (hi_u8)sizeof(hi_u8)) != HI_ERR_SUCCESS) {
        flash_encpt_cfg = ENCPT_CFG_FLAG;
    }

    return flash_encpt_cfg;
}

hi_bool is_burn_need_crypto(hi_void)
{
    hi_u8 flash_encpt_cfg = get_flash_crypto_cfg();
    if ((flash_encpt_cfg & 0x1) == ENCPT_CFG_FLAG) {
        hi_u8 flash_encpt_cnt = 0;
        hi_u32 i = 0;

        for (i = 0; i < CRYPTO_CNT_NUM; i++) {
            (hi_void)hi_efuse_read(g_efuse_id[i], &flash_encpt_cnt, (hi_u8)sizeof(hi_u8));
            if ((flash_encpt_cnt & 0x3) == 1) {
                g_writeable_efuse = g_efuse_id[i];
                return HI_TRUE;
            } else if ((flash_encpt_cnt & 0x3) == 0) {
                break;
            }
        }
    } else {
#ifdef CONFIG_FLASH_ENCRYPT_NOT_USE_EFUSE
        boot_crypto_load_cfg_from_nv();
        hi_flash_crypto_cnt *cfg = boot_crypto_get_cfg();
        if (cfg->flash_crypt_cnt == 0) {
            return HI_TRUE;
        }
#endif
    }
    return HI_FALSE;
}

hi_u32 boot_get_crypto_kernel_start(uintptr_t file_addr)
{
    hi_u32 flash_offset = 0;
    loaderboot_get_start_addr_offset(file_addr, &flash_offset);

    return flash_offset;
}

static hi_u32 crypto_decrypt_hash(hi_flash_crypto_content *key_content)
{
    hi_u32 ret;
    hi_u32 cs;
    hi_u32 content_size = (hi_u32)sizeof(hi_flash_crypto_content);

    hi_flash_crypto_content *content_tmp = (hi_flash_crypto_content *)boot_malloc(content_size);
    if (content_tmp == HI_NULL) {
        return HI_PRINT_ERRNO_MALLOC_EXAUST_ERR;
    }

    cs = (uintptr_t)(content_tmp) ^ content_size ^ (uintptr_t)(key_content) ^ content_size;
    ret = (hi_u32)memcpy_s(content_tmp, content_size, key_content, content_size, cs);
    if (ret != EOK) {
        goto fail;
    }

    hi_cipher_aes_ctrl aes_ctrl = {
        .random_en = HI_FALSE,
        .key_from = HI_CIPHER_AES_KEY_FROM_KDF,
        .work_mode = HI_CIPHER_AES_WORK_MODE_CBC,
        .key_len = HI_CIPHER_AES_KEY_LENGTH_256BIT,
    };
    cs = (uintptr_t)(aes_ctrl.iv) ^ (hi_u32)sizeof(aes_ctrl.iv) ^ (uintptr_t)(content_tmp->iv_nv) ^ IV_BYTE_LENGTH;
    ret = (hi_u32)memcpy_s(aes_ctrl.iv, sizeof(aes_ctrl.iv), content_tmp->iv_nv, IV_BYTE_LENGTH, cs);
    if (ret != EOK) {
        goto fail;
    }
    ret = hi_cipher_aes_config(&aes_ctrl);
    if (ret != HI_ERR_SUCCESS) {
        goto crypto_fail;
    }
    ret = hi_cipher_aes_crypto((uintptr_t)content_tmp->iv_content, (uintptr_t)key_content->iv_content,
        content_size - ROOT_SALT_LENGTH - IV_BYTE_LENGTH, HI_FALSE);
    if (ret != HI_ERR_SUCCESS) {
        goto crypto_fail;
    }

crypto_fail:
    (hi_void) hi_cipher_aes_destroy_config();
fail:
    loader_clear_content((hi_u8 *)content_tmp, content_size);
    crypto_mem_free(content_tmp);
    return ret;
}

static hi_u32 crypto_encrypt_hash(hi_flash_crypto_content *key_content)
{
    hi_cipher_aes_ctrl aes_ctrl;
    hi_u32 ret;
    hi_u32 content_size = (hi_u32)sizeof(hi_flash_crypto_content);
    hi_u32 encrypt_size = content_size - ROOT_SALT_LENGTH - IV_BYTE_LENGTH;

    hi_flash_crypto_content *data_tmp = (hi_flash_crypto_content *)boot_malloc(content_size);
    if (data_tmp == HI_NULL) {
        return HI_PRINT_ERRNO_MALLOC_EXAUST_ERR;
    }

    hi_u32 cs = (uintptr_t)(aes_ctrl.iv) ^ (hi_u32)sizeof(aes_ctrl.iv) ^ (uintptr_t)(key_content->iv_nv) ^
        IV_BYTE_LENGTH;
    ret = (hi_u32)memcpy_s(aes_ctrl.iv, sizeof(aes_ctrl.iv), key_content->iv_nv, IV_BYTE_LENGTH, cs);
    if (ret != EOK) {
        goto fail;
    }

    aes_ctrl.random_en = HI_FALSE;
    aes_ctrl.key_from = HI_CIPHER_AES_KEY_FROM_KDF;
    aes_ctrl.work_mode = HI_CIPHER_AES_WORK_MODE_CBC;
    aes_ctrl.key_len = HI_CIPHER_AES_KEY_LENGTH_256BIT;
    ret = hi_cipher_aes_config(&aes_ctrl);
    if (ret != HI_ERR_SUCCESS) {
        goto crypto_fail;
    }
    ret = hi_cipher_aes_crypto((uintptr_t)key_content->iv_content, (uintptr_t)(data_tmp->iv_content),
        encrypt_size, HI_TRUE);
    if (ret != HI_ERR_SUCCESS) {
        goto crypto_fail;
    }

    cs = (uintptr_t)(key_content->iv_content) ^ encrypt_size ^ (uintptr_t)(data_tmp->iv_content) ^ encrypt_size;
    ret = (hi_u32)memcpy_s(key_content->iv_content, encrypt_size, data_tmp->iv_content, encrypt_size, cs);

crypto_fail:
    (hi_void) hi_cipher_aes_destroy_config();
fail:
    loader_clear_content((hi_u8 *)data_tmp, content_size);
    crypto_mem_free(data_tmp);
    return ret;
}

static hi_u32 crypto_load_key_content(crypto_workkey_partition part, hi_flash_crypto_content *key_content)
{
    hi_u32 ret = HI_ERR_SUCCESS;
    hi_u8 hash[SHA_256_LENGTH];
    hi_u8 key_e[KEY_BYTE_LENGTH] = { 0 };

    hi_u32 cs = (uintptr_t)key_e ^ (hi_u32)sizeof(key_e) ^ 0x0 ^ KEY_BYTE_LENGTH;
    (hi_void) memset_s(key_e, sizeof(key_e), 0x0, KEY_BYTE_LENGTH, cs);
    if (part == CRYPTO_WORKKEY_KERNEL_A) {
        ret = hi_factory_nv_read(HI_NV_FTM_KERNELA_WORK_ID, key_content, sizeof(hi_flash_crypto_content), 0);
        if (ret != HI_ERR_SUCCESS) {
            goto fail;
        }
    } else if (part == CRYPTO_WORKKEY_KERNEL_A_BACKUP) {
        ret = hi_factory_nv_read(HI_NV_FTM_BACKUP_KERNELA_WORK_ID, key_content, sizeof(hi_flash_crypto_content), 0);
        if (ret != HI_ERR_SUCCESS) {
            goto fail;
        }
    }

    if (memcmp(key_content->work_text, key_e, KEY_BYTE_LENGTH) == HI_ERR_SUCCESS) {
        ret = HI_PRINT_ERRNO_CRYPTO_KEY_EMPTY_ERR;
        goto fail;
    }

    ret = crypto_decrypt_hash(key_content);
    if (ret != HI_ERR_SUCCESS) {
        goto fail;
    }

    ret = hi_cipher_hash_sha256((uintptr_t)(key_content->root_salt), sizeof(hi_flash_crypto_content) - SHA_256_LENGTH,
                                hash, SHA_256_LENGTH);
    if (ret != HI_ERR_SUCCESS) {
        goto fail;
    }
    if (memcmp(key_content->content_sh256, hash, SHA_256_LENGTH) != HI_ERR_SUCCESS) {
        ret = HI_PRINT_ERRNO_CRYPTO_KEY_INVALID_ERR;
        goto fail;
    }
fail:
    return ret;
}

static hi_u32 crypto_save_work_key(crypto_workkey_partition part, hi_flash_crypto_content *key_content)
{
    hi_u32 ret;
    hi_u32 cs;
    hi_u32 content_size = (hi_u32)sizeof(hi_flash_crypto_content);
    hi_flash_crypto_content *content_tmp = (hi_flash_crypto_content *)boot_malloc(content_size);
    if (content_tmp == HI_NULL) {
        return HI_PRINT_ERRNO_MALLOC_EXAUST_ERR;
    }

    cs = (uintptr_t)(content_tmp) ^ content_size ^ (uintptr_t)(key_content) ^ content_size;
    ret = (hi_u32)memcpy_s(content_tmp, content_size, key_content, content_size, cs);
    if (ret != EOK) {
        goto fail;
    }

    /* 先加密，再存到工厂区NV中 */
    ret = crypto_encrypt_hash(content_tmp);
    if (ret != HI_ERR_SUCCESS) {
        goto fail;
    }

    if ((hi_u32)part & CRYPTO_WORKKEY_KERNEL_A) {
        ret = hi_factory_nv_write(HI_NV_FTM_KERNELA_WORK_ID, content_tmp, content_size, 0);
        if (ret != HI_ERR_SUCCESS) {
            ret = HI_PRINT_ERRNO_CRYPTO_KEY_SAVE_ERR;
            goto fail;
        }
    }
    if ((hi_u32)part & CRYPTO_WORKKEY_KERNEL_A_BACKUP) {
        ret = hi_factory_nv_write(HI_NV_FTM_BACKUP_KERNELA_WORK_ID, content_tmp, content_size, 0);
        if (ret != HI_ERR_SUCCESS) {
            ret =  HI_PRINT_ERRNO_CRYPTO_KEY_SAVE_ERR;
            goto fail;
        }
    }

fail:
    loader_clear_content((hi_u8 *)content_tmp, content_size);
    crypto_mem_free(content_tmp);
    return ret;
}

static hi_u32 crypto_is_need_gen_key(hi_flash_crypto_content *key_content, hi_u8 *need_gen_key)
{
    hi_u32 cs;
    hi_flash_crypto_content tmp_content = {0};
    hi_u32 content_size = sizeof(hi_flash_crypto_content);

    hi_u32 ret = crypto_load_key_content(CRYPTO_WORKKEY_KERNEL_A, &tmp_content);
    if (ret != HI_ERR_SUCCESS) {
        ret = crypto_load_key_content(CRYPTO_WORKKEY_KERNEL_A_BACKUP, &tmp_content);
        if (ret == HI_PRINT_ERRNO_CRYPTO_KEY_EMPTY_ERR) {
            *need_gen_key = 1;
            return HI_ERR_SUCCESS;
        } else if (ret != HI_ERR_SUCCESS) {
            goto fail;
        } else {
            ret = crypto_save_work_key(CRYPTO_WORKKEY_KERNEL_A, &tmp_content);
        }
    }

    if (ret == HI_ERR_SUCCESS) {
        cs = (uintptr_t)(key_content) ^ content_size ^ (uintptr_t)(&tmp_content) ^ content_size;
        ret = (hi_u32)memcpy_s(key_content, content_size, &tmp_content, content_size, cs);
    }

    cs = (uintptr_t)(&tmp_content) ^ content_size ^ 0x0 ^ content_size;
    (hi_void)memset_s(&tmp_content, content_size, 0x0, content_size, cs);

fail:
    return ret;
}

static hi_u32 crypto_gen_key_content(hi_flash_crypto_content *key_content)
{
    hi_u8 salt[IV_BYTE_LENGTH];
    hi_u8 kdf_key[KEY_BYTE_LENGTH];
    hi_cipher_kdf_ctrl ctrl;

    (hi_void)hi_cipher_trng_get_random_bytes(salt, IV_BYTE_LENGTH);
    (hi_void)hi_cipher_trng_get_random_bytes(kdf_key, KEY_BYTE_LENGTH);
    (hi_void)hi_cipher_trng_get_random_bytes(key_content->iv_nv, IV_BYTE_LENGTH);
    (hi_void)hi_cipher_trng_get_random_bytes(key_content->iv_content, IV_BYTE_LENGTH);

    hi_u32 cs = (uintptr_t)(ctrl.key) ^ (hi_u32)sizeof(ctrl.key) ^ (uintptr_t)kdf_key ^ (hi_u32)sizeof(kdf_key);
    if ((hi_u32)memcpy_s(ctrl.key, sizeof(ctrl.key), kdf_key, sizeof(kdf_key), cs) != EOK) {
        return HI_ERR_FAILURE;
    }
    ctrl.salt = salt;
    ctrl.salt_len = sizeof(salt);
    ctrl.kdf_cnt = KDF_ITERATION_CNT;
    ctrl.kdf_mode = HI_CIPHER_SSS_KDF_KEY_DEVICE; /* 用户提供HUK值的方式 */
    if (hi_cipher_kdf_key_derive(&ctrl) != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }

    cs = (uintptr_t)(key_content->work_text) ^ KEY_BYTE_LENGTH ^ (uintptr_t)(ctrl.result) ^
        (hi_u32)sizeof(ctrl.result);
    if (memcpy_s(key_content->work_text, KEY_BYTE_LENGTH, ctrl.result, sizeof(ctrl.result), cs) != EOK) {
        return HI_ERR_FAILURE;
    }

    if (hi_cipher_hash_sha256((uintptr_t)(key_content->root_salt), sizeof(hi_flash_crypto_content) - SHA_256_LENGTH,
                              key_content->content_sh256, SHA_256_LENGTH) != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }

    return HI_ERR_SUCCESS;
}

static hi_u32 crypto_encrypt_data(hi_flash_crypto_content *content, hi_u32 flash_addr, hi_u32 len)
{
    hi_u32 ret = HI_ERR_FAILURE;
    hi_cipher_aes_ctrl aes_ctrl;

    hi_u8 *fw_cyp_data = boot_malloc(len);
    if (fw_cyp_data == HI_NULL) {
        return HI_PRINT_ERRNO_CRYPTO_PREPARE_ERR;
    }

    hi_u32 cs = (uintptr_t)(aes_ctrl.key) ^ (hi_u32)sizeof(aes_ctrl.key) ^
        (uintptr_t)(content->work_text) ^ KEY_BYTE_LENGTH;
    if (memcpy_s(aes_ctrl.key, sizeof(aes_ctrl.key), content->work_text, KEY_BYTE_LENGTH, cs) != EOK) {
        goto fail;
    }

    cs = (uintptr_t)(aes_ctrl.iv) ^ (hi_u32)sizeof(aes_ctrl.iv) ^ (uintptr_t)(content->iv_content) ^ IV_BYTE_LENGTH;
    if (memcpy_s(aes_ctrl.iv, sizeof(aes_ctrl.iv), content->iv_content, IV_BYTE_LENGTH, cs) != EOK) {
        goto fail;
    }

    crpto_set_aes_ctrl_default_value(&aes_ctrl);
    ret = hi_cipher_aes_config(&aes_ctrl);
    if (ret != HI_ERR_SUCCESS) {
        goto fail;
    }

    ret = hi_cipher_aes_crypto(flash_addr + SFC_BUFFER_BASE_ADDRESS, (uintptr_t)fw_cyp_data, len, HI_TRUE);
    if (ret != HI_ERR_SUCCESS) {
        goto crypto_fail;
    }

    ret = g_flash_cmd_funcs.write(flash_addr, len, (hi_u8 *)fw_cyp_data, HI_TRUE);
    if (ret != HI_ERR_SUCCESS) {
        goto crypto_fail;
    }

crypto_fail:
    (hi_void) hi_cipher_aes_destroy_config();
fail:
    crypto_mem_free(fw_cyp_data);
    return ret;
}

hi_u32 crypto_save_encrypt_status(hi_void)
{
    hi_u32 ret;
    hi_u8 flash_encpt_cfg = get_flash_crypto_cfg();
    if ((flash_encpt_cfg & 0x1) == ENCPT_CFG_FLAG) {
        hi_u8 efuse_val = 0x3;
        ret = hi_efuse_write(g_writeable_efuse, &efuse_val);
        if (ret != HI_ERR_SUCCESS) {
            return ret;
        }
    } else {
#ifdef CONFIG_FLASH_ENCRYPT_NOT_USE_EFUSE
        ret = boot_set_crypto_finish_flag();
        if (ret != HI_ERR_SUCCESS) {
            return ret;
        }
#endif
    }

    return HI_ERR_SUCCESS;
}

hi_u32 crypto_burn_encrypt(uintptr_t file_addr)
{
    hi_u8 need_gen_key = 0;
    hi_u32 crypto_kernel_addr = boot_get_crypto_kernel_start(file_addr);

    hi_flash_crypto_content *key_content = (hi_flash_crypto_content *)boot_malloc(sizeof(hi_flash_crypto_content));
    if (key_content == HI_NULL) {
        return HI_PRINT_ERRNO_CRYPTO_PREPARE_ERR;
    }

    hi_u32 ret = crypto_prepare(key_content);
    if (ret != HI_ERR_SUCCESS) {
        loader_clear_content((hi_u8 *)key_content, sizeof(hi_flash_crypto_content));
        crypto_mem_free(key_content);
        return HI_PRINT_ERRNO_CRYPTO_PREPARE_ERR;
    }

    ret = crypto_is_need_gen_key(key_content, &need_gen_key);
    if (ret != HI_ERR_SUCCESS) {
        goto fail;
    }

    if (need_gen_key == 1) {
        ret = crypto_gen_key_content(key_content);
        if (ret != HI_ERR_SUCCESS) {
            goto fail;
        }

        if (crypto_save_work_key(CRYPTO_WORKKEY_KERNEL_A_BOTH, key_content) != HI_ERR_SUCCESS) {
            goto fail;
        }
    }

    ret = crypto_encrypt_data(key_content, crypto_kernel_addr, CRYPTO_KERNEL_LENGTH);
    if (ret != HI_ERR_SUCCESS) {
        ret = HI_PRINT_ERRNO_CRYPTO_FW_ENCRYPT_ERR;
        goto fail;
    }

    ret = crypto_save_encrypt_status();
    if (ret != HI_ERR_SUCCESS) {
        goto fail;
    }
fail:
    loader_clear_content((hi_u8 *)key_content, sizeof(hi_flash_crypto_content));
    crypto_mem_free(key_content);
    crypto_destory();
    return ret;
}

hi_u32 crypto_check_encrypt(hi_void)
{
    hi_u32 ret;
    ret = hi_factory_nv_init(HI_FNV_DEFAULT_ADDR, HI_NV_DEFAULT_TOTAL_SIZE, HI_NV_DEFAULT_BLOCK_SIZE); /* NV初始化 */
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    ret = hi_flash_partition_init();
    if (ret != HI_ERR_SUCCESS) { /* use flash table */
        return ret;
    }

    hi_bool burn_encrypt_flag = is_burn_need_crypto();
    if (burn_encrypt_flag == HI_TRUE) {
        hi_flash_partition_table *partition = hi_get_partition_table();
        uintptr_t kernel_a_addr = partition->table[HI_FLASH_PARTITON_KERNEL_A].addr;

        /* 烧写flash加密流程 */
        ret = crypto_burn_encrypt(kernel_a_addr);
        if (ret != HI_ERR_SUCCESS) {
            boot_put_errno(ret);
            return ret;
        }
    }

    return HI_ERR_SUCCESS;
}

hi_u32 crypto_encrypt_factory_image(uintptr_t file_addr)
{
    /* 工厂测试程序加密流程 */
    hi_u32 ret = crypto_burn_encrypt(file_addr);
    if (ret != HI_ERR_SUCCESS) {
        boot_put_errno(ret);
        return ret;
    }

    return HI_ERR_SUCCESS;
}

#endif
