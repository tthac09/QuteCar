#include <hi_early_debug.h>
#include <hi_task.h>
#include <hi_uart.h>
#include <hi_types_base.h>
#include <hi_io.h>
#include <string.h>
#include <stdio.h>
#include <hi_time.h>
#include <hi_gpio.h>
#include <hi_watchdog.h>

#define DEMO_UART_0_NUM            HI_UART_IDX_0  //uart 0
#define DEMO_UART_1_NUM            HI_UART_IDX_1  //uart 1

/*
 * This demo simply shows how to read datas from UART2 port and then echo back.
 */
hi_void uart_demo(hi_void)
{
    printf("uart_demo start\n");
    hi_u32 ret;
    hi_u8 write_buf[20] = "123456789";
    hi_uart_attribute uart_attr = {
        .baud_rate = 115200, /* baud_rate: 115200 */
        .data_bits = 8,      /* data_bits: 8bits */
        .stop_bits = 1,
        .parity = 0,
    };

    /* Initialize uart driver */
    ret = hi_uart_init(DEMO_UART_1_NUM, &uart_attr, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to init uart! Err code = %d\n", ret);
        return;
    }

    printf("hi_uart_write start\n");
    while (1)
    {
        hi_uart_write(DEMO_UART_1_NUM,write_buf,sizeof(write_buf));
        hi_watchdog_feed();
        hi_sleep(50);
    }
    
}