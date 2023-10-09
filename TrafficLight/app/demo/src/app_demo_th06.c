/*
 * Copyright (c) 2020 HiHope Community.
 * Description: th06 demo
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */
#include <hi_adc.h>
#include <hi_early_debug.h>
#include <hi_task.h>
#include <hi_types_base.h>
#include <hi_i2c.h>
#include <hi_io.h>
#include <th06.h>
#include <hi_watchdog.h>
#include <hi_stdlib.h>
#include <hi_time.h>


#define  TH06_DEMO_TASK_STAK_SIZE    (1024*2)
#define  TH06_DEMO_TASK_PRIORITY     (25)
#define  TH06_REG_ARRAY_LEN          (6)

hi_u8  g_th06_demo_task_id =0;
hi_float th06_temperature =0.0;
hi_float th06_humidity =0.0;

hi_float g_temperature =0.0;
hi_float g_humidity =0.0;

extern hi_u8 g_current_mode;
/*写寄存器*/
hi_u32 th06_i2c_write( hi_u8 cmd)
{
    hi_u32 status =0;
    hi_i2c_idx id = 0;
    hi_i2c_data th06_i2c_write_cmd_addr ={0};
    hi_u8 _send_user_cmd[1] = {cmd};


    th06_i2c_write_cmd_addr.send_buf = _send_user_cmd;
    th06_i2c_write_cmd_addr.send_len = 1;

    status = hi_i2c_write(id, (TH06_DEVICE_ADDR<<1)|0x00, &th06_i2c_write_cmd_addr);
    // if (status != HI_ERR_SUCCESS) {
        // printf("===== Error: th06 sensor I2C write cmd address status = 0x%x! =====\r\n", status);
        // return status;
    // }
    return HI_ERR_SUCCESS;
}
/*读取 th06 serson 数据*/
hi_float th06_read(hi_i2c_idx id, hi_u32 recv_len, hi_u8 type)
{
    hi_u32 status;
    hi_u8 recv_data[TH06_REG_ARRAY_LEN] = { 0 };
    hi_i2c_data th06_i2c_data = { 0 };
    hi_u16 temper =0;
    hi_u16 humi =0;
    hi_float Tem =0;
    hi_float Hum =0;
    /* Request memory space */
    memset(recv_data, 0x0, sizeof(recv_data));
    memset(&th06_i2c_data, 0x0, sizeof(hi_i2c_data));


    th06_i2c_data.receive_buf = recv_data;
    th06_i2c_data.receive_len = recv_len;
    
    status = hi_i2c_read(id, (TH06_DEVICE_ADDR<<1)|0x01, &th06_i2c_data);
    // if (status != HI_ERR_SUCCESS) {
        // printf("===== Error: th06 sencor I2C read status = 0x%x! =====\r\n", status);
        // return status;
    // }

    if (type == TEMPERATURE) {
        temper = (recv_data[0]<<8 | recv_data[1]);//温度拼接
        Tem = (175.72*(float)temper/65536.0-46.85);   // T= -46.85+175.72*temper/(2^16-1)
        return Tem;

    } else if (type == HUMIDITY) {
        humi = (recv_data[0]<<8 | recv_data[1]);//湿度拼接
        Hum= (125.0*(float)humi/65536.0-6);           // RH = hum*125 / (2^16-1)
        return Hum;
    }
}

/*获取th06 数据*/
hi_void th06_get_data(hi_void)
{
    hi_i2c_idx id =0;
    hi_i2c_init(0, 400000); /* baudrate: 400000 */
    hi_i2c_set_baudrate(0, 400000);

    th06_i2c_write(0xE3);//tempwerature
    hi_udelay(100000);//100ms
    g_temperature = th06_read(id, 2, TEMPERATURE);

    th06_i2c_write(0xE5);//humidity
    hi_udelay(100000);//100ms
    g_humidity = th06_read(id, 2, HUMIDITY);
}

/*app demo th06*/
hi_void *app_demo_th06(hi_void *param)
{
    hi_i2c_idx id =0;
    hi_u32 status =0;
    hi_i2c_init(0, 400000); /* baudrate: 400000 */
    hi_i2c_set_baudrate(0, 400000);
    while(1){
        /* on hold master mode*/
        status = th06_i2c_write(0xE3);//tempwerature
        printf("write th06 cmd=%s\r\n",status == HI_ERR_SUCCESS?"SUCCESS":"FAILURE");
        hi_udelay(100000);//100ms
        th06_read(id, 2, TEMPERATURE);

        status = th06_i2c_write(0xE5);//humidity
        printf("write th06 cmd=%s\r\n",status == HI_ERR_SUCCESS?"SUCCESS":"FAILURE");
        hi_udelay(100000);//100ms
        th06_read(id, 2, HUMIDITY);

        hi_watchdog_feed();
        hi_sleep(20);//20ms
    }
}
/*app demo th06 task*/
hi_void app_demo_th06_task(hi_void)
{
    hi_u32 ret =0;
    hi_task_attr attr ={0};

    attr.stack_size = TH06_DEMO_TASK_STAK_SIZE;
    attr.task_prio = TH06_DEMO_TASK_PRIORITY;
    attr.task_name = (hi_char*)"app_demo_th06_task";
    ret = hi_task_create(&g_th06_demo_task_id, &attr, app_demo_th06, HI_NULL);
    if(ret != HI_ERR_SUCCESS){
        printf("Failed to create app_demo_th06_task\r\n");
    }
}