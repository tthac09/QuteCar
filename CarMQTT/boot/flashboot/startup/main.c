/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: Flash boot main.
 * Author: Hisilicon
 * Create: 2012-12-22
 */
#include <hi_flashboot.h>
#include <boot_start.h>
#include <hi_boot_rom.h>
#include "main.h"
#ifdef CONFIG_FLASH_ENCRYPT_SUPPORT
#include <crypto.h>
#endif

hi_u32 g_uart_auth;
#define FLASHBOOT_UART_DEFAULT_PARAM    {115200, 8, 1, 0, 0, 0, 2, 1, 4}

hi_void boot_kernel(uintptr_t kaddr)
{
    __asm__ __volatile__("ecall");  /* switch U-MODE -> M-MODE */
    hi_void (*entry)(hi_void) = (hi_void*)(kaddr);
    entry();
}

hi_void boot_io_init(hi_void)
{
    hi_io_set_func(HI_IO_NAME_GPIO_3, HI_IO_FUNC_GPIO_3_UART0_TXD); /* uart0 tx */
    hi_io_set_func(HI_IO_NAME_GPIO_4, HI_IO_FUNC_GPIO_4_UART0_RXD); /* uart0 rx */
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
#define XTAL_DS     0x4
#define OSC_DRV_CTL 0x2
/* gpio00 外接32K时钟;如果无外接32K,在start_fastboot中注释掉该函数 */
hi_void boot_extern_32k(hi_void)
{
    hi_u16 chip_id, chip_id_bk;
    hi_u32 ret;
    ret = hi_efuse_read(HI_EFUSE_CHIP_RW_ID, (hi_u8 *)&chip_id, (hi_u8)sizeof(hi_u8));
    if (ret != HI_ERR_SUCCESS) {
        return;
    }
    ret = hi_efuse_read(HI_EFUSE_CHIP_BK_RW_ID, (hi_u8 *)&chip_id_bk, (hi_u8)sizeof(hi_u8));
    if (ret != HI_ERR_SUCCESS) {
        return;
    }
    hi_u8 chip_ver = (chip_id >> OFFSET_4_B) & MSK_3_B; /* chip_id bit[4:7] is chip_ver. */
    hi_u8 chip_ver_bk = (chip_id_bk >> OFFSET_4_B) & MSK_3_B; /* chip_id bit[4:7] is chip_ver. */
    if (chip_ver != HI_CHIP_VER_HI3861L) {
        if (chip_ver_bk != HI_CHIP_VER_HI3861L) {
            return;
        }
    }
    hi_u32 reg_val;
    hi_reg_read(HI_IOCFG_REG_BASE + IO_CTRL_REG_BASE_ADDR, reg_val);
    reg_val &= ~(MSK_2_B << OFFSET_4_B); /* 驱动能力最大 */
    reg_val |= (MSK_2_B << OFFSET_22_B); /* external xtal, osc enable */
    reg_val &= ~(MSK_3_B << OFFSET_25_B);
    reg_val |= XTAL_DS << OFFSET_25_B;   /* 1017na */
    reg_val &= ~(MSK_2_B << OFFSET_28_B);
    reg_val |= OSC_DRV_CTL << OFFSET_28_B;   /* 4Mohm */
    hi_reg_write(HI_IOCFG_REG_BASE + IO_CTRL_REG_BASE_ADDR, reg_val);
}

/* the entry of C. */
hi_void start_fastboot(hi_void)
{
    hi_u32 ret;
    hi_malloc_func malloc_funcs = {0, };
    uart_param_stru default_uart_param = FLASHBOOT_UART_DEFAULT_PARAM;
    hi_watchdog_disable();
    hi_watchdog_enable(WDG_TIME_US);

    /* io config */
    boot_io_init();

    /* 堆区注册及初始化 */
    malloc_funcs.init = rom_boot_malloc_init;
    malloc_funcs.boot_malloc = rom_boot_malloc;
    malloc_funcs.boot_free = rom_boot_free;

    hi_register_malloc((uintptr_t)&__heap_begin__, &malloc_funcs);
    hi_u32 check_sum = ((uintptr_t)&__heap_begin__) ^ ((uintptr_t)&__heap_end__);
    boot_malloc_init((uintptr_t)&__heap_begin__, (uintptr_t)&__heap_end__, check_sum);

    /* 调试串口初始化 */
    ret = serial_init(UART0, default_uart_param);
    if (ret != HI_ERR_SUCCESS) {
        boot_msg0("uart err"); /* 使用romboot的串口配置打印 */
    }

    boot_extern_32k();
    /* FLASH驱动操作初始化 */
    boot_flash_init();

    ret = hi_factory_nv_init(HI_FNV_DEFAULT_ADDR, HI_NV_DEFAULT_TOTAL_SIZE, HI_NV_DEFAULT_BLOCK_SIZE); /* NV初始化 */
    if (ret != HI_ERR_SUCCESS) {
        boot_msg0("fnv err");
    }

    ret = hi_flash_partition_init();
    if (ret != HI_ERR_SUCCESS) {
        boot_msg0("parti err");
    }

    execute_upg_boot();

    mdelay(RESET_DELAY_MS);
    global_reset();
}
