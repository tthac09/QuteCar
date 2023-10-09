/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: wifi APIs
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
#ifdef CONFIG_WPS
#include "wps/wps.h"
#endif
#include "securec.h"
#include "los_hwi.h"
#include "los_task.h"

#define WPA_TASK_STACK_SIZE          0x1800UL
#define WPA_TASK_PRIO_NUM            4
#define WPA_CONF_LEN                 64
#define WPA_24G_CHANNEL_NUMS         14
#define WPA_REMOVE_IFACE_WAIT_TIME   60

static BOOL g_wpa_event_inited_flag;
static UINT32 g_wpataskid;
struct hisi_wifi_dev* g_wifi_dev[WPA_MAX_WIFI_DEV_NUM] = { NULL };
struct wifi_ap_opt_set g_ap_opt_set                    = { 0 };
struct wifi_sta_opt_set g_sta_opt_set                  = { 0 };
struct wifi_reconnect_set g_reconnect_set              = { 0 };
struct hisi_scan_record g_scan_record                  = { 0 };
char *g_scan_result_buf                                = NULL;
size_t g_result_len                                    = 0;
int g_mesh_sta_flag                                    = 0;
int g_fast_connect_flag                                = 0;
int g_fast_connect_scan_flag                           = 0;
int g_connecting_flag                                  = 0;
int g_usr_scanning_flag                                = 0;
int g_mesh_flag                                        = 0;
int g_scan_flag                                        = WPA_FLAG_OFF;
static unsigned int g_lock_flag                        = WPA_FLAG_OFF;
unsigned char g_quick_conn_psk[HI_WIFI_STA_PSK_LEN]    = { 0 };
unsigned int g_quick_conn_psk_flag                     = WPA_FLAG_OFF;
hi_wifi_status *g_sta_status                           = NULL;
hi_wifi_ap_sta_info *g_ap_sta_info                     = NULL;
unsigned int g_sta_num                                 = 0;
wpa_rm_network g_wpa_rm_network                        = 0;
#ifdef LOS_CONFIG_MESH
struct wifi_mesh_opt_set g_mesh_opt_set                = { 0 };
hi_wifi_mesh_peer_info *g_mesh_sta_info                = NULL;
unsigned int g_mesh_get_sta_flag                       = WPA_FLAG_OFF;
#endif /* LOS_CONFIG_MESH */
/* call back configuration */
hi_wifi_event_cb g_wpa_event_cb                        = 0;
unsigned int g_wpa_event_taskid                        = 0;
unsigned char g_direct_cb                              = 0; /* 0:create new task call cb, 1:direct call cb */
unsigned char g_cb_task_prio                           = 20; /* callback task priority */
unsigned short g_cb_stack_size                         = 0x800; /* callback task stack size 2k */

/* forward declaration */
static int try_set_lock_flag(void)
{
	unsigned int int_lock;
	int ret = HISI_OK;
	int_lock = LOS_IntLock();
	if (g_lock_flag == WPA_FLAG_ON)
		ret = HISI_FAIL;
	else
		g_lock_flag = WPA_FLAG_ON;
	LOS_IntRestore(int_lock);
	return ret;
}

static int is_lock_flag_off(void)
{
	unsigned int int_lock;
	int ret = HISI_OK;
	int_lock = LOS_IntLock();
	if (g_lock_flag == WPA_FLAG_ON)
		ret = HISI_FAIL;
	LOS_IntRestore(int_lock);
	return ret;
}

static void clr_lock_flag(void)
{
	unsigned int int_lock = LOS_IntLock();
	g_lock_flag = WPA_FLAG_OFF;
	LOS_IntRestore(int_lock);
}

static int chan_to_freq(unsigned char chan)
{
	if (chan == 0)
		return chan;
	if ((chan < 1) || (chan > 14)) { /* 1: channel 1; 14: channel 14 */
		wpa_error_log1(MSG_ERROR, "warning : bad channel number: %d \n", chan);
		return HISI_FAIL;
	}
	if (chan == 14)
		return 2414 + 5 * chan;
	return 2407 + 5 * chan;
}

static int addr_precheck(const unsigned char *addr)
{
	if (is_zero_ether_addr(addr) || is_broadcast_ether_addr(addr)) {
		wpa_error_log0(MSG_ERROR, "bad addr");
		return HISI_FAIL;
	}
	return HISI_OK;
}

static int sta_precheck(void)
{
	int ret = (hi_count_wifi_dev_in_use() >= WPA_DOUBLE_IFACE_WIFI_DEV_NUM) &&
			  (hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_P2P_DEVICE) == NULL);
	for (int i = 0; i < WPA_MAX_WIFI_DEV_NUM; i++) {
		if (g_wifi_dev[i] != NULL)
			ret |= ((g_wifi_dev[i]->iftype == HI_WIFI_IFTYPE_STATION));
	}
	if (ret)
		return HISI_FAIL;
	return HISI_OK;
}

static int is_sta_on(void)
{
	if (hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_STATION) == NULL)
		return HISI_FAIL;

	return HISI_OK;
}

static int is_ap_mesh_or_p2p_on(void)
{
	if ((hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_AP) != NULL) ||
	    (hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_P2P_DEVICE) != NULL) ||
	    (hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_MESH_POINT) != NULL))
		return HISI_OK;

	return HISI_FAIL;
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

int hi_count_wifi_dev_in_use(void)
{
	int i;
	int count = 0;
	unsigned int int_lock = LOS_IntLock();
	for (i = 0; i < WPA_MAX_WIFI_DEV_NUM; i++) {
		if (g_wifi_dev[i] != NULL)
			count++;
	}
	LOS_IntRestore(int_lock);
	return count;
}

struct hisi_wifi_dev * hi_get_wifi_dev_by_name(const char *ifname)
{
	int i;
	if (ifname == NULL)
		return NULL;
	LOS_TaskLock();
	for (i = 0; i < WPA_MAX_WIFI_DEV_NUM; i++) {
		if ((g_wifi_dev[i] != NULL) && (strcmp(g_wifi_dev[i]->ifname, ifname) == 0)) {
			g_wifi_dev[i]->network_id = i;
			LOS_TaskUnlock();
			return g_wifi_dev[i];
		}
	}
	LOS_TaskUnlock();
	return NULL;
}

struct hisi_wifi_dev * hi_get_wifi_dev_by_iftype(hi_wifi_iftype iftype)
{
	int i;
	if (iftype >= HI_WIFI_IFTYPES_BUTT)
		return NULL;

	LOS_TaskLock();
	for (i = 0; i < WPA_MAX_WIFI_DEV_NUM; i++) {
		if ((g_wifi_dev[i] != NULL) && (g_wifi_dev[i]->iftype == iftype)) {
			LOS_TaskUnlock();
			return g_wifi_dev[i];
		}
	}
	LOS_TaskUnlock();
	return NULL;
}

struct hisi_wifi_dev * hi_get_wifi_dev_by_priv(const void *ctx)
{
	int i;
	if (ctx == NULL)
		return NULL;
	LOS_TaskLock();

	for (i = 0; i < WPA_MAX_WIFI_DEV_NUM; i++) {
		if ((g_wifi_dev[i] != NULL) && (g_wifi_dev[i]->priv == ctx)) {
			LOS_TaskUnlock();
			return g_wifi_dev[i];
		}
	}
	LOS_TaskUnlock();
	return NULL;
}

struct hisi_wifi_dev * wpa_get_other_existed_wpa_wifi_dev(const void *priv)
{
	int i;
	LOS_TaskLock();
	for (i = 0; i < WPA_MAX_WIFI_DEV_NUM; i++) {
		if ((g_wifi_dev[i] != NULL) && (g_wifi_dev[i]->priv != priv) &&
			((g_wifi_dev[i]->iftype == HI_WIFI_IFTYPE_STATION) ||
			 (g_wifi_dev[i]->iftype == HI_WIFI_IFTYPE_MESH_POINT) ||
			 (g_wifi_dev[i]->iftype == HI_WIFI_IFTYPE_P2P_DEVICE))) {
			LOS_TaskUnlock();
			return g_wifi_dev[i];
		}
	}
	LOS_TaskUnlock();
	return NULL;
}

static struct hisi_wifi_dev * wifi_dev_get(hi_wifi_iftype iftype)
{
	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi_dev_get: wifi dev start or stop is running.");
		return NULL;
	}

	struct hisi_wifi_dev *wifi_dev = hi_get_wifi_dev_by_iftype(iftype);
	if (wifi_dev == NULL) {
		wpa_error_log1(MSG_ERROR, "wifi_dev_get: get %d failed.", iftype);
		return NULL;
	}

	return wifi_dev;
}

void hi_free_wifi_dev(struct hisi_wifi_dev *wifi_dev)
{
	if (wifi_dev == NULL)
		return;

	wpa_error_log1(MSG_ERROR, "hi_free_wifi_dev enter, iftype = %d", wifi_dev->iftype);
	LOS_TaskLock();
	for (int i = 0; i < WPA_MAX_WIFI_DEV_NUM; i++) {
		if (g_wifi_dev[i] == wifi_dev) {
			g_wifi_dev[i] = NULL;
			break;
		}
	}
	(VOID)memset_s(wifi_dev, sizeof(struct hisi_wifi_dev), 0, sizeof(struct hisi_wifi_dev));
	os_free(wifi_dev);
	LOS_TaskUnlock();
}

static struct hisi_wifi_dev * wifi_dev_creat(hi_wifi_iftype iftype, hi_wifi_protocol_mode phy_mode)
{
	int status;
	struct hisi_wifi_dev *wifi_dev = NULL;
	wifi_dev = (struct hisi_wifi_dev *)os_zalloc(sizeof(struct hisi_wifi_dev));
	if (wifi_dev == NULL) {
		wpa_error_log0(MSG_ERROR, "wifi_dev malloc err.");
		return NULL;
	}
	wifi_dev->ifname_len = sizeof(wifi_dev->ifname);
	status = wal_init_drv_wlan_netdev(iftype, phy_mode, wifi_dev->ifname, &(wifi_dev->ifname_len));
	if (status != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "wpa_dev_creat: wal_init_drv_wlan_netdev failed. \n\n\r!");
		hi_free_wifi_dev(wifi_dev);
		return NULL;
	}
	if ((wifi_dev->ifname_len <= 0) ||
		(wifi_dev->ifname_len >= (int)sizeof(wifi_dev->ifname)) ||
		(wifi_dev->ifname_len != (int)strlen(wifi_dev->ifname))) {
		wpa_error_log0(MSG_ERROR, " wpa_dev_creat : Invalid iface name. \n\n\r");
		goto WIFI_DEV_CREAT_ERROR;
	}
	wifi_dev->iftype = iftype;
	wpa_warning_buf(MSG_INFO, "wpa_dev_creat: ifname:%s", wifi_dev->ifname, strlen(wifi_dev->ifname));
	wpa_warning_log2(MSG_INFO, "wpa_dev_creat: len:%d, iftype: %d\n", wifi_dev->ifname_len, wifi_dev->iftype);
	return wifi_dev;

