/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: WAL layer external API interface implementation.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __AT_IO_H__
#define __AT_IO_H__

#include <hi_gpio.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

void hi_at_io_cmd_register(void);

hi_u32 gpio_get_dir(hi_gpio_idx id, hi_gpio_dir *dir);

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif
