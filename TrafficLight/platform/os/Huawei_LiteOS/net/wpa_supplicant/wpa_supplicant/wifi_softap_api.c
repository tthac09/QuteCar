/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: wifi softap APIs
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
#include "config_ssid.h"
#include "drivers/driver_hisi.h"
#include "securec.h"
#include "los_hwi.h"

static int is_hex_char(char ch)
{
	if (((ch >= '0') && (ch <= '9')) ||
		((ch >= 'A') && (ch <= 'F')) ||
		((ch >= 'a') && (ch <= 'f'))) {
		return HISI_OK;
	}
	return HISI_FAIL;
}


static int is_hex_string(const char *data, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++) {
		if (is_hex_char(data[i]) == HISI_FAIL)
			return HISI_FAIL;
	}
	return HISI_OK;
}


static int softap_start_precheck(void)
{
	int i;
	unsigned int int_lock;
	unsigned int ret = (unsigned int)(hi_count_wifi_dev_in_use() >= WPA_DOUBLE_IFACE_WIFI_DEV_NUM);
	int_lock = LOS_IntLock();
	for (i = 0; i < WPA_MAX_WIFI_DEV_NUM; i++) {
		if (g_wifi_dev[i] != NULL)
			ret |= (g_wifi_dev[i]->iftype > HI_WIFI_IFTYPE_STATION);
	}
	ret |= (g_mesh_flag == WPA_FLAG_ON);
	LOS_IntRestore(int_lock);
	return ret ? HISI_FAIL : HISI_OK;
}


static int wifi_softap_set_security(wifi_softap_config *hconf)
{
	if ((hconf->authmode != HI_WIFI_SECURITY_OPEN) &&
	     ((os_strlen((char *)hconf->ssid) > WPA_MAX_ESSID_LEN) ||
	      (os_strlen((char *)hconf->key) < WPA_MIN_KEY_LEN) ||
	      (os_strlen((char *)hconf->key) > WPA_MAX_KEY_LEN))) {
		wpa_error_log0(MSG_ERROR, "Invalid ssid or key length! \n");
		return HISI_FAIL;
	}
	if ((os_strlen((char *)hconf->key) == WPA_MAX_KEY_LEN) &&
		(is_hex_string((char *)hconf->key, WPA_MAX_KEY_LEN) == HISI_FAIL)) {
		wpa_error_log0(MSG_ERROR, "Invalid passphrase character");
		return HISI_FAIL;
	}

	switch (hconf->authmode) {
		case HI_WIFI_SECURITY_OPEN:
			hconf->auth_algs = 0;
			hconf->wpa_key_mgmt = WPA_KEY_MGMT_NONE;
			break;
		case HI_WIFI_SECURITY_WPA2PSK:
			hconf->wpa_key_mgmt = WPA_KEY_MGMT_PSK;
			hconf->wpa = 2; /* 2: WPA2-PSK */
			if (hconf->wpa_pairwise == 0)
				hconf->wpa_pairwise = WPA_CIPHER_CCMP;
			break;
		case HI_WIFI_SECURITY_WPAPSK_WPA2PSK_MIX:
			hconf->wpa_key_mgmt = WPA_KEY_MGMT_PSK;
			hconf->wpa = 3; /* 3: WPA2-PSK|WPA-PSK */
			if (hconf->wpa_pairwise == 0)
				hconf->wpa_pairwise = WPA_CIPHER_CCMP;
			break;
		default:
			wpa_error_log1(MSG_ERROR, "error, Unsupported authmode: %d \n", hconf->authmode);
			hconf->auth_algs = 0;
			hconf->wpa_key_mgmt = WPA_KEY_MGMT_NONE;
			return HISI_FAIL;
	}
	return HISI_OK;
}

