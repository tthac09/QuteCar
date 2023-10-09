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

typedef struct _hisi_driver_data_stru {
	struct hostapd_data *hapd;
	const int8 iface[IFNAMSIZ + 1];
	int8 resv[3];
	uint64 send_action_cookie;
	void *ctx;
	void *event_queue;
	hisi_iftype_enum_uint8 nlmode;
	struct l2_packet_data *eapol_sock;  /* EAPOL message sending and receiving channel */
	uint8 own_addr[ETH_ALEN];
	uint8 resv1[2];
	uint32 associated;
	uint8 bssid[ETH_ALEN];
	uint8 ssid[MAX_SSID_LEN];
	uint8 resv2[2];
	uint32 ssid_len;
	struct wpa_scan_res *res[SCAN_AP_LIMIT];
	uint32 scan_ap_num;
	uint32 beacon_set;
} hisi_driver_data_stru;

extern int wal_init_drv_wlan_netdev(hi_wifi_iftype type, hi_wifi_protocol_mode en_mode, char *ifname, int *len);
extern int wal_deinit_drv_wlan_netdev(const char *ifname);
void hisi_hapd_deinit(void *priv);
void hisi_wpa_deinit(void *priv);
void hisi_rx_mgmt_count_reset(void);
uint8 * hisi_dup_macaddr(const uint8 *addr, size_t len);
int32 hisi_mesh_enable_flag(const char *ifname, enum hisi_mesh_enable_flag_type flag_type, uint8 enable_flag);
int32 hisi_get_drv_flags(void *priv, uint64 *drv_flags);
int32 hisi_get_p2p_mac_addr(void *priv, enum wpa_driver_if_type type, uint8 *mac_addr);
int32 hisi_set_usr_app_ie(const char *ifname, uint8 set, hi_wifi_frame_type fram_type,
                          uint8 ie_type, const uint8 *ie, uint16 ie_len);
#endif /* DRIVER_HISI_H */
