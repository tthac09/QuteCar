/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: gpio demo headfile.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __APP_DEMO_IO_GPIO_H__
#define __APP_DEMO_IO_GPIO_H__

#include <hi_types_base.h>
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <hi_io.h>
#include <hi_early_debug.h>
#include <hi_gpio.h>
#include <hi_task.h>
/*****************************************************************************
  10 函数声明
*****************************************************************************/
/* gpio callback func */
hi_void gpio_isr_func(hi_void *arg);
hi_void io_gpio_demo(hi_void);
/* gpio callback demo */
hi_void gpio_isr_demo(hi_void);
hi_void app_demo_custom_io_gpio(hi_void);
#endif