static int wifi_softap_set_config(wifi_softap_config *hconf)
{
	char *res = NULL;
	char tmp_ssid[HI_WIFI_MAX_SSID_LEN + 1 + 2] = { 0 }; /* 1: string terminator; 2: two Double quotes */
	size_t len  = 0;
	int ret;
	errno_t rc;

	if (hconf == NULL) {
		wpa_error_log0(MSG_ERROR, " ---> ### hi_softap_set_config error: the hconf is NULL . \n\r");
		return HISI_FAIL;
	}
	if (hconf->channel_num > WPA_24G_CHANNEL_NUMS) {
		wpa_error_log0(MSG_ERROR, "Invalid channel_num! \n");
		return HISI_FAIL;
	}

	ret = snprintf_s(tmp_ssid, sizeof(tmp_ssid), sizeof(tmp_ssid) - 1, "\"%s\"", hconf->ssid);
	if (ret < 0) {
		return HISI_FAIL;
	}
	rc = memset_s(hconf->ssid, sizeof(hconf->ssid), 0, sizeof(hconf->ssid));
	if (rc != EOK) {
		wpa_error_log0(MSG_ERROR, "hi_softap_set_config: memset_s failed");
		return HISI_FAIL;
	}

	res = wpa_config_parse_string(tmp_ssid, &len);
	if ((res != NULL) && (len <= SSID_MAX_LEN)) {
		rc = memcpy_s(hconf->ssid, sizeof(hconf->ssid), res, len);
		if (rc != EOK) {
			wpa_error_log0(MSG_ERROR, "hi_softap_set_config: memcpy_s failed");
			ret = HISI_FAIL;
			goto AP_CONFIG_PRE_CHECK_OUT;
		}
	} else {
		wpa_error_log0(MSG_ERROR, "hi_softap_config_pre_check: wpa_config_parse_string failed");
		ret = HISI_FAIL;
		goto AP_CONFIG_PRE_CHECK_OUT;
	}

	if (wifi_softap_set_security(hconf) != HISI_OK) {
		ret = HISI_FAIL;
		goto AP_CONFIG_PRE_CHECK_OUT;
	}
	ret = HISI_OK;

AP_CONFIG_PRE_CHECK_OUT:
	if (res != NULL) {
		os_free(res);
	}
	return ret;
}

static int wifi_softap_start(wifi_softap_config *hconf, char *ifname, int *len)
{
	struct hisi_wifi_dev *wifi_dev = NULL;
	errno_t rc;

	if (softap_start_precheck() != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "wifi_softap_start: precheck fail!");
		return HISI_FAIL;
	}

	if (wifi_softap_set_config(hconf) != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "wifi_softap_start: set_config fail!");
		return HISI_FAIL;
	}

	(void)memcpy_s(&g_global_conf, sizeof(struct hostapd_conf), hconf, sizeof(struct hostapd_conf));
	wifi_dev = wifi_dev_creat(HI_WIFI_IFTYPE_AP, g_ap_opt_set.hw_mode);
	if (wifi_dev == NULL) {
		wpa_error_log0(MSG_ERROR, "wifi_softap_start: wifi_dev_creat failed.");
		return HISI_FAIL;
	}
	if ((hi_set_wifi_dev(wifi_dev) != HISI_OK) || (*len < wifi_dev->ifname_len + 1))
		goto WIFI_AP_ERROR;
	rc = memcpy_s(ifname, *len, wifi_dev->ifname, wifi_dev->ifname_len);
	if (rc != EOK) {
		wpa_error_log0(MSG_ERROR, "wifi_softap_start: memcpy_s failed");
		goto WIFI_AP_ERROR;
	}
	ifname[wifi_dev->ifname_len] = '\0';
	*len = wifi_dev->ifname_len;
	if (wifi_wpa_start(wifi_dev) == HISI_OK) {
		return HISI_OK;
	}
WIFI_AP_ERROR:
	wal_deinit_drv_wlan_netdev(wifi_dev->ifname);
	hi_free_wifi_dev(wifi_dev);
	return HISI_FAIL;
}

static int wifi_softap_config_check(hi_wifi_softap_config *conf, const char *ifname, const int *len)
{
	if ((conf == NULL) || (ifname == NULL) || (len == NULL)) {
		wpa_error_log0(MSG_ERROR, "wifi_softap_config_check: invalid param");
		return HISI_FAIL;
	}

	if (strlen(conf->ssid) > HI_WIFI_MAX_SSID_LEN) {
		wpa_error_log0(MSG_ERROR, "wifi_softap_config_check: invalid ssid length.");
		return HISI_FAIL;
	}

	if (conf->channel_num > WPA_24G_CHANNEL_NUMS) {
		wpa_error_log0(MSG_ERROR, "wifi_softap_config_check: invalid channel number!.");
		return HISI_FAIL;
	}

	if ((conf->ssid_hidden < 0) || (conf->ssid_hidden > 1)) {
		wpa_error_log0(MSG_ERROR, "wifi_softap_config_check: invalid ignore_broadcast_ssid.");
		return HISI_FAIL;
	}

	if (strlen(conf->key) > WPA_MAX_SSID_KEY_INPUT_LEN) {
		wpa_error_log0(MSG_ERROR, "wifi_softap_config_check: invalid key.");
		return HISI_FAIL;
	}

	if ((conf->authmode < HI_WIFI_SECURITY_OPEN) || (conf->authmode > HI_WIFI_SECURITY_WPAPSK_WPA2PSK_MIX)) {
		wpa_error_log0(MSG_ERROR, "Invalid authmode.");
		return HISI_FAIL;
	}
	return HISI_OK;
}

