/*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
* Description: driver_hisi
* Author: hisilicon
* Create: 2019-03-04
*/

#include "driver_hisi.h"
#include "l2_packet/l2_packet.h"
#include "ap/ap_config.h"
#include "ap/hostapd.h"
#include "driver_hisi_ioctl.h"
#include "eloop.h"
#include "wpa_supplicant/wpa_supplicant.h"
#include "wpa_supplicant/mesh.h"
#include "hi_mesh.h"
#include "securec.h"
#include "los_hwi.h"
#include <hi_cpu.h>
#include <hi_task.h>
#include "hi_at.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef struct _hisi_cmd_stru {
	int32 cmd;
	const struct wpabuf *src;
} hisi_cmd_stu;

uint8 g_ssid_prefix_flag = WPA_FLAG_OFF;
int g_sta_delay_report_flag = WPA_FLAG_OFF;
static int g_rx_mgmt_count = 0;

#define SCAN_TIME_OUT 5
#define RX_MGMT_EVENT_MAX_COUNT 15
#define DELAY_REPORT_TIMEOUT 15

uint32 hisi_alg_to_cipher_suite(enum wpa_alg alg, size_t key_len);
int32 hisi_is_ap_interface(hisi_iftype_enum_uint8 nlmode);
static void hisi_key_ext_free(hisi_key_ext_stru *key_ext);
static void hisi_ap_settings_free(hisi_ap_settings_stru  *apsettings);
int32 hisi_set_key(const char *ifname, void *priv, enum wpa_alg alg, const uint8 *addr, int32 key_idx,
                   int32 set_tx, const uint8 *seq, size_t seq_len, const uint8 *key, size_t key_len);
int32 hisi_set_ap(void *priv, struct wpa_driver_ap_params *params);
int32 hisi_send_mlme(void *priv, const uint8 *data, size_t data_len, int32 noack, unsigned int freq,
                     const u16 *csa_offs, size_t csa_offs_len);
int32 hisi_send_eapol(void *priv, const uint8 *addr, const uint8 *data, size_t data_len, int32 encrypt,
                      const uint8 *own_addr, uint32 flags);
int32 hisi_driver_send_event(const char *ifname, int32 cmd, const uint8 *buf, uint32 length);
void hisi_driver_event_process(void *eloop_data, void *user_ctx);
void * hisi_hapd_init(struct hostapd_data *hapd, struct wpa_init_params *params);
static void hisi_hw_feature_data_free(struct hostapd_hw_modes *modes, uint16 modes_num);
void hisi_rx_mgmt_process(void *ctx, hisi_rx_mgmt_stru *rx_mgmt, union wpa_event_data *event);
void hisi_tx_status_process(void *ctx, hisi_tx_status_stru *tx_status, union wpa_event_data *event);
void hisi_drv_deinit(void *priv);
void hisi_hapd_deinit(void *priv);
void hisi_driver_scan_timeout(void *eloop_ctx, void *timeout_ctx);
#ifdef _PRE_WLAN_FEATURE_REKEY_OFFLOAD
void hisi_set_rekey_info(void *priv, const uint8 *kek, size_t kek_len, const uint8 *kck,
                         size_t kck_len, const uint8 *replay_ctr);
#endif
hisi_driver_data_stru * hisi_drv_init(void *ctx, const struct wpa_init_params *params);
struct hostapd_hw_modes * hisi_get_hw_feature_data(void *priv, uint16 *num_modes, uint16 *flags);
int32 hisi_connect(hisi_driver_data_stru *drv, struct wpa_driver_associate_params *params);
int32 hisi_try_connect(hisi_driver_data_stru *drv, struct wpa_driver_associate_params *params);
void hisi_set_conn_keys(const struct wpa_driver_associate_params *wpa_params, hisi_associate_params_stru *params);
uint32 hisi_cipher_to_cipher_suite(uint32 cipher);
int32 hisi_disconnect(hisi_driver_data_stru *drv, uint16 reason_code);
void hisi_excep_recv(void *priv, const int8 *data);
int32 hisi_sta_remove(void *priv, const uint8 *addr);

int32 hisi_is_ap_interface(hisi_iftype_enum_uint8 nlmode)
{
	return ((nlmode == HI_WIFI_IFTYPE_AP) || (nlmode == HI_WIFI_IFTYPE_P2P_GO));
}

static void hisi_key_ext_free(hisi_key_ext_stru *key_ext)
{
	if (key_ext == NULL)
		return;

	if (key_ext->addr != NULL) {
		os_free(key_ext->addr);
		key_ext->addr = NULL;
	}

	if (key_ext->seq != NULL) {
		os_free(key_ext->seq);
		key_ext->seq = NULL;
	}

	if (key_ext->key != NULL) {
		os_free(key_ext->key);
		key_ext->key = NULL;
	}

	os_free(key_ext);
}

static void hisi_ap_settings_free(hisi_ap_settings_stru *apsettings)
{
	if (apsettings == NULL)
		return;

	if (apsettings->mesh_ssid != NULL) {
		os_free(apsettings->mesh_ssid);
		apsettings->mesh_ssid = NULL;
	}

	if (apsettings->ssid != NULL) {
		os_free(apsettings->ssid);
		apsettings->ssid = NULL;
	}

	if (apsettings->beacon_data.head != NULL) {
		os_free(apsettings->beacon_data.head);
		apsettings->beacon_data.head = NULL;
	}

	if (apsettings->beacon_data.tail != NULL) {
		os_free(apsettings->beacon_data.tail);
		apsettings->beacon_data.tail = NULL;
	}

	os_free(apsettings);
}

uint32 hisi_alg_to_cipher_suite(enum wpa_alg alg, size_t key_len)
{
	switch (alg) {
		case WPA_ALG_WEP:
			/* key_len = 5 : WEP40, 13 : WEP104 */
			if (key_len == WPA_WEP40_KEY_LEN)
				return RSN_CIPHER_SUITE_WEP40;
			return RSN_CIPHER_SUITE_WEP104;
		case WPA_ALG_TKIP:
			return RSN_CIPHER_SUITE_TKIP;
		case WPA_ALG_CCMP:
			return RSN_CIPHER_SUITE_CCMP;
		case WPA_ALG_GCMP:
			return RSN_CIPHER_SUITE_GCMP;
		case WPA_ALG_CCMP_256:
			return RSN_CIPHER_SUITE_CCMP_256;
		case WPA_ALG_GCMP_256:
			return RSN_CIPHER_SUITE_GCMP_256;
		case WPA_ALG_IGTK:
			return RSN_CIPHER_SUITE_AES_128_CMAC;
		case WPA_ALG_BIP_GMAC_128:
			return RSN_CIPHER_SUITE_BIP_GMAC_128;
		case WPA_ALG_BIP_GMAC_256:
			return RSN_CIPHER_SUITE_BIP_GMAC_256;
		case WPA_ALG_BIP_CMAC_256:
			return RSN_CIPHER_SUITE_BIP_CMAC_256;
		case WPA_ALG_SMS4:
			return RSN_CIPHER_SUITE_SMS4;
		case WPA_ALG_KRK:
			return RSN_CIPHER_SUITE_KRK;
		case WPA_ALG_NONE:
		case WPA_ALG_PMK:
			return 0;
		default:
			return 0;
	}
}

int hisi_init_key(hisi_key_ext_stru *key_ext, enum wpa_alg alg, const uint8 *addr, int32 key_idx, int32 set_tx,
                  const uint8 *seq, size_t seq_len, const uint8 *key, size_t key_len)
{
	key_ext->default_types = HISI_KEY_DEFAULT_TYPE_INVALID;
	key_ext->seq_len = seq_len;
	key_ext->key_len = key_len;
	key_ext->key_idx = key_idx;
	key_ext->type = HISI_KEYTYPE_DEFAULT_INVALID;
	key_ext->cipher = hisi_alg_to_cipher_suite(alg, key_len);
	if ((alg != WPA_ALG_NONE) && (key != NULL) && (key_len != 0)) {
		key_ext->key = (uint8 *)os_zalloc(key_len); /* freed by hisi_key_ext_free */
		if ((key_ext->key == NULL) || (memcpy_s(key_ext->key, key_len, key, key_len) != EOK))
			return -HISI_EFAIL;
	}
	if ((seq != NULL) && (seq_len != 0)) {
		key_ext->seq = (uint8 *)os_zalloc(seq_len); /* freed by hisi_key_ext_free */
		if ((key_ext->seq == NULL) || (memcpy_s(key_ext->seq, seq_len, seq, seq_len) != EOK))
			return -HISI_EFAIL;
	}
	if ((addr != NULL) && (!is_broadcast_ether_addr(addr))) {
		key_ext->addr = (uint8 *)os_zalloc(ETH_ADDR_LEN); /* freed by hisi_key_ext_free */
		if ((key_ext->addr == NULL) || (memcpy_s(key_ext->addr, ETH_ADDR_LEN, addr, ETH_ADDR_LEN) != EOK))
			return -HISI_EFAIL;
		if ((alg != WPA_ALG_WEP) && (key_idx != 0) && (set_tx == 0))
			key_ext->type = HISI_KEYTYPE_GROUP;
	} else if ((addr != NULL) && (is_broadcast_ether_addr(addr))) {
		key_ext->addr = NULL;
	}
	if (key_ext->type == HISI_KEYTYPE_DEFAULT_INVALID)
		key_ext->type = (key_ext->addr != NULL) ? HISI_KEYTYPE_PAIRWISE : HISI_KEYTYPE_GROUP;
	if ((alg == WPA_ALG_IGTK) || (alg == WPA_ALG_BIP_GMAC_128) ||
		(alg == WPA_ALG_BIP_GMAC_256) || (alg == WPA_ALG_BIP_CMAC_256))
		key_ext->defmgmt = HISI_TRUE;
	else
		key_ext->def = HISI_TRUE;
	if ((addr != NULL) && (is_broadcast_ether_addr(addr)))
		key_ext->default_types = HISI_KEY_DEFAULT_TYPE_MULTICAST;
	else if (addr != NULL)
		key_ext->default_types = HISI_KEY_DEFAULT_TYPE_UNICAST;
	return HISI_SUCC;
}

int32 hisi_set_key(const  char *ifname, void *priv, enum wpa_alg alg, const uint8 *addr, int32 key_idx,
                   int32 set_tx, const uint8 *seq, size_t seq_len, const uint8 *key, size_t key_len)
{
	int32 ret = HISI_SUCC;
	hisi_key_ext_stru *key_ext = HISI_PTR_NULL;
	hisi_driver_data_stru *drv = priv;

	/* addr, seq, key will be checked below */
	if ((ifname == NULL) || (priv == NULL))
		return -HISI_EFAIL;

	/* Ignore for P2P Device */
	if (drv->nlmode == HI_WIFI_IFTYPE_P2P_DEVICE)
		return HISI_SUCC;

	key_ext = os_zalloc(sizeof(hisi_key_ext_stru));
	if (key_ext == NULL)
		return -HISI_EFAIL;

	if (hisi_init_key(key_ext, alg, addr, key_idx, set_tx, seq, seq_len, key, key_len) != HISI_SUCC) {
		hisi_key_ext_free(key_ext);
		return -HISI_EFAIL;
	}

	if (alg == WPA_ALG_NONE) {
		ret = hisi_ioctl_del_key((int8 *)ifname, key_ext);
	} else {
		ret = hisi_ioctl_new_key((int8 *)ifname, key_ext);
		/* if set new key fail, just return without setting default key */
		if ((ret != HISI_SUCC) || (set_tx == 0) || (alg == WPA_ALG_NONE)) {
			hisi_key_ext_free(key_ext);
			return ret;
		}

		if ((hisi_is_ap_interface(drv->nlmode)) && (key_ext->addr != NULL) &&
			(!is_broadcast_ether_addr(key_ext->addr))) {
			hisi_key_ext_free(key_ext);
			return ret;
		}
		ret = hisi_ioctl_set_key((int8 *)ifname, key_ext);
	}

	hisi_key_ext_free(key_ext);
	return ret;
}

