/**
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Upgrade common interface file.
 */

#include <boot_upg_common.h>

hi_bool boot_upg_tool_bit_test(const hi_u8 *data, hi_u16 pos)
{
    hi_u16 base = pos / BIT_U8;
    hi_u16 i = pos % BIT_U8;
    return (hi_bool)bit_get(data[base], i);
}

hi_void boot_upg_tool_bit_set(hi_u8 *data, hi_u16 pos, hi_u8 val)
{
    hi_u16 base = pos / BIT_U8;
    hi_u16 i = pos % BIT_U8;

    bit_set(data[base], i, val);
}


