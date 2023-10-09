/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: APP common API.
 * Author: wangjian
 * Create: 2019-4-3
 */
#ifndef __SPI_H__
#define __SPI_H__

#include <hi_types_base.h>
#include <hi3861_platform_base.h>
#include <hi_spi.h>
#include <hi_time.h>
#include <hi_stdlib.h>
#include <hi_isr.h>
#include <hi_event.h>
#include <hi_sem.h>

/* if some print is needed :#define SPI_DEBUG */
#ifdef SPI_DEBUG
#define spi_printf(fmt...) do{ \
            printf("[DEBUG]"fmt); \
            printf("\n"); \
            } while (0)
#define spi_process_printf(fmt...) do{ \
                printf("[PROCESS]"fmt); \
                printf("\n"); \
                } while (0)
#else
#define spi_printf(fmt, ...)
#define spi_process_printf(fmt, ...)
#endif

#define SPI_NUM             2

#define REG_SPI_CR0         0x00        /* 控制寄存器0偏移地址 */
#define REG_SPI_CR1         0x04        /* 控制寄存器1偏移地址 */
#define REG_SPI_DR          0x08        /* 发送(接收)数据寄存器偏移地址 */
#define REG_SPI_SR          0x0c        /* 状态寄存器偏移地址 */
#define REG_SPI_CPSR        0x10        /* 时钟分频寄存器偏移地址 */
#define REG_SPI_IMSC        0x14        /* 中断屏蔽寄存器偏移地址 */
#define REG_SPI_RIS         0x18        /* 原始中断状态寄存器偏移地址 */
#define REG_SPI_MIS         0x1c        /* 屏蔽后中断状态寄存器偏移地址 */
#define REG_SPI_CR          0x20        /* 中断清除寄存器偏移地址 */
#define REG_SPI_DMACR       0x24        /* DMA功能寄存器偏移地址 */
#define REG_SPI_TXFIFOCR    0x28        /* 发送FIFO控制寄存器偏移地址 */
#define REG_SPI_RXFIFOCR    0x2c        /* 接收FIFO控制寄存器偏移地址 */

#define MASK_SPI_SR_TFE    (1<<0)       /* TX FIFO是否已空 0:未空 1:已空 */
#define MASK_SPI_SR_TNF    (1<<1)       /* TX FIFO是否未满 0:已满 1:未满 */
#define MASK_SPI_SR_RNE    (1<<2)       /* RX FIFO未空标志 0:已空 1:未空 */
#define MASK_SPI_SR_RFF    (1<<3)       /* RX FIFO已满标志 0:未满 1:已满 */
#define MASK_SPI_SR_BSY    (1<<4)       /* SPI忙标志 0:空闲 1:忙 */

#define SPI_CR0_ST_BIT_DSS  0   /* 起始bit:数据位宽 */
#define SPI_CR0_ST_BIT_FRF  4   /* 起始bit:帧格式 */
#define SPI_CR0_ST_BIT_SPO  6   /* 起始bit:极性 */
#define SPI_CR0_ST_BIT_SPH  7   /* 起始bit:相位 */
#define SPI_CR0_ST_BIT_SCR  8   /* 起始bit:串行时钟率 */

#define SPI_CR0_BIT_WIDTH_DSS   4       /* bit宽:数据位宽 */
#define SPI_CR0_BIT_WIDTH_FRF   2       /* bit宽:帧格式 */
#define SPI_CR0_BIT_WIDTH_SPO   1       /* bit宽:极性 */
#define SPI_CR0_BIT_WIDTH_SPH   1       /* bit宽:相位 */
#define SPI_CR0_BIT_WIDTH_SCR   8       /* bit宽:串行时钟率 */

#define SPI_CR1_ST_BIT_LBM      0       /* 起始bit:回环模式 */
#define SPI_CR1_ST_BIT_SSE      1       /* 起始bit:SPI使能 */
#define SPI_CR1_ST_BIT_MS       2       /* 起始bit:MASTER SLAVE */
#define SPI_CR1_ST_BIT_BIGEND   4       /* 起始bit: 大小端 */
#define SPI_CR1_ST_BIT_WAITVAL  8       /* 起始bit: microwire写和读之间等待拍数 */
#define SPI_CR1_ST_BIT_WAITEN   15      /* 起始bit:microwire写和读之间等待拍数使能 */

#define SPI_CR1_BIT_WIDTH_LBM       1   /* bit宽:回环模式 */
#define SPI_CR1_BIT_WIDTH_SSE       1   /* bit宽:SPI使能 */
#define SPI_CR1_BIT_WIDTH_MS        1   /* bit宽:MASTER SLAVE */
#define SPI_CR1_BIT_WIDTH_BIGEND    1   /* bit宽: 大小端 */
#define SPI_CR1_BIT_WIDTH_WAITVAL   7   /* bit宽:microwire写和读之间等待拍数 */
#define SPI_CR1_BIT_WIDTH_WAITEN    1   /* bit宽:microwire写和读之间等待拍数使能 */

#define SPI_INT_BIT_TX_FIFO_WATER_LINE (1<<3)   /* 发送fifo 水线中断 */
#define SPI_INT_BIT_RX_FIFO_WATER_LINE (1<<2)   /* 接收fifo 水线中断 */
#define SPI_INT_BIT_RX_FIFO_TIME_OUT   (1<<1)   /* 接收fifo 超时中断 */
#define SPI_INT_BIT_RX_FIFO_OVER_FLOW  (1<<0)   /* 接收fifo 溢出中断 */

#define SPI_INT_BIT_RTIC (1<<1)   /* 清除超时中断 */
#define SPI_INT_BIT_RORIC (1<<0)  /* 清除接收溢出中断 */

#define SPI_TX_DMAE (1<<1)  /* 使能DMA的发送FIFO */
#define SPI_RX_DMAE (1<<0)  /* 使能DMA的接收FIFO */

#define SPI_FIFO_LINE_OFFSET    3
#define SPI_FIFO_MAX_VAL        7     /*  接收/发送fifo最大长度 */
#define SPI_FIFO_LINE_MASK      0x7

#define SPI_UNUSE_DATA      0xFFFF      /* 半双工模式下无效数据对应电平 */

#define spi_get_transfer_size(burst) (((burst) == DMA_BURST_MSIZE_1) ? 1 : (1 << ((burst) + 1)))

#define MEM_TO_SPI 1
#define SPI_TO_MEM 2

#define SCR_MAX                 255
#define SCR_MIN                 0
#define CPSDVSR_MAX             254
#define CPSDVSR_MIN             4

#define SPI0_FIFO_LEN           256
#define SPI1_FIFO_LEN           64
#define SPI0_FIFO_THRESHOLD     128
#define SPI1_FIFO_THRESHOLD     32

#define SPI_HOST_TIMEOUT_US         1000000
#define SPI_HOST_TIMEOUT_MS         1000

#define SPI_SLAVE_TIMEOUT_US        10000000

#define SPI0_TX_FIFO_WATER_LINE     6
#define SPI0_RX_FIFO_WATER_LINE     6
#define SPI1_TX_FIFO_WATER_LINE     4
#define SPI1_RX_FIFO_WATER_LINE     3

#define SPI0_TX_FIFO_DMA_WLINE_64  7
#define SPI0_RX_FIFO_DMA_WLINE_128 6
#define SPI1_TX_FIFO_DMA_WLINE_16  4
#define SPI1_RX_FIFO_DMA_WLINE_32  4

/* 40 or 24M */
#define SPI_DEFAULT_CLK             160000000
#define spi_max_speed(clk) ((clk) / ((SCR_MIN + 1) * CPSDVSR_MIN))
#define spi_min_speed(clk) ((clk) / ((SCR_MAX + 1) * CPSDVSR_MAX))

#define SPI_WRITE_FLAG   0x1             /* 发送数据 */
#define SPI_READ_FLAG    0x2             /* 接收数据 */


/* spi 复用寄存器 */
#define GPIO_00_SEL 0x604
#define GPIO_01_SEL 0x608
#define GPIO_02_SEL 0x60c
#define GPIO_03_SEL 0x610

