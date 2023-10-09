/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
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

#ifndef _WIFI_API_H_
#define _WIFI_API_H_

#include "hi_wifi_api.h"
#include "config_ssid.h"
#include "los_event.h"
#ifdef LOS_CONFIG_MESH
#include "hi_wifi_mesh_api.h"
#endif /* LOS_CONFIG_MESH */

#define WPA_MIN_KEY_LEN                 8
#define WPA_MAX_KEY_LEN                 64
#define WPA_HT_CAPA_LEN                 20
#define WPA_MAX_SSID_LEN                32
#define WPA_MAX_ESSID_LEN               WPA_MAX_SSID_LEN
#define WPA_AP_MIN_BEACON               33
#define WPA_AP_MAX_BEACON               1000
#define WPA_24G_FREQ_TXT_LEN            4
#define WPA_WEP40_KEY_LEN               5
#define WPA_WEP104_KEY_LEN              13
#define WPA_AP_MAX_DTIM                 30
#define WPA_AP_MIN_DTIM                 1
#define WPA_MAX_REKEY_TIME              86400
#define WPA_MIN_REKEY_TIME              30
#define WPA_DOUBLE_IFACE_WIFI_DEV_NUM   2
#define WPA_BASE_WIFI_DEV_NUM           1
#define WPA_MAX_WIFI_DEV_NUM            2
#define WPA_NETWORK_ID_TXT_LEN          sizeof(int)
#define WPA_CTRL_CMD_LEN                256
#define WPA_CMD_BUF_SIZE                64
#define WPA_MIN(x, y)                   (((x) < (y)) ? (x) : (y))
#define WPA_STR_LEN(x)                  (strlen(x) + 1)
#define WPA_EXTERNED_SSID_LEN           (WPA_MAX_SSID_LEN * 5)
#define WPA_CMD_BSSID_LEN               32
#define WPA_CMD_MIN_SIZE                16
#define WPA_MAX_NUM_STA                 8
#define WPA_SSID_SCAN_PREFEX_ENABLE     1
#define WPA_SSID_SCAN_PREFEX_DISABEL    0
#define WPA_P2P_INTENT_LEN              13
#define WPA_INT_TO_CHAR_LEN             11
#define WPA_P2P_PEER_CMD_LEN            27
#define WPA_P2P_MIN_INTENT              0
#define WPA_P2P_MAX_INTENT              15
#define WPA_DELAY_2S                    200
#define WPA_DELAY_3S                    300
#define WPA_DELAY_5S                    500
#define WPA_DELAY_10S                   1000
#define WPA_DELAY_60S                   6000
#define WPA_BIT0                        (1 << 0)
#define WPA_BIT1                        (1 << 1)
#define WPA_BIT2                        (1 << 2)
#define WPA_BIT3                        (1 << 3)
#define WPA_BIT4                        (1 << 4)
#define WPA_BIT5                        (1 << 5)
#define WPA_BIT6                        (1 << 6)
#define WPA_BIT7                        (1 << 7)
#define WPA_BIT8                        (1 << 8)
#define WPA_BIT9                        (1 << 9)
#define WPA_BIT10                       (1 << 10)
#define WPA_BIT11                       (1 << 11)
#define WPA_BIT12                       (1 << 12)
#define WPA_BIT13                       (1 << 13)
#define WPA_BIT14                       (1 << 14)
#define WPA_BIT15                       (1 << 15)
#define WPA_BIT16                       (1 << 16)
#define WPA_BIT17                       (1 << 17)
#define WPA_BIT18                       (1 << 18)
#define WPA_BIT19                       (1 << 19)
#define WPA_BIT20                       (1 << 20)
#define WPA_BIT21                       (1 << 21)
#define WPA_BIT22                       (1 << 22)
#define WPA_BIT23                       (1 << 23)
#define WPA_BIT24                       (1 << 24)
#define WPA_BIT26                       (1 << 26)
#define WPA_BIT27                       (1 << 27)
#define WPA_BIT28                       (1 << 28)
#define WPA_BIT29                       (1 << 29)
#define WPA_BIT30                       (1 << 30)

#define WPA_P2P_SCAN_MAX_CMD            32
#define WPA_P2P_IFNAME_MAX_LEN          10
#define WPA_P2P_DEFAULT_PERSISTENT      1
#define WPA_P2P_DEFAULT_GO_INTENT       6

