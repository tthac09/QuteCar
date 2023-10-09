/*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved. \n
* Description: dfx_sys.h \n
* Author: Hisilicon \n
* Create: 2019-01-03
*/

#ifndef __DFX_SYS_H__
#define __DFX_SYS_H__

#include "hi_types.h"
#include "hi_mdm_types.h"
#include <hi_ft_nv.h>

HI_START_HEADER

typedef struct {
    hi_u16 send_uart_fail_cnt;
    hi_u16 ack_ind_malloc_fail_cnt;
    hi_u16 msg_malloc_fail_cnt;
    hi_u16 msg_send_fail_cnt;
    hi_u16 msg_overbig_cnt;
    hi_u16 ind_send_fail_cnt;
    hi_u16 ind_malloc_fail_cnt;
    hi_u8 diag_queue_used_cnt;
    hi_u8 diag_queue_total_cnt;
    hi_u8 dec_fail_cnt;
    hi_u8 enc_fail_cnt;
    hi_u16 pkt_size_err_cnt;
    hi_u32 local_req_cnt;
    hi_u16 req_cache_overflow_cnt;
    hi_u8 conn_excep_cnt;
    hi_u8 conn_bu_cnt;
    hi_u8 chl_busy_cnt;
    hi_u8 req_overbig1_cnt;
    hi_u8 cmd_list_total_cnt;  /**< 总共支持注册的命令列表个数 */
    hi_u8 cmd_list_used_cnt;   /**< 已经注册的命令列表个数 */
    hi_u8 stat_list_total_cnt; /**< 总共支持注册的统计量对象列表个数 */
    hi_u8 stat_list_used_cnt;  /**< 已经注册的统计量对象列表个数 */
    hi_u8 req_overbig2_cnt;
    hi_u8 invalid_dec_id;
    hi_u8 heart_beat_timeout_cnt;
    hi_u8 rx_start_flag_wrong_cnt;
    hi_u8 rx_cs_wrong_cnt;
    hi_u8 rx_pkt_data_size_wrong_cnt;
    hi_u16 msg_enqueue_fail_cnt;
    hi_u16 pad;
} hi_stat_diag;

HI_END_HEADER
#endif  /* __DFX_SYS_H__ */
