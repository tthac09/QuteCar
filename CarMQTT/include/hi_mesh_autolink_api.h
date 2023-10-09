/**
* @file hi_mesh_autolink_api.h
*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved. \n
* Description: header file for mesh autolink api.CNcomment:描述：Mesh自动组网api接口头文件。CNend\n
* Author: Hisilicon \n
* Create: 2020-04-01
*/

/**
 * @defgroup hi_mesh_autolink WiFi Mesh Autolink Settings
 * @ingroup hi_wifi
 */

#ifndef __HI_MESH_AUTOLINK_API_H__
#define __HI_MESH_AUTOLINK_API_H__

#include "hi_wifi_mesh_api.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**
 * @ingroup hi_mesh_autolink
 *
 * Auth type of mesh autolink.CNcomment: Mesh自动组网的认证类型.CNend
 */
typedef enum {
    HI_MESH_OPEN,                       /**< OPEN类型 */
    HI_MESH_AUTH,                       /**< 加密类型 */

    HI_MESH_AUTH_TYPE_BUTT
}hi_mesh_autolink_auth_type;


/**
 * @ingroup hi_mesh_autolink
 *
 * Node type that usr config of mesh autolink.CNcomment: Mesh自动组网的用户输入节点类型.CNend
 */
typedef enum {
    HI_MESH_USRCONFIG_MBR,                  /**< 用户指定MBR节点角色 */
    HI_MESH_USRCONFIG_MR,                   /**< 用户指定MR节点角色 */
    HI_MESH_USRCONFIG_MSTA,                 /**< 用户指定MSTA节点角色 */
    HI_MESH_USRCONFIG_AUTO,                 /**< 自动竞选节点角色(MBR/MR中选择) */

    HI_MESH_AUTOLINK_ROUTER_MSTA,           /**< 未加入组网后的返回结果(非用户配置角色) */

    HI_MESH_USRCONFIG_BUTT
}hi_mesh_autolink_usrcfg_node_type;


/**
 * @ingroup hi_mesh_autolink
 *
 * Node type information.CNcomment: Mesh自动组网的节点信息体.CNend
 */
typedef struct {
    char ifname_first[WIFI_IFNAME_MAX_SIZE + 1];
    int len_first;
    char ifname_second[WIFI_IFNAME_MAX_SIZE + 1];
    int len_second;
}hi_mesh_autolink_role_cb_info;

/**
 * @ingroup hi_mesh_autolink
 *
 * Struct of mesh autolink config.CNcomment:mesh自动组网配置参数CNend
 *
 */
typedef struct {
    char ssid[HI_WIFI_MAX_SSID_LEN + 1];                   /**< SSID */
    hi_mesh_autolink_auth_type auth;                       /**< Autolink认证类型 */
    char key[HI_WIFI_MESH_KEY_LEN_MAX + 1];                /**< 密码 */
    hi_mesh_autolink_usrcfg_node_type usr_cfg_role;        /**< 用户设置的节点类型 */
}hi_mesh_autolink_config;

/**
 * @ingroup hi_mesh_autolink
 *
 * Struct of autolink role callback.CNcomment:Mesh自动组网角色回调结构体.CNend
 *
 */
typedef struct {
    hi_mesh_autolink_usrcfg_node_type role;   /**< 节点角色 */
    hi_mesh_autolink_role_cb_info info;       /**< 事件信息 */
} hi_mesh_autolink_role_cb;

/**
 * @ingroup hi_mesh_autolink
 *
 * callback function definition of mesh autolink result.CNcommment:mesh自动组网结果回调接口定义.CNend
 */
typedef void (* hi_mesh_autolink_cb)(const hi_mesh_autolink_role_cb *role);

