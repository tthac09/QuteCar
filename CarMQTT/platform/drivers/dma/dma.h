/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: APP common API.
 * Author: wangjian
 * Create: 2019-3-14
 */

/**
* @file dma.h
*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.  \n
*
* Description: DMA interfaces. \n
*/

/** @defgroup hct_dma
 *  @ingroup drivers
 */

#ifndef _DMA_H_
#define _DMA_H_

#include <hi_types_base.h>
#include <hi_dma.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef DMA_DEBUG
#define dma_print(fmt...)       \
    do {                        \
        printf(fmt);            \
        printf("\r\n"); \
    } while (0)
#else
#define dma_print(fmt...)
#endif

/* DMA 寄存器地址 */
#define DMA_BASE_ADDR          0x40200000
#define DMA_INT_STAT           (DMA_BASE_ADDR + 0x000)
#define DMA_INT_TC_STAT        (DMA_BASE_ADDR + 0x004)
#define DMA_INT_TC_CLR         (DMA_BASE_ADDR + 0x008)
#define DMA_INT_ERR_STAT       (DMA_BASE_ADDR + 0x00C)
#define DMA_INT_ERR_CLR        (DMA_BASE_ADDR + 0x010)
#define DMA_RAW_INT_TC_STATUS  (DMA_BASE_ADDR + 0x014)
#define DMA_RAW_INT_ERR_STATUS (DMA_BASE_ADDR + 0x018)
#define DMA_ENBLD_CHNS         (DMA_BASE_ADDR + 0x01C)
#define DMA_SOFT_BREQ          (DMA_BASE_ADDR + 0x020)
#define DMA_SOFT_SREQ          (DMA_BASE_ADDR + 0x024)
#define DMA_SOFT_LBREQ         (DMA_BASE_ADDR + 0x028)
#define DMA_SOFT_LSREQ         (DMA_BASE_ADDR + 0x02C)
#define DMA_CFG_REG            (DMA_BASE_ADDR + 0x030)
#define DMA_SYNC               (DMA_BASE_ADDR + 0x034)
/* Source Address Register for Channel x */
#define dma_sar(x) (DMA_BASE_ADDR + 0x100 + (x)*0x020)
/* Destination Address Register for Channel x */
#define dma_dar(x) (DMA_BASE_ADDR + 0x104 + (x)*0x020)
/* Linked List Pointer Register for Channel x */
#define dma_lli(x) (DMA_BASE_ADDR + 0x108 + (x)*0x020)
/* Control Register for Channel x */
#define dma_ctl(x) (DMA_BASE_ADDR + 0x10C + (x)*0x020)
/* Configuration Register for Channel x */
#define dma_cfg(x) (DMA_BASE_ADDR + 0x110 + (x)*0x020)

#define DMA_MASK_INT   0
#define DMA_UNMASK_INT 1

#define DMA_CHANNEL_0 0x01
#define DMA_CHANNEL_1 0x02
#define DMA_CHANNEL_2 0x04
#define DMA_CHANNEL_3 0x08

#define DMA_CHANNEL_NUM_0 0
#define DMA_CHANNEL_NUM_1 1
#define DMA_CHANNEL_NUM_2 2
#define DMA_CHANNEL_NUM_3 3

#define DMA_CHANNEL_NUM 4

#define DMA_DISABLE 0
#define DMA_ENABLE  1

/* 地址递增设置位，存储器设备的地址传输时需要递增，外设不需要 */
#define DMA_TR_ADDR_INCREMENT 1
#define DMA_TR_ADDR_NOCHANGE  0

#define DMA_WORD_WIDTH 4

/* transfer_size 寄存器为12位，最大支持长度为4095 */
#define DMA_TS_MAX 4095
/* 为保证4字节对齐，每个BLOCK长度设为4092 */
#define DMA_TS_BLOCK 4092

#define DMA_TRANSFER_COMPLETE   0
#define DMA_TRANSFER_INCOMPLETE 1
#define DMA_TRANSFER_ERR        2

#define DMA_TIMEOUT_US 50000

#define dma_pkt_b_to_dma_addr(_virt_addr) ((hi_u32)(_virt_addr) + PKT_B_OFFSET)
#define dma_pkt_h_to_dma_addr(_virt_addr) ((hi_u32)(_virt_addr) + PKT_H_OFFSET)

#define DCACHE_ENABLE 1
#define DCACHE_EN_REG 0x7C1 /* DCACHE使能寄存器 */

/**
 * @ingroup hct_dma
 *
 * DMA transfer mode. CNcomment:DMA传输模式。CNend
 */
typedef enum {
    DMA_MEM_TO_MEM = 0,
    DMA_MEM_TO_PHL,
    DMA_PHL_TO_MEM,
    DMA_PHL_TO_PHL,
} hi_dma_tr_type;

