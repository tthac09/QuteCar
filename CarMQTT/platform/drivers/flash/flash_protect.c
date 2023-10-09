/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: flash protect process implementatioin.
 * Author: Hisilicon
 * Create: 2019-12-27
 */
#include "flash_prv.h"
#ifdef HI_FLASH_SUPPORT_FLASH_PROTECT
#include <hi_timer.h>
#include <hi_stdlib.h>
#endif
#include "flash_protect.h"
#include <hi_errno.h>

EXTERN_ROM_BSS_SECTION support_flash_protect g_support_flash_protect = {0};

#ifdef HI_FLASH_SUPPORT_FLASH_PROTECT

EXTERN_ROM_BSS_SECTION hi_flash_protect_ctrl g_protect_ctrl = {
    0,
};
EXTERN_ROM_BSS_SECTION hi_spi_flash_ctrl *g_flash_ctrl = HI_NULL;

/*
  * 1.不同芯片设置不同，更换flash芯片务必重新确认表格；
  * 2.bit设置参见Flash 手册memory protection章节，本表格参考W25Q16JL/W25Q16JW/GD25LE16/GD25WQ16/EN25S16
  * 3.根据实际空间可以精简或细化；
 */
HI_CONST hi_flash_protect_size g_flash_protect_size_upper[] = { /* 高地址保护 */
    /* { 0b111100, 0x8, 0 },  32K boot:保护地址0x8000-0x1fffff,并将PROTECT_FLASH_ALL宏修改为0x3C */
    { 0b101001, 0x10, 0 }, /* 64K boot:保护地址0x10000-0x1fffff */
    { 0b101100, 0x80, 0 }, /* 保护地址0x80000-0x1fffff */
    { 0b000101, 0x100, 0 }, /* 高地址保护1024KB */
    { 0b000100, 0x180, 0 }, /* 高地址保护512KB */
    { 0b000001, 0x1F0, 0 }, /* 高地址保护64KB */
    { 0b010100, 0x1F8,  0 },  /* 高地址保护32KB */
    { 0b000000, 0x200,  0 },  /* 不保护 */
};

BSP_RAM_TEXT_SECTION hi_u32 spi_flash_write_sr_reg(hi_u8 cmd, hi_u8* p_data, hi_u8 data_len, hi_bool is_volatile)
{
    hi_u32 temp_data = 0;
    hi_u32 ret;
    if (data_len > 0) {
        ret = (hi_u32)memcpy_s(&temp_data, sizeof(temp_data), p_data, data_len); /* 4 */
        if (ret != EOK) {
            return ret;
        }
    }
    if (is_volatile) {
        hisfc_write(SFC_REG_CMD_INS, SPI_CMD_VSR_WREN);
        hisfc_write(SFC_REG_CMD_CONFIG, (hi_u32)(SFC_CMD_CONFIG_SEL_CS | SFC_CMD_CONFIG_START));
        spif_wait_config_start();
    } else {
        ret = spif_write_enable(HI_TRUE);
        if (ret != HI_ERR_SUCCESS) {
            return ret;
        }
    }
    hisfc_write(SFC_REG_CMD_INS, cmd);
    hisfc_write(SFC_REG_CMD_DATABUF1, temp_data);
    hisfc_write(SFC_REG_CMD_CONFIG,
                SFC_CMD_CONFIG_SEL_CS
                | SFC_CMD_CONFIG_DATA_EN
                | sfc_cmd_config_data_cnt(data_len)
                | SFC_CMD_CONFIG_START);
    spif_wait_config_start();
    return HI_ERR_SUCCESS;
}

BSP_RAM_TEXT_SECTION hi_u32 flash_protect_set_protect(hi_u8 cmp_bp, hi_bool is_volatile)
{
    hi_u32 ret;
    hi_u8 p_data[2] = {0}; /* 2 */
    hi_u8 cmp = (cmp_bp >> 5) & 0x1; /* 5 */
    hi_u8 bp = cmp_bp & 0x1F;
    hi_flash_protect_ctrl *p_ctrl = &g_protect_ctrl;
    if (p_ctrl->enable == HI_FALSE) {
        return HI_ERR_SUCCESS; /* 未使能也返回成功 */
    }
    ret = spif_wait_ready(HI_TRUE, SPI_CMD_SR_WIPN, SPI_SR_BIT_WIP);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR, &p_data[0], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR2, &p_data[1], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    if (((p_data[0] & (0x1F << 2)) == (bp << 2)) && ((p_data[1] & (0x1 << 6)) == (cmp << 6))) { /* 2 6 */
        return HI_ERR_SUCCESS;
    }
    p_data[0] &= ~(0x1f<<2);      /* 2 */
    p_data[0] |= (hi_u8)(bp<<2);  /* 2 */
    p_data[1] &= ~(0x1<<6);       /* 6 */
    p_data[1] |= (hi_u8)(cmp<<6); /* 6 */
    ret = spi_flash_write_sr_reg(SPI_CMD_WRSR1, p_data, 2, is_volatile); /* 2 : p_data len */
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spif_wait_ready(HI_TRUE, SPI_CMD_SR_WIPN, SPI_SR_BIT_WIP); /* wait flash WIP is zero */
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR, &p_data[0], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR2, &p_data[1], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    if (((p_data[0] & (0x1F<<2)) == (bp<<2)) && ((p_data[1] & (0x1<<6)) == (cmp<<6))) { /* 2 6 */
        return HI_ERR_SUCCESS;
    } else {
        return p_data[0] | (p_data[1] << 8); /* 8 */
    }
}

