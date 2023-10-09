/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: Flash boot main.
 * Author: Hisilicon
 * Create: 2012-12-22
 */

#include <hi_boot_rom.h>
#include <hi_loaderboot_flash.h>
#include <cmd_loop.h>
#include "main.h"

hi_void boot_io_init(hi_void)
{
    hi_io_set_func(HI_IO_NAME_GPIO_3, HI_IO_FUNC_GPIO_3_UART0_TXD); /* uart0 tx */
    hi_io_set_func(HI_IO_NAME_GPIO_4, HI_IO_FUNC_GPIO_4_UART0_RXD); /* uart0 rx */
    hi_io_set_func(HI_IO_NAME_GPIO_13, HI_IO_FUNC_GPIO_13_SSI_DATA);
    hi_io_set_func(HI_IO_NAME_GPIO_14, HI_IO_FUNC_GPIO_14_SSI_CLK);
    hi_io_set_func(HI_IO_NAME_SFC_CLK, HI_IO_FUNC_SFC_CLK_SFC_CLK);
    hi_io_set_func(HI_IO_NAME_SFC_IO3, HI_IO_FUNC_SFC_IO_3_SFC_HOLDN);
}

hi_void boot_flash_init(hi_void)
{
    hi_flash_cmd_func flash_funcs = {0};
    flash_funcs.init = hi_flash_init;
    flash_funcs.read = hi_flash_read;
    flash_funcs.write = hi_flash_write;
    flash_funcs.erase = hi_flash_erase;
    hi_cmd_regist_flash_cmd(&flash_funcs);
    (hi_void) hi_flash_init();
}

/* the entry of C. */
hi_void start_loaderboot(hi_void)
{
    uart_ctx *cmd_ctx = HI_NULL;
    hi_malloc_func malloc_funcs = {0, };

    /* io config */
    boot_io_init();

    /* 堆区注册及初始化 */
    malloc_funcs.init = rom_boot_malloc_init;
    malloc_funcs.boot_malloc = rom_boot_malloc;
    malloc_funcs.boot_free = rom_boot_free;

    hi_register_malloc((uintptr_t)&__heap_begin__, &malloc_funcs);
    hi_u32 check_sum = (uintptr_t)(&__heap_begin__) ^ (uintptr_t)(&__heap_end__);
    boot_malloc_init((uintptr_t)&__heap_begin__, (uintptr_t)&__heap_end__, check_sum);

    /* FLASH驱动操作初始化 */
    boot_flash_init();

    cmd_ctx = cmd_loop_init();
    if (cmd_ctx == HI_NULL) {
        boot_msg0("cmd init fail");
        reset();
        while (1) {
            ;
        }
    }

    /* 进入等待命令循环，关闭看门狗 */
    hi_watchdog_disable();
    loader_ack(ACK_SUCCESS);
    boot_msg0("Entry loader");

    hi_u32 ret = flash_protect_set_protect(0, HI_FALSE);
    if (ret != HI_ERR_SUCCESS) {
        boot_msg0("Unlock Fail!");
    }

    boot_msg0("============================================\n");
    cmd_loop(cmd_ctx);
    reset();
    while (1) {
        ;
    }
}

