/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:example of using FLASH APIs to pair FLASH devices.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __APP_DEMO_FLASH_H__
#define __APP_DEMO_FLASH_H__

#include <hi_types_base.h>

#ifdef TEST_KERNEL_FLASH
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <hi_flash.h>
#include <hi_task.h>
#include <hi_stdlib.h>
#include <hi_watchdog.h>
#include <hi_early_debug.h>
#include <hi_time.h>
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define TEST_SIZE 0x1000
#define TEST_FLASH_OFFSET 0x1FF000
/*****************************************************************************
  10 函数声明
*****************************************************************************/
hi_void cmd_test_flash(hi_u32 test_time, hi_u32 test_mode);
#endif
#endif