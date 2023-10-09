/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: stack protectorr.
 * Author: Hisilicon
 * Create: 2020-03-22
 */

#include "hi_boot_rom.h"

unsigned long __stack_chk_guard = 0xd00a0dff;

void __stack_chk_fail(void)
{
    boot_msg1("stack-protector:corrupted in:", (uintptr_t)__builtin_return_address(0));
    udelay(2000); /* delay 2000 us to reset */
    global_reset();
}