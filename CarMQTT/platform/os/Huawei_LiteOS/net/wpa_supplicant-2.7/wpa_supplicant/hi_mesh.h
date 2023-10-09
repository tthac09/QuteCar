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
#ifdef LOS_CONFIG_MESH_GTK
#include "wifi_api.h"
#endif /* LOS_CONFIG_MESH_GTK */

#define WPA_OUITYPE_MESH                        2
#define WPA_OUISUBTYPE_MESH_HISI_RSN            1
#define WPA_OUISUBTYPE_MESH_HISI_OPTIMIZATION   2
#define WPA_OUISUBTYPE_MESH_HISI_REQ            3
#define WPA_OUISUBTYPE_MESH_HISI_STA_REQ        4
#define WPA_OUISUBTYPE_MESH_HISI_BEACON         5
#define WPA_OUISUBTYPE_MESH_HISI_RSP            6
#define WPA_OUISUBTYPE_MESH_HISI_MESHID         7

/* HUAWEI OUI define */
#define WPA_WLAN_OUI_HUAWEI                     0x00E0FC
#define WPA_OUI_HISI_MESH                       0x00E0FC02
#define WPA_OUI_HISI_MESH_LENGTH                4
#define HI_MESH_CONFIG_IE_LENGTH                7
#define HI_MESH_PROBE_REQ_IE_LENGTH             5
#define WPA_MESH_HISI_OPTIMIZATION_LEN          2
#define WPA_MESH_MAX_PLINK_NUM                  63
#define WPA_MESH_PEER_SCAN_TIMEOUT              5
#define WPA_MESH_PERIOD_SCAN_TIMEOUT            120
#define WPA_MAX_MESH_CONNECT_TRY_CNT            3
#define WPA_MESH_LOWEST_PRIORITY_NUM            0
#define WPA_BSS_COMPAR_CHANGE                   1
#define WPA_BSS_COMPAR_REMAIN                   (-1)
#define WPA_MESH_HISI_OPTIMIZATION_VAL_LEN      11
#define WPA_MESH_HISI_OPT_BCN_PRIO_OFFSET       7
#define WPA_MESH_HISI_OPT_IS_MBR_OFFSET         8
#define WPA_MESH_HISI_OPT_ACPT_STA_OFFSET       9
#define WPA_MESH_HISI_OPT_MESH_RANK_OFFSET      10
#define WPA_MESH_HISI_OPT_SWITCH_STATUS_OFFSET  12
#define WPA_MESH_CONFIG_PEERING_NUM_OFFSET      7
#define WPA_MESH_CONFIG_MESH_CAPABILITY_OFFSET  8
#define WPA_MESH_PEERNUM_MSK                    0x3f

extern int g_mesh_sta_flag;
extern int g_mesh_reject_peer_flag;
extern int g_mesh_peer_scan_flag;
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

void hi_wpa_mesh_add_scan_req_ie(const struct wpa_supplicant *wpa_s, struct wpabuf **extra_ie);
u8 * hi_hostapd_eid_mesh_id(const struct hostapd_data *hapd, u8 *eid);
u8 * hi_hostapd_eid_mesh_cfg(const struct hostapd_data *hapd, u8 *eid);
int wpa_mesh_ap_config_set(const struct wpa_ssid *ssid);
int wpa_supplicant_ctrl_iface_mesh_start(struct wpa_supplicant *wpa_s, char *cmd);
void hi_wpa_mesh_peer(struct wpa_supplicant *wpa_s, struct wpa_bss *bss);
int hi_wpa_mesh_ap_scan(struct wpa_supplicant *wpa_s);
void hi_wpa_meshap_req_scan(struct wpa_supplicant *wpa_s, int sec, int usec);
void hi_wpa_join_mesh(struct wpa_supplicant *wpa_s);
void hi_wpa_rejoin_mesh(struct wpa_supplicant *wpa_s);
void hi_wpa_join_mesh_completed(struct wpa_supplicant *wpa_s);
int wpa_sta_event_check(enum wpa_event_type event);
void hi_wpa_mesh_accept_peer_update(struct wpa_supplicant *wpa_s, char *accept_peer);
int wpas_mesh_searching(const struct wpa_supplicant *wpa_s);
u8 * hi_wpa_scan_get_mesh_id(const struct wpa_scan_res *res);
void hi_event_new_mesh_peer(struct wpa_supplicant *wpa_s, const u8 *addr, u8 mesh_bcn_priority, u8 mesh_is_mbr);
void hi_mesh_mpm_send_plink_close_action(struct wpa_supplicant *wpa_s, const u8 *sta_addr);
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
#endif
#endif /* HI_MESH_H */
