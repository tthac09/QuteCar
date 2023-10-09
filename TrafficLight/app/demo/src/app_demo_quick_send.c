/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: app main.
 * Author: Hisilicon
 * Create: 2019-03-04
 */
#include <hi3861_platform.h>
#include <hi_mdm.h>
#include "app_main.h"
#ifndef CONFIG_FACTORY_TEST_MODE
#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/netifapi.h"
#endif
#include <hi_flash.h>
#include <hi_nv.h>
#include <hi_lowpower.h>
#include <hi_diag.h>
#include <hi_crash.h>
#include <hi_sal.h>
#include <hi_shell.h>
#include "app_demo_upg_verify.h"
#if defined(CONFIG_AT_COMMAND) || defined(CONFIG_FACTORY_TEST_MODE)
#include <hi_at.h>
#endif
#include <hi_fs.h>
#include "hi_wifi_api.h"
#include <hi_partition_table.h>
#include <hi_ver.h>
#include <hi_cpu.h>
#include <hi_crash.h>
#ifdef CONFIG_DMA_SUPPORT
#include <hi_dma.h>
#endif
#ifdef CONFIG_I2C_SUPPORT
#include <hi_i2c.h>
#endif
#ifdef CONFIG_I2S_SUPPORT
#include <hi_i2s.h>
#endif
#ifdef CONFIG_SPI_SUPPORT
#include <hi_spi.h>
#endif
#ifdef CONFIG_PWM_SUPPORT
#include <hi_pwm.h>
#endif
#ifdef CONFIG_SDIO_SUPPORT
#include <hi_sdio_device.h>
#include <hi_watchdog.h>
#include <app_demo_sdio_device.h>
#endif
#ifdef CONFIG_HILINK
#include "hilink.h"
#endif
#ifdef CONFIG_QUICK_SEND_MODE
#include "hi_wifi_quick_send_api.h"
#endif
#include <app_io_init.h>

#define APP_DEMO_RELEASE_MEM_TASK_SIZE 0x200
/* 高优先级(不得小于3)，尽快释放系统app_main栈内存 */
/* If the priority is lower than 3, release the app_main stack memory as soon as possible. */
#define APP_DEMO_RELEASE_MEM_TASK_PRIO 0x3

#ifndef CONFIG_QUICK_SEND_MODE

#define APP_INIT_VAP_NUM    2
#ifdef CONFIG_MESH_SUPPORT
#define APP_INIT_USR_NUM    6
#else
#define APP_INIT_USR_NUM    2
#endif

#else
#define APP_INIT_VAP_NUM    1
#define APP_INIT_USR_NUM    1
#endif

#define APP_INIT_EVENT_NUM  4

#define PERIPHERAL_INIT_ERR_FLASH   (1 << 0)
#define PERIPHERAL_INIT_ERR_UART0   (1 << 1)
#define PERIPHERAL_INIT_ERR_UART1   (1 << 2)
#define PERIPHERAL_INIT_ERR_UART2   (1 << 3)
#define PERIPHERAL_INIT_ERR_IO      (1 << 4)
#define PERIPHERAL_INIT_ERR_CIPHER  (1 << 5)
#define PERIPHERAL_INIT_ERR_DMA     (1 << 6)
#define PERIPHERAL_INIT_ERR_I2C     (1 << 7)
#define PERIPHERAL_INIT_ERR_I2S     (1 << 8)
#define PERIPHERAL_INIT_ERR_SPI     (1 << 9)
#define PERIPHERAL_INIT_ERR_PWM     (1 << 10)
#define PERIPHERAL_INIT_ERR_SDIO    (1 << 11)

#ifdef CONFIG_SDIO_SUPPORT
#define APP_SDIO_INIT_TASK_SIZE 0x1000
#define APP_SDIO_INIT_TASK_PRIO 25

static hi_void *sdio_init_task_body(hi_void *param)
{
    printf("start sdio init\r\n");
    hi_unref_param(param);

    hi_watchdog_disable();
    hi_u32 ret = hi_sdio_init();
    if (ret != HI_ERR_SUCCESS) {
        printf("sdio driver init fail\r\n");
    }
    hi_watchdog_enable();
    app_demo_sdio_callback_init();
    printf("finish sdio init\r\n");
    return HI_NULL;
}

