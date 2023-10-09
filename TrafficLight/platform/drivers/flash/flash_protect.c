/*
 * Copyright (c) 2020 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "flash_protect.h"

#include <hi_early_debug.h>
#include <hi_errno.h>
#include <hi_stdlib.h>

#include "flash_prv.h"
#include "hi_task.h"
#include <hi_time.h>

#ifdef HI_FLASH_SUPPORT_FLASH_PROTECT
#include <hi_timer.h>
#include <hi_sal.h>
#include <hi_sem.h>
#endif

EXTERN_ROM_BSS_SECTION support_flash_protect g_support_flash_protect = {0};

EXTERN_ROM_BSS_SECTION static hi_flash_protect_ctrl g_protect_ctrl = {
    0,
};
EXTERN_ROM_BSS_SECTION static hi_spi_flash_ctrl *g_flash_ctrl = HI_NULL;

/*
 * 1. Different chips have different settings. Before replacing a flash chip, check the table again.
 * 2. For details about the bit setting, see section "memory protection" in the 
      peripheral(W25Q16JL/W25Q16JW/GD25LE16/GD25WQ16/EN25S16) specifications.
 * 3. Simplify or refine based on the actual space.
 */
HI_CONST hi_flash_protect_size g_flash_protect_size_lower[] = {
    /* bit[5]:cmp
     * bit[4:0]:bp[4:0]:  For details, see peripheral specifications
     */
    { 0b000000, 0x0,  0 },  /* No protection */
    { 0b011100, 0x8,  0 },  /* Protect 32KB */
    { 0b001001, 0x10, 0 },  /* Protect 64KB */
    { 0b001100, 0x80, 0 },  /* Protect 512KB */
    { 0b001101, 0x100, 0 }, /* Protect 1024KB */
};

hi_void flash_protect_lock(hi_void)
{
    hi_task_lock();
    disable_int_in_flash();
}

hi_void flash_protect_unlock(hi_void)
{
    enable_int_in_flash();
    hi_task_unlock();
}
BSP_RAM_TEXT_SECTION static hi_u32 spi_flash_write_sr_reg(hi_u8 cmd, hi_u8* p_data, hi_u8 data_len, hi_bool is_volatile)
{
    hi_u32 temp_data = 0;
    hi_u32 ret;
    if (p_data == NULL) {
        return HI_ERR_FAILURE;
    }
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

BSP_RAM_TEXT_SECTION hi_u32 wait_ready_read_regs(hi_u8 *low_reg, hi_u8 *high_reg)
{
    if ((low_reg == HI_NULL) || (high_reg == HI_NULL)) {
        return HI_ERR_FAILURE;
    }
    hi_u32 ret = spif_wait_ready(HI_TRUE, SPI_CMD_SR_WIPN, SPI_SR_BIT_WIP);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR, low_reg, 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR2, high_reg, 1);
    return ret;
}

