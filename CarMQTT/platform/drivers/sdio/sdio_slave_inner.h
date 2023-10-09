/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: sdio slave mode inner APIs.
 * Author: hisilicon
 * Create: 2019-01-17
 */

/**
 * @defgroup iot_sdio SDIO Slave
 * @ingroup drivers
 */

#ifndef __SDIO_SLAVE_INNER_H__
#define __SDIO_SLAVE_INNER_H__

#include <hi_types_base.h>
#include <hi_sdio_slave.h>

/* The max scatter buffers when host to device */
#define HISDIO_HOST2DEV_SCATT_MAX               64
#define HISDIO_HOST2DEV_SCATT_SIZE              64

/* The max scatter buffers when device to host */
#define HISDIO_DEV2HOST_SCATT_MAX               64
#define HISDIO_DEV2HOST_SCATT_SIZE              64

/* 64B used to store the scatt info,1B means 1 pkt. */
#define HISDIO_H2D_SCATT_BUFFLEN_ALIGN_BITS     3
#define HISDIO_H2D_SCATT_BUFFLEN_ALIGN          8

#define HISDIO_D2H_SCATT_BUFFLEN_ALIGN_BITS     5
#define HISDIO_D2H_SCATT_BUFFLEN_ALIGN          512

#define HSDIO_HOST2DEV_PKTS_MAX_LEN             1544

/**
 * @ingroup iot_sdio
 *
 * SDIO sleep stage.
 */
typedef enum {
    SLEEP_REQ_WAITING       = 0,
    SLEEP_ALLOW_SND         = 1,
    SLEEP_DISALLOW_SND      = 2,
} hi_sdio_host_sleep_stage_e;

/**
 * @ingroup iot_sdio
 *
 * SDIO wakeup stage.
 */
typedef enum {
    WAKEUP_HOST_INIT        = 0,
    WAKEUP_REQ_SND          = 1,
    WAKEUP_RSP_RCV          = 2,
} hi_sdio_host_wakeupstage_e;

/**
 * @ingroup iot_sdio
 *
 * SDIO works status.
 */
typedef enum {
    SDIO_CHAN_ERR    = 0x0,
    SDIO_CHAN_RESET,
    SDIO_CHAN_INIT,
    SDIO_CHAN_SLEEP,
    SDIO_CHAN_WAKE,
    SDIO_CHAN_WORK,
    /* Status Number */
    SDIO_CHAN_BUTT
} hi_sdio_chanstatus;

/**
 * @ingroup iot_sdio
 *
 * SDIO status info.
 */
typedef struct {
    hi_u8     allow_sleep;
    hi_u8     tx_status;       /* point to g_chan_tx_status variable address */
    hi_u8     sleep_status;    /* point to g_sleep_stage variable address */
    hi_sdio_chanstatus  work_status; /* point to g_chan_work_status variable address */
} hi_sdio_status_info;

/**
 * @ingroup iot_sdio
 *
 * SDIO status structure.
 */
typedef struct {
    hi_u16          rd_arg_invalid_cnt;
    hi_u16          wr_arg_invlaid_cnt;
    hi_u16          unsupport_int_cnt;
    hi_u16          mem_int_cnt;
    hi_u16          fn1_wr_over;
    hi_u16          fn1_rd_over;
    hi_u16          fn1_rd_error;
    hi_u16          fn1_rd_start;
    hi_u16          fn1_wr_start;
    hi_u16          fn1_rst;
    hi_u16          fn1_msg_rdy;
    hi_u16          fn1_ack_to_arm_int_cnt;
    hi_u16          fn1_adma_end_int;
    hi_u16          fn1_suspend;
    hi_u16          fn1_resume;
    hi_u16          fn1_adma_int;
    hi_u16          fn1_adma_err;
    hi_u16          fn1_en_int;
    hi_u16          fn1_msg_isr;                 /**< device收到msg中断次数 */
    hi_u16          soft_reset_cnt;
} hi_sdio_status;

/**
 * @ingroup iot_sdio
 *
 * SDIO transfer channel structure.
 */
