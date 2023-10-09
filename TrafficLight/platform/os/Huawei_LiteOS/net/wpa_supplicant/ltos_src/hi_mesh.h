/*
 * WPA Supplicant - Basic mesh peer management
 * Copyright (c) 2013-2014, cozybit, Inc.  All rights reserved.
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
/****************************************************************************
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations,
 * which might include those applicable to Huawei LiteOS of U.S. and the country in
 * which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in
 * compliance with such applicable export control laws and regulations.
 ****************************************************************************/

#ifndef HI_MESH_H
#define HI_MESH_H

#include "utils/common.h"
#include "ap/hostapd.h"
#include "wpa_supplicant_i.h"
#include "drivers/driver_hisi.h"
#include "ap/wpa_auth.h"
#include "mesh_rsn.h"
#ifdef LOS_CONFIG_MESH_GTK
#include "wifi_api.h"
#endif /* LOS_CONFIG_MESH_GTK */

/* HUAWEI OUI define */
#define WPA_WLAN_OUI_HUAWEI        0x00E0FC
#define WPA_MESH_PEER_SCAN_TIMEOUT 5

extern int g_mesh_sta_flag;
extern int g_mesh_reject_peer_flag;
extern int g_mesh_connect_flag;

struct hi_mesh_ap_param {
	unsigned char mesh_bcn_priority;
	unsigned char mesh_is_mbr;
	unsigned char accept_sta;
	unsigned char peering_num;
	unsigned char accept_peer;
	unsigned short mesh_rank;
	unsigned char switch_status;
};

const u8 * wpa_bss_get_hi_mesh_ie(const struct wpa_bss *bss);
void hi_mesh_ie_parse(struct ieee802_11_elems *elems, const u8 *pos, size_t elen);
void hi_wpa_mesh_add_scan_req_ie(const struct wpa_supplicant *wpa_s, struct wpabuf **extra_ie);
u8 * hi_hostapd_eid_mesh_id(const struct hostapd_data *hapd, u8 *eid);
u8 * hi_hostapd_eid_mesh_cfg(const struct hostapd_data *hapd, u8 *eid);
int wpa_mesh_ap_config_set(const struct wpa_ssid *ssid);
int wpa_supplicant_ctrl_iface_mesh_start(struct wpa_supplicant *wpa_s, char *cmd);
int hi_wpa_mesh_peer(struct wpa_supplicant *wpa_s, struct wpa_bss *bss);
int hi_wpa_mesh_ap_scan(struct wpa_supplicant *wpa_s);
void hi_wpa_meshap_req_scan(struct wpa_supplicant *wpa_s, int sec, int usec);
void hi_wpa_join_mesh(struct wpa_supplicant *wpa_s);
void hi_wpa_rejoin_mesh(struct wpa_supplicant *wpa_s);
void hi_wpa_join_mesh_completed(struct wpa_supplicant *wpa_s);
int wpa_sta_event_check(enum wpa_event_type event);
void hi_wpa_mesh_accept_peer_update(struct wpa_supplicant *wpa_s, char *accept_peer);
int wpas_mesh_searching(const struct wpa_supplicant *wpa_s);
u8 * hi_wpa_scan_get_mesh_id(const struct wpa_scan_res *res);
int hi_mesh_mpm_send_plink_close_action(struct wpa_supplicant *wpa_s, const u8 *sta_addr);
void hi_mesh_report_connect_results(struct sta_info *sta, bool is_connected, unsigned short reason_code);
void hi_mesh_deinit(void);
int hi_wpa_find_hisi_mesh_ie(const u8 *start, size_t len);
int wpa_supplicant_match_privacy(struct wpa_bss *bss, struct wpa_ssid *ssid);
void wpa_bss_mesh_scan_result_sort(struct wpa_supplicant *wpa_s);
int hi_wpa_get_mesh_ap_param(const struct wpa_bss *bss, struct hi_mesh_ap_param* mesh_ap_param);
void hi_wpa_mesh_scan(void *wpa_s, void *data);
struct wpa_ssid * hi_wpa_scan_res_match(struct wpa_supplicant *wpa_s, int i, struct wpa_bss *bss,
                                        struct wpa_ssid *group, int only_first_ssid);
#ifdef LOS_CONFIG_MESH_GTK
void hi_mesh_gtk_rekey(struct hisi_wifi_dev *wifi_dev);
int32 hisi_set_mesh_mgtk(const char *ifname, const uint8 *addr, const uint8 *mgtk, size_t mgtk_len);
int hisi_mesh_plink_timer_mgtk_process(struct wpa_supplicant *wpa_s, struct sta_info *sta,
										const struct mesh_conf *conf);
int hisi_mesh_mgtk_info_accept_process(struct wpa_supplicant *wpa_s, struct sta_info *sta);
int hisi_mesh_mgtk_ack_accept_process(struct wpa_supplicant *wpa_s, struct sta_info *sta);
u8 * hi_mesh_rsn_protect_frame_set_ampe_ie(const struct mesh_rsn *rsn, u8 **p_ampe_ie, size_t *plen,
										   const u8 *cat, const struct wpabuf *buf, struct sta_info *sta);
int hi_mesh_mesh_rsn_process_ampe_check(u8 **ppos, const u8 *cat, struct sta_info *sta);
#endif /* LOS_CONFIG_MESH_GTK */
int hi_mesh_set_rsn_mgtk(struct mesh_rsn *rsn, const struct wpa_auth_config *conf);
u8 * hi_mesh_rsn_protect_frame_set_cat(const u8 *cat, const u8 *ie, size_t *pcat_len);
const u8 * hi_mesh_rsn_process_ampe_set_cat(const struct ieee802_11_elems *elems);
void hisi_driver_event_mesh_close_process(hisi_driver_data_stru *drv, int8 *data_ptr);
int hisi_mesh_init(void *priv);
int32 hisi_mesh_sta_add(void *priv, struct hostapd_sta_add_params *params);

#endif /* HI_MESH_H */