WIFI_DEV_CREAT_ERROR:
	wal_deinit_drv_wlan_netdev(wifi_dev->ifname);
	hi_free_wifi_dev(wifi_dev);
	return NULL;
}

static int hi_set_wifi_dev(struct hisi_wifi_dev *wifi_dev)
{
	unsigned int int_lock = LOS_IntLock();
	for (int i = 0; i < WPA_MAX_WIFI_DEV_NUM; i++) {
		if (g_wifi_dev[i] == NULL) {
			g_wifi_dev[i] = wifi_dev;
			g_wifi_dev[i]->network_id = i;
			LOS_IntRestore(int_lock);
			return HISI_OK;
		}
	}
	LOS_IntRestore(int_lock);
	return HISI_FAIL;
}

int hi_wpa_ssid_config_set(struct wpa_ssid *ssid, const char *name, const char *value)
{
	errno_t rc;
	u8 *ssid_txt = NULL;
	if ((ssid == NULL) || (name == NULL) || (value == NULL) || (strlen(value) > HI_WIFI_MAX_SSID_LEN))
		return HISI_FAIL;
	if (((ssid->ssid) != NULL) && (strlen((char *) (ssid->ssid)) == strlen(value)) &&
	    (os_memcmp(ssid->ssid, value, strlen(value)) == 0)) {
		return HISI_FAIL;
	}
	str_clear_free((char *) (ssid->ssid));
	ssid_txt = os_zalloc(HI_WIFI_MAX_SSID_LEN + 1);
	if (ssid_txt == NULL)
		return HISI_FAIL;
	rc = memcpy_s(ssid_txt, HI_WIFI_MAX_SSID_LEN + 1, value, strlen(value));
	if (rc != EOK) {
		os_free(ssid_txt);
		wpa_error_log1(MSG_ERROR, "hi_wpa_ssid_config_set memcpy_s failed(%d).", rc);
		return HISI_FAIL;
	}
	ssid->ssid = ssid_txt;
	ssid->ssid_len = strlen((char *)ssid_txt);
	return HISI_OK;
}

static int wifi_sta_psk_init(char *param, unsigned int param_len,
                             unsigned int key_len, const struct wpa_assoc_request *assoc)
{
	int ret;
	if ((param == NULL) || (param_len == 0) || (assoc == NULL))
		return HISI_FAIL;
#ifdef CONFIG_DRIVER_HISILICON
	if (key_len < WPA_MAX_KEY_LEN) {
		ret = snprintf_s(param, param_len, param_len - 1, "\"%s\"", assoc->key);
		if (ret < 0)
			return HISI_FAIL;
	} else {
		if (memcpy_s(param, WPA_MAX_SSID_KEY_INPUT_LEN, assoc->key, key_len) != EOK)
			return HISI_FAIL;
	}
#else
	ret  = snprintf_s(param, param_len, param_len - 1, "\"%s\"", assoc->key);
	if (ret < 0)
		return HISI_FAIL;
#endif
	return HISI_OK;
}

static int wifi_sta_set_network_psk(struct wpa_supplicant *wpa_s, const char* id, struct wpa_assoc_request *assoc)
{
	char param[WPA_MAX_SSID_KEY_INPUT_LEN + 4 + 1] = { 0 }; // add \"\" (length 4) for param
	unsigned int key_len = os_strlen(assoc->key);

	if (assoc->auth != HI_WIFI_SECURITY_OPEN) { /* set security type */
		switch (assoc->auth) {
			case HI_WIFI_SECURITY_WEP:
				wpa_cli_configure_wep(wpa_s, id, assoc);
				break;
			case HI_WIFI_SECURITY_WPA2PSK:
				/* fall-through */
			case HI_WIFI_SECURITY_WPAPSK_WPA2PSK_MIX:
				if (g_sta_opt_set.pmf >= HI_WIFI_MGMT_FRAME_PROTECTION_OPTIONAL)
					wpa_cli_set_network(wpa_s, id, "key_mgmt", "WPA-PSK WPA-PSK-SHA256");
				else
					wpa_cli_set_network(wpa_s, id, "key_mgmt", "WPA-PSK");

				if (wifi_sta_psk_init(param, sizeof(param), key_len, assoc) != HISI_OK)
					return HISI_FAIL;

				wpa_cli_set_network(wpa_s, id, "psk", param);
				break;
			default:
				return HISI_FAIL;
		}
	} else {
		wpa_cli_set_network(wpa_s, id, "auth_alg", "OPEN");
		wpa_cli_set_network(wpa_s, id, "key_mgmt", "NONE");
	}

	if (assoc->wpa_pairwise == HI_WIFI_PAIRWISE_TKIP)
		wpa_cli_set_network(wpa_s, id, "pairwise", "TKIP");
	else if (assoc->wpa_pairwise == HI_WIFI_PAIRWISE_AES)
		wpa_cli_set_network(wpa_s, id, "pairwise", "CCMP");

	return HISI_OK;
}

int hi_freq_to_channel(int freq, unsigned int *channel)
{
	unsigned char tmp_channel = 0;
	if (channel == NULL)
		return HISI_FAIL;

	if (ieee80211_freq_to_chan(freq, &tmp_channel) == NUM_HOSTAPD_MODES)
		return HISI_FAIL;

	*channel = tmp_channel;
	return HISI_OK;
}

static int wifi_scan_ssid_set(struct wpa_scan_params *params, char *buf, int bufflen)
{
	int ret;
	if (params->ssid_len < 0) {
		wpa_error_log0(MSG_ERROR, " ssid scan len error .\n");
		return HISI_FAIL;
	}
	if (params->ssid_len == 0)
		return HISI_OK;

	ret = snprintf_s(buf, bufflen, bufflen - 1, "ssid %s", params->ssid);
	if (ret < 0)
		return HISI_FAIL;

	ret = snprintf_s(g_scan_record.ssid, sizeof(g_scan_record.ssid), sizeof(params->ssid) - 1, "%s", params->ssid);
	if (ret < 0)
		return HISI_FAIL;

	return HISI_OK;
}

static int wifi_scan_param_handle(const struct wpa_scan_params *params, char *addr_txt, int addr_len,
                                  char *freq_buff, int freq_len)
{
	errno_t rc;
	int freq;
	int ret;
	if ((params == NULL) || (addr_txt == NULL) || (freq_buff == NULL)) {
		wpa_error_log0(MSG_ERROR, "wpa_pre_prarms_handle: input params is NULL\n");
		return HISI_FAIL;
	}
	rc = memset_s(&g_scan_record, sizeof(g_scan_record), 0, sizeof(g_scan_record));
	if (rc != EOK) {
		wpa_error_log0(MSG_ERROR, "wpa_pre_prarms_handle memset_s failed");
		return HISI_FAIL;
	}
	if (params->flag == HISI_BSSID_SCAN) {
		ret = snprintf_s(addr_txt, addr_len, addr_len - 1, MACSTR, MAC2STR(params->bssid));
		if (ret < 0) {
			wpa_error_log0(MSG_ERROR, "wpa_pre_prarms_handle snprintf_s faild");
			return HISI_FAIL;
		}
	}
	g_scan_record.flag = params->flag;
	if (params->flag == HISI_PREFIX_SSID_SCAN)
		g_ssid_prefix_flag = WPA_FLAG_ON;
	// freq scan
	freq = chan_to_freq(params->channel);
	if (freq > 0) {
		ret = snprintf_s(freq_buff, freq_len, freq_len - 1, "freq=%d", freq);
		if (ret < 0) {
			wpa_error_log0(MSG_ERROR, "wpa_pre_prarms_handle snprintf_s faild");
			return HISI_FAIL;
		}
		g_scan_record.channel = params->channel;
	} else if (freq != 0) {
		wpa_error_log0(MSG_ERROR, "wpa_pre_prarms_handle invalid .\n");
		return HISI_FAIL;
	}
	return HISI_OK;
}

static int wifi_scan_buffer_process(const char *freq_buff, const char *ssid_buff,
                                    const char *bssid_buff, hi_wifi_iftype iftype)
{
	int sum;
	int ret                            = HISI_OK;
	unsigned int ret_val               = HISI_OK;
	char buf[WPA_EXTERNED_SSID_LEN]    = { 0 };
	struct wpa_supplicant *wpa_s       = NULL;
	struct hisi_wifi_dev *wifi_dev = hi_get_wifi_dev_by_iftype(iftype);
	if ((wifi_dev == NULL) || ((wifi_dev)->priv == NULL)) {
		wpa_error_log0(MSG_ERROR, "wpa_buffer_handle: get wifi dev failed");
		return HISI_FAIL;
	}
	wpa_s = (struct wpa_supplicant *)(wifi_dev->priv);
	if (freq_buff[0] != '\0') {
		ret = snprintf_s(buf, sizeof(buf), strlen(freq_buff) + 1, "%s ", freq_buff);
		if (ret < 0) {
			wpa_error_log0(MSG_ERROR, "wpa_buffer_handle: freq_buff snprintf_s faild");
			return HISI_FAIL;
		}
	}
	sum = ret;
	if (bssid_buff[0] != '\0') {
		ret = snprintf_s(buf + sum, sizeof(buf) - sum, strlen(bssid_buff) + 1, "%s ", bssid_buff);
		if (ret < 0) {
			wpa_error_log0(MSG_ERROR, "wpa_buffer_handle: bssid_buff snprintf_s faild");
			return HISI_FAIL;
		}
	}
	sum += ret;
	if (ssid_buff[0] != '\0') {
		ret = snprintf_s(buf + sum, sizeof(buf) - sum, strlen(ssid_buff) + 1, "%s", ssid_buff);
		if (ret < 0) {
			wpa_error_log0(MSG_ERROR, "wpa_buffer_handle: ssid_buff snprintf_s faild");
			return HISI_FAIL;
		}
	}
	g_usr_scanning_flag = WPA_FLAG_ON;
	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_SCAN_OK);
	if (wpa_cli_scan(wpa_s, buf) == HISI_OK) {
		wpa_error_log0(MSG_ERROR, "LOS_EventRead,wait for WPA_EVENT_SCAN_OK");
		ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_SCAN_OK, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
	}
	g_usr_scanning_flag = WPA_FLAG_OFF;
	g_ssid_prefix_flag = WPA_FLAG_OFF;
	if (ret_val != WPA_EVENT_SCAN_OK) {
		wpa_error_log0(MSG_ERROR, " --->warning : wpa scan failed! \n");
		return HISI_FAIL;
	}
	return HISI_OK;
}