typedef struct {
    hi_u32                 send_data_len;
    hi_u16                 last_msg;
    hi_u16                 panic_forced_timeout;
    hi_u16                 chan_msg_cnt[D2H_MSG_COUNT];
} hi_sdio_chan_info;

/**
 * @ingroup iot_sdio
 *
 * SDIO message structure.
 */
typedef struct {
    hi_u32        pending_msg;
    hi_u32        sending_msg;
} hi_sdio_message;

/**
 * @ingroup iot_sdio
 *
 * SDIO infomation structure.
 */
typedef struct {
    hi_u8                volt_switch_flag;        /**< Sdio voltage conversion flag.
                                                          CNcomment:SDIO电压转换标志CNend */
    hi_u8                host_to_device_msg_flag; /**< MSG_FLAG_ON indicates that the DEVICE receives the allowed
                                                     sleep msg. CNcomment:MSG_FLAG_ON表示DEVICE收到允许睡眠msg.CNend */
    hi_u16               reinit_times;
    hi_u16               gpio_int_times;
    hi_u16               pad;
    hi_sdio_status       sdio_status;             /**< Sdio statistics.CNcomment:SDIO统计CNend */
    hi_sdio_chan_info    chan_info;
    hi_sdio_message      chan_msg_stat;
} hi_sdio_info;

