/**
* @file hi_wifi_api.h
*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved. \n
* Description: header file for wifi quick send api.CNcomment:描述：WiFi 快启发包api接口头文件.CNend\n
* Author: Hisilicon \n
* Create: 2019-01-03
*/

/**
 * @defgroup hi_wifi_basic WiFi Basic Settings
 * @ingroup hi_wifi
 */

#ifndef __HI_WIFI_QUICK_SEND_API_H__
#define __HI_WIFI_QUICK_SEND_API_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/**
 * @ingroup hi_wifi_quick_send_basic
 *
 * Sequence number field offset in 802.11 frame.CNcomment:序列号字段在802.11帧中的偏移.CNend
 */
#define QUICK_SEND_SEQ_NUM_OFFSET 22

/**
 * @ingroup hi_wifi_quick_send_basic
 *
 * Source mac address field offset in 802.11 frame.CNcomment:源mac地址字段在802.11帧中的偏移.CNend
 */
#define QUICK_SEND_SRC_MAC_OFFSET 10

/**
 * @ingroup hi_wifi_quick_send_basic
 *
 * destination mac address field offset in 802.11 frame.CNcomment:目标mac地址字段在802.11帧中的偏移.CNend
 */
#define QUICK_SEND_DST_MAC_OFFSET 16

/**
 * @ingroup hi_wifi_quick_send_basic
 *
 * Length of mac address field.CNcomment:mac地址字段长度.CNend
 */
#define QUICK_SEND_MAC_LEN 6

/**
 * @ingroup hi_wifi_quick_send_basic
 *
 * Offset of bssid field.CNcomment:bssid字段偏移.CNend
 */
#define QUICK_SEND_BSSID_OFFSET 4

/**
 * @ingroup hi_wifi_quick_send_basic
 *
 * Length of bssid field.CNcomment:bssid字段长度.CNend
 */
#define QUICK_SEND_BSSID_LEN 6

/**
 * @ingroup hi_wifi_quick_send_basic
 *
 * Send frame success.CNcomment:发送成功.CNend
 */
#define QUICK_SEND_SUCCESS   1

/**
 * @ingroup hi_wifi_quick_send_basic
 *
 * Invalid send result.CNcomment:无效的发送结果.CNend
 */
#define QUICK_SEND_RESULT_INVALID   255

/**
 * @ingroup hi_wifi_quick_send_basic
 *
 * Max ssid length.CNcomment:ssid最大长度.CNend
 */
#define QUICK_SEND_SSID_MAX_LEN  (32 + 1)

/**
 * @ingroup hi_wifi_quick_send_basic
 *
 * Mac address length.CNcomment:mac地址长度.CNend
 */
#define QUICK_SEND_MAC_ADDR_LEN  6

/**
 * @ingroup hi_wifi_quick_send_basic
 *
 * Mac address length.CNcomment:mac地址长度.CNend
 */
typedef enum {
    WIFI_PHY_MODE_11N = 0,
    WIFI_PHY_MODE_11G = 1,
    WIFI_PHY_MODE_11B = 2,
    WIFI_PHY_MODE_BUTT
}hi_wifi_phy_mode;

/**
 * @ingroup hi_wifi_basic
 *
 * Struct of frame filter config in monitor mode.CNcomment:报文接收过滤设置.CNend
 */
typedef struct {
    char mdata_en : 1;  /**< get multi-cast data frame flag. CNcomment: 使能接收组播(广播)数据包.CNend */
    char udata_en : 1;  /**< get single-cast data frame flag. CNcomment: 使能接收单播数据包.CNend */
    char mmngt_en : 1;  /**< get multi-cast mgmt frame flag. CNcomment: 使能接收组播(广播)管理包.CNend */
    char umngt_en : 1;  /**< get single-cast mgmt frame flag. CNcomment: 使能接收单播管理包.CNend */
    char resvd    : 4;  /**< reserved bits. CNcomment: 保留字段.CNend */
} hi_wifi_rx_filter;

/**
* @ingroup  hi_wifi_basic
* @brief  Config tx parameter.CNcomment:配置发送参数.CNend
*
* @par Description:
*           Config tx parameter.CNcomment:配置发送参数.CNend
*
* @attention  1.CNcomment:发帧前需要配置一次,初始化信道/速率等参数.CNend\n
* @param  ch　　        [IN]     Type #char *, channel. CNcomment:信道字符串,"0"~"14",如"6".CNend
* @param  rate_code     [IN]     Type #char *, rate code. CNcomment:速率码字符串.
*                                phy_mode为0时,值范围:"0"~"7",分别对应mcs0~mcs7;
*                                phy_mode为1时,值范围:"48","24","12","6","54","36","18","9"Mbps)
*                                phy_mode为2时,值范围:"1","2","5.5","11"Mbps).CNend
* @param  phy_mode      [IN]     Type #hi_wifi_phy_mode, phy mode. CNcomment:协议模式,如11b/g/n.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
*
* @par Dependency:
*            @li hi_wifi_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
unsigned int hi_wifi_cfg_tx_para(char *ch, char *rate_code, hi_wifi_phy_mode phy_mode);

/**
* @ingroup  hi_wifi_quick_send_basic
* @brief  Get frame send result.CNcomment:获取发送结果.CNend
*
* @par Description:
*           Get frame send result.CNcomment:获取发送结果.CNend
*
* @attention  NULL
* @param      NULL
*
* @retval  #QUICK_SEND_SUCCESS   Send success.
* @retval #Other  Error code
*
* @par Dependency:
*            @li hi_wifi_quick_send_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
unsigned char hi_wifi_get_quick_send_result(void);

/**
* @ingroup  hi_wifi_quick_send_basic
* @brief  Reset frame send result.CNcomment:重置发送报文结果为无效.CNend
*
* @par Description:
*           Reset frame send result.CNcomment:重置发送报文结果为无效.CNend
*
* @attention  1.CNcomment:Called before send.CNend\n
* @param      NULL
*
* @retval NULL
*
* @par Dependency:
*            @li hi_wifi_quick_send_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
void hi_wifi_reset_quick_send_result(void);

/**
* @ingroup  hi_wifi_quick_send_basic
* @brief  Send a custom 802.11 frame.CNcomment:发送用户定制802.11报文.CNend
*
* @par Description:
*           Send a custom 802.11 frame.CNcomment:发送用户定制802.11报文.CNend
*
* @attention  1.CNcomment:最大支持发送1400字节的报文.CNend\n
*             2.CNcomment:报文须按照802.11协议格式封装.CNend\n
*             3.CNcomment:返回值仅表示数据是否成功进入发送队列,不表示空口发送状态.CNend\n
*             3.CNcomment:发送结果用hi_wifi_get_quick_send_result()接口查询.CNend\n
* @param  payload       [IN]     Type #unsigned char *, payload. CNcomment:帧内容.CNend
* @param  len           [IN]     Type #unsigned short, frame length. CNcomment:帧长度.CNend
*
* @retval #HISI_OK  Excute successfully
* @retval #Other           Error code
*
* @par Dependency:
*            @li hi_wifi_quick_send_api.h: WiFi API
* @see  NULL
* @since Hi3861_V100R001C00
*/
unsigned int hi_wifi_tx_proc_fast(unsigned char *payload, unsigned short len);

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end of hi_wifi_quick_send_api.h */

