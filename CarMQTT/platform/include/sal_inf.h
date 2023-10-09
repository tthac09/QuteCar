/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: SAL提供给Modem内部使用的接口,仅供 MAC/SAL/DRV/OS使用,APP禁止使用.
 * Author: wangjun
 * Create: 2019-03-04
 */

#ifndef __SAL_INF_H__
#define __SAL_INF_H__
#include <hi_types.h>
HI_START_HEADER
#include <hi_sal.h>
#include <hi_task.h>
#include <hi_ft_nv.h>
#include <hi_mdm.h>
#include <hi_config.h>

typedef struct {
    hi_bool enable_save;
    hi_bool delay_over;
    hi_bool has_save;
    hi_u8 rsv;
    hi_u32 rst_times;
    hi_u32 handle;
} sal_rst_times_ctrl;

hi_u32 sal_rst_times_init(hi_void);
sal_rst_times_ctrl *sal_get_rst_times_ctrl(hi_void);
hi_u32 sal_uart_port_allocation(hi_uart_func_idx uart_func_id, hi_u8 *uart_id);

HI_END_HEADER
#endif /* __SAL_INF_H__ */