static int wifi_wpa_scan(struct wpa_scan_params *params, hi_wifi_iftype iftype)
{
	int ret;
	errno_t rc;
	char freq_buff[WPA_CMD_MIN_SIZE]        = { 0 };
	char ssid_buff[WPA_EXTERNED_SSID_LEN]   = { 0 };
	char bssid_buff[WPA_CMD_BSSID_LEN]      = { 0 };
	char addr_txt[HI_WIFI_TXT_ADDR_LEN + 1] = { 0 };

	wpa_error_log0(MSG_ERROR, " ---> ### hi_wpa_scan enter. \n\n\r");
	if (iftype == HI_WIFI_IFTYPE_AP)
		return HISI_FAIL;

	if (wifi_scan_param_handle(params, addr_txt, HI_WIFI_TXT_ADDR_LEN + 1, freq_buff, WPA_CMD_MIN_SIZE) != HISI_OK)
		return HISI_FAIL;

	ret = wifi_scan_ssid_set(params, ssid_buff, sizeof(ssid_buff));
	if (ret != HISI_OK) {
		wpa_error_log0(MSG_ERROR, " hi_wpa_scan: hi_wifi_wpa_ssid_scan_set failed");
		return HISI_FAIL;
	}
	if (params->flag == HISI_BSSID_SCAN) {
		ret = snprintf_s(bssid_buff, WPA_CMD_BSSID_LEN, WPA_CMD_BSSID_LEN - 1, "bssid=%s", addr_txt);
		if (ret < 0) {
			wpa_error_log0(MSG_ERROR, "hi_wpa_scan snprintf_s faild");
			return HISI_FAIL;
		}
		rc = memcpy_s(g_scan_record.bssid, ETH_ALEN, params->bssid, ETH_ALEN);
		if (rc != EOK)
			return HISI_FAIL;
	}
	if (((strlen(freq_buff) + 1) + (strlen(bssid_buff) + 1) + (strlen(ssid_buff) + 1)) > WPA_EXTERNED_SSID_LEN) {
		wpa_error_log0(MSG_ERROR, " ssid_buff is too long.\n");
		return HISI_FAIL;
	}
	return wifi_scan_buffer_process(freq_buff, ssid_buff, bssid_buff, iftype);
}

static int wifi_scan_params_parse(const hi_wifi_scan_params *sp, struct wpa_scan_params *scan_params)
{
	unsigned int len;
	if ((sp == NULL) || (sp->scan_type == HI_WIFI_BASIC_SCAN)) {
		scan_params->flag = HISI_SCAN;
		wpa_warning_log0(MSG_ERROR, "hi_wifi_sta_scan: basic scan!");
		return HISI_OK;
	}
	switch (sp->scan_type) {
		case HI_WIFI_CHANNEL_SCAN:
			if ((sp->channel == 0) || (sp->channel > WPA_24G_CHANNEL_NUMS)) {
				wpa_error_log0(MSG_ERROR, "Invalid channel_num!");
				return HISI_FAIL;
			}
			scan_params->channel = sp->channel;
			scan_params->flag = HISI_CHANNEL_SCAN;
			break;
		case HI_WIFI_SSID_SCAN:
			/* fall-through */
		case HI_WIFI_SSID_PREFIX_SCAN:
			len = strlen(sp->ssid);
			if ((sp->ssid_len == 0) || (len != sp->ssid_len) || (len > HI_WIFI_MAX_SSID_LEN) ||
				(sp->ssid_len > HI_WIFI_MAX_SSID_LEN)) {
				wpa_error_log0(MSG_ERROR, "invalid scan ssid parameter");
				return HISI_FAIL;
			}
			scan_params->ssid_len = sp->ssid_len;
			if ((memcpy_s(scan_params->ssid, sizeof(scan_params->ssid), sp->ssid, sp->ssid_len + 1)) != EOK) {
				wpa_error_log0(MSG_ERROR, "HI_WIFI_SSID_PREFIX_SCAN wpa_scan_params_parse memcpy_s");
				return HISI_FAIL;
			}
			scan_params->flag = (sp->scan_type == HI_WIFI_SSID_SCAN) ? HISI_SSID_SCAN : HISI_PREFIX_SSID_SCAN;
			break;
		case HI_WIFI_BSSID_SCAN:
			if (addr_precheck(sp->bssid) != HISI_OK) {
				wpa_error_log0(MSG_ERROR, "invalid scan bssid parameter");
				return HISI_FAIL;
			}
			scan_params->flag = HISI_BSSID_SCAN;
			if ((memcpy_s(scan_params->bssid, ETH_ALEN, sp->bssid, ETH_ALEN)) != EOK) {
				wpa_error_log0(MSG_ERROR, "HI_WIFI_BSSID_SCAN wpa_scan_params_parse memcpy_s");
				return HISI_FAIL;
			}
			break;
		default:
			wpa_error_log0(MSG_ERROR, "hi_wifi_sta_scan: Invalid scan_type!");
			return HISI_FAIL;
	}
	return HISI_OK;
}

static int wifi_scan(hi_wifi_iftype iftype, bool is_mesh, const hi_wifi_scan_params *sp)
{
	int ret;
	struct wpa_scan_params scan_params = { 0 };
	wpa_error_log0(MSG_ERROR, " ---> ### wpa_scan enter. \n\n\r");

	if (is_mesh == true) {
		if ((iftype == HI_WIFI_IFTYPE_STATION) && (g_mesh_sta_flag == WPA_FLAG_OFF)) {
			wpa_error_log0(MSG_ERROR, "wpa_scan: mesh sta scan while g_mesh_sta_flag is off");
			return HISI_FAIL;
		}

		if ((iftype == HI_WIFI_IFTYPE_MESH_POINT) && (g_mesh_sta_flag != WPA_FLAG_OFF)) {
			wpa_error_log0(MSG_ERROR, "wpa_scan: mesh scan while g_mesh_sta_flag is on");
			return HISI_FAIL;
		}
	} else if (iftype == HI_WIFI_IFTYPE_STATION) {
			if (g_mesh_sta_flag != WPA_FLAG_OFF) {
				wpa_error_log0(MSG_ERROR, "wpa_scan: sta scan while g_mesh_sta_flag is on.");
				return HISI_FAIL;
			}
	} else {
		wpa_error_log0(MSG_ERROR, "wpa_scan: Invalid scan cmd.");
		return HISI_FAIL;
	}

	if ((wifi_dev_get(iftype) == NULL)) {
		wpa_error_log0(MSG_ERROR, "wpa_scan: wifi dev get fail.");
		return HISI_FAIL;
	}
	if (wifi_scan_params_parse(sp, &scan_params) != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "wpa_scan: scan_params_parse fail.");
		return HISI_FAIL;
	}

	g_scan_flag = WPA_FLAG_ON;
	ret = wifi_wpa_scan(&scan_params, iftype);
	if (ret != HISI_OK) {
		g_scan_flag = WPA_FLAG_OFF;
		wpa_error_log0(MSG_ERROR, "wpa_scan fail.");
		return HISI_FAIL;
	}

	wpa_error_log1(MSG_ERROR, "wpa_scan: scan_params.flag: %d", scan_params.flag);
	return HISI_OK;
}

int hi_wifi_sta_scan(void)
{
	return wifi_scan(HI_WIFI_IFTYPE_STATION, false, NULL);
}

int hi_wifi_sta_advance_scan(hi_wifi_scan_params *sp)
{
	return wifi_scan(HI_WIFI_IFTYPE_STATION, false, sp);
}

static int wifi_scan_result(hi_wifi_iftype iftype)
{
	struct hisi_wifi_dev *wifi_dev = wifi_dev_get(iftype);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL) ||
	    (wifi_dev->iftype == HI_WIFI_IFTYPE_AP) ||
	    (wifi_dev->iftype >= HI_WIFI_IFTYPE_P2P_CLIENT)) {
		wpa_error_log0(MSG_ERROR, "wifi_scan_result: get wifi dev failed\n");
		return HISI_FAIL;
	}

	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_GET_SCAN_RESULT_FLAG);
	if (wpa_cli_scan_results((struct wpa_supplicant *)(wifi_dev->priv)) != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "wifi_scan_result: wpa_cli_scan_results failed.");
		return HISI_FAIL;
	}

	wpa_error_log0(MSG_ERROR, "wifi_scan_result: wait for WPA_EVENT_GET_SCAN_RESULT_FLAG");
	unsigned int ret_val;
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_GET_SCAN_RESULT_FLAG,
	                        LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
	if (ret_val == WPA_EVENT_GET_SCAN_RESULT_OK) {
		if (g_scan_result_buf == NULL)
			return HISI_FAIL;
	} else if ((ret_val == WPA_EVENT_GET_SCAN_RESULT_ERROR) || (ret_val == LOS_ERRNO_EVENT_READ_TIMEOUT)) {
		wpa_error_log0(MSG_ERROR, "wifi_scan_result: get scan result failed.");
		return HISI_FAIL;
	}
	return HISI_OK;
}

static int wifi_scan_result_bssid_parse(char **starttmp, void *buf, size_t *reply_len)
{
	char *endtmp = NULL;
	char addr_txt[HI_WIFI_TXT_ADDR_LEN + 1] = { 0 };
	hi_wifi_ap_info *ap_list = (hi_wifi_ap_info *)buf;
	endtmp = strchr(*starttmp, '\t');
	if (endtmp == NULL)
		return HISI_FAIL;

	*endtmp = '\0';
	if (strncpy_s(addr_txt, sizeof(addr_txt), *starttmp, HI_WIFI_TXT_ADDR_LEN) != EOK)
		return HISI_FAIL;

	if (hwaddr_aton(addr_txt, ap_list->bssid))
		return HISI_FAIL;

	*reply_len -= endtmp - *starttmp + 1;
	*starttmp = ++endtmp;
	return HISI_OK;
}

