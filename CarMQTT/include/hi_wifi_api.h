/**
* @file hi_wifi_api.h
*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved. \n
* Description: header file for wifi api.CNcomment:描述：WiFi api接口头文件.CNend\n
* Author: Hisilicon \n
* Create: 2019-01-03
*/

/**
 * @defgroup hi_wifi_basic WiFi Basic Settings
 * @ingroup hi_wifi
 */

#ifndef __HI_WIFI_API_H__
#define __HI_WIFI_API_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**
 * mac transform string.CNcomment:地址转为字符串.CNend
 */
#ifndef MACSTR
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#ifndef MAC2STR
#define mac2str(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif

#ifndef bit
#define bit(x) (1U << (x))
#endif

/**
 * @ingroup hi_wifi_basic
 *
 * TKIP of cipher mode.CNcomment:加密方式为TKIP.CNend
 */
#define WIFI_CIPHER_TKIP                 bit(3)

/**
 * @ingroup hi_wifi_basic
 *
 * CCMP of cipher mode.CNcomment:加密方式为CCMP.CNend
 */
#define WIFI_CIPHER_CCMP                 bit(4)

#define WIFI_24G_CHANNEL_NUMS 14

/**
 * @ingroup hi_wifi_basic
 *
 * max interiface name length.CNcomment:网络接口名最大长度.CNend
 */
#define WIFI_IFNAME_MAX_SIZE             16

/**
 * @ingroup hi_wifi_basic
 *
 * The minimum timeout of a single reconnection.CNcomment:最小单次重连超时时间.CNend
 */
#define WIFI_MIN_RECONNECT_TIMEOUT   2

/**
 * @ingroup hi_wifi_basic
 *
 * The maximum timeout of a single reconnection, representing an infinite number of loop reconnections.
 * CNcomment:最大单次重连超时时间，表示无限次循环重连.CNend
 */
#define WIFI_MAX_RECONNECT_TIMEOUT   65535

/**
 * @ingroup hi_wifi_basic
 *
 * The minimum auto reconnect interval.CNcomment:最小自动重连间隔时间.CNend
 */
#define WIFI_MIN_RECONNECT_PERIOD    1

/**
 * @ingroup hi_wifi_basic
 *
 * The maximum auto reconnect interval.CNcomment:最大自动重连间隔时间.CNend
 */
#define WIFI_MAX_RECONNECT_PERIOD   65535

/**
 * @ingroup hi_wifi_basic
 *
 * The minmum times of auto reconnect.CNcomment:最小自动重连连接次数.CNend
 */
#define WIFI_MIN_RECONNECT_TIMES    1

/**
 * @ingroup hi_wifi_basic
 *
 * The maximum times of auto reconnect.CNcomment:最大自动重连连接次数.CNend
 */
#define WIFI_MAX_RECONNECT_TIMES   65535

/**
 * @ingroup hi_wifi_basic
 *
 * max scan number of ap.CNcomment:支持扫描ap的最多数目.CNend
 */
#define WIFI_SCAN_AP_LIMIT               32

/**
 * @ingroup hi_wifi_basic
 *
 * length of status buff.CNcomment:获取连接状态字符串的长度.CNend
 */
#define WIFI_STATUS_BUF_LEN_LIMIT        512

/**
 * @ingroup hi_wifi_basic
 *
 * Decimal only WPS pin code length.CNcomment:WPS中十进制pin码长度.CNend
 */
#define WIFI_WPS_PIN_LEN             8

/**
 * @ingroup hi_wifi_basic
 *
 * default max num of station.CNcomment:默认支持的station最大个数.CNend
 */
#define WIFI_DEFAULT_MAX_NUM_STA         6

/**
 * @ingroup hi_wifi_basic
 *
 * return success value.CNcomment:返回成功标识.CNend
 */
#define HISI_OK                         0

/**
 * @ingroup hi_wifi_basic
 *
 * return failed value.CNcomment:返回值错误标识.CNend
 */
#define HISI_FAIL                       (-1)

/**
 * @ingroup hi_wifi_basic
 *
 * Max length of SSID.CNcomment:SSID最大长度定义.CNend
 */
#define HI_WIFI_MAX_SSID_LEN  32

/**
 * @ingroup hi_wifi_basic
 *
 * Length of MAC address.CNcomment:MAC地址长度定义.CNend
 */
#define HI_WIFI_MAC_LEN        6

/**
 * @ingroup hi_wifi_basic
 *
 * String length of bssid, eg. 00:00:00:00:00:00.CNcomment:bssid字符串长度定义(00:00:00:00:00:00).CNend
 */
#define HI_WIFI_TXT_ADDR_LEN   17

/**
 * @ingroup hi_wifi_basic
 *
 * Length of Key.CNcomment:KEY 密码长度定义.CNend
 */
#define HI_WIFI_AP_KEY_LEN     64

/**
 * @ingroup hi_wifi_basic
 *
 * Maximum  length of Key.CNcomment:KEY 最大密码长度.CNend
 */
#define HI_WIFI_MAX_KEY_LEN    64

/**
 * @ingroup hi_wifi_basic
 *
 * Return value of invalid channel.CNcomment:无效信道返回值.CNend
 */
#define HI_WIFI_INVALID_CHANNEL 0xFF

/**
 * @ingroup hi_wifi_basic
 *
 * Index of Vendor IE.CNcomment:Vendor IE 最大索引.CNend
 */
#define HI_WIFI_VENDOR_IE_MAX_IDX 1

/**
 * @ingroup hi_wifi_basic
 *
 * Max length of Vendor IE.CNcomment:Vendor IE 最大长度.CNend
 */
#define HI_WIFI_VENDOR_IE_MAX_LEN 255

/**
 * @ingroup hi_wifi_basic
 *
 * Length range of frame for user use(24-1400).CNcomment:用户定制报文长度范围(24-1400).CNend
 */
#define HI_WIFI_CUSTOM_PKT_MAX_LEN 1400
#define HI_WIFI_CUSTOM_PKT_MIN_LEN 24

/**
 * @ingroup hi_wifi_basic
 *
 * Length of wpa psk.CNcomment:wpa psk的长度.CNend
 */
#define HI_WIFI_STA_PSK_LEN                 32

/**
 * @ingroup hi_wifi_basic
 *
 * Max num of retry.CNcomment:软件重传的最大次数.CNend
 * Max time of retry.CNcomment:软件重传的最大时间.CNend
 */
#define HI_WIFI_RETRY_MAX_NUM               15
#define HI_WIFI_RETRY_MAX_TIME              200

/**
 * @ingroup hi_wifi_basic
 *
 * Minimum priority of callback task.CNcomment:事件回调task的最小优先级.CNend
 * Max priority of callback task.CNcomment:事件回调task的最大优先级.CNend
 */
#define HI_WIFI_CB_MIN_PRIO                 10
#define HI_WIFI_CB_MAX_PRIO                 50

/**
 * @ingroup hi_wifi_basic
 *
 * Minimum stack size of callback task.CNcomment:最小的回调线程栈大小.CNend
 */
#define HI_WIFI_CB_MIN_STACK                1024

/**
 * @ingroup hi_wifi_basic
 *
 * Reporting data type of monitor mode.CNcomment:混杂模式上报的数据类型.CNend
 */
typedef enum {
    HI_WIFI_MONITOR_OFF,                /**< close monitor mode. CNcomment: 关闭混杂模式.CNend */
    HI_WIFI_MONITOR_MCAST_DATA,         /**< report multi-cast data frame. CNcomment: 上报组播(广播)数据包.CNend */
    HI_WIFI_MONITOR_UCAST_DATA,         /**< report single-cast data frame. CNcomment: 上报单播数据包.CNend */
    HI_WIFI_MONITOR_MCAST_MANAGEMENT,   /**< report multi-cast mgmt frame. CNcomment: 上报组播(广播)管理包.CNend */
    HI_WIFI_MONITOR_UCAST_MANAGEMENT,   /**< report sigle-cast mgmt frame. CNcomment: 上报单播管理包.CNend */

    HI_WIFI_MONITOR_BUTT
} hi_wifi_monitor_mode;

/**
 * @ingroup hi_wifi_basic
 *
 * Definition of protocol frame type.CNcomment:协议报文类型定义.CNend
 */
typedef enum {
    HI_WIFI_PKT_TYPE_BEACON,        /**< Beacon packet. CNcomment: Beacon包.CNend */
    HI_WIFI_PKT_TYPE_PROBE_REQ,     /**< Probe Request packet. CNcomment: Probe Request包.CNend */
    HI_WIFI_PKT_TYPE_PROBE_RESP,    /**< Probe Response packet. CNcomment: Probe Response包.CNend */
    HI_WIFI_PKT_TYPE_ASSOC_REQ,     /**< Assoc Request packet. CNcomment: Assoc Request包.CNend */
    HI_WIFI_PKT_TYPE_ASSOC_RESP,    /**< Assoc Response packet. CNcomment: Assoc Response包.CNend */

    HI_WIFI_PKT_TYPE_BUTT
}hi_wifi_pkt_type;

