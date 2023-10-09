/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:example of using ADC APIs to pair ADC devices.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __APP_DEMO_ADC_H__
#define __APP_DEMO_ADC_H__

#include <hi_types_base.h>

#define APP_DEMO_ADC
#ifdef APP_DEMO_ADC
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <hi_adc.h>
#include <hi_stdlib.h>
#include <hi_early_debug.h>
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define ADC_TEST_LENGTH  128
#define VLT_MIN 100
/*****************************************************************************
  10 函数声明
*****************************************************************************/
hi_void app_demo_adc_test(hi_void);
#endif
#endif