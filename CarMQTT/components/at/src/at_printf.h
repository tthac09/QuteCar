/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description:  AT command output interface inner header. \n
 * Author: hisilicon
 * Create: 2020-01-02
 */

#ifndef __AT_PRINTF_H__
#define __AT_PRINTF_H__

#include <printf.h>

hi_void serial_putc_at(hi_char c);

extern uintptr_t g_at_uart_baseaddr;
extern LITE_OS_SEC_TEXT INT32 __dprintf(const CHAR *format, va_list arg_v, OUTPUT_FUNC pFputc, VOID *cookie);

hi_s32 hi_at_printf_crashinfo(const hi_char *fmt, ...);

#endif
