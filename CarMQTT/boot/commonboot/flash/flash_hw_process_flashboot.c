/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: spi flash contraller hardware driver.
 * Author: Hisilicon
 * Create: 2012-12-22
 */
#include <boot_rom.h>

/* QE for MX */
hi_u32 spi_flash_enable_quad_mode_mx(hi_void)
{
    hi_u32 ret;
    hi_u8 data = 0;

    ret = spi_flash_read_reg(SPI_CMD_RDSR, &data, 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    /* QE bit is enable already, do nothing. */
    if (data & SPI_QE_EN_MX) {
        return HI_ERR_SUCCESS;
    }

    data |= SPI_QE_EN_MX;
    ret = spi_flash_write_reg(SPI_CMD_WRSRCR, &data, 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    ret = spi_flash_read_reg(SPI_CMD_RDSR, &data, 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    /* QE bit is enable already, do nothing */
    if (data & SPI_QE_EN_MX) {
        return HI_ERR_SUCCESS;
    }

    return HI_ERR_FLASH_QUAD_MODE_COMPARE_REG;
}


/* QE for W25Q */
hi_u32 spi_flash_enable_quad_mode(hi_void)
{
    hi_u32 ret;
    hi_u8 data[2] = { 0 }; /* flash has 2 RDSR reg */

    ret = spi_flash_read_reg(SPI_CMD_RDSR2, &data[1], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    /* QE bit is enable already, do nothing */
    if (data[1] & SPI_QE_EN) {
        return HI_ERR_SUCCESS;
    }

    ret = spi_flash_read_reg(SPI_CMD_RDSR, &data[0], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    /* SPI_CMD_WRSR1¼æÈÝÀÏ¿îFlash */
    data[1] |= SPI_QE_EN;
    ret = spi_flash_write_reg(SPI_CMD_WRSR1, data, 2); /* flash has 2 RDSR reg */
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    ret = spi_flash_read_reg(SPI_CMD_RDSR2, &data[1], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    if (data[1] & SPI_QE_EN) {
        return HI_ERR_SUCCESS;
    }

    return HI_ERR_FLASH_QUAD_MODE_COMPARE_REG;
}

hi_u32 spi_flash_configure_driver_strength(hi_flash_drv_strength drv_strength)
{
    hi_u8 data = 0;
    hi_u32 ret;

    if (drv_strength >= SPI_SR3_DRV_MAX) {
        return HI_ERR_FLASH_INVALID_PARAMETER;
    }

    ret = spi_flash_read_reg(SPI_CMD_RDSR3, &data, 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    if (((hi_u32)drv_strength << 5) == (data & (SPI_SR3_DRV_MASK << 5))) { /* left shift 5bit */
        return HI_ERR_SUCCESS;
    }

    data &= ~(SPI_SR3_DRV_MASK << 5); /* left shift 5bit */
    data |= (hi_u32)drv_strength << 5; /* left shift 5bit */
    ret = spi_flash_write_reg(SPI_CMD_WRSR3, &data, 1);

    return ret;
}
