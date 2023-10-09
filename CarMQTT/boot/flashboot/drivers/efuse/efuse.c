/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: efuse sdk interface implementation.
 * Author: wangjun
 * Create: 2019-05-08
 */

#include "efuse.h"
#include "efuse_drv.h"

hi_u32 efuse_start_addr_unaligned_read(hi_u16 start_bit, hi_u16 size, hi_u8 diff_head_read, hi_u8 *data)
{
    hi_u32 ret = HI_ERR_FAILURE;
    hi_u32 check_sum;

    /* 根据3886 Efuse排布，起始地址非8bit对齐，计算出的实际读取size只可能为如下值(8/16/24/72) */
    if (size == SIZE_8_BITS) {
        ret = efuse_read_bits(start_bit, size, data);
        if (ret == HI_ERR_SUCCESS) {
            data[0] = data[0] >> diff_head_read;
        }
    } else if (size == SIZE_16_BITS) {
        hi_u16 tmp_data = 0;
        ret = efuse_read_bits(start_bit, size, (hi_u8 *)&tmp_data);
        if (ret == HI_ERR_SUCCESS) {
            tmp_data = tmp_data >> diff_head_read;
            if (start_bit == 0xE0) {
                *data = (hi_u8)(tmp_data & 0xFF);
            } else {
                *(hi_u16 *)data = tmp_data;
            }
        }
    } else if (size == SIZE_24_BITS) {
        hi_u32 tmp_data = 0;
        ret = efuse_read_bits(start_bit, SIZE_24_BITS, (hi_u8 *)&tmp_data);
        if (ret == HI_ERR_SUCCESS) {
            tmp_data = tmp_data >> diff_head_read;
            check_sum = (uintptr_t)data ^ DATA_LENGTH ^ (uintptr_t)(hi_u8 *)&tmp_data ^ DATA_LENGTH;
            if (memcpy_s(data, DATA_LENGTH, (hi_u8 *)&tmp_data, DATA_LENGTH, check_sum) != EOK) {
                return HI_ERR_FAILURE;
            }
        }
    } else if (size == SIZE_72_BITS) {
        hi_u8 tmp_data[SIZE_72_BITS / SIZE_8_BITS] = { 0 };
        hi_u32 data_u32[2]; /* U64 is divided into 2 U32 */
        hi_u8 end_u8;
        ret = efuse_read_bits(start_bit, SIZE_72_BITS, &tmp_data[0]);
        if (ret == HI_ERR_SUCCESS) {
            data_u32[0] = *(hi_u32 *)&tmp_data[0];  /* first U32 offset is 0 */
            data_u32[1] = *(hi_u32 *)&tmp_data[4];  /* sencond U32 offset is 4 */
            end_u8 = *(hi_u8 *)&tmp_data[8]; /* the last u8 bit */
            /* 丢掉第一个U32多读的低位(diff_head_read位) */
            data_u32[0] = data_u32[0] >> diff_head_read;
            data_u32[0] = data_u32[0] | (data_u32[1] << (SIZE_32_BITS - diff_head_read));
            data_u32[1] = data_u32[1] >> diff_head_read;
            /* 取第二个char的低位，作为第二个U32的高位(diff_head_read位) */
            data_u32[1] = data_u32[1] | ((hi_u32)end_u8 << (SIZE_32_BITS - diff_head_read));
            *(hi_u64 *)data = (((hi_u64)data_u32[1] << SIZE_32_BITS) | data_u32[0]);
        }
    }
    return ret;
}

hi_u32 efuse_bits_read(hi_u16 start_bit, hi_u16 size, hi_u8 *data, hi_u32 data_len)
{
    hi_u8 diff_head_read = 0;
    hi_u32 origine_size;
    hi_u32 ret;
    if (data_len > EFUSE_MAX_INDEX_SIZE) {
        return HI_ERR_FAILURE;
    }

    origine_size = size;
    if ((start_bit & 0x7) != 0x0) {
        diff_head_read = start_bit % SIZE_8_BITS;
        start_bit = start_bit - diff_head_read; /* 芯片要求 起始地址8bit对齐读取 */
        size = size + diff_head_read;
    }

    if ((size & 0x7) != 0x0) {
        size = ((size >> THREE_BITS_OFFSET) + 1) << THREE_BITS_OFFSET; /* 芯片要求 8bit为单位读取 */
    }

    if (diff_head_read == 0) {
        /* 起始地址8bit对齐 */
        ret = efuse_read_bits(start_bit, size, data);
        if (ret != HI_ERR_SUCCESS) {
            return ret;
        }
    } else {
        /* 起始地址非8bit对齐 */
        ret = efuse_start_addr_unaligned_read(start_bit, size, diff_head_read, data);
        if (ret != HI_ERR_SUCCESS) {
            return ret;
        }
    }

    if (origine_size <= SIZE_8_BITS) {
        *data &= ((1 << origine_size) - 1);
    } else if (origine_size <= SIZE_16_BITS) {
        *(hi_u16 *)data &= ((1 << origine_size) - 1);
    } else if (origine_size < SIZE_32_BITS) {
        *(hi_u32 *)data &= (((hi_u32)1 << origine_size) - 1);
    }

    return HI_ERR_SUCCESS;
}

