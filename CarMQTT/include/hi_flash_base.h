/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: flash info.
 * Author: wuxianfeng
 * Create: 2019-05-30
 */

#ifndef __HI_FLASH_BASE_H__
#define __HI_FLASH_BASE_H__

#include <hi_types_base.h>

#define HI_FLASH_CMD_ADD_FUNC   0
#define HI_FLASH_CMD_GET_INFO   1  /**< IOCTL command ID for obtaining the flash information.
                                        The corresponding output parameter points to the hi_flash_info structure.
CNcomment:IOCTL获取Flash信息，对应出参指向结构体为hi_flash_info.CNend */
#define HI_FLASH_CMD_IS_BUSY    2  /**< IOCTL Obtain whether the flash memory is busy. The corresponding output
                                        parameter point type is hi_bool.
CNcomment:IOCTL获取Flash是否busy，对应出参指向类型为hi_bool CNend */

#define HI_FLASH_CHIP_ID_NUM    3
#define HI_FLASH_CAPACITY_ID    2

/**
* @ingroup  iot_flash
*
* Flash information obtaining structure, used to describe the return structure of the command ID HI_FLASH_CMD_GET_INFO.
CNcomment:Flash信息获取结构体，用于描述命令ID(HI_FLASH_CMD_GET_INFO)的返回结构体。CNend
*/
typedef struct {
    hi_char *name;                     /**< Flash name.CNcomment:Flash名字CNend  */
    hi_u8   id[HI_FLASH_CHIP_ID_NUM];  /**< Flash Id  */
    hi_u8   pad;
    hi_u32 total_size;                 /**< Flash totoal size (unit: byte).
                                          CNcomment:Flash总大小（单位：byte）CNend  */
    hi_u32 sector_size;                /**< Flash block size (unit: byte).
                                          CNcomment:Flash块大小（单位：byte）CNend */
} hi_flash_info;

#endif

