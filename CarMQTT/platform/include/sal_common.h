/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: SAL模块内部接口文件
 * Author: Hisilicon
 * Create: 2016-07-29
 */

#ifndef __SAL_COMMON_H__
#define __SAL_COMMON_H__

#include <dfx_sal.h>
#include "hi_crash.h"

hi_void sal_show_run_time(hi_void);

// ****************************************************************************
// 消息定义
// ****************************************************************************
#define ID_MSG_DIAG_BASE 0x100

enum {
    ID_MSG_DIAG_DATA_TX = (ID_MSG_DIAG_BASE + 1),  /* 数据包上报 */
    ID_MSG_DIAG_DATA_USER_TX,                      /* 用户数据包上报 */
    ID_MSG_DIAG_ACK_RX,
    ID_MSG_DIAG_ACK_TX,                            /* ACK包，需要释放参数指针 */
    ID_MSG_DIAG_RX,
    ID_MSG_DIAG_TIMER,
    ID_MSG_DIAG_MSG,
    ID_MSG_DIAG_ACK_MDM_RX
};

#define diag_msg_free(msg)  hi_free(HI_MOD_ID_SAL_DIAG, (hi_pvoid)((msg)->param[0]))
#define diag_msg_free0(msg) hi_free(HI_MOD_ID_SAL_DIAG, (hi_pvoid)((msg)->param[0]))
#define diag_buffering_msg_free(msg) diag_free_buffer((diag_buffer_id)hi_lou16((msg)->param[3]), \
    (hi_pvoid)(uintptr_t)((msg)->param[0]))

// Helper routine
#define init_exception_polling_wait() hi_sleep(HI_DMS_CHL_EXCEPTION_POLLING_WAIT)
#define uart_exception_polling_wait() hi_sleep(HI_DMS_UART_EXCEPTION_POLLING_WAIT)

// ****************************************************************************
/* 初始化过程中, 关键阶段(成功和失败)的统计 */
// ****************************************************************************
HI_EXTERN hi_void flash_set_crash_flag(hi_void);
HI_EXTERN hi_u32 flash_read_crash(HI_IN hi_u32 addr, HI_IN hi_void *data, HI_IN hi_u32 size);
HI_EXTERN hi_u32 flash_write_crash(HI_IN hi_u32 addr, HI_IN hi_void *data, HI_IN hi_u32 size);
HI_EXTERN hi_u32 flash_erase_crash(HI_IN hi_u32 addr, HI_IN hi_u32 size);

#if defined(PRODUCT_CFG_DIAG_FRM_ERR_PK_RPT)
hi_u32 diag_chl_error_report(hi_u32 ret, hi_u32 id);
#else
#define diag_chl_error_report(ret, id) \
    do {                               \
    } while (0)
#endif

/* ERR REPORT ONLY for DIAG inside */
#define diag_chl_err_rpt(id, ret)                                   \
    do {                                                            \
        if ((ret) != HI_ERR_SUCCESS && (ret) != HI_ERR_DIAG_CONSUMED) { \
            diag_chl_error_report((ret), (id));                         \
        }                                                           \
    } while (0)

#define DIAG_ERR_RPT_LEN 48

HI_EXTERN hi_stat_diag g_stat_diag;

hi_u32 hi_syserr_get_at_printf(hi_syserr_info *info);

#define CRASH_INFO_EID_SAVE_ADDR 6
#define CRASH_INFO_EID_SAVE_SIZE 4
#define CRASH_INFO_RID_SAVE_ADDR 10
#define CRASH_INFO_RID_SAVE_SIZE 6

#endif /* __SAL_COMMON_H__ */