/*****************************************************************************
* 函数名称: hi_efuse_read
* 功能描述: 根据索引号ID 读取efuse空间数据.
*
* 参数说明:
* efuse_id: efuse 区域ID.
* data: 存放待读取efuse值的地址
* data_len: 给data分配的空间，单位byte; (data_len*8)不能小于efuse_id对应efuse字段的长度向上8bits 对齐。
*
* 返 回 值:
* HI_ERR_SUCCESS:成功
* 其它 : 失败
*
* 调用要求:无
* 1.首先调用hi_get_efuse_cfg_by_id 获取区域长度。
* 2.根据长度向上取整获取读取空间。
* 3.调用本接口。
*****************************************************************************/
hi_u32 hi_efuse_read(hi_efuse_idx efuse_id, hi_u8 *data, hi_u8 data_len)
{
    hi_u16 start_bit = 0;
    hi_u16 size = 0;
    hi_u16 align_size;
    hi_u8 flag = EFUSE_IDX_NRW;

    if (efuse_id >= HI_EFUSE_IDX_MAX || data == HI_NULL) {
        return HI_ERR_EFUSE_INVALIDATE_PARA;
    }

    get_efuse_cfg_by_id(efuse_id, &start_bit, &size, &flag);

    if (flag == EFUSE_IDX_WO) {
        return HI_ERR_EFUSE_INVALIDATE_AUTH;
    }

    align_size = ((size & 0x7) != 0x0) ? (((size >> THREE_BITS_OFFSET) + 1) << THREE_BITS_OFFSET) : size;

    if (align_size > ((hi_u16)data_len * EIGHT_BITS)) {
        return HI_ERR_EFUSE_INVALIDATE_PARA;
    }

    return efuse_bits_read(start_bit, size, data, data_len);
}

/*****************************************************************************
* 函数名称: hi_efuse_write
* 功能描述: 根据索引号ID 写入efuse空间数据.
*
* 参数说明:
* efuse_id: efuse 区域ID.
* data: 即将向efuse中写入的数据.
*
* 返 回 值:
* HI_ERR_SUCCESS:成功
* 其他 : 失败
******************************************************************************/
hi_u32 hi_efuse_write(hi_efuse_idx efuse_id, const hi_u8 *data)
{
    hi_u16 start_bit = 0;
    hi_u16 size = 0;
    hi_u8 flag = EFUSE_IDX_NRW;
    hi_char err_state[EFUSE_MAX_INDEX_SIZE] = {
        0,
    };
    hi_u32 i;
    hi_u32 ret;

    if (efuse_id >= HI_EFUSE_IDX_MAX || data == HI_NULL) {
        boot_msg1("parameter err !", efuse_id);
        return HI_ERR_EFUSE_INVALIDATE_PARA;
    }

    get_efuse_cfg_by_id(efuse_id, &start_bit, &size, &flag);
    if (flag == EFUSE_IDX_RO) {
        boot_msg1("This section can not be write !flag = ", flag);
        return HI_ERR_EFUSE_INVALIDATE_AUTH;
    }

    ret = efuse_write_bits(start_bit, size, data, (hi_u8 *)&err_state[0]);
    if (ret != HI_ERR_SUCCESS) {
        boot_msg0("efuse write err");
        return ret;
    }

    for (i = 0; i < EFUSE_MAX_INDEX_SIZE; i++) {
        if (err_state[i]) {
            boot_msg1("errstate num is", i);
            return HI_ERR_EFUSE_WRITE_ERR;
        }
    }

    return HI_ERR_SUCCESS;
}

/*****************************************************************************
* 函数名称: hi_efuse_lock
* 功能描述: 根据索引号ID 锁定efuse某区域.
*
* 参数说明:
* lock_id: 锁定efuse 区域ID.
*
* 返 回 值:
* HI_ERR_SUCCESS:成功
* 其他 : 失败
*****************************************************************************/
hi_u32 hi_efuse_lock(hi_efuse_lock_id lock_id)
{
    hi_u16 err_stat = 0;
    hi_u32 ret;
    hi_u16 start_bit = 0;
    hi_u8 lock_val = 1;

    if (lock_id >= HI_EFUSE_LOCK_MAX) {
        return HI_ERR_EFUSE_INVALIDATE_ID;
    }

    if (lock_id < HI_EFUSE_LOCK_FLASH_ENCPY_CNT3_ID) {
        start_bit = EFUSE_LOCK_START_BITS + lock_id;
    } else {
        start_bit = EFUSE_LOCK_FIELD2_START_BITS + lock_id - HI_EFUSE_LOCK_FLASH_ENCPY_CNT3_ID;
    }

    ret = efuse_write_bits(start_bit, 1, &lock_val, (hi_u8 *)&err_stat);
    if ((ret == HI_ERR_SUCCESS) && (err_stat == 0)) {
        return HI_ERR_SUCCESS;
    } else if (err_stat != 0) {
        return HI_ERR_EFUSE_WRITE_ERR;
    }

    return ret;
}

/*****************************************************************************
* 函数名称: hi_efuse_get_lockstat
* 功能描述: 获取efuse锁定状态.
*
* 参数说明:
* lock_stat: 返回的锁定状态数据指针，数据前EFUSE_LOCK_SIZE有效.
*
* 返 回 值:
* HI_ERR_SUCCESS:成功
* 其他 : 失败
*****************************************************************************/
hi_u32 hi_efuse_get_lockstat(hi_u64 *lock_stat)
{
    hi_u16 start_bit = EFUSE_LOCK_START_BITS;
    hi_u16 size = EFUSE_LOCK_SIZE;
    hi_u8 diff_head_read;
    hi_u64 tmp_data = 0;
    hi_u64 tmp_data_filed_two = 0;
    hi_u32 ret;

    if (lock_stat == HI_NULL) {
        return HI_ERR_FAILURE;
    }

    /* start address and size 8 bits align. */
    diff_head_read = start_bit % EIGHT_BITS;
    start_bit = start_bit - diff_head_read;
    size = size + diff_head_read;

    ret = efuse_read_bits(start_bit, size, (hi_u8 *)&tmp_data);
    if (ret == HI_ERR_SUCCESS) {
        tmp_data = tmp_data >> diff_head_read;
    } else {
        return HI_ERR_FAILURE;
    }

    start_bit = EFUSE_LOCK_FIELD2_START_BITS;
    size = EFUSE_LOCK_FIELD2_SIZE;

    /* start address and size 8 bits align. */
    diff_head_read = start_bit % EIGHT_BITS;
    start_bit = start_bit - diff_head_read;
    size = size + diff_head_read;

    ret = efuse_read_bits(start_bit, size, (hi_u8 *)&tmp_data_filed_two);
    if (ret == HI_ERR_SUCCESS) {
        tmp_data_filed_two = tmp_data_filed_two >> diff_head_read;
    } else {
        return HI_ERR_FAILURE;
    }

    *lock_stat = (((tmp_data_filed_two << EFUSE_LOCK_SIZE) & 0x1F000000000) | (tmp_data & 0xFFFFFFFFF));

    return HI_ERR_SUCCESS;
}

/*****************************************************************************
* 函数名称: hi_efuse_usr_read
* 功能描述: 通用efuse bit读取接口，起始地址和长度需要8bit对齐.
*
* 参数说明:
* start_bit: efuse起始bit位.
* size: 待读取的bit位数，如果输入不是8bit对齐则函数内部会处理为8bit对齐，用户读取数据后需处理后使用。
* key_data: 读取出的数据存放内存地址.
*
* 返 回 值:
* HI_ERR_SUCCESS:成功
* 其他 : 失败
******************************************************************************/
hi_u32 hi_efuse_usr_read(hi_u16 start_bit, hi_u16 size, hi_u8 *key_data)
{
    hi_u16 real_size = size;

    if ((start_bit >= EFUSE_PGM_ADDR_SIZE) || ((start_bit & 0x7) != 0) || (key_data == HI_NULL) || (size == 0)) {
        return HI_ERR_FAILURE;
    }

    /* 芯片要求 8bit为单位读取 */
    if ((size & 0x7) != 0x0) {
        real_size = ((size >> 3) + 1) << 3; /* right shift 3bit;add;then left shift 3bit */
    }

    if ((start_bit + real_size) > EFUSE_PGM_ADDR_SIZE) {
        return HI_ERR_FAILURE;
    }

    return efuse_read_bits(start_bit, real_size, key_data);
}

/******************************************************************************
* 函数名称: hi_efuse_usr_write
* 功能描述: 通用eufse bit写入接口，最大写入32个字节，即256bit.
*
* 参数说明:
* start_bit: efuse起始bit位.
* size: 用户写入数据长度，单位为bit.
* key_data: 待写入的数据放到该地址，最长为32byte。
*
* 返 回 值:
* 其他 : 失败
* HI_ERR_SUCCESS:成功
*****************************************************************************/
hi_u32 hi_efuse_usr_write(hi_u16 start_bit, hi_u16 size, const hi_u8 *key_data)
{
    hi_u8 usr_err_stat[EFUSE_MAX_INDEX_SIZE];
    hi_u32 i;
    hi_u32 ret;

    if (size > (EFUSE_MAX_INDEX_SIZE * SIZE_8_BITS) || key_data == HI_NULL || size == 0) {
        return HI_ERR_FAILURE;
    }
    hi_u32 check_sum = (uintptr_t)usr_err_stat ^ EFUSE_MAX_INDEX_SIZE ^ 0 ^ EFUSE_MAX_INDEX_SIZE;
    memset_s(usr_err_stat, EFUSE_MAX_INDEX_SIZE, 0, EFUSE_MAX_INDEX_SIZE, check_sum);

    ret = efuse_write_bits(start_bit, size, key_data, (hi_u8 *)&usr_err_stat[0]);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    for (i = 0; i < EFUSE_MAX_INDEX_SIZE; i++) {
        if (usr_err_stat[i]) {
            return HI_ERR_FAILURE;
        }
    }

    return HI_ERR_SUCCESS;
}

