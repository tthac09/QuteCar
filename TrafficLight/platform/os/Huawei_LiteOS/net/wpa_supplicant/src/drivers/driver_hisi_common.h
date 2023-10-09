/*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
* Description: driver_hisi_common header
* Author: hisilicon
* Create: 2019-03-04
*/

#ifndef __DRIVER_HISI_COMMON_H__
#define __DRIVER_HISI_COMMON_H__

/*****************************************************************************
  1 Other header files included
*****************************************************************************/
#include "los_typedef.h"
#include "hi_wifi_api.h"
#ifdef CONFIG_MESH
#include "hi_wifi_mesh_api.h"
#endif /* LOS_CONFIG_MESH */

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  2 Basic data type definitions
*****************************************************************************/
typedef char int8;
typedef signed short int16;
typedef signed int int32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

/*******************************************************************************
	Uncertain length, 32bits on 32-bit machine, 64bits on 64-bit machine
*******************************************************************************/
typedef signed long long int64;
typedef unsigned long long uint64;
typedef unsigned int size_t;
typedef signed int ssize_t;
typedef unsigned long ulong;
typedef signed long slong;
/*****************************************************************************
  3 Macro definition
*****************************************************************************/
#define OAL_STATIC static
#define OAL_VOLATILE volatile
#ifdef INLINE_TO_FORCEINLINE
#define OAL_INLINE __forceinline
#else
#define OAL_INLINE inline
#endif

#define HISI_SUCC 0
#define HISI_EFAIL  1
#define HISI_EINVAL 22

#ifndef ETH_ADDR_LEN
#define ETH_ADDR_LEN 6
#endif

#ifndef MAX_SSID_LEN
#define MAX_SSID_LEN 32
#endif

#ifndef HISI_MAX_NR_CIPHER_SUITES
#define HISI_MAX_NR_CIPHER_SUITES 5
#endif

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#ifndef HISI_MAX_NR_AKM_SUITES
#define HISI_MAX_NR_AKM_SUITES 2
#endif

#ifndef	HISI_PTR_NULL
#define	HISI_PTR_NULL NULL
#endif

#ifndef	SCAN_AP_LIMIT
#define	SCAN_AP_LIMIT 64
#endif

#ifndef   NETDEV_UP
#define   NETDEV_UP   0x0001
#endif
#ifndef   NETDEV_DOWN
#define   NETDEV_DOWN 0x0002
#endif

#ifndef   NOTIFY_DONE
#define   NOTIFY_DONE 0x0000
#endif

#ifdef _PRE_WLAN_FEATURE_REKEY_OFFLOAD
#define HISI_REKEY_OFFLOAD_KCK_LEN            16
#define HISI_REKEY_OFFLOAD_KEK_LEN            16
#define HISI_REKEY_OFFLOAD_REPLAY_COUNTER_LEN 8
#endif
/*****************************************************************************
  4 Enum definition
*****************************************************************************/
typedef enum {
	HISI_FALSE = 0,
	HISI_TRUE = 1,

	HISI_BUTT
} hisi_bool_enum;
typedef uint8 hisi_bool_enum_uint8;

#define HISI_KEYTYPE_DEFAULT_INVALID (-1)
typedef uint8 hisi_iftype_enum_uint8;

typedef enum {
	HISI_KEYTYPE_GROUP,
	HISI_KEYTYPE_PAIRWISE,
	HISI_KEYTYPE_PEERKEY,

	NUM_HISI_KEYTYPES
} hisi_key_type_enum;
typedef uint8 hisi_key_type_enum_uint8;

typedef enum {
	HISI_KEY_DEFAULT_TYPE_INVALID,
	HISI_KEY_DEFAULT_TYPE_UNICAST,
	HISI_KEY_DEFAULT_TYPE_MULTICAST,

	NUM_HISI_KEY_DEFAULT_TYPES
} hisi_key_default_types_enum;
typedef uint8 hisi_key_default_types_enum_uint8;

typedef enum {
	HISI_NO_SSID_HIDING,
	HISI_HIDDEN_SSID_ZERO_LEN,
	HISI_HIDDEN_SSID_ZERO_CONTENTS
} hisi_hidden_ssid_enum;
typedef uint8 hisi_hidden_ssid_enum_uint8;

