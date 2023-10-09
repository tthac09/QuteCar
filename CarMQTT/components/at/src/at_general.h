/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: WAL layer external API interface implementation.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __AT_GENERAL_H__
#define __AT_GENERAL_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define DEFAULT_IFNAME_LOCALHOST    "lo"
#define DEFAULT_IFNAME_AP           "ap0"
#define DEFAULT_IFNAME_STA          "wlan0"
#define DEFAULT_IFNAME_MESH         "mesh0"

hi_void hi_at_general_cmd_register(hi_void);
hi_void hi_at_general_factory_test_cmd_register(hi_void);
hi_u32 at_wifi_ifconfig(hi_s32 argc, const hi_char **argv);
hi_u32 osShellDhcps(hi_s32 argc, const hi_char **argv);
hi_u32 at_lwip_ifconfig(hi_s32 argc, const hi_char **argv);
hi_void at_send_serial_data(hi_char *serial_data);

extern hi_u32 get_rf_cmu_pll_param(hi_s16 *high_temp, hi_s16 *low_temp, hi_s16 *compesation);

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif
