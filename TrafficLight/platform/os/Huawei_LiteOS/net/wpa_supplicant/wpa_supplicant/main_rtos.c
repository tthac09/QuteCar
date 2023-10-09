/*
 * WPA Supplicant / main() function for UNIX like OSes and MinGW
 * Copyright (c) 2003-2013, Jouni Malinen <j@w1.fi>
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

#include "common.h"
#include "wpa_supplicant_i.h"
#include "wpa_supplicant.h"
#include "los_task.h"
#include "wifi_api.h"
#include "eloop.h"
#include "hostapd/hostapd_if.h"
#ifdef CONFIG_P2P
#include "p2p_supplicant.h"
#endif /* CONFIG_P2P */
#include "securec.h"
#ifdef LOS_INLINE_FUNC_CROP
#include "drivers/driver.h"
#endif /* LOS_INLINE_FUNC_CROP */

EVENT_CB_S g_wpa_event;
#ifdef CONFIG_DRIVER_HISILICON
static struct wpa_global *g_wpa_global = NULL;
static struct wpa_interface *g_wpa_iface = NULL;
#endif /* CONFIG_DRIVER_HISILICON */

struct wpa_global * wpa_supplicant_get_wpa_global(void)
{
	return g_wpa_global;
}

void wpa_supplicant_global_deinit()
{
	os_free(g_wpa_iface);
	g_wpa_iface = NULL;
	g_wpa_global = NULL;
	wpa_error_log0(MSG_ERROR, "wpa_supplicant_exit\n");
}

static int wpa_supplicant_main_int(const char* ifname, struct hisi_wifi_dev **wifi_dev, struct wpa_global **global)
{
	if (ifname == NULL) {
		wpa_error_log0(MSG_ERROR, "wpa_supplicant_main: ifname is null \n");
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_ERROR);
		return HISI_FAIL;
	}
	wpa_debug_level = MSG_DEBUG;
	wpa_printf(MSG_DEBUG, "wpa_supplicant_main: ifname = %s", ifname);
	*wifi_dev = hi_get_wifi_dev_by_name(ifname);
	if (*wifi_dev == NULL) {
		wpa_error_log0(MSG_ERROR, "wpa_supplicant_main: get_wifi_dev_by_name failed \n");
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_ERROR);
		return HISI_FAIL;
	}
	*global = wpa_supplicant_init();
	if (*global == NULL) {
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_ERROR);
		return HISI_FAIL;
	}
	return HISI_OK;
}

int wpa_supplicant_main(const char* ifname)
{
	struct wpa_interface *ifaces     = NULL;
	struct wpa_interface *iface      = NULL;
	struct wpa_global *global        = NULL;
	struct hisi_wifi_dev *wifi_dev   = NULL;
	char driver[MAX_DRIVER_NAME_LEN] = {"hisi"};
	int iface_count;
	int exitcode = HISI_OK;
	int ret;
	wpa_error_log0(MSG_ERROR, "---> wpa_supplicant_main enter.");

	ret = wpa_supplicant_main_int(ifname, &wifi_dev, &global);
	if (ret != HISI_OK)
		return HISI_FAIL;

	ifaces = os_zalloc(sizeof(struct wpa_interface));
	if (ifaces == NULL)
		goto OUT;
	iface = ifaces;
	iface_count = 1;
	iface->ifname = ifname;
	iface->driver = driver;

#ifdef CONFIG_DRIVER_HISILICON
	g_wpa_global = global;
	g_wpa_iface = ifaces;
#endif /* CONFIG_DRIVER_HISILICON */

	for (int i = 0; (exitcode == HISI_OK) && (i < iface_count); i++) {
		struct wpa_supplicant *wpa_s = wpa_supplicant_add_iface(global, &ifaces[i], NULL);
		if (wpa_s == NULL) {
			exitcode = HISI_FAIL;
			break;
		}
		LOS_TaskLock();
		wifi_dev->priv = wpa_s;
		LOS_TaskUnlock();
		wpa_error_buf(MSG_ERROR, "wpa_supplicant_main: wifi_dev: ifname = %s\n", wifi_dev->ifname, strlen(wifi_dev->ifname));
	}
	if (wifi_dev->iftype == HI_WIFI_IFTYPE_STATION)
		g_connecting_ssid = NULL;
	if (exitcode == HISI_OK) {
		exitcode = wpa_supplicant_run(global);
		if (exitcode != HISI_FAIL) {
			(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_OK);
			return HISI_OK;
		}
	}

OUT:
	wpa_supplicant_deinit(global);
#ifdef CONFIG_DRIVER_HISILICON
	g_wpa_global = NULL;
	g_wpa_iface = NULL;
#endif
	LOS_TaskLock();
	if (wifi_dev != NULL)
		wifi_dev->priv = NULL;
	LOS_TaskUnlock();
	os_free(ifaces);
	// send sta start failed event
	(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_ERROR);
	return HISI_FAIL;
}


 int wpa_check_reconnect_timeout_match(const struct wpa_supplicant *wpa_s)
{
	int ret;
	if (wpa_s == NULL)
		return 0;
	if ((g_reconnect_set.enable == WPA_FLAG_OFF) ||
	    (g_reconnect_set.pending_flag == WPA_FLAG_ON)) {
		return 0;
	}
	ret = ((wpa_s->wpa_state >= WPA_ASSOCIATING) && (wpa_s->wpa_state != WPA_COMPLETED)) ||
	       ((wpa_s->wpa_state == WPA_COMPLETED) && (wpa_s->current_ssid != NULL) &&
	       (g_reconnect_set.current_ssid == wpa_s->current_ssid));
	return ret;
}

