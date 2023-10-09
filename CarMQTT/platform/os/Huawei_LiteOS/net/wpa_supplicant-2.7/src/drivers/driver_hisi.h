/*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
* Description: driver_hisi header
* Author: hisilicon
* Create: 2019-03-04
*/

#ifndef DRIVER_HISI_H
#define DRIVER_HISI_H

#include "driver.h"
#include "driver_hisi_common.h"
#include "wpa_supplicant/wifi_api.h"

struct modes {
	int32 modes_num_rates;
	int32 mode;
};

extern int wal_init_drv_wlan_netdev(hi_wifi_iftype type, hi_wifi_protocol_mode en_mode, char *ifname, int *len);
extern int wal_deinit_drv_wlan_netdev(const char *ifname);
void hisi_hapd_deinit(void *priv);
void hisi_wpa_deinit(void *priv);
int32 hisi_set_mesh_mgtk(const char *ifname, const uint8 *addr, const uint8 *mgtk, size_t mgtk_len);
int32 hisi_mesh_enable_flag(const char *ifname, enum hisi_mesh_enable_flag_type flag_type, uint8 enable_flag);
int32 hisi_get_drv_flags(void *priv, uint64 *drv_flags);
int32 hisi_get_p2p_mac_addr(void *priv, enum wpa_driver_if_type type, uint8 *mac_addr);
#ifdef CONFIG_MESH
int32 hisi_set_usr_app_ie(const char *ifname, uint8 set, hi_wifi_frame_type fram_type,
                          uint8 ie_type, const uint8 *ie, uint16 ie_len);
#endif /* CONFIG_MESH */
#endif /* DRIVER_HISI_H */
