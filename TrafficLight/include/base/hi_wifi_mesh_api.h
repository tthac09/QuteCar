/**
 * @file hi_wifi_mesh_api.h
 *
 * Copyright (c) 2020 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @defgroup hi_wifi_mesh WiFi Mesh Settings
 * @ingroup hi_wifi
 */

#ifndef __HI_WIFI_MESH_API_H__
#define __HI_WIFI_MESH_API_H__

#include "hi_wifi_api.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**
 * @ingroup hi_wifi_mesh
 *
 * max auth type length.CNcomment:用户输入的认证方式最大长度CNend
 */
#define WPA_MAX_AUTH_TYPE_INPUT_LEN     32

/**
 * @ingroup hi_wifi_mesh
 *
 * max mesh key length.CNcomment:用户输入的mesh密钥最大长度CNend
 */
#define HI_WIFI_MESH_KEY_LEN_MAX 63

/**
 * @ingroup hi_wifi_basic
 * Struct of scan result.CNcomment:扫描结果结构体CNend
 */
typedef struct {
    char ssid[HI_WIFI_MAX_SSID_LEN + 1];    /**< SSID 只支持ASCII字符 */
    unsigned char bssid[HI_WIFI_MAC_LEN];   /**< BSSID */
    unsigned int channel;                   /**< 信道号 */
    hi_wifi_auth_mode auth;                 /**< 认证类型 */
    int rssi;                               /**< 信号强度 */
    unsigned char resv : 4;                 /**< Reserved */
    unsigned char hisi_mesh_flag : 1;       /**< HI MESH标志 */
    unsigned char is_mbr : 1;               /**< 是否是MBR标志 */
    unsigned char accept_for_sta : 1;       /**< 是否允许STA接入 */
    unsigned char accept_for_peer : 1;      /**< 是否允许Mesh AP接入 */
    unsigned char bcn_prio;                 /**< BCN优先级 */
    unsigned char peering_num;              /**< 对端连接的数目 */
    unsigned short mesh_rank;               /**< mesh rank值 */
    unsigned char switch_status;            /**< 标记mesh ap是否处于信道切换状态 */
} hi_wifi_mesh_scan_result_info;

/**
 * @ingroup hi_wifi_mesh
 *
 * Struct of connected mesh.CNcomment:已连接的peer结构体。CNend
 *
 */
typedef struct {
    unsigned char mac[HI_WIFI_MAC_LEN];       /**< 对端mac地址 */
    unsigned char mesh_bcn_priority;          /**< BCN优先级 */
    unsigned char mesh_is_mbr : 1;            /**< 是否是MBR */
    unsigned char mesh_block : 1;             /**< block是否置位 */
    unsigned char mesh_role : 1;              /**< mesh的角色 */
} hi_wifi_mesh_peer_info;

/**
 * @ingroup hi_wifi_mesh
 *
 * Struct of mesh's config.CNcomment:mesh配置参数CNend
 *
 */
typedef struct {
    char ssid[HI_WIFI_MAX_SSID_LEN + 1];     /**< SSID 只支持ASCII字符 */
    char key[HI_WIFI_AP_KEY_LEN + 1];        /**< 密码 */
    hi_wifi_auth_mode auth;                  /**< 认证类型，只支持HI_WIFI_SECURITY_OPEN和HI_WIFI_SECURITY_SAE */
    unsigned char channel;                   /**< 信道号 */
} hi_wifi_mesh_config;

/**
 * @ingroup hi_wifi_mesh
 *
 * Status type.CNcomment:mesh节点状态类型CNend
 *
 */
typedef enum {
    MAC_HISI_MESH_UNSPEC = 0, /* 未确定mesh节点角色 */
    MAC_HISI_MESH_STA,        /* Mesh-STA节点角色 */
    MAC_HISI_MESH_MG,         /* Mesh-MG节点角色 */
    MAC_HISI_MESH_MBR,        /* Mesh-MBR节点角色 */

    MAC_HISI_MESH_NODE_BUTT,
} mac_hisi_mesh_node_type_enum;
typedef unsigned char mac_hisi_mesh_node_type_enum_uint8;

