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

#include "hi_mesh.h"
#include "utils/eloop.h"
#include "config.h"
#include "bss.h"
#include "ctrl_iface.h"
#include "wifi_api.h"
#include "ap/sta_info.h"
#include "ap/hostapd.h"
#include "ap/ieee802_11.h"
#include "driver_i.h"
#include "mesh_mpm.h"
#include "mesh_rsn.h"
#include "mesh.h"
#ifdef LOS_CONFIG_MESH_GTK
#include "ap/wpa_auth.h"
#include "ap/wpa_auth_i.h"
#endif /* LOS_CONFIG_MESH_GTK */
#include "hi_at.h"
#include "securec.h"

int g_mesh_reject_peer_flag = 0;
int g_mesh_connect_flag = 0;
int g_mesh_try_connect_count = 0;
#ifndef LOS_CONFIG_MESH_TRIM
int g_mesh_auto_peer_flag = WPA_FLAG_OFF;
#endif /* LOS_CONFIG_MESH_TRIM */

u8 * hi_hostapd_eid_mesh_cfg(const struct hostapd_data *hapd, u8 *eid)
{
	struct mesh_conf *mconf = NULL;
	u8 *pos = eid;
	if (!g_mesh_flag) {
		return pos;
	}
	if ((hapd == NULL) || (hapd->iface == NULL) || (hapd->iface->mconf == NULL))
		return pos;
	mconf = hapd->iface->mconf;
	*pos++ = WLAN_EID_MESH_CONFIG;
	*pos++ = HI_MESH_CONFIG_IE_LENGTH;
	*pos++ = mconf->mesh_pp_id;
	*pos++ = mconf->mesh_pm_id;
	*pos++ = mconf->mesh_cc_id;
	*pos++ = mconf->mesh_sp_id;
	*pos++ = mconf->mesh_auth_id;
	u8 info = (hapd->num_plinks > WPA_MESH_MAX_PLINK_NUM ? WPA_MESH_MAX_PLINK_NUM : hapd->num_plinks) << 1;
	/* TODO: Add Connected to Mesh Gate/AS subfields */
	*pos++ = info;
	/* always forwarding & accepting plinks for now */
	*pos++ = MESH_CAP_ACCEPT_ADDITIONAL_PEER;
	return pos;
}


u8 * hi_hostapd_eid_mesh_id(const struct hostapd_data *hapd, u8 *eid)
{
	struct mesh_conf *mconf = NULL;
	u8 *pos = eid;
	if (!g_mesh_flag || (hapd == NULL) || (hapd->iface == NULL) || (hapd->iface->mconf == NULL)) {
		return pos;
	}
	mconf = hapd->iface->mconf;
	*pos++ = WLAN_EID_MESH_ID;
	*pos++ = mconf->meshid_len;

	if (mconf->meshid_len > 0) {
		if (memcpy_s(pos, MAX_SSID_LEN, mconf->meshid, mconf->meshid_len) != EOK) {
			wpa_error_log0(MSG_ERROR, "hi_hostapd_eid_mesh_id memcpy_s fail.");
			return NULL;
		}
		pos += WPA_MIN(MAX_SSID_LEN, mconf->meshid_len);
	}
	return pos;
}

void hi_wpa_mesh_add_scan_req_ie(const struct wpa_supplicant *wpa_s, struct wpabuf **extra_ie)
{
	if (!g_mesh_sta_flag || (wpa_s->ifmsh != NULL) || (extra_ie == NULL)) {
		return;
	}
	/* EID + LEN + IE value */
	size_t ielen = HI_MESH_PROBE_REQ_IE_LENGTH + 2;

	if (wpabuf_resize(extra_ie, ielen) != 0) {
		return;
	}
	wpabuf_put_u8(*extra_ie, WLAN_EID_VENDOR_SPECIFIC);
	wpabuf_put_u8(*extra_ie, HI_MESH_PROBE_REQ_IE_LENGTH);
	wpabuf_put_be24(*extra_ie, WPA_WLAN_OUI_HUAWEI);
	wpabuf_put_u8(*extra_ie, WPA_OUITYPE_MESH);
	wpabuf_put_u8(*extra_ie, WPA_OUISUBTYPE_MESH_HISI_STA_REQ);
}

void hi_wpa_mesh_add_action_ie(const struct wpa_supplicant *wpa_s, struct wpabuf *buf)
{
	if (wpa_s->ifmsh == NULL) {
		wpa_error_log0(MSG_ERROR, "hi_wpa_mesh_add_action_ie is not needed.");
		return;
	}

	wpa_error_log0(MSG_ERROR, "hi_wpa_mesh_add_action_ie enter.");

	wpabuf_put_u8(buf, WLAN_EID_VENDOR_SPECIFIC);
	/* IE value length */
	wpabuf_put_u8(buf, WPA_OUI_HISI_MESH_LENGTH);
	wpabuf_put_be32(buf, WPA_OUI_HISI_MESH);
}

