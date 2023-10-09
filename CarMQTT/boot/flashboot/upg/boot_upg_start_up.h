/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Interface of upg start up under boot.
 * Author: Hisilicon
 * Create: 2019-12-28
 */

#ifndef __BOOT_UPG_START_UP_H
#define __BOOT_UPG_START_UP_H

#include <boot_upg_common.h>

#define ENV_REFRESH_NV 0x55

#define HI_UPG_MODE_NORMAL  0 /* normal mode */
#define HI_UPG_MODE_UPGRADE 1 /* upgrade mode */
#define UPG_MAX_BACKUP_CNT  3

#define HI_NV_FTM_STARTUP_CFG_ID 0x3
#define HI_NV_FTM_FACTORY_MODE 0x9

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
    hi_u32 factory_mode;          /* 0:normal_mode;1:factory_mode */
    uintptr_t factory_addr_start; /* factory bin start address */
    hi_u32 factory_size;          /* factory bin size */
    hi_u32 factory_valid;         /* 0:invalid;1:valid */
}hi_nv_ftm_factory_mode;

hi_u32 boot_upg_save_cfg_to_nv(hi_void);
hi_void boot_upg_load_cfg_from_nv(hi_void);
hi_nv_ftm_startup_cfg *boot_upg_get_cfg(hi_void);

#endif /* __BOOT_UPG_START_UP_H */
