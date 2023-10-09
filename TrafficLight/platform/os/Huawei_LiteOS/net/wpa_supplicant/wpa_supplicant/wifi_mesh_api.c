/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: wifi mesh APIs
 * Author: shizhencang
 * Create: 2019-02-10
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 * --------------------------------------------------------------------------- */

#include "utils/common.h"
#include "wifi_api.h"
#include "utils/eloop.h"
#include "wpa_supplicant_i.h"
#include "wpa_cli_rtos.h"
#include "wpa_supplicant.h"
#include "common/ieee802_11_common.h"
#ifdef LOS_CONFIG_MESH
#include "hi_mesh.h"
#endif /* LOS_CONFIG_MESH */
#include "config_ssid.h"
#include "src/crypto/sha1.h"
#include "drivers/driver_hisi.h"
#include "securec.h"
#include "los_hwi.h"
#include "los_task.h"


#ifdef LOS_CONFIG_MESH
struct wifi_mesh_opt_set g_mesh_opt_set                = { 0 };
hi_wifi_mesh_peer_info *g_mesh_sta_info                = NULL;
unsigned int g_mesh_get_sta_flag                       = WPA_FLAG_OFF;
#endif /* LOS_CONFIG_MESH */


#ifdef LOS_CONFIG_MESH
int hi_wifi_set_mesh_sta(unsigned char enable)
{
	if (enable > 1) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_set_mesh_sta: invalid param.");
		return HISI_FAIL;
	}

	if ((g_mesh_flag == WPA_FLAG_ON) ||
		(hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_MESH_POINT) != NULL) ||
		(hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_P2P_DEVICE) != NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_set_mesh_sta: station, mesh ap or p2p is in use");
		return HISI_FAIL;
	}

	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_set_mesh_sta: wifi dev start or stop is running.");
		return HISI_FAIL;
	}
	g_mesh_sta_flag = enable;
	wpa_error_log1(MSG_ERROR, "hi_wifi_set_mesh_sta: mesh_sta flag %d.", g_mesh_sta_flag);
	return HISI_OK;
}

int hi_wifi_mesh_sta_scan(void)
{
	return wifi_scan(HI_WIFI_IFTYPE_STATION, true, NULL);
}

int hi_wifi_mesh_sta_advance_scan(hi_wifi_scan_params *sp)
{
	return wifi_scan(HI_WIFI_IFTYPE_STATION, true, sp);
}

int hi_wifi_mesh_scan(void)
{
	return wifi_scan(HI_WIFI_IFTYPE_MESH_POINT, true, NULL);
}

int hi_wifi_mesh_advance_scan(hi_wifi_scan_params *sp)
{
	return wifi_scan(HI_WIFI_IFTYPE_MESH_POINT, true, sp);
}

static void wifi_scan_result_mesh_flag_parse(const char *starttmp, hi_wifi_mesh_scan_result_info *ap_list)
{
	char *str_hi_mbr       = NULL;
	char *str_hi_acpt_sta  = NULL;
	char *str_hi_acpt_peer = NULL;
	char *str_hi_peer_num  = NULL;
	char *str_hi_bcn_prio  = NULL;
	char *str_hi_rank = NULL;
	char *str_hi_switch_status = NULL;
	str_hi_mbr       = strstr(starttmp, "MBR");
	str_hi_acpt_sta  = strstr(starttmp, "ACPTS");
	str_hi_acpt_peer = strstr(starttmp, "ACPTP");
	str_hi_peer_num  = strstr(starttmp, "PNUM");
	str_hi_bcn_prio  = strstr(starttmp, "BPRI");
	str_hi_rank = strstr(starttmp, "RANK");
	str_hi_switch_status = strstr(starttmp, "CHSW");
	unsigned int str_len;

	if (str_hi_mbr != NULL)
		ap_list->is_mbr = WPA_FLAG_ON;

	if (str_hi_acpt_sta != NULL)
		ap_list->accept_for_sta = WPA_FLAG_ON;

	if (str_hi_acpt_peer != NULL)
		ap_list->accept_for_peer = WPA_FLAG_ON;

	if (str_hi_peer_num != NULL) {
		str_len = (unsigned int)strlen("PNUM");
		ap_list->peering_num = (unsigned char)atoi(&str_hi_peer_num[str_len]);
	}

	if (str_hi_bcn_prio != NULL) {
		str_len = strlen("BPRI");
		ap_list->bcn_prio = (unsigned char)atoi(&str_hi_bcn_prio[str_len]);
	}

	if (str_hi_rank != NULL) {
		str_len = (unsigned int)strlen("RANK");
		ap_list->mesh_rank = (unsigned short)atoi(&str_hi_rank[str_len]);
	}

	if (str_hi_switch_status != NULL) {
		str_len = (unsigned int)strlen("CHSW");
		ap_list->switch_status = (unsigned char)atoi(&str_hi_switch_status[str_len]);
	}
}

