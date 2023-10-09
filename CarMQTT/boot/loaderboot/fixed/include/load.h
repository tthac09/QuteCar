/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Boot secure command head file.
 * Author: wangjian
 * Create: 2019-07-22
 */

#ifndef __LOAD_H__
#define __LOAD_H__

#include <hi_boot_rom.h>

extern hi_char *g_ymodem_buf;
hi_u32 load_malloc_init(hi_void);
hi_s32 load_malloc_deinit(hi_void);
hi_u32 load_serial_ymodem(hi_u32 offset, hi_u32 *addr, hi_u32 check_sum);

#endif