hi_u32 app_sdio_init(hi_void)
{
    /* Create a task to init sdio */
    hi_u32 sdio_init_task_id = 0;
    hi_task_attr attr = {0};
    attr.stack_size = APP_SDIO_INIT_TASK_SIZE;
    attr.task_prio = APP_SDIO_INIT_TASK_PRIO;
    attr.task_name = (hi_char*)"sdio_init";
    hi_u32 ret = hi_task_create(&sdio_init_task_id, &attr, sdio_init_task_body, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("Falied to create sdio init task!\n");
    }
    return ret;
}
#endif

#define CLKEN_I2C0      14
#define CLKEN_I2C1      15
#define CLKEN_SPI0      5
#define CLKEN_SPI1      0
#define CLKEN_MONITOR   6
#define CLKEN_DMA_WBUS  1
#define CLKEN1_PWM5     10
#define CLKEN1_PWM_BUS  6
#define CLKEN1_PWM      5
#define CLKEN1_PWM4     4
#define CLKEN1_PWM3     3
#define CLKEN1_PWM2     2
#define CLKEN1_PWM1     1
#define CLKEN1_PWM0     0
#define CLKEN1_PWM_ALL  ((1 << CLKEN1_PWM0) | (1 << CLKEN1_PWM1) | (1 << CLKEN1_PWM2) | (1 << CLKEN1_PWM3) | \
                        (1 << CLKEN1_PWM4) | (1 << CLKEN1_PWM5))
#define CLKEN2_I2S_BUS  11
#define CLKEN2_I2S      10
#define CLKEN_UART2     6
#define CLKEN_UART2_BUS 9
#define CLKEN_TIMER1    7
#define CLKEN_TIMER2    8
#define CLKEN_SDIO_WBUS 4

hi_void peripheral_close_clken(hi_void)
{
    hi_u16 reg_val;
    hi_reg_read16(CLDO_CTL_CLKEN_REG, reg_val);
    reg_val &= ~((1 << CLKEN_I2C0) | (1 << CLKEN_I2C1));
    reg_val &= ~((1 << CLKEN_SPI0) | (1 << CLKEN_SPI1));
    reg_val &= ~((1 << CLKEN_DMA_WBUS) | (1 << CLKEN_MONITOR));
    reg_val &= ~((1 << CLKEN_TIMER1) | (1 << CLKEN_TIMER2));
    hi_reg_write16(CLDO_CTL_CLKEN_REG, reg_val); /* disable clken0 clk gate */

#ifndef CONFIG_PWM_HOLD_AFTER_REBOOT
    hi_reg_read16(CLDO_CTL_CLKEN1_REG, reg_val);
    reg_val &= ~CLKEN1_PWM_ALL;
    reg_val &= ~((1 << CLKEN1_PWM_BUS) | (1 << CLKEN1_PWM));
    hi_reg_write16(CLDO_CTL_CLKEN1_REG, reg_val); /* disable clken1 clk gate */
#endif

    hi_reg_read16(CLDO_CTL_CLKEN2_REG, reg_val);
    reg_val &= ~((1 << CLKEN2_I2S) | (1 << CLKEN2_I2S_BUS));
    hi_reg_write16(CLDO_CTL_CLKEN2_REG, reg_val); /* disable clken2 clk gate */
    hi_reg_read16(W_CTL_UART_MAC80M_CLKEN_REG, reg_val);
#ifdef CONFIG_SDIO_SUPPORT
        reg_val &= ~((1 << CLKEN_UART2) | (1 << CLKEN_UART2_BUS));
#else
        reg_val &= ~((1 << CLKEN_UART2) | (1 << CLKEN_UART2_BUS) | (1 << CLKEN_SDIO_WBUS));
#endif
    hi_reg_write16(W_CTL_UART_MAC80M_CLKEN_REG, reg_val); /* disable uart_mac80m clk gate */
    hi_reg_write16(PMU_CMU_CTL_CLK_960M_GT_REG, 0x1); /* disable 960m clk gate */
}

static hi_uart_attribute g_at_uart_cfg  = {115200, 8, 1, 0, 0};

hi_bool g_have_inited = HI_FALSE;

