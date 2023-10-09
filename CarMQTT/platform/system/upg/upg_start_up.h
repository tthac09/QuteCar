/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Interface of upg start up.
 * Author: Hisilicon
 * Create: 2019-12-10
 */
#ifndef _UPG_START_UP_H_
#define _UPG_START_UP_H_

#include <upg_common.h>

#ifndef offsetof
#define offsetof(type, member) ((hi_u32) & ((type *)0)->member)
#endif

#define ENV_REFRESH_NV 0x55

hi_u32 upg_get_start_up_nv(hi_nv_ftm_startup_cfg *cfg);
hi_u32 upg_save_start_up_nv(hi_nv_ftm_startup_cfg *cfg);

#endif /* _UPG_START_UP_H_ */