static int wifi_scan_result_freq_parse(char **starttmp, void *buf, size_t *reply_len)
{
	char *endtmp = NULL;
	hi_wifi_ap_info *ap_list = (hi_wifi_ap_info *)buf;
	endtmp = strchr(*starttmp, '\t');
	if (endtmp == NULL)
		return HISI_FAIL;

	*endtmp = '\0';
	if (hi_freq_to_channel(atoi(*starttmp), &(ap_list->channel)) != HISI_OK)
		ap_list->channel = 0;

	*reply_len -= endtmp - *starttmp + 1;
	*starttmp = ++endtmp;
	return HISI_OK;
}

static int wifi_scan_result_rssi_parse(char **starttmp, void *buf, size_t *reply_len)
{
	char *endtmp = NULL;
	hi_wifi_ap_info *ap_list = (hi_wifi_ap_info *)buf;

	endtmp = strchr(*starttmp, '\t');
	if (endtmp == NULL)
		return HISI_FAIL;

	*endtmp = '\0';
	if (**starttmp == '-') {
		ap_list->rssi = 0 - atoi(++*starttmp);
		*reply_len -= endtmp - *starttmp + 1 + 1;
		*starttmp = ++endtmp;
	} else {
		ap_list->rssi = atoi(*starttmp);
		*reply_len -= endtmp - *starttmp + 1;
		*starttmp = ++endtmp;
	}
	return HISI_OK;
}

static void wifi_scan_result_base_flag_parse(const char *starttmp, void *buf)
{
	hi_wifi_ap_info *ap_list = (hi_wifi_ap_info *)buf;
	char *str_open      = strstr(starttmp, "OPEN");
	char *str_sae       = strstr(starttmp, "SAE");
	char *str_wpa_psk   = strstr(starttmp, "WPA-PSK");
	char *str_wpa2_psk  = strstr(starttmp, "WPA2-PSK");
	char *str_wpa       = strstr(starttmp, "WPA-EAP");
	char *str_wpa2      = strstr(starttmp, "WPA2-EAP");
	char *str_wep       = strstr(starttmp, "WEP");
	char *str_wps       = strstr(starttmp, "WPS");
	char *str_hisi_mesh = strstr(starttmp, "HISI_MESH");
	char *str_rsn_psk   = strstr(starttmp, "RSN-PSK");

	if (str_sae != NULL)
		ap_list->auth = HI_WIFI_SECURITY_SAE;
	else if ((str_wpa_psk != NULL) && ((str_wpa2_psk != NULL) || (str_rsn_psk != NULL)))
		ap_list->auth = HI_WIFI_SECURITY_WPAPSK_WPA2PSK_MIX;
	else if (str_wpa_psk != NULL)
		ap_list->auth = HI_WIFI_SECURITY_WPAPSK;
	else if ((str_wpa2_psk != NULL) || (str_rsn_psk != NULL))
		ap_list->auth = HI_WIFI_SECURITY_WPA2PSK;
	else if (str_wpa != NULL)
		ap_list->auth = HI_WIFI_SECURITY_WPA;
	else if (str_wpa2 != NULL)
		ap_list->auth = HI_WIFI_SECURITY_WPA2;
	else if (str_wep != NULL)
		ap_list->auth = HI_WIFI_SECURITY_WEP;
	else if (str_open != NULL)
		ap_list->auth = HI_WIFI_SECURITY_OPEN;
	else
		ap_list->auth = HI_WIFI_SECURITY_UNKNOWN;

	if (str_wps != NULL)
		ap_list->wps_flag = WPA_FLAG_ON;

	if (str_hisi_mesh != NULL)
		ap_list->hisi_mesh_flag = WPA_FLAG_ON;
}

static int wifi_scan_result_filter_parse(void *buf)
{
	hi_wifi_ap_info *ap_list = (hi_wifi_ap_info *)buf;
	unsigned char ssid_flag;
	unsigned char chl_flag;
	unsigned char prefix_ssid_flag;
	unsigned char bssid_flag;
	prefix_ssid_flag = (os_strncmp(g_scan_record.ssid, ap_list->ssid, strlen(g_scan_record.ssid)) == 0);
	chl_flag = (g_scan_record.channel == ap_list->channel);
	ssid_flag = (os_strcmp(g_scan_record.ssid, ap_list->ssid) == 0);
	bssid_flag = (os_memcmp(g_scan_record.bssid, ap_list->bssid, sizeof(ap_list->bssid)) == 0);
	return (g_scan_record.flag == HISI_SCAN_UNSPECIFIED) ||
	       (g_scan_record.flag == HISI_SCAN) ||
	       ((g_scan_record.flag == HISI_PREFIX_SSID_SCAN) && prefix_ssid_flag) ||
	       ((g_scan_record.flag == HISI_CHANNEL_SCAN) && chl_flag) ||
	       ((g_scan_record.flag == HISI_BSSID_SCAN) && bssid_flag) ||
	       ((g_scan_record.flag == HISI_SSID_SCAN) && ssid_flag);
}

static int wifi_scan_result_ssid_parse(char **starttmp, void *buf, size_t *reply_len)
{
	char *endtmp = NULL;
	hi_wifi_ap_info *ap_list = (hi_wifi_ap_info *)buf;

	endtmp = strchr(*starttmp, '\n');
	if (endtmp == NULL)
		return HISI_FAIL;

	*endtmp = '\0';
	if (strncpy_s(ap_list->ssid, sizeof(ap_list->ssid), *starttmp, endtmp - *starttmp) != EOK) {
		ap_list->ssid[(sizeof(ap_list->ssid)) - 1] = '\0';
		*reply_len -= endtmp - *starttmp + 1;
		*starttmp = ++endtmp;
		return HISI_FAIL;
	}
	ap_list->ssid[(sizeof(ap_list->ssid)) - 1] = '\0';
	*reply_len -= endtmp - *starttmp + 1;
	*starttmp = ++endtmp;
	return wifi_scan_result_filter_parse(ap_list) ? HISI_OK : HISI_FAIL;
}

/*************************************************************************************
 buf content format:
 bssid / frequency / signal level / flags / ssid
 ac:9e:17:95:a1:90 2462 -21 [WPA2-PSK-CCMP][ESS] ssid-test
 ac:a2:13:99:11:7b 2437 -35 [WPA2-PSK-CCMP][ESS] ssid
 00:21:27:78:68:ec 2412 -37 [WPA-PSK-CCMP+TKIP][WPA2-PSK-CCMP+TKIP-preauth][ESS] abc
 ***************************************************************************************/
static int wifi_scan_results_parse(hi_wifi_ap_info *ap_list, unsigned int *ap_num)
{
	char *starttmp = NULL;
	char *endtmp   = NULL;
	int count = 0;
	size_t reply_len = g_result_len;
	char *buf = g_scan_result_buf;
	int ret = HISI_FAIL;
	if (buf == NULL)
		goto EXIT;
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

int hi_wifi_sta_scan_results(hi_wifi_ap_info *ap_list, unsigned int *ap_num)
{
	if ((ap_list == NULL) || (ap_num == NULL) || (g_mesh_sta_flag == WPA_FLAG_ON))
		return HISI_FAIL;

	if (wifi_dev_get(HI_WIFI_IFTYPE_STATION) == NULL)
		return HISI_FAIL;

	if (wifi_scan_result(HI_WIFI_IFTYPE_STATION) == HISI_OK)
		return wifi_scan_results_parse(ap_list, ap_num);
	else {
		(void)memset_s(&g_scan_record, sizeof(g_scan_record), 0, sizeof(g_scan_record));
		*ap_num = 0;
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_SCAN_RESULT_FREE_OK);
		return HISI_FAIL;
	}
}


int hi_wifi_psk_calc(hi_wifi_sta_psk_config psk_config, unsigned char *get_psk, unsigned int psk_len)
{
	unsigned char psk_calc[HI_WIFI_STA_PSK_LEN] = { 0 };
	struct hisi_wifi_dev *wifi_dev = NULL;

	if ((get_psk == NULL) || (psk_len != HI_WIFI_STA_PSK_LEN))
		return HISI_FAIL;

	if ((strlen((char *)psk_config.ssid) > HI_WIFI_MAX_SSID_LEN) || (strlen((char *)psk_config.ssid) < 1)) {
		wpa_error_log0(MSG_ERROR, "invalid ssid length.");
		return HISI_FAIL;
	}
	if ((strlen(psk_config.key) > HI_WIFI_MAX_KEY_LEN) || (strlen(psk_config.key) < 1)) {
		wpa_error_log0(MSG_ERROR, "invalid key length.");
		return HISI_FAIL;
	}

	wifi_dev = wifi_dev_get(HI_WIFI_IFTYPE_STATION);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_psk_calc: get wifi dev failed\n");
		return HISI_FAIL;
	}
	if (memset_s(get_psk, psk_len, 0, psk_len) != EOK) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_psk_calc: memset_s failed\n");
		return HISI_FAIL;
	}
	if (pbkdf2_sha1(psk_config.key, psk_config.ssid, strlen((char *)psk_config.ssid),
	    WPA_STA_ITERA, psk_calc, WPA_STA_PMK_LEN) != HISI_OK) {
		return HISI_FAIL;
	}
	if (memcpy_s(get_psk, psk_len, psk_calc, HI_WIFI_STA_PSK_LEN) != EOK)
		return HISI_FAIL;

	return HISI_OK;
}

int hi_wifi_psk_calc_and_store(hi_wifi_sta_psk_config psk_config)
{
	if (hi_wifi_psk_calc(psk_config, g_quick_conn_psk, HI_WIFI_STA_PSK_LEN) != HISI_OK)
		return HISI_FAIL;

	return HISI_OK;
}

static int wifi_connect_prepare(struct wpa_assoc_request *assoc,
                                struct wpa_supplicant *wpa_s,
                                const char* network_id_txt,
                                bool is_quick_connect)
{
	char addr_txt[HI_WIFI_TXT_ADDR_LEN + 1] = { 0 };
	char fre_buff[WPA_CMD_MIN_SIZE] = { 0 };

	wpa_cli_disconnect(wpa_s);
	wpa_cli_remove_network(wpa_s, network_id_txt);
	wpa_cli_add_network(wpa_s);

	if (assoc->scan_type & HI_WPA_BIT_SCAN_BSSID) { // set bssid
		if (snprintf_s(addr_txt, sizeof(addr_txt), sizeof(addr_txt) - 1, MACSTR, MAC2STR(assoc->bssid)) < 0) {
			wpa_error_log0(MSG_ERROR, "wifi_connect_prepare addr_txt snprintf_s faild.\n");
			return HISI_FAIL;
		}
		wpa_cli_set_network(wpa_s, network_id_txt, "bssid", addr_txt);
	}

	if (assoc->scan_type & HI_WPA_BIT_SCAN_SSID) { // set ssid
		if (strlen(assoc->ssid) > WPA_MAX_SSID_LEN) {
			wpa_error_log0(MSG_ERROR, "wifi_connect_prepare: ssid len error.\n");
			return HISI_FAIL;
		}
		wpa_cli_set_network(wpa_s, network_id_txt, "ssid", assoc->ssid);
	}

	if (is_quick_connect) { // set frequence
		if (snprintf_s(fre_buff, sizeof(fre_buff), sizeof(fre_buff) - 1, "%d", chan_to_freq(assoc->channel)) < 0) {
			wpa_error_log0(MSG_ERROR, "wifi_connect_prepare: fre_buff snprintf_s faild");
			return HISI_FAIL;
		}
		wpa_cli_set_network(wpa_s, network_id_txt, "frequency", fre_buff);
	}

	wpa_cli_set_network(wpa_s, network_id_txt, "mode", "0"); // set sta mode

	if (wifi_sta_set_network_psk(wpa_s, network_id_txt, assoc) != HISI_OK) { // set security type
		wpa_cli_remove_network(wpa_s, network_id_txt);
		return HISI_FAIL;
	}

	return HISI_OK;
}

static int wifi_connect(struct wpa_assoc_request *assoc, bool is_quick_connect)
{
	struct hisi_wifi_dev *wifi_dev = NULL;
	char network_id_txt[WPA_NETWORK_ID_TXT_LEN + 1] = { 0 };
	int ret;
	unsigned int ret_val = HISI_OK;

	wpa_error_log0(MSG_ERROR, " ---> ### wifi_connect enter. \n\n\r");

	wifi_dev = hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_STATION);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL)) {
		wpa_error_log0(MSG_ERROR, "wifi_connect: get wifi dev failed\n");
		return HISI_FAIL;
	}
	if (assoc->scan_type == HI_WPA_BIT_SCAN_UNKNOW) {
		wpa_error_log0(MSG_ERROR, "wifi_connect: please set ssid or bssid\n");
		return HISI_FAIL;
	}
	ret = snprintf_s(network_id_txt, sizeof(network_id_txt), sizeof(network_id_txt) - 1, "%d", wifi_dev->network_id);
	if (ret < 0) {
		wpa_error_log0(MSG_ERROR, "wifi_connect: network_id_txt snprintf_s failed");
		return HISI_FAIL;
	}
	if (wifi_connect_prepare(assoc, (struct wpa_supplicant *)(wifi_dev->priv), network_id_txt, is_quick_connect) < 0)
		return HISI_FAIL;

	g_fast_connect_flag = is_quick_connect ? WPA_FLAG_ON : WPA_FLAG_OFF;
	g_connecting_flag = WPA_FLAG_ON;
	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_QUICK_CONNECT_FLAG);

	wpa_cli_select_network((struct wpa_supplicant *)(wifi_dev->priv), network_id_txt);
	if (is_quick_connect) {
		wpa_error_log0(MSG_ERROR, "LOS_EventRead,wait for WPA_EVENT_QUICK_CONNECT_FLAG");
		ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_QUICK_CONNECT_FLAG, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
		g_fast_connect_flag = WPA_FLAG_OFF;
		g_connecting_flag = WPA_FLAG_OFF;
		if (ret_val == WPA_EVENT_QUICK_CONNECT_OK) {
			wpa_warning_log0(MSG_INFO, "wifi_fast_connect successful.");
		} else if ((ret_val == WPA_EVENT_QUICK_CONNECT_ERROR) || (ret_val == LOS_ERRNO_EVENT_READ_TIMEOUT)) {
			wpa_error_log0(MSG_ERROR, "wifi_fast_connect fail.");
			return HISI_FAIL;
		}
	}

	return HISI_OK;
}

static int wifi_sta_connect_param_check(const hi_wifi_assoc_request *req)
{
	if (req == NULL) {
		wpa_error_log0(MSG_ERROR, "input param is NULL.");
		return HISI_FAIL;
	}
	if ((req->auth < HI_WIFI_SECURITY_OPEN) || (req->auth > HI_WIFI_SECURITY_SAE)) {
		wpa_error_log0(MSG_ERROR, "invalid auth.");
		return HISI_FAIL;
	}
	if (is_broadcast_ether_addr(req->bssid)) {
		wpa_error_log0(MSG_ERROR, "invalid bssid.");
		return HISI_FAIL;
	}
	if ((req->pairwise < HI_WIFI_PARIWISE_UNKNOWN) || (req->pairwise > HI_WIFI_PAIRWISE_TKIP_AES_MIX)) {
		wpa_error_log0(MSG_ERROR, "invalid pairwise.");
		return HISI_FAIL;
	}
	return HISI_OK;
}

static int wifi_sta_connect(const hi_wifi_assoc_request *req,
                            const hi_wifi_fast_assoc_request *fast_request,
                            bool is_fast_connnect)
{
	int ret;
	struct wpa_assoc_request assoc;

	ret = wifi_sta_connect_param_check(req);
	if (ret != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_connect_precheck failed.");
		return HISI_FAIL;
	}
	assoc.scan_type = HI_WPA_BIT_SCAN_UNKNOW;
	if ((strlen(req->ssid) > 0) && (strlen(req->ssid) <= HI_WIFI_MAX_SSID_LEN)) {
		assoc.scan_type |= HI_WPA_BIT_SCAN_SSID;
		if (memcpy_s(assoc.ssid, sizeof(assoc.ssid), req->ssid, strlen(req->ssid) + 1) != EOK)
			return HISI_FAIL;
	}
	if (!is_zero_ether_addr(req->bssid))  {
		assoc.scan_type |= HI_WPA_BIT_SCAN_BSSID;
		if (memcpy_s(assoc.bssid, sizeof(assoc.bssid), req->bssid, sizeof(req->bssid)) != EOK)
			return HISI_FAIL;
	} else {
		if (memset_s(assoc.bssid, sizeof(assoc.bssid), 0, sizeof(req->bssid)) != EOK)
			return HISI_FAIL;
	}
	if (req->auth != HI_WIFI_SECURITY_OPEN) {
		if ((strlen(req->key) > WPA_MAX_SSID_KEY_INPUT_LEN) || (strlen(req->key) == 0)) {
			wpa_error_log0(MSG_ERROR, "invalid key.length.");
			return HISI_FAIL;
		}
		if (memcpy_s(assoc.key, sizeof(assoc.key), req->key, strlen(req->key) + 1) != EOK)
			return HISI_FAIL;
	} else {
		if (strlen(req->key) != 0) {
			wpa_error_log0(MSG_ERROR, "invalid key.length.");
			return HISI_FAIL;
		}
	}
	assoc.auth = req->auth;
	assoc.wpa_pairwise = req->pairwise;
	if (is_fast_connnect) {
		assoc.channel = fast_request->channel;
	}

	ret = wifi_connect(&assoc, is_fast_connnect);
	if (ret != HISI_OK) {
		wpa_error_log1(MSG_ERROR, "%d:hi_wifi_connect failed.", is_fast_connnect);
		return HISI_FAIL;
	}
	return HISI_OK;
}

int hi_wifi_sta_connect(hi_wifi_assoc_request *req)
{
	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}
	return wifi_sta_connect(req, NULL, false);
}

int hi_wifi_sta_fast_connect(hi_wifi_fast_assoc_request *fast_request)
{
	int ret;
	if (fast_request == NULL) {
		wpa_error_log0(MSG_ERROR, "input param is NULL.");
		return HISI_FAIL;
	}
	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}
	if ((fast_request->channel < 1) || (fast_request->channel > WPA_24G_CHANNEL_NUMS)) {
		wpa_error_log0(MSG_ERROR, "invalid channel.");
		return HISI_FAIL;
	}

	if (fast_request->psk_flag == HI_WIFI_WPA_PSK_USE_OUTER) {
		if (fast_request->psk[0] == '\0') {
			wpa_error_log0(MSG_ERROR, "invalid psk.");
			return HISI_FAIL;
		}
		if (memcpy_s(g_quick_conn_psk, sizeof(g_quick_conn_psk), fast_request->psk, sizeof(fast_request->psk)) != EOK) {
			wpa_error_log0(MSG_ERROR, "hi_wifi_sta_fast_connect memcpy_s failed.");
			return HISI_FAIL;
		}
	}
	g_quick_conn_psk_flag = (fast_request->psk_flag == HI_WIFI_WPA_PSK_NOT_USE) ? WPA_FLAG_OFF : WPA_FLAG_ON;
	ret = wifi_sta_connect(&fast_request->req, fast_request, true);
	if ((fast_request->psk_flag == HI_WIFI_WPA_PSK_USE_OUTER) &&
		(memset_s(g_quick_conn_psk, sizeof(g_quick_conn_psk), 0, sizeof(g_quick_conn_psk)) != EOK)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_fast_connect memset_s failed.");
	}
	g_quick_conn_psk_flag = WPA_FLAG_OFF;
	return ret;
}

int hi_wifi_sta_disconnect(void)
{
	int ret;
	char network_id_txt[WPA_NETWORK_ID_TXT_LEN + 1] = { 0 };

	wpa_error_log0(MSG_ERROR, " ---> ### hi_wifi_sta_disconnect enter. \n\n\r");

	struct hisi_wifi_dev *wifi_dev = wifi_dev_get(HI_WIFI_IFTYPE_STATION);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_disconnect: get wifi dev failed\n");
		return HISI_FAIL;
	}

	ret = wpa_cli_disconnect((struct wpa_supplicant *)(wifi_dev->priv));
	if (ret != HISI_OK)
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_disconnect fail.");

	ret = snprintf_s(network_id_txt, sizeof(network_id_txt), sizeof(network_id_txt) - 1, "%d", wifi_dev->network_id);
	if (ret < 0) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_disconnect snprintf_s faild");
		return HISI_FAIL;
	}
	wpa_cli_remove_network((struct wpa_supplicant *)(wifi_dev->priv), network_id_txt);
	g_connecting_flag = WPA_FLAG_OFF;
	return HISI_OK;
}