/**
 * @ingroup hi_wifi_iftype
 *
 * Interface type of wifi.CNcomment:wifi 接口类型.CNend
 */
typedef enum {
    HI_WIFI_IFTYPE_UNSPECIFIED,
    HI_WIFI_IFTYPE_ADHOC,
    HI_WIFI_IFTYPE_STATION,
    HI_WIFI_IFTYPE_AP,
    HI_WIFI_IFTYPE_AP_VLAN,
    HI_WIFI_IFTYPE_WDS,
    HI_WIFI_IFTYPE_MONITOR,
    HI_WIFI_IFTYPE_MESH_POINT,
    HI_WIFI_IFTYPE_P2P_CLIENT,
    HI_WIFI_IFTYPE_P2P_GO,
    HI_WIFI_IFTYPE_P2P_DEVICE,

    HI_WIFI_IFTYPES_BUTT
} hi_wifi_iftype;

/**
 * @ingroup hi_wifi_basic
 *
 * Definition of bandwith type.CNcomment:接口带宽定义.CNend
 */
typedef enum {
    HI_WIFI_BW_HIEX_5M,     /**< 窄带5M带宽 */
    HI_WIFI_BW_HIEX_10M,    /**< 窄带10M带宽 */
    HI_WIFI_BW_LEGACY_20M,  /**< 20M带宽 */
    HI_WIFI_BW_BUTT         /**< hi_wifi_bw枚举定界 */
} hi_wifi_bw;

/**
 * @ingroup hi_wifi_basic
 *
 * The protocol mode of softap and station interfaces.CNcomment:softap和station接口的protocol模式.CNend
 */
typedef enum {
    HI_WIFI_PHY_MODE_11BGN, /**< 802.11BGN 模式 */
    HI_WIFI_PHY_MODE_11BG,  /**< 802.11BG 模式 */
    HI_WIFI_PHY_MODE_11B,   /**< 802.11B 模式 */
    HI_WIFI_PHY_MODE_BUTT   /**< hi_wifi_protocol_mode枚举定界 */
} hi_wifi_protocol_mode;

/**
 * @ingroup hi_wifi_basic
 *
 * Authentification type enum.CNcomment:认证类型(连接网络不支持HI_WIFI_SECURITY_WPAPSK).CNend
 */
typedef enum {
    HI_WIFI_SECURITY_OPEN,                  /**< 认证类型:开放 */
    HI_WIFI_SECURITY_WEP,                   /**< 认证类型:WEP */
    HI_WIFI_SECURITY_WPA2PSK,               /**< 认证类型:WPA2-PSK */
    HI_WIFI_SECURITY_WPAPSK_WPA2PSK_MIX,    /**< 认证类型:WPA-PSK/WPA2-PSK混合 */
    HI_WIFI_SECURITY_WPAPSK,                /**< 认证类型:WPA-PSK */
    HI_WIFI_SECURITY_WPA,                   /**< 认证类型:WPA */
    HI_WIFI_SECURITY_WPA2,                  /**< 认证类型:WPA2 */
    HI_WIFI_SECURITY_SAE,                   /**< 认证类型:SAE */
    HI_WIFI_SECURITY_UNKNOWN                /**< 其他认证类型:UNKNOWN */
} hi_wifi_auth_mode;

/**
 * @ingroup hi_wifi_basic
 *
 * Encryption type enum.CNcoment:加密类型.CNend
 *
 */
typedef enum {
    HI_WIFI_PARIWISE_UNKNOWN,               /**< 加密类型:UNKNOWN */
    HI_WIFI_PAIRWISE_AES,                   /**< 加密类型:AES     */
    HI_WIFI_PAIRWISE_TKIP,                  /**< 加密类型:TKIP     */
    HI_WIFI_PAIRWISE_TKIP_AES_MIX           /**< 加密类型:TKIP AES混合 */
} hi_wifi_pairwise;

/**
 * @ingroup hi_wifi_basic
 *
 * PMF type enum.CNcomment:PMF管理帧保护模式类型.CNend
 */
typedef enum {
    HI_WIFI_MGMT_FRAME_PROTECTION_CLOSE,        /**< 管理帧保护模式:关闭 */
    HI_WIFI_MGMT_FRAME_PROTECTION_OPTIONAL,     /**< 管理帧保护模式:可选 */
    HI_WIFI_MGMT_FRAME_PROTECTION_REQUIRED      /**< 管理帧保护模式:必须 */
} hi_wifi_pmf_options;

/**
 * @ingroup hi_wifi_basic
 *
 * Type of connect's status.CNcomment:连接状态.CNend
 */
typedef enum {
    HI_WIFI_DISCONNECTED,   /**< 连接状态:未连接 */
    HI_WIFI_CONNECTED,      /**< 连接状态:已连接 */
} hi_wifi_conn_status;

/**
 * @ingroup hi_wifi_basic
 *
 * wifi's operation mode.CNcomment:wifi的工作模式.CNend
 */
typedef enum {
    HI_WIFI_MODE_INFRA = 0,               /**< STA模式 */
    HI_WIFI_MODE_AP    = 2,               /**< AP 模式 */
    HI_WIFI_MODE_MESH  = 5                /**< MESH 模式 */
} hi_wifi_mode;

/**
 * @ingroup hi_wifi_basic
 *
 * Event type of WiFi event.CNcomment:WiFi的事件类型.CNend
 */
typedef enum {
    HI_WIFI_EVT_UNKNOWN,             /**< UNKNOWN */
    HI_WIFI_EVT_SCAN_DONE,           /**< STA扫描完成 */
    HI_WIFI_EVT_CONNECTED,           /**< 已连接 */
    HI_WIFI_EVT_DISCONNECTED,        /**< 断开连接 */
    HI_WIFI_EVT_WPS_TIMEOUT,         /**< WPS事件超时 */
    HI_WIFI_EVT_MESH_CONNECTED,      /**< MESH已连接 */
    HI_WIFI_EVT_MESH_DISCONNECTED,   /**< MESH断开连接 */
    HI_WIFI_EVT_AP_START,            /**< AP开启 */
    HI_WIFI_EVT_STA_CONNECTED,       /**< AP和STA已连接 */
    HI_WIFI_EVT_STA_DISCONNECTED,    /**< AP和STA断开连接 */
    HI_WIFI_EVT_STA_FCON_NO_NETWORK, /**< STA快速链接,扫描不到网络 */
    HI_WIFI_EVT_MESH_CANNOT_FOUND,   /**< MESH关联扫不到对端 */
    HI_WIFI_EVT_MESH_SCAN_DONE,      /**< MESH扫描完成 */
    HI_WIFI_EVT_MESH_STA_SCAN_DONE,  /**< MESH STA扫描完成 */
    HI_WIFI_EVT_AP_SCAN_DONE,        /**< AP扫描完成 */
    HI_WIFI_EVT_BUTT                 /**< hi_wifi_event_type枚举定界 */
} hi_wifi_event_type;

/**
 * @ingroup hi_wifi_basic
 *
 * Scan type enum.CNcomment:扫描类型.CNend
 */
typedef enum {
    HI_WIFI_BASIC_SCAN,             /**< 普通扫描 */
    HI_WIFI_CHANNEL_SCAN,           /**< 指定信道扫描 */
    HI_WIFI_SSID_SCAN,              /**< 指定SSID扫描 */
    HI_WIFI_SSID_PREFIX_SCAN,       /**< SSID前缀扫描 */
    HI_WIFI_BSSID_SCAN,             /**< 指定BSSID扫描 */
} hi_wifi_scan_type;

/**
 * @ingroup hi_wifi_basic
 *
 * WPA PSK usage type.CNcomment: WPA PSK使用策略.CNend
 */
typedef enum {
    HI_WIFI_WPA_PSK_NOT_USE,        /**< 不使用 */
    HI_WIFI_WPA_PSK_USE_INNER,      /**< 使用内部PSK */
    HI_WIFI_WPA_PSK_USE_OUTER,      /**< 使用外部PSK */
} hi_wifi_wpa_psk_usage_type;

/**
 * @ingroup hi_wifi_basic
 *
 * parameters of scan.CNcomment:station和mesh接口scan参数.CNend
 */
typedef struct {
    char ssid[HI_WIFI_MAX_SSID_LEN + 1];    /**< SSID 只支持ASCII字符 */
    unsigned char bssid[HI_WIFI_MAC_LEN];   /**< BSSID */
    unsigned char ssid_len;                 /**< SSID长度 */
    unsigned char channel;                  /**< 信道号，取值范围1-14，不同区域取值范围有差异 */
    hi_wifi_scan_type scan_type;            /**< 扫描类型 */
} hi_wifi_scan_params;

/**
 * @ingroup hi_wifi_basic
 *
 * Struct of scan result.CNcomment:扫描结果结构体.CNend
 */
