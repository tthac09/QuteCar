/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Get upg start up info.
 * Author: Hisilicon
 * Create: 2019-12-10
 */

#include <upg_start_up.h>

hi_u32 upg_save_start_up_nv(hi_nv_ftm_startup_cfg *cfg)
{
    hi_u32 ret = hi_factory_nv_write(HI_NV_FTM_STARTUP_CFG_ID, cfg, sizeof(hi_nv_ftm_startup_cfg), 0);
    upg_print("[upg save nv]write,addr_start:0x%x \r\n", cfg->addr_start);
    if (ret != HI_ERR_SUCCESS) {
        upg_print("[upg save nv]write fail,ret:0x%x \r\n", ret);
        return ret;
    }
    return ret;
}

hi_u32 upg_check_start_addr(hi_u32 addr)
{
    hi_flash_partition_table *partition = hi_get_partition_table();
    uintptr_t kernel_a_addr = partition->table[HI_FLASH_PARTITON_KERNEL_A].addr;
#if defined(CONFIG_COMPRESSION_OTA_SUPPORT)
    if (addr != kernel_a_addr) {
        return HI_ERR_UPG_ADD_START;
    }
#else
    uintptr_t kernel_b_addr = partition->table[HI_FLASH_PARTITON_KERNEL_B].addr;
    if ((addr != kernel_a_addr) && (addr != kernel_b_addr)) {
        /* cppcheck误报，uintptr_t是unsigned int类型，不是unsigned long long类型 */
        return HI_ERR_UPG_ADD_START;
    }
#endif
    return HI_ERR_SUCCESS;
}

hi_u32 upg_get_start_up_nv(hi_nv_ftm_startup_cfg *cfg)
{
    hi_nv_ftm_startup_cfg nv_cfg = { 0 };
    hi_flash_partition_table *partition = hi_get_partition_table();
    uintptr_t kernel_a_addr = partition->table[HI_FLASH_PARTITON_KERNEL_A].addr;
    hi_u32 ret = hi_factory_nv_read(HI_NV_FTM_STARTUP_CFG_ID, &nv_cfg, sizeof(hi_nv_ftm_startup_cfg), 0);
    if (ret != HI_ERR_SUCCESS) {
        upg_print("[upg get start nv]read fail:0x%x \r\n", ret);
        (hi_void) memset_s(cfg, sizeof(hi_nv_ftm_startup_cfg), 0, sizeof(hi_nv_ftm_startup_cfg));
        cfg->addr_start = kernel_a_addr;
        cfg->mode = HI_UPG_MODE_UPGRADE;
        cfg->cnt_max = UPG_MAX_BACKUP_CNT;
        return HI_ERR_SUCCESS;
    }

    ret = upg_check_start_addr(nv_cfg.addr_start);
    if (ret != HI_ERR_SUCCESS) {
        upg_print("[upg get start nv]check addr fail:0x%x \r\n", nv_cfg.addr_start);
        (hi_void) memset_s(cfg, sizeof(hi_nv_ftm_startup_cfg), 0, sizeof(hi_nv_ftm_startup_cfg));
        cfg->addr_start = kernel_a_addr;
        cfg->mode = HI_UPG_MODE_UPGRADE;
        cfg->cnt_max = UPG_MAX_BACKUP_CNT;
    } else {
        if (memcpy_s(cfg, sizeof(hi_nv_ftm_startup_cfg), &nv_cfg, sizeof(hi_nv_ftm_startup_cfg)) != EOK) {
            return HI_ERR_FAILURE;
        }
    }

    return HI_ERR_SUCCESS;
}