typedef enum {
	HISI_IOCTL_SET_AP = 0,
	HISI_IOCTL_NEW_KEY,
	HISI_IOCTL_DEL_KEY,
	HISI_IOCTL_SET_KEY,
	HISI_IOCTL_SEND_MLME,
	HISI_IOCTL_SEND_EAPOL,
	HISI_IOCTL_RECEIVE_EAPOL,
	HISI_IOCTL_ENALBE_EAPOL,
	HISI_IOCTL_DISABLE_EAPOL,
	HIIS_IOCTL_GET_ADDR,
	HISI_IOCTL_SET_MODE,
	HIIS_IOCTL_GET_HW_FEATURE,
	HISI_IOCTL_SCAN,
	HISI_IOCTL_DISCONNET,
	HISI_IOCTL_ASSOC,
	HISI_IOCTL_SET_NETDEV,
	HISI_IOCTL_CHANGE_BEACON,
	HISI_IOCTL_SET_REKEY_INFO,
 	HISI_IOCTL_STA_REMOVE,
	HISI_IOCTL_SEND_ACTION,
	HISI_IOCTL_SET_MESH_USER,
	HISI_IOCTL_SET_MESH_GTK,
	HISI_IOCTL_EN_ACCEPT_PEER,
	HISI_IOCTL_EN_ACCEPT_STA,
	HISI_IOCTL_ADD_IF,
	HISI_IOCTL_PROBE_REQUEST_REPORT,
	HISI_IOCTL_REMAIN_ON_CHANNEL,
	HISI_IOCTL_CANCEL_REMAIN_ON_CHANNEL,
	HISI_IOCTL_SET_P2P_NOA,
	HISI_IOCTL_SET_P2P_POWERSAVE,
	HISI_IOCTL_SET_AP_WPS_P2P_IE,
	HISI_IOCTL_REMOVE_IF,
	HISI_IOCTL_GET_P2P_MAC_ADDR,
	HISI_IOCTL_GET_DRIVER_FLAGS,
	HISI_IOCTL_SET_USR_APP_IE,
	HISI_IOCTL_DELAY_REPORT,
	HISI_IOCTL_SEND_EXT_AUTH_STATUS,
	HWAL_EVENT_BUTT
} hisi_event_enum;
typedef uint8 hisi_event_enum_uint8;

typedef enum {
	HISI_ELOOP_EVENT_NEW_STA = 0,
	HISI_ELOOP_EVENT_DEL_STA,
	HISI_ELOOP_EVENT_RX_MGMT,
	HISI_ELOOP_EVENT_TX_STATUS,
	HISI_ELOOP_EVENT_SCAN_DONE,
	HISI_ELOOP_EVENT_SCAN_RESULT,
	HISI_ELOOP_EVENT_CONNECT_RESULT,
	HISI_ELOOP_EVENT_DISCONNECT,
	HISI_ELOOP_EVENT_MESH_CLOSE,
	HISI_ELOOP_EVENT_REMAIN_ON_CHANNEL,
	HISI_ELOOP_EVENT_CANCEL_REMAIN_ON_CHANNEL,
	HISI_ELOOP_EVENT_CHANNEL_SWITCH,
	HISI_ELOOP_EVENT_TIMEOUT_DISCONN,
	HISI_ELOOP_EVENT_EXTERNAL_AUTH,
	HISI_ELOOP_EVENT_BUTT
} hisi_eloop_event_enum;
typedef uint8 hisi_eloop_event_enum_uint8;

/**
 * Action to perform with external authentication request.
 * @HISI_EXTERNAL_AUTH_START: Start the authentication.
 * @HISI_EXTERNAL_AUTH_ABORT: Abort the ongoing authentication.
 */
typedef enum {
    HISI_EXTERNAL_AUTH_START,
    HISI_EXTERNAL_AUTH_ABORT,

    HISI_EXTERNAL_AUTH_BUTT,
}hisi_external_auth_action_enum;
typedef uint8 hisi_external_auth_action_enum_uint8;

typedef enum {
	HISI_MFP_NO,
	HISI_MFP_OPTIONAL,
	HISI_MFP_REQUIRED,
} hisi_mfp_enum;
typedef uint8 hisi_mfp_enum_uint8;
typedef uint8 hisi_mesh_plink_state_enum_uint8;
typedef enum {
	HISI_AUTHTYPE_OPEN_SYSTEM = 0,
	HISI_AUTHTYPE_SHARED_KEY,
	HISI_AUTHTYPE_FT,
	HISI_AUTHTYPE_SAE,
	HISI_AUTHTYPE_NETWORK_EAP,
	/* keep last */
	HISI_AUTHTYPE_NUM,
	HISI_AUTHTYPE_MAX = HISI_AUTHTYPE_NUM - 1,
	HISI_AUTHTYPE_AUTOMATIC,
	HISI_AUTHTYPE_BUTT
} hisi_auth_type_enum;
typedef uint8 hisi_auth_type_enum_uint8;

typedef enum {
	HISI_SCAN_SUCCESS,
	HISI_SCAN_FAILED,
	HISI_SCAN_REFUSED,
	HISI_SCAN_TIMEOUT
} hisi_scan_status_enum;