int hi_wpa_find_hisi_mesh_ie(const u8 *start, size_t len)
{
	size_t left = len;
	const u8 *pos = start;
	while (left >= 2) {
		u8 id, elen;
		/* EID + LEN + IE value */
		id = *pos++;
		elen = *pos++;
		left -= 2;

		if (elen > left) {
			return WPA_FLAG_OFF;
		}
		if ((id == WLAN_EID_VENDOR_SPECIFIC) && (elen >= 4) && (WPA_GET_BE32(pos) == WPA_OUI_HISI_MESH)) {
			wpa_error_log0(MSG_ERROR, "hi_wpa_find_hisi_mesh_ie is finded.");
			return WPA_FLAG_ON;
		}
		left -= elen;
		pos += elen;
	}
	return WPA_FLAG_OFF;
}

const u8 * hi_wpa_get_hisi_mesh_opt_ie(const u8 *start, size_t len)
{
	size_t left = len;
	const u8 *pos = start;
	if (pos == NULL)
		return NULL;
	while (left >= 2) {
		u8 id, elen;
		/* EID + LEN + IE value */
		id = pos[0];
		elen = pos[1];
		left -= 2;

		if (elen > left) {
			return NULL;
		}
		if ((id == WLAN_EID_VENDOR_SPECIFIC) && (elen == WPA_MESH_HISI_OPTIMIZATION_VAL_LEN) &&
			(WPA_GET_BE32(&pos[2]) == WPA_OUI_HISI_MESH) && (pos[6] == WPA_OUISUBTYPE_MESH_HISI_OPTIMIZATION)) {
			wpa_error_log0(MSG_ERROR, "hi_wpa_get_hisi_mesh_opt_ie is finded.");
			return pos;
		}
		left -= elen;
		pos += 2 + elen;
	}
	return NULL;
}

int hi_wpa_get_mesh_ap_param(const struct wpa_bss *bss, struct hi_mesh_ap_param* mesh_ap_param)
{
	const u8 *hisi_mesh_config = NULL;
	const u8 *hisi_mesh_opt = NULL;
	if ((bss == NULL) || (mesh_ap_param == NULL))
		return HISI_FAIL;
	hisi_mesh_opt = hi_wpa_get_hisi_mesh_opt_ie((const u8 *) (bss + 1), bss->ie_len);
	if (hisi_mesh_opt == NULL)
		return HISI_FAIL;
	mesh_ap_param->mesh_bcn_priority = hisi_mesh_opt[WPA_MESH_HISI_OPT_BCN_PRIO_OFFSET];
	mesh_ap_param->mesh_is_mbr = !!hisi_mesh_opt[WPA_MESH_HISI_OPT_IS_MBR_OFFSET];
	mesh_ap_param->accept_sta = !!hisi_mesh_opt[WPA_MESH_HISI_OPT_ACPT_STA_OFFSET];
	mesh_ap_param->mesh_rank = hisi_mesh_opt[WPA_MESH_HISI_OPT_MESH_RANK_OFFSET] |
		(hisi_mesh_opt[WPA_MESH_HISI_OPT_MESH_RANK_OFFSET + 1] << 8); // 8 is mov bits
	mesh_ap_param->switch_status = hisi_mesh_opt[WPA_MESH_HISI_OPT_SWITCH_STATUS_OFFSET];
	hisi_mesh_config = wpa_bss_get_ie(bss, WLAN_EID_MESH_CONFIG);
	if ((hisi_mesh_config != NULL) && (hisi_mesh_config[1] == HI_MESH_CONFIG_IE_LENGTH)) {
		mesh_ap_param->peering_num = (hisi_mesh_config[WPA_MESH_CONFIG_PEERING_NUM_OFFSET] >> 1) & WPA_MESH_PEERNUM_MSK;
		mesh_ap_param->accept_peer = hisi_mesh_config[WPA_MESH_CONFIG_MESH_CAPABILITY_OFFSET] & WPA_BIT0;
	}
	return HISI_OK;
}

