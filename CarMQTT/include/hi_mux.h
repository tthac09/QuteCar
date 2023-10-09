/**
* @file hi_mux.h
*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.  \n
* Description: Mutex APIs.   \n
* Author: Hisilicon   \n
* Create: 2019-01-17
*/

/**
 * @defgroup iot_mux Mutex
 * @ingroup osa
 */

#ifndef __HI_MUX_H__
#define __HI_MUX_H__
#include <hi_types_base.h>

/**
* @ingroup  iot_mux
* @brief  Creates a mutex.CNcomment:创建互斥锁。CNend
*
* @par 描述:
*           Creates a mutex.CNcomment:创建互斥锁。CNend
*
* @attention None
* @param  mux_id  [OUT] type #hi_u32*，Mutex handle.CNcomment:互斥锁句柄。CNend
*
* @retval #0      Success
* @retval #Other  Failure. For details, see hi_errno.h.
* @par 依赖:
*            @li hi_mux.h：Describes mutex APIs.CNcomment:文件用于描述互斥锁相关接口。CNend
* @see  hi_mux_delete。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_mux_create (hi_u32 *mux_id);

/**
* @ingroup  iot_mux
* @brief  Deletes a mutex.CNcomment:删除互斥锁。CNend
*
* @par 描述:
*           Deletes a mutex.CNcomment:删除互斥锁。CNend
*
* @attention None
*
* @param  mux_id  [IN] type #hi_u32*，Mutex handle.CNcomment:互斥锁句柄。CNend
*
* @retval #0      Success
* @retval #Other  Failure. For details, see hi_errno.h.
* @par 依赖:
*            @li hi_mux.h：Describes mutex APIs.CNcomment:文件用于描述互斥锁相关接口。CNend
* @see  hi_mux_create。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_mux_delete(hi_u32 mux_id);

/**
* @ingroup  iot_mux
* @brief  Waits for a mutex.CNcomment:等待互斥锁。CNend
*
* @par 描述:
*           Waits for a mutex.CNcomment:等待互斥锁。CNend
*
* @attention Mutexes support priority inversion.CNcomment:互斥锁支持优先级翻转。CNend
* @param  mux_id     [IN] type #hi_u32*，Mutex handle.CNcomment:互斥锁句柄。CNend
* @param  timeout_ms [IN] type #hi_u32，Timeout period (unit: ms). HI_SYS_WAIT_FOREVER indicates permanent waiting.
CNcomment:超时时间（单位：ms）。HI_SYS_WAIT_FOREVER为永久等待。CNend
*
* @retval #0      Success
* @retval #Other  Failure. For details, see hi_errno.h.
* @par 依赖:
*            @li hi_mux.h：Describes mutex APIs.CNcomment:文件用于描述互斥锁相关接口。CNend
* @see  hi_mux_post。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_mux_pend(hi_u32 mux_id, hi_u32 timeout_ms);

/**
* @ingroup  iot_mux
* @brief  Releases a mutex.CNcomment:释放互斥锁。CNend
*
* @par 描述:
*           Releases a mutex.CNcomment:释放互斥锁。CNend
*
* @attention A mutex can be released only in the task that has obtained the mutex.
CNcomment:互斥锁只能在获取到互斥锁的任务中释放。CNend
*
* @param  mux_id  [IN] type #hi_u32*，Mutex handle.CNcomment:互斥锁句柄。CNend
*
* @retval #0      Success
* @retval #Other  Failure. For details, see hi_errno.h.
* @par 依赖:
*            @li hi_mux.h：Describes mutex APIs.CNcomment:文件用于描述互斥锁相关接口。CNend
* @see  hi_mux_pend。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_mux_post(hi_u32 mux_id);

#endif

