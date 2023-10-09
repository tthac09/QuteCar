/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: efuse demo headfile.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __APP_DEMO_EFUSE_H__
#define __APP_DEMO_EFUSE_H__

#include <hi_types_base.h>
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <hi_early_debug.h>
#include <hi_efuse.h>
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define EFUSE_USR_RW_SAMPLE_BUFF_MAX_LEN 2 /* efuse customer_rsvd0字段的长度为64bits, 读写数据需要2个32bit空间来存储 */
/*****************************************************************************
  10 函数声明
*****************************************************************************/
hi_u32 get_efuse_id_size_test(hi_void);
hi_void efuse_get_lock_stat(hi_void);
hi_u32 efuse_usr_read(hi_void);
hi_u32 efuse_usr_write(hi_void);
hi_u32 efuse_usr_lock(hi_void);
hi_u32 sample_usr_efuse(hi_void);
hi_u32 efuse_id_read(hi_void);
hi_u32 efuse_id_write(hi_void);
hi_u32 efuse_id_lock(hi_void);
hi_u32 sample_id_efuse(hi_void);
hi_void efuse_demo(hi_void);
#endif