int hi_wifi_sta_get_connect_info(hi_wifi_status *connect_status)
{
	struct hisi_wifi_dev *wifi_dev = NULL;
	struct wpa_supplicant *wpa_s   = NULL;
	errno_t rc;
	unsigned int ret_val;

	if (connect_status == NULL) {
		wpa_error_log0(MSG_ERROR, "input param is NULL.");
		return HISI_FAIL;
	}
	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}
	rc = memset_s(connect_status, sizeof(hi_wifi_status), 0, sizeof(hi_wifi_status));
	if (rc != EOK)
		return HISI_FAIL;

	wifi_dev = hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_STATION);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL))
		return HISI_FAIL;

	wpa_s = (struct wpa_supplicant *)(wifi_dev->priv);

	g_sta_status = connect_status;
	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_STA_STATUS_FLAG);
	(void)wpa_cli_sta_status(wpa_s);
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_STA_STATUS_FLAG, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);

	g_sta_status = NULL;
	if (ret_val != WPA_EVENT_STA_STATUS_OK) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_get_connect_info fail.");
		return HISI_FAIL;
	}
	return HISI_OK;
}

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

static unsigned int wifi_wpa_task_create(struct hisi_wifi_dev *wifi_dev)
{
	TSK_INIT_PARAM_S wpa_task;
	wpa_error_log0(MSG_ERROR, "---> hi_wpa_task_create enter.");
	(VOID)memset_s(&wpa_task, sizeof(TSK_INIT_PARAM_S), 0, sizeof(TSK_INIT_PARAM_S));
	wpa_task.pfnTaskEntry = (TSK_ENTRY_FUNC)wpa_supplicant_main_task;
	wpa_task.uwStackSize  = WPA_TASK_STACK_SIZE;
	wpa_task.pcName = "wpa_supplicant";
	wpa_task.usTaskPrio = WPA_TASK_PRIO_NUM;
	wpa_task.auwArgs[0] = (UINT32)(UINTPTR)wifi_dev;
	wpa_task.uwResved   = LOS_TASK_STATUS_DETACHED;
	return LOS_TaskCreate(&g_wpataskid, &wpa_task);
}

static int wifi_wpa_start(struct hisi_wifi_dev *wifi_dev)
{
	unsigned int ret_val;

	/* the caller asure wifi_dev is not NULL */
	wpa_error_log0(MSG_ERROR, "---> wifi_wpa_start enter.");

	unsigned int int_lock = LOS_IntLock();
	if (g_wpa_event_inited_flag == WPA_FLAG_OFF) {
		(void)LOS_EventInit(&g_wpa_event);
		g_wpa_event_inited_flag = WPA_FLAG_ON;
	}
	LOS_IntRestore(int_lock);

	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_WPA_START_FLAG);
	wpa_error_log0(MSG_ERROR, "wifi_wpa_start: wait for WPA_EVENT_WPA_START_FLAG");

	int wifi_dev_count = hi_count_wifi_dev_in_use();
	if (wifi_dev_count < WPA_BASE_WIFI_DEV_NUM) {
		wpa_error_log1(MSG_ERROR, "wifi_wpa_start: wifi_dev_count = %d.\n", wifi_dev_count);
		return HISI_FAIL;
	}

	if (wifi_dev_count == WPA_BASE_WIFI_DEV_NUM) {
		ret_val = wifi_wpa_task_create(wifi_dev);
		if (ret_val != LOS_OK) {
			wpa_error_log1(MSG_ERROR, "wifi_wpa_start: wpa_supplicant task create failed, ret_val = %x.", ret_val);
			return HISI_FAIL;
		}
	} else {
		if (wpa_cli_if_start(NULL, wifi_dev->iftype, wifi_dev->ifname) != HISI_OK) {
			wpa_error_log0(MSG_ERROR, "wifi_wpa_start: if start failed.");
			return HISI_FAIL;
		}
	}

	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_WPA_START_FLAG, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, LOS_WAIT_FOREVER);
	if (ret_val == WPA_EVENT_WPA_START_OK) {
		wpa_error_log0(MSG_ERROR, "wifi_wpa_start: successful.\n");
	} else if (ret_val == WPA_EVENT_WPA_START_ERROR) {
		wpa_error_log1(MSG_ERROR, "wifi_wpa_start: failed, ret: %x\n", ret_val);
		return HISI_FAIL;
	}
	return HISI_OK;
}

static int wifi_wpa_stop(hi_wifi_iftype iftype)
{
	struct hisi_wifi_dev *wifi_dev  = NULL;
	char ifname[WIFI_IFNAME_MAX_SIZE + 1] = {0};
	int ret;
	wpa_error_log0(MSG_ERROR, "---> wifi_wpa_stop enter.");

	wifi_dev = hi_get_wifi_dev_by_iftype(iftype);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL) || (wifi_dev->iftype == HI_WIFI_IFTYPE_AP)) {
		wpa_error_log0(MSG_ERROR, "wifi_wpa_stop: get wifi dev failed");
		return HISI_FAIL;
	}
	if (memcpy_s(ifname, WIFI_IFNAME_MAX_SIZE + 1, wifi_dev->ifname, WIFI_IFNAME_MAX_SIZE) != EOK)
		return HISI_FAIL;

	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_STA_STOP_FLAG);

	if (wpa_cli_terminate(NULL, ELOOP_TASK_WPA) != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "wifi_wpa_stop: terminate failed");
		return HISI_FAIL;
	}
	(void)LOS_EventRead(&g_wpa_event, WPA_EVENT_STA_STOP_FLAG, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, LOS_WAIT_FOREVER);

	ret = wal_deinit_drv_wlan_netdev(ifname);
	if (ret != HISI_OK)
		wpa_error_log0(MSG_ERROR, "wifi_wpa_stop: wal_deinit_drv_wlan_netdev failed!");

	return ret;
}

static int wifi_add_iface(struct hisi_wifi_dev *wifi_dev)
{
	struct hisi_wifi_dev *old_wifi_dev = NULL;
	int ret;
	unsigned int ret_val;

	wpa_error_log0(MSG_ERROR, "---> wifi_add_iface enter.");

	old_wifi_dev = wpa_get_other_existed_wpa_wifi_dev(NULL);
	if ((old_wifi_dev == NULL) || (old_wifi_dev->priv == NULL)) {
		wpa_error_log0(MSG_ERROR, "no wpa wifi dev is running.");
		return HISI_FAIL;
	}
	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_ADD_IFACE_FLAG);

	wpa_error_log0(MSG_ERROR, "wifi_add_iface: wait for WPA_EVENT_ADD_IFACE_FLAG");
	ret = wpa_cli_add_iface((struct wpa_supplicant *)old_wifi_dev->priv, wifi_dev->ifname);
	if (ret != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "wpa_add_iface: failed\n");
		return HISI_FAIL;
	}

	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_ADD_IFACE_FLAG, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, LOS_WAIT_FOREVER);
	wpa_error_log0(MSG_ERROR, "wpa_add_iface: LOS_EventRead forever out\n");
	if (ret_val == WPA_EVENT_ADD_IFACE_OK) {
		wpa_error_log0(MSG_ERROR, "wpa_add_iface: successful.");
	} else if (ret_val == WPA_EVENT_ADD_IFACE_ERROR) {
		wpa_error_log1(MSG_ERROR, "wpa_add_iface failed, ret: %x\n", ret);
		return HISI_FAIL;
	}
	return HISI_OK;
}

static int wifi_remove_iface(struct hisi_wifi_dev *wifi_dev)
{
	struct wpa_supplicant *wpa_s = NULL;
	char ifname[WIFI_IFNAME_MAX_SIZE + 1] = {0};
	int ret;
	unsigned int ret_val;

	/* the caller asure wifi_dev is not NULL */
	wpa_s = wifi_dev->priv;
	if (memcpy_s(ifname, WIFI_IFNAME_MAX_SIZE, wifi_dev->ifname, WIFI_IFNAME_MAX_SIZE) != EOK)
		return HISI_FAIL;

	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_REMOVE_IFACE_FLAG);

	wpa_error_log0(MSG_ERROR, "LOS_EventRead, wait for WPA_EVENT_REMOVE_IFACE_FLAG");
	ret = wpa_cli_remove_iface(wpa_s);
	if (ret != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "wifi_remove_iface: failed\n");
		return HISI_FAIL;
	}
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_REMOVE_IFACE_FLAG,
	                        LOS_WAITMODE_OR | LOS_WAITMODE_CLR, LOS_WAIT_FOREVER);
	wpa_error_log0(MSG_ERROR, "wifi_remove_iface: LOS_EventRead forever out\n");
	if (ret_val != WPA_EVENT_REMOVE_IFACE_FLAG)
		wpa_error_log0(MSG_ERROR, "wifi_remove_iface: failed!\n");

	wal_deinit_drv_wlan_netdev(ifname);
	return HISI_OK;
}

int hi_wifi_sta_start(char *ifname, int *len)
{
	struct hisi_wifi_dev *wifi_dev = NULL;
	int rc;
	int ret;

	if ((ifname == NULL) || (len == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_start: invalid param");
		return HISI_FAIL;
	}

	if (try_set_lock_flag() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_start: wifi dev start or stop is running.");
		return HISI_FAIL;
	}

	if (sta_precheck() != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_start: sta_precheck fail!");
		goto WIFI_STA_EXIT;
	}

	wifi_dev = wifi_dev_creat(HI_WIFI_IFTYPE_STATION, g_sta_opt_set.hw_mode);
	if (wifi_dev == NULL) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_start: wifi_dev_creat failed.");
		goto WIFI_STA_EXIT;
	}
	if ((hi_set_wifi_dev(wifi_dev) != HISI_OK) || (*len < wifi_dev->ifname_len + 1))
		goto WIFI_STA_ERROR;

	rc = memcpy_s(ifname, *len, wifi_dev->ifname, wifi_dev->ifname_len);
	if (rc != EOK) {
		wpa_error_log0(MSG_ERROR, "Could not copy wifi dev ifname.");
		goto WIFI_STA_ERROR;
	}
	ifname[wifi_dev->ifname_len] = '\0';
	*len = wifi_dev->ifname_len;
	if ((hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_P2P_DEVICE) != NULL) ||
		(hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_MESH_POINT) != NULL)) {
		ret = wifi_add_iface(wifi_dev);
	} else
		ret = wifi_wpa_start(wifi_dev);

	if (ret != HISI_OK)
		goto WIFI_STA_ERROR;

	clr_lock_flag();
	return HISI_OK;
WIFI_STA_ERROR:
	wal_deinit_drv_wlan_netdev(wifi_dev->ifname);
	hi_free_wifi_dev(wifi_dev);
WIFI_STA_EXIT:
	clr_lock_flag();
	return HISI_FAIL;
}

