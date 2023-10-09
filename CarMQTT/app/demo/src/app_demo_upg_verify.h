/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Interface of app demo upg.
 * Author: Hisilicon
 * Create: 2020-03-04
 */

#ifndef _APP_DEMO_UPG_VERIFY_H_
#define _APP_DEMO_UPG_VERIFY_H_

#include <hi_upg_api.h>

#define HI_APP_CHIP_PRODUCT_LEN 8 /* 7: chip product;1£ºPadding */

typedef struct {
    hi_char chip_product[HI_APP_CHIP_PRODUCT_LEN];
    hi_u8 reserved[24]; /* 24 bytes reserved */
}app_upg_user_info;

hi_u32 app_demo_upg_init(hi_void);

#endif /* _APP_DEMO_UPG_VERIFY_H_ */

