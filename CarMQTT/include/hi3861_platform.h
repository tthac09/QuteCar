/*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
* Description: Hi3861 platform head.
* Author: HiSilicon
* Create: 2019-4-3
*/
#ifndef __HI3861_PLATFORM_H
#define __HI3861_PLATFORM_H

#include <hi_types.h>
#include <hi3861_platform_base.h>

#ifdef HI_BOARD_FPGA
#define PKT_H_START_ADDR    (0x03100000 + 0x4000) /* 16K use for MAC data collect */
#define PKT_H_LEN           0x2000 /* PKT_H:8K MIN:7K */
#define PKT_B_START_ADDR    (0x03100000 + 0x2000 + 0x4000)
#define PKT_B_LEN           0x8000 /* PKT_B:32K MIN:32K */
#else
#ifdef HI_ON_RAM
#define PKT_H_START_ADDR    0x02400000
#define PKT_H_LEN           0x2000 /* PKT_H:8K MIN:7K */
#define PKT_B_START_ADDR    0x03100000
#define PKT_B_LEN           0x10000 /* PKT_B:64K MIN:32K */
#else
#define PKT_H_START_ADDR    0x02400000
#define PKT_H_LEN           0x2000 /* PKT_H:8K MIN:7K */
#define PKT_B_LEN           0x8000 /* PKT_B:32K MIN:32K */
#ifdef FEATURE_DAQ
#define PKT_B_START_ADDR    (0x03100000 + 0x4000) /* 16K use for MAC data collect */
#else
#define PKT_B_START_ADDR    (0x03100000)
#endif

#endif
#endif

#define CALI_PMU_32K_CLK_VAL    60

#define HI_CHIP_ID_1131SV200    0xFF
#define HI_CHIP_VER_HI3861L     0x0
#define HI_CHIP_VER_HI3861      0x1
#define HI_CHIP_VER_HI3881      0x2

hi_void app_io_init(hi_void);
hi_void app_main(hi_void);

#endif /* __HI3861_PLATFORM_H */

