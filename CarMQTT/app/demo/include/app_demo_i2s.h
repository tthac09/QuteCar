/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:example of using I2S APIs to pair I2S devices.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __APP_DEMO_I2S_H__
#define __APP_DEMO_I2S_H__

#include <hi_types_base.h>
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <hi_errno.h>
#include <hi_mem.h>
#include <hi_event.h>
#include <hi_time.h>
#include <hi_task.h>
#include <hi_early_debug.h>
#include <hi_config.h>
#include <hi_stdlib.h>
#include <hi_i2s.h>
#include <hi_i2c.h>
#include <hi_flash.h>
#include <es8311_codec.h>
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define I2S_TEST_TASK_STAK_SIZE     2048
#define I2S_TEST_TASK_PRIORITY      28
#define AUDIO_PLAY_BUF_SIZE         4096
#define AUDIO_RECORD_BUF_SIZE       3072
#define AUDIO_RECORD_FINISH_BIT     (1 << 0)
#define ALL_AUDIO_RECORD_FINISH_BIT (1 << 1)

#define AUDIO_PLAY_FIEL_MODE         0
#define AUDIO_RECORD_AND_PLAY_MODE   1
#ifndef I2S_TEST_DEBUG
#define i2s_print(ftm...) do {printf(ftm);}while (0);
#else
#define i2s_print(ftm...)
#endif
/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
typedef struct {
    hi_u32 flash_start_addr;
    hi_u32 data_len;
} audio_map;

typedef struct {
    hi_u8 *play_buf;
    hi_u8 *record_buf;
} test_audio_attr;
/*****************************************************************************
  10 函数声明
*****************************************************************************/
hi_void i2s_demo(hi_void);
#endif
