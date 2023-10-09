/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:example of using I2C APIs to pair I2C devices.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __APP_DEMO_I2C_H__
#define __APP_DEMO_I2C_H__

#include <hi_types_base.h>

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <hi_i2c.h>
#include <hi_early_debug.h>
#include <hi_stdlib.h>
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define I2C_REG_ARRAY_LEN 64
#define ES8311_DEV_ADDR 0x30          /* 11000 0 */
#define ES8311_REG_ADDR 0x10
#define I2C_SEND_LEN_2  2
#define I2C_RECV_LEN_1  1
/*****************************************************************************
  10 函数声明
*****************************************************************************/
hi_void i2c_demo_send_data_init(hi_void);
hi_u32 i2c_demo_write(hi_i2c_idx id, hi_u16 device_addr, hi_u32 send_len);
hi_u32 i2c_demo_writeread(hi_i2c_idx id, hi_u16 device_addr, hi_u32 recv_len);
hi_void i2c_demo(hi_i2c_idx id);
hi_void app_demo_custom_i2c(hi_void);

#endif