/**
* @ingroup  iot_sdio
* @brief  sdio data init function
*
* @par 描述:
*         sdio data initialization function.CNcomment:sdio 数据初始化函数。CNend
*
* @attention None
* @param  None
*
* @retval  None
*
* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_void hi_sdio_data_init(hi_void);

/**
* @ingroup  iot_sdio
* @brief  sdio memory init function
*
* @par 描述:
*         sdio memory initialization function.CNcomment:sdio 内存初始化函数。CNend
*
* @attention None
* @param  None
*
* @retval # None
*
* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_void hi_sdio_memory_init(hi_void);

/**
* @ingroup  iot_sdio
* @brief  sdio wakeup host function
*
* @par 描述:
*         sdio wakeup host function.CNcomment:sdio 唤醒Host 函数。CNend
*
* @attention None
* @param  None
*
* @retval #0          Success.

* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_sdio_wakeup_host(hi_void);

/**
* @ingroup  iot_sdio
* @brief  sdio proc memory malloc fail function
*
* @par 描述:
*         sdio proc memory malloc fail function.CNcomment:sdio Proc 内存申请失败函数。CNend
*
* @attention None
* @param  [IN] mem_type type #hi_u8, memory type. see enum _hcc_netbuf_queue_type_ CNend.
          CNcomment:内存类型，参考enum _hcc_netbuf_queue_type_ 类型CNend
* @param  [IN] resv_buf type #hi_void**, Memory address assigned to the rx_buf after a failure.
CNcomment:失败后赋值给rx_buf的内存地址CNend
* @param  [OUT] rx_buf  type #hi_void*, Indicates the returned reserved memory address.
CNcomment:返回的保留内存地址CNend
*
* @retval #0       Success.
* @retval #Other   Failure. For details, see hi_errno.h.

* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_sdio_procmem_alloc_fail(hi_u8 mem_type, hi_void **rx_buf, hi_void *resv_buf);

/**
* @ingroup  iot_sdio
* @brief  sdio flow ctronl function
*
* @par 描述:
*         sdio flow ctronl function.CNcomment:sdio 流控控制函数。CNend
*
* @attention。
* @param  [IN] enable       Indicates whether to enable flow control. The value true indicates that flow control is
*         enabled, and the value false indicates that flow control is disabled.CNcomment:使能流控位，true为打开流控，
false为关闭流控CNend
* @param  [IN] free_pkts    Current free_pkts value.CNcomment:当前free_pkts值。CNend
* @param  [IN] mem_level    Enables or disables the memory threshold for flow control.
CNcomment:打开或关闭流控的内存水线。CNend
* @param  [INOUT] ctl_flag  Current flow control status.CNcomment:当前流控状态。CNend
*
* @retval  None
*
* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_void hi_sdio_flow_ctrl(hi_bool enable, hi_u32 free_pkts, hi_u8 *ctl_flag, hi_u16 mem_level);

/**
* @ingroup  iot_sdio
* @brief  start to read data
*
* @par 描述:
*         start to read data.CNcomment:启动读数据CNend
*
* @attention。
* @param   [IN] read_bytes Number of bytes read.CNcomment:读取的字节数CNend
*
* @retval    None
*
* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_void hi_sdio_read_err_retry(hi_u32 read_bytes);

/**
* @ingroup  iot_sdio
* @brief  pad transfer data Len
*
* @par 描述:
*         Byte alignment for transmitted data.CNcomment:对传输数据进行字节对齐CNend
*
* @attention。
* @param   [IN] txdata_len      Number of transmitted bytes.CNcomment:传输的字节数CNend
*
* @retval  Number of bytes to be transmitted in the returned byte. If the value is greater than 512, the byte is 512
*          bytes. If the value is smaller than 512 bytes, the byte is aligned with 4 bytes.CNcomment:返回字节对其后
的传输字节数，大于512按512字节对其，小于则按4字节对齐CNend
*
* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_sdio_padding_xfercnt(hi_u32 txdata_len);

/**
* @ingroup  iot_sdio
* @brief  send device panic message
*
* @par 描述:
*         send device panic message.CNcomment:发送Device 崩溃消息，重启device.CNend
*
* @attention。
* @param    None
*
* @retval    None
*
* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_void hi_sdio_send_panic_msg(hi_void);

/**
* @ingroup  iot_sdio
* @brief  get current sdio status.
*
* @par 描述:
*         get current sdio status.CNcomment:获取当前SDIO通道状态信息CNend
*
* @attention。
* @param    [IN]  hi_sdio_status_info*   Storage status information BUFFER pointer.
CNcomment:存储状态信息BUFFER指针CNend
*
* @retval    #HI_ERR_SUCCESS    Success
* @retval    #HI_ERR_FAILURE    Failure
*
* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_sdio_get_status(hi_sdio_status_info *satus_info);

/**
* @ingroup  iot_sdio
* @brief  set current sdio status.
*
* @par 描述:
*         set current sdio status.CNcomment:设置当前SDIO通道状态信息CNend
*
* @attention。
* @param    [IN]  const hi_sdio_status_info*   Pointing to the storage status information buffer.
CNcomment:指向存储状态信息buffer.Nend
*
* @retval    #HI_ERR_SUCCESS    Success
* @retval    #HI_ERR_FAILURE    Failure
*
* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_sdio_set_status(const hi_sdio_status_info *satus_info);

/**
* @ingroup  iot_sdio
* @brief  Check sdio whether is sending/pending message.
*
* @par 描述:
*         Check sdio whether is sending/pending message.CNcomment:当前SDIO通道是否正在工作中CNend
*
* @attention。
* @param    None
*
* @retval    #true    If the sdio is sending or receiving a message, true is returned.
CNcomment:如果sdio正在发送或者接收消息，则返回true.CNend
* @retval    #false   If the sdio does not process the message, the value false is returned.
CNcomment:如果sdio不在处理消息，返回false.CNend
*
* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_bool hi_sdio_is_busy(hi_void);

/**
* @ingroup  iot_sdio
* @brief  get sdio statistics information.
*
* @par 描述:
*         Obtains currect sdio status.CNcomment:当前SDIO状态信息CNend
*
* @attention。
* @param     None.
*
* @retval    hi_sdio_info_s*  pointer to sdio information buffer.
*
* @par 依赖:
*           @li hi_sdio_slave.h：Describe sdio slave APIs.CNcomment:该接口声明所在的头文件。CNend
* @see  None。
* @since Hi3861_V100R001C00
*/
hi_sdio_info *hi_sdio_get_info(hi_void);

hi_u32 sdio_process_msg(hi_u32 send_msg, hi_u32 clear_msg);

/* the following parameters are used in sdio flash */
extern volatile hi_sdio_info g_sdio_info;
extern hi_sdio_extendfunc g_extend_func_info;

#endif /* end of sdio_slave_inner.h */