#define GPIO_05_SEL 0x618
#define GPIO_06_SEL 0x61c
#define GPIO_07_SEL 0x620
#define GPIO_08_SEL 0x624

#define GPIO_09_SEL 0x628
#define GPIO_10_SEL 0x62c
#define GPIO_11_SEL 0x630
#define GPIO_12_SEL 0x634

/**
 * SPI EVENT定义
 */
#define HI_EVENT_BIT_RX_DATA          0x1 /* 接收数据同步EVENT */
#define HI_EVENT_BIT_TX_DATA          0x2 /* 发送数据同步EVENT */
#define HI_EVENT_BIT_RX_DATA_TIME_OUT 0x4 /* 接收数据同步EVENT */
#define HI_EVENT_BIT_RX_FIFO_OVER_FLOW 0x8 /* 接收数据同步EVENT */

#define HI_EVENT_BIT_DMA_RX_DATA          0x10 /* 接收数据同步EVENT */
#define HI_EVENT_BIT_DMA_RX_ERR_DATA      0x20 /* 发送数据同步EVENT */
#define HI_EVENT_BIT_DMA_TX_DATA          0x40 /* 发送数据同步EVENT */
#define HI_EVENT_BIT_DMA_TX_ERR_DATA      0x80 /* 发送数据同步EVENT */

typedef enum {
    SPI_OPT_SET_CFG = 0x1,         /* 是否对SPI进行设置 */
    SPI_OPT_ENABLE_SPI = 0x2,      /* 是否使能SPI */
    SPI_OPT_DISABLE_SPI = 0x4,     /* 是否关闭SPI */
    SPI_OPT_TASKED_SIGNAL = 0x8,   /* 传输完成信号 */
    SPI_OPT_SEND_FIX_DATA = 0x10,  /* 发送固定数据 */
    SPI_OPT_RCV_FIX_DATA = 0x20,   /* 接收固定数据 */
    SPI_OPT_WAIT_SIGNAL = 0x40,    /* 是否等待信号量互斥 */
    SPI_OPT_FREE_SIGNAL = 0x80,    /* 是否释放信号量 */
} spi_opt;

/**
 * @ingroup hct_spi
 *
 * 通信参数：主从设备。
 */
typedef enum {
    SPI_CFG_ROLE_MASTER, /* 主设备 */
    SPI_CFG_ROLE_SLAVE,  /* 从设备 */
} spi_cfg_role;

/**
 * @ingroup hct_spi
 *
 * 通信参数：SPI软件BUFFER类型。
 */
typedef enum {
    SPI_DATA_WIDTH_1BYTES = 1,  /* SPI通信位宽 HI_SPI_CFG_DATA_WIDTH_E_4BIT至HI_SPI_CFG_DATA_WIDTH_E_8BIT */
    SPI_DATA_WIDTH_2BYTES,      /* SPI通信位宽 HI_SPI_CFG_DATA_WIDTH_E_9BIT至HI_SPI_CFG_DATA_WIDTH_E_16BIT */
} spi_data_width;

/**
 * @ingroup hct_spi
 *
 * 通信参数：SPI0发送水线，字节表示一个发送单位，非固定8bit。
 */
typedef enum {
    HI_SPI0_TX_FIFO_WATER_LINE_1,   /**< 发送水线为1byte */
    HI_SPI0_TX_FIFO_WATER_LINE_4,   /**< 发送水线为4byte */
    HI_SPI0_TX_FIFO_WATER_LINE_8,   /**< 发送水线为8byte */
    HI_SPI0_TX_FIFO_WATER_LINE_16,  /**< 发送水线为16byte */
    HI_SPI0_TX_FIFO_WATER_LINE_32,  /**< 发送水线为32byte */
    HI_SPI0_TX_FIFO_WATER_LINE_64,  /**< 发送水线为64byte */
    HI_SPI0_TX_FIFO_WATER_LINE_128, /**< 发送水线为128byte */
    HI_SPI0_TX_FIFO_WATER_LINE_192, /**< 发送水线为192byte */
} hi_spi0_tx_fifo_water_line;

/**
 * @ingroup hct_spi
 *
 * 通信参数：SPI0接收水线，字节表示一个发送单位，非固定8bit。
 */
