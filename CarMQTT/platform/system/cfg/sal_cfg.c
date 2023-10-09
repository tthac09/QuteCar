/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: sal_cfg.c
 * Author: hisilicon
 * Create: 2019-08-27
 */

#include <hi_sal_cfg.h>
#include <hi_early_debug.h>
#include "sal_cfg.h"
#include <hi_nv.h>

hi_u32 g_auto_wdg_rst_sys_timeout = PRODUCT_CFG_AUTO_WDG_RESET_SYSTEM_TIMEOUT;

hi_void hi_tee_irq_handler(void)
{
#ifdef CONFIG_TEE_HUKS_SUPPORT
#ifdef CONFIG_FACTORY_TEST_MODE
    hks_handle_secure_pki_provision();
#else
    hks_handle_secure_call();
#endif
#else
    printf("receive tee irq\r\n");
#endif
}

hi_u32 hi_get_rf_xtal_compesation_param(rf_cfg_xtal_compesation *xtal_param)
{
    if (xtal_param == HI_NULL) {
        return HI_ERR_FAILURE;
    }

    return hi_factory_nv_read(INIT_CONFIG_XTAL_COMPESATION, (hi_void *)xtal_param, sizeof(rf_cfg_xtal_compesation), 0);
}

hi_void hi_diag_register_wifi_log()
{
#ifdef CONFIG_DIAG_SUPPORT
    oam_log_hook_func log_hook = {
        hi_diag_log_msg0,
        hi_diag_log_msg1,
        hi_diag_log_msg2,
        hi_diag_log_msg3,
        hi_diag_log_msg4,
        hi_diag_log_msg_buffer
    };
    oam_log_register_hook_func(&log_hook);
#endif
}