static int wifi_mesh_scan_results_parse(hi_wifi_mesh_scan_result_info *ap_list, unsigned int *ap_num)
{
	char *starttmp = NULL;
	char *endtmp   = NULL;
	int count = 0;
	size_t reply_len = g_result_len;
	char *buf = g_scan_result_buf;
	int ret = HISI_FAIL;

	int max_element = WPA_MIN(*ap_num, WIFI_SCAN_AP_LIMIT);
	starttmp = strchr(buf, '\n');
	if (starttmp == NULL)
		goto EXIT;

	*starttmp++ = '\0';
	reply_len -= starttmp - buf;
	while (count < max_element && reply_len > 0) {
		if (wifi_scan_result_bssid_parse(&starttmp, &ap_list[count], &reply_len) == HISI_FAIL) {
			ret = HISI_OK; // set ok if it is the end of the string
			goto EXIT;
		}
		if ((wifi_scan_result_freq_parse(&starttmp, &ap_list[count], &reply_len) == HISI_FAIL) ||
			(wifi_scan_result_rssi_parse(&starttmp, &ap_list[count], &reply_len) == HISI_FAIL))
			goto EXIT;

		endtmp = strchr(starttmp, '\t');
		if (endtmp == NULL)
			goto EXIT;

		*endtmp = '\0';
		wifi_scan_result_base_flag_parse(starttmp, &ap_list[count]);

		wifi_scan_result_mesh_flag_parse(starttmp, &ap_list[count]);

		reply_len -= endtmp - starttmp + 1;
		starttmp = ++endtmp;
		if (wifi_scan_result_ssid_parse(&starttmp, &ap_list[count], &reply_len) == HISI_OK)
			count++;
		else
			(void)memset_s(&ap_list[count], sizeof(*ap_list), 0, sizeof(*ap_list));
	}
	ret = HISI_OK;
EXIT:
	(void)memset_s(&g_scan_record, sizeof(g_scan_record), 0, sizeof(g_scan_record));
	*ap_num = count;
	g_scan_result_buf = NULL;
	(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_SCAN_RESULT_FREE_OK);
	return ret;
}

static int wifi_mesh_scan_results(hi_wifi_mesh_scan_result_info *ap_list, unsigned int *ap_num, bool is_sta)
{
	if ((ap_list == NULL) || (ap_num == NULL))
		return HISI_FAIL;
	if (is_sta == true) {
		if (g_mesh_sta_flag == WPA_FLAG_OFF)
			return HISI_FAIL;
	} else {
		if (g_mesh_sta_flag == WPA_FLAG_ON)
			return HISI_FAIL;
	}
	if (memset_s(ap_list, sizeof(hi_wifi_mesh_scan_result_info) * (*ap_num),
	    0, sizeof(hi_wifi_mesh_scan_result_info) * (*ap_num)) != EOK)
		return HISI_FAIL;
	hi_wifi_iftype iftype = (is_sta == true) ? HI_WIFI_IFTYPE_STATION : HI_WIFI_IFTYPE_MESH_POINT;
	if (wifi_scan_result(iftype) == HISI_OK)
		return wifi_mesh_scan_results_parse(ap_list, ap_num);
	else {
		(void)memset_s(&g_scan_record, sizeof(g_scan_record), 0, sizeof(g_scan_record));
		*ap_num = 0;
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_SCAN_RESULT_FREE_OK);
		return HISI_FAIL;
	}
}

int hi_wifi_mesh_sta_scan_results(hi_wifi_mesh_scan_result_info *ap_list, unsigned int *ap_num)
{
	return wifi_mesh_scan_results(ap_list, ap_num, true);
}