typedef struct {
    char ssid[HI_WIFI_MAX_SSID_LEN + 1];    /**< SSID 只支持ASCII字符 */
    unsigned char bssid[HI_WIFI_MAC_LEN];   /**< BSSID */
    unsigned int channel;                   /**< 信道号，取值范围1-14，不同区域取值范围有差异 */
    hi_wifi_auth_mode auth;                 /**< 认证类型 */
    int rssi;                               /**< 信号强度 */
    unsigned char wps_flag : 1;             /**< WPS标识 */
    unsigned char wps_session : 1;          /**< WPS标识 PBC-0/PIN-1 */
    unsigned char wmm : 1;                  /**< WMM标识 */
    unsigned char resv : 1;                 /**< Reserved */
    unsigned char hisi_mesh_flag : 1;       /**< HI MESH标识 */
} hi_wifi_ap_info;

/**
 * @ingroup hi_wifi_basic
 *
 * Struct of connect parameters.CNcomment:station连接结构体.CNend
 */
typedef struct {
    char ssid[HI_WIFI_MAX_SSID_LEN + 1];    /**< SSID 只支持ASCII字符*/
    hi_wifi_auth_mode auth;                 /**< 认证类型 */
    char key[HI_WIFI_MAX_KEY_LEN + 1];      /**< 秘钥 */
    unsigned char bssid[HI_WIFI_MAC_LEN];   /**< BSSID */
    hi_wifi_pairwise pairwise;              /**< 加密方式, 可选，不需指定时填为0 */
} hi_wifi_assoc_request;

/**
 * @ingroup hi_wifi_basic
 *
 * Struct of fast connect parameters.CNcomment:station快速连接结构体.CNend
 */
typedef struct {
    hi_wifi_assoc_request req;                  /**< 关联请求 */
    unsigned char channel;                      /**< 信道号，取值范围1-14，不同区域取值范围有差异 */
    unsigned char psk[HI_WIFI_STA_PSK_LEN];     /**< psk， 使用hi_wifi_psk_calc_and_store()时此参数无需填写 */
    hi_wifi_wpa_psk_usage_type psk_flag;        /**< 传入psk的标志, 可选,不需指定时填为0 */
} hi_wifi_fast_assoc_request;

/**
 * @ingroup hi_wifi_basic
 *
 * Status of sta's connection.CNcomment:获取station连接状态.CNend
 */
typedef struct {
    char ssid[HI_WIFI_MAX_SSID_LEN + 1];    /**< SSID 只支持ASCII字符 */
    unsigned char bssid[HI_WIFI_MAC_LEN];   /**< BSSID */
    unsigned int channel;                   /**< 信道号，取值范围1-14，不同区域取值范围有差异 */
    hi_wifi_conn_status status;             /**< 连接状态 */
} hi_wifi_status;

/**
 * @ingroup hi_wifi_basic
 *
 * Event type of wifi scan done.CNcomment:扫描完成事件.CNend
 */
typedef struct {
    unsigned short bss_num;                 /**< 扫描到的ap数目 */
} event_wifi_scan_done;

/**
 * @ingroup hi_wifi_basic
 *
 * Event type of wifi connected CNcomment:wifi的connect事件信息.CNend
 */
typedef struct {
    char ssid[HI_WIFI_MAX_SSID_LEN + 1];    /**< SSID 只支持ASCII字符 */
    unsigned char bssid[HI_WIFI_MAC_LEN];   /**< BSSID */
    unsigned char ssid_len;                 /**< SSID长度 */
    char ifname[WIFI_IFNAME_MAX_SIZE + 1];  /**< 接口名称 */
} event_wifi_connected;

/**
 * @ingroup hi_wifi_basic
 *
 * Event type of wifi disconnected.CNcomment:wifi的断开事件信息.CNend
 */
typedef struct {
    unsigned char bssid[HI_WIFI_MAC_LEN];    /**< BSSID */
    unsigned short reason_code;              /**< 断开原因 */
    char ifname[WIFI_IFNAME_MAX_SIZE + 1];   /**< 接口名称 */
} event_wifi_disconnected;

/**
 * @ingroup hi_wifi_basic
 *
 * Event type of ap connected sta.CNcomment:ap连接sta事件信息.CNend
 */
typedef struct {
    char addr[HI_WIFI_MAC_LEN];    /**< 连接AP的sta地址 */
} event_ap_sta_connected;

/**
 * @ingroup hi_wifi_basic
 *
 * Event type of ap disconnected sta.CNcomment:ap断开sta事件信息.CNend
 */
typedef struct {
    unsigned char addr[HI_WIFI_MAC_LEN];    /**< AP断开STA的MAC地址 */
    unsigned short reason_code;             /**< AP断开连接的原因值 */
} event_ap_sta_disconnected;

/**
 * @ingroup hi_wifi_basic
 *
 * Event type of mesh connected.CNcomment:mesh的connect事件信息.CNend
 */
typedef struct {
    unsigned char addr[HI_WIFI_MAC_LEN];    /**< MESH连接的peer MAC地址 */
} event_mesh_connected;

/**
 * @ingroup hi_wifi_basic
 *
 * Event type of mesh disconnected.CNcomment:mesh的disconnect事件信息.CNend
 */
typedef struct {
    unsigned char addr[HI_WIFI_MAC_LEN];    /**< MESH断开连接的peer MAC地址 */
    unsigned short reason_code;             /**< MESH断开连接的reason code */
} event_mesh_disconnected;

/**
 * @ingroup hi_wifi_basic
 *
 * Event type wifi information.CNcomment:wifi的事件信息体.CNend
 */
typedef union {
    event_wifi_scan_done wifi_scan_done;            /**< WIFI扫描完成事件信息 */
    event_wifi_connected wifi_connected;            /**< WIFI连接事件信息 */
    event_wifi_disconnected wifi_disconnected;      /**< WIFI断开连接事件信息 */
    event_ap_sta_connected ap_sta_connected;        /**< AP连接事件信息 */
    event_ap_sta_disconnected ap_sta_disconnected;  /**< AP断开连接事件信息 */
    event_mesh_connected mesh_connected;            /**< MESH连接事件信息 */
    event_mesh_disconnected mesh_disconnected;      /**< MESH断开连接事件信息 */
} hi_wifi_event_info;

/**
 * @ingroup hi_wifi_basic
 *
 * Struct of WiFi event.CNcomment:WiFi事件结构体.CNend
 *
 */
typedef struct {
    hi_wifi_event_type event;   /**< 事件类型 */
    hi_wifi_event_info info;    /**< 事件信息 */
} hi_wifi_event;

/**
 * @ingroup hi_wifi_basic
 *
 * Struct of softap's basic config.CNcomment:softap基本配置.CNend
 *
 */
typedef struct {
    char ssid[HI_WIFI_MAX_SSID_LEN + 1];    /**< SSID : 只支持ASCII字符 */
    char key[HI_WIFI_AP_KEY_LEN + 1];       /**< 密码 */
    unsigned char channel_num;              /**< 信道号，取值范围1-14，不同区域取值范围有差异 */
    int ssid_hidden;                        /**< 是否隐藏SSID */
    hi_wifi_auth_mode authmode;             /**< 认证方式 */
    hi_wifi_pairwise pairwise;              /**< 加密方式，可选，不需指定时填为0 */
} hi_wifi_softap_config;

/**
 * @ingroup hi_wifi_basic
 *
 * mac address of softap's user.CNcomment:与softap相连的station mac地址.CNend
 *
 */
typedef struct {
    unsigned char mac[HI_WIFI_MAC_LEN];     /**< MAC address.CNcomment:与softap相连的station mac地址.CNend */
} hi_wifi_ap_sta_info;

/**
 * @ingroup hi_wifi_basic
 *
 * Struct of frame filter config in monitor mode.CNcomment:混杂模式报文接收过滤设置.CNend
 */
typedef struct {
    char mdata_en : 1;  /**< get multi-cast data frame flag. CNcomment: 使能接收组播(广播)数据包.CNend */
    char udata_en : 1;  /**< get single-cast data frame flag. CNcomment: 使能接收单播数据包.CNend */
    char mmngt_en : 1;  /**< get multi-cast mgmt frame flag. CNcomment: 使能接收组播(广播)管理包.CNend */
    char umngt_en : 1;  /**< get single-cast mgmt frame flag. CNcomment: 使能接收单播管理包.CNend */
    char resvd    : 4;  /**< reserved bits. CNcomment: 保留字段.CNend */
} hi_wifi_ptype_filter;

/**
 * @ingroup hi_wifi_basic
 *
 * Struct of WPA psk calc config.CNcomment:计算WPA psk需要设置的参数.CNend
 */
typedef struct {
    unsigned char ssid[HI_WIFI_MAX_SSID_LEN + 1]; /**< SSID 只支持ASCII字符 */
    char key[HI_WIFI_AP_KEY_LEN + 1];             /**< 密码 */
} hi_wifi_sta_psk_config;

/**
 * @ingroup hi_wifi_basic
 *
 * callback function definition of monitor mode.CNcommment:混杂模式收包回调接口定义.CNend
 */
typedef int (*hi_wifi_promis_cb)(void* recv_buf, int frame_len, signed char rssi);

/**
 * @ingroup hi_wifi_basic
 *
 * callback function definition of wifi event.CNcommment:wifi事件回调接口定义.CNend
 */
typedef void (*hi_wifi_event_cb)(const hi_wifi_event *event);