int wpa_mesh_ap_config_set(const struct wpa_ssid *ssid)
{
	errno_t rc;
	rc = memset_s(&g_global_conf, sizeof(struct hostapd_conf), 0, sizeof(struct hostapd_conf));
	rc |= memcpy_s(g_global_conf.driver, MAX_DRIVER_NAME_LEN, "hisi", WPA_STR_LEN("hisi"));
	rc |= memcpy_s(g_global_conf.ssid, MAX_SSID_LEN, ssid->ssid, ssid->ssid_len);
	if (rc != EOK) {
		wpa_error_log1(MSG_ERROR, "wpa_mesh_ap_config_set memcpy_s or memset_s failed(%d).", rc);
		return HISI_FAIL;
	}
	(void)hi_freq_to_channel(ssid->frequency, &g_global_conf.channel_num);
	g_global_conf.ignore_broadcast_ssid  = ssid->ignore_broadcast_ssid;
	g_global_conf.wpa_key_mgmt = ssid->key_mgmt;
	if (g_global_conf.wpa_key_mgmt != WPA_KEY_MGMT_NONE) {
		g_global_conf.wpa_key_mgmt = WPA_KEY_MGMT_PSK;
		g_global_conf.wpa_pairwise = WPA_CIPHER_CCMP;
		g_global_conf.wpa = WPA_PROTO_WPA | WPA_PROTO_RSN;
		rc = memcpy_s(g_global_conf.key, WPA_MAX_KEY_LEN + 1, ssid->passphrase, os_strlen(ssid->passphrase));
		if (rc != EOK) {
			wpa_error_log1(MSG_ERROR, "wpa_mesh_ap_config_set key memcpy_s failed(%d).", rc);
			return HISI_FAIL;
		}
	}
	g_ap_opt_set.hw_mode = HI_WIFI_PHY_MODE_11BGN;
	return 0;
}

#ifndef LOS_CONFIG_MESH_TRIM
static struct sta_info* hi_event_mesh_mpm_add_peer(struct wpa_supplicant *wpa_s,
					   const u8 *addr, u8 mesh_bcn_priority, u8 mesh_is_mbr)
{
	struct hostapd_sta_add_params params;
	struct mesh_conf *conf = wpa_s->ifmsh->mconf;
	struct hostapd_data *data = wpa_s->ifmsh->bss[0];
	struct sta_info *sta = NULL;
	int ret;

	sta = ap_get_sta(data, addr);
	if (sta == NULL) {
		sta = ap_sta_add(data, addr);
		if (sta == NULL)
			return NULL;
	}

	/* Set WMM by default since Mesh STAs are QoS STAs */
	sta->flags |= WLAN_STA_WMM;
	sta->mesh_bcn_priority = mesh_bcn_priority;
	sta->mesh_is_mbr = mesh_is_mbr;
	if (!sta->my_lid)
		mesh_mpm_init_link(wpa_s, sta);

	if (hostapd_get_aid(data, sta) < 0) {
		wpa_msg(wpa_s, MSG_ERROR, "No AIDs available");
		ap_free_sta(data, sta);
		return NULL;
	}

	/* insert into driver */
	(VOID)memset_s(&params, sizeof(params), 0, sizeof(params));
	params.supp_rates = sta->supported_rates;
	params.supp_rates_len = sta->supported_rates_len;
	params.addr = addr;
	params.plink_state = sta->plink_state;
	params.aid = sta->aid;
	params.peer_aid = sta->peer_aid;
	params.listen_interval = 100;
	params.flags |= WPA_STA_WMM;
	params.flags_mask |= WPA_STA_AUTHENTICATED;
	if (conf->security == MESH_CONF_SEC_NONE) {
		params.flags |= (WPA_STA_AUTHORIZED | WPA_STA_AUTHENTICATED);
	} else {
		sta->flags |= WLAN_STA_MFP;
		params.flags |= WPA_STA_MFP;
	}
	params.mesh_bcn_priority = sta->mesh_bcn_priority;
	params.mesh_is_mbr       = sta->mesh_is_mbr;
	ret = wpa_drv_sta_add(wpa_s, &params);
	if (ret) {
		wpa_msg(wpa_s, MSG_ERROR, "Driver failed to insert " MACSTR ": %d", MAC2STR(addr), ret);
		ap_free_sta(data, sta);
		return NULL;
	}
	return sta;
}