int hi_wifi_softap_start(hi_wifi_softap_config *conf, char *ifname, int *len)
{
	struct hostapd_conf hapd_conf = { 0 };
	errno_t rc;

	if (wifi_softap_config_check(conf, ifname, len) == HISI_FAIL)
		return HISI_FAIL;

	if (try_set_lock_flag() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_softap_start: wifi dev start or stop is running.");
		return HISI_FAIL;
	}

	rc = strcpy_s(hapd_conf.driver, MAX_DRIVER_NAME_LEN, "hisi");
	if (rc != EOK)
		goto WIFI_AP_FAIL;

	hapd_conf.channel_num = conf->channel_num;
	rc = memcpy_s(hapd_conf.ssid, sizeof(hapd_conf.ssid), conf->ssid, strlen(conf->ssid) + 1);
	if (rc != EOK)
		goto WIFI_AP_FAIL;
	hapd_conf.ignore_broadcast_ssid = conf->ssid_hidden;
	hapd_conf.authmode = conf->authmode;

	if ((conf->authmode != HI_WIFI_SECURITY_OPEN) && (strlen(conf->key) >= WPA_MIN_KEY_LEN)) {
		rc = memcpy_s((char *)hapd_conf.key, sizeof(hapd_conf.key), conf->key, strlen(conf->key) + 1);
		if (rc != EOK)
			goto WIFI_AP_FAIL;
	}

	if ((hapd_conf.authmode == HI_WIFI_SECURITY_WPA2PSK) ||
	    (hapd_conf.authmode == HI_WIFI_SECURITY_WPAPSK_WPA2PSK_MIX)) { // pairwise
		if (conf->pairwise == HI_WIFI_PAIRWISE_TKIP)
			hapd_conf.wpa_pairwise = WPA_CIPHER_TKIP;
		else if ((conf->pairwise == HI_WIFI_PAIRWISE_AES) || (conf->pairwise == HI_WIFI_PARIWISE_UNKNOWN))
			hapd_conf.wpa_pairwise = WPA_CIPHER_CCMP;
		else if (conf->pairwise == HI_WIFI_PAIRWISE_TKIP_AES_MIX)
			hapd_conf.wpa_pairwise = WPA_CIPHER_TKIP | WPA_CIPHER_CCMP;
		else {
			wpa_error_log0(MSG_ERROR, "hi_wifi_softap_start: invalid encryption.");
			goto WIFI_AP_FAIL;
		}
	}
	if (wifi_softap_start(&hapd_conf, ifname, len) != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_softap_start: softap start failed.");
		goto WIFI_AP_FAIL;
	}
	clr_lock_flag();
	return HISI_OK;
WIFI_AP_FAIL:
	clr_lock_flag();
	return HISI_FAIL;
}