#define MESH_AP                         1
#define MESH_STA                        0

#ifndef MAX_SSID_LEN
#define MAX_SSID_LEN                    32
#endif
#define MAX_DRIVER_NAME_LEN             16
#define WPA_MAX_SSID_KEY_INPUT_LEN      128
#define WPA_TXT_ADDR_LEN                17
#define WPA_INVITE_ADDR_LEN             23
#define WPA_24G_FREQ_TXT_LEN            4
#define WPA_INVITE_PERSISTENT_ID        13
#define WPA_STA_PMK_LEN                 32
#define WPA_STA_ITERA                   4096
#define WPA_MAX_TRY_FREQ_SCAN_CNT       3
#define WPA_24G_CHANNEL_NUMS            14

// BIT25 and BIT0 should not be used.
typedef enum {
	WPA_EVENT_AP_DEAUTH_FAIL             = WPA_BIT1,
	WPA_EVENT_AP_DEAUTH_FLAG             = (WPA_BIT30 | WPA_BIT1),
	WPA_EVENT_GET_SCAN_RESULT_OK         = WPA_BIT2,
	WPA_EVENT_GET_SCAN_RESULT_ERROR      = WPA_BIT3,
	WPA_EVENT_GET_SCAN_RESULT_FLAG       = (WPA_BIT2 | WPA_BIT3),
	WPA_EVENT_SCAN_RESULT_FREE_OK        = WPA_BIT4,
	WPA_EVENT_WPA_START_OK               = WPA_BIT5,
	WPA_EVENT_WPA_START_ERROR            = WPA_BIT6,
	WPA_EVENT_WPA_START_FLAG             = (WPA_BIT5 | WPA_BIT6),
	WPA_EVENT_STA_STOP_OK                = WPA_BIT7,
	WPA_EVENT_STA_STOP_ERROR             = WPA_BIT8,
	WPA_EVENT_STA_STOP_FLAG              = (WPA_BIT7 | WPA_BIT8),
	WPA_EVENT_STA_RM_NETWORK_OK          = WPA_BIT9,
	WPA_EVENT_STA_STATUS_OK              = WPA_BIT10,
	WPA_EVENT_STA_STATUS_FAIL            = WPA_BIT11,
	WPA_EVENT_STA_STATUS_FLAG            = (WPA_BIT10 | WPA_BIT11),
	WPA_EVENT_AP_STOP_OK                 = WPA_BIT12,
	WPA_EVENT_QUICK_CONNECT_OK           = WPA_BIT13,
	WPA_EVENT_QUICK_CONNECT_ERROR        = WPA_BIT14,
	WPA_EVENT_QUICK_CONNECT_FLAG         = (WPA_BIT13 | WPA_BIT14),
	WPA_EVENT_ADD_IFACE_OK               = WPA_BIT15,
	WPA_EVENT_ADD_IFACE_ERROR            = WPA_BIT16,
	WPA_EVENT_ADD_IFACE_FLAG             = (WPA_BIT15 | WPA_BIT16),
	WPA_EVENT_REMOVE_IFACE_FLAG          = WPA_BIT17,
	WPA_EVENT_SCAN_OK                    = WPA_BIT18,
	WPA_EVENT_MESH_START_OK              = WPA_BIT19,
	WPA_EVENT_GET_STATUS_OK              = WPA_BIT20,
	WPA_EVENT_GET_STATUS_ERROR           = WPA_BIT21,
	WPA_EVENT_GET_STATUS_FLAG            = (WPA_BIT20 | WPA_BIT21),
	WPA_EVENT_STATUS_FREE_OK             = WPA_BIT22,
	WPA_EVENT_GET_PEER_OK                = WPA_BIT23,
	WPA_EVENT_GET_PEER_ERROR             = WPA_BIT24,
	WPA_EVENT_GET_PEER_FLAG              = (WPA_BIT23 | WPA_BIT24),
	WPA_EVENT_SHOW_STA_OK                = WPA_BIT26,
	WPA_EVENT_SHOW_STA_ERROR             = WPA_BIT27,
	WPA_EVENT_SHOW_STA_FLAG              = (WPA_BIT26 | WPA_BIT27),
	WPA_EVENT_MESH_DEAUTH_OK             = WPA_BIT28,
	WPA_EVENT_MESH_DEAUTH_FAIL           = WPA_BIT29,
	WPA_EVENT_MESH_DEAUTH_FLAG           = (WPA_BIT28 | WPA_BIT29),
	WPA_EVENT_AP_DEAUTH_OK               = WPA_BIT30
} hisi_event_flag;

