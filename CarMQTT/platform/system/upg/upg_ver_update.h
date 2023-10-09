/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.  \n
 * Description: Upgrade ver update header file.  \n
 * Author: Hisilicon  \n
 * Create: 2020-02-21
 */

#ifndef _UPG_VER_UPDATE_H_
#define _UPG_VER_UPDATE_H_

#include "hi_types.h"

#define UPG_NMI_BASE_ADDRESS 0x40010000
#define UPG_NMI_CTRL         0x0258
#define UPG_NMI_INT_MOD_DONE_EN_POS 0

#define UPG_UPDATE_VER_NONE     0
#define UPG_UPDATE_VER_FIRMARE  1
#define UPG_UPDATE_VER_BOOT     2
hi_bool upg_update_ver(hi_void);
hi_u32 upg_start_and_wait_update_ver(hi_u8 update_ver_type);

#endif /* _UPG_VER_UPDATE_H_ */