void hi_event_new_mesh_peer(struct wpa_supplicant *wpa_s, const u8 *addr, u8 mesh_bcn_priority, u8 mesh_is_mbr)
{
	if ((wpa_s == NULL) || (wpa_s->ifmsh == NULL) || (wpa_s->ifmsh->mconf == NULL) || (addr == NULL))
		return;
	struct mesh_conf *conf = wpa_s->ifmsh->mconf;
	struct hostapd_data *data = wpa_s->ifmsh->bss[0];
	struct wpa_ssid *ssid = wpa_s->current_ssid;
	struct sta_info *sta = ap_get_sta(data, addr);
	if (sta != NULL) {
		wpa_error_log0(MSG_ERROR, "hi_event_new_mesh_peer: the peer has been add into the sta_lists.");
		return;
	}
	sta = hi_event_mesh_mpm_add_peer(wpa_s, addr, mesh_bcn_priority, mesh_is_mbr);
	if (sta == NULL)
		return;
	sta->mesh_initiative_peering = WPA_FLAG_ON;
	if (ssid && ssid->no_auto_peer &&
	    (is_zero_ether_addr(data->mesh_required_peer) || os_memcmp(data->mesh_required_peer, addr, ETH_ALEN) != 0)) {
		wpa_msg(wpa_s, MSG_INFO, "will not initiate new peer link with " MACSTR " because of no_auto_peer", MAC2STR(addr));
		if (data->mesh_pending_auth != NULL) {
			struct os_reltime age;
			const struct ieee80211_mgmt *mgmt = NULL;
			struct hostapd_frame_info fi;

			mgmt = wpabuf_head(data->mesh_pending_auth);
			os_reltime_age(&data->mesh_pending_auth_time, &age);
			if ((age.sec < 2) && os_memcmp(mgmt->sa, addr, ETH_ALEN) == 0) {
				wpa_warning_log2(MSG_DEBUG, "mesh: Process pending Authentication frame from %u.%06u seconds ago",
				                (unsigned int) age.sec, (unsigned int) age.usec);
				(void)memset_s(&fi, sizeof(fi), 0, sizeof(fi));
				(void)ieee802_11_mgmt(data, wpabuf_head(data->mesh_pending_auth), wpabuf_len(data->mesh_pending_auth), &fi);
			}
			wpabuf_free(data->mesh_pending_auth);
			data->mesh_pending_auth = NULL;
		}
		return;
	}

	if (conf->security == MESH_CONF_SEC_NONE) {
		if ((sta->plink_state < PLINK_OPN_SNT) || (sta->plink_state > PLINK_ESTAB))
			mesh_mpm_plink_open(wpa_s, sta, PLINK_OPN_SNT);
	} else {
		mesh_rsn_auth_sae_sta(wpa_s, sta);
	}
}
#endif /* LOS_CONFIG_MESH_TRIM */

int wpas_mesh_searching(const struct wpa_supplicant *wpa_s)
{
	if ((wpa_s->ifmsh != NULL) || g_mesh_sta_flag) {
		return HISI_OK;
	}
	return HISI_FAIL;
}

/* Vendor Spec |Length| OUI|TYPE|SUBTYPE|MESHID LEN|MESHID|
		1	  |  1   |  3 |  1 |   1   |	 1	| Var | */
u8 * hi_wpa_scan_get_mesh_id(const struct wpa_scan_res *res)
{
	u8 *end   = NULL;
	u8 *pos   = NULL;
	if (res == NULL)
		return NULL;
	pos = (u8 *) (res + 1);
	end = pos + res->ie_len;

	while (end - pos > 1) {
		if (2 + pos[1] > end - pos) {
			break;
		}
		if ((pos[0] == WLAN_EID_VENDOR_SPECIFIC) &&
			(WPA_GET_BE32(&pos[2]) == WPA_OUI_HISI_MESH) &&
			(pos[6] == WPA_OUISUBTYPE_MESH_HISI_MESHID)) {
				return pos + 6;
			}
		pos += 2 + pos[1];
	}
	return NULL;
}

struct wpa_ssid * hi_mesh_scan_res_match(struct wpa_supplicant *wpa_s, int i, struct wpa_bss *bss,
												 struct wpa_ssid *group, int only_first_ssid)
{
	struct wpa_ssid *ssid = NULL;
	const u8 *match_ssid = NULL;
	size_t match_ssid_len;

	(void)i;
	match_ssid = bss->ssid;
	match_ssid_len = bss->ssid_len;

	for (ssid = group; ssid != NULL; ssid = (only_first_ssid ? NULL : ssid->next)) {
		int check_ssid = 1;
		if (wpas_network_disabled(wpa_s, ssid))
			continue;

		if (ssid->bssid_set && (ssid->ssid_len == 0) && (os_memcmp(bss->bssid, ssid->bssid, ETH_ALEN) == 0))
			check_ssid = 0;
		if (check_ssid && (ssid->ssid != NULL) && ((match_ssid_len != ssid->ssid_len) ||
			 (os_memcmp(match_ssid, ssid->ssid, match_ssid_len) != 0)))
			continue;

		if (ssid->bssid_set && (os_memcmp(bss->bssid, ssid->bssid, ETH_ALEN) != 0))
			continue;

		if (wpa_bss_get_vendor_ie(bss, WPA_OUI_HISI_MESH) == NULL)
			continue;
		if (!wpa_supplicant_match_privacy(bss, ssid))
			continue;

#ifdef CONFIG_MESH
		if ((ssid->mode == IEEE80211_MODE_MESH) && (ssid->frequency > 0) && (ssid->frequency != bss->freq))
			continue;
#endif /* CONFIG_MESH */

		/* Matching configuration found */
		return ssid;
	}
	/* No matching configuration found */
	return NULL;
}