static void hisi_set_ap_freq(hisi_ap_settings_stru *apsettings, const struct wpa_driver_ap_params *params)
{
	if (params->freq != NULL) {
		apsettings->freq_params.mode               = params->freq->mode;
		apsettings->freq_params.freq               = params->freq->freq;
		apsettings->freq_params.channel            = params->freq->channel;
		apsettings->freq_params.ht_enabled         = params->freq->ht_enabled;
		apsettings->freq_params.center_freq1       = params->freq->center_freq1;
		apsettings->freq_params.bandwidth          = HISI_CHAN_WIDTH_20;
	}
}

static int hisi_set_ap_beacon_data(hisi_ap_settings_stru *apsettings, const struct wpa_driver_ap_params *params)
{
	if ((params->head != NULL) && (params->head_len != 0)) {
		apsettings->beacon_data.head_len = params->head_len;
		/* beacon_data.head freed by hisi_ap_settings_free */
		apsettings->beacon_data.head = (uint8 *)os_zalloc(apsettings->beacon_data.head_len);
		if (apsettings->beacon_data.head == NULL)
			return -HISI_EFAIL;
		if (memcpy_s(apsettings->beacon_data.head, apsettings->beacon_data.head_len,
		    params->head, params->head_len) != EOK)
			return -HISI_EFAIL;
	}

	if ((params->tail != NULL) && (params->tail_len != 0)) {
		apsettings->beacon_data.tail_len = params->tail_len;
		/* beacon_data.tail freed by hisi_ap_settings_free */
		apsettings->beacon_data.tail = (uint8 *)os_zalloc(apsettings->beacon_data.tail_len);
		if (apsettings->beacon_data.tail == NULL)
			return -HISI_EFAIL;

		if (memcpy_s(apsettings->beacon_data.tail, apsettings->beacon_data.tail_len,
		    params->tail, params->tail_len) != EOK)
			return -HISI_EFAIL;
	}
	return HISI_SUCC;
}

int32 hisi_set_ap(void *priv, struct wpa_driver_ap_params *params)
{
	int32 ret;
	hisi_ap_settings_stru *apsettings = HISI_PTR_NULL;
	hisi_driver_data_stru *drv = (hisi_driver_data_stru *)priv;
	if ((priv == NULL) || (params == NULL) || (params->freq == NULL))
		return -HISI_EFAIL;

	apsettings = os_zalloc(sizeof(hisi_ap_settings_stru));
	if (apsettings == NULL)
		return -HISI_EFAIL;
	apsettings->beacon_interval = params->beacon_int;
	apsettings->dtim_period     = params->dtim_period;
	apsettings->hidden_ssid     = params->hide_ssid;
	if ((params->auth_algs & (WPA_AUTH_ALG_OPEN | WPA_AUTH_ALG_SHARED)) == (WPA_AUTH_ALG_OPEN | WPA_AUTH_ALG_SHARED))
		apsettings->auth_type = HISI_AUTHTYPE_AUTOMATIC;
	else if ((params->auth_algs & WPA_AUTH_ALG_SHARED) == WPA_AUTH_ALG_SHARED)
		apsettings->auth_type = HISI_AUTHTYPE_SHARED_KEY;
	else
		apsettings->auth_type = HISI_AUTHTYPE_OPEN_SYSTEM;

	/* wifi driver will copy mesh_ssid by itself. */
	if ((params->ssid != NULL) && (params->ssid_len != 0)) {
		apsettings->ssid_len = params->ssid_len;
		apsettings->ssid = (uint8 *)os_zalloc(apsettings->ssid_len);
		if ((apsettings->ssid == NULL) || (memcpy_s(apsettings->ssid, apsettings->ssid_len,
		    params->ssid, params->ssid_len) != EOK))
			goto FAILED;
	}
	hisi_set_ap_freq(apsettings, params);
	if (hisi_set_ap_beacon_data(apsettings, params) != HISI_SUCC)
		goto FAILED;
	if (drv->beacon_set == HISI_TRUE)
		ret = hisi_ioctl_change_beacon(drv->iface, apsettings);
	else
		ret = hisi_ioctl_set_ap(drv->iface, apsettings);
	if (ret == HISI_SUCC)
		drv->beacon_set = HISI_TRUE;
	hisi_ap_settings_free(apsettings);
	return ret;
FAILED:
	hisi_ap_settings_free(apsettings);
	return -HISI_EFAIL;
}

int32 hisi_send_mlme(void *priv, const uint8 *data, size_t data_len,
                     int32 noack, unsigned int freq, const u16 *csa_offs, size_t csa_offs_len)
{
	int32 ret;
	hisi_driver_data_stru *drv = priv;
	hisi_mlme_data_stru *mlme_data = HISI_PTR_NULL;
	errno_t rc;
	(void)freq;
	(void)csa_offs;
	(void)csa_offs_len;
	(void)noack;
	if ((priv == NULL) || (data == NULL))
		return -HISI_EFAIL;
	mlme_data = os_zalloc(sizeof(hisi_mlme_data_stru));
	if (mlme_data == NULL)
		return -HISI_EFAIL;
	mlme_data->data = NULL;
	mlme_data->data_len = data_len;
	mlme_data->send_action_cookie = &(drv->send_action_cookie);
	if ((data != NULL) && (data_len != 0)) {
		mlme_data->data = (uint8 *)os_zalloc(data_len);
		if (mlme_data->data == NULL) {
			os_free(mlme_data);
			mlme_data = NULL;
			return -HISI_EFAIL;
		}
		rc = memcpy_s(mlme_data->data, data_len, data, data_len);
		if (rc != EOK) {
			os_free(mlme_data->data);
			mlme_data->data = NULL;
			os_free(mlme_data);
			return -HISI_EFAIL;
		}
	}
	ret = hisi_ioctl_send_mlme(drv->iface, mlme_data);
	os_free(mlme_data->data);
	mlme_data->data = NULL;
	os_free(mlme_data);
	if (ret != HISI_SUCC)
		ret = -HISI_EFAIL;
	return ret;
}

void hisi_receive_eapol(void *ctx, const uint8 *src_addr, const uint8 *buf, uint32 len)
{
	hisi_driver_data_stru *drv = ctx;
	if ((ctx == NULL) || (src_addr == NULL) || (buf == NULL) || (len < sizeof(struct l2_ethhdr))) {
		wpa_error_log0(MSG_DEBUG, "hisi_receive_eapol invalid input.");
		return;
	}

	drv_event_eapol_rx(drv->ctx, src_addr, buf + sizeof(struct l2_ethhdr), len - sizeof(struct l2_ethhdr));
}

int32 hisi_send_eapol(void *priv, const uint8 *addr, const uint8 *data, size_t data_len,
                      int32 encrypt, const uint8 *own_addr, uint32 flags)
{
	hisi_driver_data_stru  *drv = priv;
	int32 ret;
	uint32 frame_len;
	uint8 *frame_buf = HISI_PTR_NULL;
	uint8 *payload = HISI_PTR_NULL;
	struct l2_ethhdr *l2_ethhdr = HISI_PTR_NULL;
	errno_t rc;
	(void)encrypt;
	(void)flags;

	if ((priv == NULL) || (addr == NULL) || (data == NULL) || (own_addr == NULL))
		return -HISI_EFAIL;

	frame_len = data_len + sizeof(struct l2_ethhdr);
	frame_buf = os_zalloc(frame_len);
	if (frame_buf == NULL)
		return -HISI_EFAIL;

	l2_ethhdr = (struct l2_ethhdr *)frame_buf;
	rc = memcpy_s(l2_ethhdr->h_dest, ETH_ADDR_LEN, addr, ETH_ADDR_LEN);
	if (rc != EOK) {
		os_free(frame_buf);
		return -HISI_EFAIL;
	}
	rc = memcpy_s(l2_ethhdr->h_source, ETH_ADDR_LEN, own_addr, ETH_ADDR_LEN);
	if (rc != EOK) {
		os_free(frame_buf);
		return -HISI_EFAIL;
	}
	l2_ethhdr->h_proto = host_to_be16(ETH_P_PAE);
	payload = (uint8 *)(l2_ethhdr + 1);
	rc = memcpy_s(payload, data_len, data, data_len);
	if (rc != EOK) {
		os_free(frame_buf);
		return -HISI_EFAIL;
	}
	ret = l2_packet_send(drv->eapol_sock, addr, ETH_P_EAPOL, frame_buf, frame_len);
	os_free(frame_buf);
	return ret;
}

static hisi_driver_data_stru * hisi_send_event_get_drv(const char *ifname)
{
	hisi_driver_data_stru *drv = NULL;
	struct hisi_wifi_dev *wifi_dev = hi_get_wifi_dev_by_name(ifname);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL))
		return NULL;
	if (wifi_dev->iftype == HI_WIFI_IFTYPE_AP) {
		if (eloop_is_running(ELOOP_TASK_HOSTAPD) != HISI_OK)
			return NULL;
		drv = ((struct hostapd_data*)(wifi_dev->priv))->drv_priv;
	} else {
		if (eloop_is_running(ELOOP_TASK_WPA) != HISI_OK)
			return NULL;
		drv = ((struct wpa_supplicant*)(wifi_dev->priv))->drv_priv;
	}
	return drv;
}

int32 hisi_driver_send_event(const char *ifname, int32 cmd, const uint8 *buf, uint32 length)
{
	int8 *data_ptr = NULL;
	int8 *packet = NULL;
	hisi_driver_data_stru *drv = NULL;
	unsigned int int_save;
	int32 ret;
	errno_t rc;

	if (ifname == NULL)
		return -HISI_EFAIL;

	drv = hisi_send_event_get_drv(ifname);
	if ((drv == NULL) || (drv->event_queue == NULL))
		return -HISI_EFAIL;
#ifndef LOS_CONFIG_P2P
	if (cmd == HISI_ELOOP_EVENT_TX_STATUS)
		return -HISI_EFAIL;
#endif /* LOS_CONFIG_P2P */
	if ((cmd == HISI_ELOOP_EVENT_RX_MGMT) && (g_rx_mgmt_count >= RX_MGMT_EVENT_MAX_COUNT)) {
		wpa_warning_log1(MSG_DEBUG, "rx_mgmt is full : %d\n\r", g_rx_mgmt_count);
		return -HISI_EFAIL;
	}
	packet = (int8 *)os_zalloc(length + 8);
	if (packet == NULL)
		return -HISI_EFAIL;

	data_ptr = packet;
	*(uint32 *)data_ptr = cmd;
	*(uint32 *)(data_ptr + 4) = length;
	data_ptr += 8;
	/* Append the Actual Data */
	if ((buf != NULL) && (length != 0)) {
		rc = memcpy_s(data_ptr, length, buf, length);
		if (rc != EOK) {
			os_free(packet);
			return -HISI_EFAIL;
		}
	}
	/* Post message and wakeup event */
	ret = eloop_post_event(drv->event_queue, (void *)packet, 1);
	if (ret != HISI_SUCC) {
		os_free(packet);
	} else if (cmd == HISI_ELOOP_EVENT_RX_MGMT) {
		int_save = LOS_IntLock();
		g_rx_mgmt_count++;
		LOS_IntRestore(int_save);
	}
	return ret;
}

