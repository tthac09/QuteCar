/*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
* Description: Debug information output interfaces.
* Author: HiSilicon
* Create: 2019-4-3
*/

#ifndef __HI_EARLY_DEBUG_H__
#define __HI_EARLY_DEBUG_H__
#include <stdarg.h>

void edb_put_str_only(const char *s);
void edb_put_str_p0(char *str);
void edb_put_str_p1(char *str, unsigned int p1);
void edb_put_str_p2(char *str, unsigned int p1, unsigned int p2);
void edb_put_str_p3(char *str, unsigned int p1, unsigned int p2, unsigned int p3);
void edb_put_str_p4(char *str, unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4);
void edb_put_buf(char *buf, unsigned int len);
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list va);
int dprintf(const char *fmt, ...);
void edb_set_print_level(unsigned int level);

#endif

