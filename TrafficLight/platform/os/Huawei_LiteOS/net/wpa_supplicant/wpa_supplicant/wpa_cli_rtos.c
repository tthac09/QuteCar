/*
 * WPA Supplicant - command line interface for wpa_supplicant daemon
 * Copyright (c) 2004-2013, Jouni Malinen <j@w1.fi>
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

#include "utils/common.h"
#include "eloop.h"
#include "wifi_api.h"
#include "ctrl_iface.h"
#ifdef CONFIG_LITEOS_WPA
#include "wpa_supplicant_i.h"
#endif /* CONFIG_LITEOS_WPA */
#include "securec.h"
#include "los_hwi.h"

static int wpa_ctrl_command(void *buf)
{
	unsigned int int_save;
	int ret;
	int_save = LOS_IntLock();
	if (g_ctrl_iface_data == NULL) {
		wpa_error_log0(MSG_ERROR,"ctrl command: ctrl iface data is NULL\n");
		LOS_IntRestore(int_save);
		return HISI_FAIL;
	}
	ret = eloop_post_event(g_ctrl_iface_data->event_queue, buf, 1);
	LOS_IntRestore(int_save);
	return ret;
}

static int write_cmd(char *buf, size_t buflen, int argc, char *argv[])
{
	int i, res;
	char *pos = NULL;
	int sum;
	pos = buf;
	if (buflen < 1)
		return HISI_FAIL;
	res = snprintf_s(pos, buflen,  buflen - 1, "%s", argv[0]);
	if (res < 0)
		goto fail;
	pos += res;
	argc--;
	argv++;
	sum = res;
	for (i = 0; i < argc; i++) {
		res = snprintf_s(pos, buflen - sum, buflen - sum - 1," %s", argv[i]);
		if (res < 0)
			goto fail;
		pos += res;
		sum += res;
	}

	buf[buflen - 1] = '\0';
	return HISI_OK;

fail:
	wpa_error_log0(MSG_ERROR,"write_cmd: cmd is too Long \n");
	return HISI_FAIL;
}

int wpa_cli_cmd(struct wpa_supplicant *wpa_s, int argc, char *argv[])
{
	struct ctrl_iface_event_buf *event_buf  = NULL;
	char *buf = NULL;
	event_buf = (struct ctrl_iface_event_buf *)os_malloc(sizeof(struct ctrl_iface_event_buf));
	if (event_buf == NULL)
		return HISI_FAIL;

	buf = (char *)os_malloc(WPA_CTRL_CMD_LEN);
	if (buf == NULL)
		goto fail_event;

	if (write_cmd(buf, WPA_CTRL_CMD_LEN, argc, argv) < 0)
		goto fail;

	event_buf->wpa_s = wpa_s;
	event_buf->buf   = buf;
	if (wpa_ctrl_command(event_buf) < 0) {
		wpa_printf(MSG_ERROR, "wpa_cli_cmd fail! cmd = %s\n", buf);
		goto fail;
	}

	return HISI_OK;
fail:
	os_free(buf);
fail_event:
	os_free(event_buf);
	return HISI_FAIL;
}

#ifdef CONFIG_WPS
int wpa_cli_wps_pbc(struct wpa_supplicant *wpa_s, const char *bssid)
{
	char *cmd_bssid_null[] = {"WPS_PBC"};
	char *cmd_bssid[] = {"WPS_PBC", (char *)bssid};

	wpa_warning_log0(MSG_DEBUG, "Enter wpa_cli_wps_pbc");
	if (bssid == NULL) {
		wpa_warning_log0(MSG_DEBUG, "Enter wpa_cli_wps_pbc cmd_bssid_null");
		return wpa_cli_cmd(wpa_s, 1, cmd_bssid_null);
	} else {
		return wpa_cli_cmd(wpa_s, 2, cmd_bssid);
	}
}