void wpa_supplicant_reconnect_restart(void *eloop_ctx, void *timeout_ctx)
{
	(void)timeout_ctx;
	struct wpa_supplicant *wpa_s = eloop_ctx;
	if (wpa_s == NULL) {
		wpa_error_log0(MSG_ERROR, "wpa_supplicant_reconnect_restart input NULL ptr!");
		return;
	}
	if ((g_reconnect_set.enable == WPA_FLAG_OFF) || (g_connecting_ssid == NULL)) {
		wpa_error_log0(MSG_ERROR, "reconnect policy disabled!");
		return;
	}
	if (wpa_s->wpa_state < WPA_AUTHENTICATING) {
		g_reconnect_set.pending_flag = WPA_FLAG_ON;
		g_connecting_flag = WPA_FLAG_ON;
		wpa_supplicant_select_network(wpa_s, g_connecting_ssid);
		wpa_error_log0(MSG_ERROR, "wpa_supplicant_reconnect_restart!");
	}

	if (g_reconnect_set.timeout > 0)
		eloop_register_timeout(g_reconnect_set.timeout, 0, wpa_supplicant_reconnect_timeout, wpa_s, NULL);
}

void wpa_supplicant_reconnect_timeout(void *eloop_ctx, void *timeout_ctx)
{
	(void)timeout_ctx;
	struct wpa_supplicant *wpa_s = eloop_ctx;
	if (wpa_s == NULL) {
		wpa_error_log0(MSG_ERROR, "wpa_supplicant_reconnect_timeout input NULL ptr!");
		return;
	}
	if (g_reconnect_set.enable == WPA_FLAG_OFF) {
		wpa_error_log0(MSG_ERROR, "reconnect policy disabled!");
		return;
	}
	if ((wpa_s->wpa_state != WPA_COMPLETED) && (g_reconnect_set.pending_flag)) {
		wpas_request_disconnection(wpa_s);
		g_connecting_flag = WPA_FLAG_OFF;
		wpa_error_log0(MSG_ERROR, "wpa reconnect timeout, try to connect next period!");
	}
	g_reconnect_set.pending_flag = WPA_FLAG_OFF;
	if (++g_reconnect_set.try_count >= g_reconnect_set.max_try_count) {
		g_reconnect_set.current_ssid = NULL;
		wpa_error_log0(MSG_ERROR, "wpa reconnect timeout, do not try to connect any more !");
		return;
	}
	if (g_reconnect_set.period > 0)
		eloop_register_timeout(g_reconnect_set.period, 0, wpa_supplicant_reconnect_restart, wpa_s, NULL);
}

static hi_wifi_event g_disconnect_delay_report_events = { 0 };