enum hisi_scan_record_flag {
	HISI_SCAN_UNSPECIFIED,
	HISI_SCAN,
	HISI_CHANNEL_SCAN,
	HISI_SSID_SCAN,
	HISI_PREFIX_SSID_SCAN,
	HISI_BSSID_SCAN,
};

typedef enum {
	HI_WPA_RM_NETWORK_END = 0,
	HI_WPA_RM_NETWORK_START,
	HI_WPA_RM_NETWORK_WORKING
} wpa_rm_network;

typedef enum {
	HI_WPA_BIT_SCAN_UNKNOW        = 0,
	HI_WPA_BIT_SCAN_SSID          = BIT0,
	HI_WPA_BIT_SCAN_BSSID         = BIT1,
} wpa_scan_type;

struct wpa_assoc_request {
	/* ssid input before ssid txt parsing */
	char ssid[HI_WIFI_MAX_SSID_LEN + 1];
	char key[WPA_MAX_SSID_KEY_INPUT_LEN + 1];
	unsigned char bssid[HI_WIFI_MAC_LEN];
	hi_wifi_auth_mode auth;
	u8 channel;
	hi_wifi_pairwise wpa_pairwise;
	unsigned char scan_type;
	hi_wifi_wpa_psk_usage_type psk_flag;
};

struct hisi_scan_record {
	char ssid[HI_WIFI_MAX_SSID_LEN + 1];
	unsigned char bssid[HI_WIFI_MAC_LEN];
	unsigned int channel;
	enum hisi_scan_record_flag flag;
};

struct wpa_scan_params {
	char ssid[HI_WIFI_MAX_SSID_LEN + 1];
	int  ssid_len;
	unsigned int channel;
	enum hisi_scan_record_flag flag;
	unsigned char bssid[HI_WIFI_MAC_LEN];
};
enum hisi_iftype {
	HISI_IFTYPE_UNSPECIFIED,
	HISI_IFTYPE_ADHOC,
	HISI_IFTYPE_STATION,
	HISI_IFTYPE_AP,
	HISI_IFTYPE_AP_VLAN,
	HISI_IFTYPE_WDS,
	HISI_IFTYPE_MONITOR,
	HISI_IFTYPE_MESH_POINT,
	HISI_IFTYPE_P2P_CLIENT,
	HISI_IFTYPE_P2P_GO,
	HISI_IFTYPE_P2P_DEVICE,

	/* keep last */
	NUM_HISI_IFTYPES,
	HISI_IFTYPE_MAX = NUM_HISI_IFTYPES - 1
};

enum hisi_mesh_enable_flag_type {
	HISI_MESH_ENABLE_ACCEPT_PEER,
	HISI_MESH_ENABLE_ACCEPT_STA,
	HISI_MESH_ENABLE_FLAG_BUTT
};

struct wifi_ap_opt_set {
	hi_wifi_protocol_mode hw_mode;
	int short_gi_off;
	int beacon_period;
	int dtim_period;
	int wpa_group_rekey;
};

struct wifi_mesh_opt_set {
	int beacon_period;
	int dtim_period;
	int wpa_group_rekey;
};

struct wifi_sta_opt_set {
	hi_wifi_protocol_mode hw_mode;
	hi_wifi_pmf_options pmf;
	int  usr_pmf_set_flag;
};

struct wifi_reconnect_set {
	int enable;
	unsigned int timeout;
	unsigned int period;
	unsigned int max_try_count;
	unsigned int try_count;
	unsigned int try_freq_scan_count;
	int pending_flag;
	struct wpa_ssid *current_ssid;
};

struct hisi_wifi_dev {
	hi_wifi_iftype iftype;
	void *priv;
	int network_id;
	int ifname_len;
	char ifname[WIFI_IFNAME_MAX_SIZE + 1];
	char reserve[1];
};

struct hostapd_conf {
	unsigned char bssid[ETH_ALEN];
	char ssid[HI_WIFI_MAX_SSID_LEN + 1];
	unsigned int channel_num;
	int wpa_key_mgmt;
	int wpa_pairwise;
	int authmode;
	unsigned char key[WPA_MAX_KEY_LEN + 1];
	int auth_algs;
	unsigned char wep_idx;
	int wpa;
	char driver[MAX_DRIVER_NAME_LEN];
	int ignore_broadcast_ssid;
};