struct wpa_ssid * hi_wpa_scan_res_match(struct wpa_supplicant *wpa_s,
					 int i, struct wpa_bss *bss,
					 struct wpa_ssid *group,
					 int only_first_ssid)
{
	struct wpa_ssid *ssid = NULL;
	if ((wpa_s == NULL) || (bss == NULL) || (group == NULL)) {
		return NULL;
	}
	struct hisi_wifi_dev *wifi_dev = hi_get_wifi_dev_by_priv(wpa_s);
	if (wifi_dev == NULL) {
		return NULL;
	}
	if (((wifi_dev->iftype == HI_WIFI_IFTYPE_MESH_POINT) && (group->mode == IEEE80211_MODE_MESH)) ||
	  ((wpa_bss_get_vendor_ie(bss, WPA_OUI_HISI_MESH) != NULL) && (group->mode == IEEE80211_MODE_INFRA) &&
	   (wifi_dev->iftype == HI_WIFI_IFTYPE_STATION) && g_mesh_sta_flag)) {
		ssid = hi_mesh_scan_res_match(wpa_s, i, bss, group, only_first_ssid);
	} else {
		ssid = wpa_scan_res_match(wpa_s, i, bss, group, only_first_ssid);
	}
	return ssid;
}

static int wpa_mesh_scan(struct wpa_supplicant *wpa_s) {
	int ret;
	char buf[WPA_EXTERNED_SSID_LEN] = { 0 };
	struct hisi_wifi_dev *wifi_dev = NULL;
	struct wpa_ssid *ssid = NULL;
	char reply[WPA_EXTERNED_SSID_LEN] = { 0 };
	const int reply_size = WPA_EXTERNED_SSID_LEN;
	int reply_len;
	char *pos = buf;
	wifi_dev = hi_get_wifi_dev_by_priv(wpa_s);
	if (wifi_dev == NULL) {
		wpa_error_log0(MSG_ERROR, "wpa_mesh_scan: get wifi dev failed\n");
		return HISI_FAIL;
	}
	ssid = wpa_config_get_network(wpa_s->conf, wifi_dev->network_id);
	if (ssid == NULL) {
		return HISI_FAIL;
	}
	ret = sprintf_s(pos, WPA_EXTERNED_SSID_LEN, "freq=%d", wpa_s->ifmsh->freq);
	if (ret < 0) {
		wpa_error_log0(MSG_ERROR, "wpa_mesh_scan sprintf_s freq faild");
		return HISI_FAIL;
	}
	pos += ret;
	ret = sprintf_s(pos, WPA_EXTERNED_SSID_LEN, " bssid="MACSTR, MAC2STR(ssid->bssid));
	if (ret < 0) {
		wpa_error_log0(MSG_ERROR, "wpa_mesh_scan sprintf_s bssid faild");
		return HISI_FAIL;
	}
	wpa_mesh_ctrl_scan(wpa_s, buf, reply, reply_size, &reply_len);
	return HISI_OK;
}

void hi_wpa_mesh_scan(void *ctx, void *data)
{
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)ctx;
	(void)data;
	if ((wpa_s == NULL) || (wpa_s->ifmsh == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wpa_mesh_scan: mesh is not start.");
		return;
	}
	if (wpa_mesh_scan(wpa_s) != HISI_OK) {
		wpa_error_log0(MSG_ERROR, "hi_wpa_mesh_scan fail.");
	}
}

