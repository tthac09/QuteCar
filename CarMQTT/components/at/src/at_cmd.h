/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: AT cmd table.
 * Author: liangguangrui
 * Create: 2019-10-15
 */
#ifndef __AT_CMD_H__
#define __AT_CMD_H__

#include "at.h"
#include <hi_at.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

at_cmd_func_list *at_get_list(hi_void);
at_cmd_func *at_find_proc_func(const at_cmd_attr *cmd_parsed);

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif
