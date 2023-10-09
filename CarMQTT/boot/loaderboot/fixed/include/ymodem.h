/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Ymodem protocol.
 * Author: Hisilicon
 * Create: 2019-10-22
 */

#ifndef __YMODEM_H__
#define __YMODEM_H__

#include <hi_types.h>
#define LOADER_RAM_TEXT       __attribute__ ((section(".ram.text")))

hi_u32  ymodem_open(hi_void);
hi_void ymodem_close(hi_void);
hi_u32  ymodem_read(hi_char* buf, hi_u32 size, hi_u32 cs);
hi_u32  ymodem_get_length(hi_void);
hi_u32 loader_ymodem_open(hi_void);

#endif