hi_void peripheral_init(hi_void)
{
    hi_u32 ret;
    hi_u32 err_info = 0;
    hi_cipher_set_clk_switch(HI_TRUE);
    peripheral_close_clken();
    hi_flash_deinit();
    ret = hi_flash_init();
    if (ret != HI_ERR_SUCCESS) {
        err_info |= PERIPHERAL_INIT_ERR_FLASH;
    }

    if (g_have_inited == HI_FALSE) {
        /* app_io_set_gpio2_clkout_enable(HI_TRUE); set gpio2 clock out  */

        ret = hi_uart_init(HI_UART_IDX_1, &g_at_uart_cfg, HI_NULL);
        if (ret != HI_ERR_SUCCESS) {
            err_info |= PERIPHERAL_INIT_ERR_UART1;
        }
    } else {
        /* app_io_set_gpio2_clkout_enable(HI_TRUE); output clock after wakeup, user also can output clock whenever needed */

        ret = hi_uart_lp_restore(HI_UART_IDX_1);
        if (ret != HI_ERR_SUCCESS) {
            err_info |= PERIPHERAL_INIT_ERR_UART0;
        }
        ret = hi_uart_lp_restore(HI_UART_IDX_0);
        if (ret != HI_ERR_SUCCESS) {
            err_info |= PERIPHERAL_INIT_ERR_UART1;
        }
        ret = hi_uart_lp_restore(HI_UART_IDX_2);
        if (ret != HI_ERR_SUCCESS) {
            err_info |= PERIPHERAL_INIT_ERR_UART2;
        }
    }
    g_have_inited = HI_TRUE;

    app_io_init();

    ret = hi_cipher_init();
    if (ret != HI_ERR_SUCCESS) {
        err_info |= PERIPHERAL_INIT_ERR_CIPHER;
    }
}

hi_void peripheral_init_no_sleep(hi_void)
{
    /* 示例:深睡唤醒不需要重新初始化的外设，可放置在此函数初始化 */
    /* Peripherals that don't need to be reinitialized during deep sleep wakeup are initialized here. */
#ifdef CONFIG_SDIO_SUPPORT
    hi_sdio_set_powerdown_when_deep_sleep(HI_FALSE);
    hi_u32 ret = app_sdio_init();
    if (ret != HI_ERR_SUCCESS) {
        printf("sdio init failed\r\n");
    }
#endif
}

#ifdef CONFIG_QUICK_SEND_MODE
hi_u8 g_bssid[QUICK_SEND_BSSID_LEN] = {0x00, 0xE0, 0xFC, 0x26, 0x51, 0xCD};
hi_u8 g_src_mac[QUICK_SEND_MAC_LEN] = {0x00, 0xE0, 0xFC, 0x26, 0x51, 0xB4};
hi_u8 g_dst_mac[QUICK_SEND_MAC_LEN] = {0x00, 0xE0, 0xFC, 0x26, 0x51, 0xCD};
hi_u16 g_seq_num = 0xffff;

hi_void set_frame_seq_num(hi_u8 buf[], hi_u16 size)
{
    /* 用随机值的低12bit初始化 */
    if (g_seq_num == 0xffff) {
        hi_u32 rand_data;
        (hi_void)hi_cipher_trng_get_random(&rand_data);
        g_seq_num = (hi_u16)(rand_data & 0xfff);
    }
    /* 超过上限,回绕 */
    if (g_seq_num > 0xfff) {
        g_seq_num = 0;
    }
    hi_u16 seq_num = g_seq_num << 4; /* 4:前4位是frag number, */
    /* 设置seq num */
    if (memcpy_s(buf + QUICK_SEND_SEQ_NUM_OFFSET, size - QUICK_SEND_SEQ_NUM_OFFSET, &seq_num, sizeof(hi_u16)) != EOK) {
        printf("set_frame_seq_num: copy seq num failed\r\n");
    }
    ++g_seq_num;
}

