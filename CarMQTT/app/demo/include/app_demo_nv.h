/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:example of using NV APIs to pair NV devices.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __APP_DEMO_NV_H__
#define __APP_DEMO_NV_H__

#include <hi_types_base.h>

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <hi_stdlib.h>
#include <hi_early_debug.h>
#include <hi_nv.h>
#include <hi_flash.h>
#include <hi_partition_table.h>
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define  HI_NV_DEMO_RST_CFG_ID  0x22
#define  HI_FACTORY_NV_DEMO_ID  0x1
/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
typedef struct {
    hi_u32 nv_demo_test_num0;
    hi_u32 nv_demo_test_num1;
    hi_u32 nv_demo_test_num2;
    hi_u32 nv_demo_test_num3;
} hi_nv_demo_tset_cfg;

typedef struct {
    hi_s32 nv_factory_demo_test_num0;
    hi_s32 nv_factory_demo_test_num1;
    hi_s32 nv_factory_demo_test_num2;
} hi_factory_nv_demo_tset_cfg;
/*****************************************************************************
  10 函数声明
*****************************************************************************/
hi_void nv_demo(hi_void);
hi_void factory_nv_demo(hi_void);

#endif