/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: Uart authentication head file.
 * Author: Hisilicon
 * Create: 2012-12-22
 */

#ifndef __UART_AUTH_H__
#define __UART_AUTH_H__

#include <secure.h>
#include "hi_boot_rom.h"
#include <hi_types.h>

#define UART_PACKET_START_FLAG 0xDEADBEEF

#define UART_PACKET_PAYLOAD_MAX 1024

HI_EXTERN uart_param_stru g_uart_param;
HI_EXTERN hi_u32 g_uart_int_type;

enum {
    UART_TYPE_ROMBOOT_HANDSHAKE = 0xF0,
    UART_TYPE_ACK = 0xE1,
    UART_TYPE_FILE_START = 0xD2,
    UART_TYPE_FILE_END = 0xC3,
    UART_TYPE_CMD = 0xB4,
    UART_TYPE_DATA = 0xA5,
    UART_TYPE_FLASHBOOT_HANDSHAKE = 0x0F,
};

typedef struct {
    hi_u32 start_flag;  /* 起始标识: 0xDEADBEEF */
    hi_u16 packet_size; /* 报文长度：真实传输数据长度，要求不超过1024 */
    hi_u8 type;         /* 报文类型 */
    hi_u8 pad;
} packet_data_head;

typedef struct {
    packet_data_head head;
    hi_u8 payload[UART_PACKET_PAYLOAD_MAX]; /**< 报文数据 */
    hi_u16 check_sum;   /* 校验和 */
    hi_u8 rev[2];       /* 2: rev */
} packet_data_info;

/* UART AUTH context */
typedef struct {
    hi_u8 status;
    hi_u8 pad;
    hi_u16 received;
    packet_data_info packet;
} uart_ctx;

hi_u32 uart_process(hi_uart uart, hi_u32 interrupt_timeout_ms);

#endif /* __UART_AUTH_H__ */