int hi_wifi_mesh_scan_results(hi_wifi_mesh_scan_result_info *ap_list, unsigned int *ap_num)
{
	return wifi_mesh_scan_results(ap_list, ap_num, false);
}

static int wifi_mesh_set_network_psk(struct wpa_supplicant *wpa_s, const char *id, hi_wifi_mesh_config *assoc)
{
	char param[WPA_MAX_SSID_KEY_INPUT_LEN + 4 + 1] = { 0 }; // add \"\" (length 4) for param
	unsigned int key_len = os_strlen(assoc->key);
	errno_t rc;
	int ret;

	if (assoc->auth == HI_WIFI_SECURITY_OPEN) {
		if ((wpa_cli_set_network(wpa_s, id, "auth_alg", "OPEN") == HISI_OK) &&
		    (wpa_cli_set_network(wpa_s, id, "key_mgmt", "NONE") == HISI_OK))
			return HISI_OK;
		else
			return HISI_FAIL;
	}

	if (assoc->auth == HI_WIFI_SECURITY_SAE) {
		if (wpa_cli_set_network(wpa_s, id, "key_mgmt", "SAE") != HISI_OK)
			return HISI_FAIL;
	} else {
		wpa_error_log0(MSG_ERROR, "wifi_set_mesh_network_psk: auth not support.\n");
		return HISI_FAIL;
	}

	(void)memset_s(param, sizeof(param), 0, sizeof(param));
	if (key_len < WPA_MAX_KEY_LEN) {
		ret = snprintf_s(param, sizeof(param), sizeof(param) - 1, "\"%s\"", assoc->key);
		if (ret < 0) {
			wpa_error_log1(MSG_ERROR, "wifi_set_mesh_network_psk: snprintf_s failed(%d).", ret);
			return HISI_FAIL;
		}
	} else {
		rc = memcpy_s(param, sizeof(param), assoc->key, key_len);
		if (rc != EOK) {
			wpa_error_log1(MSG_ERROR, "wifi_mesh_set_network_psk: memcpy_s failed(%d).", rc);
			return HISI_FAIL;
		}
	}

	if (wpa_cli_set_network(wpa_s, id, "psk", param) != HISI_OK)
		return HISI_FAIL;

	return HISI_OK;
}

static int wifi_mesh_config_check(hi_wifi_mesh_config *config)
{
	if ((strlen(config->ssid) < 1) || (strlen(config->ssid) > WPA_MAX_SSID_LEN)) {
		wpa_error_log0(MSG_ERROR, "wifi_mesh_start_config_check: invalid ssid");
		return HISI_FAIL;
	}

	if ((config->channel < 1) || (config->channel > WPA_24G_CHANNEL_NUMS)) {
		wpa_error_log0(MSG_ERROR, "wifi_mesh_start_config_check: invalid channel");
		return HISI_FAIL;
	}

	if ((config->auth < HI_WIFI_SECURITY_OPEN) || (config->auth > HI_WIFI_SECURITY_SAE)) {
		wpa_error_log0(MSG_ERROR, "wifi_mesh_start_config_check: invalid authmode");
		return HISI_FAIL;
	}

	if (config->auth != HI_WIFI_SECURITY_OPEN &&
		((strlen(config->key) < WPA_MIN_KEY_LEN) || (strlen(config->key) > WPA_MAX_KEY_LEN))) {
		wpa_error_log0(MSG_ERROR, "wifi_mesh_start_config_check: invalid key length");
		return HISI_FAIL;
	}
	return HISI_OK;
}

