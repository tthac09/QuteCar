/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: AT cmd table.
 * Author: liangguangrui
 * Create: 2019-10-15
 */
#include <hi_stdlib.h>
#include <at.h>
#include <hi_at.h>
#include <at_cmd.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

HI_PRV at_cmd_func_list g_at_cmd_list = { 0 };

at_cmd_func_list* at_get_list(hi_void)
{
    return (at_cmd_func_list*)&g_at_cmd_list;
}

at_cmd_func *at_find_proc_func(const at_cmd_attr *cmd_parsed)
{
    at_cmd_func_list *cmd_list = at_get_list();
    hi_u32 i = 0;

    for (i = 0; i < AT_CMD_LIST_NUM; i++) {
        hi_u16 j = 0;

        for (j = 0; j < cmd_list->at_cmd_num[i]; j++) {
            at_cmd_func *cmd_func = (at_cmd_func *) ((cmd_list->at_cmd_list[i] + j));
            if ((cmd_parsed->at_cmd_len == cmd_func->at_cmd_len) &&
                (strcmp(cmd_parsed->cmd_name, cmd_func->at_cmd_name) == 0)) {
                return cmd_func;
            }
        }
    }

    return HI_NULL;
}

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif
