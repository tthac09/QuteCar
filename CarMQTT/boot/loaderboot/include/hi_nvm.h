/**
 * @file hi_nvm.h
 *
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved. \n
 * Description: hi_nvm.h. \n
 * Author: hisilicon \n
 * Create: 2019-08-27
 */

#ifndef __HI_NVM_H__
#define __HI_NVM_H__

#include <hi_types.h>
#include <hi_boot_rom.h>

#define hi_make_identifier(a, b, c, d)      hi_makeu32(hi_makeu16(a, b), hi_makeu16(c, d))
#define HNV_FILE_SIGNATURE               hi_make_identifier('H', 'N', 'V', '$')
#define FNV_FILE_SIGNATURE               hi_make_identifier('F', 'N', 'V', '#')

#define FACTORY_NV_SIZE   0x2000
#define FLASH_BLOCK_SIZE  0x1000
#define HNV_NCRC_SIZE  8                      /* 不做crc的长度 */
#define NV_TOTAL_MAX_NUM  255                 /* 可设置的nv最大个数 */
#define HNV_FAULT_TOLERANT_TIMES  3           /* 容错次数 */

#define HNV_MANAGE_FIXED_LEN  24              /* 截止到reserve */

/* 工厂区NV 结构体需要和kernel下保持完全一致，固定后不能修改 */
typedef struct _hi_nvm_manage_s_ {
    hi_u32  magic;              /* nv头魔术字 */
    hi_u32  crc;                /* nv管理区和数据区crc32 起：ver；止：flash_size结尾 */
    hi_u8   ver;                /* nv管理区结构体版本号 */
    hi_u8   head_len;           /* nv头的长度，起：magic；止：reserve结尾，且4字节向后对齐 */
    hi_u16  total_num;          /* nv总个数 */
    hi_u32  seq;                /* 写nv序号标志(升级依然保留，以计算寿命) */
    hi_u32  ver_magic;          /* 版本魔术字，和kernel版本魔术字匹配 */
    hi_u32  flash_size;         /* NV占用的FLASH大小，如4096(4K)、65536(64K),暂时未用，后续归一 */
    hi_u8   keep_id_range[2];   /* 升级保留id范围，0:id下边界 1:id上边界 size 2 */
    hi_u8   reserve[2];         /* size 2 */
    hi_u8   nv_item_data[0];    /* 索引表 */
} hi_nv_manage;

typedef struct hi_nv_item_index_s_ {
    hi_u8 nv_id;
    hi_u8 nv_len;                /* nv实际长度，不算crc32值，crc32紧挨着存放 */
    hi_u16 nv_offset;            /* 相对本nv区偏移 */
} hi_nv_item_index;

typedef struct _hi_nv_ctrl_s_ {
    hi_u32 base_addr;
    hi_u32 block_size;
    hi_u32 total_block_size;
    hi_u32 current_addr;         /* 保留更快 */
    hi_u32 seq;
    hi_u32 sem_handle;

    hi_u8 init_flag;
    hi_u8 reserve;
    hi_u16 total_num;         /* nv个数 */
    hi_u32 ver_magic;
    hi_nv_item_index* index;
} hi_nv_ctrl;

typedef enum _hi_nv_type_e_ {
    HI_TYPE_NV = 0,
    HI_TYPE_FACTORY_NV,
    HI_TYPE_TEMP,
    HI_TYPE_NV_MAX,
} hi_nv_type;

hi_u32 hi_nv_flush_keep_ids(hi_u8* addr, hi_u32 len);
hi_u32 hi_nv_block_write(hi_u8* nv_file, hi_u32 len, hi_u32 flag);

/** @defgroup iot_nv NV Management APIs
* @ingroup  iot_flashboot
*/
/**
* @ingroup  iot_nv
* Maximum length of an NV item (unit: byte). CNcomment:NV项最大长度（单位：byte）。CNend
*/
#define HNV_ITEM_MAXLEN             (256 - 4)

/**
* @ingroup  iot_nv
*/
#define PRODUCT_CFG_NV_REG_NUM_MAX  20

/**
* @ingroup  iot_nv
*/
#define HI_FNV_DEFAULT_ADDR         0x8000

/**
* @ingroup  iot_nv
*/
#define HI_NV_DEFAULT_TOTAL_SIZE    0x2000

/**
* @ingroup  iot_nv
*/
#define HI_NV_DEFAULT_BLOCK_SIZE    0x1000

/**
* @ingroup  iot_nv
*
* Factory NV area user area end ID. CNcomment:工厂区NV结束ID。CNend
*/
#define HI_NV_FACTORY_USR_ID_END    0x20

/**
* @ingroup  iot_nv
*/
#define  HI_NV_FTM_FLASH_PARTIRION_TABLE_ID  0x02

/**
* @ingroup  iot_nv
* @brief Initializes NV management in the factory partition.CNcomment:工厂区NV初始化。CNend
*
* @par 描述:
*          Initializes NV management in the factory partition.CNcomment: 工厂区NV管理初始化。CNend
*
* @attention The parameters cannot be set randomly and must match the product delivery plan.
CNcomment:参数不能随意配置，需要与产品出厂规划匹配。CNend
* @param  addr [IN] type #hi_u32，Start address of the NV factory partition in the flash. The address is planned by
*                   the factory and set by the boot macro #FACTORY_NV_ADDR.
CNcomment:设置工厂区NV FLASH首地址，由出厂规划，boot宏定义FACTORY_NV_ADDR 统一设置。CNend
*
* @retval #0            Success.
* @retval #Other        Failure. For details, see hi_errno.h.
* @par 依赖:
*           @li hi_flashboot.h：Describes NV APIs.CNcomment:文件用于描述NV相关接口。CNend
* @see hi_factory_nv_write | hi_factory_nv_read。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_factory_nv_init(hi_u32 addr, hi_u32 total_size, hi_u32 block_size);

/**
* @ingroup  iot_nv
* @brief Sets the NV value in the factory partition. CNcomment:设置工厂区NV值。CNend
*
* @par 描述:
*           Sets the NV value in the factory partition.CNcomment:设置工厂区NV值。CNend
*
* @attention None
* @param  id    [IN] type #hi_u8，NV item ID, ranging from #HI_NV_FACTORY_ID_START to #HI_NV_FACTORY_USR_ID_END.
CNcomment:NV项ID，范围从#HI_NV_FACTORY_ID_START到#HI_NV_FACTORY_USR_ID_END。CNend
* @param  pdata [IN] type #hi_pvoid，NV item data.CNcomment:NV项数据。CNend
* @param  len   [IN] type #hi_u8，Length of an NV item (unit: byte). The maximum value is #HNV_ITEM_MAXLEN.
CNcomment:NV项长度（单位：byte），最大不允许超过HNV_ITEM_MAXLEN。CNend
*
* @retval #0            Success.
* @retval #Other        Failure. For details, see hi_errno.h.
* @par 依赖:
*           @li hi_flashboot.h：Describes NV APIs.CNcomment:文件用于描述NV相关接口。CNend
* @see hi_factory_nv_read。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_factory_nv_write(hi_u8 id, hi_pvoid pdata, hi_u8 len, hi_u32 flag);

/**
* @ingroup  iot_nv
* @brief Reads the NV value in the factory partition.CNcomment:读取工厂区NV值。CNend
*
* @par 描述:
*           Reads the NV value in the factory partition.读取工厂区NV值。CNend
*
* @attention None
*
* @param  id      [IN] type #hi_u8，NV item ID, ranging from #HI_NV_NORMAL_ID_START to #HI_NV_NORMAL_USR_ID_END.
CNcomment:NV项ID，范围从#HI_NV_NORMAL_ID_START到#HI_NV_NORMAL_USR_ID_END。CNend
* @param  pdata   [IN] type #hi_pvoid，NV item data.CNcomment:NV项数据。CNend
* @param  len     [IN] type #hi_u8，Length of an NV item (unit: byte). The maximum value is HNV_ITEM_MAXLEN.
CNcomment:NV项长度（单位：byte），最大不允许超过HNV_ITEM_MAXLEN。CNend
*
* @retval #0            Success.
* @retval #Other        Failure. For details, see hi_errno.h.
* @par 依赖:
*           @li hi_flashboot.h：Describes NV APIs.CNcomment:文件用于描述NV相关接口。CNend
* @see hi_factory_nv_write。
* @since Hi3861_V100R001C00
*/
hi_u32 hi_factory_nv_read(hi_u8 id, hi_pvoid pdata, hi_u8 len, hi_u32 flag);

/** @defgroup iot_crc32 CRC32 APIs
* @ingroup iot_flashboot
*/
/**
* @ingroup  iot_crc32
* @brief  Generates a 16-bit CRC value.CNcomment:生成16位CRC校验值。CNend
*
* @par 描述:
*           Generates a 16-bit CRC value.CNcomment:生成16位CRC校验值。CNend
*
* @attention None
* @param  crc               [IN] type #hi_u16，The CRC initial value.CNcomment:CRC初始值。CNend
* @param  p                 [IN] type #const hi_u8*，Pointer to the buffer to be verified.
CNcomment:被校验Buffer指针。CNend
* @param  len               [IN] type #hi_u32，Length of the buffer to be verified (unit: Byte).
CNcomment:被校验Buffer长度（单位：byte）。CNend
*
* @retval #HI_ERR_SUCCESS   Success
* @retval #Other            Failure. For details, see hi_boot_err.h.
*
* @par 依赖:
*            @li hi_flashboot.h：Describes CRC APIs.CNcomment:文件包含CRC校验接口。CNend
* @see  None
* @since Hi3861_V100R001C00
*/
hi_u32  hi_crc32 (hi_u32 crc, const hi_u8 *p, hi_u32 len);

#endif   /* __HI_NVM_H__ */