/**
 * @ingroup hct_dma
 *
 * Peripheral ID that supports DMA transfer. CNcomment:支持DMA传输的外设ID。CNend
 */
typedef enum {
    UART0_RX = 0,
    UART0_TX,
    UART1_RX,
    UART1_TX,
    UART2_RX,
    UART2_TX,
    SPI0_RX,
    SPI0_TX,
    SPI1_RX,
    SPI1_TX,
    I2S0_RX,
    I2S0_TX,
    PHL_MAX,
} hi_dma_phl;

/**
 * @ingroup hct_dma
 *
 * One DMA burst transmission length. CNcomment:一次DMA burst传输长度。CNend
 */
typedef enum {
    DMA_BURST_MSIZE_1 = 0,
    DMA_BURST_MSIZE_4,
    DMA_BURST_MSIZE_8,
    DMA_BURST_MSIZE_16,
    DMA_BURST_MSIZE_32,
    DMA_BURST_MSIZE_64,
    DMA_BURST_MSIZE_128,
    DMA_BURST_MSIZE_256,
} hi_dma_burst_size;

/* 通道ID寄存器结构 */
typedef union dma_ch_sel {
    struct {
        hi_u32 channel_0 : 1;
        hi_u32 channel_1 : 1;
        hi_u32 channel_2 : 1;
        hi_u32 channel_3 : 1;
        hi_u32 reserved : 28;
    } ch_bit;
    hi_u32 ch_set_u32;
} dma_ch_sel;

/* DMA CFG寄存器结构，目前只支持master1，0:little endian模式 1:big endian模式 */
typedef union dma_init_cfg {
    struct {
        hi_u32 dma_en : 1;
        hi_u32 master1_endianness : 1;
        hi_u32 master2_endianness : 1;
        hi_u32 reserved : 29;
    } dma_cfg_bit;
    hi_u32 dma_cfg_u32;
} dma_init_cfg;

/* DMA CHANNEL CFG寄存器结构 */
typedef union dma_ch_cfg {
    struct {
        hi_u32 en : 1;             /* 通道使能 */
        hi_u32 src_peripheral : 4; /* 源外设ID */
        hi_u32 reserved0 : 1;
        hi_u32 dst_peripheral : 4; /* 目标外设ID */
        hi_u32 reserved1 : 1;
        hi_u32 flow_cntrl : 3;   /* 传输模式，见hi_dma_tr_type */
        hi_u32 err_int_mask : 1; /* err 中断屏蔽位 */
        hi_u32 tc_int_mask : 1;  /* tc 中断屏蔽位 */
        hi_u32 lock : 1;
        hi_u32 active : 1;
        hi_u32 halt : 1;
        hi_u32 reserved2 : 13;
    } ch_cfg_bit;
    hi_u32 ch_cfg_u32;
} dma_ch_cfg;

typedef union dma_llp {
    struct {
        hi_u32 lms : 1; /* 载入下一个链表结点的Master,目前只支持master1:0 */
        hi_u32 reserved : 1;
        hi_u32 loc : 30; /* 下一个LLI在内存中的起始地址，32位对齐 */
    } llp_bit;
    hi_u32 llp_u32;
} dma_llp;

typedef struct dma_channel_para dma_channel_para_t;

/**
 * @ingroup dma
 *
 * The channel control register structure of DMA. CNcomment:DMA 通道控制寄存器结构。CNend
 */
typedef struct dma_ch_ctl_t dma_ch_ctl;
typedef union dma_ch_ctl {
    struct {
        hi_u32 transfer_size : 12; /* 传输长度，最大4095，以src_width为单位 */
        hi_u32 src_burst_size : 3; /* 源设备一次burst传输的长度 */
        hi_u32 dst_burst_size : 3; /* 目标设备一次burst传输的长度 */
        hi_u32 src_width : 3;      /* 源设备传输位宽,参考 hi_dma_data_width */
        hi_u32 dst_width : 3;      /* 目标设备传输位宽,参考 hi_dma_data_width */
        hi_u32 master_src_sel : 1; /* 目前只支持master1:0 */
        hi_u32 master_dst_sel : 1; /* 目前只支持master1:0 */
        hi_u32 src_inc : 1;        /* 源地址是否自增 */
        hi_u32 dst_inc : 1;        /* 目标地址是否自增 */
        hi_u32 prot : 3;
        hi_u32 lli_tc_int_en : 1; /* 当前LLI结点传输完是否触发中断 */
    } ch_ctl_bit;
    hi_u32 ch_ctl_u32;
} dma_ch_ctl_t;

/**
 * @ingroup dma
 *
 * The link structure of DMA link list item. CNcomment:DMA link list item链表结构。CNend
 */
typedef struct dma_lli_stru dma_lli_stru_t;
typedef struct dma_lli_stru {
    hi_u32 saddr;             /* LLI结点源地址 */
    hi_u32 daddr;             /* LLI结点目标地址 */
    dma_lli_stru_t *llp_next; /* 下个LLI结点 */
    dma_ch_ctl_t st_ctl;      /* LLI对应的通道控制寄存器 */
} dma_lli_stru_t;

