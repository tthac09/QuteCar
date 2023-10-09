/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: header file of efuse driver.
 * Author: wangjun
 * Create: 2019-05-08
 */

#ifndef __EFUSE_H__
#define __EFUSE_H__
#include <hi_types.h>

#define EIGHT_BITS        8
#define THREE_BITS_OFFSET 3
#define SIZE_8_BITS       8
#define SIZE_16_BITS      16
#define SIZE_24_BITS      24
#define SIZE_32_BITS      32
#define SIZE_64_BITS      64
#define SIZE_72_BITS      72
#define DATA_LENGTH       4

hi_u32 efuse_bits_read(hi_u16 start_bit, hi_u16 size, hi_u8 *data, hi_u32 data_len);

#endif /* __EFUSE_H__ */

