/**
* @file hi_watchdog.h
*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.  \n
* Description: Watchdog interfaces.   \n
* Author: Hisilicon   \n
* Create: 2019-07-03
*/

/**
 * @defgroup iot_watchdog Watchdog
 * @ingroup drivers
 */
#ifndef __HI_WATCHDOG_H__
#define __HI_WATCHDOG_H__

#include <hi_types_base.h>

/**
* @ingroup  iot_watchdog
* @brief Enables the watchdog.CNcomment:使能看门狗。CNend
*
* @par 描述:
*          Enables the watchdog.CNcomment:使能看门狗。CNend
*
* @attention None
* @param  None
*
* @retval None
* @par 依赖:
*            @li hi_watchdog.h：describes the watchdog APIs.CNcomment:文件用于描述看门狗相关接口。CNend
* @see None
* @since Hi3861_V100R001C00
*/
hi_void hi_watchdog_enable(hi_void);

/**
* @ingroup  iot_watchdog
* @brief Feeds the watchdog.CNcomment:喂狗。CNend
*
* @par 描述: Feeds the watchdog.CNcomment:喂狗。CNend
*
* @attention None
* @param  None
*
* @retval None
* @par 依赖:
*            @li hi_watchdog.h：describes the watchdog APIs.CNcomment:文件用于描述看门狗相关接口。CNend
* @see None
* @since Hi3861_V100R001C00
*/
hi_void hi_watchdog_feed(hi_void);

/**
* @ingroup  iot_watchdog
* @brief Disables the watchdog.CNcomment:关闭看门狗。CNend
*
* @par 描述:
*           @li Disable the clock enable control of the watchdog.CNcomment:禁止WatchDog时钟使能控制位。CNend
*           @li Mask the watchdog reset function.CNcomment:屏蔽WatchDog复位功能。CNend
*
* @attention None
* @param  None
*
* @retval None
* @par 依赖:
*            @li hi_watchdog.h：describes the watchdog APIs.CNcomment:文件用于描述看门狗相关接口。CNend
* @see None
* @since Hi3861_V100R001C00
*/
hi_void hi_watchdog_disable(hi_void);

#endif