void hi_wpa_meshap_req_scan(struct wpa_supplicant *wpa_s, int sec, int usec)
{
	int res;
	hi_wifi_event wpa_events = { 0 };
	if (g_mesh_connect_flag == WPA_FLAG_ON) {
		if (g_mesh_try_connect_count > WPA_MAX_MESH_CONNECT_TRY_CNT) {
			wpa_dbg(wpa_s, MSG_ERROR, "mesh connect canceled .");
			hi_wpa_join_mesh_completed(wpa_s);
			hi_at_printf("+NOTICE:PEER NOT FOUND\r\n");
			wpa_events.event = HI_WIFI_EVT_MESH_CANNOT_FOUND;
			if (g_wpa_event_cb != NULL)
				wifi_new_task_event_cb(&wpa_events);
			return;
		}
		g_mesh_try_connect_count += 1;
	}
	res = eloop_deplete_timeout(sec, usec, hi_wpa_mesh_scan, wpa_s,
					NULL);
	if (res == 1) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Rescheduling mesh scan request: %d.%06d sec",
			sec, usec);
	} else if (res == 0) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Ignore new mesh scan request for %d.%06d sec"
			    " since an earlier request is scheduled to trigger sooner", sec, usec);
	} else {
		wpa_dbg(wpa_s, MSG_DEBUG, "Setting mesh scan request: %d.%06d sec",
			sec, usec);
		(void)eloop_register_timeout(sec, usec, hi_wpa_mesh_scan, wpa_s, NULL);
	}
}

/* Vendor Spec |Length| OUI|TYPE|SUBTYPE|MESH PRIORITY|
		1	  |  1   |  3 |  1 |   1   |	   1	 | */
int wpa_bss_scan_get_mesh_priority(const struct wpa_bss *bss, u8 *mesh_priority)
{
	const u8 *pos   = (const u8 *) (bss + 1);
	const u8 *end   = pos + bss->ie_len;
	while ((end - pos) > 1) {
		if ((2 + pos[1]) > (end - pos)) {
			break;
		}
		if ((pos[0] == WLAN_EID_VENDOR_SPECIFIC) && (pos[1] == WPA_MESH_HISI_OPTIMIZATION_VAL_LEN) &&
			(WPA_GET_BE32(&pos[2]) == WPA_OUI_HISI_MESH) &&
			(pos[6] == WPA_OUISUBTYPE_MESH_HISI_OPTIMIZATION)) {
			*mesh_priority = pos[7];
			return HISI_OK;
		}
		pos += 2 + pos[1];
	}
	return HISI_FAIL;
}

/* Compare function for sorting scan results when searching a mesh for
 * provisioning. Return >0 if @b is considered better. */
int wpa_bss_mesh_priority_compar(struct wpa_bss *bss_a, struct wpa_bss *bss_b)
{
	u8  mesh_priority_a = 0;
	u8  mesh_priority_b = 0;
	int ret_a;
	int ret_b;
	if ((bss_a == NULL) || (bss_b == NULL))
		return WPA_BSS_COMPAR_REMAIN;
	/* Optimization - check hisi mesh priority existence . */
	ret_a = wpa_bss_scan_get_mesh_priority(bss_a, &mesh_priority_a);
	ret_b = wpa_bss_scan_get_mesh_priority(bss_b, &mesh_priority_b);
	if ((ret_a == HISI_OK) && (ret_b != HISI_OK)) {
		return WPA_BSS_COMPAR_REMAIN;
	}
	if ((ret_a != HISI_OK) && (ret_b == HISI_OK)) {
		return WPA_BSS_COMPAR_CHANGE;
	}

	if (mesh_priority_a > mesh_priority_b) {
		return WPA_BSS_COMPAR_REMAIN;
	}
	if (mesh_priority_a < mesh_priority_b) {
		return WPA_BSS_COMPAR_CHANGE;
	}

	/* all things being equal, use signal level; if signal levels are
	 * identical, use quality values since some drivers may only report
	 * that value and leave the signal level zero */
	/* all things being equal, use signal level; */
#ifndef HISI_SCAN_SIZE_CROP
	if (bss_b->level == bss_a->level) {
		return bss_b->qual - bss_a->qual;
	}
#endif /* HISI_SCAN_SIZE_CROP */
	return bss_b->level - bss_a->level;
}

void wpa_bss_mesh_scan_result_sort(struct wpa_supplicant *wpa_s)
{
	int i, j, len;
	struct wpa_bss *bss_a = NULL;
	struct wpa_bss *bss_b = NULL;
	struct dl_list *p = NULL;
	struct dl_list *q = NULL;
	struct dl_list *head = &wpa_s->bss_id;
	len = (int)dl_list_len(&wpa_s->bss);
	for (i = 0; i < (len - 1); i++) {
		p = head->next;
		q = p->next;
		for (j = 0; j < (len - 1 - i); j++) {
			bss_a = dl_list_entry(p, struct wpa_bss, list_id);
			bss_b = dl_list_entry(q, struct wpa_bss, list_id);

			if (wpa_bss_mesh_priority_compar(bss_a, bss_b) > 0) {
				p->prev->next = q;
				q->next->prev = p;
				q->prev = p->prev;
				p->prev = q;
				p->next = q->next;
				q->next = p;
				q = p->next;
			} else {
				p = p->next;
				q = p->next;
			}
		}
	}
}

