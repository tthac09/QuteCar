/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: boot start.
 * Author: Hisilicon
 * Create: 2019-12-10
 */

#ifndef __BOOT_START_H__
#define __BOOT_START_H__

#include <boot_upg_start_up.h>

hi_void boot_kernel(uintptr_t kaddr);
hi_void execute_upg_boot(hi_void);
hi_void boot_upg_init_verify_addr(const hi_nv_ftm_startup_cfg *cfg);

#endif