static inline void hisi_driver_event_new_sta_process(const hisi_driver_data_stru *drv, int8 *data_ptr)
{
	hisi_new_sta_info_stru *new_sta_info = NULL;
	union wpa_event_data event;

	(void)memset_s(&event, sizeof(union wpa_event_data), 0, sizeof(union wpa_event_data));
	new_sta_info = (hisi_new_sta_info_stru *)data_ptr;
	if (is_zero_ether_addr((const uint8 *)new_sta_info->macaddr)) {
		wpa_supplicant_event(drv->ctx, EVENT_DISASSOC, NULL);
	} else {
		event.assoc_info.reassoc     = new_sta_info->reassoc;
		event.assoc_info.req_ies     = new_sta_info->ie;
		event.assoc_info.req_ies_len = new_sta_info->ielen;
		event.assoc_info.addr        = new_sta_info->macaddr;
		wpa_supplicant_event(drv->ctx, EVENT_ASSOC, &event);
	}

	if (new_sta_info->ie != NULL) {
		os_free(new_sta_info->ie);
		new_sta_info->ie = NULL;
	}
}

static inline void hisi_driver_event_del_sta_process(hisi_driver_data_stru *drv, int8 *data_ptr)
{
	union wpa_event_data event;
	event.disassoc_info.addr = (uint8 *)data_ptr;
	if (drv->ctx != NULL)
		wpa_supplicant_event(drv->ctx, EVENT_DISASSOC, &event);
}

static inline void hisi_driver_event_channel_switch_process(hisi_driver_data_stru *drv, int8 *data_ptr)
{
	union wpa_event_data event;
	event.ch_switch.freq = (int)(((hisi_ch_switch_stru *)data_ptr)->freq);
	if (drv->ctx != NULL)
		wpa_supplicant_event(drv->ctx, EVENT_CH_SWITCH, &event);
}

static void hisi_driver_event_scan_result_process(hisi_driver_data_stru *drv, int8 *data_ptr)
{
	hisi_scan_result_stru *scan_result = NULL;
	scan_result = (hisi_scan_result_stru *)data_ptr;
	struct wpa_scan_res *res = NULL;
	errno_t rc;
#ifndef HISI_SCAN_SIZE_CROP
	res = (struct wpa_scan_res *)os_zalloc(sizeof(struct wpa_scan_res) + scan_result->ie_len + scan_result->beacon_ie_len);
#else
	res = (struct wpa_scan_res *)os_zalloc(sizeof(struct wpa_scan_res) + scan_result->ie_len);
#endif /* HISI_SCAN_SIZE_CROP */
	if (res == NULL)
		goto FAILED;
	res->flags      = scan_result->flags;
	res->freq       = scan_result->freq;
	res->caps       = scan_result->caps;
#ifndef HISI_SCAN_SIZE_CROP
	res->beacon_int = scan_result->beacon_int;
	res->qual       = scan_result->qual;
#endif /* HISI_SCAN_SIZE_CROP */
	res->level      = scan_result->level;
	res->age        = scan_result->age;
	res->ie_len     = scan_result->ie_len;
#ifndef HISI_SCAN_SIZE_CROP
	res->beacon_ie_len = scan_result->beacon_ie_len;
#endif /* HISI_CODE_CROP */
	rc = memcpy_s(res->bssid, ETH_ADDR_LEN, scan_result->bssid, ETH_ADDR_LEN);
	if (rc != EOK)
		goto FAILED;
#ifndef HISI_SCAN_SIZE_CROP
	rc = memcpy_s(&res[1], scan_result->ie_len + scan_result->beacon_ie_len,
					scan_result->variable, scan_result->ie_len + scan_result->beacon_ie_len);
#else
	rc = memcpy_s(&res[1], scan_result->ie_len, scan_result->variable, scan_result->ie_len);
#endif /* HISI_SCAN_SIZE_CROP */
	if (rc != EOK)
		goto FAILED;
	if (drv->scan_ap_num >= SCAN_AP_LIMIT) {
		wpa_error_log0(MSG_ERROR, "hisi_driver_event_process: drv->scan_ap_num >= SCAN_AP_LIMIT");
		goto FAILED;
	}
	drv->res[drv->scan_ap_num++] = res;
	goto OUT;
FAILED:
	if (res != NULL)
		os_free(res);
OUT:
	os_free(scan_result->variable);
	scan_result->variable = NULL;
}

static void hisi_driver_event_connect_result_process(hisi_driver_data_stru *drv, int8 *data_ptr)
{
	hisi_connect_result_stru *accoc_info = NULL;
	accoc_info = (hisi_connect_result_stru *)data_ptr;
	union wpa_event_data event;
	errno_t rc;
	(void)memset_s(&event, sizeof(union wpa_event_data), 0, sizeof(union wpa_event_data));
	if (accoc_info->status != 0) {
		drv->associated = HISI_DISCONNECT;
		if ((accoc_info->status == WLAN_STATUS_INVALID_PMKID) ||
		    (accoc_info->status == WLAN_STATUS_AUTH_TIMEOUT)) {
			event.assoc_reject.status_code = accoc_info->status;
			wpa_supplicant_event(drv->ctx, EVENT_ASSOC_REJECT, &event);
		} else {
			event.disassoc_info.reason_code = accoc_info->status;
			wpa_supplicant_event(drv->ctx, EVENT_DISASSOC, &event);
		}
	} else {
		drv->associated = HISI_CONNECT;
		rc = memcpy_s(drv->bssid, ETH_ALEN, accoc_info->bssid, ETH_ALEN);
		if (rc != EOK) {
			os_free(accoc_info->resp_ie);
			accoc_info->resp_ie = NULL;
			return;
		}
		event.assoc_info.req_ies      = accoc_info->req_ie;
		event.assoc_info.req_ies_len  = accoc_info->req_ie_len;
		event.assoc_info.resp_ies     = accoc_info->resp_ie;
		event.assoc_info.resp_ies_len = accoc_info->resp_ie_len;
		event.assoc_info.addr         = accoc_info->bssid;
		event.assoc_info.freq         = accoc_info->freq;
		wpa_supplicant_event(drv->ctx, EVENT_ASSOC, &event);
	}
	if (accoc_info->req_ie != NULL) {
		os_free(accoc_info->req_ie);
		accoc_info->req_ie = NULL;
	}
	if (accoc_info->resp_ie != NULL) {
		os_free(accoc_info->resp_ie);
		accoc_info->resp_ie = NULL;
	}
}

static inline void hisi_driver_event_disconnect_process(hisi_driver_data_stru *drv, int8 *data_ptr)
{
	hisi_disconnect_stru *discon_info = NULL;
	union wpa_event_data event;
	(void)memset_s(&event, sizeof(union wpa_event_data), 0, sizeof(union wpa_event_data));

	drv->associated = HISI_DISCONNECT;
	discon_info = (hisi_disconnect_stru *)data_ptr;
	event.disassoc_info.reason_code = discon_info->reason;
	event.disassoc_info.ie          = discon_info->ie;
	event.disassoc_info.ie_len      = discon_info->ie_len;
	if (g_wpa_rm_network == HI_WPA_RM_NETWORK_START)
		g_wpa_rm_network = HI_WPA_RM_NETWORK_WORKING;
	wpa_supplicant_event(drv->ctx, EVENT_DISASSOC, &event);
	if (discon_info->ie != NULL) {
		os_free(discon_info->ie);
		discon_info->ie = NULL;
	}
}

static void hisi_driver_event_timeout_disc_process(hisi_driver_data_stru *drv)
{
	struct wpa_supplicant *wpa_s = NULL;
	struct hisi_wifi_dev *wifi_dev = hi_get_wifi_dev_by_name(drv->iface);
	if ((wifi_dev == NULL) || (wifi_dev->priv == NULL) || (wifi_dev->iftype != HI_WIFI_IFTYPE_STATION))
		return;
	wpa_s = wifi_dev->priv;
	if ((g_sta_delay_report_flag == WPA_FLAG_ON) && (wpa_s->wpa_state != WPA_COMPLETED)) {
		hi_at_printf("+NOTICE:DISCONNECTED\r\n");
		wpa_send_disconnect_delay_report_events();
	}
	g_sta_delay_report_flag = WPA_FLAG_OFF;
}

#ifdef CONFIG_WPA3
static void hisi_free_ext_auth(hisi_external_auth_stru *external_auth_stru)
{
	if (external_auth_stru == NULL)
		return;

	if (external_auth_stru->ssid != NULL) {
		os_free(external_auth_stru->ssid);
		external_auth_stru->ssid = NULL;
	}

	if (external_auth_stru->pmkid != NULL) {
		os_free(external_auth_stru->pmkid);
		external_auth_stru->pmkid = NULL;
	}
}

static void hisi_driver_event_external_auth(hisi_driver_data_stru *drv, int8 *data_ptr)
{
	hisi_external_auth_stru *external_auth_stru = (hisi_external_auth_stru *)data_ptr;
	union wpa_event_data event;
	(void)memset_s(&event, sizeof(union wpa_event_data), 0, sizeof(union wpa_event_data));
	event.external_auth.bssid = external_auth_stru->bssid;
	event.external_auth.ssid = external_auth_stru->ssid;
	event.external_auth.ssid_len = external_auth_stru->ssid_len;
	event.external_auth.pmkid = external_auth_stru->pmkid;
	event.external_auth.key_mgmt_suite = external_auth_stru->key_mgmt_suite;
	event.external_auth.status = external_auth_stru->status;
	event.external_auth.action = external_auth_stru->auth_action;
	wpa_supplicant_event(drv->ctx, EVENT_EXTERNAL_AUTH, &event);
	hisi_free_ext_auth(external_auth_stru);
}
#endif /* CONFIG_WPA3 */

static inline void hisi_driver_event_scan_done_process(hisi_driver_data_stru *drv, int8 *data_ptr)
{
	hisi_driver_scan_status_stru *status = (hisi_driver_scan_status_stru *)data_ptr;
	eloop_cancel_timeout(hisi_driver_scan_timeout, drv, drv->ctx);
	if (status->scan_status != HISI_SCAN_SUCCESS) {
		wpa_error_log1(MSG_ERROR, "hisi_driver_event_process: wifi driver scan not success(%d)", status->scan_status);
		return;
	}
	wpa_supplicant_event(drv->ctx, EVENT_SCAN_RESULTS, NULL);
}

static inline void hisi_driver_event_process_internal(hisi_driver_data_stru *drv, int8 *data_ptr, int32 cmd)
{
	union wpa_event_data event;
	wpa_warning_log1(MSG_DEBUG, "hisi_driver_event %d", cmd);
	(void)memset_s(&event, sizeof(union wpa_event_data), 0, sizeof(union wpa_event_data));
	switch (cmd) {
		case HISI_ELOOP_EVENT_NEW_STA:
			hisi_driver_event_new_sta_process(drv, data_ptr);
			break;
		case HISI_ELOOP_EVENT_DEL_STA:
			hisi_driver_event_del_sta_process(drv, data_ptr);
			break;
		case HISI_ELOOP_EVENT_RX_MGMT:
			hisi_rx_mgmt_process(drv->ctx, (hisi_rx_mgmt_stru *)data_ptr, &event);
			break;
		case HISI_ELOOP_EVENT_TX_STATUS:
			hisi_tx_status_process(drv->ctx, (hisi_tx_status_stru *)data_ptr, &event);
			break;
		case HISI_ELOOP_EVENT_SCAN_DONE:
			hisi_driver_event_scan_done_process(drv, data_ptr);
			break;
		case HISI_ELOOP_EVENT_SCAN_RESULT:
			hisi_driver_event_scan_result_process(drv, data_ptr);
			break;
		case HISI_ELOOP_EVENT_CONNECT_RESULT:
			hisi_driver_event_connect_result_process(drv, data_ptr);
			break;
		case HISI_ELOOP_EVENT_DISCONNECT:
			hisi_driver_event_disconnect_process(drv, data_ptr);
			break;
#ifdef LOS_CONFIG_MESH
		case HISI_ELOOP_EVENT_MESH_CLOSE:
			hisi_driver_event_mesh_close_process(drv, data_ptr);
			break;
#endif /* LOS_CONFIG_MESH */
		case HISI_ELOOP_EVENT_CHANNEL_SWITCH:
			hisi_driver_event_channel_switch_process(drv, data_ptr);
			break;
		case HISI_ELOOP_EVENT_TIMEOUT_DISCONN:
			hisi_driver_event_timeout_disc_process(drv);
			break;
#ifdef CONFIG_WPA3
		case HISI_ELOOP_EVENT_EXTERNAL_AUTH:
			hisi_driver_event_external_auth(drv, data_ptr);
			break;
#endif /* CONFIG_WPA3 */
		default:
			break;
	}
}

void hisi_driver_event_process(void *eloop_data, void *user_ctx)
{
	hisi_driver_data_stru *drv = NULL;
	void *packet = NULL;
	int8 *data_ptr = NULL;
	int32 cmd;

	(void)user_ctx;
	if (eloop_data == NULL)
		return;
	drv = eloop_data;
	for (;;) {
		packet = eloop_read_event(drv->event_queue, 0);
		if (packet == NULL)
			break;
		hi_cpup_load_check_proc(hi_task_get_current_id(), LOAD_SLEEP_TIME_DEFAULT);

		data_ptr = packet; /* setup a pointer to the payload */
		cmd = *(uint32 *)data_ptr;
		data_ptr += 8;
		if (data_ptr != NULL)
			hisi_driver_event_process_internal(drv, data_ptr, cmd);
		os_free(packet); /* release the packet */
		packet = NULL;
	}

	return;
}

void hisi_driver_ap_event_process(void *eloop_data, void *user_ctx)
{
	/* p_user_ctx may be NULL */
	if (eloop_data == NULL)
		return;
	hisi_driver_event_process(eloop_data, user_ctx);
}

void hisi_rx_mgmt_count_reset(void)
{
	unsigned int int_save;
	int_save = LOS_IntLock();
	g_rx_mgmt_count = 0;
	LOS_IntRestore(int_save);
}

static void hisi_rx_mgmt_count_decrease(void)
{
	unsigned int int_save;
	int_save = LOS_IntLock();
	g_rx_mgmt_count--;
	if (g_rx_mgmt_count < 0)
		g_rx_mgmt_count = 0;
	LOS_IntRestore(int_save);
}

void hisi_rx_mgmt_process(void *ctx, hisi_rx_mgmt_stru *rx_mgmt, union wpa_event_data *event)
{
	hisi_rx_mgmt_count_decrease();
	if ((ctx == NULL) || (rx_mgmt == NULL) || (event == NULL))
		return;
	if (rx_mgmt->buf == NULL)
		return;
	event->rx_mgmt.frame = rx_mgmt->buf;
	event->rx_mgmt.frame_len = rx_mgmt->len;
	event->rx_mgmt.ssi_signal = rx_mgmt->sig_mbm;
	event->rx_mgmt.freq = rx_mgmt->freq;

	wpa_supplicant_event(ctx, EVENT_RX_MGMT, event);

	if (rx_mgmt->buf != NULL) {
		os_free(rx_mgmt->buf);
		rx_mgmt->buf = NULL;
	}
	return;
}

void hisi_tx_status_process(void *ctx, hisi_tx_status_stru *tx_status, union wpa_event_data *event)
{
	uint16 fc;
	struct ieee80211_hdr *hdr = NULL;

	if ((ctx == NULL) || (tx_status == NULL) || (event == NULL))
		return;

	if (tx_status->buf == NULL)
		return;

	hdr = (struct ieee80211_hdr *)tx_status->buf;
	fc = le_to_host16(hdr->frame_control);

	event->tx_status.type = WLAN_FC_GET_TYPE(fc);
	event->tx_status.stype = WLAN_FC_GET_STYPE(fc);
	event->tx_status.dst = hdr->addr1;
	event->tx_status.data = tx_status->buf;
	event->tx_status.data_len = tx_status->len;
	event->tx_status.ack = (tx_status->ack != HISI_FALSE);

	wpa_supplicant_event(ctx, EVENT_TX_STATUS, event);

	if (tx_status->buf != NULL) {
		os_free(tx_status->buf);
		tx_status->buf = NULL;
	}
	return;
}

hisi_driver_data_stru * hisi_drv_init(void *ctx, const struct wpa_init_params *params)
{
	uint8 addr_tmp[ETH_ALEN] = { 0 };
	hisi_driver_data_stru *drv = HISI_PTR_NULL;
	errno_t rc;

	if ((ctx == NULL) || (params == NULL) || (params->ifname == NULL) || (params->own_addr == NULL))
		return NULL;
	drv = os_zalloc(sizeof(hisi_driver_data_stru));
	if (drv == NULL)
		goto INIT_FAILED;

	drv->ctx = ctx;
	rc = memcpy_s((int8 *)drv->iface, sizeof(drv->iface), params->ifname, sizeof(drv->iface));
	if (rc != EOK) {
		os_free(drv);
		drv = NULL;
		goto INIT_FAILED;
	}
	/*  just for ap drv init */
	if ((eloop_register_event(&drv->event_queue, sizeof(void *),
	                          hisi_driver_ap_event_process, drv, NULL)) != HISI_SUCC)
		goto INIT_FAILED;

	drv->eapol_sock = l2_packet_init((char *)drv->iface, NULL, ETH_P_EAPOL, hisi_receive_eapol, drv, 1);
	if (drv->eapol_sock == NULL)
		goto INIT_FAILED;

	if (l2_packet_get_own_addr(drv->eapol_sock, addr_tmp, sizeof(addr_tmp)))
		goto INIT_FAILED;

	/* The mac address is passed to the hostapd data structure for hostapd startup */
	rc = memcpy_s(params->own_addr, ETH_ALEN, addr_tmp, ETH_ALEN);
	if (rc != EOK)
		goto INIT_FAILED;
	rc = memcpy_s(drv->own_addr, ETH_ALEN, addr_tmp, ETH_ALEN);
	if (rc != EOK)
		goto INIT_FAILED;

	return drv;
INIT_FAILED:
	if (drv != NULL) {
		if (drv->eapol_sock != NULL)
			l2_packet_deinit(drv->eapol_sock);
		os_free(drv);
	}
	return NULL;
}

#define HISI_MEM_FREE(type, element) do { \
		if ((type)->element != NULL) { \
			os_free((type)->element); \
			(type)->element = NULL; \
		} \
	} while (0)

/* release wifi drive unreleased memory */
void hisi_drv_remove_mem(const hisi_driver_data_stru *drv)
{
	hisi_new_sta_info_stru *new_sta_info = NULL;
	hisi_rx_mgmt_stru *rx_mgmt = NULL;
	hisi_scan_result_stru *scan_result = NULL;
	hisi_connect_result_stru *accoc_info = NULL;
	hisi_disconnect_stru *discon_info = NULL;
	void *packet = NULL;
	int8 *data_ptr = NULL;
	int32 l_cmd;
	for (; ;) {
		packet = eloop_read_event(drv->event_queue, 0);
		if (packet == NULL)
			return;
		/* setup a pointer to the payload */
		data_ptr = packet;
		l_cmd    = *(uint32 *)data_ptr;
		data_ptr += 8;
		/* decode the buffer */
		switch (l_cmd) {
			case HISI_ELOOP_EVENT_NEW_STA:
				new_sta_info = (hisi_new_sta_info_stru *)data_ptr;
				HISI_MEM_FREE(new_sta_info, ie);
				break;
			case HISI_ELOOP_EVENT_RX_MGMT:
				hisi_rx_mgmt_count_decrease();
				rx_mgmt = (hisi_rx_mgmt_stru *)data_ptr;
				HISI_MEM_FREE(rx_mgmt, buf);
				break;
			case HISI_ELOOP_EVENT_SCAN_RESULT:
				scan_result = (hisi_scan_result_stru *)data_ptr;
				HISI_MEM_FREE(scan_result, variable);
				break;
			case HISI_ELOOP_EVENT_CONNECT_RESULT:
				accoc_info = (hisi_connect_result_stru *)data_ptr;
				HISI_MEM_FREE(accoc_info, req_ie);
				HISI_MEM_FREE(accoc_info, resp_ie);
				break;
			case HISI_ELOOP_EVENT_DISCONNECT:
				discon_info = (hisi_disconnect_stru *)data_ptr;
				HISI_MEM_FREE(discon_info, ie);
				break;
			default:
				break;
		}
		os_free(packet);
	}
}

void * hisi_hapd_init(struct hostapd_data *hapd, struct wpa_init_params *params)
{
	hisi_driver_data_stru *drv = NULL;
	hisi_bool_enum_uint8 status;
	int32 ret;
	int32 send_event_cb_reg_flag;

	if ((hapd == NULL) || (params == NULL) || (hapd->conf == NULL))
		return NULL;

	drv = hisi_drv_init(hapd, params);
	if (drv == NULL)
		return NULL;

	send_event_cb_reg_flag = hisi_register_send_event_cb(hisi_driver_send_event);
	drv->hapd = hapd;
	status = HI_TRUE;
	/* set netdev open or stop */
	ret = hisi_ioctl_set_netdev(drv->iface, &status);
	if (ret != HISI_SUCC)
		goto INIT_FAILED;
	return (void *)drv;
INIT_FAILED:
	if (send_event_cb_reg_flag == HISI_SUCC)
		hisi_register_send_event_cb(NULL);
	hisi_drv_deinit(drv);
	return NULL;
}

void hisi_drv_deinit(void *priv)
{
	hisi_driver_data_stru   *drv = HISI_PTR_NULL;
	if (priv == NULL)
		return;
	drv = (hisi_driver_data_stru *)priv;
	if (drv->eapol_sock != NULL)
		l2_packet_deinit(drv->eapol_sock);
	hisi_drv_remove_mem(drv);
	eloop_unregister_event(drv->event_queue, sizeof(void *));
	drv->event_queue = NULL;
	os_free(drv);
}

void hisi_hapd_deinit(void *priv)
{
	int32 ret;
	errno_t rc;
	hisi_driver_data_stru *drv = HISI_PTR_NULL;
	hisi_set_mode_stru set_mode;
	hisi_bool_enum_uint8 status;

	if (priv == NULL)
		return;

	rc = memset_s(&set_mode, sizeof(hisi_set_mode_stru), 0, sizeof(hisi_set_mode_stru));
	if (rc != EOK)
		return;
	drv = (hisi_driver_data_stru *)priv;
	set_mode.iftype = HI_WIFI_IFTYPE_STATION;
	status = HISI_FALSE;

	hisi_ioctl_set_netdev(drv->iface, &status);
	ret = hisi_ioctl_set_mode(drv->iface, &set_mode);
	if (ret != HISI_SUCC) {
		wpa_error_log0(MSG_ERROR, "hisi_hapd_deinit , hisi_ioctl_set_mode fail.");
		return;
	}
	if (hi_count_wifi_dev_in_use() >= WPA_DOUBLE_IFACE_WIFI_DEV_NUM) {
		wpa_error_log0(MSG_DEBUG, "wifidev in use");
	} else {
		hisi_register_send_event_cb(NULL);
	}
	hisi_drv_deinit(priv);
}

static void hisi_hw_feature_data_free(struct hostapd_hw_modes *modes, uint16 modes_num)
{
	uint16 loop;

	if (modes == NULL)
		return;
	for (loop = 0; loop < modes_num; ++loop) {
		if (modes[loop].channels != NULL) {
			os_free(modes[loop].channels);
			modes[loop].channels = NULL;
		}
		if (modes[loop].rates != NULL) {
			os_free(modes[loop].rates);
			modes[loop].rates = NULL;
		}
	}
	os_free(modes);
}

struct hostapd_hw_modes * hisi_get_hw_feature_data(void *priv, uint16 *num_modes, uint16 *flags)
{
	struct modes modes_data[] = { { 12, HOSTAPD_MODE_IEEE80211G }, { 4, HOSTAPD_MODE_IEEE80211B } };
	size_t loop;
	uint32 index;
	hisi_hw_feature_data_stru hw_feature_data;

	if ((priv == NULL) || (num_modes == NULL) || (flags == NULL))
		return NULL;
	(void)memset_s(&hw_feature_data, sizeof(hisi_hw_feature_data_stru), 0, sizeof(hisi_hw_feature_data_stru));
	hisi_driver_data_stru *drv = (hisi_driver_data_stru *)priv;
	*num_modes = 2; /* 2: mode only for 11b + 11g */
	*flags     = 0;

	if (hisi_ioctl_get_hw_feature(drv->iface, &hw_feature_data) != HISI_SUCC)
		return NULL;

	struct hostapd_hw_modes *modes = os_calloc(*num_modes, sizeof(struct hostapd_hw_modes));
	if (modes == NULL)
		return NULL;

	for (loop = 0; loop < *num_modes; ++loop) {
		modes[loop].channels = NULL;
		modes[loop].rates    = NULL;
	}

	modes[0].ht_capab = hw_feature_data.ht_capab;
	for (index = 0; index < sizeof(modes_data) / sizeof(struct modes); index++) {
		modes[index].mode         = modes_data[index].mode;
		modes[index].num_channels = hw_feature_data.channel_num;
		modes[index].num_rates    = modes_data[index].modes_num_rates;
		modes[index].channels     = os_calloc(hw_feature_data.channel_num, sizeof(struct hostapd_channel_data));
		modes[index].rates        = os_calloc(modes[index].num_rates, sizeof(int));
		if ((modes[index].channels == NULL) || (modes[index].rates == NULL)) {
			hisi_hw_feature_data_free(modes, *num_modes);
			return NULL;
		}

		for (loop = 0; loop < (size_t)hw_feature_data.channel_num; loop++) {
			modes[index].channels[loop].chan = hw_feature_data.iee80211_channel[loop].channel;
			modes[index].channels[loop].freq = hw_feature_data.iee80211_channel[loop].freq;
			modes[index].channels[loop].flag = hw_feature_data.iee80211_channel[loop].flags;
		}

		for (loop = 0; loop < (size_t)modes[index].num_rates; loop++)
			modes[index].rates[loop] = hw_feature_data.bitrate[loop];
	}
	return modes;
}

void * hisi_wpa_init(void *ctx, const char *ifname, void *global_priv)
{
	int32 ret;
	int32 send_event_cb_reg_flag = HISI_FAIL;
	hisi_bool_enum_uint8 status = HISI_TRUE;
	hisi_set_mode_stru set_mode;

	(void)global_priv;
	if ((ctx == NULL) || (ifname == NULL))
		return NULL;

	(void)memset_s(&set_mode, sizeof(hisi_set_mode_stru), 0, sizeof(hisi_set_mode_stru));
	hisi_driver_data_stru *drv = os_zalloc(sizeof(hisi_driver_data_stru));
	if (drv == NULL)
		goto INIT_FAILED;
	drv->ctx = ctx;
	if (memcpy_s((int8 *)drv->iface, sizeof(drv->iface), ifname, sizeof(drv->iface)) != EOK)
		goto INIT_FAILED;
	send_event_cb_reg_flag = hisi_register_send_event_cb(hisi_driver_send_event);
	ret = eloop_register_event(&drv->event_queue, sizeof(void *), hisi_driver_event_process, drv, NULL);
	if (ret != HISI_SUCC)
		goto INIT_FAILED;

	drv->eapol_sock = l2_packet_init((char *)drv->iface, NULL, ETH_P_EAPOL, hisi_receive_eapol, drv, 1);
	if (drv->eapol_sock == NULL)
		goto INIT_FAILED;

	if (l2_packet_get_own_addr(drv->eapol_sock, drv->own_addr, sizeof(drv->own_addr)))
		goto INIT_FAILED;

	/* set netdev open or stop */
	ret = hisi_ioctl_set_netdev(drv->iface, &status);
	if (ret != HISI_SUCC)
		goto INIT_FAILED;
	return drv;

INIT_FAILED:
	if (send_event_cb_reg_flag == HISI_SUCC)
		hisi_register_send_event_cb(NULL);
	hisi_drv_deinit(drv);
	return NULL;
}

void hisi_wpa_deinit(void *priv)
{
	hisi_bool_enum_uint8 status;
	int32 ret;
	hisi_driver_data_stru *drv = HISI_PTR_NULL;
	hisi_set_mode_stru set_mode;
	errno_t rc;

	if ((priv == NULL))
		return;

	rc = memset_s(&set_mode, sizeof(hisi_set_mode_stru), 0, sizeof(hisi_set_mode_stru));
	if (rc != EOK)
		return;
	status = HISI_FALSE;
	drv = (hisi_driver_data_stru *)priv;
	eloop_cancel_timeout(hisi_driver_scan_timeout, drv, drv->ctx);

	ret = hisi_ioctl_set_netdev(drv->iface, &status);
	if (ret != HISI_SUCC)
		wpa_error_log0(MSG_DEBUG, "hisi_wpa_deinit, close netdev fail");

	if (hi_count_wifi_dev_in_use() >= WPA_DOUBLE_IFACE_WIFI_DEV_NUM) {
		wpa_warning_log0(MSG_DEBUG, "wifidev in use");
	} else {
		hisi_register_send_event_cb(NULL);
	}
	hisi_drv_deinit(priv);
}

void hisi_driver_scan_timeout(void *eloop_ctx, void *timeout_ctx)
{
	(void)eloop_ctx;
	if (timeout_ctx == NULL)
		return;
	wpa_supplicant_event(timeout_ctx, EVENT_SCAN_RESULTS, NULL);
}

static void hisi_scan_free(hisi_scan_stru *scan_params)
{
	if (scan_params == NULL)
		return;

	if (scan_params->ssids != NULL) {
		os_free(scan_params->ssids);
		scan_params->ssids = NULL;
	}
	if (scan_params->bssid != NULL) {
		os_free(scan_params->bssid);
		scan_params->bssid = NULL;
	}

	if (scan_params->extra_ies != NULL) {
		os_free(scan_params->extra_ies);
		scan_params->extra_ies = NULL;
	}

	if (scan_params->freqs != NULL) {
		os_free(scan_params->freqs);
		scan_params->freqs = NULL;
	}

	os_free(scan_params);
}

int hisi_scan_process_ssid(struct wpa_driver_scan_params *params, hisi_scan_stru *scan_params)
{
	errno_t rc;
	size_t loop;
	if (params->num_ssids == 0)
		return -HISI_EFAIL;

	scan_params->num_ssids = params->num_ssids;
	/* scan_params->ssids freed by hisi_scan_free */
	scan_params->ssids = (hisi_driver_scan_ssid_stru *)os_zalloc(sizeof(hisi_driver_scan_ssid_stru) * params->num_ssids);
	if (scan_params->ssids == NULL)
		return -HISI_EFAIL;

	for (loop = 0; (loop < params->num_ssids) && (loop < WPAS_MAX_SCAN_SSIDS); loop++) {
		wpa_hexdump_ascii(MSG_MSGDUMP, "hisi: Scan SSID", params->ssids[loop].ssid, params->ssids[loop].ssid_len);

		if (params->ssids[loop].ssid_len > MAX_SSID_LEN) {
			wpa_error_log3(MSG_ERROR, "params->ssids[%d].ssid_len is %d, num %d:",
			               loop, params->ssids[loop].ssid_len, params->num_ssids);
			params->ssids[loop].ssid_len = MAX_SSID_LEN;
		}
		if (params->ssids[loop].ssid_len) {
			rc = memcpy_s(scan_params->ssids[loop].ssid, MAX_SSID_LEN,
			              params->ssids[loop].ssid, params->ssids[loop].ssid_len);
			if (rc != EOK)
				return -HISI_EFAIL;
		}
		scan_params->ssids[loop].ssid_len = params->ssids[loop].ssid_len;
	}

	return HISI_SUCC;
}

int hisi_scan_process_bssid(const struct wpa_driver_scan_params *params, hisi_scan_stru *scan_params)
{
	errno_t rc;
	if (params->bssid != NULL) {
		/* scan_params->bssid freed by hisi_scan_free */
		scan_params->bssid = (uint8 *)os_zalloc(ETH_ALEN);
		if (scan_params->bssid == NULL)
			return -HISI_EFAIL;
		rc = memcpy_s(scan_params->bssid, ETH_ALEN, params->bssid, ETH_ALEN);
		if (rc != EOK) {
			return -HISI_EFAIL;
		}
	}
	return HISI_SUCC;
}

int hisi_scan_process_extra_ies(const struct wpa_driver_scan_params *params, hisi_scan_stru *scan_params)
{
	errno_t rc;
	if ((params->extra_ies != NULL) && (params->extra_ies_len != 0)) {
		wpa_hexdump(MSG_MSGDUMP, "hisi: Scan extra IEs", params->extra_ies, params->extra_ies_len);
		/* scan_params->extra_ies freed by hisi_scan_free */
		scan_params->extra_ies = (uint8 *)os_zalloc(params->extra_ies_len);
		if (scan_params->extra_ies == NULL)
			return -HISI_EFAIL;

		rc = memcpy_s(scan_params->extra_ies, params->extra_ies_len, params->extra_ies, params->extra_ies_len);
		if (rc != EOK) {
			return -HISI_EFAIL;
		}
		scan_params->extra_ies_len = params->extra_ies_len;
	}
	return HISI_SUCC;
}

int hisi_scan_process_freq(const struct wpa_driver_scan_params *params, hisi_scan_stru *scan_params)
{
	uint32 num_freqs;
	int32 *freqs = NULL;
	errno_t rc;

	if (params->freqs != NULL) {
		num_freqs = 0;
		freqs = params->freqs;

		/* Calculate the number of channels, non-zero is a valid value */
		for (; *freqs != 0; freqs++) {
			num_freqs++;
			if (num_freqs > 14)
				return -HISI_EFAIL;
		}
		scan_params->num_freqs = num_freqs;
		/* scan_params->freqs freed by hisi_scan_free */
		scan_params->freqs = (int32 *)os_zalloc(num_freqs * (sizeof(int32)));
		if (scan_params->freqs == NULL)
			return -HISI_EFAIL;
		rc = memcpy_s(scan_params->freqs, num_freqs * (sizeof(int32)), params->freqs, num_freqs * (sizeof(int32)));
		if (rc != EOK) {
			return -HISI_EFAIL;
		}
	}
	return HISI_SUCC;
}

int32 hisi_scan(void *priv, struct wpa_driver_scan_params *params)
{
	hisi_scan_stru *scan_params = NULL;
	hisi_driver_data_stru *drv = NULL;
	int32 timeout;
	int32 ret;

	if ((priv == NULL) || (params == NULL) || (params->num_ssids > WPAS_MAX_SCAN_SSIDS))
		return -HISI_EFAIL;
	drv = (hisi_driver_data_stru *)priv;
	scan_params = (hisi_scan_stru *)os_zalloc(sizeof(hisi_scan_stru));
	if (scan_params == NULL)
		return -HISI_EFAIL;

	if ((hisi_scan_process_ssid(params, scan_params) != HISI_SUCC) ||
	    (hisi_scan_process_bssid(params, scan_params) != HISI_SUCC) ||
	    (hisi_scan_process_extra_ies(params, scan_params) != HISI_SUCC) ||
	    (hisi_scan_process_freq(params, scan_params) != HISI_SUCC)) {
		hisi_scan_free(scan_params);
		return -HISI_EFAIL;
	}
	scan_params->fast_connect_flag = g_fast_connect_flag ? WPA_FLAG_ON : WPA_FLAG_OFF;
	scan_params->prefix_ssid_scan_flag = g_ssid_prefix_flag;
	wpa_error_log1(MSG_ERROR, "prefix_ssid_scan_flag = %d", scan_params->prefix_ssid_scan_flag);
	ret = hisi_ioctl_scan(drv->iface, scan_params);
	hisi_scan_free(scan_params);

	timeout = SCAN_TIME_OUT;
	eloop_cancel_timeout(hisi_driver_scan_timeout, drv, drv->ctx);
	eloop_register_timeout(timeout, 0, hisi_driver_scan_timeout, drv, drv->ctx);

	return ret;
}

/*****************************************************************************
* Name         : hisi_get_scan_results
* Description  : get scan results
* Input param  : void *priv
* Return value : struct wpa_scan_results *
*****************************************************************************/
struct wpa_scan_results * hisi_get_scan_results(void *priv)
{
	struct wpa_scan_results *results = HISI_PTR_NULL;
	hisi_driver_data_stru *drv = priv;
	uint32 loop;
	errno_t rc;

	if ((priv == NULL))
		return NULL;

	results = (struct wpa_scan_results *)os_zalloc(sizeof(struct wpa_scan_results));
	if (results == NULL)
		return NULL;

	results->num = drv->scan_ap_num;
	if (results->num == 0) {
		os_free(results);
		return NULL;
	}
	results->res = (struct wpa_scan_res **)os_zalloc(results->num * sizeof(struct wpa_scan_res *));
	if (results->res == NULL) {
		os_free(results);
		return NULL;
	}
	rc = memcpy_s(results->res, results->num * sizeof(struct wpa_scan_res *),
	              drv->res, results->num * sizeof(struct wpa_scan_res *));
	if (rc != EOK) {
		os_free(results->res);
		os_free(results);
		return NULL;
	}
	drv->scan_ap_num = 0;
	for (loop = 0; loop < SCAN_AP_LIMIT; loop++)
		drv->res[loop] = NULL;
	wpa_warning_log1(MSG_DEBUG, "Received scan results (%u BSSes)", (uint32) results->num);
	return results;
}

/*****************************************************************************
* Name         : hisi_cipher_to_cipher_suite
* Description  : get cipher suite from cipher
* Input param  : uint32 cipher
* Return value : uint32
*****************************************************************************/
uint32 hisi_cipher_to_cipher_suite(uint32 cipher)
{
	switch (cipher) {
		case WPA_CIPHER_CCMP_256:
			return RSN_CIPHER_SUITE_CCMP_256;
		case WPA_CIPHER_GCMP_256:
			return RSN_CIPHER_SUITE_GCMP_256;
		case WPA_CIPHER_CCMP:
			return RSN_CIPHER_SUITE_CCMP;
		case WPA_CIPHER_GCMP:
			return RSN_CIPHER_SUITE_GCMP;
		case WPA_CIPHER_TKIP:
			return RSN_CIPHER_SUITE_TKIP;
		case WPA_CIPHER_WEP104:
			return RSN_CIPHER_SUITE_WEP104;
		case WPA_CIPHER_WEP40:
			return RSN_CIPHER_SUITE_WEP40;
		case WPA_CIPHER_GTK_NOT_USED:
			return RSN_CIPHER_SUITE_NO_GROUP_ADDRESSED;
		default:
			return 0;
	}
}

void hisi_set_conn_keys(const struct wpa_driver_associate_params *wpa_params, hisi_associate_params_stru *params)
{
	int32 loop;
	uint8 privacy = 0; /* The initial value means unencrypted */
	errno_t rc;

	if ((wpa_params == NULL) || (params == NULL))
		return;

	for (loop = 0; loop < 4; loop++) {
		if (wpa_params->wep_key[loop] == NULL)
			continue;
		privacy = 1;
		break;
	}

	if ((wpa_params->wps == WPS_MODE_PRIVACY) ||
		((wpa_params->pairwise_suite != 0) && (wpa_params->pairwise_suite != WPA_CIPHER_NONE)))
		privacy = 1;
	if (privacy == 0)
		return;
	params->privacy = privacy;
	for (loop = 0; loop < 4; loop++) {
		if (wpa_params->wep_key[loop] == NULL)
			continue;

		params->key_len = wpa_params->wep_key_len[loop];
		params->key = (uint8 *)os_zalloc(params->key_len);
		if (params->key == NULL)
			return;

		rc = memcpy_s(params->key, params->key_len, wpa_params->wep_key[loop], params->key_len);
		if (rc != EOK) {
			os_free(params->key);
			params->key = NULL;
			return;
		}
		params->key_idx = wpa_params->wep_tx_keyidx;
		break;
	}

	return;
}

static void hisi_connect_free(hisi_associate_params_stru *assoc_params)
{
	if (assoc_params == NULL)
		return;

	if (assoc_params->ie != NULL) {
		os_free(assoc_params->ie);
		assoc_params->ie = NULL;
	}
	if (assoc_params->crypto != NULL) {
		os_free(assoc_params->crypto);
		assoc_params->crypto = NULL;
	}
	if (assoc_params->ssid != NULL) {
		os_free(assoc_params->ssid);
		assoc_params->ssid = NULL;
	}
	if (assoc_params->bssid != NULL) {
		os_free(assoc_params->bssid);
		assoc_params->bssid = NULL;
	}
	if (assoc_params->key != NULL) {
		os_free(assoc_params->key);
		assoc_params->key = NULL;
	}

	os_free(assoc_params);
}

static int hisi_assoc_params_set(hisi_driver_data_stru *drv,
                                 struct wpa_driver_associate_params *params,
                                 hisi_associate_params_stru *assoc_params)
{
	if (params->bssid != NULL) {
		assoc_params->bssid = (uint8 *)os_zalloc(ETH_ALEN); /* freed by hisi_connect_free */
		if (assoc_params->bssid == NULL)
			return -HISI_EFAIL;

		if (memcpy_s(assoc_params->bssid, ETH_ALEN, params->bssid, ETH_ALEN) != EOK)
			return -HISI_EFAIL;
	}

	if (params->freq.freq != 0)
		assoc_params->freq = params->freq.freq;

	assoc_params->auto_conn = g_sta_delay_report_flag;

	if (params->ssid_len > MAX_SSID_LEN)
		params->ssid_len = MAX_SSID_LEN;

	if ((params->ssid != NULL) && (params->ssid_len != 0)) {
		assoc_params->ssid = (uint8 *)os_zalloc(params->ssid_len); /* freed by hisi_connect_free */
		if (assoc_params->ssid == NULL)
			return -HISI_EFAIL;
		assoc_params->ssid_len = params->ssid_len;
		if (memcpy_s(assoc_params->ssid, assoc_params->ssid_len, params->ssid, params->ssid_len) != EOK)
			return -HISI_EFAIL;
		if (memset_s(drv->ssid, MAX_SSID_LEN, 0, MAX_SSID_LEN) != EOK)
			return -HISI_EFAIL;
		if (memcpy_s(drv->ssid, MAX_SSID_LEN, params->ssid, params->ssid_len) != EOK)
			return -HISI_EFAIL;
		drv->ssid_len = params->ssid_len;
	}

	if ((params->wpa_ie != NULL) && (params->wpa_ie_len != 0)) {
		assoc_params->ie = (uint8 *)os_zalloc(params->wpa_ie_len); /* freed by hisi_connect_free */
		if (assoc_params->ie == NULL)
			return -HISI_EFAIL;
		assoc_params->ie_len = params->wpa_ie_len;
		if (memcpy_s(assoc_params->ie, assoc_params->ie_len, params->wpa_ie, params->wpa_ie_len) != EOK)
			return -HISI_EFAIL;
	}

	return HISI_SUCC;
}

static int hisi_assoc_param_crypto_set(const struct wpa_driver_associate_params *params,
                                       hisi_associate_params_stru *assoc_params)
{
	unsigned int wpa_ver = 0;
	uint32 akm_suites_num = 0;
	uint32 ciphers_pairwise_num = 0;
	int32 mgmt = RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X;
	/* assoc_params->crypto freed by hisi_connect_free */
	assoc_params->crypto = (hisi_crypto_settings_stru *)os_zalloc(sizeof(hisi_crypto_settings_stru));
	if (assoc_params->crypto == NULL)
		return -HISI_EFAIL;

	if (params->wpa_proto != 0) {
		if (params->wpa_proto & WPA_PROTO_WPA)
			wpa_ver |= (unsigned int)HISI_WPA_VERSION_1;
		if (params->wpa_proto & WPA_PROTO_RSN)
			wpa_ver |= (unsigned int)HISI_WPA_VERSION_2;
		assoc_params->crypto->wpa_versions = wpa_ver;
	}

	if (params->pairwise_suite != WPA_CIPHER_NONE) {
		assoc_params->crypto->ciphers_pairwise[ciphers_pairwise_num++]
			= hisi_cipher_to_cipher_suite(params->pairwise_suite);
		assoc_params->crypto->n_ciphers_pairwise = ciphers_pairwise_num;
	}

	if (params->group_suite != WPA_CIPHER_NONE)
		assoc_params->crypto->cipher_group = hisi_cipher_to_cipher_suite(params->group_suite);

	if (params->key_mgmt_suite == WPA_KEY_MGMT_PSK ||
	    params->key_mgmt_suite == WPA_KEY_MGMT_SAE ||
	    params->key_mgmt_suite == WPA_KEY_MGMT_PSK_SHA256) {
		switch (params->key_mgmt_suite) {
			case WPA_KEY_MGMT_PSK_SHA256:
				mgmt = RSN_AUTH_KEY_MGMT_PSK_SHA256;
				break;
			case WPA_KEY_MGMT_SAE:
				mgmt = RSN_AUTH_KEY_MGMT_SAE;
				break;
			case WPA_KEY_MGMT_PSK: /* fall through */
			default:
				mgmt = RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X;
				break;
		}
		assoc_params->crypto->akm_suites[akm_suites_num++] = mgmt;
		assoc_params->crypto->n_akm_suites = akm_suites_num;
	}

	return HISI_SUCC;
}

int32 hisi_try_connect(hisi_driver_data_stru *drv, struct wpa_driver_associate_params *params)
{
	hisi_associate_params_stru *assoc_params = HISI_PTR_NULL;
	hisi_auth_type_enum type = HISI_AUTHTYPE_BUTT;
	int32 algs = 0;

	if ((drv == NULL) || (params == NULL))
		return -HISI_EFAIL;

	assoc_params = (hisi_associate_params_stru *)os_zalloc(sizeof(hisi_associate_params_stru));
	if (assoc_params == NULL)
		goto failure;

	if (hisi_assoc_params_set(drv, params, assoc_params) != HISI_SUCC)
		goto failure;

	if (hisi_assoc_param_crypto_set(params, assoc_params) != HISI_SUCC)
		goto failure;

	assoc_params->mfp = params->mgmt_frame_protection;
	wpa_error_log1(MSG_ERROR, "hisi try connect: pmf = %d", assoc_params->mfp);

	if ((unsigned int)(params->auth_alg) & WPA_AUTH_ALG_OPEN)
		algs++;
	if ((unsigned int)(params->auth_alg) & WPA_AUTH_ALG_SHARED)
		algs++;
	if ((unsigned int)(params->auth_alg) & WPA_AUTH_ALG_LEAP)
		algs++;
	if (algs > 1) {
		assoc_params->auth_type = HISI_AUTHTYPE_AUTOMATIC;
		goto skip_auth_type;
	}

	if ((unsigned int)params->auth_alg & WPA_AUTH_ALG_OPEN)
		type = HISI_AUTHTYPE_OPEN_SYSTEM;
	else if ((unsigned int)params->auth_alg & WPA_AUTH_ALG_SHARED)
		type = HISI_AUTHTYPE_SHARED_KEY;
	else if ((unsigned int)params->auth_alg & WPA_AUTH_ALG_LEAP)
		type = HISI_AUTHTYPE_NETWORK_EAP;
	else if ((unsigned int)params->auth_alg & WPA_AUTH_ALG_FT)
		type = HISI_AUTHTYPE_FT;
	else if ((unsigned int)params->auth_alg & WPA_AUTH_ALG_SAE)
		type = HISI_AUTHTYPE_SAE;
	else
		goto failure;

	assoc_params->auth_type = type;

skip_auth_type:
	hisi_set_conn_keys(params, assoc_params);
	if (hisi_ioctl_assoc(drv->iface, assoc_params) != HISI_SUCC)
		goto failure;

	wpa_warning_log0(MSG_DEBUG, "Connect request send successfully");

	hisi_connect_free(assoc_params);
	return HISI_SUCC;

failure:
	hisi_connect_free(assoc_params);
	return -HISI_EFAIL;
}

int32 hisi_connect(hisi_driver_data_stru *drv, struct wpa_driver_associate_params *params)
{
	int32 ret;
	if ((drv == NULL) || (params == NULL))
		return -HISI_EFAIL;

	ret = hisi_try_connect(drv, params);
	if (ret != HISI_SUCC) {
		if (hisi_disconnect(drv, WLAN_REASON_PREV_AUTH_NOT_VALID))
			return -HISI_EFAIL;
		ret = hisi_try_connect(drv, params);
	}
	return ret;
}


int32 hisi_associate(void *priv, struct wpa_driver_associate_params *params)
{
	hisi_driver_data_stru *drv = priv;
	if ((drv == NULL) || (params == NULL))
		return -HISI_EFAIL;
	return hisi_connect(drv, params);
}

int32 hisi_disconnect(hisi_driver_data_stru *drv, uint16 reason_code)
{
	int32 ret;
	uint16 new_reason_code;
	if (drv == NULL)
		return -HISI_EFAIL;
	new_reason_code = reason_code;
	ret = hisi_ioctl_disconnet(drv->iface, &new_reason_code);
	if (ret == HISI_SUCC)
		drv->associated = HISI_DISCONNECT;
	return ret;
}

int32 hisi_deauthenticate(void *priv, const uint8 *addr, uint16 reason_code)
{
	hisi_driver_data_stru *drv = priv;
	(void)addr;
	if (priv == NULL)
		return -HISI_EFAIL;
	return hisi_disconnect(drv, reason_code);
}

int32 hisi_get_bssid(void *priv, uint8 *bssid)
{
	hisi_driver_data_stru *drv = priv;
	errno_t rc;
	if ((priv == NULL) || (bssid == NULL))
		return -HISI_EFAIL;
	if (drv->associated == HISI_DISCONNECT)
		return -HISI_EFAIL;

	rc = memcpy_s(bssid, ETH_ALEN, drv->bssid, ETH_ALEN);
	if (rc != EOK)
		return -HISI_EFAIL;
	return HISI_SUCC;
}

int32 hisi_get_ssid(void *priv, uint8 *ssid)
{
	hisi_driver_data_stru *drv = priv;
	errno_t rc;
	if ((priv == NULL) || (ssid == NULL))
		return -HISI_EFAIL;
	if (drv->associated == HISI_DISCONNECT)
		return -HISI_EFAIL;
	rc = memcpy_s(ssid, MAX_SSID_LEN, drv->ssid, drv->ssid_len);
	if (rc != EOK)
		return -HISI_EFAIL;
	return (int32)drv->ssid_len;
}

int32 hisi_get_drv_flags(void *priv, uint64 *drv_flags)
{
	hisi_driver_data_stru *drv = priv;
	hisi_get_drv_flags_stru *params = NULL;
	int32 ret;

	if ((priv == NULL) || (drv_flags == NULL))
		return -HISI_EFAIL;

	/* get drv_flags from the driver layer */
	params = (hisi_get_drv_flags_stru *)os_zalloc(sizeof(hisi_get_drv_flags_stru));
	if (params == NULL)
		return -HISI_EFAIL;
	params->drv_flags = 0;

	ret = hisi_ioctl_get_drv_flags(drv->iface, params);
	if (ret != HISI_SUCC) {
		wpa_error_log0(MSG_ERROR, "hisi_get_drv_flags: hisi_ioctl_get_drv_flags failed.");
		os_free(params);
		return -HISI_EFAIL;
	}
	*drv_flags = params->drv_flags;
	os_free(params);
	return ret;
}

const uint8 * hisi_get_mac_addr(void *priv)
{
	hisi_driver_data_stru *drv = priv;
	if (priv == NULL)
		return NULL;
	return drv->own_addr;
}

int32 hisi_wpa_send_eapol(void *priv, const uint8 *dest, uint16 proto, const uint8 *data, uint32 data_len)
{
	hisi_driver_data_stru *drv = priv;
	int32 ret;
	uint32 frame_len;
	uint8 *frame_buf = HISI_PTR_NULL;
	uint8 *payload = HISI_PTR_NULL;
	struct l2_ethhdr *l2_ethhdr = HISI_PTR_NULL;
	errno_t rc;

	if ((priv == NULL) || (data == NULL) || (dest == NULL)) {
		return -HISI_EFAIL;
	}

	frame_len = data_len + sizeof(struct l2_ethhdr);
	frame_buf = os_zalloc(frame_len);
	if (frame_buf == NULL)
		return -HISI_EFAIL;

	l2_ethhdr = (struct l2_ethhdr *)frame_buf;
	rc = memcpy_s(l2_ethhdr->h_dest, ETH_ADDR_LEN, dest, ETH_ADDR_LEN);
	if (rc != EOK) {
		os_free(frame_buf);
		return -HISI_EFAIL;
	}
	rc = memcpy_s(l2_ethhdr->h_source, ETH_ADDR_LEN, drv->own_addr, ETH_ADDR_LEN);
	if (rc != EOK) {
		os_free(frame_buf);
		return -HISI_EFAIL;
	}
	l2_ethhdr->h_proto = host_to_be16(proto);

	payload = (uint8 *)(l2_ethhdr + 1);
	rc = memcpy_s(payload, data_len, data, data_len);
	if (rc != EOK) {
		os_free(frame_buf);
		return -HISI_EFAIL;
	}
	ret = l2_packet_send(drv->eapol_sock, dest, host_to_be16(proto), frame_buf, frame_len);
	os_free(frame_buf);
	return ret;
}

