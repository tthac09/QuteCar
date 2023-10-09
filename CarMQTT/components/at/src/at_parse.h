/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Parse AT cmd.
 * Author: liangguangrui
 * Create: 2019-10-15
 */
#ifndef __AT_PARSE_H__
#define __AT_PARSE_H__

#include "at.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

hi_u32 at_cmd_parse(hi_char *cmd_line, at_cmd_attr *cmd_parsed);

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif