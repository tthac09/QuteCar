/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: app main.
 * Author: Hisilicon
 * Create: 2019-03-04
 */

#ifndef __APP_MAIN_H__
#define __APP_MAIN_H__

#include <hi_types_base.h>

typedef struct {
    hi_u16 gpio6_cfg;
    hi_u16 gpio8_cfg;
    hi_u16 gpio10_cfg;
    hi_u16 gpio11_cfg;
    hi_u16 gpio12_cfg;
    hi_u16 gpio13_cfg;
    hi_u16 sfc_csn_cfg;
} app_iocfg_backup;

#endif // __APP_MAIN_H__