/**
 * @ingroup hi_wifi_mesh
 *
 * Struct of node's config.CNcomment:node配置参数CNend
 *
 */
typedef struct _mac_cfg_mesh_nodeinfo_stru {
    mac_hisi_mesh_node_type_enum_uint8 node_type;           /* 本节点角色 */
    unsigned char mesh_accept_sta;                          /* 是否接受sta关联 */
    unsigned char user_num;                                 /* 关联用户数 */
    unsigned char privacy;                                  /* 是否加密 */
    unsigned char chan;                                     /* 信道号 */
    unsigned char priority;                                 /* bcn优先级 */
    unsigned char rsv[2];                                   /* 2 byte保留 */
}mac_cfg_mesh_nodeinfo_stru;

/**
* @ingroup  hi_wifi_mesh
* @brief  Mesh disconnect peer by mac address.CNcomment:mesh指定断开连接的网络。CNend
*
* @par Description:
*          Mesh disconnect peer by mac address.CNcomment:softap指定断开连接的网络。CNend
*
* @attention  NULL
* @param  addr             [IN]     Type  #const unsigned char *, peer mac address.CNcomment:对端MAC地址。CNend
* @param  addr_len         [IN]     Type  #unsigned char, peer mac address length.CNcomment:对端MAC地址长度。CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_disconnect(const unsigned char *addr, unsigned char addr_len);

/**
* @ingroup  hi_wifi_mesh
* @brief  Start mesh interface.CNcomment:开启mesh。CNend
*
* @par Description:
*           Add mesh interface.CNcomment:开启mesh。CNend
*
* @attention  1. The memories of <ifname> and <len> should be requested by the caller，
*                the input value of len must be the same as the length of ifname（the recommended length is 17Bytes）.\n
*                CNcomment:1. <ifname>和<len>由调用者申请内存，用户写入len的值必须与ifname长度一致（建议长度为17Bytes）.CNend \n
*             2. SSID only supports ASCII characters.
*                CNcomment:2. SSID 只支持ASCII字符.CNend \n
*             3. This is a blocking function.CNcomment:3．此函数为阻塞函数.CNend
* @param config    [IN]     Type  #hi_wifi_mesh_config * mesh's configuration.CNcomment:mesh配置。CNend \n
* @param ifname    [IN/OUT] Type  #char * mesh interface name.CNcomment:创建的mesh接口名称。CNend \n
* @param len       [IN/OUT] Type  #int * mesh interface name length.CNcomment:创建的mesh接口名称的长度。CNend \n
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_start(hi_wifi_mesh_config *config, char *ifname, int *len);

/**
* @ingroup  hi_wifi_mesh
* @brief  Connect to mesh device by mac address.CNcomment:通过对端mac地址连接mesh。CNend
*
* @par Description:
*           Connect to mesh device by mac address.CNcomment:通过对端mac地址连接mesh。CNend
*
* @attention  NULL
* @param  mac             [IN]    Type  #const unsigned char * peer mac address.CNcomment:对端mesh节点的mac地址。CNend
* @param  len             [IN]    Type  #const int   the len of mac address.CNcomment:mac地址的长度。CNend
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_connect(const unsigned char *mac, const int len);

/**
* @ingroup  hi_wifi_mesh
* @brief  Set mesh support/not support mesh peer connections.CNcomment:设置mesh支持/不支持mesh peer连接。CNend
*
* @par Description:
*           Set mesh support/not support mesh peer connections.CNcomment:设置mesh支持/不支持mesh peer连接。CNend
*
* @attention  1. Default support peer connect.CNcomment:1. 默认支持mesh peer连接。CNend \n
*             2. The enable_peer_connect value can only be 1 or 0. CNcomment:2. enable_peer_connect值只能为1或0。CNend
* @param  enable_accept_peer    [IN]    Type  #unsigned char flag to support mesh connection.
*                                             CNcomment:是否支持mesh连接的标志。CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_set_accept_peer(unsigned char enable_peer_connect);

/**
* @ingroup  hi_wifi_mesh
* @brief  Set mesh support/not support mesh sta connections.CNcomment:设置mesh支持/不支持mesh sta连接。CNend
*
* @par Description:
*           Set mesh support/not support mesh sta connections.CNcomment:设置mesh支持/不支持mesh sta连接。CNend
*
* @attention 1. Default not support sta connect. CNcomment:1. 默认不支持mesh sta连接。CNend \n
*            2. The enable_sta_connect value can only be 1 or 0. CNcomment:2. enable_sta_connect值只能为1或0。CNend \n
             3. This value can only be modified after the node joins the Mesh network.
                CNcomment: 3. 只有节点加入Mesh网络后才可以修改该值。 CNend
* @param  enable_accept_sta    [IN]    Type  #unsigned char flag to support mesh sta connection.
*                                            CNcomment:是否支持sta连接的标志。CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_set_accept_sta(unsigned char enable_sta_connect);

/**
* @ingroup  hi_wifi_mesh
* @brief  Set sta supports mesh capability.CNcomment:设置sta支持mesh能力。CNend
*
* @par Description:
*           Set sta supports mesh capability.CNcomment:sta支持mesh能力。CNend
*
* @attention 1. Default is not mesh sta. CNcomment:1. 默认不是mesh sta。CNend \n
*            2. The enable value can only be 1 or 0.. CNcomment:2. enable值只能为1或0。CNend
* @param  enable          [IN]    Type  #unsigned char flag of sta's ability to support mesh.
*                                       CNcomment:sta支持mesh能力的标志。CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_set_mesh_sta(unsigned char enable);

/**
* @ingroup  hi_wifi_mesh
* @brief  Start mesh sta scan. CNcomment:mesh sta 扫描。CNend
*
* @par Description:
*           Start mesh sta scan. CNcomment:mesh sta 扫描。CNend
*
* @attention  NULL
* @param void.
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_sta_scan(void);

/**
* @ingroup  hi_wifi_mesh
* @brief  Start mesh sta advance scan.CNcomment:mesh sta 高级扫描。CNend
*
* @par Description:
*           Start mesh sta advance scan.
*
* @attention  1. Advance scan can scan with ssid only,channel only,bssid only,prefix_ssid only，
*             and the combination parameters scanning does not support. \n
*             CNcomment:1 .高级扫描分别单独支持 ssid扫描，信道扫描，bssid扫描，ssid前缀扫描, 不支持组合参数扫描方式。CNend \n
*             2. Scanning mode, subject to the type set by scan_type.
*             CNcomment:2 .扫描方式，以scan_type传入的类型为准。CNend
* @param  sp          [IN]    Type #hi_wifi_scan_params * parameters of scan.CNcomment:扫描网络参数设置CNend
*
* @retval #HISI_OK    Execute successfully.
* @retval #HISI_FAIL  Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_sta_advance_scan(hi_wifi_scan_params *sp);

/**
* @ingroup  hi_wifi_mesh
* @brief  Start mesh peer scan. CNcomment:mesh peer 扫描。CNend
*
* @par Description:
*           Start mesh peer scan. CNcomment:mesh peer 扫描。CNend
*
* @attention  NULL
* @param void
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_scan(void);

/**
* @ingroup  hi_wifi_mesh
* @brief  Start mesh peer advance scan.CNcomment:mesh peer 高级扫描。CNend
*
* @par Description:
*           Start mesh peer advance scan.CNcomment:mesh peer 高级扫描。CNend
*
* @attention  1. Advance scan can scan with ssid only,channel only,bssid only,prefix_ssid only，
*             and the combination parameters scanning does not support. \n
*             CNcomment:1 .高级扫描分别单独支持 ssid扫描，信道扫描，bssid扫描，ssid前缀扫描, 不支持组合参数扫描方式。CNend \n
*             2. Scanning mode, subject to the type set by scan_type.
*             CNcomment:2 .扫描方式，以scan_type传入的类型为准。CNend
* @param  sp          [IN]    Type  #hi_wifi_scan_params * mesh's scan parameters.CNcomment:mesh peer支持的扫描方式。CNend
*
* @retval #HISI_OK    Execute successfully.
* @retval #HISI_FAIL  Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_advance_scan(hi_wifi_scan_params *sp);

/**
* @ingroup  hi_wifi_mesh
* @brief  Get the results of mesh peer scan.CNcomment:获取 mesh peer 扫描网络的结果。CNend
*
* @par Description:
*           Get the results of mesh peer scan..CNcomment:获取 mesh peer 扫描网络的结果。CNend
*
* @attention  1.ap_list: malloc by user.CNcomment:1.扫描结果参数。由用户动态申请CNend \n
*             2.ap_list max size: (hi_wifi_mesh_scan_result_info ap_list) * 32. \n
*             CNcomment:2.ap_list 最大为（hi_wifi_mesh_scan_result_info ap_list）* 32。CNend \n
*             3.ap_num:Parameters can be passed in to specify the number of scanned results.The maximum is 32. \n
*             CNcomment:3.可以传入参数，指定获取扫描到的结果数量，最大为32。CNend \n
*             4.If the callback function of the reporting user is used,
*             ap_num refers to bss_num in event_wifi_scan_done. \n
*             CNcomment:4.如果使用上报用户的回调函数，ap_num参考event_wifi_scan_done中的bss_num。CNend \n
*             5.ap_num should be same with number of hi_wifi_mesh_scan_result_info structures applied,
*             Otherwise, it will cause memory overflow. \n
*             CNcomment:5.ap_num和申请的hi_wifi_mesh_scan_result_info结构体数量一致，否则可能造成内存溢出。CNend \n
*             6. SSID only supports ASCII characters.
*             CNcomment:6. SSID 只支持ASCII字符.CNend \n
*             7. The rssi in the scan results needs to be divided by 100 to get the actual rssi. \n
*             CNcomment:7. 扫描结果中的rssi需要除以100才能获得实际的rssi.CNend
* @param  ap_list         [IN/OUT]    Type #hi_wifi_mesh_scan_result_info * ap_list.CNcomment:扫描到的结果。CNend
* @param  ap_num          [IN/OUT]    Type #unsigned int * number of scan result.CNcomment:扫描到的网络数目。CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_scan_results(hi_wifi_mesh_scan_result_info *ap_list, unsigned int *ap_num);

/**
* @ingroup  hi_wifi_mesh
* @brief  Get the results of mesh sta scan.CNcomment:获取 mesh sta 扫描网络的结果。CNend
*
* @par Description:
*           Get the results of mesh sta scan..CNcomment:获取 mesh sta 扫描网络的结果。CNend
*
* @attention  1.ap_list: malloc by user.CNcomment:1.扫描结果参数。由用户动态申请CNend \n
*             2.max size: (hi_wifi_mesh_scan_result_info ap_list) * 32. \n
*             CNcomment:2.足够的结构体大小，最大为（hi_wifi_mesh_scan_result_info ap_list）* 32。CNend \n
*             3.ap_num:Parameters can be passed in to specify the number of scanned results.The maximum is 32.
*             CNcomment:3.可以传入参数，指定获取扫描到的结果数量，最大为32。CNend \n
*             4.If the callback function of the reporting user is used,
*             ap_num refers to bss_num in event_wifi_scan_done. \n
*             CNcomment:4.如果使用上报用户的回调函数，ap_num参考event_wifi_scan_done中的bss_num。CNend \n
*             5.ap_num should be same with number of hi_wifi_mesh_scan_result_info structures applied,Otherwise,
*             it will cause memory overflow. \n
*             CNcomment:5.ap_num和申请的hi_wifi_mesh_scan_result_info结构体数量一致，否则可能造成内存溢出。CNend \n
*             6. SSID only supports ASCII characters.
*             CNcomment:6. SSID 只支持ASCII字符.CNend \n
*             7. The rssi in the scan results needs to be divided by 100 to get the actual rssi. \n
*             CNcomment:7. 扫描结果中的rssi需要除以100才能获得实际的rssi.CNend
* @param  ap_list         [IN/OUT]    Type #hi_wifi_mesh_scan_result_info * ap_list.CNcomment:扫描到的结果。CNend
* @param  ap_num          [IN/OUT]    Type #unsigned int * number of scan result.CNcomment:扫描到的网络数目。CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_sta_scan_results(hi_wifi_mesh_scan_result_info *ap_list, unsigned int *ap_num);

/**
* @ingroup  hi_wifi_mesh
* @brief  Close mesh interface.CNcomment:停止mesh接口。CNend
*
* @par Description:
*           Close mesh interface.CNcomment:停止mesh接口。CNend
*
* @attention  1. This is a blocking function.CNcomment:1．此函数为阻塞函数.CNend
* @param  NULL
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_stop(void);

/**
* @ingroup  hi_wifi_mesh
* @brief  Get all user's information of mesh.CNcomment:mesh获取已连接的peer的信息。CNend
*
* @par Description:
*           Get all user's information of mesh.CNcomment:mesh获取已连接的peer的信息。CNend
*
* @attention  1.peer_list: malloc by user.CNcomment:1.扫描结果参数。由用户动态申请CNend \n
*             2.peer_list max size: (hi_wifi_mesh_peer_info peer_list) * 6. \n
*             CNcomment:2.peer_list 足够的结构体大小，最大为hi_wifi_mesh_peer_info* 6。CNend \n
*             3.peer_num:Parameters can be passed in to specify the number of connected peers.The maximum is 6. \n
*             CNcomment:3.可以传入参数，指定获取已接入的peer个数，最大为6。CNend \n
*             4.peer_num should be the same with number of hi_wifi_mesh_peer_info structures applied, Otherwise,
*             it will cause memory overflow. \n
*             CNcomment:4.peer_num和申请的hi_wifi_mesh_peer_info结构体数量一致，否则可能造成内存溢出。CNend
* @param  peer_list        [IN/OUT]    Type  #hi_wifi_mesh_peer_info *, peer information.CNcomment:连接的peer信息。CNend
* @param  peer_num         [IN/OUT]    Type  #unsigned int *, peer number.CNcomment:peer的个数。CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_mesh_get_connected_peer(hi_wifi_mesh_peer_info *peer_list, unsigned int *peer_num);

/**
* @ingroup  hi_wifi_mesh
* @brief  Set switch channel enable or disable.CNcomment:设置MESH AP信道使能或者无效.CNend
*
* @par Description:
*         Set switch channel and disable switch channel.CNcomment:设置MESH AP信道使能或者无效.CNend
*
* @attention  default switch channel is enable，set HI_FALSE is disable and HI_TRUE is enable
* @param  ifname           [IN]     Type  #const char *, interface name.CNcomment:接口名.CNend
* @param  ifname_len       [IN]     Type  #unsigned char, interface name length.CNcomment:接口名长度.CNend
* @param  enable           [IN]     Type  #unsigned char, enable or disable.CNcomment:使能或者无效.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other    Error code
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_protocol_chn_switch_enable(const char *ifname, unsigned char ifname_len, hi_u8 enable);

/**
* @ingroup  hi_wifi_mesh
* @brief  Set switch channel.CNcomment:设置MESH AP信道切换.CNend
*
* @par Description:
*         Set switch channel.CNcomment:设置MESH AP信道切换.CNend
*
* @attention  NULL
* @param  ifname        [IN]   Type  #const char *, interface name.CNcomment:接口名.CNend
* @param  ifname_len    [IN]   Type  #unsigned char, interface name length.CNcomment:接口名长度.CNend
* @param  channel       [IN]   Type  #unsigned char, switch channel.CNcomment:信道号.CNend
* @param  switch_count  [IN]   Type  #unsigned char, switch channel offset beacons.CNcomment:切换信道偏移beacon的个数.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_wifi_protocol_chn_switch(const char *ifname, unsigned char ifname_len,
    unsigned char channel, unsigned char switch_count);

/**
* @ingroup  hi_wifi_mesh
* @brief  Set switch channel.CNcomment:查询MESH 节点信息.CNend
*
* @par Description:
*         Set switch channel.CNcomment:查询MESH 节点信息.CNend
*
* @attention  NULL
* @param  mesh_node_info     [OUT]     Type  #mac_cfg_mesh_nodeinfo_stru *, node info.CNcomment:节点信息.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
unsigned int hi_wifi_get_mesh_node_info(mac_cfg_mesh_nodeinfo_stru *mesh_node_info);

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end of hi_wifi_mesh_api.h */