static hi_u32 get_timer_val(hi_flash_protect_type type)
{
    if (type == HI_FLASH_PROTECT_TYPE_1) {
        return PROTECT_TIMEOUT_1;
    } else if (type == HI_FLASH_PROTECT_TYPE_2) {
        return PROTECT_TIMEOUT_2;
    } else {
        return 0;
    }
}

hi_void flash_protect_timeout(hi_u32 is_volatile)
{
    hi_flash_protect_ctrl *p_ctrl = &g_protect_ctrl;
    support_flash_protect *fp = &g_support_flash_protect;
    hi_u32 int_value = hi_int_lock();
    p_ctrl->is_volatile = (hi_bool)is_volatile;
    fp->protect_all = HI_TRUE;
    hi_int_restore(int_value);
}

hi_u32 flash_protect_all_area(hi_void)
{
    hi_u32 ret = HI_ERR_SUCCESS;
    hi_flash_protect_ctrl *p_ctrl = &g_protect_ctrl;
    support_flash_protect *fp = &g_support_flash_protect;
    if ((!hi_is_int_context()) && (get_flash_op_during_flash() != HI_TRUE)) {
        ret = hi_mux_pend(g_flash_ctrl->mutex_handle, 0);
    }
    if (ret != HI_ERR_SUCCESS) {
        hi_timer_start(p_ctrl->timer_handle, HI_TIMER_TYPE_ONCE, p_ctrl->timer_timeout, flash_protect_timeout,
            (hi_u32)p_ctrl->is_volatile);
        fp->protect_all = HI_FALSE;
        return ret;
    }
    p_ctrl->current_block = 0;
    /* 如果全保护失败， p_ctrl->current_block已经设置为全保护状态地址，不影响下次擦写操作 */
    ret = flash_protect_set_protect(PROTECT_FLASH_ALL, p_ctrl->is_volatile);
    if (ret != HI_ERR_SUCCESS) {
        hi_timer_start(p_ctrl->timer_handle, HI_TIMER_TYPE_ONCE, p_ctrl->timer_timeout, flash_protect_timeout,
            (hi_u32)p_ctrl->is_volatile);
    }
    fp->protect_all = HI_FALSE;
    hi_u32 unlock_ret = flash_unlock();
    if (ret == HI_ERR_SUCCESS) {
        ret = unlock_ret;
    }

    return ret;
}

hi_u32 flash_protect_prv(hi_u32 flash_offset, hi_u32 size, hi_bool is_volatile)
{
    hi_u32 ret;
    hi_s8 i;
    hi_flash_protect_ctrl *p_ctrl = &g_protect_ctrl;
    hi_u32 block;
    block = (flash_offset + size - 1) / PRODUCT_CFG_FLASH_BLOCK_SIZE; /* unlock protect areas */
    if (block < p_ctrl->current_block) {
     /* 要擦/写地址已经解开保护 */
        return HI_ERR_SUCCESS;
    }
    for (i = 0; i < (hi_s8)(sizeof(g_flash_protect_size_upper) / sizeof(hi_flash_protect_size)); i++) {
        if (block < g_flash_protect_size_upper[i].block) {
            break;
        }
    }
    if (i < (hi_s8)(sizeof(g_flash_protect_size_upper) / sizeof(hi_flash_protect_size))) {
        ret = flash_protect_set_protect(g_flash_protect_size_upper[i].cmp_bp, is_volatile);
    } else {
        ret = flash_protect_set_protect(0, is_volatile);
    }
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    if (i < (hi_s8)(sizeof(g_flash_protect_size_upper) / sizeof(hi_flash_protect_size))) {
        p_ctrl->current_block = g_flash_protect_size_upper[i].block;
    } else {
        p_ctrl->current_block = block;
    }
    return ret;
}

