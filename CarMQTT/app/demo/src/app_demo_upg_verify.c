/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: app upg demo
 * Author: Hisilicon
 * Create: 2020-03-04
 */

#include "app_demo_upg_verify.h"
#include <hi_early_debug.h>
#include <hi_stdlib.h>

hi_u32 app_demo_upg_check_chip_product(const hi_upg_user_info *info, hi_void *param)
{
    hi_char file_chip[HI_APP_CHIP_PRODUCT_LEN] = { 0 };
    app_upg_user_info *user_info = HI_NULL;

    hi_unref_param(param);
    if (info == HI_NULL) {
        printf("[app upg demo verify]null.");
        return HI_ERR_FAILURE;
    }
    user_info = (app_upg_user_info *)info;
    if (sprintf_s(file_chip, HI_APP_CHIP_PRODUCT_LEN, "%s", CONFIG_CHIP_PRODUCT_NAME) < 0) {
        printf("[app upg demo verify]sprintf_s error \r\n");
        return HI_ERR_FAILURE;
    }
    printf("[app upg demo verify]kernel chip product:%s \r\n", file_chip);

    if (memcmp(user_info->chip_product, file_chip, HI_APP_CHIP_PRODUCT_LEN) == EOK) {
        return HI_ERR_SUCCESS;
    }

    return HI_ERR_FAILURE;
}

hi_u32 app_demo_upg_init(hi_void)
{
    return hi_upg_register_file_verify_fn(app_demo_upg_check_chip_product, HI_NULL);
}


