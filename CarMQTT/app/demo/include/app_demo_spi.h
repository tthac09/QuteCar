/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:example of using SPI APIs to pair SPI devices.
 * Author: Hisilicon
 * Create: 2019-11-11
 */

#ifndef __APP_DEMO_SPI_H__
#define __APP_DEMO_SPI_H__

#include <hi_types_base.h>

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <hi3861_platform.h>
#include <hi_task.h>
#include <hi_stdlib.h>
#include <hi_early_debug.h>
#include <hi_time.h>
#include <hi_watchdog.h>
#include <hi_io.h>
#include <hi_spi.h>
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define BUF_LENGTH       512
#define BUF_LENGTH_HALF (BUF_LENGTH >> 1)
#define TEST_LOOP_LENGTH 256
#define test_spi_printf(fmt...)     \
    do {                            \
        printf("[SPI TEST]" fmt); \
        printf("\n");     \
    } while (0)

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
typedef struct {
    hi_spi_cfg_basic_info cfg_info;
    hi_spi_idx spi_id;
    hi_u32 loop;
    hi_u32 length;
    hi_bool irq;
    hi_bool slave;
    hi_bool lb;
    hi_bool dma_en;
} test_spi_para;

typedef enum {
    TEST_CASE_ALL,
    TEST_CASE_POL0_PHA0 = 1,
    TEST_CASE_POL0_PHA1,
    TEST_CASE_POL1_PHA0,
    TEST_CASE_POL1_PHA1,
    TEST_CASE_MOTOROLA,
    TEST_CASE_TI,
    /* TEST_CASE_MICROWIRE 不支持自回环测试 */
    TEST_CASE_BIT4,
    TEST_CASE_BIT7,
    TEST_CASE_BIT8,
    TEST_CASE_BIT9 = 10,
    TEST_CASE_BIT15,
    TEST_CASE_BIT16,
    TEST_CASE_CLK_MIN,
    TEST_CASE_CLK_16,
    TEST_CASE_CLK_50,
    TEST_CASE_CLK_100,
    TEST_CASE_CLK_200,
    TEST_CASE_CLK_MAX,
    TEST_CASE_PARAMETER_WRONG,
    TEST_CASE_SLAVE = 20,
    TEST_CASE_MASTER,
    TEST_CASE_MAX,
} hi_spi_test_case;
/*****************************************************************************
  10 函数声明
*****************************************************************************/
hi_u32 app_demo_spi_para_test(hi_spi_cfg_basic_info *spi_para);
hi_u32 app_demo_spi_test_case(hi_u8 test_case);
hi_void app_demo_spi_test(hi_spi_idx spi_id, hi_u32 irq_en, hi_u32 test_case, hi_u32 loop);
hi_void app_demo_spi_cmd_host_read(hi_spi_idx spi_id, hi_u32 length, hi_u32 loop);
hi_void app_demo_spi_cmd_slave_write(hi_spi_idx spi_id, hi_u32 length, hi_u32 loop);
hi_void app_demo_spi_cmd_host_write(hi_spi_idx spi_id, hi_u32 length, hi_u32 loop);
hi_void app_demo_spi_cmd_slave_read(hi_spi_idx spi_id, hi_u32 length, hi_u32 loop);
#endif
