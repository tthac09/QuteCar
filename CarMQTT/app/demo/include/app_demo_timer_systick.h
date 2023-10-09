/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:example of using TIMER APIs to pair TIMER devices.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __APP_DEMO_TIMER_SYSTICK_H__
#define __APP_DEMO_TIMER_SYSTICK_H__

#include <hi_types_base.h>

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <hi_hrtimer.h>
#include <hi_early_debug.h>
#include <hi_systick.h>
#include <hi_task.h>
#include <hi_time.h>
#include <hi_timer.h>
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define NO_CALLBACK 0xffff
#define HRTIMER_EXPIRE 2000       /* expire timer is 2000us */
#define WORK_TICK_DELAY_US 31     /* 1/32K */
#define APP_DEMO_TIMER_WAIT 10000 /* wait 10s */
/*****************************************************************************
  10 函数声明
*****************************************************************************/
hi_void hr_timer_callback(hi_u32 data);
hi_void app_demo_hrtimer(hi_void);
hi_void app_demo_systick(hi_void);
static hi_void app_demo_timer_handle(hi_u32 data);
hi_void app_demo_timer(hi_void);
#endif