static int wifi_mesh_start(hi_wifi_mesh_config *config, struct hisi_wifi_dev *wifi_dev)
{
	char network_id_txt[WPA_NETWORK_ID_TXT_LEN + 1] = { 0 };
	char freq_txt[WPA_24G_FREQ_TXT_LEN + 1] = { 0 };
	int channel;
	int ret;
	unsigned int ret_val;

	if ((config == NULL) || (wifi_dev == NULL) || (wifi_dev->priv == NULL) ||
	    (wifi_mesh_config_check(config) != HISI_OK)) {
		wpa_error_log0(MSG_ERROR, "wifi_mesh_start_internal: invalid param.");
		return HISI_FAIL;
	}

	ret = snprintf_s(network_id_txt, sizeof(network_id_txt), sizeof(network_id_txt) - 1, "%d", wifi_dev->network_id);
	if (ret < 0)
		return HISI_FAIL;

	channel = chan_to_freq(config->channel); /* already checked in wifi_mesh_config_check */
	ret = snprintf_s(freq_txt, sizeof(freq_txt), sizeof(freq_txt) - 1, "%d", channel);
	if (ret < 0)
		return HISI_FAIL;

	g_mesh_flag = WPA_FLAG_ON;
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(wifi_dev->priv);
	wpa_cli_remove_network(wpa_s, network_id_txt);

	if ((wpa_cli_remove_network(wpa_s, network_id_txt) != HISI_OK) ||
	    (wpa_cli_add_network(wpa_s) != HISI_OK) ||
	    (wpa_cli_set_network(wpa_s, network_id_txt, "ssid", config->ssid) != HISI_OK) ||
	    (wpa_cli_set_network(wpa_s, network_id_txt, "ignore_broadcast_ssid", "1") != HISI_OK) ||
	    (wifi_mesh_set_network_psk(wpa_s, network_id_txt, config) != HISI_OK) ||
	    (wpa_cli_set_network(wpa_s, network_id_txt, "mode", "5") != HISI_OK) ||
	    (wpa_cli_set_network(wpa_s, network_id_txt, "frequency", freq_txt) != HISI_OK))
		goto FAIL;

	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_MESH_START_OK);
	if (wpa_cli_select_network(wpa_s, network_id_txt) != HISI_OK)
		goto FAIL;

	wpa_error_log0(MSG_ERROR, "LOS_EventRead WPA_EVENT_MESH_START_OK");
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_MESH_START_OK, (LOS_WAITMODE_OR | LOS_WAITMODE_CLR), WPA_DELAY_5S);
	if (ret_val != WPA_EVENT_MESH_START_OK) {
		wpa_error_log1(MSG_ERROR, "LOS_EventRead WPA_EVENT_MESH_START_OK faile ret_val =%x", ret_val);
		goto FAIL;
	}
	return HISI_OK;

FAIL:
	wpa_error_log0(MSG_ERROR, "wifi_mesh_start_internal: failed.");
	wpa_cli_remove_network(wpa_s, network_id_txt);
	g_mesh_flag = WPA_FLAG_OFF;
	return HISI_FAIL;
}

static int wifi_mesh_start_cond_check(void)
{
	if (try_set_lock_flag() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}

	if ((is_ap_mesh_or_p2p_on() == HISI_OK) || (g_mesh_sta_flag == WPA_FLAG_ON)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_mesh_start: should not start mesh.");
		clr_lock_flag();
		return HISI_FAIL;
	}
	return HISI_OK;
}

int hi_wifi_mesh_start(hi_wifi_mesh_config *config, char *ifname, int *len)
{
	int ret;
	int first_iface_flag  = WPA_FLAG_ON;

	if ((config == NULL) || (ifname == NULL) || (len == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_mesh_start: invalid param");
		return HISI_FAIL;
	}

	if (wifi_mesh_start_cond_check() != HISI_OK)
		return HISI_FAIL;

	struct hisi_wifi_dev *wifi_dev = wifi_dev_creat(HI_WIFI_IFTYPE_MESH_POINT, HI_WIFI_PHY_MODE_11BGN);
	if (wifi_dev == NULL)
		goto WIFI_MESH_EXIT;

	if ((hi_set_wifi_dev(wifi_dev) != HISI_OK) || (*len < wifi_dev->ifname_len + 1))
		goto WIFI_MESH_ERROR;

	ret = memcpy_s(ifname, *len, wifi_dev->ifname, wifi_dev->ifname_len);
	if (ret != EOK)
		goto WIFI_MESH_ERROR;

	ifname[wifi_dev->ifname_len] = '\0';
	*len = wifi_dev->ifname_len;

	if ((hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_P2P_DEVICE) != NULL) ||
		(hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_STATION) != NULL)) {
		first_iface_flag = WPA_FLAG_OFF;
		ret = wifi_add_iface(wifi_dev);
	} else
		ret = wifi_wpa_start(wifi_dev);

	if (ret != HISI_OK) {
		wpa_warning_log0(MSG_INFO, "hi_wifi_mesh_start: add iface or start wpa failed.");
		goto WIFI_MESH_ERROR;
	}

	if (wifi_mesh_start(config, wifi_dev) != HISI_OK) {
		if (first_iface_flag == WPA_FLAG_OFF)
			wifi_remove_iface(wifi_dev);
		else
			wifi_wpa_stop(HI_WIFI_IFTYPE_MESH_POINT);

		wpa_warning_log0(MSG_INFO, "hi_wpa_mesh_start failed.");
		goto WIFI_MESH_EXIT;
	}
	clr_lock_flag();
	return HISI_OK;