/*****************************************************************************
  5 STRUCT definition
*****************************************************************************/
typedef struct {
	hisi_scan_status_enum scan_status;
} hisi_driver_scan_status_stru;

typedef struct {
	uint8 set;  // 0: del, 1: add
	uint8 ie_type;
	uint16 ie_len;
	hi_wifi_frame_type frame_type;
	uint8 *ie;
} hisi_usr_app_ie_stru;

typedef struct {
	unsigned int cmd;
	void *buf;
} hisi_ioctl_command_stru;
typedef int32 (*hisi_send_event_cb)(const char*, signed int, const unsigned char *, unsigned int);

typedef struct {
	int32  reassoc;
	size_t ielen;
	uint8  *ie;
	uint8  macaddr[ETH_ADDR_LEN];
	uint8  resv[2];
} hisi_new_sta_info_stru;

typedef struct {
	uint8  *buf;
	uint32 len;
	int32  sig_mbm;
	int32  freq;
} hisi_rx_mgmt_stru;

typedef struct {
	uint8                *buf;
	uint32               len;
	hisi_bool_enum_uint8 ack;
	uint8                resv[3];
} hisi_tx_status_stru;

typedef struct {
	uint32 freq;
	size_t data_len;
	uint8  *data;
	uint64 *send_action_cookie;
} hisi_mlme_data_stru;

typedef struct {
	size_t head_len;
	size_t tail_len;
	uint8 *head;
	uint8 *tail;
} hisi_beacon_data_stru;

typedef struct {
	uint8 *dst;
	uint8 *src;
	uint8 *bssid;
	uint8 *data;
	size_t data_len;
} hisi_action_data_stru;

typedef struct {
	uint8                            *addr;
	hisi_mesh_plink_state_enum_uint8 plink_state;
	uint8                            set;
	uint8                            mesh_bcn_priority;
	uint8                            mesh_is_mbr;
	uint8                            mesh_initiative_peering;
} hisi_mesh_usr_params_stru;

typedef struct {
	uint8 enable_flag;
} hisi_mesh_enable_flag_stru;

typedef struct {
	uint8  enable;
	uint16 timeout;
	uint8  resv;
} hisi_delay_report_stru;

typedef struct {
	int32 mode;
	int32 freq;
	int32 channel;

	/* for HT */
	int32 ht_enabled;

	/* 0 = HT40 disabled, -1 = HT40 enabled,
	 * secondary channel below primary, 1 = HT40
	 * enabled, secondary channel above primary */
	int32 sec_channel_offset;

	/* for VHT */
	int32 vht_enabled;

	/* valid for both HT and VHT, center_freq2 is non-zero
	 * only for bandwidth 80 and an 80+80 channel */
	int32 center_freq1;
	int32 center_freq2;
	int32 bandwidth;
} hisi_freq_params_stru;

typedef struct {
	int32                             type;
	uint32                            key_idx;
	uint32                            key_len;
	uint32                            seq_len;
	uint32                            cipher;
	uint8                             *addr;
	uint8                             *key;
	uint8                             *seq;
	hisi_bool_enum_uint8              def;
	hisi_bool_enum_uint8              defmgmt;
 	hisi_key_default_types_enum_uint8 default_types;
	uint8                             resv;
} hisi_key_ext_stru;

typedef struct {
	hisi_freq_params_stru       freq_params;
	hisi_beacon_data_stru       beacon_data;
	size_t                      ssid_len;
	int32                       beacon_interval;
	int32                       dtim_period;
	uint8                       *ssid;
	hisi_hidden_ssid_enum_uint8 hidden_ssid;
	hisi_auth_type_enum_uint8   auth_type;
	size_t                      mesh_ssid_len;
	uint8                       *mesh_ssid;
} hisi_ap_settings_stru;

typedef struct {
	uint8                  bssid[ETH_ADDR_LEN];
	hisi_iftype_enum_uint8 iftype;
	uint8                  resv;
} hisi_set_mode_stru;

typedef struct {
	uint8  *puc_buf;
	uint32 ul_len;
} hisi_tx_eapol_stru;

typedef struct {
	uint8  *buf;
	uint32 len;
} hisi_rx_eapol_stru;

typedef struct {
	void *callback;
	void *contex;
} hisi_enable_eapol_stru;

typedef struct {
	uint16 channel;
	uint8  resv[2];
	uint32 freq;
	uint32 flags;
} hisi_ieee80211_channel_stru;

typedef struct {
	int32                       channel_num;
	uint16                      bitrate[12];
	uint16                      ht_capab;
	uint8                       resv[2];
	hisi_ieee80211_channel_stru iee80211_channel[14];
} hisi_hw_feature_data_stru;

typedef struct {
	uint8  ssid[MAX_SSID_LEN];
	size_t ssid_len;
} hisi_driver_scan_ssid_stru;