int wpa_cli_wps_pin(struct wpa_supplicant *wpa_s, const char *pin, const char *bssid)
{
	wpa_warning_log0(MSG_DEBUG, "Enter wpa_cli_wps_pin");
	if (pin == NULL) {
		wpa_warning_log0(MSG_DEBUG, "wpa_cli_wps_pin : pin is NULL");
		return HISI_FAIL;
	}
	if (bssid != NULL) {
		char *cmd_bssid_pin[] = {"WPS_PIN", (char *)bssid, (char *)pin};
		return wpa_cli_cmd(wpa_s, 3, cmd_bssid_pin);
	} else {
		char *cmd_null_pin[] = {"WPS_PIN any", (char *)pin};
		return wpa_cli_cmd(wpa_s, 2, cmd_null_pin);
	}
}
#endif /* CONFIG_WPS */

int wpa_cli_scan(struct wpa_supplicant *wpa_s, const char *buf)
{
	char *cmd[] = {"SCAN", (char *)buf};

	return wpa_cli_cmd(wpa_s, 2, cmd);
}

int wpa_cli_scan_results(struct wpa_supplicant *wpa_s)
{
	char *cmd[] = {"SCAN_RESULTS"};

	return wpa_cli_cmd(wpa_s, 1, cmd);
}

int wpa_cli_channel_scan(struct wpa_supplicant *wpa_s, int channel)
{
	int freq;
	char buf[10];

	if ((channel > 14) || (channel < 1)) {
		wpa_warning_log0(MSG_DEBUG, "set channel error\n");
		return HISI_FAIL;
	}

	if (channel != 14)
		freq = (channel - 1) * 5 + 2412;
	else
		freq = 2484;

	if (snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "freq=%d",freq) < 0)
		return HISI_FAIL;

	char *cmd[] = {"SCAN",buf};

	return wpa_cli_cmd(wpa_s, 2, cmd);
}

int wpa_cli_ap_deauth(struct wpa_supplicant *wpa_s, const char *buf)
{
	char *cmd[] = {"AP_DEAUTH", (char *)buf};

	return wpa_cli_cmd(wpa_s, 2, cmd);
}

#ifdef LOS_CONFIG_MESH
int wpa_cli_mesh_deauth(struct wpa_supplicant *wpa_s, const char *buf)
{
	char *cmd[] = {"MESH_DEAUTH", (char *)buf};

	return wpa_cli_cmd(wpa_s, 2, cmd);
}

int wpa_cli_join_mesh(struct wpa_supplicant *wpa_s)
{
	char *cmd[] = {"JOIN_MESH"};

	return wpa_cli_cmd(wpa_s, 1, cmd);
}
#endif /* LOS_CONFIG_MESH */

int wpa_cli_ap_scan(struct wpa_supplicant *wpa_s, const char *mode)
{
	char *cmd[] = {"AP_SCAN", (char *)mode};

	return wpa_cli_cmd(wpa_s, 2, cmd);
}

int wpa_cli_add_iface(struct wpa_supplicant *wpa_s, const char *ifname)
{
	char *cmd[] = {"ADD_IFACE", (char *)ifname};

	return wpa_cli_cmd(wpa_s, 2, cmd);
}

int wpa_cli_add_network(struct wpa_supplicant *wpa_s)
{
	char *cmd[] = {"ADD_NETWORK"};

	return wpa_cli_cmd(wpa_s, 1, cmd);
}

int wpa_cli_disconnect(struct wpa_supplicant *wpa_s)
{
	char *cmd[] = {"DISCONNECT"};

	return wpa_cli_cmd(wpa_s, 1, cmd);
}

int wpa_cli_remove_network(struct wpa_supplicant *wpa_s, const char *id)
{
	char *cmd_remove_all[] = {"REMOVE_NETWORK all"};
	char *cmd_remove[] = {"REMOVE_NETWORK", (char *)id};
	unsigned int ret;

	g_wpa_rm_network = HI_WPA_RM_NETWORK_START;
	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_STA_RM_NETWORK_OK);
	if (id == NULL)
		wpa_cli_cmd(wpa_s, 1, cmd_remove_all);
	else
		wpa_cli_cmd(wpa_s, 2, cmd_remove);
	wpa_error_log0(MSG_ERROR, "LOS_EventRead,wait for WPA_EVENT_STA_RM_NETWORK_OK");

	ret = LOS_EventRead(&g_wpa_event, WPA_EVENT_STA_RM_NETWORK_OK, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
	if (ret == WPA_EVENT_STA_RM_NETWORK_OK)
		wpa_error_log0(MSG_INFO, "wpa_cli_remove_network successful.");
	g_wpa_rm_network = HI_WPA_RM_NETWORK_END;
	return (ret == WPA_EVENT_STA_RM_NETWORK_OK) ? HISI_OK : HISI_FAIL;
}

