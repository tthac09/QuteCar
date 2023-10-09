/**
* @file hi_systick.h
*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.  \n
* Description: system tick APIs.   \n
* Author: Hisilicon   \n
* Create: 2019-07-03
*/

/**
 * @defgroup systick System Tick Status
 * @ingroup drivers
 */
#ifndef __HI_SYSTICK_H__
#define __HI_SYSTICK_H__
#include <hi_types_base.h>

/**
* @ingroup  systick
* @brief  Obtains systick currect value. CNcomment:获取systick当前计数值。CNend
*
* @par 描述:
* @li   Obtains the current count value of systick. The time of each value is determined by the systick clock source.
*       The systick clock is 32Khz, and the tick value is 1/32000 seconds.CNcomment:获取systick当前计数值。
每个值的时间由systick时钟源决定。systick时钟为32Khz，一个tick值为1/32000秒。CNend
* $li   After the system is powered on, systick immediately adds a count from 0.CNcomment:系统上电运行后，
systick立刻从0开始递增加一计数。CNend
*
* @attention The delay interface is invoked in the interface. Therefore, it is prohibited to invoke this interface in
*            the interrupt context.CNcomment:接口内调用了延时接口，所以禁止在中断上下文中调用该接口。CNend
* @param  None
*
* @retval #hi_u64 Indicates the obtained current count value.CNcomment:获取到的当前计数值。CNend
*
* @par 依赖:
*           @li hi_systick.h：Describes systick APIs.CNcomment:文件用于描述SYSTICK相关接口。CNend
* @see  hi_systick_clear。
* @since Hi3861_V100R001C00
*/
hi_u64 hi_systick_get_cur_tick(hi_void);

/**
* @ingroup  systick
* @brief  The value of systick is cleared.CNcomment:将systick计数值清零。CNend
*
* @par 描述:
*         The value of systick is cleared.CNcomment:将systick计数值清零。CNend
*
* @attention After the interface is returned, the clock cycles of three systick clocks need to be cleared.
CNcomment:接口返回后需要等三个systick的时钟周期才会完成清零。CNend
* @param  None
*
* @retval None
* @par 依赖:
*           @li hi_systick.h：Describes systick APIs.CNcomment:文件用于描述SYSTICK相关接口。CNend
* @see  hi_systick_get_cur_tick。
* @since Hi3861_V100R001C00
*/
hi_void hi_systick_clear(hi_void);

#endif