int hi_wifi_sta_stop(void)
{
	int ret = HISI_FAIL;

	if (try_set_lock_flag() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}

	struct hisi_wifi_dev *wifi_dev = hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_STATION);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_stop: get wifi dev failed");
		goto WIFI_STA_STOP_FAIL;
	}
	if (wpa_get_other_existed_wpa_wifi_dev(wifi_dev->priv) != NULL)
		ret = wifi_remove_iface(wifi_dev);
	else
		ret = wifi_wpa_stop(HI_WIFI_IFTYPE_STATION);
	if (ret != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "wpa_stop failed");
		ret = HISI_FAIL;
		goto WIFI_STA_STOP_FAIL;
	}

	(void)memset_s(&g_sta_opt_set, sizeof(struct wifi_sta_opt_set), 0, sizeof(struct wifi_sta_opt_set));
	(void)memset_s(&g_reconnect_set, sizeof(struct wifi_reconnect_set), 0, sizeof(struct wifi_reconnect_set));
	(void)memset_s(&g_scan_record, sizeof(struct hisi_scan_record), 0, sizeof(struct hisi_scan_record));

	g_mesh_sta_flag = WPA_FLAG_OFF;
	g_connecting_flag = WPA_FLAG_OFF;
	g_wpa_rm_network = HI_WPA_RM_NETWORK_END;

	ret = HISI_OK;
WIFI_STA_STOP_FAIL:
	clr_lock_flag();
	return ret;
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
	(void)memcpy_s(ifname, WIFI_IFNAME_MAX_SIZE, wifi_dev->ifname, WIFI_IFNAME_MAX_SIZE);
	(void)LOS_EventClear(&g_wpa_event, ~WPA_EVENT_AP_STOP_OK);
	wpa_error_log0(MSG_ERROR, "hi_wifi_softap_stop: wait for WPA_EVENT_AP_STOP_OK");

	if (wpa_cli_terminate(NULL, ELOOP_TASK_HOSTAPD) != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_softap_stop: wpa_cli_terminate failed");
		goto WIFI_AP_STOP_EXIT;
	}
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_AP_STOP_OK, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, LOS_WAIT_FOREVER);
	wpa_error_log0(MSG_ERROR, "hi_wifi_softap_stop: LOS_EventRead forever out\n");
	if (ret_val != WPA_EVENT_AP_STOP_OK)
		wpa_error_log0(MSG_ERROR, "hi_wifi_softap_stop: error!\n");

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
	wpa_error_log0(MSG_ERROR, "LOS_EventRead,wait for WPA_EVENT_AP_DEAUTH_FLAG");
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_AP_DEAUTH_FLAG, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
	if (ret_val != WPA_EVENT_AP_DEAUTH_OK)
		return HISI_FAIL;
	return HISI_OK;
}

#ifdef CONFIG_WPS
int hi_wifi_sta_wps_pbc(unsigned char *bssid)
{
	int ret;
	struct hisi_wifi_dev *wifi_dev = NULL;
	char network_id_txt[WPA_NETWORK_ID_TXT_LEN + 1] = { 0 };
	char addr_txt[HI_WIFI_TXT_ADDR_LEN + 1] = { 0 };

	wifi_dev = wifi_dev_get(HI_WIFI_IFTYPE_STATION);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_wps_pbc: get wifi dev failed.");
		return HISI_FAIL;
	}
	if (bssid != NULL) {
		if (addr_precheck(bssid) != HISI_OK) {
			wpa_error_log0(MSG_ERROR, "Invalid bssid.");
			return HISI_FAIL;
		}
	}
	ret = snprintf_s(network_id_txt, sizeof(network_id_txt), sizeof(network_id_txt) - 1, "%d", wifi_dev->network_id);
	if (ret < 0) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_wps_pbc snprintf_s faild");
		return HISI_FAIL;
	}
	wpa_cli_remove_network((struct wpa_supplicant *)(wifi_dev->priv), network_id_txt);
	if (bssid == NULL) {
		ret = wpa_cli_wps_pbc((struct wpa_supplicant *)(wifi_dev->priv), NULL);
	} else {
		ret = snprintf_s(addr_txt, sizeof(addr_txt), sizeof(addr_txt) - 1, MACSTR, MAC2STR(bssid));
		if (ret < 0) {
			wpa_error_log0(MSG_ERROR, "hi_wifi_sta_wps_pbc snprintf_s faild");
			return HISI_FAIL;
		}
		ret = wpa_cli_wps_pbc((struct wpa_supplicant *)(wifi_dev->priv), addr_txt);
	}
	if (ret != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_wps_pbc failed");
		return HISI_FAIL;
	}
	return HISI_OK;
}

int hi_wifi_sta_wps_pin(char *pin, unsigned char *bssid)
{
	int ret;
	unsigned int i, len;
	char network_id_txt[WPA_NETWORK_ID_TXT_LEN + 1] = { 0 };
	struct hisi_wifi_dev *wifi_dev                  = NULL;
	char addr_txt[HI_WIFI_TXT_ADDR_LEN + 1]         = { 0 };

	wifi_dev = wifi_dev_get(HI_WIFI_IFTYPE_STATION);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_wps_pin: get wifi dev failed\n");
		return HISI_FAIL;
	}

	if (pin == NULL) {
		wpa_warning_log0(MSG_DEBUG, "wpa_cli_wps_pin : pin is NULL");
		return HISI_FAIL;
	}
	if (bssid != NULL) {
		if (addr_precheck(bssid) != HISI_OK) {
			wpa_error_log0(MSG_ERROR, "Invalid bssid.");
			return HISI_FAIL;
		}
	}
	len = strlen(pin);
	for (i = 0; i < len; i++)
		if ((pin[i] < '0') || (pin[i] > '9')) {
			wpa_error_log0(MSG_ERROR, "wps_pin: pin Format should be 0 ~ 9.");
			return HISI_FAIL;
	}
	ret = snprintf_s(network_id_txt, sizeof(network_id_txt), sizeof(network_id_txt) - 1, "%d", wifi_dev->network_id);
	if (ret < 0) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_sta_wps_pin: network_id_txt snprintf_s faild");
		return HISI_FAIL;
	}
	wpa_cli_remove_network((struct wpa_supplicant *)(wifi_dev->priv), network_id_txt);
	if (bssid == NULL) {
		ret = wpa_cli_wps_pin((struct wpa_supplicant *)(wifi_dev->priv), pin, NULL);
	} else {
		ret = snprintf_s(addr_txt, sizeof(addr_txt), sizeof(addr_txt) - 1, MACSTR, MAC2STR(bssid));
		if (ret < 0) {
			wpa_error_log0(MSG_ERROR, "hi_wifi_sta_wps_pin: addr_txt snprintf_s faild");
			return HISI_FAIL;
		}
		ret = wpa_cli_wps_pin((struct wpa_supplicant *)(wifi_dev->priv), pin, addr_txt);
	}
	return ret;
}

int hi_wifi_sta_wps_pin_get(char *pin, unsigned int len)
{
	unsigned int val = 0;
	int ret;

	if (pin == NULL) {
		wpa_error_log0(MSG_ERROR, " ---> pin ptr is NULL.");
		return HISI_FAIL;
	}
	if (len != (WIFI_WPS_PIN_LEN + 1)) {
		wpa_error_log1(MSG_ERROR, " ---> wps pin buffer length should be %d.", WIFI_WPS_PIN_LEN + 1);
		return HISI_FAIL;
	}
	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}
	ret = wps_generate_pin(&val);
	if (ret != HISI_OK) {
		wpa_error_log0(MSG_ERROR, " ---> wps generate pin failed.");
		return HISI_FAIL;
	}
	ret = snprintf_s(pin, WIFI_WPS_PIN_LEN + 1, WIFI_WPS_PIN_LEN, "%08u", val);
	return (ret < 0) ? HISI_FAIL : HISI_OK;
}
#endif /* CONFIG_WPS */

/*****************************change STA settingts start ***********************************/
static int wifi_sta_set_cond_check(void)
{
	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi_sta_set_cond_check: wifi dev start or stop is running.");
		return HISI_FAIL;
	}
	if (is_sta_on() == HISI_OK) {
		wpa_error_log0(MSG_ERROR, "wifi_sta_set_cond_check: sta is already in progress, set failed.");
		return HISI_FAIL;
	}
	return HISI_OK;
}

int hi_wifi_set_pmf(hi_wifi_pmf_options pmf)
{
	if (wifi_sta_set_cond_check() == HISI_FAIL)
		return HISI_FAIL;

	if ((pmf < HI_WIFI_MGMT_FRAME_PROTECTION_CLOSE) || (pmf > HI_WIFI_MGMT_FRAME_PROTECTION_REQUIRED)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_set_pmf: invalid pmf options.");
		return HISI_FAIL;
	}

	g_sta_opt_set.pmf = pmf;
	g_sta_opt_set.usr_pmf_set_flag = WPA_FLAG_ON;
	return HISI_OK;
}

int hi_wifi_sta_set_reconnect_policy(int enable, unsigned int seconds,
                                     unsigned int period, unsigned int max_try_count)
{
	struct hisi_wifi_dev *wifi_dev = NULL;
	struct wpa_supplicant *wpa_s = NULL;
	struct wpa_ssid *current_ssid = g_reconnect_set.current_ssid;
	int is_connected = WPA_FLAG_OFF;

	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}
	if (is_sta_on() != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "sta is not started, set reconnect policy failed.");
		return HISI_FAIL;
	}

	if ((enable < WPA_FLAG_OFF) || (enable > WPA_FLAG_ON) ||
	    (seconds < WIFI_MIN_RECONNECT_TIMEOUT) || (seconds > WIFI_MAX_RECONNECT_TIMEOUT) ||
	    (period < WIFI_MIN_RECONNECT_PERIOD) || (period > WIFI_MAX_RECONNECT_PERIOD) ||
	    (max_try_count < WIFI_MIN_RECONNECT_TIMES) || (max_try_count > WIFI_MAX_RECONNECT_TIMES)) {
		wpa_error_log0(MSG_ERROR, "input value error.");
		return HISI_FAIL;
	}
	LOS_TaskLock();
	(void)memset_s(&g_reconnect_set, sizeof(struct wifi_reconnect_set), 0, sizeof(struct wifi_reconnect_set));
	g_reconnect_set.enable = enable;
	g_reconnect_set.timeout = seconds;
	g_reconnect_set.period = period;
	g_reconnect_set.max_try_count = max_try_count;

	wifi_dev = hi_get_wifi_dev_by_iftype(HI_WIFI_IFTYPE_STATION);
	if ((wifi_dev != NULL) && (wifi_dev->priv != NULL)) {
		wpa_s = (struct wpa_supplicant *)wifi_dev->priv;
		is_connected = (wpa_s->wpa_state == WPA_COMPLETED) ? WPA_FLAG_ON : WPA_FLAG_OFF;
		g_reconnect_set.current_ssid = (is_connected == WPA_FLAG_ON) ? wpa_s->current_ssid : current_ssid;
	}
	LOS_TaskUnlock();
	if ((enable != WPA_FLAG_ON) || (is_connected == WPA_FLAG_ON))
		wpa_cli_sta_set_delay_report(wpa_s, enable);
	return HISI_OK;
}

int hi_wifi_sta_set_protocol_mode(hi_wifi_protocol_mode hw_mode)
{
	if (wifi_sta_set_cond_check() == HISI_FAIL)
		return HISI_FAIL;

	if ((hw_mode < HI_WIFI_PHY_MODE_11BGN) || (hw_mode >= HI_WIFI_PHY_MODE_BUTT)) {
		wpa_error_log0(MSG_ERROR, "physical mode value is error.");
		return HISI_FAIL;
	}
	g_sta_opt_set.hw_mode = hw_mode;
	return HISI_OK;
}

hi_wifi_pmf_options hi_wifi_get_pmf(void)
{
	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}
	wpa_error_log1(MSG_ERROR, "pmf:%d.", g_sta_opt_set.pmf);
	return g_sta_opt_set.pmf;
}

hi_wifi_protocol_mode hi_wifi_sta_get_protocol_mode(void)
{
	if (is_lock_flag_off() == HISI_FAIL) {
		wpa_error_log0(MSG_ERROR, "wifi dev start or stop is running.");
		return HISI_FAIL;
	}
	wpa_error_log1(MSG_ERROR, "sta phymode:%d.", g_sta_opt_set.hw_mode);
	return g_sta_opt_set.hw_mode;
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
	unsigned int ret;

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

	wpa_error_log0(MSG_ERROR, "hi_wifi_softap_get_connected_sta: wait for WPA_EVENT_SHOW_STA_FLAG");
	(void)wpa_cli_show_sta(wpa_s);
	ret = LOS_EventRead(&g_wpa_event, WPA_EVENT_SHOW_STA_FLAG, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
	g_ap_sta_info = NULL;
	if ((ret == WPA_EVENT_SHOW_STA_ERROR) || (ret == LOS_ERRNO_EVENT_READ_TIMEOUT)) {
		*sta_num = 0;
		g_sta_num = 0;
		return HISI_FAIL;
	}
	*sta_num = g_sta_num;
	g_sta_num = 0;
	return HISI_OK;
}

void wifi_event_task_handler(UINT32 event)
{
	if (g_wpa_event_cb != NULL) {
		g_wpa_event_cb((hi_wifi_event *)(UINTPTR)event);
	}
	g_wpa_event_taskid = 0;
	os_free((void *)(UINTPTR)event);
}

/* create a task and call user's cb */
void wifi_new_task_event_cb(hi_wifi_event *event_cb)
{
	TSK_INIT_PARAM_S event_cb_task = {0};
	hi_wifi_event *event_new = NULL;
	unsigned int timeout = 0;

	if (g_wpa_event_cb == NULL) {
		return;
	}
	if (g_direct_cb == 1) { /* call directly */
		g_wpa_event_cb(event_cb);
		return;
	}
	while (g_wpa_event_taskid != 0) {
		LOS_TaskDelay(1);   /* delay 1 tick and try again */
		timeout++;
		if (timeout > 100) { /* 100 tick = 1s */
    		wpa_error_log0(MSG_ERROR, "Event cb task has been created.");
    		return;
		}
	}
	event_new = (hi_wifi_event *)os_zalloc(sizeof(hi_wifi_event));
	if (event_new == NULL) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_event malloc err.");
		return;
	}
	if (memcpy_s(event_new, sizeof(hi_wifi_event), event_cb, sizeof(hi_wifi_event)) != EOK) {
		wpa_error_log0(MSG_ERROR, "memcpy event info err.");
		os_free(event_new);
		return;
	}
	event_cb_task.pfnTaskEntry = (TSK_ENTRY_FUNC)wifi_event_task_handler;
	event_cb_task.uwStackSize  = g_cb_stack_size;
	event_cb_task.pcName = "wpa_event_cb";
	event_cb_task.usTaskPrio = g_cb_task_prio;      /* task prio, must lower than wifi/lwip/wpa */
	event_cb_task.auwArgs[0] = (UINT32)(UINTPTR)event_new;
	event_cb_task.uwResved   = LOS_TASK_STATUS_DETACHED;
	if (LOS_TaskCreate(&g_wpa_event_taskid, &event_cb_task) != LOS_OK) {
		wpa_error_log0(MSG_ERROR, "Event cb create task failed.");
		os_free(event_new);
	}
}

int hi_wifi_config_callback(unsigned char mode, unsigned char task_prio, unsigned short stack_size)
{
	/* only support 0 and 1 */
	if ((mode & 1) != mode) {
		wpa_error_log1(MSG_ERROR, "hi_wifi_config_callback: invalid mode:%d.", mode);
		return HISI_FAIL;
	}
	g_direct_cb = mode;
	/* directly call don't need follow configuraion */
	if (g_direct_cb == 1) {
		return HISI_OK;
	}
	if ((task_prio < HI_WIFI_CB_MIN_PRIO) || (task_prio > HI_WIFI_CB_MAX_PRIO)) {
		wpa_error_log1(MSG_ERROR, "hi_wifi_config_callback: invalid prio:%d.", task_prio);
		return HISI_FAIL;
	}
	if (stack_size < HI_WIFI_CB_MIN_STACK) {
		wpa_error_log1(MSG_ERROR, "hi_wifi_config_callback: invalid stack size:%d.", stack_size);
		return HISI_FAIL;
	}
	g_cb_task_prio = task_prio;
	g_cb_stack_size = stack_size & 0xFFF0;  /* 16bytes allign */
	return HISI_OK;
}

int hi_wifi_register_event_callback(hi_wifi_event_cb event_cb)
{
	g_wpa_event_cb = event_cb;
	return HISI_OK;
}

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

	wpa_error_log0(MSG_ERROR, "wifi_mesh_start_internal: wait for WPA_EVENT_MESH_START_OK");
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_MESH_START_OK, (LOS_WAITMODE_OR | LOS_WAITMODE_CLR), WPA_DELAY_5S);
	if (ret_val != WPA_EVENT_MESH_START_OK)
		goto FAIL;

	wpa_error_log0(MSG_ERROR, "wifi_mesh_start_internal: successful.");
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

		wpa_error_log0(MSG_ERROR, "LOS_EventRead, wait for WPA_EVENT_MESH_DEAUTH_FLAG");
		unsigned int ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_MESH_DEAUTH_FLAG,
		                                     LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
		if (ret_val != WPA_EVENT_MESH_DEAUTH_OK)
			return HISI_FAIL;
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

	wpa_error_log0(MSG_ERROR, "LOS_EventRead, wait for WPA_EVENT_SHOW_STA_FLAG");
	ret_val = LOS_EventRead(&g_wpa_event, WPA_EVENT_SHOW_STA_FLAG, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, WPA_DELAY_5S);
	g_mesh_sta_info = NULL;
	g_mesh_get_sta_flag = WPA_FLAG_OFF;
	if ((ret_val == WPA_EVENT_SHOW_STA_ERROR) || (ret_val == LOS_ERRNO_EVENT_READ_TIMEOUT)) {
		*peer_num = 0;
		g_sta_num = 0;
		return HISI_FAIL;
	}
	*peer_num = g_sta_num;
	g_sta_num = 0;
	return HISI_OK;
}

static int wifi_set_usr_app_ie(hi_wifi_iftype iftype, hi_wifi_frame_type fram_type,
                               hi_wifi_usr_ie_type usr_ie_type,
                               const unsigned char *ie, unsigned short ie_len)
{
	struct hisi_wifi_dev *wifi_dev = NULL;
	unsigned char valid_iftype = (iftype == HI_WIFI_IFTYPE_MESH_POINT) ||
	                             ((iftype == HI_WIFI_IFTYPE_STATION) && (g_mesh_sta_flag == WPA_FLAG_ON));

	if ((usr_ie_type >= HI_WIFI_USR_IE_BUTT) || (fram_type >= HI_WIFI_FRAME_TYPE_BUTT) || !valid_iftype) {
		wpa_error_log0(MSG_ERROR, "wifi_set_usr_app_ie_internal: invalid param.");
		return HISI_FAIL;
	}

	if ((iftype == HI_WIFI_IFTYPE_STATION) && ((unsigned int)fram_type & HI_WIFI_FRAME_TYPE_BEACON)) {
		wpa_error_log0(MSG_ERROR, "wifi_set_usr_app_ie_internal: conflict between frame type and iftype.");
		return HISI_FAIL;
	}

	wifi_dev = wifi_dev_get(iftype);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL)) {
		wpa_error_log0(MSG_ERROR, "wifi_set_usr_app_ie_internal: get wifi dev failed.");
		return HISI_FAIL;
	}

	if (hisi_set_usr_app_ie(wifi_dev->ifname, ((ie != NULL) ? 1 : 0), fram_type, usr_ie_type, ie, ie_len) != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "wifi_set_usr_app_ie_internal: set usr ie failed.");
		return HISI_FAIL;
	}
	return HISI_OK;
}

int hi_wifi_add_usr_app_ie(hi_wifi_iftype iftype, hi_wifi_frame_type frame_type,
                           hi_wifi_usr_ie_type usr_ie_type, const unsigned char *ie, unsigned short ie_len)
{
	if ((ie == NULL) || (ie_len > HI_WIFI_USR_IE_MAX_SIZE) || (ie_len == 0)) {
		wpa_error_log0(MSG_ERROR, "hi_wifi_add_usr_app_ie: invalid input.");
		return HISI_FAIL;
	}

	return wifi_set_usr_app_ie(iftype, frame_type, usr_ie_type, ie, ie_len); /* 1: add */
}

int hi_wifi_delete_usr_app_ie(hi_wifi_iftype iftype, hi_wifi_frame_type frame_type,
                              hi_wifi_usr_ie_type usr_ie_type)
{
	return wifi_set_usr_app_ie(iftype, frame_type, usr_ie_type, NULL, 0); /* 0: delete */
}
#endif /* LOS_CONFIG_MESH */

