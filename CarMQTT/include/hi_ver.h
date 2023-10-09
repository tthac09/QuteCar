/**
* @file hi_ver.h
*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved. \n
* Description: Soft ver interfaces. \n
* Author: Hisilicon \n
* Create: 2019-03-04
*/

/** @defgroup iot_ver Soft ver
 * @ingroup system
 */

#ifndef __HI_VER_H__
#define __HI_VER_H__
#include <hi_types.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
* @ingroup  iot_ver
* @brief  Obtains SDK version information. CNcomment:获取SDK版本信息CNend
*
* @par 描述:
*         Obtains SDK version information. CNcomment:获取SDK版本信息CNend
* @attention None
* @retval #hi_char*     SDK version information string. CNcomment:SDK版本信息字符串CNend
*
* @par Dependency:
*      @li hi_ver.h: This file describes version information APIs.CNcomment:文件用于描述系统相关接口.CNend
* @see  None
* @since Hi3861_V100R001C00
*/
const hi_char *hi_get_sdk_version(hi_void);

/**
* @ingroup  iot_ver
* @brief  Obtains boot version in secure boot mode. CNcomment:安全启动模式下，获取BOOT版本号CNend
*
* @par 描述:
*         Obtains boot version in secure boot mode. CNcomment:安全启动模式下，获取BOOT版本号CNend
* @attention Ver always be 0 in non-secure boot mode. CNcomment: 非安全启动模式下，该版本号始终为0。CNend
* @retval #hi_u8     boot ver num, value from 0-16, Return 0xFF means get boot ver fail.
CNcomment:boot版本号，有效值为0-16，返回0xFF表示获取BOOT版本号失败CNend
* @par Dependency:
*      @li hi_ver.h: This file describes version information APIs.CNcomment:文件用于描述系统相关接口.CNend
* @see  None
* @since Hi3861_V100R001C00
*/
hi_u8 hi_get_boot_ver(hi_void);

/**
* @ingroup  iot_ver
* @brief  Obtains kernel version in secure boot mode. CNcomment:安全启动模式下，获取kernel版本号CNend
*
* @par 描述:
*         Obtains kernel version in secure boot mode. CNcomment:安全启动模式下，获取kernel版本号CNend
* @attention Ver always be 0 in non-secure boot mode. CNcomment:非安全启动模式下，该版本号始终为0。CNend
* @retval #hi_u8     kernel ver num, value from 0-48, Return 0xFF means get kernel ver fail.
CNcomment:kernel版本号，有效值为0-48，返回0xFF表示获取kernel版本号失败CNend
*
* @par Dependency:
*      @li hi_ver.h: This file describes version information APIs.CNcomment:文件用于描述系统相关接口.CNend
* @see  None
* @since Hi3861_V100R001C00
*/
hi_u8 hi_get_kernel_ver(hi_void);

#ifdef __cplusplus
}
#endif
#endif
