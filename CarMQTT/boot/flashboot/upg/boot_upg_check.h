/*
  * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
  * Description: boot upg check.
  * Author: Hisilicon
  * Create: 2019-12-10
 */

#ifndef __BOOT_UPG_CHECK_H__
#define __BOOT_UPG_CHECK_H__

#include <boot_upg_common.h>

#define PRODUCT_UPG_FILE_IMAGE_ID 0x3C78961E
#define KERELN_VER_MAX          48
#define KERNEL_VER_LEN          6
hi_u32 boot_upg_get_common_head(hi_u32 addr, hi_upg_common_head *head);
hi_u32 boot_upg_get_section_head(hi_u32 addr, hi_upg_section_head *section_head);
hi_u32 boot_upg_check_common_head(const hi_upg_common_head *head);
hi_u32 boot_upg_check_file(hi_u32 flash_addr);
hi_void boot_get_start_addr_offset(hi_u32 addr, hi_u32 *offset);
hi_u32 boot_upg_check_code_ver(hi_u8 ver);

#endif
