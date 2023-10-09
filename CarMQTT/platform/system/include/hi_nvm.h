/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: hi_nvm.h.
 * Author: hisilicon
 * Create: 2019-08-27
 */

#ifndef __HI_NVM_H__
#define __HI_NVM_H__

#include <hi_types.h>

#define HNV_FILE_SIGNATURE               hi_make_identifier('H', 'N', 'V', '$')
#define FNV_FILE_SIGNATURE               hi_make_identifier('F', 'N', 'V', '#')

#define FLASH_BLOCK_SIZE  0x1000
#define HNV_NCRC_SIZE     8                   /* 不做crc的长度 */
#define NV_TOTAL_MAX_NUM  255                 /* 可设置的nv最大个数 */
#define HNV_FAULT_TOLERANT_TIMES  3           /* 容错次数 */

#define HNV_MANAGE_FIXED_LEN  24              /* 截止到reserve */

/* 工厂区NV 结构体需要和kernel下保持完全一致，固定后不能修改 */
typedef struct _hi_nvm_manage_s_ {
    hi_u32  magic;              /*  nv头魔术字 */
    hi_u32  crc;                /*  nv管理区和数据区crc32 起：ver；止：flash_size结尾 */
    hi_u8   ver;                /*  nv管理区结构体版本号 */
    hi_u8   head_len;           /*  nv头的长度，起：magic；止：reserve结尾，且4字节向后对齐 */
    hi_u16  total_num;          /*  nv总个数 */
    hi_u32  seq;                /* 写nv序号标志(升级依然保留，以计算寿命) */
    hi_u32  ver_magic;          /* 版本魔术字，和kernel版本魔术字匹配 */
    hi_u32  flash_size;         /*  NV占用的FLASH大小，如4096(4K)、65536(64K),暂时未用，后续归一 */
    hi_u8   keep_id_range[2];   /* 升级保留id范围，0:id下边界 1:id上边界 size 2 */
    hi_u8   reserve[2];         /* size 2 */
    hi_u8   nv_item_data[0];    /* 索引表 */
} hi_nv_manage;

typedef struct hi_nv_item_index_s_ {
    hi_u8 nv_id;
    hi_u8 nv_len;                /* nv实际长度，不算crc32值，crc32紧挨着存放 */
    hi_u16 nv_offset;            /* 相对本nv区偏移 */
} hi_nv_item_index;

typedef struct _hi_nv_ctrl_s_ {
    hi_u32 base_addr;
    hi_u32 block_size;
    hi_u32 total_block_size;
    hi_u32 current_addr;         /* 保留更快 */
    hi_u32 seq;
    hi_u32 sem_handle;

    hi_u8 init_flag;
    hi_u8 reserve;
    hi_u16 total_num;         /* nv个数 */
    hi_u32 ver_magic;
    hi_nv_item_index* index;
} hi_nv_ctrl;

typedef enum _hi_nv_type_e_ {
    HI_TYPE_NV = 0,
    HI_TYPE_FACTORY_NV,
    HI_TYPE_TEMP,
    HI_TYPE_NV_MAX,
} hi_nv_type;

hi_u32 hi_nv_flush_keep_ids(hi_u8* addr, hi_u32 len);
hi_u32 hi_nv_block_write(hi_u8* nv_file, hi_u32 len, hi_u32 flag);

#endif   /* __HI_NVM_H__ */