void wpa_send_disconnect_delay_report_events(void)
{
	g_disconnect_delay_report_events.event = HI_WIFI_EVT_DISCONNECTED;
	if ((g_wpa_event_cb != NULL) && !is_zero_ether_addr(g_disconnect_delay_report_events.info.wifi_disconnected.bssid))
		wifi_new_task_event_cb(&g_disconnect_delay_report_events);
	(void)memset_s(&g_disconnect_delay_report_events, sizeof(hi_wifi_event), 0, sizeof(hi_wifi_event));
}

void wpa_save_disconnect_event(const struct wpa_supplicant *wpa_s, const u8 *bssid, u16 reason_code)
{
	errno_t rc;
	unsigned char *old_bssid = g_disconnect_delay_report_events.info.wifi_disconnected.bssid;
	if ((wpa_s == NULL) || (bssid == NULL))
		return;
	if ((!is_zero_ether_addr(old_bssid)) && (memcmp(old_bssid, bssid, ETH_ALEN) != 0))
		return;
	rc = memset_s(&g_disconnect_delay_report_events, sizeof(hi_wifi_event), 0, sizeof(hi_wifi_event));
	if (rc != EOK)
		return;
	g_disconnect_delay_report_events.event = HI_WIFI_EVT_DISCONNECTED;
	rc = memcpy_s(g_disconnect_delay_report_events.info.wifi_disconnected.ifname, WIFI_IFNAME_MAX_SIZE + 1,
	              wpa_s->ifname, WIFI_IFNAME_MAX_SIZE);
	if (rc != EOK)
		return;
	rc = memcpy_s(g_disconnect_delay_report_events.info.wifi_disconnected.bssid, ETH_ALEN, bssid, ETH_ALEN);
	if (rc != EOK)
		return;
	g_disconnect_delay_report_events.info.wifi_disconnected.reason_code = reason_code;
}

void wpa_sta_delay_report_deinit(const struct hisi_wifi_dev *wifi_dev)
{
	errno_t rc;
	if ((wifi_dev == NULL) || (wifi_dev->iftype != HI_WIFI_IFTYPE_STATION))
		return;
	g_sta_delay_report_flag = WPA_FLAG_OFF;
	rc = memset_s(&g_disconnect_delay_report_events, sizeof(hi_wifi_event), 0, sizeof(hi_wifi_event));
	if (rc != EOK)
		wpa_error_log0(MSG_ERROR, "wpa_sta_delay_report_deinit memset_s fail !");
}

#ifdef LOS_INLINE_FUNC_CROP
int wpa_drv_get_bssid(struct wpa_supplicant *wpa_s, u8 *bssid)
{
	if ((wpa_s != NULL) && (wpa_s->driver != NULL) && (wpa_s->driver->get_bssid != NULL))
		return wpa_s->driver->get_bssid(wpa_s->drv_priv, bssid);
	return -1;
}

int wpa_drv_get_ssid(struct wpa_supplicant *wpa_s, u8 *ssid)
{
	if ((wpa_s != NULL) && (wpa_s->driver != NULL) && (wpa_s->driver->get_ssid != NULL))
		return wpa_s->driver->get_ssid(wpa_s->drv_priv, ssid);
	return -1;
}

int wpa_drv_set_key(struct wpa_supplicant *wpa_s,
				  enum wpa_alg alg, const u8 *addr,
				  int key_idx, int set_tx,
				  const u8 *seq, size_t seq_len,
				  const u8 *key, size_t key_len)
{
	if ((wpa_s == NULL) || (wpa_s->driver == NULL) || (wpa_s->driver->get_ssid == NULL))
		return -1;

	if (alg != WPA_ALG_NONE) {
		if ((key_idx >= 0) && (key_idx <= 6))
			wpa_s->keys_cleared &= ~BIT(key_idx);
		else
			wpa_s->keys_cleared = 0;
	}
	if (wpa_s->driver->set_key) {
		return wpa_s->driver->set_key(wpa_s->ifname, wpa_s->drv_priv,
			                      alg, addr, key_idx, set_tx,
			                      seq, seq_len, key, key_len);
	}
	return -1;
}
#endif /* LOS_INLINE_FUNC_CROP */