int hi_wifi_softap_stop(void)
{
	errno_t rc;
	int ret;
	unsigned int ret_val;
	struct hisi_wifi_dev *wifi_dev  = NULL;
	char ifname[WIFI_IFNAME_MAX_SIZE + 1] = {0};

	if (try_set_lock_flag() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_softap_start: wifi dev start or stop is running.");
		return HISI_FAIL;
	}

	wifi_dev = hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_AP);
	if (wifi_dev == NULL) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_softap_stop: get wifi dev failed\n");
		goto WIFI_AP_STOP_EXIT;
	}
	g_ap_sta_info = NULL;
	rc = memset_s(&g_ap_opt_set, sizeof(struct wifi_ap_opt_set), 0, sizeof(struct wifi_ap_opt_set));
	if (rc != EOK)
		goto WIFI_AP_STOP_EXIT;
	rc = memcpy_s(ifname, WIFI_IFNAME_MAX_SIZE, wifi_dev->ifname, WIFI_IFNAME_MAX_SIZE);
	if (rc != EOK)
		goto WIFI_AP_STOP_EXIT;
	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_AP_STOP_OK);
	wpa_error_log0(MSG_ERROR, "LOS_EventClear WPA_EVENT_AP_STOP_OK");

	if (wpa_cli_terminate(NULL, ELOOP_TASK_HOSTAPD) != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_softap_stop: wpa_cli_terminate failed");
		goto WIFI_AP_STOP_EXIT;
	}
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_AP_STOP_OK, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, LOS_WAIT_FOREVER);
	if (ret_val == WPA_EVENT_AP_STOP_OK) {
		wpa_error_log0(MSG_ERROR, "LOS_EventRead WPA_EVENT_AP_STOP_OK success");
	} else {
		wpa_error_log1(MSG_ERROR, "LOS_EventRead WPA_EVENT_AP_STOP_OK failed ret_val = %x", ret_val);
	}
	ret = wal_deinit_drv_wlan_netdev(ifname);
	if (ret != HISI_SUCC)
		wpa_error_log0(MSG_ERROR, "hi_wifi_softap_stop: wal_deinit_drv_wlan_netdev failed!");

	clr_lock_flag();
	return ret;
WIFI_AP_STOP_EXIT:
	clr_lock_flag();
	return HISI_FAIL;
}

int hi_wifi_softap_deauth_sta(const unsigned char *addr, unsigned char addr_len)
{
	struct hostapd_data *hapd      = NULL;
	struct hisi_wifi_dev *wifi_dev = NULL;
	char addr_txt[HI_WIFI_TXT_ADDR_LEN + 1] = { 0 };
	int ret;
	unsigned int ret_val;

	if ((addr == NULL) || (addr_precheck(addr) != HISI_OK) || (addr_len != ETH_ALEN)) {
		wpa_error_log0(MSG_ERROR, "Invalid addr.");
		return HISI_FAIL;
	}

	wifi_dev = wifi_dev_get(HI_WIFI_IFTYPE_AP);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_softap_deauth_sta: get wifi dev failed.");
		return HISI_FAIL;
	}

	hapd = wifi_dev->priv;
	if (hapd == NULL) {
		wpa_error_log0(MSG_ERROR, "hapd get fail.");
		return HISI_FAIL;
	}
	ret = snprintf_s(addr_txt, sizeof(addr_txt), sizeof(addr_txt) - 1, MACSTR, MAC2STR(addr));
	if (ret < 0) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_mesh_connect snprintf_s faild");
		return HISI_FAIL;
	}
	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_AP_DEAUTH_FLAG);
	wpa_cli_ap_deauth((struct wpa_supplicant *)hapd, addr_txt);
	wpa_error_log0(MSG_ERROR, "LOS_EventRead WPA_EVENT_AP_DEAUTH_FLAG");
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_AP_DEAUTH_FLAG, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
	if (ret_val != WPA_EVENT_AP_DEAUTH_OK) {
		wpa_error_log1(MSG_ERROR, "LOS_EventRead WPA_EVENT_AP_DEAUTH_FLAG failed ret_val = %x", ret_val);
		return HISI_FAIL;
	}
	return HISI_OK;
}

static int wifi_softap_set_cond_check(void)
{
	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}
	if (is_ap_mesh_or_p2p_on() == HISI_OK) {
		wpa_warning_log0(MSG_INFO, "ap or mesh is already in progress, set ap config failed.");
		return HISI_FAIL;
	}

	return HISI_OK;
}

int hi_wifi_softap_set_protocol_mode(hi_wifi_protocol_mode hw_mode)
{
	if (wifi_softap_set_cond_check() == HISI_FAIL)
		return HISI_FAIL;

	if ((hw_mode < HI_WIFI_PHY_MODE_11BGN) || (hw_mode >= HI_WIFI_PHY_MODE_BUTT)) {
		wpa_error_log0(MSG_ERROR, "physical mode value is error.");
		return HISI_FAIL;
	}
	g_ap_opt_set.hw_mode = hw_mode;
	return HISI_OK;
}

