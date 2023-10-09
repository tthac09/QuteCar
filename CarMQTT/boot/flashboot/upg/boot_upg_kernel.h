/**
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Interface of upgrade Kernel
 * Author: Hisilicon
 * Create: 2020-2-27
 */
#ifndef __BOOT_UPG_KERNEL_H__
#define __BOOT_UPG_KERNEL_H__

#include "boot_upg_common.h"

hi_u32 boot_upg_kernel_process(hi_u32 addr_start, hi_u32 addr_write);

#endif