WIFI_MESH_ERROR:
	wal_deinit_drv_wlan_netdev(wifi_dev->ifname);
	hi_free_wifi_dev(wifi_dev);
WIFI_MESH_EXIT:
	clr_lock_flag();
	return HISI_FAIL;
}

int hi_wifi_mesh_stop(void)
{
	int ret;

	if (try_set_lock_flag() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}

	struct hisi_wifi_dev *wifi_dev = hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_MESH_POINT);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_mesh_stop: get wifi dev failed");
		goto WIFI_MESH_STOP_EXIT;
	}

	if (wpa_get_other_existed_wpa_wifi_dev(wifi_dev->priv) != NULL)
		ret = wifi_remove_iface(wifi_dev);
	else
		ret = wifi_wpa_stop(HI_WIFI_IFTYPE_MESH_POINT);

	if (ret != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_mesh_stop fail.");
		goto WIFI_MESH_STOP_EXIT;
	}
	g_mesh_sta_info = NULL;
	g_mesh_get_sta_flag = WPA_FLAG_OFF;
	g_mesh_flag = WPA_FLAG_OFF;

	if ((memset_s(&g_mesh_opt_set, sizeof(struct wifi_mesh_opt_set), 0, sizeof(struct wifi_mesh_opt_set)) != EOK) ||
	    (memset_s(&g_ap_opt_set, sizeof(struct wifi_ap_opt_set), 0, sizeof(struct wifi_ap_opt_set)) != EOK)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_mesh_stop memset_s failed");
		goto WIFI_MESH_STOP_EXIT;
	}

	clr_lock_flag();
	return HISI_OK;
WIFI_MESH_STOP_EXIT:
	clr_lock_flag();
	return HISI_FAIL;
}

static int wifi_mesh_set_accept(unsigned char enable_connect,
                                enum hisi_mesh_enable_flag_type flag_type)
{
	if (enable_connect > WPA_FLAG_ON) {
		wpa_error_log0(MSG_ERROR, "wifi_mesh_set_accept_internal: invalid param.");
		return HISI_FAIL;
	}

	struct hisi_wifi_dev *wifi_dev = wifi_dev_get(HI_WIFI_IFTYPE_MESH_POINT);
	if (wifi_dev == NULL)
		return HISI_FAIL;

	if (flag_type == HISI_MESH_ENABLE_ACCEPT_PEER)
		g_mesh_reject_peer_flag = !enable_connect;

	if (hisi_mesh_enable_flag(wifi_dev->ifname, flag_type, enable_connect) != HISI_OK)
		return HISI_FAIL;
	return HISI_OK;
}

int hi_wifi_mesh_set_accept_peer(unsigned char enable_peer_connect)
{
	return wifi_mesh_set_accept(enable_peer_connect, HISI_MESH_ENABLE_ACCEPT_PEER);
}

int hi_wifi_mesh_set_accept_sta(unsigned char enable_sta_connect)
{
	return wifi_mesh_set_accept(enable_sta_connect, HISI_MESH_ENABLE_ACCEPT_STA);
}

/*
 * action = 1: connect
 * action = 0: disconnect
 */
static int wifi_mesh_link_manage(const unsigned char *mac, const int len, unsigned char action)
{
	char network_id_txt[WPA_NETWORK_ID_TXT_LEN + 1] = { 0 };
	char addr_txt[HI_WIFI_TXT_ADDR_LEN + 1]         = { 0 };

	if ((mac == NULL) || (addr_precheck(mac) != HISI_OK) || (len != ETH_ALEN)) {
		wpa_error_log0(MSG_ERROR, "wifi_mesh_link_manage: invalid param.");
		return HISI_FAIL;
	}

	struct hisi_wifi_dev *wifi_dev = wifi_dev_get(HI_WIFI_IFTYPE_MESH_POINT);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL))
		return HISI_FAIL;

	struct wpa_supplicant *wpa_s = wifi_dev->priv;

	if ((wpa_s->ifmsh == NULL) || (wpa_s->ifmsh->bss[0] == NULL)) {
		wpa_error_log0(MSG_ERROR, "wifi_mesh_link_manage: mesh not start.");
		return HISI_FAIL;
	}

	if (snprintf_s(addr_txt, sizeof(addr_txt), sizeof(addr_txt) - 1, MACSTR, MAC2STR(mac)) < 0) {
		wpa_error_log0(MSG_ERROR, "wifi_mesh_link_manage: addr_txt snprintf_s faild");
		return HISI_FAIL;
	}

	if (action == 1) { /* 1: connect */
		if (snprintf_s(network_id_txt, sizeof(network_id_txt), sizeof(network_id_txt) - 1, "%d", wifi_dev->network_id) < 0) {
			wpa_error_log0(MSG_ERROR, "wifi_mesh_link_manage: network_id_txt snprintf_s faild");
			return HISI_FAIL;
		}

		wpa_cli_set_network(wpa_s, network_id_txt, "bssid", addr_txt); /* set bssid */
		wpa_cli_join_mesh(wpa_s); /* join mesh */
	} else {
		(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_MESH_DEAUTH_FLAG);
		wpa_cli_mesh_deauth(wpa_s, addr_txt); /* leave mesh */

		wpa_error_log0(MSG_ERROR, "LOS_EventRead WPA_EVENT_MESH_DEAUTH_FLAG");
		unsigned int ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_MESH_DEAUTH_FLAG,
		                                     LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
		if (ret_val != WPA_EVENT_MESH_DEAUTH_OK) {
			wpa_error_log1(MSG_ERROR, "LOS_EventRead WPA_EVENT_MESH_DEAUTH_FLAG failed ret_val = %x", ret_val);
			return HISI_FAIL;
		}
	}
	return HISI_OK;
}

int hi_wifi_mesh_connect(const unsigned char *mac, const int len)
{
	return wifi_mesh_link_manage(mac, len, 1); /* 1: connect */
}

int hi_wifi_mesh_disconnect(const unsigned char *mac, unsigned char len)
{
	return wifi_mesh_link_manage(mac, len, 0); /* 0: disconnect */
}

int hi_wifi_mesh_get_connected_peer(hi_wifi_mesh_peer_info *peer_list, unsigned int *peer_num)
{
	unsigned int ret_val;

	if ((peer_list == NULL) || (peer_num == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_mesh_get_connected_peer: invalid params.");
		return HISI_FAIL;
	}

	struct hisi_wifi_dev *wifi_dev = wifi_dev_get(HI_WIFI_IFTYPE_MESH_POINT);
	if (wifi_dev == NULL)
		return HISI_FAIL;

	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(wifi_dev->priv);
	if ((wpa_s == NULL) || (wpa_s->ifmsh == NULL) || (g_mesh_get_sta_flag == WPA_FLAG_ON)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_mesh_get_connected_peer: mesh may not start or busy.");
		return HISI_FAIL;
	}

	if (memset_s(peer_list, sizeof(hi_wifi_mesh_peer_info) * (*peer_num), 0,
	    sizeof(hi_wifi_mesh_peer_info) * (*peer_num)) != EOK) {
		return HISI_FAIL;
	}

	g_mesh_sta_info = peer_list;
	g_sta_num = *peer_num;
	g_mesh_get_sta_flag = WPA_FLAG_ON;
	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_SHOW_STA_FLAG);
	(void)wpa_cli_show_sta(wpa_s);

	wpa_error_log0(MSG_ERROR, "LOS_EventRead WPA_EVENT_SHOW_STA_FLAG");
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_SHOW_STA_FLAG, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
	g_mesh_sta_info = NULL;
	g_mesh_get_sta_flag = WPA_FLAG_OFF;
	if ((ret_val == WPA_EVENT_SHOW_STA_ERROR) || (ret_val == LOS_ERRNO_EVENT_READ_TIMEOUT)) {
		wpa_error_log1(MSG_ERROR, "LOS_EventRead WPA_EVENT_SHOW_STA_FLAG failed ret_val = %x", ret_val);
		*peer_num = 0;
		g_sta_num = 0;
		return HISI_FAIL;
	}
	*peer_num = g_sta_num;
	g_sta_num = 0;
	return HISI_OK;
}

#endif /* LOS_CONFIG_MESH */