/**
* @ingroup  hi_mesh_autolink
* @brief  Mesh start autolink network.CNcomment:mesh启动自动组网。CNend
*
* @par Description:
*          Mesh start autolink network.CNcomment:mesh启动自动组网。CNend
*
* @attention  1. SSID only supports ASCII characters.
*                CNcomment:1. SSID 只支持ASCII字符.CNend
* @param  config [IN]  Type  #hi_mesh_autolink_config * mesh's autolink configuration.CNcomment:mesh自动入网配置。CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_mesh_start_autolink(hi_mesh_autolink_config *config);

/**
* @ingroup  hi_mesh_autolink
* @brief  Close mesh autolink.CNcomment:停止mesh自动组网模块。CNend
*
* @par Description:
*           Close mesh autolink.CNcomment:停止mesh自动组网模块。CNend
*
* @attention  NULL
* @param  NULL
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency:
*            @li hi_wifi_mesh_api.h: WiFi-MESH API
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_mesh_exit_autolink(void);

/**
* @ingroup  hi_mesh_autolink
* @brief  Set the rssi threshold of the router when MBR node connect.
*           CNcomment:设置MBR节点关联路由器的RSSI阈值。CNend
*
* @par Description:
*           Set the router rssi threshold.CNcomment:设置路由器的RSSI阈值。CNend
*
* @attention  1. The valid range of RSSI threshold is -127 ~ 127.
*                CNcomment:1. RSSI阈值的有效范围是-127         ~ 127.CNend
* @param  router_rssi [IN]  Type  #int * mesh's rssi threshold of mbr connecting to router.
*           CNcomment:MBR关联到路由器的RSSI阈值CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency: NULL
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_mesh_autolink_set_router_rssi_threshold(int router_rssi);

/**
* @ingroup  hi_mesh_autolink
* @brief  Get the rssi threshold of the router when MBR node connect.
*           CNcomment:获取MBR节点关联路由器的RSSI阈值。CNend
*
* @par Description:
*           Get the router rssi threshold.CNcomment:获取路由器的RSSI阈值。CNend
*
* @attention 1. The memories of <router_rssi> memories are requested by the caller.
*               CNcomment:1. <router_rssi>由调用者申请内存CNend
* @param  router_rssi [OUT]  Type  #int * mesh's rssi threshold of mbr connecting to router.
*           CNcomment:MBR关联到路由器的RSSI阈值CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency: NULL
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_mesh_autolink_get_router_rssi_threshold(int *router_rssi);

/**
* @ingroup  hi_mesh_autolink
* @brief  Set the bandwidth value of the mesh network. CNcomment:设置Mesh网络的带宽。CNend
*
* @par Description:
*           Set the bandwidth value of the mesh network. CNcomment:设置Mesh网络的带宽。CNend
*
* @attention 1. Should be called before calling hi_mesh_start_autolink api to establish mesh network.
*            CNcomment:1. 需要在调用hi_mesh_start_autolink接口组建网络前调用CNend
* @param  bw [IN]  Type  #hi_wifi_bw * The bandwidth value of mesh network.
*           CNcomment:Mesh网络的带宽值CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency: NULL
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_mesh_autolink_set_bw(hi_wifi_bw bw);

/**
* @ingroup  hi_mesh_autolink
* @brief  Get the bandwidth value of the mesh network. CNcomment:获取Mesh网络的带宽。CNend
*
* @par Description:
*           Get the bandwidth value of the mesh network. CNcomment:获取Mesh网络的带宽。CNend
*
* @attention 1. The memories of <bw> memories are requested by the caller.
*               CNcomment:1. <bw>由调用者申请内存CNend
* @param  bw [OUT]  Type  #hi_wifi_bw * The bandwidth value of mesh network.
*           CNcomment:Mesh网络的带宽值CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency: NULL
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_mesh_autolink_get_bw(hi_wifi_bw *bw);

/**
* @ingroup  hi_mesh_autolink
* @brief  register mesh autolink user callback interface.CNcomment:注册Mesh自动组网回调函数接口.CNend
*
* @par Description:
*           register mesh autolink user callback interface.CNcomment:注册Mesh自动组网回调函数接口.CNend
*
* @attention  NULL
* @param  role_cb  [OUT]    Type #hi_mesh_autolink_cb *, role callback .CNcomment:回调函数.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency: NULL
* @see  NULL
* @since Hi3861_V100R001C00
*/
int hi_mesh_autolink_register_event_callback(hi_mesh_autolink_cb role_cb);

/**
* @ingroup  hi_mesh_autolink
* @brief  set device oui interface.CNcomment:设置产品的厂家oui编码.CNend
*
* @par Description:
*           set device ou interface.CNcomment:设置产品的厂家oui编码.CNend
*
* @attention  NULL
* @param  oui  [IN]    Type #hi_u8 *, oui code .CNcomment:产品厂家编码，为3个字节无符号数，范围0-0XFF.CNend
* @param  oui_len  [IN]    Type #hi_u8 *, oui code lenth .CNcomment:厂家编码长度,长度为3个字节.CNend
*
* @retval #HISI_OK        Execute successfully.
* @retval #HISI_FAIL      Execute failed.
* @par Dependency: NULL
* @see  NULL
* @since Hi3861_V100R001C00
*/
hi_s8 hi_mesh_set_oui(const hi_u8 *oui, hi_u8 oui_len);


#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end of hi_mesh_autolink_api.h */