hi_u32 flash_protect(hi_u32 flash_offset, hi_u32 size, hi_u32 timeout, hi_bool is_volatile)
{
    hi_u32 ret;
    hi_flash_protect_ctrl *p_ctrl = &g_protect_ctrl;
    if (p_ctrl->enable == HI_FALSE) {
        return HI_ERR_SUCCESS; /* 未使能也返回成功 */
    }
    ret = flash_protect_prv(flash_offset, size, is_volatile);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    if (timeout) {
        if (timeout == PROTECT_TIMEOUT_AUTO) {
            p_ctrl->timer_timeout = get_timer_val((hi_flash_protect_type)(p_ctrl->default_type));
        } else {
            p_ctrl->timer_timeout = timeout;
        }
        ret = hi_timer_start(p_ctrl->timer_handle, HI_TIMER_TYPE_ONCE, p_ctrl->timer_timeout, flash_protect_timeout,
            (hi_u32)is_volatile);
    } else {
        ret = hi_timer_stop(p_ctrl->timer_handle);
    }
    return ret;
}
#endif

support_flash_protect *flash_get_support_flash_protect_info(hi_void)
{
    return &g_support_flash_protect;
}
/* hi_flash_protect_init 内部使用函数 */
hi_u32 flash_protect_init_cfg(hi_flash_protect_type type)
{
#ifdef HI_FLASH_SUPPORT_FLASH_PROTECT
    hi_flash_protect_ctrl *p_ctrl = &g_protect_ctrl;
    support_flash_protect *fp = &g_support_flash_protect;
    hi_u32 ret;
    ret = flash_lock();
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    p_ctrl->default_type = type;
    p_ctrl->init = HI_TRUE;
    p_ctrl->enable = HI_TRUE;

    ret = flash_protect_set_protect(PROTECT_FLASH_ALL, HI_FALSE); /* protect all flash chip */
    if (ret != HI_ERR_SUCCESS) {
        flash_unlock();
        return ret;
    }

    fp->protect_all = HI_FALSE;
    fp->protect_all_area = flash_protect_all_area;
    fp->support_flash_protect = HI_TRUE;
    p_ctrl->current_block = 0;
    hi_u32 unlock_ret = flash_unlock();
    if (ret == HI_ERR_SUCCESS) {
        ret = unlock_ret;
    }
    return ret;
#else
    hi_unref_param(type);
    return HI_ERR_SUCCESS;
#endif
}

const hi_u8 g_unknown_chip[] = "UNKNOWN";
hi_u32 hi_flash_protect_init(hi_flash_protect_type type)
{
    support_flash_protect *fp = &g_support_flash_protect;
#ifdef HI_FLASH_SUPPORT_FLASH_PROTECT
    hi_u32 ret;
    hi_flash_protect_ctrl *p_ctrl = &g_protect_ctrl;
    g_flash_ctrl = flash_get_spi_flash_ctrl_info();
    if (p_ctrl->init == HI_TRUE) {
        return HI_ERR_FLASH_PROTECT_RE_INIT;
    }
    if (type >= HI_FLASH_PROTECT_TYPE_MAX) {
        return HI_ERR_FLASH_INVALID_PARAM;
    }
    if (type == HI_FLASH_PROTECT_NONE) {
        return HI_ERR_FLASH_PROTECT_NOT_SUPPORT;
    }
    if (g_flash_ctrl->basic_info.chip_name == HI_NULL) {
        return HI_ERR_FLASH_NOT_INIT;
    }
    if (memcmp(g_flash_ctrl->basic_info.chip_name, g_unknown_chip, sizeof(g_unknown_chip)) == 0) {
        return HI_ERR_FLASH_PROTECT_NOT_FIND_CHIP;
    }
    ret = hi_timer_create(&(p_ctrl->timer_handle));
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = flash_protect_init_cfg(type);
    if (ret != HI_ERR_SUCCESS) {
        goto end;
    }
    return ret;
end:
    hi_timer_delete(p_ctrl->timer_handle);
    fp->protect_all_area = HI_NULL;
    p_ctrl->enable = HI_FALSE;
    return ret;
#else
    hi_unref_param(type);
    fp->protect_all_area = HI_NULL;
    return HI_ERR_SUCCESS;
#endif
}

hi_u32 hi_flash_protect_deinit(hi_void)
{
#ifdef HI_FLASH_SUPPORT_FLASH_PROTECT
    /* full-chip volatile protection, resources are not released */
    hi_u32 ret = flash_protect_set_protect(PROTECT_FLASH_ALL, HI_TRUE);
    hi_flash_protect_ctrl *p_ctrl = &g_protect_ctrl;
    support_flash_protect *fp = &g_support_flash_protect;
    if (fp->protect_all) {
        fp->protect_all = HI_FALSE;
    }
    p_ctrl->current_block = 0;
    return ret;
#endif

    return HI_ERR_SUCCESS;
}