typedef struct hostapd_conf wifi_softap_config;

typedef struct {
	u8 mac[ETH_ALEN];
} hi_sta_info;

extern int g_fast_connect_flag;
extern int g_fast_connect_scan_flag;
extern int g_connecting_flag;
extern wpa_rm_network g_wpa_rm_network;
extern int g_usr_scanning_flag;
extern size_t g_result_len;
extern char *g_scan_result_buf;
extern struct wifi_reconnect_set g_reconnect_set;
extern struct wifi_sta_opt_set g_sta_opt_set;
extern struct hisi_scan_record g_scan_record;
extern struct wpa_ssid *g_connecting_ssid;
extern hi_wifi_event_cb g_wpa_event_cb;
extern EVENT_CB_S g_wpa_event;
extern unsigned int g_sta_num;
extern hi_wifi_status *g_sta_status;
extern unsigned int g_mesh_get_sta_flag;
extern unsigned char  g_quick_conn_psk[HI_WIFI_STA_PSK_LEN];
extern unsigned int  g_quick_conn_psk_flag;
extern struct wifi_ap_opt_set g_ap_opt_set;
extern struct hostapd_data *g_hapd;
extern hi_wifi_ap_sta_info *g_ap_sta_info;
extern int g_mesh_flag;
extern int g_mesh_sta_flag;
extern int g_scan_flag;
extern struct hostapd_conf g_global_conf;
extern unsigned char g_ssid_prefix_flag;
extern struct hisi_wifi_dev* g_wifi_dev[WPA_MAX_WIFI_DEV_NUM];

#ifdef LOS_CONFIG_MESH
extern struct wifi_mesh_opt_set g_mesh_opt_set;
extern hi_wifi_mesh_peer_info *g_mesh_sta_info;
#endif /* LOS_CONFIG_MESH */

void wifi_new_task_event_cb(hi_wifi_event *event_cb);
void hi_free_wifi_dev(struct hisi_wifi_dev *wifi_dev);
int hi_wpa_ssid_config_set(struct wpa_ssid *ssid, const char *name, const char *value);
int hi_freq_to_channel(int freq, unsigned int *channel);
struct hisi_wifi_dev * hi_get_wifi_dev_by_name(const char *ifname);
struct hisi_wifi_dev * hi_get_wifi_dev_by_iftype(hi_wifi_iftype iftype);
struct hisi_wifi_dev * hi_get_wifi_dev_by_priv(const void *ctx);
int hi_count_wifi_dev_in_use(void);
struct hisi_wifi_dev * wpa_get_other_existed_wpa_wifi_dev(const void *priv);
int hi_wpa_scan(struct wpa_scan_params *params, hi_wifi_iftype iftype);
int wifi_scan(hi_wifi_iftype iftype, bool is_mesh, const hi_wifi_scan_params *sp);
int wifi_scan_result_bssid_parse(char **starttmp, void *buf, size_t *reply_len);
int wifi_scan_result_freq_parse(char **starttmp, void *buf, size_t *reply_len);
int wifi_scan_result_rssi_parse(char **starttmp, void *buf, size_t *reply_len);
void wifi_scan_result_base_flag_parse(const char *starttmp, void *buf);
int wifi_scan_result_filter_parse(void *buf);
int wifi_scan_result_ssid_parse(char **starttmp, void *buf, size_t *reply_len);
int wifi_scan_result(hi_wifi_iftype iftype);
int chan_to_freq(unsigned char chan);
int addr_precheck(const unsigned char *addr);
int try_set_lock_flag(void);
void clr_lock_flag(void);
int is_lock_flag_off(void);
int is_ap_mesh_or_p2p_on(void);
int wifi_add_iface(struct hisi_wifi_dev *wifi_dev);
int wifi_wpa_start(struct hisi_wifi_dev *wifi_dev);
int wifi_remove_iface(struct hisi_wifi_dev *wifi_dev);
int wifi_wpa_stop(hi_wifi_iftype iftype);
int hi_set_wifi_dev(struct hisi_wifi_dev *wifi_dev);
struct hisi_wifi_dev * wifi_dev_get(hi_wifi_iftype iftype);
struct hisi_wifi_dev * wifi_dev_creat(hi_wifi_iftype iftype, hi_wifi_protocol_mode phy_mode);

#endif