typedef struct {
	hisi_driver_scan_ssid_stru *ssids;
	int32                      *freqs;
	uint8                      *extra_ies;
	uint8                      *bssid;
	uint8                      num_ssids;
	uint8                      num_freqs;
	uint8                      prefix_ssid_scan_flag;
	uint8                      fast_connect_flag;
	int32                      extra_ies_len ;
} hisi_scan_stru;

typedef struct {
	uint32 freq;
	uint32 duration;
} hisi_on_channel_stru;

typedef struct {
	uint8 type;
} hisi_if_add_stru;

typedef struct {
	int32 start;
	int32 duration;
	uint8 count;
	uint8 resv[3];
} hisi_p2p_noa_stru;

typedef struct {
	int32 legacy_ps;
	int8  opp_ps;
	uint8 ctwindow;
	int8  resv[2];
} hisi_p2p_power_save_stru;

typedef struct {
	uint8 ifname[IFNAMSIZ];
} hisi_if_remove_stru;

typedef struct {
	uint8 type;
	uint8 mac_addr[ETH_ADDR_LEN];
	uint8 resv;
} hisi_get_p2p_addr_stru;
typedef struct {
	hi_wifi_iftype iftype;
	uint8          *mac_addr;
} hisi_iftype_mac_addr_stru;
typedef struct {
	uint64 drv_flags;
} hisi_get_drv_flags_stru;

typedef struct {
	int32 freq;
} hisi_ch_switch_stru;

/* The driver reports an event to trigger WPA to start SAE authentication. */
typedef struct {
	hisi_external_auth_action_enum auth_action;
	uint8 bssid[ETH_ADDR_LEN];
	uint8 *ssid;
	uint32 ssid_len;
	uint32 key_mgmt_suite;
	uint16 status;
	uint8 *pmkid;
}hisi_external_auth_stru;

typedef struct {
	uint32 wpa_versions;
	uint32 cipher_group;
	int32  n_ciphers_pairwise;
	uint32 ciphers_pairwise[HISI_MAX_NR_CIPHER_SUITES];
	int32  n_akm_suites;
	uint32 akm_suites[HISI_MAX_NR_AKM_SUITES];
} hisi_crypto_settings_stru;

typedef struct {
	uint8                     *bssid;
	uint8                     *ssid;
	uint8                     *ie;
	uint8                     *key;
	uint8                     auth_type;
	uint8                     privacy;
	uint8                     key_len;
	uint8                     key_idx;
	uint8                     mfp;
	uint8                     auto_conn;
	uint8                     rsv[2];    /* 2: reserve 2 bytes */
	uint32                    freq;
	uint32                    ssid_len;
	uint32                    ie_len;
	hisi_crypto_settings_stru *crypto;
} hisi_associate_params_stru;

typedef struct {
	uint8  *req_ie;
	size_t req_ie_len;
	uint8  *resp_ie;
	size_t resp_ie_len;
	uint8  bssid[ETH_ADDR_LEN];
	uint8  rsv[2];
	uint16 status;
	uint16 freq;
} hisi_connect_result_stru;

typedef struct {
	int32  flags;
	uint8  bssid[ETH_ADDR_LEN];
	int16  caps;
	int32  freq;
#ifndef HISI_SCAN_SIZE_CROP
	int16  beacon_int;
	int32  qual;
	uint32 beacon_ie_len;
#endif /* HISI_SCAN_SIZE_CROP */
	int32  level;
	uint32 age;
	uint32 ie_len;
	uint8  *variable;
} hisi_scan_result_stru;

typedef struct {
	uint8  *ie;
	uint16 reason;
	uint8  rsv[2];
	uint32 ie_len;
} hisi_disconnect_stru;

#ifdef _PRE_WLAN_FEATURE_REKEY_OFFLOAD
typedef struct {
	uint8 kck[HISI_REKEY_OFFLOAD_KCK_LEN];
	uint8 kek[HISI_REKEY_OFFLOAD_KEK_LEN];
	uint8 replay_ctr[HISI_REKEY_OFFLOAD_REPLAY_COUNTER_LEN];
} hisi_rekey_offload_stru;
#endif

typedef struct {
	uint8 peer_addr[ETH_ADDR_LEN];
	uint8 mesh_bcn_priority;
	uint8 mesh_is_mbr;
	int8  rssi;
	int8  reserved[3];
} hisi_mesh_new_peer_candidate_stru;

/*****************************************************************************
  6 Function declaration
*****************************************************************************/
extern int32 hisi_register_send_event_cb(hisi_send_event_cb func);
extern int32 hisi_hwal_wpa_ioctl(int8 *ifname, hisi_ioctl_command_stru *cmd);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /* end of driver_hisi_common.h */