#ifdef _PRE_WLAN_FEATURE_REKEY_OFFLOAD
void hisi_set_rekey_info(void *priv, const uint8 *kek, size_t kek_len,
                         const uint8 *kck, size_t kck_len, const uint8 *replay_ctr)
{
	hisi_driver_data_stru *drv = HISI_PTR_NULL;
	hisi_rekey_offload_stru *rekey_offload = HISI_PTR_NULL;
	errno_t rc;
	if ((priv == HISI_PTR_NULL) || (kek == HISI_PTR_NULL) || (kck == HISI_PTR_NULL) || (replay_ctr == HISI_PTR_NULL))
		return;
	drv = (hisi_driver_data_stru *)priv;
	rekey_offload = os_zalloc(sizeof(hisi_rekey_offload_stru));
	if (rekey_offload == HISI_PTR_NULL)
		return;
	rc = memcpy_s(rekey_offload->kck, HISI_REKEY_OFFLOAD_KCK_LEN, kck, kck_len);
	rc |= memcpy_s(rekey_offload->kek, HISI_REKEY_OFFLOAD_KEK_LEN, kek, kek_len);
	rc |= memcpy_s(rekey_offload->replay_ctr, HISI_REKEY_OFFLOAD_REPLAY_COUNTER_LEN,
	               replay_ctr, HISI_REKEY_OFFLOAD_REPLAY_COUNTER_LEN);
	if (rc != EOK) {
		os_free(rekey_offload);
		return;
	}
	if (hisi_ioctl_set_rekey_info(drv->iface, rekey_offload) != HISI_SUCC)
		wpa_error_log0(MSG_ERROR, "hisi_set_rekey_info set rekey info failed!");
	os_free(rekey_offload);
	return;
}
#endif

/* hisi_dup_macaddr: malloc mac addr buffer, should be used with os_free()  */
uint8 * hisi_dup_macaddr(const uint8 *addr, size_t len)
{
	uint8 *res = NULL;
	errno_t rc;
	if (addr == NULL)
		return NULL;
	res = (uint8 *)os_zalloc(ETH_ADDR_LEN);
	if (res == NULL)
		return NULL;
	rc = memcpy_s(res, ETH_ADDR_LEN, addr, len);
	if (rc) {
		os_free(res);
		return NULL;
	}
	return res;
}

#ifdef CONFIG_MESH
int32 hisi_mesh_enable_flag(const char *ifname,
			enum hisi_mesh_enable_flag_type flag_type, uint8 enable_flag)
{
	int32 ret;
	hisi_mesh_enable_flag_stru *enable_flag_stru = NULL;
	if (ifname == NULL)
		return -HISI_EFAIL;
	enable_flag_stru = os_zalloc(sizeof(hisi_mesh_enable_flag_stru));
	if (enable_flag_stru == NULL)
		return -HISI_EFAIL;
	enable_flag_stru->enable_flag = enable_flag;
	ret = hisi_ioctl_mesh_enable_flag((int8 *)ifname, flag_type, enable_flag_stru);
	os_free(enable_flag_stru);
	return ret;
}
#endif /* CONFIG_MESH */

int32 hisi_set_usr_app_ie(const char *ifname, uint8 set, hi_wifi_frame_type fram_type,
                          uint8 ie_type, const uint8 *ie, uint16 ie_len)
{
	int32 ret;
	hisi_usr_app_ie_stru *app_ie = NULL;
	if ((ifname == NULL) || (set && ((ie == NULL) || (ie_len == 0)))) {
		wpa_error_log0(MSG_ERROR, "hisi_set_usr_app_ie: input error.");
		return -HISI_EFAIL;
	}
	app_ie = (hisi_usr_app_ie_stru *)os_zalloc(sizeof(hisi_usr_app_ie_stru));
	if (app_ie == NULL)
		return -HISI_EFAIL;
	app_ie->set = set;
	app_ie->ie_type = ie_type;
	app_ie->ie_len = ie_len;
	app_ie->frame_type = fram_type;
	if (app_ie->set) {
		app_ie->ie = os_zalloc(app_ie->ie_len);
		if (app_ie->ie == NULL) {
			os_free(app_ie);
			return -HISI_EFAIL;
		}
		if (memcpy_s(app_ie->ie, app_ie->ie_len, ie, ie_len) != EOK) {
			ret = -HISI_EFAIL;
			goto FAIL;
		}
	}
	wpa_error_log4(MSG_DEBUG, "hisi_set_usr_app_ie fram_type %d, type %d, ie_len %d, set %d",
	               app_ie->frame_type, app_ie->ie_type, app_ie->ie_len, app_ie->set);
	ret = hisi_ioctl_set_usr_app_ie(ifname, (const void *)app_ie);
FAIL:
	os_free(app_ie->ie);
	app_ie->ie = NULL;
	os_free(app_ie);
	return ret;
}

int32 hisi_set_delay_report(const char *ifname, int enable_flag)
{
	int32 ret;
	hisi_delay_report_stru *delay_report_stru = NULL;
	if ((ifname == NULL) || ((enable_flag != WPA_FLAG_OFF) && (enable_flag != WPA_FLAG_ON)))
		return -HISI_EFAIL;
	delay_report_stru = os_zalloc(sizeof(hisi_delay_report_stru));
	if (delay_report_stru == NULL)
		return -HISI_EFAIL;
	delay_report_stru->enable = enable_flag;
	delay_report_stru->timeout = DELAY_REPORT_TIMEOUT;
	ret = hisi_ioctl_set_delay_report((int8 *)ifname, delay_report_stru);
	if ((ret == HISI_SUCC) && (enable_flag == WPA_FLAG_ON))
		g_sta_delay_report_flag = WPA_FLAG_ON;
	os_free(delay_report_stru);
	return ret;
}

#ifdef CONFIG_WPA3
int32 hisi_send_ext_auth_status(void *priv, struct external_auth *params)
{
	int32 ret;
	hisi_driver_data_stru *drv = priv;
	hisi_external_auth_stru *external_auth_stru;

	if ((priv == NULL) || (params == NULL) || (params->bssid == NULL))
		return -HISI_EFAIL;
	external_auth_stru = os_zalloc(sizeof(hisi_external_auth_stru));
	if (external_auth_stru == NULL)
		return -HISI_EFAIL;
	if (memcpy_s(external_auth_stru->bssid, ETH_ADDR_LEN, params->bssid, ETH_ADDR_LEN) != EOK) {
		os_free(external_auth_stru);
		return -HISI_EFAIL;
	}
	external_auth_stru->status = params->status;
	ret = hisi_ioctl_send_ext_auth_status(drv->iface, external_auth_stru);
	os_free(external_auth_stru);
	return ret;
}
#endif /* CONFIG_WPA3 */

static void hisi_action_data_buf_free(hisi_action_data_stru *hisi_action_data)
{
	if (hisi_action_data == NULL)
		return;

	if (hisi_action_data->dst != NULL) {
		os_free(hisi_action_data->dst);
		hisi_action_data->dst = NULL;
	}
	if (hisi_action_data->src != NULL) {
		os_free(hisi_action_data->src);
		hisi_action_data->src = NULL;
	}
	if (hisi_action_data->bssid != NULL) {
		os_free(hisi_action_data->bssid);
		hisi_action_data->bssid = NULL;
	}
	if (hisi_action_data->data != NULL) {
		os_free(hisi_action_data->data);
		hisi_action_data->data = NULL;
	}
}

int32 hisi_send_action(void *priv, unsigned int freq, unsigned int wait, const u8 *dst, const u8 *src,
                       const u8 *bssid, const u8 *data, size_t data_len, int no_cck)
{
	hisi_action_data_stru hisi_action_data = {0};
	hisi_driver_data_stru *drv = NULL;
	int32 ret;
	(void)freq;
	(void)wait;
	(void)no_cck;
	ret = (priv == HISI_PTR_NULL) || (data == HISI_PTR_NULL) || (dst == HISI_PTR_NULL) || (src == HISI_PTR_NULL);
	if (ret)
		return -HISI_EFAIL;
	drv = (hisi_driver_data_stru *)priv;
 	hisi_action_data.data_len = data_len;
	hisi_action_data.dst = hisi_dup_macaddr(dst, ETH_ADDR_LEN);
	if (hisi_action_data.dst == NULL)
		return -HISI_EFAIL;
	hisi_action_data.src = hisi_dup_macaddr(src, ETH_ADDR_LEN);
	if (hisi_action_data.src == NULL) {
		hisi_action_data_buf_free(&hisi_action_data);
		return -HISI_EFAIL;
	}
	hisi_action_data.bssid = hisi_dup_macaddr(bssid, ETH_ADDR_LEN);
	if (hisi_action_data.bssid == NULL) {
		hisi_action_data_buf_free(&hisi_action_data);
		return -HISI_EFAIL;
	}
	hisi_action_data.data = (uint8 *)dup_binstr(data, data_len);
	if (hisi_action_data.data == NULL) {
		hisi_action_data_buf_free(&hisi_action_data);
		return -HISI_EFAIL;
	}
	ret = hisi_ioctl_send_action(drv->iface, &hisi_action_data);
	hisi_action_data_buf_free(&hisi_action_data);
	return ret;
}

int32 hisi_sta_remove(void *priv, const uint8 *addr)
{
	hisi_driver_data_stru *drv = HISI_PTR_NULL;
	int32 ret;

	if ((priv == HISI_PTR_NULL) || (addr == HISI_PTR_NULL))
		return -HISI_EFAIL;
	drv = (hisi_driver_data_stru *)priv;
	ret = hisi_ioctl_sta_remove(drv->iface, addr);
	if (ret != HISI_SUCC)
		return -HISI_EFAIL;
	return HISI_SUCC;
}

/* wpa_supplicant driver ops */
#ifndef WIN32
const struct wpa_driver_ops wpa_driver_hisi_ops =
{
	.name                     = "hisi",
	.desc                     = "hisi liteos driver",
	.get_bssid                = hisi_get_bssid,
	.get_ssid                 = hisi_get_ssid,
	.set_key                  = hisi_set_key,
	.scan2                    = hisi_scan,
	.get_scan_results2        = hisi_get_scan_results,
	.deauthenticate           = hisi_deauthenticate,
	.associate                = hisi_associate,
	.send_eapol               = hisi_wpa_send_eapol,
	.init2                    = hisi_wpa_init,
	.deinit                   = hisi_wpa_deinit,
	.set_ap                   = hisi_set_ap,
	.send_mlme                = hisi_send_mlme,
	.get_hw_feature_data      = hisi_get_hw_feature_data,
#ifdef CONFIG_MESH
	.init_mesh                = hisi_mesh_init,
	.sta_add                  = hisi_mesh_sta_add,
#endif /* CONFIG_MESH */
	.sta_remove               = hisi_sta_remove,
	.hapd_send_eapol          = hisi_send_eapol,
	.hapd_init                = hisi_hapd_init,
	.hapd_deinit              = hisi_hapd_deinit,
	.send_action              = hisi_send_action,
#ifdef _PRE_WLAN_FEATURE_REKEY_OFFLOAD
	.set_rekey_info           = hisi_set_rekey_info,
#else
	.set_rekey_info           = NULL,
#endif
#ifdef CONFIG_TDLS
	.send_tdls_mgmt           = NULL,
	.tdls_oper                = NULL,
#endif /* CONFIG_TDLS */
	.get_drv_flags            = hisi_get_drv_flags,
	.get_mac_addr             = hisi_get_mac_addr,
	.set_delay_report         = hisi_set_delay_report,
#ifdef CONFIG_WPA3
	.send_external_auth_status = hisi_send_ext_auth_status,
#endif /* CONFIG_WPA3 */

#ifdef ANDROID
	.driver_cmd               = NULL,
#endif /* ANDROID */
};
#endif

#ifdef __cplusplus
#if __cplusplus
	}
#endif
#endif
