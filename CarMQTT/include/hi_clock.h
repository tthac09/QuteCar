/**
* @file hi_clock.h
*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.  \n
* Description: Clock configuration.   \n
* Author: Hisilicon   \n
* Create: 2019-01-17
*/

/**
 * @defgroup iot_clock Crystal Clock
 * @ingroup drivers
 */

#ifndef __HI_CLOCK_H__
#define __HI_CLOCK_H__
#include <hi_types_base.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HI_XTAL_24MHZ_VAL   24000000
#define HI_XTAL_40MHZ_VAL   40000000

/**
 * @ingroup iot_clock Xtal Clock Configuration
 *
 * Crystal clock frequency.CNcomment:晶体时钟频率。CNend
 */
typedef enum {
    HI_XTAL_CLOCK_24M,   /**< 24M crystal clock frequency.CNcomment:24M晶体时钟CNend */
    HI_XTAL_CLOCK_40M,   /**< 40M crystal clock frequency.CNcomment:40M晶体时钟CNend */
    HI_XTAL_CLOCK_MAX    /**< MAX value, invalid.CNcomment:最大值，不可使用CNend */
}hi_xtal_clock;

/**
* @ingroup  iot_clock
* @brief Obtains the crystal clock frequency.CNcomment:获取晶体时钟频率。CNend
*
* @par 描述:
*           Obtains the crystal clock frequency.CNcomment:获取晶体时钟频率。CNend
*
* @attention None
* @param     None
*
* @retval #HI_XTAL_CLOCK_24M  24M crystal clock frequency.CNcomment:24M晶体时钟。CNend
* @retval #HI_XTAL_CLOCK_40M  40M crystal clock frequency.CNcomment:40M晶体时钟。CNend
* @retval #HI_XTAL_CLOCK_MAX  MAX value, invalid.CNcomment:最大值，不可使用。CNend
* @par 依赖:
*            @li hi_clock.h：Describes hardware clock APIS.
CNcomment:文件用于描述硬件时钟相关接口。CNend
* @see None
* @since Hi3861_V100R001C00
*/
HI_EXTERN hi_xtal_clock hi_get_xtal_clock(hi_void);

/**
* @ingroup  iot_clock
* @brief Configure the frequency divider of the 24M/40M crystal oscillator to generate the 32K clock.
CNcomment:配置24M/40M晶体分频值，产生32K时钟。CNend
*
* @par 描述:
*        Configure the frequency divider of the 24M/40M crystal oscillator to generate the 32K clock.
CNcomment:配置24M/40M晶体分频值，产生32K时钟。CNend
*
* @attention Impact Scope:systick、rtc.CNcomment:影响范围:systick、rtc。CNend
* @param     None
*
* @retval None
* @par 依赖:
*            @li hi_clock.h：Describes hardware clock APIS.
CNcomment:文件用于描述硬件时钟相关接口。CNend
* @see None
* @since Hi3861_V100R001C00
*/
HI_EXTERN hi_void hi_cfg_xtal_clk_div(hi_void);

#ifdef __cplusplus
}
#endif

#endif