int hi_wifi_softap_set_shortgi(int flag)
{
	if (wifi_softap_set_cond_check() == HISI_FAIL)
		return HISI_FAIL;

	if ((flag < WPA_FLAG_OFF) || (flag > WPA_FLAG_ON)) {
		wpa_error_log0(MSG_ERROR, "input flag error.");
		return HISI_FAIL;
	}
	g_ap_opt_set.short_gi_off = !flag;
	return HISI_OK;
}

int hi_wifi_softap_set_beacon_period(int beacon_period)
{
	if (wifi_softap_set_cond_check() == HISI_FAIL)
		return HISI_FAIL;

	if ((beacon_period < WPA_AP_MIN_BEACON) || (beacon_period > WPA_AP_MAX_BEACON)) {
		wpa_error_log2(MSG_ERROR, "beacon value must be %d ~ %d.", WPA_AP_MIN_BEACON, WPA_AP_MAX_BEACON);
		return HISI_FAIL;
	}
	g_ap_opt_set.beacon_period = beacon_period;
	return HISI_OK;
}

int hi_wifi_softap_set_dtim_period(int dtim_period)
{
	if (wifi_softap_set_cond_check() == HISI_FAIL)
		return HISI_FAIL;

	if ((dtim_period > WPA_AP_MAX_DTIM) || (dtim_period < WPA_AP_MIN_DTIM)) {
		wpa_error_log2(MSG_ERROR, "dtim_period must be %d ~ %d.", WPA_AP_MIN_DTIM, WPA_AP_MAX_DTIM);
		return HISI_FAIL;
	}
	g_ap_opt_set.dtim_period = dtim_period;
	return HISI_OK;
}

int hi_wifi_softap_set_group_rekey(int wifi_group_rekey)
{
	if (wifi_softap_set_cond_check() == HISI_FAIL)
		return HISI_FAIL;

	if ((wifi_group_rekey > WPA_MAX_REKEY_TIME) || (wifi_group_rekey < WPA_MIN_REKEY_TIME)) {
		wpa_error_log2(MSG_ERROR, "group_rekey must be %d ~ %d.", WPA_MIN_REKEY_TIME, WPA_MAX_REKEY_TIME);
		return HISI_FAIL;
	}
	g_ap_opt_set.wpa_group_rekey = wifi_group_rekey;
	return HISI_OK;
}

hi_wifi_protocol_mode hi_wifi_softap_get_protocol_mode(void)
{
	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}
	wpa_error_log1(MSG_ERROR, "softap phymode:%d.", g_ap_opt_set.hw_mode);
	return g_ap_opt_set.hw_mode;
}

int hi_wifi_softap_get_connected_sta(hi_wifi_ap_sta_info *sta_list, unsigned int *sta_num)
{
	unsigned int ret_val;

	if ((sta_list == NULL) || (sta_num == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_softap_get_connected_sta: input params error.");
		return HISI_FAIL;
	}

	struct hisi_wifi_dev *wifi_dev = wifi_dev_get(HI_WIFI_IFTYPE_AP);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL) || (g_ap_sta_info != NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_softap_get_connected_sta: softap may not start or busy.");
		return HISI_FAIL;
	}

	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(wifi_dev->priv);

	if (memset_s(sta_list, sizeof(hi_wifi_ap_sta_info) * (*sta_num), 0,
	    sizeof(hi_wifi_ap_sta_info) * (*sta_num)) != EOK)
		return HISI_FAIL;

	g_ap_sta_info = sta_list;
	g_sta_num = *sta_num;
	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_SHOW_STA_FLAG);
	(void)wpa_cli_show_sta(wpa_s);
	wpa_error_log0(MSG_ERROR, "LOS_EventRead WPA_EVENT_SHOW_STA_FLAG");
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_SHOW_STA_FLAG, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
	g_ap_sta_info = NULL;
	if ((ret_val == WPA_EVENT_SHOW_STA_ERROR) || (ret_val == LOS_ERRNO_EVENT_READ_TIMEOUT)) {
		wpa_error_log1(MSG_ERROR, "LOS_EventRead WPA_EVENT_SHOW_STA_FLAG ret_val = %x", ret_val);
		*sta_num = 0;
		g_sta_num = 0;
		return HISI_FAIL;
	}
	*sta_num = g_sta_num;
	g_sta_num = 0;
	return HISI_OK;
}