typedef enum {
    HI_SPI0_RX_FIFO_WATER_LINE_255, /**< 接收水线为255byte */
    HI_SPI0_RX_FIFO_WATER_LINE_252, /**< 接收水线为252byte */
    HI_SPI0_RX_FIFO_WATER_LINE_248, /**< 接收水线为248byte */
    HI_SPI0_RX_FIFO_WATER_LINE_240, /**< 接收水线为240byte */
    HI_SPI0_RX_FIFO_WATER_LINE_224, /**< 接收水线为224byte */
    HI_SPI0_RX_FIFO_WATER_LINE_192, /**< 接收水线为192byte */
    HI_SPI0_RX_FIFO_WATER_LINE_128, /**< 接收水线为128byte */
    HI_SPI0_RX_FIFO_WATER_LINE_32,  /**< 接收水线为32byte */
} hi_spi0_rx_fifo_water_line;
/**
 * @ingroup hct_spi
 *
 * 通信参数：SPI1发送水线，字节表示一个发送单位，非固定8bit。
 */
typedef enum {
    HI_SPI1_TX_FIFO_WATER_LINE_1,  /**< 发送水线为1byte */
    HI_SPI1_TX_FIFO_WATER_LINE_4,  /**< 发送水线为4byte */
    HI_SPI1_TX_FIFO_WATER_LINE_8,  /**< 发送水线为8byte */
    HI_SPI1_TX_FIFO_WATER_LINE_16, /**< 发送水线为16byte */
    HI_SPI1_TX_FIFO_WATER_LINE_32, /**< 发送水线为32byte */
    HI_SPI1_TX_FIFO_WATER_LINE_48, /**< 发送水线为64byte */
    HI_SPI1_TX_FIFO_WATER_LINE_56, /**< 发送水线为56byte */
    HI_SPI1_TX_FIFO_WATER_LINE_64, /**< 发送水线为64byte */
} hi_spi1_tx_fifo_water_line;
/**
 * @ingroup hct_spi
 *
 * 通信参数：SPI1接收水线，字节表示一个发送单位，非固定8bit。
 */
typedef enum {
    HI_SPI1_RX_FIFO_WATER_LINE_65, /**< 接收水线为65byte */
    HI_SPI1_RX_FIFO_WATER_LINE_62, /**< 接收水线为62byte */
    HI_SPI1_RX_FIFO_WATER_LINE_48, /**< 接收水线为48byte */
    HI_SPI1_RX_FIFO_WATER_LINE_32, /**< 接收水线为32byte */
    HI_SPI1_RX_FIFO_WATER_LINE_16, /**< 接收水线为16byte */
    HI_SPI1_RX_FIFO_WATER_LINE_8,  /**< 接收水线为8byte */
    HI_SPI1_RX_FIFO_WATER_LINE_4,  /**< 接收水线为4byte */
    HI_SPI1_RX_FIFO_WATER_LINE_1,  /**< 接收水线为1byte */
} hi_spi1_rx_fifo_water_line;

typedef struct {
    hi_u16 cr0;                 /* SPI CR0寄存器设置  */
    hi_u16 cr1;                 /* SPI CR1寄存器设置  */
    hi_u16 cpsdvsr;             /* SPI CPSR寄存器设置  */
} spi_inner_cfg;

typedef struct {
    hi_u16 data_width:4;        /* 数据位宽，取值见hi_spi_cfg_data_width  */
    hi_u16 fram_mode:2;         /* 数据协议，取值见hi_spi_cfg_fram_mode  */
    hi_u16 cpol:1;              /* 极性，取值见hi_spi_cfg_clock_cpol  */
    hi_u16 cpha:1;              /* 相位，取值见hi_spi_cfg_clock_cpha  */
    hi_u16 scr:8;               /* 时钟率：决定SPI时钟因素之一  */
    hi_u16 loop_back:1;         /* 是否为回环模式  */
    hi_u16 reserver_1:1;        /* 保留,SPI使能位  */
    hi_u16 is_slave:1;          /* 主从模式  */
    hi_u16 reserver_2:1;
    hi_u16 endian:1;            /* 大小端，取值见hi_spi_cfg_endian  */
    hi_u16 reserver_3:11;
    hi_u16 cpsdvsr;             /* 时钟率：决定SPI时钟因素之一，必须是2～254之间的偶数  */
    hi_u16 rx_fifo_line;        /* 接收水线  */
    hi_u16 tx_fifo_line;        /* 发送水线  */
    hi_u16 rx_fifo_dma_line;        /* DMA burst水线  */
    hi_u16 tx_fifo_dma_line;        /* DMA burst水线  */
    hi_u16 pad;        /* 发送水线  */
} spi_hw_cfg;

