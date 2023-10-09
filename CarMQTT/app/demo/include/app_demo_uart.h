/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:example of using UART APIs to pair UART devices.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __APP_DEMO_UART_H__
#define __APP_DEMO_UART_H__

#include <hi_types_base.h>
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <hi_early_debug.h>
#include <hi_task.h>
#include <hi_uart.h>
#include <app_demo_uart.h>
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define WRITE_BY_INT
#define UART_DEMO_TASK_STAK_SIZE 2048
#define UART_DEMO_TASK_PRIORITY  25
#define DEMO_UART_NUM            HI_UART_IDX_2
#define UART_BUFF_SIZE           32
/*****************************************************************************
  10 函数声明
*****************************************************************************/
static hi_void *uart_demo_task(hi_void *param);
hi_void uart_demo(hi_void);
#endif