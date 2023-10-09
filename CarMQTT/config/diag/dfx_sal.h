/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: dfx_sal.h
 * Author: Hisilicon
 * Create: 2019-4-3
 */

#ifndef __DFX_SAL_H__
#define __DFX_SAL_H__
#include <hi_ft_nv.h>
#include <dfx_sys.h>

/******************************************************************************
 命令ID定义 [0x7000, 0x8000)
 ******************************************************************************/
#define ID_DIAG_CMD_REMOTE              0x7000
#define ID_DIAG_CMD_DO_FRAME_ERR        0x7007 /* DIAG error report */

#define HI_DIAG_VER_FULL_PRODUCT_NAME_MAX_SIZE    60 /* VER版本说明 */
#define HI_DIAG_VER_SDK_PRODUCT_NAME_MAX_SIZE     24 /* SDK版本号 */
#define HI_DIAG_VER_BOOT_NAME_MAX_SIZE            8
#define HI_DIAG_VER_FULL_HW_INFO_MAX_SIZE         40 /* 硬件信息 */
#define HI_DIAG_VER_FULL_DEV_VER_NAME_MAX_SIZE    40 /* 设备信息 */

/* 精简结构，远程查询版本号命令(mini_ver)结构体，成员由VER结构体提取 */
typedef struct {
    hi_char ver[HI_DIAG_VER_FULL_PRODUCT_NAME_MAX_SIZE];
    hi_char sdk_ver[HI_DIAG_VER_SDK_PRODUCT_NAME_MAX_SIZE];
    hi_char dev[HI_DIAG_VER_FULL_DEV_VER_NAME_MAX_SIZE];
    hi_char hw[HI_DIAG_VER_FULL_HW_INFO_MAX_SIZE];
    hi_char boot_ver[HI_DIAG_VER_BOOT_NAME_MAX_SIZE];
} diag_cmd_soft_new_ver;

#define HI_SYS_ERR_SIGNATURE     hi_make_identifier('s', 'y', 's', 'E')

typedef struct {
    hi_u32 id;
} hi_dbg_stat_q;

#endif /* __DFX_SAL_H__ */