typedef struct dma_channel_para {
    dma_llp llp;                  /* 通道传输LLI链表第二个结点地址 */
    dma_ch_ctl_t ch_ctl;            /* 通道控制寄存器 */
    dma_ch_cfg ch_cfg;            /* 通道设置寄存器 */
    hi_void (*cb)(hi_u32);     /* 指向用户定义的回调函数 */
    volatile hi_u32 is_transfering; /* 0表示数据传送完成,1表示数据正在传送，2表示传输错误 */
    dma_lli_stru_t *ch_lli;         /* 通道的link list链表头 */
} dma_channel_para;

typedef struct dma_data {
    hi_bool is_inited;
    volatile hi_u32 ch_mask;
    dma_channel_para ch_data[DMA_CHANNEL_NUM]; /* 4个通道数据数组 */
} dma_data;

/**
 * @ingroup hct_dma
 *
 * The general setting structure of the user's incoming DMA. It is used for the transmission participated by IO.
 CNcomment:用户传入DMA的通用设置结构，主要用于外设参与的传输。CNend
 */
typedef struct hi_dma_para {
    hi_u32 tr_type;        /* DMA传输模式，取值见hi_dma_tr_type */
    hi_u32 src_phl;        /* 源外设ID，取值见hi_dma_phl，如源设备是存储器设为0 */
    hi_u32 dst_phl;        /* 目标外设ID，取值见hi_dma_phl 如目的设备是存储器设为0 */
    uintptr_t src_addr;    /* 源地址，源地址必须与源设备传输宽度对齐 */
    uintptr_t dst_addr;    /* 目标地址，目的地址必须与目的设备的传输宽度对齐 */
    hi_u32 src_burst_size; /* 源设备一次burst传输的长度，取值见hi_dma_burst_size_e, 与目的设备保持一致 */
    hi_u32 dst_burst_size; /* 目标设备一次burst传输的长度，取值见hi_dma_burst_size_e, 与源设备保持一致 */
    hi_u32 src_width;      /* 源设备传输位宽,取值见hi_dma_data_width_e，与目的外设一致 */
    hi_u32 dst_width;      /* 目标设备传输位宽,取值见hi_dma_data_width_e，与源外设一致 */
    hi_u32 transfer_size;  /* 传输长度，以源设备传输位宽为单位，外设参与传输时需是burst_size整数倍 */
    hi_void (*cb)(hi_u32);  /* 传输结束回调，参数为传输完成或传输错误 #DMA_INT_XXX */
} hi_dma_para;

/**
* @ingroup  hct_dma
* @brief  Start dma transmission. CNcomment:启动dma传输。CNend
*
* @par 描述:
*           Start dma transmission and channel will be released after success or failure.
CNcomment:启动dma传输，成功或失败后会释放通道。CNend
*
* @attention
* @param  dma_para         [IN/OUT] type #hi_dma_user_para_sSetting incoming dma transfer parameter.
CNcomment:传入DMA传输参数设置。CNend
* @param  block            [IN]     type #hi_bool，Whether to block for waiting dma tranmission completeness
CNcomment:是否阻塞等待DMA传输完成。CNend
*
* @retval #HI_ERR_SUCCESS  Success.
* @retval #Other           Failure. For details, see hi_errno.h.
* @par 依赖:
*            @li hi_dma.h：   DMA driver implementation interface.  CNcomment:DMA驱动实现接口。CNend
* @see  None
* @since Hi3861_V100R001C00
 */
hi_u32 hi_dma_transfer(const hi_dma_para *dma_para, hi_bool block);
/**
* @ingroup  hct_dma
* @brief  Start dma transmission. CNcomment:启动dma传输。CNend
*
* @par 描述:
*           Start dma transmission and channel will be released after success or failure.
CNcomment:启动dma传输，成功或失败后会释放通道。CNend
*
* @attention
* @param  dma_para         [IN/OUT] type #hi_dma_user_para_sSetting incoming dma transfer parameter.
CNcomment:传入DMA传输参数设置。CNend
* @param  dma_ch            [IN]     type #hi_bool，return dma channel number
CNcomment:返回申请的通道号。CNend
*
* @retval #HI_ERR_SUCCESS  Success.
* @retval #Other           Failure. For details, see hi_errno.h.
* @par 依赖:
*            @li hi_dma.h：   DMA driver implementation interface.  CNcomment:DMA驱动实现接口。CNend
* @see  None
* @since Hi3861_V100R001C00
 */
hi_u32 dma_hw_request_transfer(const hi_dma_para *dma_para, hi_u32 *dma_ch);
void dma_write_data(hi_u32 ch_num, const hi_dma_para *dma_para);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