/**
* @ingroup  hi_wifi_basic
* @brief  Wifi initialize.CNcomment:wifi初始化.CNend
*
* @par Description:
        Wifi driver initialize.CNcomment:wifi驱动初始化，不创建wifi设备.CNend
*
* @attention  NULL
* @param  vap_res_num   [IN]  Type #const unsigned char, vap num[rang: 1-3].CNcomment:vap资源个数，取值[1-3].CNend
* @param  user_res_num  [IN]  Type #const unsigned char, user resource num[1-7].
*           CNcomment:用户资源个数，多vap时共享，取值[1-7].CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other    Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_init(const unsigned char vap_res_num, const unsigned char user_res_num);

/**
* @ingroup  hi_wifi_basic
* @brief  Wifi de-initialize.CNcomment:wifi去初始化.CNend
*
* @par Description:
*           Wifi driver de-initialize.CNcomment:wifi驱动去初始化.CNend
*
* @attention  NULL
* @param  NULL
*
* @retval #HISI_OK  Excute successfully
* @retval #Other    Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_deinit(void);

/**
* @ingroup  hi_wifi_basic
* @brief  Get wifi initialize status.CNcomment:获取wifi初始化状态.CNend
*
* @par Description:
        Get wifi initialize status.CNcomment:获取wifi初始化状态.CNend
*
* @attention  NULL
* @param  NULL
*
* @retval #1  Wifi is initialized.CNcoment:Wifi已经初始化.CNend
* @retval #0  Wifi is not initialized.CNcoment:Wifi没有初始化.CNend
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
unsigned char hi_wifi_get_init_status(void);

/**
* @ingroup  hi_wifi_basic
* @brief  Set protocol mode of sta.CNcomment:设置station接口的protocol模式.CNend
*
* @par Description:
*           Set protocol mode of sta, set before calling hi_wifi_sta_start().\n
*           CNcomment:配置station接口的protocol模式, 在sta start之前调用.CNend
*
* @attention  Default mode 802.11BGN CNcomment:默认模式 802.11BGN.CNend
* @param  mode            [IN]     Type #hi_wifi_protocol_mode, protocol mode.
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_set_protocol_mode(hi_wifi_protocol_mode mode);

/**
* @ingroup  hi_wifi_basic
* @brief  Get protocol mode of.CNcomment:获取station接口的protocol模式.CNend
*
* @par Description:
*           Get protocol mode of station.CNcomment:获取station接口的protocol模式.CNend
*
* @attention  NULL
* @param      NULL
*
* @retval #hi_wifi_protocol_mode protocol mode.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
hi_wifi_protocol_mode hi_wifi_sta_get_protocol_mode(void);

/**
* @ingroup  hi_wifi_basic
* @brief  Config pmf settings of sta.CNcomment:配置station的pmf.CNend
*
* @par Description:
*           Config pmf settings of sta, set before sta start.CNcomment:配置station的pmf, 在sta start之前调用.CNend
*
* @attention  Default pmf enum value 1. CNcomment:默认pmf枚举值1.CNend
* @param  pmf           [IN]     Type #hi_wifi_pmf_options, pmf enum value.CNcoment:pmf枚举值.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_set_pmf(hi_wifi_pmf_options pmf);

/**
* @ingroup  hi_wifi_basic
* @brief  Get pmf settings of sta.CNcomment:获取station的pmf设置.CNend
*
* @par Description:
*           Get pmf settings of sta.CNcomment:获取station的pmf设置.CNend
*
* @attention  NULL
* @param      NULL
*
* @retval #hi_wifi_pmf_options pmf enum value.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
hi_wifi_pmf_options hi_wifi_get_pmf(void);

/**
* @ingroup  hi_wifi_basic
* @brief  Start wifi station.CNcomment:开启STA.CNend
*
* @par Description:
*           Start wifi station.CNcomment:开启STA.CNend
*
* @attention  1. Multiple interfaces of the same type are not supported.CNcomment:1. 不支持使用多个同类型接口.CNend\n
*             2. Dual interface coexistence support: STA + AP or STA + MESH.
*                CNcomment:2. 双接口共存支持：STA + AP or STA + MESH.CNend\n
*             3. Start timeout 5s.CNcomment:3. 启动超时时间5s.CNend\n
*             4. The memories of <ifname> and <len> should be requested by the caller，
*                the input value of len must be the same as the length of ifname（the recommended length is 17Bytes）.\n
*                CNcomment:4. <ifname>和<len>由调用者申请内存，用户写入len的值必须与ifname长度一致（建议长度为17Bytes）.CNend
* @param  ifname          [IN/OUT]     Type #char *, device name.CNcomment:接口名.CNend
* @param  len             [IN/OUT]     Type #int *, length of device name.CNcomment:接口名长度.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_start(char *ifname, int *len);

/**
* @ingroup  hi_wifi_basic
* @brief  Close wifi station.CNcomment:关闭STA.CNend
*
* @par Description:
*           Close wifi station.CNcomment:关闭STA.CNend
*
* @attention  NULL
* @param  NULL
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_stop(void);

/**
* @ingroup  hi_wifi_basic
* @brief  Start sta basic scanning in all channels.CNcomment:station进行全信道基础扫描.CNend
*
* @par Description:
*           Start sta basic scanning in all channels.CNcomment:启动station全信道基础扫描.CNend
*
* @attention  NULL
* @param     NULL
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_scan(void);

/**
* @ingroup  hi_wifi_basic
* @brief  Start station scanning with specified parameter.CNcomment:station执行带特定参数的扫描.CNend
*
* @par Description:
*           Start station scanning with specified parameter.CNcomment:station执行带特定参数的扫描.CNend
*
* @attention  1. advance scan can scan with ssid only,channel only,bssid only,prefix_ssid only，
*                and the combination parameters scanning does not support.\n
*             CNcomment:1. 高级扫描分别单独支持 ssid扫描，信道扫描，bssid扫描，ssid前缀扫描, 不支持组合参数扫描方式.CNend\n
*             2. Scanning mode, subject to the type set by scan_type.
*              CNcomment:2 .扫描方式，以scan_type传入的类型为准。CNend
*             3. SSID only supports ASCII characters.\n
*                CNcomment:3. SSID 只支持ASCII字符.CNend
* @param  sp            [IN]    Type #hi_wifi_scan_params * parameters of scan.CNcomment:扫描网络参数设置.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_advance_scan(hi_wifi_scan_params *sp);

/**
* @ingroup  hi_wifi_basic
* @brief  station进行扫描。
*
* @par Description:
*           Get station scan result.CNcomment:获取station扫描结果.CNend
* @attention  1. The memories of <ap_list> and <ap_num> memories are requested by the caller. \n
*             The <ap_list> size up to : sizeof(hi_wifi_ap_info ap_list) * 32. \n
*             CNcomment:1. <ap_list>和<ap_num>由调用者申请内存, \n
*             <ap_list>size最大为：sizeof(hi_wifi_ap_info ap_list) * 32.CNend \n
*             2. ap_num: parameters can be passed in to specify the number of scanned results.The maximum is 32. \n
*             CNcomment:2. ap_num: 可以传入参数，指定获取扫描到的结果数量，最大为32。CNend \n
*             3. If the user callback function is used, ap num refers to bss_num in event_wifi_scan_done. \n
*             CNcomment:3. 如果使用上报用户的回调函数，ap_num参考event_wifi_scan_done中的bss_num。CNend \n
*             4. ap_num should be same with number of hi_wifi_ap_info structures applied,
*                Otherwise, it will cause memory overflow. \n
*             CNcomment:4. ap_num和申请的hi_wifi_ap_info结构体数量一致，否则可能造成内存溢出。CNend \n
*             5. SSID only supports ASCII characters.\n
*             CNcomment:5. SSID 只支持ASCII字符.CNend
* @param  ap_list         [IN/OUT]    Type #hi_wifi_ap_info * scan result.CNcomment:扫描的结果.CNend
* @param  ap_num          [IN/OUT]    Type #unsigned int *, number of scan result.CNcomment:扫描到的网络数目.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_scan_results(hi_wifi_ap_info *ap_list, unsigned int *ap_num);

/**
* @ingroup  hi_wifi_basic
* @brief  sta start connect.CNcomment:station进行连接网络.CNend
*
* @par Description:
*           sta start connect.CNcomment:station进行连接网络.CNend
*
* @attention  1.<ssid> and <bssid> cannot be empty at the same time. CNcomment:1. <ssid>与<bssid>不能同时为空.CNend\n
*             2. When <auth_type> is set to OPEN, the <passwd> parameter is not required.
*                CNcomment:2. <auth_type>设置为OPEN时，无需<passwd>参数.CNend\n
*             3. This function is non-blocking.CNcomment:3. 此函数为非阻塞式.CNend\n
*             4. Pairwise can be set, default is 0.CNcomment:4. pairwise 可设置, 默认为0.CNend\n
*             5. If the station is already connected to a network, disconnect the existing connection and
*                then connect to the new network.\n
*                CNcomment:5. 若station已接入某个网络，则先断开已有连接，然后连接新网络.CNend\n
*             6. If the wrong SSID, BSSID or key is passed in, the HISI_OK will be returned,
*                but sta cannot connect the ap.
*                CNcomment:6. 如果传入错误的ssid，bssid或者不正确的密码，返回成功，但连接ap失败。CNend\n
*             7. SSID only supports ASCII characters.
*                CNcomment:7. SSID 只支持ASCII字符.CNend
*             8. Only support auth mode as bellow: \n
*                 HI_WIFI_SECURITY_WPAPSK_WPA2PSK_MIX, \n
*                 HI_WIFI_SECURITY_WPA2PSK, \n
*                 HI_WIFI_SECURITY_WEP, \n
*                 HI_WIFI_SECURITY_OPEN \n
*                CNcomment:8. 只支持以下认证模式：\n
*                 HI_WIFI_SECURITY_WPAPSK_WPA2PSK_MIX, \n
*                 HI_WIFI_SECURITY_WPA2PSK, \n
*                 HI_WIFI_SECURITY_WEP, \n
*                 HI_WIFI_SECURITY_OPEN \n

* @param  req    [IN]    Type #hi_wifi_assoc_request * connect parameters of network.CNcomment:连接网络参数设置.CNend
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_connect(hi_wifi_assoc_request *req);

/**
* @ingroup  hi_wifi_basic
* @brief  Start fast connect.CNcomment:station进行快速连接网络.CNend
*
* @par Description:
*           Start fast connect.CNcomment:station进行快速连接网络.CNend
*
* @attention  1. <ssid> and <bssid> cannot be empty at the same time. CNcomment:1．<ssid>与<bssid>不能同时为空.CNend\n

*             2. When <auth_type> is set to OPEN, the <passwd> parameter is not required.
*                CNcomment:2．<auth_type>设置为OPEN时，无需<passwd>参数.CNend\n
*             3. <chn> There are differences in the range of values, and China is 1-13.
*                CNcomment:3．<chn>取值范围不同区域有差异，中国为1-13.CNend\n
*             4. This function is non-blocking.4．此函数为非阻塞式.CNend\n
*             5. Pairwise can be set, set to zero by default.CNcomment:5. pairwise 可设置,默认置零.CNend\n
*             6. <psk>和<psk_flag> are optional parameters, set to zero by default.
*                CNcomment:6. <psk>和<psk_flag>为可选参数，无需使用时填0.CNend\n
*             7. If the wrong SSID, BSSID or key is passed in, the HISI_FAIL will be returned,
*                and sta cannot connect the ap.
*                CNcomment:7. 如果传入错误的ssid，bssid或者不正确的密码，返回失败并且连接ap失败。CNend\n
*             8. SSID only supports ASCII characters.
*                CNcomment:8. SSID 只支持ASCII字符.CNend
* @param fast_request [IN] Type #hi_wifi_fast_assoc_request *,fast connect parameters. CNcomment:快速连接网络参数.CNend

* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_fast_connect(hi_wifi_fast_assoc_request *fast_request);

/**
* @ingroup  hi_wifi_basic
* @brief  Disconnect from network.CNcomment:station断开相连的网络.CNend
*
* @par Description:
*           Disconnect from network.CNcomment:station断开相连的网络.CNend
*
* @attention  NULL
* @param  NULL
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_disconnect(void);

/**
* @ingroup  hi_wifi_basic
* @brief  Set reconnect policy.CNcomment:station设置重新连接网络机制.CNend
*
* @par Description:
*           Set reconnect policy.CNcomment:station设置重新连接网络机制.CNend
*
* @attention  1. It is recommended called after STA start or connected successfully.
*             CNcomment:1. 在STA启动后或者关联成功后调用该接口.CNend\n
*             2. The reconnection policy will be triggered when the station is disconnected from ap.\n
*             CNcomment:2. 重连机制将于station下一次去关联时生效,当前已经去关联设置无效.CNend\n
*             3. The Settings will take effect on the next reconnect timer.\n
*             CNcomment:3. 重关联过程中更新重关联配置将于下一次重连计时生效.CNend\n
*             4. After calling station connect/disconnect or station stop, stop reconnecting.
*             CNcomment:4. 调用station connect/disconnect或station stop，停止重连.CNend\n
*             5. If the target network cannot be found by scanning,
                 the reconnection policy cannot trigger to take effect.\n
*             CNcomment:5. 若扫描不到目标网络，重连机制无法触发生效.CNend\n
*             6. When the <seconds> value is 65535, it means infinite loop reconnection.
*             CNcomment:6. <seconds>取值为65535时，表示无限次循环重连.CNend\n
*             7.Enable reconnect, user and lwip will not receive disconnect event when disconnected from ap until 15
*               seconds later and still don't reconnect to ap successfully.
*             CNcomment:7. 使能自动重连,wifi将在15s内尝试自动重连并在此期间不上报去关联事件到用户和lwip协议栈,
*                          做到15秒内重连成功用户和上层网络不感知.CNend\n

* @param  enable        [IN]    Type #int enable reconnect.0-disable/1-enable.CNcomment:使能重连网络参数.CNend
* @param  seconds       [IN]    Type #unsigned int reconnect timeout in seconds for once, range:[2-65535].
*                                                  CNcomment:单次重连超时时间，取值[2-65535].CNend
* @param  period        [IN]    Type #unsigned int reconnect period in seconds, range:[1-65535].
                                                   CNcomment:重连间隔周期，取值[1-65535].CNend
* @param  max_try_count [IN]    Type #unsigned int max reconnect try count number，range:[1-65535].
                                                   CNcomment:最大重连次数，取值[1-65535].CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_set_reconnect_policy(int enable, unsigned int seconds,
    unsigned int period, unsigned int max_try_count);

/**
* @ingroup  hi_wifi_basic
* @brief  Get status of sta.CNcomment:获取station连接的网络状态.CNend
*
* @par Description:
*           Get status of sta.CNcomment:获取station连接的网络状态.CNend
*
* @attention  NULL
* @param  connect_status  [IN/OUT]    Type #hi_wifi_status *, connect status， memory is requested by the caller.
*                                                             CNcomment:连接状态, 由调用者申请内存.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_get_connect_info(hi_wifi_status *connect_status);

/**
* @ingroup  hi_wifi_basic
* @brief  Start pbc connect in WPS.CNcomment:设置WPS进行pbc连接.CNend
*
* @par Description:
*           Start pbc connect in WPS.CNcomment:设置WPS进行pbc连接.CNend
*
* @attention  1. bssid can be NULL or MAC. CNcomment:1. bssid 可以指定mac或者填NULL.CNend
* @param  bssid   [IN]  Type #unsigned char * mac address
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_wps_pbc(unsigned char *bssid);

/**
* @ingroup  hi_wifi_basic
* @brief  Start pin connect in WPS.CNcomment:WPS通过pin码连接网络.CNend
*
* @par Description:
*           Start pin connect in WPS.CNcomment:WPS通过pin码连接网络.CNend
*
* @attention  1. Bssid can be NULL or MAC. CNcomment:1. bssid 可以指定mac或者填NULL.CNend \n
*             2. Decimal only WPS pin code length is 8 Bytes.CNcomment:2. WPS中pin码仅限十进制，长度为8 Bytes.CNend
* @param  pin      [IN]   Type #char * pin code
* @param  bssid    [IN]   Type #unsigned char * mac address
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_wps_pin(char *pin, unsigned char *bssid);

/**
* @ingroup  hi_wifi_basic
* @brief  Get pin code.CNcomment:WPS获取pin码.CNend
*
* @par Description:
*           Get pin code.CNcomment:WPS获取pin码.CNend
*
* @attention  Decimal only WPS pin code length is 8 Bytes.CNcomment:WPS中pin码仅限十进制，长度为8 Bytes.CNend
* @param  pin    [IN/OUT]   Type #char *, pin code buffer, should be obtained, length is 9 Bytes.
*                                                               The memory is requested by the caller.\n
*                                       CNcomment:待获取pin码,长度为9 Bytes。由调用者申请内存.CNend
* @param  len    [IN]   Type #int, length of pin code
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_wps_pin_get(char* pin, unsigned int len);

/**
* @ingroup  hi_wifi_basic
* @brief  Get rssi value.CNcomment:获取rssi值.CNend
*
* @par Description:
*           Get current rssi of ap which sta connected to.CNcomment:获取sta当前关联的ap的rssi值.CNend
*
* @attention  NULL
* @param  NULL
*
* @retval #0x7F          Invalid value.
* @retval #Other         rssi
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_get_ap_rssi(void);

/**
* @ingroup  hi_wifi_basic
* @brief  Set sta pm mode.CNcomment:设置STA的低功耗参数.CNend
*
* @par Description:
*           Set sta pm mode.CNcomment:设置STA的低功耗参数..CNend
*
* @attention  1.CNcomment:参数值越小,功耗越低但性能表现和抗干扰会越差,建议使用默认值或根据流量动态配置.CNend\n
              2.CNcomment:所有参数配置0,表示该参数使用默认值.CNend\n
              3.CNcomment:仅支持设置STA的低功耗参数.CNend\n
              4.CNcomment:需要在关联成功后配置,支持动态配置.CNend\n
              5.CNcomment:定时器首次启动不计数,故实际睡眠时间为配置的重启次数+1乘以周期.CNend\n
* @param  timer       [IN]  Type  #unsigned char CNcomment:低功耗定时器周期,默认50ms,取值[0-100]ms.CNend
* @param  time_cnt    [IN]  Type  #unsigned char CNcomment:低功耗定时器重启次数,达到该次数后wifi无数据收发则进入休眠,
                                                 默认为4,取值[0-10].CNend
* @param  bcn_timeout [IN]  Type  #unsigned char CNcomment:等待接收beacon的超时时间,默认10ms,取值[0-100]ms.CNend
* @param  mcast_timeout [IN]  Type  #unsigned char CNcomment:等待接收组播/广播帧的超时时间,默认30ms,取值[0-100]ms.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_sta_set_pm_param(unsigned char timer, unsigned char time_cnt, unsigned char bcn_timeout,
                             unsigned char mcast_timeout);

/**
* @ingroup  hi_wifi_basic
* @brief  WPA PSK Calculate.CNcomment:计算WPA PSK.CNend
*
* @par Description:
*           PSK Calculate.CNcomment:计算psk.CNend
*
* @attention  1. support only WPA PSK. CNcomment:1. 只支持WPA psk计算.CNend
*             2. SSID only supports ASCII characters. CNcomment:2. SSID 只支持ASCII字符.CNend
* @param  psk_config    [IN]    Type #hi_wifi_sta_psk_config
* @param  get_psk       [IN/OUT]   Type #const unsigned char *，Psk to be obtained, length is 32 Bytes.
*                                                               The memory is requested by the caller.
*                                       CNcomment:待获取psk,长度为32 Bytes。由调用者申请内存.CNend
* @param  psk_len       [IN]    Type #unsigned int
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_psk_calc(hi_wifi_sta_psk_config psk_config, unsigned char *get_psk, unsigned int psk_len);

/**
* @ingroup  hi_wifi_basic
* @brief  WPA PSK Calculate，then keep it inside .CNcomment:计算WPA PSK, 并做内部保存.CNend
*
* @par Description:
*           psk Calculate.CNcomment:计算psk.CNend
*
* @attention  1. support only WPA PSK. CNcomment:1. 只支持WPA psk计算.CNend
*             2. SSID only supports ASCII characters. CNcomment:2. SSID 只支持ASCII字符.CNend
* @param  psk_config    [IN]    Type #hi_wifi_sta_psk_config
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_psk_calc_and_store(hi_wifi_sta_psk_config psk_config);

/**
* @ingroup  hi_wifi_basic
* @brief  config calling mode of user's callback interface.CNcomment:配置用户回调接口的调用方式.CNend
*
* @par Description:
*           config calling mode of user's callback interface.CNcomment:配置用户回调接口的调用方式.CNend
*
* @attention  1. Wpa's task has high priority and call wifi's api directly may be cause error.
              CNcomment:1. wpa线程优先级高,直接调用方式下在该回调接口内再次调用某些api会导致线程卡死.CNend
              2. If you have create a task in your app, you should use mode:0, or mode:1 is adervised.
              CNcomment:2. 上层应用已创建task来处理事件回调建议使用直接调用方式,否则建议使用线程调用方式.CNend
              3. Configuration will keep till system reboot, set again when you start a new station.
              CNcomment:3. 参数会保持上一次设置值直到系统重启,重新启动station后建议再配置一次.CNend
              4. Configuration will work immediately whenever you set.
              CNcomment:4. 可随时配置该参数,配置成功即生效.CNend
* @param  mode       [IN]    Type #unsigned char , call mode, 1:direct and 0:create task[default].
                             CNcomment:回调调用方式,1:wpa线程直接调用,0:新建一个低优先级线程调用,默认.CNend
* @param  task_prio  [IN]    Type #unsigned char , task priority, range(10-50) .
                             CNcomment:新建线程优先级,取值范围(10-50).CNend
* @param  stack_size [IN]    Type #unsigned short , task stack size, more than 1K bytes, default: 2k.
                             CNcomment:新建线程栈空间,需大于1K,默认2k.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_config_callback(unsigned char mode, unsigned char task_prio, unsigned short stack_size);

/**
* @ingroup  hi_wifi_basic
* @brief  register user callback interface.CNcomment:注册回调函数接口.CNend
*
* @par Description:
*           register user callback interface.CNcomment:注册回调函数接口.CNend
*
* @attention  NULL
* @param  event_cb  [OUT]    Type #hi_wifi_event_cb *, event callback .CNcomment:回调函数.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_register_event_callback(hi_wifi_event_cb event_cb);

/**
* @ingroup  hi_wifi_basic
* @brief  Set protocol mode of softap.CNcomment:设置softap接口的protocol模式.CNend
*
* @par Description:
*           Set protocol mode of softap.CNcomment:设置softap接口的protocol模式.CNend\n
*           Initiallize config, set before softap start.CNcomment:初始配置,在softap start之前调用.CNend
*
* @attention  Default mode(802.11BGN) CNcomment:默认模式（802.11BGN）.CNend
* @param  mode            [IN]     Type  #hi_wifi_protocol_mode protocol mode.
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_softap_set_protocol_mode(hi_wifi_protocol_mode mode);

/**
* @ingroup  hi_wifi_basic
* @brief  Get protocol mode of softap.CNcomment:获取softap接口的protocol模式.CNend
*
* @par Description:
*           Get protocol mode of softap.CNcomment:获取softap接口的protocol模式.CNend
*
* @attention  NULL
* @param      NULL
*
* @retval #hi_wifi_protocol_mode protocol mode.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
hi_wifi_protocol_mode hi_wifi_softap_get_protocol_mode(void);

/**
* @ingroup  hi_wifi_basic
* @brief  Set softap's beacon interval.CNcomment:设置softap的beacon周期.CNend
*
* @par Description:
*           Set softap's beacon interval.CNcomment:设置softap的beacon周期.CNend
*           Initialized config sets before interface starts.CNcomment:初始配置softap启动之前调用.CNend
*
* @attention  NULL
* @param  beacon_period      [IN]     Type  #int beacon period in milliseconds, range(33ms~1000ms), default(100ms)
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_softap_set_beacon_period(int beacon_period);

/**
* @ingroup  hi_wifi_basic
* @brief  Set softap's dtim count.CNcomment:设置softap的dtim周期.CNend
*
* @par Description:
*           Set softap's dtim count.CNcomment:设置softap的dtim周期.CNend
*           Initialized config sets before interface starts.CNcomment:初始配置softap启动之前调用.CNend
*
* @attention  NULL
* @param  dtim_period     [IN]     Type  #int, dtim period , range(1~30), default(2)
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_softap_set_dtim_period(int dtim_period);

/**
* @ingroup  hi_wifi_basic
* @brief  Set update time of softap's group key.CNcomment:配置softap组播秘钥更新时间.CNend
*
* @par Description:
*           Set update time of softap's group key.CNcomment:配置softap组播秘钥更新时间.CNend\n
*           Initialized config sets before interface starts.CNcomment:初始配置softap启动之前调用.CNend\n
*           If you need to use the rekey function, it is recommended to use WPA+WPA2-PSK + CCMP encryption.
*           CNcomment:若需要使用rekey功能，推荐使用WPA+WPA2-PSK + CCMP加密方式.CNend
*
* @attention  When using wpa2psk-only + CCMP encryption, rekey is forced to 86400s by default.
*    CNcomment:当使用wpa2psk-only + CCMP加密方式时  ，rekey默认强制改为 86400.CNend
* @param  wpa_group_rekey [IN]     Type  #int, update time in seconds, range(30s-86400s), default(86400s)
*                                   CNcomment:更新时间以秒为单位，范围（30s-86400s）,默认（86400s）.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_softap_set_group_rekey(int wifi_group_rekey);

/**
* @ingroup  hi_wifi_basic
* @brief  Set short-gi of softap.CNcomment:设置softap的SHORT-GI功能.CNend
*
* @par Description:
*           Enable or disable short-gi of softap.CNcomment:开启或则关闭softap的SHORT-GI功能.CNend\n
*           Initialized config sets before interface starts.CNcomment:初始配置softap启动之前调用.CNend
* @attention  NULL
* @param  flag            [IN]    Type  #int, enable(1) or disable(0). default enable(1).
                                        CNcomment:使能标志，默认使能（1）.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_softap_set_shortgi(int flag);

/**
* @ingroup  hi_wifi_basic
* @brief  Start softap interface.CNcomment:开启SoftAP.CNend
*
* @par Description:
*           Start softap interface.CNcomment:开启SoftAP.CNend
*
* @attention  1. Multiple interfaces of the same type are not supported.CNcomment:不支持使用多个同类型接口.CNend\n
*             2. Dual interface coexistence support: STA + AP. CNcomment:双接口共存支持：STA + AP.CNend \n
*             3. Start timeout 5s.CNcomment:启动超时时间5s。CNend \n
*             4. Softap key length range(8 Bytes - 64 Bytes).CNcomment:softap key长度范围（8 Bytes - 64 Bytes）.CNend \n
*             5. Only support auth mode as bellow: \n
*                 HI_WIFI_SECURITY_WPAPSK_WPA2PSK_MIX, \n
*                 HI_WIFI_SECURITY_WPA2PSK, \n
*                 HI_WIFI_SECURITY_OPEN \n
*                CNcomment:5. 只支持以下认证模式：\n
*                 HI_WIFI_SECURITY_WPAPSK_WPA2PSK_MIX, \n
*                 HI_WIFI_SECURITY_WPA2PSK, \n
*                 HI_WIFI_SECURITY_OPEN.CNend \n
*             6. The memories of <ifname> and <len> should be requested by the caller，
*                the input value of len must be the same as the length of ifname（the recommended length is 17Bytes）.\n
*                CNcomment:6. <ifname>和<len>由调用者申请内存，用户写入len的值必须与ifname长度一致（建议长度为17Bytes）.CNend
*             7. SSID only supports ASCII characters. \n
*                CNcomment:7. SSID 只支持ASCII字符.CNend
* @param  conf            [IN]      Type  #hi_wifi_softap_config * softap's configuration.CNcomment:SoftAP配置.CNend
* @param  ifname          [IN/OUT]  Type  #char interface name.CNcomment:接口名字.CNend
* @param  len             [IN/OUT]  Type  #int * interface name length.CNcomment:接口名字长度.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_softap_start(hi_wifi_softap_config *conf, char *ifname, int *len);

/**
* @ingroup  hi_wifi_basic
* @brief  Close softap interface.CNcomment:关闭SoftAP.CNend
*
* @par Description:
*           Close softap interface.CNcomment:关闭SoftAP.CNend
*
* @attention  NULL
* @param  NULL
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_softap_stop(void);

/**
* @ingroup  hi_wifi_basic
* @brief  Get all user's information of softap.CNcomment:softap获取已连接的station的信息.CNend
*
* @par Description:
*           Get all user's information of softap.CNcomment:softap获取已连接的station的信息.CNend
*
* @attention  1. parameter of sta_num max value is set in hi_wifi_init.
*                CNcomment:1. sta_num 最大值在hi_wifi_init中设置.CNend\n
*             2. The memories of <sta_list> and <sta_num> memories are requested by the caller.\n
*                CNcomment:2. <sta_list>和<sta_num>由调用者申请内存.CNend
* @param  sta_list        [IN/OUT]  Type  #hi_wifi_ap_sta_info *, station information.CNcomment:STA信息.CNend
* @param  sta_num         [IN/OUT]  Type  #unsigned int *, station number.CNcomment:STA个数.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_softap_get_connected_sta(hi_wifi_ap_sta_info *sta_list, unsigned int *sta_num);

/**
* @ingroup  hi_wifi_basic
* @brief  Softap deauth user by mac address.CNcomment:softap指定断开连接的station网络.CNend
*
* @par Description:
*          Softap deauth user by mac address.CNcomment:softap指定断开连接的station网络.CNend
*
* @attention  NULL
* @param  addr             [IN]     Type  #const unsigned char *, station mac address.CNcomment:MAC地址.CNend
* @param  addr_len         [IN]     Type  #unsigned char, station mac address length, must be 6.
*                                         CNcomment:MAC地址长度,必须为6.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_softap_deauth_sta(const unsigned char *addr, unsigned char addr_len);

/**
* @ingroup  hi_wifi_basic
* @brief  set mac address.CNcomment:设置MAC地址.CNend
*
* @par Description:
*           Set original mac address.CNcomment:设置起始mac地址.CNend\n
*           mac address will increase or recycle when adding or deleting device.
*           CNcomment:添加设备mac地址递增，删除设备回收对应的mac地址.CNend
*
* @attention  NULL
* @param  mac_addr          [IN]     Type #char *, mac address.CNcomment:MAC地址.CNend
* @param  mac_len           [IN]     Type #unsigned char, mac address length.CNcomment:MAC地址长度.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other    Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_set_macaddr(const char *mac_addr, unsigned char mac_len);

/**
* @ingroup  hi_wifi_basic
* @brief  get mac address.CNcomment:获取MAC地址.CNend
*
* @par Description:
*           Get original mac address.CNcomment:获取mac地址.CNend\n
*           mac address will increase or recycle when adding device or deleting device.
*           CNcomment:添加设备mac地址递增，删除设备回收对应的mac地址.CNend
*
* @attention  NULL
* @param  mac_addr          [OUT]    Type #char *, mac address.
* @param  mac_len           [IN]     Type #unsigned char, mac address length.
*
* @retval #HISI_OK  Excute successfully
* @retval #Other    Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_get_macaddr(char *mac_addr, unsigned char mac_len);

/**
* @ingroup  hi_wifi_basic
* @brief  Set country code.CNcomment:设置国家码.CNend
*
* @par Description:
*           Set country code(two uppercases).CNcomment:设置国家码，由两个大写字符组成.CNend
*
* @attention  1.Before setting the country code, you must call hi_wifi_init to complete the initialization.
*             CNcomment:设置国家码之前，必须调用hi_wifi_init初始化完成.CNend\n
*             2.cc_len应大于等于3.CNcomment:cc_len should be greater than or equal to 3.CNend
* @param  cc               [IN]     Type  #char *, country code.CNcomment:国家码.CNend
* @param  cc_len           [IN]     Type  #unsigned char, country code length.CNcomment:国家码长度.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_set_country(const char *cc, unsigned char cc_len);

/**
* @ingroup  hi_wifi_basic
* @brief  Get country code.CNcomment:获取国家码.CNend
*
* @par Description:
*           Get country code.CNcomment:获取国家码，由两个大写字符组成.CNend
*
* @attention  1.Before getting the country code, you must call hi_wifi_init to complete the initialization.
*             CNcomment:获取国家码之前，必须调用hi_wifi_init初始化完成.CNend
* @param  cc               [OUT]     Type  #char *, country code.CNcomment:国家码.CNend
* @param  len              [IN/OUT]  Type  #int *, country code length.CNcomment:国家码长度.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_get_country(char *cc, int *len);

/**
* @ingroup  hi_wifi_basic
* @brief  Set bandwidth.CNcomment:设置带宽.CNend
*
* @par Description:
*           Set bandwidth, support 5M/10M/20M.CNcomment:设置接口的工作带宽，支持5M 10M 20M带宽的设置.CNend
*
* @attention  NULL
* @param  ifname           [IN]     Type  #const char *, interface name.CNcomment:接口名.CNend
* @param  ifname_len       [IN]     Type  #unsigned char, interface name length.CNcomment:接口名长度.CNend
* @param  bw               [IN]     Type  #hi_wifi_bw, bandwidth enum.CNcomment:带宽.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_set_bandwidth(const char *ifname, unsigned char ifname_len, hi_wifi_bw bw);

/**
* @ingroup  hi_wifi_basic
* @brief  Get bandwidth.CNcomment:获取带宽.CNend
*
* @par Description:
*           Get bandwidth.CNcomment:获取带宽.CNend
*
* @attention  NULL
* @param  ifname           [IN]     Type  #const char *, interface name.CNcomment:接口名.CNend
* @param  ifname_len       [IN]     Type  #unsigned char, interface name length.CNcomment:接口名长度.CNend
*
* @retval #bandwidth enum.CNcomment:带宽的枚举值.CNend
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
hi_wifi_bw hi_wifi_get_bandwidth(const char *ifname, unsigned char ifname_len);

/**
* @ingroup  hi_wifi_basic
* @brief  Set channel.CNcomment:设置信道.CNend
*
* @par Description:
*           Set channel.CNcomment:设置信道.CNend
*
* @attention  NULL
* @param  ifname           [IN]     Type  #const char *, interface name.CNcomment:接口名.CNend
* @param  ifname_len       [IN]     Type  #unsigned char, interface name length.CNcomment:接口名长度.CNend
* @param  channel          [IN]     Type  #int *, listen channel.CNcomment:信道号.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_set_channel(const char *ifname, unsigned char ifname_len, int channel);

/**
* @ingroup  hi_wifi_basic
* @brief  Get channel.CNcomment:获取信道.CNend
*
* @par Description:
*           Get channel.CNcomment:获取信道.CNend
*
* @attention  NULL
* @param  ifname           [IN]     Type  #const char *, interface name.CNcomment:接口名.CNend
* @param  ifname_len       [IN]     Type  #unsigned char, interface name length.CNcomment:接口名长度.CNend
*
* @retval #HI_WIFI_INVALID_CHANNEL
* @retval #Other                   chanel value.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_get_channel(const char *ifname, unsigned char ifname_len);

/**
* @ingroup  hi_wifi_basic
* @brief  Set monitor mode.CNcomment:设置混杂模式.CNend
*
* @par Description:
*           Enable/disable monitor mode of interface.CNcomment:设置指定接口的混杂模式使能.CNend
*
* @attention  NULL
* @param  ifname           [IN]     Type  #const char * interface name.CNcomment:接口名.CNend
* @param  enable           [IN]     Type  #int enable(1) or disable(0).CNcomment:开启/关闭.CNend
* @param  filter           [IN]     Type  #hi_wifi_ptype_filter * filtered frame type enum.CNcomment:过滤列表.CNend
*
* @retval #HI_ERR_SUCCESS  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_promis_enable(const char *ifname, int enable, const hi_wifi_ptype_filter *filter);

/**
* @ingroup  hi_wifi_basic
* @brief  Register receive callback in monitor mode.CNcomment:注册混杂模式的收包回调函数.CNend
*
* @par Description:
*           Register receive callback in monitor mode.CNcomment:注册混杂模式的收包回调函数.CNend\n
*           Wifi driver will put the receive frames to this callback.
*           CNcomment:驱动将混杂模式的收到的报文递交到注册的回调函数处理.CNend
*
* @attention  NULL
* @param  data_cb          [IN]     Type  #hi_wifi_promis_cb callback function pointer.CNcomment:混杂模式回调函数.CNend
*
* @retval #HI_ERR_SUCCESS  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_promis_set_rx_callback(hi_wifi_promis_cb data_cb);

/**
* @ingroup  hi_wifi_basic
* @brief    Open/close system power save.CNcomment:开启/关闭WiFi低功耗模式并配置预期休眠时间.CNend
*
* @par Description:
*           Open/close system power save.CNcomment:开启/关闭WiFi低功耗模式并配置预期休眠时间.CNend
*
* @attention  NULL
* @param  enable     [IN] Type  #unsigned char, enable(1) or disable(0).CNcomment:开启/关闭WiFi低功耗.CNend
* @param  sleep_time [IN] Type  #unsigned int, expected sleep time(uint: ms). CNcomment:预期休眠时间(单位: 毫秒),
*                               参考有效范围33ms~4000ms, 准确的时间根据dtim*beacon和sleep_time值计算,
*                               关闭低功耗或者不配置有效休眠时间时需要将sleep_time配置为0(休眠时间由关联的ap决定).CNend
*
* @retval #HI_ERR_SUCCESS  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_set_pm_switch(unsigned char enable, unsigned int sleep_time);

/**
* @ingroup  hi_wifi_basic
* @brief    Set arp offload on/off.CNcomment:设置arp offload 打开/关闭.CNend
*
* @par Description:
*           Set arp offload on with ip address, or set arp offload off.
*           CNcomment:设置arp offload打开、并且设置相应ip地址，或者设置arp offload关闭.CNend
*
* @attention  NULL
* @param  ifname          [IN]     Type  #const char *, device name.
* @param  en              [IN]     Type  #unsigned char, arp offload type, 1-on, 0-off.
* @param  ip              [IN]     Type  #unsigned int, ip address in network byte order, eg:192.168.50.4 -> 0x0432A8C0.
*
* @retval #HISI_OK         Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
unsigned char hi_wifi_arp_offload_setting(const char *ifname, unsigned char en, unsigned int ip);

/**
* @ingroup  hi_wifi_basic
* @brief    Set nd offload on/off.CNcomment:设置nd offload 打开/关闭.CNend
*
* @par Description:
*           Set nd offload on with ipv6 address, or set nd offload off.
*           CNcomment:设置nd offload打开、设置正确的解析后的ipv6地址，或设置nd offload关闭.CNend
*
* @attention  NULL
* @param  ifname          [IN]     Type  #const char *, device name.
* @param  en              [IN]     Type  #unsigned char, nd offload type, 1-on, 0-off.
* @param  ip              [IN]     Type  #unsigned char *, ipv6 address after parsing.
*                          eg:FE80::F011:31FF:FEE8:DB6E -> 0xfe80000000f01131fffee8db6e
*
* @retval #HISI_OK         Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
unsigned char hi_wifi_nd_offload_setting(const char *ifname, unsigned char en, unsigned char *ip6);

/**
* @ingroup  hi_wifi_basic
* @brief  Set tx power.CNcomment:设置发送功率上限.CNend
*
* @par Description:
*           Set maximum tx power.CNcomment:设置指定接口的发送功率上限.CNend
*
* @attention  1/only softAP can set maximum tx power.CNcomment:只有AP可以设置最大发送功率.CNend
*             2/should start softAP before set tx power.CNcomment:只有在AP start之后才可以设置.CNend
* @param  ifname           [IN]     Type  #const char * interface name.
* @param  power            [IN]     Type  #int maximum tx power value, range (0-19]dBm.
*
* @retval #HI_ERR_SUCCESS  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_set_txpower_up_limit(const char *ifname, int power);

/**
* @ingroup  hi_wifi_basic
* @brief  Get tx power.CNcomment:获取发送功率上限.CNend
*
* @par Description:
*           Get maximum tx power setting.CNcomment:获取接口的最大发送功率限制值.CNend
*
* @attention  NULL
* @param  ifname           [IN]     Type  #const char * interface name.
*
* @retval #tx power value.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_get_txpower_up_limit(const char *ifname);

/**
* @ingroup  hi_wifi_basic
* @brief  Set retry params.CNcomment:设置软件重传策略.CNend
*
* @par Description:
*           Set retry params.CNcomment:设置指定接口的软件重传策略.CNend
*
* @attention  1.CNcomment:本API需要在STA或AP start之后调用.CNend
* @param  ifname    [IN]     Type  #const char * interface name.CNcomment:接口名.CNend
* @param  type      [IN]     Type  #unsigned char retry type.
*                            CNcomment:0:次数重传（数据帧）; 1:次数重传（管理帧）; 2:时间重传.CNend
* @param  limit     [IN]     Type  #unsigned char limit value.
*                            CNcomment:重传次数(0~15次)/重传时间(0~200个时间粒度,时间粒度10ms).CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
unsigned int hi_wifi_set_retry_params(const char *ifname, unsigned char type, unsigned char limit);

/**
* @ingroup  hi_wifi_basic
* @brief  Set cca threshold.CNcomment:设置CCA门限.CNend
*
* @par Description:
*           Set cca threshold.CNcomment:设置CCA门限.CNend
*
* @attention  1.CNcomment:threshold设置范围是-128~126时，阈值固定为设置值.CNend\n
*             2.CNcomment:threshold设置值为127时，恢复默认阈值-62dBm，并使能动态调整.CNend
* @param  ifname          [IN]     Type #char *, device name. CNcomment:接口名.CNend
* @param  threshold       [IN]     Type #char, threshold. CNcomment:门限值.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
*
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
unsigned int hi_wifi_set_cca_threshold(const char* ifname, signed char threshold);

/**
* @ingroup  hi_wifi_basic
* @brief  Set tx power offset.CNcomment:设置发送功率偏移.CNend
*
* @par Description:
*           Set tx power offset.CNcomment:设置发送功率偏移.CNend
*
* @attention  1.CNcomment:offset设置范围是-150~30，单位0.1dB.参数超出范围按最接近的边界值设置CNend\n
*             2.CNcomment:offset设置,可能会影响信道功率平坦度和evm.CNend
* @param  ifname          [IN]     Type #char *, device name. CNcomment:接口名.CNend
* @param  offset          [IN]     Type #signed short, offset. CNcomment:门限值.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
*
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
unsigned int hi_wifi_set_tx_pwr_offset(const char* ifname, signed short offset);

/**
* @ingroup  hi_wifi_basic
* @brief  Send a custom frame.CNcomment:发送用户定制报文.CNend
*
* @par Description:
*           Send a custom frame.CNcomment:发送用户定制报文.CNend
*
* @attention  1.CNcomment:最大支持发送1400字节的报文.CNend\n
*             2.CNcomment:报文须按照802.11协议格式封装.CNend\n
*             3.CNcomment:采用管理帧速率发送,发送长包效率较低.CNend\n
*             4.CNcomment:返回值仅表示数据是否成功进入发送队列,不表示空口发送状态.CNend\n
* @param  ifname        [IN]     Type #char *, device name. CNcomment:接口名.CNend
* @param  data          [IN]     Type #unsigned char *, frame. CNcomment:帧内容.CNend
* @param  len           [IN]     Type #unsigned int *, frame length. CNcomment:帧长度.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
*
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_send_custom_pkt(const char* ifname, const unsigned char *data, unsigned int len);

/**
* @ingroup  hi_wifi_basic
* @brief  Set tcp mode.CNcomment:设置tpc开关.CNend
*
* @par Description:
*           Set tpc mode.CNcomment:设置tpc开关.CNend
*
* @attention  1.CNcomment:mode范围是0~1,0:关闭发送功率自动控制,1:打开发送功率自动控制.CNend
* @param  ifname          [IN]     Type #char *, device name. CNcomment:接口名.CNend
* @param  ifname_len      [IN]     Type #unsigned char, interface name length.CNcomment:接口名长度.CNend
* @param  tpc_value       [IN]     Type #unsigned int, tpc_value. CNcomment:tpc开关.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
*
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
unsigned int hi_wifi_set_tpc(const char* ifname, unsigned char ifname_len, unsigned int tpc_value);

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end of hi_wifi_api.h */