BSP_RAM_TEXT_SECTION hi_u32 flash_protect_read_check_reg(hi_u8 *reg_value, hi_u8 reg_size)
{
    hi_u8 first_reg0;
    hi_u8 first_reg1;
    hi_u8 second_reg0;
    hi_u8 second_reg1;
    hi_u32 ret;
    if ((reg_value == NULL) || (reg_size < 2)) { /* 2: reg0 reg1 */
        return HI_ERR_FLASH_INVALID_PARAM;
    }
    ret = wait_ready_read_regs(&first_reg0, &first_reg1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = wait_ready_read_regs(&second_reg0, &second_reg1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    if ((first_reg0 & WB_REG1_CHECK_MSK) != (second_reg0 & WB_REG1_CHECK_MSK)) {
        return HI_ERR_FLASH_CHECK_REG_WRONG;
    }
    if ((first_reg1 & WB_REG2_CHECK_MSK) != (second_reg1 & WB_REG2_CHECK_MSK)) {
        return HI_ERR_FLASH_CHECK_REG_WRONG;
    }
    *reg_value = first_reg0;
    *(reg_value + 1) = first_reg1;
    return ret;
}

BSP_RAM_TEXT_SECTION hi_u32 flash_protect_set_protect(hi_u8 cmp_bp, hi_bool is_volatile, hi_bool forced_write)
{
    hi_u8 p_data[2] = {0}; /* 2 register buffer */
    hi_u8 cmp = (cmp_bp >> 5) & 0x1; /* 5 */
    hi_u8 bp = cmp_bp & 0x1F;
    if (g_protect_ctrl.enable == HI_FALSE) {
        return HI_ERR_SUCCESS;
    }
    if ((g_flash_ctrl != NULL) && (g_protect_ctrl.need_check == FLASH_PROTECT_NEED_CHK)) {
        flash_wb_erase_recovery(g_flash_ctrl->basic_info.chip_id);
    }
    hi_u8 *p1 = &p_data[0];
    hi_u8 *p2 = &p_data[1];
    flash_protect_lock();
    hi_u32 ret = flash_protect_read_check_reg(p_data, sizeof(p_data));
    if (ret != HI_ERR_SUCCESS) {
        goto end;
    }
    if (forced_write == HI_FALSE) {
        if (((p_data[0] & (0x1F << 2)) == (bp << 2)) && ((p_data[1] & (0x1 << 6)) == (cmp << 6))) { /* 2 6 */
            ret = HI_ERR_SUCCESS;
            goto end;
        }
    }
    p_data[0] &= ~(0x1f<<2);      /* 2 */
    p_data[0] |= (hi_u8)(bp<<2);  /* 2 */
    p_data[0] &= ~(0x1 << WB_REG1_SRP_OFFSET);  /* srp need always 0 */
    p_data[1] &= ~(0x1 << WB_REG2_SRL_OFFSET);  /* srl need always 0 */
    p_data[1] &= ~(0x1<<6);       /* 6 */
    p_data[1] |= (hi_u8)(cmp<<6); /* 6 */
    ret = spi_flash_write_sr_reg(SPI_CMD_WRSR1, p_data, 2, is_volatile); /* 2 : p_data len */
    if (ret != HI_ERR_SUCCESS) {
        goto end;
    }
    ret = wait_ready_read_regs(p1, p2);
    if (ret != HI_ERR_SUCCESS) {
        goto end;
    }
    if (((p_data[0] & (0x1F<<2)) == (bp<<2)) && ((p_data[1] & (0x1<<6)) == (cmp<<6))) { /* 2 6 */
        ret = HI_ERR_SUCCESS;
    } else {
        ret =  p_data[0] | (p_data[1] << 8); /* 8 */
    }
end:
    flash_protect_unlock();
    return ret;
}


#ifdef HI_FLASH_SUPPORT_FLASH_PROTECT
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
    hi_u32 int_value = hi_int_lock();
    g_protect_ctrl.is_volatile = (hi_bool)is_volatile;
    g_support_flash_protect.protect_all = HI_TRUE;
    hi_sem_signal(g_flash_prot_sem);
    hi_int_restore(int_value);
}

hi_u32 flash_protect_all_area(hi_void)
{
    hi_u32 ret = HI_ERR_SUCCESS;
    hi_u32 timer_ret;
    g_support_flash_protect.protect_all = HI_FALSE;
    if (g_protect_ctrl.enable == HI_FALSE) {
        return HI_ERR_SUCCESS;
    }
    if ((!hi_is_int_context()) && (get_flash_op_during_flash() != HI_TRUE)) {
        ret = hi_mux_pend(g_flash_ctrl->mutex_handle, 0);
    }
    if (ret != HI_ERR_SUCCESS) {
        timer_ret = hi_timer_start(g_protect_ctrl.timer_handle, HI_TIMER_TYPE_ONCE, g_protect_ctrl.timer_timeout,
            flash_protect_timeout, (hi_u32)g_protect_ctrl.is_volatile);
        if (timer_ret != HI_ERR_SUCCESS) {
            g_protect_ctrl.current_block = g_flash_ctrl->chip_size / PRODUCT_CFG_FLASH_BLOCK_SIZE;
            g_support_flash_protect.protect_all = HI_TRUE;
        }
        return ret;
    }
    g_protect_ctrl.current_block = g_flash_ctrl->chip_size / PRODUCT_CFG_FLASH_BLOCK_SIZE;
    ret = flash_protect_set_protect(PROTECT_FLASH_ALL, g_protect_ctrl.is_volatile, HI_FALSE);
    if (ret != HI_ERR_SUCCESS) {
        timer_ret = hi_timer_start(g_protect_ctrl.timer_handle, HI_TIMER_TYPE_ONCE, g_protect_ctrl.timer_timeout,
            flash_protect_timeout, (hi_u32)g_protect_ctrl.is_volatile);
        if (timer_ret != HI_ERR_SUCCESS) {
            flash_info_print("protect all: start timer fail\r\n");
            g_support_flash_protect.protect_all = HI_TRUE;
        }
    }
    hi_u32 unlock_ret = flash_unlock();
    if (unlock_ret != HI_ERR_SUCCESS) {
        flash_info_print("protect all: flash unlock fail\r\n");
    }
    if (ret == HI_ERR_SUCCESS) {
        ret = unlock_ret;
    }

    return ret;
}

static hi_u32 flash_protect_prv(hi_u32 flash_offset, hi_u32 size, hi_bool is_volatile)
{
    hi_u32 ret;
    hi_s8 i;
    hi_u32 block = (flash_offset / PRODUCT_CFG_FLASH_BLOCK_SIZE);
    hi_unref_param(size);
    /* Kernel flash low address arotection policy. */
    if (block > g_protect_ctrl.current_block) {
        return HI_ERR_SUCCESS;
    }
    for (i = (sizeof(g_flash_protect_size_lower) / sizeof(hi_flash_protect_size)) - 1; i >= 0; i--) {
        if (block >= g_flash_protect_size_lower[i].block) {
            break;
        }
    }
    if (i >= 0) {
        ret = flash_protect_set_protect(g_flash_protect_size_lower[i].cmp_bp, is_volatile, HI_FALSE);
    } else {
        ret = flash_protect_set_protect(0, is_volatile, HI_FALSE);
    }
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    g_protect_ctrl.current_block = (i >= 0) ? g_flash_protect_size_lower[i].block : block;
    return ret;
}

static hi_u32 check_addr_valid(hi_u32 flash_offset, hi_u32 flash_offset_end, hi_u32 flash_code_start,
    hi_u32 flash_code_end)
{
    if ((flash_code_start < HI_FLASH_BASE) || (flash_code_end < HI_FLASH_BASE)) {
        return HI_ERR_FLASH_PROTECT_ADDR_WRONG;
    }
    flash_code_start -= HI_FLASH_BASE;
    flash_code_end -= HI_FLASH_BASE;
    if ((flash_offset < flash_code_end) && (flash_offset_end > flash_code_start)) {
        return HI_ERR_FLASH_PROTECT_ADDR_WRONG;
    }
    return HI_ERR_SUCCESS;
}

#if defined(CONFIG_FLASH_ENCRYPT_SUPPORT)
static hi_u32 check_encrypt_addr_valid(hi_u32 flash_offset, hi_u32 flash_offset_end, hi_u32 flash_code_start,
    hi_u32 flash_code_end)
{
    if ((flash_code_start < HI_FLASH_BASE) || (flash_code_end < HI_FLASH_BASE)) {
        return HI_ERR_FLASH_PROTECT_ADDR_WRONG;
    }
    flash_code_start -= HI_FLASH_BASE;
    flash_code_start -= KERNEL_HEAD;
    flash_code_end -= HI_FLASH_BASE;
    if ((flash_offset < flash_code_end) && (flash_offset_end > flash_code_start)) {
        return HI_ERR_FLASH_PROTECT_ADDR_WRONG;
    }
    return HI_ERR_SUCCESS;
}
#endif

static hi_u32 flash_protect_check_addr(hi_u32 flash_offset, hi_u32 size)
{
    hi_u32 flash_code_start = (hi_u32)&__text_cache_start2_;
    hi_u32 flash_code_end = (hi_u32)&__text_rodata_end_;
    hi_u32 flash_offset_end = flash_offset + size;
    hi_u32 ret;
    if ((flash_code_start < HI_FLASH_BASE) || (flash_code_end < HI_FLASH_BASE)) {
        return HI_ERR_FLASH_PROTECT_ADDR_WRONG;
    }
    flash_code_start -= HI_FLASH_BASE;
#ifndef CONFIG_FLASH_ENCRYPT_SUPPORT
    flash_code_start -= KERNEL_HEAD;
#endif
    flash_code_end -= HI_FLASH_BASE;
    if ((flash_offset < flash_code_end) && (flash_offset_end > flash_code_start)) {
        return HI_ERR_FLASH_PROTECT_ADDR_WRONG;
    }

    flash_code_start = (hi_u32)&__ram_text_load;
    flash_code_end = flash_code_start + ((hi_u32)&__ram_text_size);
    ret = check_addr_valid(flash_offset, flash_offset_end, flash_code_start, flash_code_end);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    flash_code_start = (hi_u32)&__data_load;
    flash_code_end = flash_code_start + ((hi_u32)&__data_size);
    ret = check_addr_valid(flash_offset, flash_offset_end, flash_code_start, flash_code_end);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

#if defined(CONFIG_FLASH_ENCRYPT_SUPPORT)
    flash_code_start = (hi_u32)&__crypto_ram_text_load;
    flash_code_end = flash_code_start + ((hi_u32)&__crypto_ram_text_size);
    ret = check_encrypt_addr_valid(flash_offset, flash_offset_end, flash_code_start, flash_code_end);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
#endif
    return HI_ERR_SUCCESS;
}

hi_u32 flash_protect(hi_u32 flash_offset, hi_u32 size, hi_u32 timeout, hi_bool is_volatile)
{
    hi_u32 ret;
    hi_flash_protect_ctrl *p_ctrl = &g_protect_ctrl;
    if (p_ctrl->enable == HI_FALSE) {
        return HI_ERR_SUCCESS;
    }
    /* code in flash detect */
    ret = flash_protect_check_addr(flash_offset, size);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
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

BSP_RAM_TEXT_SECTION hi_u32 flash_wb_read_reg3(hi_u8 *reg_val)
{
    hi_u32 ret;
    hi_u8 reg_first;
    hi_u8 reg_sencod;
    hi_u8 reg_third;
    if (reg_val == NULL) {
        return HI_ERR_FLASH_INVALID_PARAM;
    }
    ret = spif_wait_ready(HI_TRUE, SPI_CMD_SR_WIPN, SPI_SR_BIT_WIP); /* wait until flash is idle */
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR3, &reg_first, 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    hi_udelay(READ_REG_DELAY_10US);
    ret = spi_flash_read_reg(SPI_CMD_RDSR3, &reg_sencod, 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    hi_udelay(READ_REG_DELAY_20US);
    ret = spi_flash_read_reg(SPI_CMD_RDSR3, &reg_third, 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    if (((reg_first & WB_CHECK_REG3_MSK) != (reg_sencod & WB_CHECK_REG3_MSK)) ||
        ((reg_first & WB_CHECK_REG3_MSK) != (reg_third & WB_CHECK_REG3_MSK))) {
        return HI_ERR_FLASH_CHECK_REG_WRONG;
    }
    *reg_val = reg_first;
    return ret;
}

/* just for wb flash, Enable the S19 function to prevent flash erasing and writing exceptions caused by sudden
   power-off. */
BSP_RAM_TEXT_SECTION hi_void flash_wb_erase_recovery(const hi_u8 *chip_id)
{
    hi_u8 reg_val;
    hi_u8 check_times;
    hi_u32 ret;
    hi_u32 delay_time = FIRST_DELAY_US;
    if (chip_id == HI_NULL) {
        return;
    }
    /* if use another wb flash ID, set the parameters according flash manual */
    if ((memcmp(chip_id, g_flash_default_info_tbl[1].chip_id, HI_FLASH_CHIP_ID_NUM) != 0) && /* W25Q16JW */
        (memcmp(chip_id, g_flash_default_info_tbl[2].chip_id, HI_FLASH_CHIP_ID_NUM) != 0)) { /* 2:W25Q16JL */
        /* the flash id not support s19 function */
        return;
    }
    flash_protect_lock();

    for (check_times = 0; check_times < MAX_CHECK_REG_TIMES; check_times++) {
        delay_time += (check_times == 0) ? 0 : NEXT_DELAY_US;
        ret = flash_wb_read_reg3(&reg_val);
        if (ret != HI_ERR_SUCCESS) {
            hi_udelay(delay_time);
            continue;
        }
        /* check hold , wps is 0, s19 is 1  */
        if ((((reg_val >> WB_REG3_HOLD_OFFSET) & 0x1) == 0) && (((reg_val >> WB_REG3_WPS_OFFSET) & 0x1) == 0) &&
            (((reg_val >> WB_REG3_S19_OFFSET) & 0x1) == 0x1)) {
            g_protect_ctrl.need_check = FLASH_PROTECT_NOT_CHK;
            goto end;
        }
        reg_val &= ~((0x1 << WB_REG3_HOLD_OFFSET) | (0x1 << WB_REG3_WPS_OFFSET));
        reg_val |= (0x1 << WB_REG3_S19_OFFSET);
        ret = spi_flash_write_sr_reg(SPI_CMD_WRSR3, &reg_val, 1, HI_FALSE);
        if (ret != HI_ERR_SUCCESS) {
            hi_udelay(delay_time);
            continue;
        }
        ret = flash_wb_read_reg3(&reg_val);
        if (ret != HI_ERR_SUCCESS) {
            hi_udelay(delay_time);
            continue;
        }
        /* check hold , wps is 0, s19 is 1  */
        if ((((reg_val >> WB_REG3_HOLD_OFFSET) & 0x1) == 0) && (((reg_val >> WB_REG3_WPS_OFFSET) & 0x1) == 0) &&
            (((reg_val >> WB_REG3_S19_OFFSET) & 0x1) == 0x1)) {
            g_protect_ctrl.need_check = FLASH_PROTECT_NOT_CHK;
            goto end;
        }
        hi_udelay(delay_time);
    }
    g_protect_ctrl.need_check = FLASH_PROTECT_NEED_CHK;
end:
    flash_protect_unlock();
}

support_flash_protect *flash_get_support_flash_protect_info(hi_void)
{
    return &g_support_flash_protect;
}

hi_u32 hi_flash_protect_enable(hi_bool enable)
{
    hi_u32 ret = HI_ERR_SUCCESS;
#ifdef HI_FLASH_SUPPORT_FLASH_PROTECT
    if (g_protect_ctrl.init == HI_FALSE || g_protect_ctrl.enable == enable) {
        return ret;
    }
    ret = flash_lock();
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    if (enable == HI_TRUE) {
        g_protect_ctrl.enable = HI_TRUE;
        ret = flash_protect_set_protect(PROTECT_FLASH_ALL, HI_FALSE, HI_FALSE);
        if (ret != HI_ERR_SUCCESS) {
            printf("set protect fail:%x \n", ret);
            g_protect_ctrl.enable = HI_FALSE;
            flash_unlock();
            return ret;
        }
        g_support_flash_protect.protect_all = HI_FALSE;
        g_support_flash_protect.protect_all_area = flash_protect_all_area;
        g_support_flash_protect.support_flash_protect = HI_TRUE;
        g_protect_ctrl.current_block = g_flash_ctrl->chip_size / PRODUCT_CFG_FLASH_BLOCK_SIZE;
    } else {
        g_support_flash_protect.protect_all_area = HI_NULL;
        if (hi_timer_stop(g_protect_ctrl.timer_handle) != HI_ERR_SUCCESS) {
            flash_info_print("stop timer fail\r\n");
        }
        ret = flash_protect_set_protect(0x0, HI_FALSE, HI_TRUE);
        if (ret != HI_ERR_SUCCESS) {
            printf("non protect fail:%x \n", ret);
            flash_unlock();
            return ret;
        }
        g_protect_ctrl.enable = HI_FALSE;
    }
    ret = flash_unlock();
#else
    hi_unref_param(enable);
#endif
    return ret;
}

#ifdef HI_FLASH_SUPPORT_FLASH_PROTECT
static hi_u32 flash_protect_init_cfg(hi_void)
{
    hi_u32 ret;
    ret = hi_timer_create(&(g_protect_ctrl.timer_handle));
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = flash_lock();
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    g_protect_ctrl.enable = HI_TRUE;
    ret = flash_protect_set_protect(PROTECT_FLASH_ALL, HI_FALSE, HI_FALSE); /* protect all flash chip */
    if (ret != HI_ERR_SUCCESS) {
        g_protect_ctrl.enable = HI_FALSE;
        flash_unlock();
        goto end;
    }
    g_support_flash_protect.protect_all = HI_FALSE;
    g_support_flash_protect.protect_all_area = flash_protect_all_area;
    g_support_flash_protect.support_flash_protect = HI_TRUE;
    g_protect_ctrl.current_block = g_flash_ctrl->chip_size / PRODUCT_CFG_FLASH_BLOCK_SIZE;
    g_protect_ctrl.init = HI_TRUE;
    ret = flash_unlock();
    return ret;
end:
    if (hi_timer_delete(g_protect_ctrl.timer_handle) != HI_ERR_SUCCESS) {
        flash_info_print("protect init: delete timer fail\r\n");
    }
    g_protect_ctrl.timer_handle = 0xFFFFFFFF;
    g_support_flash_protect.protect_all_area = HI_NULL;
    g_protect_ctrl.enable = HI_FALSE;
    g_protect_ctrl.init = HI_FALSE;
    return ret;
}
#else
static hi_u32 flash_protect_init_cfg(hi_void)
{
    hi_u32 ret;
    ret = flash_lock();
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    g_protect_ctrl.enable = HI_TRUE;
    ret = flash_protect_set_protect(0x0, HI_FALSE, HI_FALSE);
    if (ret != HI_ERR_SUCCESS) {
        g_protect_ctrl.enable = HI_FALSE;
        flash_unlock();
        return ret;
    }
    ret = flash_unlock();
    g_protect_ctrl.enable = HI_FALSE;
    g_support_flash_protect.protect_all_area = HI_NULL;
    g_protect_ctrl.init = HI_TRUE;
    return ret;
}
#endif

BSP_RAM_TEXT_SECTION hi_void cmu_err_isr(hi_u32 data)
{
    hi_u32 ret;
    hi_u32 freq;
    ret = hi_get_lowspeed_clk_freq(&freq);
    if ((ret != HI_ERR_SUCCESS) || (freq == 0)) {
        freq = HI3861_32K_DEFAULT_FREQ;
    }
    hi_u16 reg_val = (SOFT_RST_GLB_PILSE_MS * freq) / MS_PER_S;
    hi_reg_write16(GLB_CTL_SOFT_GLB_RST_REG, reg_val);
    __asm__ __volatile__("fence":::"memory");
    hi_reg_clrbit16(GLB_CTL_AON_SOFT_RST_W_REG, SOFT_RST_GLB_POS);
    hi_reg_clrbit16(PMU_CMU_CTL_PMU_STATUS_GRM_STICK_CLR_REG, UVLO_GRM_LSICK_CLR);
    while (1) {}
    hi_unref_param(data);
}

hi_void flash_protect_cmu_err_init(hi_void)
{
    hi_u32 ret;
    hi_u16 reg_val;
    hi_reg_read16(PMU_CMU_CTL_PMU_STATUS_GRM_TIME_REG, reg_val);
    reg_val &= ~(0xF);
    reg_val |= 0x1;      /* pmu 30us */
    hi_reg_write16(PMU_CMU_CTL_PMU_STATUS_GRM_TIME_REG, reg_val);

    hi_reg_read16(PMU_CMU_CTL_VBAT_TH_CFG_REG, reg_val);
    reg_val &= ~(0x3);
    reg_val |= 0x2;      /* VBAT 2.1V */
    hi_reg_write16(PMU_CMU_CTL_VBAT_TH_CFG_REG, reg_val);
    ret = hi_irq_request(PMU_CMU_ERR_IRQ, HI_IRQ_FLAG_PRI1 | HI_IRQ_FLAG_NOT_IN_FLASH, (irq_routine)cmu_err_isr, 0);
    if (ret != HI_ERR_SUCCESS) {
        printf("request cmu irq fail:%x\n", ret);
        return;
    }
    hi_reg_setbit16(PMU_CMU_CTL_PMU_STATUS_GRM_INT_EN_REG, UVLO_GRM_INT_EN);
}

hi_u32 hi_flash_protect_init(hi_flash_protect_type type)
{
    hi_u32 ret;
    const hi_u8 unknown_chip[] = "UNKNOWN";
    if (type >= HI_FLASH_PROTECT_NONE) {
        return HI_ERR_FLASH_PROTECT_NOT_SUPPORT;
    }
    if (g_protect_ctrl.init == HI_TRUE) {
        return HI_ERR_FLASH_PROTECT_RE_INIT;
    }
    g_protect_ctrl.need_check = FLASH_PROTECT_NOT_CHK;
    g_protect_ctrl.timer_handle = 0xFFFFFFFF;
    g_protect_ctrl.default_type = type;
    g_flash_ctrl = flash_get_spi_flash_ctrl_info();
    if ((g_flash_ctrl == HI_NULL) || (g_flash_ctrl->basic_info.chip_name == HI_NULL)) {
        return HI_ERR_FLASH_NOT_INIT;
    }
    if (memcmp(g_flash_ctrl->basic_info.chip_name, unknown_chip, sizeof(unknown_chip)) == 0) {
        return HI_ERR_FLASH_PROTECT_NOT_FIND_CHIP;
    }
    flash_protect_cmu_err_init();
    ret = flash_protect_init_cfg();
    /* if the slef-recovery function of the wb flash is not required, please comment out the flash_wb_erase_recovery
        function */
    flash_wb_erase_recovery(g_flash_ctrl->basic_info.chip_id);
    return ret;
}

hi_u32 hi_flash_protect_deinit(hi_void)
{
#ifdef HI_FLASH_SUPPORT_FLASH_PROTECT
    if (g_protect_ctrl.init == HI_FALSE || g_protect_ctrl.enable == HI_FALSE) {
        return HI_ERR_SUCCESS;
    }
    /* full-chip volatile protection, resources are not released */
    hi_u32 ret = flash_protect_set_protect(PROTECT_FLASH_ALL, HI_TRUE, HI_FALSE);
    if (g_support_flash_protect.protect_all) {
        g_support_flash_protect.protect_all = HI_FALSE;
    }
    g_protect_ctrl.current_block = g_flash_ctrl->chip_size / PRODUCT_CFG_FLASH_BLOCK_SIZE;
    return ret;
#endif

    return HI_ERR_SUCCESS;
}
