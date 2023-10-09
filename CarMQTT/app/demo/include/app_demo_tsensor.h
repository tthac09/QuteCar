/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:tsensor_demo headfile.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __APP_DEMO_I2S_H__
#define __APP_DEMO_I2S_H__

#include <hi_types_base.h>
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <hi_early_debug.h>
#include <hi_task.h>
#include <hi_systick.h>
#include <hi_time.h>
#include <hi_tsensor.h>
#include <app_demo_tsensor.h>
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define TEMPERRATURE_OUT_THRESHOLD_LOW    (-30)
#define TEMPERRATURE_OUT_THRESHOLD_HIGH   100
#define TEMPERRATURE_OVER_THRESHOLD       115
#define TEMPERRATURE_POWER_DOWN_THRESHOLD 125
#define TSENSOR_CALIBRATE_CODE            0x07
#define TSENSOR_CALIBRATE_CODE_LOAD_EFUSE 0
#define TSENSOR_CALIBRATE_CODE_LOAD_REG   1
#define TSENSOR_PERIOD_VALUE              500
#define TSENSOR_TEST_INTERVAL             1000
#define TSENSOR_TEST_LOOP                 10
#define TSENSOR_GET_TEMPERRATURE_INTERVAL (TSENSOR_PERIOD_VALUE * 31.25 + 192 * 16)
/*****************************************************************************
  10 函数声明
*****************************************************************************/
static hi_void tensor_collect_finish_irq(hi_s16 irq_temperature);
static hi_void tensor_outtemp_irq(hi_s16 irq_temperature);
static hi_void tensor_overtemp_irq(hi_s16 irq_temperature);
static hi_void read_temprature_none_irq_single(hi_void);
hi_void tsensor_demo(hi_void);
#endif
