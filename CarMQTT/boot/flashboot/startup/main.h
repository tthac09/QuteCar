/*
  * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
  * Description: flashboot main head.
  * Author: Hisilicon
  * Create: 2019-12-10
 */

#ifndef __MAIN_H__
#define __MAIN_H__

HI_EXTERN hi_u32 __heap_begin__;
HI_EXTERN hi_u32 __heap_end__;

#define HI_CHIP_VER_HI3861L     0x0
#define IO_CTRL_REG_BASE_ADDR   0x904   /* io控制寄存器基地址，用于驱动能力、上下拉等配置 */
/* bit mask */
#define MSK_2_B                 0x3
#define MSK_3_B                 0x7
/* bit offset */
#define OFFSET_4_B              4
#define OFFSET_22_B             22
#define OFFSET_25_B             25
#define OFFSET_28_B             28

#endif
