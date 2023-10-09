/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: sal head file.
 * Author: Hisilicon
 * Create: 2012-12-22
 */
#ifndef __HI_SAL_NV_H__
#define __HI_SAL_NV_H__

#define  HI_NV_SYS_RST_TIMES        0x40
#define  HI_NV_SYS_RST_CFG_ID       0x41
#define  HI_NV_SYS_UART_PORT_ID     0x42

typedef struct {
    hi_u32 rst_times;   /**< 重启次数统计 */
    hi_u32 enable_save;   /**< 使能重启次数统计 */
} hi_sys_reset_times;

/* 主动复位配置 */
typedef struct {
    hi_u8 enable_rst;           /**< 主动复位使能开关 */
    hi_u8 rsv[3];               /**< 预留3byte */
    hi_u32 secure_begin_time;   /**< 定期重启时间下限，单位: 秒 */
    hi_u32 secure_end_time;     /**< 定期重启时间上限，单位: 秒 */
    hi_u32 max_time_usr0;       /**< 用户预留0时间门限，单位: 秒 */
    hi_u32 max_time_usr1;       /**< 用户预留1时间门限，单位: 秒 */
} hi_nv_reset_cfg_id;

typedef enum {
    UART_FUNC_AT,
    UART_FUNC_SHELL,
    UART_FUNC_DIAG,
    UART_FUNC_SIGMA,
    UART_FUNC_MAX,
} hi_uart_func_idx;

/* uart port allocation */
typedef struct {
    hi_u8 uart_port_at;
    hi_u8 uart_port_debug;
    hi_u8 uart_port_sigma;
    hi_u8 uart_port_reserved;
} hi_nv_uart_port_alloc;

#endif /* __HI_SAL_NV_H__ */

