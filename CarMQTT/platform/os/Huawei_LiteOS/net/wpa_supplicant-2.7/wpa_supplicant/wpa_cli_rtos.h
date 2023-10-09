/*
 * wpa_supplicant/hostapd / Debug prints
 * Copyright (c) 2002-2013, Jouni Malinen <j@w1.fi>
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2019. All rights reserved.
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

#ifndef WPA_CLI_RTOS_H
#define WPA_CLI_RTOS_H

int wpa_cli_scan(struct wpa_supplicant *wpa_s, const char *buf);
int wpa_cli_scan_results(struct wpa_supplicant *wpa_s);
int wpa_cli_channel_scan(struct wpa_supplicant *wpa_s, int channel);
int wpa_cli_ap_scan(struct wpa_supplicant *wpa_s, const char *mode);
int wpa_cli_add_network(struct wpa_supplicant *wpa_s);
int wpa_cli_disconnect(struct wpa_supplicant *wpa_s);
int wpa_cli_remove_network(struct wpa_supplicant *wpa_s, const char *id);
int wpa_cli_remove_iface(struct wpa_supplicant *wpa_s);
int wpa_cli_select_network(struct wpa_supplicant *wpa_s, const char *id);
int wpa_cli_set_network(struct wpa_supplicant *wpa_s, const char* id, const char *param, const char *value);
int wpa_cli_show_sta(struct wpa_supplicant *wpa_s);
int wpa_cli_sta_status(struct wpa_supplicant *wpa_s);
void wpa_cli_configure_wep(struct wpa_supplicant *wpa_s, const char *id, struct wpa_assoc_request *assoc);
int wpa_cli_if_start(struct wpa_supplicant *wpa_s, hi_wifi_iftype iftype, const char *ifname);
int wpa_cli_add_iface(struct wpa_supplicant *wpa_s, const char *ifname);
int wpa_cli_terminate(struct wpa_supplicant *wpa_s, eloop_task_type e_type);
int wpa_cli_ap_deauth(struct wpa_supplicant *wpa_s, const char *buf);
#ifdef CONFIG_WPS
int wpa_cli_wps_pbc(struct wpa_supplicant *wpa_s, const char *bssid);
int wpa_cli_wps_pin(struct wpa_supplicant *wpa_s, const char *pin, const char *bssid);
#endif /* CONFIG_WPS */
#ifdef LOS_CONFIG_MESH
int wpa_cli_mesh_deauth(struct wpa_supplicant *wpa_s, const char *buf);
int wpa_cli_join_mesh(struct wpa_supplicant *wpa_s);
#endif /* LOS_CONFIG_MESH */
int wpa_cli_sta_set_delay_report(struct wpa_supplicant *wpa_s, int enable);

#endif /* WPA_CLI_RTOS_H */