int wpa_cli_terminate(struct wpa_supplicant *wpa_s, eloop_task_type e_type)
{
	char buf[10];

	if (snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "%d", e_type) < 0)
		return HISI_FAIL;

	char *cmd[] = {"ELOOP_TERMINATE", buf};

	return wpa_cli_cmd(wpa_s, 2, cmd);
}

int wpa_cli_remove_iface(struct wpa_supplicant *wpa_s)
{
	char *cmd[] = {"REMOVE_IFACE"};

	return wpa_cli_cmd(wpa_s, 1, cmd);
}

int wpa_cli_select_network(struct wpa_supplicant *wpa_s, const char *id)
{
	char *cmd[] = {"SELECT_NETWORK", (char *)id};

	return wpa_cli_cmd(wpa_s, 2, cmd);
}

int wpa_cli_set_network(struct wpa_supplicant *wpa_s, const char* id, const char *param, const char *value)
{
	char *cmd[] = {"SET_NETWORK", (char *)id, (char *)param, (char *)value};

	return wpa_cli_cmd(wpa_s, 4, cmd);
}

int wpa_cli_show_sta(struct wpa_supplicant *wpa_s)
{
	char *cmd[] = {"SHOW_STA"};

	return wpa_cli_cmd(wpa_s, 1, cmd);
}

int wpa_cli_sta_status(struct wpa_supplicant *wpa_s)
{
	char *cmd[] = {"STA_STATUS"};

	return wpa_cli_cmd(wpa_s, 1, cmd);
}

void wpa_cli_configure_wep(struct wpa_supplicant *wpa_s, const char *id, struct wpa_assoc_request *assoc)
{
	char command[33];
	int ret;

	if ((assoc == NULL) || (assoc->auth != HI_WIFI_SECURITY_WEP))
		return;

	(void)wpa_cli_set_network(wpa_s, id, "auth_alg", "OPEN SHARED");
	(void)wpa_cli_set_network(wpa_s, id, "key_mgmt", "NONE");
	ret = snprintf_s(command, sizeof(command), sizeof(command) - 1, "wep_key%d", 0);
	if (ret < 0)
		return;
	(void)wpa_cli_set_network(wpa_s, id, command, assoc->key);
	ret = snprintf_s(command, sizeof(command), sizeof(command) - 1, "%d", 0);
	if (ret < 0)
		return;
	(void)wpa_cli_set_network(wpa_s, id, "wep_tx_keyidx", command);
}

int wpa_cli_if_start(struct wpa_supplicant *wpa_s, hi_wifi_iftype iftype, const char *ifname)
{
	wpa_error_log0(MSG_ERROR, "---> wpa_cli_if_start enter.");
	char *cmd_softap[] = {"SOFTAP_START", (char *)ifname};
	char *cmd_wpa[] = {"WPA_START", (char *)ifname};

	if (iftype == HI_WIFI_IFTYPE_AP)
		return wpa_cli_cmd(wpa_s, 2, cmd_softap);

	return wpa_cli_cmd(wpa_s, 2, cmd_wpa);
}

int wpa_cli_sta_set_delay_report(struct wpa_supplicant *wpa_s, int enable)
{
	char *cmd_enable[] = {"STA_SET_DELAY_RP", "1"};
	char *cmd_disable[] = {"STA_SET_DELAY_RP", "0"};
	if (enable == WPA_FLAG_ON)
		return wpa_cli_cmd(wpa_s, 2, cmd_enable);
	else
		return wpa_cli_cmd(wpa_s, 2, cmd_disable);
}

