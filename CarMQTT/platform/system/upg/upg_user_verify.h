/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Interface of upg user verify.
 * Author: Hisilicon
 * Create: 2020-03-03
 */

#ifndef __UPG_USER_VERIFY_H__
#define __UPG_USER_VERIFY_H__

#include "upg_common.h"

typedef struct {
    hi_u32 (*upg_file_check_fn)(const hi_upg_user_info *info, hi_void *param);
    hi_void *user_param;
} upg_user_verify_inf;

hi_u32 upg_user_define_verify(const hi_upg_user_info *info);

#endif /* __UPG_USER_VERIFY_H__ */