typedef struct {
    hi_u32 time_out_ms;         /* 传输超时时间   */
    hi_u32 trans_opt;
} spi_trans_attr;

typedef struct {
    hi_pvoid buf;               /* 数据指针  */
    volatile hi_u32 cur_pos;    /* 当前读位置  */
    volatile hi_u32 cur_cnt;    /* 当次读数个数  */
} spi_buf;

typedef struct {
    hi_u32 reg_base;            /* SPI寄存器基地址  */
    hi_u32 irq_num;             /* SPI中断号  */
    hi_u32 sem_id;              /* SPI互斥信号量  */
    hi_u32 event_id;            /* SPI读写同步事件ID  */
    hi_bool use_dma;            /* 使用DMA传输  */
    hi_bool use_irq;            /* 使用中断传输  */
    volatile hi_bool transferring;  /* 是否传输中  */
    volatile hi_bool disable_later; /* 是否延迟关闭  */
    hi_spi_usr_func prepare_func;   /* 数据通信前用户准备函数  */
    hi_spi_usr_func restore_func;   /* 数据通信后用户恢复函数  */
    spi_hw_cfg spi_cfg;
    hi_u32 single_len;          /* 单次传输长度, 小于FIFO深度（单位：DATA_WIDTH，BYTE对齐）  */
    hi_u32 trans_len;           /* 单次传输长度, 小于FIFO深度（单位：DATA_WIDTH，BYTE对齐）  */
    spi_buf tx_buf;
    spi_buf rx_buf;
} spi_ctrl;

HI_EXTERN spi_ctrl *g_spi_ctrl[SPI_NUM];

hi_void spi_isr(spi_ctrl *spi_dev_ctrl);
hi_void spi_isr_enable(hi_u32 reg_base, hi_u16 enable_bits);
hi_void spi_isr_disable(hi_u32 reg_base, hi_u16 disable_bits);
hi_u32 spi_trans_prepare(spi_ctrl *spi_hw_ctrl, spi_trans_attr *trans_attr);
hi_void spi_trans_restore(spi_ctrl *spi_hw_ctrl, const spi_trans_attr *trans_attr);
hi_u32 spi_transfer_8bits_block(const spi_ctrl *spi_hw_ctrl, hi_u32 options);
hi_u32 spi_transfer_16bits_block(const spi_ctrl *spi_hw_ctrl, hi_u32 options);
hi_void spi_set_fifo_line(const spi_ctrl *spi_hw_ctrl);
hi_u32 spi_config(const spi_ctrl *spi_hw_ctrl);
hi_void spi_reset(const spi_ctrl *spi_hw_ctrl);
hi_void spi_disable(spi_ctrl *ctrl);
hi_void spi_flush_fifo(hi_u32 reg_base);

hi_void spi_isr_clear_cr(hi_u32 reg_base, hi_u16 clear_bit);
#ifdef CONFIG_SPI_DMA_SUPPORT
hi_u32 spi_hd_dma_read_fifo(spi_ctrl *spi_dev_ctrl, hi_u32 timeout_ms);
hi_u32 spi_hd_dma_write_fifo(spi_ctrl *spi_dev_ctrl, hi_u32 timeout_ms);
hi_void spi_set_dma_fifo_line(const spi_ctrl *spi_hw_ctrl);
hi_void spi_dma_enable(hi_u32 reg_base, hi_u16 enable_bits);
hi_void spi_dma_disable(hi_u32 reg_base, hi_u16 disable_bits);
#endif

#endif