hi_bool send_frame(hi_u8 buf[], hi_u16 size, hi_bool brocast)
{
    hi_u8 waite_cnt = 0;
    hi_u8 resend_waite_cnt = 0;
    /* 设置源mac地址 */
    if (memcpy_s(buf + QUICK_SEND_SRC_MAC_OFFSET, size - QUICK_SEND_SRC_MAC_OFFSET,
        g_src_mac, QUICK_SEND_MAC_LEN) != EOK) {
        return HI_FALSE;
    }
    /* 设置seq num */
    set_frame_seq_num(buf, size);
    if (brocast) {
        /* 设置目标mac地址 */
        if (memset_s(buf + QUICK_SEND_DST_MAC_OFFSET, size - QUICK_SEND_DST_MAC_OFFSET,
            0xFF, QUICK_SEND_MAC_LEN) != EOK) {
            return HI_FALSE;
        }
        /* 设置BSSID */
        if (memset_s(buf + QUICK_SEND_BSSID_OFFSET, size - QUICK_SEND_BSSID_OFFSET,
            0xFF, QUICK_SEND_BSSID_LEN) != EOK) {
            return HI_FALSE;
        }
    } else {
        /* 设置目标mac地址 */
        if (memcpy_s(buf + QUICK_SEND_DST_MAC_OFFSET, size - QUICK_SEND_DST_MAC_OFFSET,
            g_dst_mac, QUICK_SEND_MAC_LEN) != EOK) {
            return HI_FALSE;
        }
        /* 设置BSSID */
        if (memcpy_s(buf + QUICK_SEND_BSSID_OFFSET, size - QUICK_SEND_BSSID_OFFSET,
            g_bssid, QUICK_SEND_BSSID_LEN) != EOK) {
            return HI_FALSE;
        }
    }
    if (hi_wifi_tx_proc_fast(buf, size) != 0) {
        printf("send cmd failed, call hi_wifi_tx_proc_fast fail.\r\n");
    }
    if (brocast) {
        return HI_TRUE;
    }

    hi_u8 result = hi_wifi_get_quick_send_result();
    while ((result != QUICK_SEND_SUCCESS) && (++waite_cnt <= 15)) { /* 15:最多延迟15ms等待发送成功 */
        hi_sleep(1); /* 1:wait 1ms for pkt to send */
        result = hi_wifi_get_quick_send_result();
        /* send fail or 4ms no response */
        if ((result != QUICK_SEND_SUCCESS) && (++resend_waite_cnt > 5)) { /* 5:发送次数 */
            if (hi_wifi_tx_proc_fast(buf, size) != 0) {
                printf("send cmd failed, call hi_wifi_tx_proc_fast fail.\r\n");
            }
            resend_waite_cnt = 0;
        }
    }
    if (waite_cnt > 15) { /* 15:发送次数 */
        printf("send cmd failed\r\n");
        return HI_FALSE;
    }
    return HI_TRUE;
}
#endif


/*****************************************************************************
 功能描述  : 混杂模式下收包上报
*****************************************************************************/
int recv_cb(void* recv_buf, int frame_len, signed char rssi)
{
    hi_u8 *buf = (hi_u8 *)recv_buf;
    if (frame_len > 9) { /* 9:最短长度 */
        printf("resv buf: %u , len: %d , %x:%x:%x:%x\r\n", *(unsigned int*)recv_buf, frame_len,
            buf[6], buf[7], buf[8], buf[9]); /* 6 7 8 9:bssid后4位 */
    }
    return HI_ERR_SUCCESS;
}

static hi_void *app_demo_task_second_body(hi_void *param)
{
    /* Releases the app_main stack memory. */
    hi_unref_param(param);
    return HI_NULL;
}

static hi_void *app_demo_task_body(hi_void *param)
{
    /* Releases the app_main stack memory. */
    hi_unref_param(param);

    hi_u32 task_id = 0;
    hi_task_attr attr = {0};
    attr.stack_size = APP_DEMO_RELEASE_MEM_TASK_SIZE;
    attr.task_prio = APP_DEMO_RELEASE_MEM_TASK_PRIO;
    attr.task_name = (hi_char*)"app_demo_second";
    hi_u32 ret = hi_task_create(&task_id, &attr, app_demo_task_second_body, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("Falied to create app_demo_second task:0x%x\n", ret);
    }
    return HI_NULL;
}

hi_void app_demo_task_release_mem(hi_void)
{
    /* Releases the app_main stack memory. */
    hi_u32 task_id = 0;
    hi_task_attr attr = {0};
    attr.stack_size = APP_DEMO_RELEASE_MEM_TASK_SIZE;
    attr.task_prio = APP_DEMO_RELEASE_MEM_TASK_PRIO;
    attr.task_name = (hi_char*)"app_demo";
    hi_u32 ret = hi_task_create(&task_id, &attr, app_demo_task_body, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("Falied to create app_demo task:0x%x\n", ret);
    }
    return;
}