void hi_wpa_mesh_peer(struct wpa_supplicant *wpa_s, struct wpa_bss *bss)
{
	u8 *addr = bss->bssid;
	u8 *ies = (u8 *)(bss + 1);
	size_t ie_len   = bss->ie_len;
	struct sta_info *sta = ap_get_sta(wpa_s->ifmsh->bss[0], addr);
	if (sta != NULL) {
		wpa_error_log0(MSG_ERROR, "hi_wpa_mesh_peer: the peer has been add into the sta_lists.");
		hi_wpa_join_mesh_completed(wpa_s);
		if (sta->plink_state == PLINK_ESTAB) {
			hi_at_printf("+NOTICE:MESH CONNECTED\r\n");
			hi_mesh_report_connect_results(sta, true, 0);
		}
		return;
	}

	eloop_cancel_timeout(hi_wpa_mesh_scan, wpa_s, NULL);

	if (g_mesh_reject_peer_flag) {
		wpa_error_log0(MSG_ERROR, "hi_wpa_mesh_peer: reject mesh peer now.");
		return;
	}

	wpa_mesh_notify_peer(wpa_s, addr, ies, ie_len);
}

void hi_wpa_rejoin_mesh(struct wpa_supplicant *wpa_s)
{
	if ((wpa_s == NULL) || (wpa_s->ifmsh == NULL) || (wpa_s->ifmsh->mconf == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wpa_mesh_peer: mesh is not start.");
		return;
	}
	g_mesh_connect_flag = WPA_FLAG_ON;
	g_mesh_try_connect_count = 0;
	hi_wpa_meshap_req_scan(wpa_s, 0, 0);
}

void hi_wpa_join_mesh(struct wpa_supplicant *wpa_s)
{
	if ((wpa_s == NULL) || (wpa_s->ifmsh == NULL) || (wpa_s->ifmsh->mconf == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_wpa_mesh_peer: mesh is not start.");
		return;
	}
	g_mesh_connect_flag = WPA_FLAG_ON;
	g_mesh_try_connect_count = 0;
	if (wpa_mesh_scan(wpa_s) != HISI_OK) {
		g_mesh_try_connect_count = 0;
		g_mesh_connect_flag = WPA_FLAG_OFF;
	}
}

void hi_wpa_join_mesh_completed(struct wpa_supplicant *wpa_s)
{
	eloop_cancel_timeout(hi_wpa_mesh_scan, wpa_s, NULL);
	g_mesh_connect_flag = WPA_FLAG_OFF;
	g_mesh_try_connect_count = 0;
}
void hi_mesh_deinit(void)
{
	g_mesh_reject_peer_flag = WPA_FLAG_OFF;
	g_mesh_connect_flag = WPA_FLAG_OFF;
#ifndef LOS_CONFIG_MESH_TRIM
	g_mesh_auto_peer_flag = WPA_FLAG_OFF;
#endif /* LOS_CONFIG_MESH_TRIM */
	g_mesh_try_connect_count = 0;
}

static void hi_mesh_mpm_send_plink_close_action_put_buf(struct wpa_supplicant *wpa_s,
	                                                    struct wpabuf *buf, const struct mesh_conf *conf)
{
	u8 ie_len;
	int ampe;

	ampe = conf->security & MESH_CONF_SEC_AMPE;

	wpabuf_put_u8(buf, WLAN_ACTION_SELF_PROTECTED);
	wpabuf_put_u8(buf, PLINK_CLOSE);

	/* Peer closing frame */
	/* IE: Mesh ID */
	wpabuf_put_u8(buf, WLAN_EID_MESH_ID);
	wpabuf_put_u8(buf, conf->meshid_len);
	wpabuf_put_data(buf, conf->meshid, conf->meshid_len);
	/* IE: hisi mesh OUI */
	hi_wpa_mesh_add_action_ie(wpa_s, buf);
	/* IE: Mesh Peering Management element */
	ie_len = 4;
	if (ampe) {
		ie_len += PMKID_LEN;
	}
	ie_len += 2;
	ie_len += 2; /* reason code */

	wpabuf_put_u8(buf, WLAN_EID_PEER_MGMT);
	wpabuf_put_u8(buf, ie_len);
	/* peering protocol */
	if (ampe)
		wpabuf_put_le16(buf, 1);
	else
		wpabuf_put_le16(buf, 0);
	wpabuf_put_le16(buf, 0);
	wpabuf_put_le16(buf, 0); /* add plid */
	wpabuf_put_le16(buf, WLAN_REASON_MESH_PEERING_CANCELLED);
	if (ampe) {
		wpabuf_put(buf, PMKID_LEN);
	}
}

void hi_mesh_mpm_send_plink_close_action(struct wpa_supplicant *wpa_s, const u8 *sta_addr)
{
	struct wpabuf *buf = NULL;
	struct hostapd_iface *ifmsh = NULL;
	struct mesh_conf *conf = NULL;
	int ret;
	size_t buf_len;
	if ((wpa_s == NULL) || (wpa_s->ifmsh == NULL) || (wpa_s->ifmsh->mconf == NULL)) {
		return;
	}
	ifmsh = wpa_s->ifmsh;
	conf = ifmsh->mconf;
	buf_len = 2 +	  /* Category and Action */
#ifndef CONFIG_DRIVER_HISILICON
		  2 +	  /* capability info */
#endif
		  2 +	  /* AID */
#ifndef CONFIG_DRIVER_HISILICON
		  2 + 8 +  /* supported rates */
		  2 + (32 - 8) +
#endif
		  2 + 32 + /* mesh ID */
		  2 + 4 +  /* hisi mesh OUI */
		  2 + 7 +  /* mesh config */
		  2 + 24 + /* peering management */
		  2 + 96 + /* AMPE */
		  2 + 16;  /* MIC */

	buf = wpabuf_alloc(buf_len);
	if (buf == NULL)
		return;

	hi_mesh_mpm_send_plink_close_action_put_buf(wpa_s, buf, conf);

	ret = wpa_drv_send_action(wpa_s, wpa_s->assoc_freq, 0,
				  sta_addr, wpa_s->own_addr, wpa_s->own_addr,
				  wpabuf_head(buf), wpabuf_len(buf), 0);
	if (ret < 0)
		wpa_error_log0(MSG_DEBUG, "mesh close action send failed");
	wpabuf_free(buf);
}

void hi_mesh_report_connect_results(struct sta_info *sta, bool is_connected, unsigned short reason_code)
{
	hi_wifi_event wpa_events = { 0 };
	if ((g_wpa_event_cb == NULL) || (sta == NULL))
		return;
	/* both mesh_connected or mesh_disconnected have addr member */
	(void)memcpy_s(wpa_events.info.mesh_connected.addr, ETH_ALEN, sta->addr, ETH_ALEN);
	wpa_events.event = (is_connected == true) ? HI_WIFI_EVT_MESH_CONNECTED : HI_WIFI_EVT_MESH_DISCONNECTED;
	if (is_connected != true)
		wpa_events.info.mesh_disconnected.reason_code = reason_code;
	wifi_new_task_event_cb(&wpa_events);
}

#ifdef LOS_CONFIG_MESH_GTK
void hi_mesh_gtk_rekey(struct hisi_wifi_dev *wifi_dev)
{
	struct mesh_rsn *rsn = NULL;
	struct sta_info *sta = NULL;
	struct wpa_supplicant *wpa_s = NULL;
	struct wpa_group *hapd_group = NULL;
	if ((wifi_dev->priv == NULL) || (g_hapd == NULL) || (g_hapd->wpa_auth == NULL)) {
		wpa_error_log0(MSG_ERROR, "hi_mesh_gtk_rekey: mesh ap is not ready.");
		return;
	}
	wpa_s = wifi_dev->priv;
	hapd_group = g_hapd->wpa_auth->group;
	rsn = wpa_s->mesh_rsn;
	if ((hapd_group == NULL) || (rsn == NULL)) {
		return;
	}
	/* mgtk update */
	if (memcpy_s(rsn->mgtk, WPA_TK_MAX_LEN, hapd_group->GTK[hapd_group->GN - 1], rsn->mgtk_len) != EOK) {
		return;
	}
	wpa_hexdump_key(MSG_DEBUG, "WPA: mesh Group Key", rsn->mgtk, rsn->mgtk_len);
	/* mgtk rekey */
	for (sta = g_hapd->sta_list; sta; sta = sta->next) {
		if (sta->plink_state == PLINK_ESTAB) {
			mesh_mpm_mgtk_rekey(wpa_s, sta);
		}
	}
	wpa_error_log0(MSG_ERROR, "hi_mesh_gtk_rekey done.");
}
#endif /* LOS_CONFIG_MESH_GTK */

int wpa_sta_event_check(enum wpa_event_type event)
{
	return ((event == EVENT_SCAN_RESULTS) ||
			(event == EVENT_RX_MGMT) ||
			(event == EVENT_CH_SWITCH));
}
