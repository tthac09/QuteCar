/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: header file of efuse driver.
 * Author: wangjun
 * Create: 2019-05-08
 */

#ifndef __EFUSE_DRV_H__
#define __EFUSE_DRV_H__
#include <hi_boot_rom.h>

typedef struct {
    hi_u16 id_start_bit;    /* 起始 bit位 */
    hi_u16 id_size;         /* 以bit为单位 */
    hi_u8 attr;             /* 0x0:不可读写，0x1:只读，0x2:只写，0x3:可读可写 */
} hi_efuse_stru;

#define EFUSE_PGM_EN   (HI_EFUSE_REG_BASE + 0x0)
#define EFUSE_PGM_ADDR (HI_EFUSE_REG_BASE + 0x4)
#define EFUSE_RD_EN    (HI_EFUSE_REG_BASE + 0x8)
#define EFUSE_RD_ADDR  (HI_EFUSE_REG_BASE + 0xc)
#define EFUSE_STATUS   (HI_EFUSE_REG_BASE + 0x10)
#define EFUSE_RDATA    (HI_EFUSE_REG_BASE + 0x14)

#define EFUSE_WRITE_READY_STATUS (1 << 0) /* 写完成状态，1表示完成 */
#define EFUSE_READ_READY_STATUS  (1 << 1) /* 读完成状态，1表示完成 */
#define EFUSE_STATUS_MASK        (0x7 << 2)
#define EFUSE_PO_STATUS_READY    (0x1 << 2) /* 上电后的读操作是否结束，1表示完成 */
#define EFUSE_STATUS_READY       (0x1 << 4) /* 忙闲状态，0表示空闲 */

#define EFUSE_CTRL_ST   (0x1 << 5)
#define EFUSE_EN_SWITCH (1 << 0)
#define EFUSE_EN_OK     0

#define EFUSE_STATUS_RD    (1 << 1)
#define EFUSE_8_BIT        8
#define EFUSE_KEY_LOCK_BIT 2

#define EFUSE_TIMEOUT_DEFAULT 1000000 /* 1秒 */
#define EFUSE_TIMECNT_TICK    10      /* 必须可以被EFUSE_TIMEOUT_DEFAULT整除 */

#define EFUSE_PGM_ADDR_SIZE          2048 /* 单位为bit */
#define EFUSE_USER_RESEVED_START_BIT 1884 /* 用户保留数据区起始bit */
#define EFUSE_USER_RESEVED_END_BIT   2011 /* 用户保留数据区结束bit */
#define EFUSE_LOCK_START_BITS        2012 /* 第一加锁区起始bit */
#define EFUSE_LOCK_FIELD2_START_BITS 235  /* 第二加锁区起始bit */
#define EFUSE_LOCK_SIZE              36   /* 第一加锁区加锁的总bit位数 */
#define EFUSE_LOCK_FIELD2_SIZE       5    /* 第二加锁区加锁的总bit位数 */
#define EFUSE_MAX_INDEX_SIZE         32   /* 最大写入的efuse数据长度(单位字节) */

#define EFUSE_IDX_NRW 0x0 /* 不可读写 */
#define EFUSE_IDX_RO  0x1 /* 只读 */
#define EFUSE_IDX_WO  0x2 /* 只写 */
#define EFUSE_IDX_RW  0x3 /* 可读可写 */

hi_efuse_stru *get_efuse_cfg(hi_void);
hi_void get_efuse_cfg_by_id(hi_efuse_idx idx, hi_u16 *start_bit, hi_u16 *size, hi_u8 *attr);
hi_u32 efuse_read_bits(hi_u16 start_bit, hi_u16 size, hi_u8 *key_data);
hi_u32 efuse_write_bits(hi_u16 start_bit, hi_u16 size, const hi_u8 *key_data, hi_u8 *err_state);

#endif /* __EFUSE_H__ */

