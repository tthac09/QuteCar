/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: 工厂模式下的NV项设置
 * Author: shenhankun
 * Create: 2019-10-26
 */

#ifndef __FT_NV_H__
#define __FT_NV_H__
#include <hi_types.h>

/******************************************************************************
 NV ID 定义 [0xF00, 0xFFF]
 *****************************************************************************/
#define HI_NV_FTM_FLASH_PARTIRION_TABLE_ID   0x02
#define HI_FTM_PRODUCT_ID_RANGE_MRS          500

#define HI_NV_FTM_STARTUP_CFG_ID   0x3 /* hi_nv_ftm_startup_cfg */
#define HI_NV_FTM_FACTORY_MODE     0x9 /* hi_nv_ftm_factory_mode */

typedef struct {
    uintptr_t addr_start; /* boot start address */
    hi_u16 mode;          /* upgrade mode */
    hi_u8 file_type;      /* file type:boot or code+nv */
    hi_u8 refresh_nv;     /* refresh nv when the flag bit 0x55 is read */
    hi_u8 reset_cnt;     /* number of restarts in upgrade mode */
    hi_u8 cnt_max;       /* the maximum number of restarts (default value : 3) */
    hi_u16 reserved1;
    uintptr_t addr_write; /* write kernel upgrade file address */
    hi_u32 reserved2;    /* reserved */
} hi_nv_ftm_startup_cfg;

typedef struct {
    hi_u32 factory_mode;          /* 0:normal_mode;1:factory_mode */
    uintptr_t factory_addr_start; /* factory bin start address */
    hi_u32 factory_size;          /* factory bin size */
    hi_u32 factory_valid;         /* 0:invalid;1:valid */
}hi_nv_ftm_factory_mode;

#endif  /* __FT_NV_H__ */