#ifdef CONFIG_QUICK_SEND_MODE
hi_void app_init(hi_void)
{
    hi_u32 ret;
    peripheral_init();
    peripheral_init_no_sleep();

    (hi_void)hi_event_init(APP_INIT_EVENT_NUM, HI_NULL);
    hi_sal_init();
    /* 此处设为TRUE后中断中看门狗复位会显示复位时PC值，但有复位不完全风险，量产版本请务必设为FALSE */
    /*
     * If this parameter is set to TRUE, the PC value during reset is displayed when the watchdog is reset.
     * However,the reset may be incomplete.
     * Therefore, you must set this parameter to FALSE for the mass production version.
     */
    hi_syserr_watchdog_debug(HI_FALSE);
    /* 默认记录宕机信息到FLASH，根据应用场景，可不记录，避免频繁异常宕机情况损耗FLASH寿命 */
    /* By default, breakdown information is recorded in the flash memory. You can choose not to record breakdown
     * information based on the application scenario to prevent flash servicelife loss caused by frequent breakdown. */
    hi_syserr_record_crash_info(HI_TRUE);

    /* 如果不需要使用Histudio查看WIFI驱动运行日志等，无需初始化diag */
    /* if not use histudio for diagnostic, diag initialization is unnecessary */
    /* Shell and Diag use the same uart port, only one of them can be selected */
#ifndef CONFIG_FACTORY_TEST_MODE

#ifndef ENABLE_SHELL_DEBUG
#ifdef CONFIG_DIAG_SUPPORT
    (hi_void)hi_diag_init();
#endif
#else
    (hi_void)hi_shell_init();
#endif

    tcpip_init(NULL, NULL);
#endif
    ret = hi_wifi_init(APP_INIT_VAP_NUM, APP_INIT_USR_NUM);
    if (ret != HISI_OK) {
        printf("wifi init failed!\n");
    }
}

hi_void app_main(hi_void)
{
#ifndef CONFIG_FACTORY_TEST_MODE
    app_init();

    hi_wifi_ptype_filter filter;
    filter.mdata_en = 0;
    filter.mmngt_en = 0;
    filter.udata_en = 1;
    filter.umngt_en = 1;
    filter.custom_en = 0;

    /* 配置发送参数,6信道,11b 1Mbps */
    if (hi_wifi_cfg_tx_para("6", "1", WIFI_PHY_MODE_11B) != 0) {
        printf("hi_wifi_cfg_tx_para ...failed\n");
        return;
    }

    /* 获取设备mac地址 */
    if (hi_wifi_get_macaddr((hi_char *)&g_src_mac, QUICK_SEND_MAC_LEN) != 0) {
        printf("hi_wifi_get_macaddr ...failed\n");
    }

   /* 发送第1个数据帧 */
    /* BSSID:4~9,src MAC:10~15,dst mac:16~21 */
    hi_u8 buf[] = {0x48, 0x01, 0x3C, 0x00, 0xC8, 0x3A, 0x35, 0xF2, 0x6F, 0x61, 0x54, 0x11, 0x31, 0xF7, 0xAC, 0x1F,
        0xC8, 0x3A, 0x35, 0xF2, 0x6F, 0x61, 0x20, 0x00, 0x55, 0x55, 0x55};
    if (!send_frame(buf, sizeof(buf), HI_FALSE)) {
        printf("send cmd 1 failed\n");
    }
    /* 发送第2个数据帧 */
    hi_u8 buf1[] = {0x48, 0x01, 0x3C, 0x00, 0xC8, 0x3A, 0x35, 0xF2, 0x6F, 0x61, 0x54, 0x11, 0x31, 0xF7, 0xAC, 0x1F,
        0xC8, 0x3A, 0x35, 0xF2, 0x6F, 0x61, 0x20, 0x00, 0x66, 0x66, 0x66};
    if (!send_frame(buf1, sizeof(buf1), HI_FALSE)) {
        printf("send cmd 2 failed\n");
    }
    hi_wifi_promis_set_rx_callback(recv_cb);
    if (hi_wifi_promis_enable("Hisilicon0", HI_TRUE, &filter) != HI_ERR_SUCCESS) {
        printf("hi_wifi_promis_enable:: set error!\r\n");
        return;
    }

    hi_u8 auth[] = {0xB0, 0x00, 0x3A, 0x01, 0x50, 0x2B, 0xD2, 0xB4, 0x7E, 0x29, 0x90, 0x94, 0x97, 0x30, 0x60, 0x9A,
        0x50, 0x2B, 0xD2, 0xB4, 0x7E, 0x29, 0xE0, 0xA2, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xDD, 0x09, 0x00, 0x10,
        0x18, 0x02, 0x00, 0x00, 0x12, 0x00, 0x00};
    if (!send_frame(auth, sizeof(auth), HI_FALSE)) {
        printf("send auth failed\n");
    }
    hi_sleep(3); /* 3:ms,wait for response */
    if (hi_wifi_promis_enable("Hisilicon0", HI_FALSE, &filter) != HI_ERR_SUCCESS) {
        printf("hi_wifi_promis_enable:: set error!\r\n");
        return;
    }

    app_demo_task_release_mem();  /* Task used to release the system stack memory. */

#endif
}
#endif

