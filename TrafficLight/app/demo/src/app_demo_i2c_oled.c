/*
 * Copyright (c) 2020 HiHope Community.
 * Description: oled demo
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */
#include "hi_i2c.h"
#include <hi_early_debug.h>
#include <string.h>
#include <hi_time.h>
#include <SSD1306_OLED.h>
#include <code_tab.h>
#include <los_hwi.h>
#include <hi_task.h>
#include <hi_mux.h>
#include <hi_watchdog.h>
#include <app_demo_multi_sample.h>
#include <app_demo_traffic_sample.h>
extern hi_u8 g_current_mode;

hi_u8 ndef_file_wechat[1024] = {
    0x00,
    0x20,
    0xd4,
    0x0f,
    0x0e,
    0x61,
    0x6e,
    0x64,
    0x72,
    0x6f,
    0x69,
    0x64,
    0x2e,
    0x63,
    0x6f,
    0x6d,
    0x3a,
    0x70,
    0x6b,
    0x67,
    0x63,
    0x6f,
    0x6d,
    0x2e,
    0x74,
    0x65,
    0x6e,
    0x63,
    0x65,
    0x6e,
    0x74,
    0x2e,
    0x6d,
    0x6d,
};
hi_u8 ndef_file_today_headline[1024] = {
    0x00,
    0x2d,
    0xd4,
    0x0f,
    0x1b,
    0x61,
    0x6e,
    0x64,
    0x72,
    0x6f,
    0x69,
    0x64,
    0x2e,
    0x63,
    0x6f,
    0x6d,
    0x3a,
    0x70,
    0x6b,
    0x67,
    0x63,
    0x6f,
    0x6d,
    0x2e,
    0x73,
    0x73,
    0x2e,
    0x61,
    0x6e,
    0x64,
    0x72,
    0x6f,
    0x69,
    0x64,
    0x2e,
    0x61,
    0x72,
    0x74,
    0x69,
    0x63,
    0x6c,
    0x65,
    0x2e,
    0x6e,
    0x65,
    0x77,
    0x73,
};
hi_u8 ndef_file_taobao[1024] = {
    0x00,
    0x23,
    0xd4,
    0x0f,
    0x11,
    0x61,
    0x6e,
    0x64,
    0x72,
    0x6f,
    0x69,
    0x64,
    0x2e,
    0x63,
    0x6f,
    0x6d,
    0x3a,
    0x70,
    0x6b,
    0x67,
    0x63,
    0x6f,
    0x6d,
    0x2e,
    0x74,
    0x61,
    0x6f,
    0x62,
    0x61,
    0x6f,
    0x2e,
    0x74,
    0x61,
    0x6f,
    0x62,
    0x61,
    0x6f,
};
hi_u8 ndef_file_huawei_smart_life[1024] = {
    0x00,
    0x26,
    0xd4,
    0x0f,
    0x14,
    0x61,
    0x6e,
    0x64,
    0x72,
    0x6f,
    0x69,
    0x64,
    0x2e,
    0x63,
    0x6f,
    0x6d,
    0x3a,
    0x70,
    0x6b,
    0x67,
    0x63,
    0x6f,
    0x6d,
    0x2e,
    0x68,
    0x75,
    0x61,
    0x77,
    0x65,
    0x69,
    0x2e,
    0x73,
    0x6d,
    0x61,
    0x72,
    0x74,
    0x68,
    0x6f,
    0x6d,
    0x65,
};
hi_u8 ndef_file_histreaming[1024] = {
    0x00,
    0x3d,
    0xd4,
    0x0f,
    0x2b,
    0x61,
    0x6e,
    0x64,
    0x72,
    0x6f,
    0x69,
    0x64,
    0x2e,
    0x63,
    0x6f,
    0x6d,
    0x3a,
    0x70,
    0x6b,
    0x67,
    0x61,
    0x70,
    0x70,
    0x6b,
    0x69,
    0x74,
    0x2e,
    0x6f,
    0x70,
    0x65,
    0x6e,
    0x73,
    0x6f,
    0x75,
    0x72,
    0x63,
    0x65,
    0x2e,
    0x67,
    0x69,
    0x7a,
    0x77,
    0x69,
    0x74,
    0x73,
    0x2e,
    0x63,
    0x6f,
    0x6d,
    0x2e,
    0x6d,
    0x79,
    0x61,
    0x70,
    0x70,
    0x6c,
    0x69,
    0x63,
    0x61,
    0x74,
    0x69,
    0x6f,
    0x6e,
};
#define I2C_REG_ARRAY_LEN (64)
#define OLED_SEND_BUFF_LEN (28)
#define OLED_SEND_BUFF_LEN2 (25)
#define OLED_SEND_BUFF_LEN3 (27)
#define OLED_SEND_BUFF_LEN4 (29)
#define Max_Column (128)
#define OLED_DEMO_TASK_STAK_SIZE (1024 * 2)
#define OLED_DEMO_TASK_PRIORITY (25)
#define OLED_DISPLAY_INTERVAL_TIME (1)
#define SEND_CMD_LEN (2)

hi_u8 g_oled_demo_task_id = 0;
hi_u32 g_mux_id = 0;

extern hi_float g_temperature;
extern hi_float g_humidity;
extern hi_double g_combustible_gas_value;

extern hi_u8 ndef_file[1024];
extern hi_u32 th06_get_data(hi_void);
extern hi_void mq2_get_data(hi_void);

extern hi_u8 g_menu_type;
extern hi_u8 g_menu_select;
extern hi_u8 g_current_mode;
extern hi_u8 g_current_type;
extern hi_u8 g_key_down_flag;
extern hi_u8 g_menu_mode;
extern hi_u32 g_gpio7_first_key_dwon;

extern hi_void all_light_out(hi_void);

hi_void colorful_light_func(hi_void);
hi_void led_menu_func_select(hi_void);
extern hi_void traffic_control_mode_sample(hi_void);
extern hi_void traffic_auto_mode_sample(hi_void);
extern hi_void return_main_enum_sample(hi_void);
extern hi_void gpio_control(hi_io_name gpio, hi_gpio_idx id, hi_gpio_dir dir, hi_gpio_value gpio_val, hi_u8 val);
extern hi_void traffic_human_mode_sample(hi_void);
static hi_u8 hi3861_board_led_test = 0;

/*
    *@bref   向ssd1306 屏幕寄存器写入命令
    *status 0：表示写入成功，否则失败
*/
static hi_u32 i2c_write_byte(hi_u8 reg_addr, hi_u8 cmd)
{
    hi_u32 status = 0;
    hi_i2c_idx id = 0; //I2C 0
    hi_u8 send_len = 2;
    hi_u8 user_data = cmd;
    hi_i2c_data oled_i2c_cmd = {0};
    hi_i2c_data oled_i2c_write_cmd = {0};

    hi_u8 send_user_cmd[SEND_CMD_LEN] = {OLED_ADDRESS_WRITE_CMD, user_data};
    hi_u8 send_user_data[SEND_CMD_LEN] = {OLED_ADDRESS_WRITE_DATA, user_data};

    /*如果是写命令，发写命令地址0x00*/
    if (reg_addr == OLED_ADDRESS_WRITE_CMD)
    {
        oled_i2c_write_cmd.send_buf = send_user_cmd;
        oled_i2c_write_cmd.send_len = send_len;
        status = hi_i2c_write(id, OLED_ADDRESS, &oled_i2c_write_cmd);
        if (status != HI_ERR_SUCCESS)
        {
            return status;
        }
    }
    /*如果是写数据，发写数据地址0x40*/
    else if (reg_addr == OLED_ADDRESS_WRITE_DATA)
    {
        oled_i2c_cmd.send_buf = send_user_data;
        oled_i2c_cmd.send_len = send_len;
        status = hi_i2c_write(id, OLED_ADDRESS, &oled_i2c_cmd);
        if (status != HI_ERR_SUCCESS)
        {
            return status;
        }
    }

    return HI_ERR_SUCCESS;
}

/*写命令操作*/
static hi_u32 write_cmd(hi_u8 cmd) //写命令
{
    hi_u8 status = 0;
    /*写设备地址*/
    status = i2c_write_byte(OLED_ADDRESS_WRITE_CMD, cmd);
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
}
/*写数据操作*/
static hi_void write_data(hi_u8 i2c_data) //写数据
{
    hi_u8 status = 0;
    /*写设备地址*/
    status = i2c_write_byte(OLED_ADDRESS_WRITE_DATA, i2c_data);
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
}
/* ssd1306 oled 初始化*/
hi_u32 oled_init(hi_void)
{
    hi_u32 status;
    hi_udelay(DELAY_100_MS); //100ms  这里的延时很重要

    status = write_cmd(DISPLAY_OFF); //--display off
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_LOW_COLUMN_ADDRESS); //---set low column address
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_HIGH_COLUMN_ADDRESS); //---set high column address
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_START_LINE_ADDRESS); //--set start line address
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_PAGE_ADDRESS); //--set page address
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(CONTRACT_CONTROL); // contract control
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(FULL_SCREEN); //--128
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_SEGMENT_REMAP); //set segment remap
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(NORMAL); //--normal / reverse
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_MULTIPLEX); //--set multiplex ratio(1 to 64)
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DUTY); //--1/32 duty
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SCAN_DIRECTION); //Com scan direction
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DISPLAY_OFFSET); //-set display offset
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DISPLAY_TYPE);
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(OSC_DIVISION); //set osc division
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DIVISION);
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(COLOR_MODE_OFF); //set area color mode off
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(COLOR);
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PRE_CHARGE_PERIOD); //Set Pre-Charge Period
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PERIOD);
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PIN_CONFIGUARTION); //set com pin configuartion
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(CONFIGUARTION);
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_VCOMH); //set Vcomh
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(VCOMH);
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_CHARGE_PUMP_ENABLE); //set charge pump enable
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PUMP_ENABLE);
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(TURN_ON_OLED_PANEL); //--turn on oled panel
    if (status != HI_ERR_SUCCESS)
    {
        return HI_ERR_FAILURE;
    }
    return HI_ERR_SUCCESS;
}
/*
    @bref set start position 设置起始点坐标
    @param hi_u8 x:write start from x axis
           hi_u8 y:write start from y axis
*/
hi_void oled_set_position(hi_u8 x, hi_u8 y)
{
    write_cmd(0xb0 + y);
    write_cmd(((x & 0xf0) >> 4) | 0x10);
    write_cmd(x & 0x0f);
}
/*全屏填充*/
hi_void oled_fill_screen(hi_u8 fii_data)
{
    hi_u8 m = 0;
    hi_u8 n = 0;

    for (m = 0; m < 8; m++)
    {
        write_cmd(0xb0 + m);
        write_cmd(0x00);
        write_cmd(0x10);

        for (n = 0; n < 128; n++)
        {
            write_data(fii_data);
        }
    }
}
/*
    @bref Clear from a location
    @param hi_u8 fill_data: write data to screen register 
           hi_u8 line:write positon start from Y axis 
           hi_u8 pos :write positon start from x axis 
           hi_u8 len:write data len
*/
hi_void oled_position_clean_screen(hi_u8 fill_data, hi_u8 line, hi_u8 pos, hi_u8 len)
{
    hi_u8 m = line;
    hi_u8 n = 0;

    write_cmd(0xb0 + m);
    write_cmd(0x00);
    write_cmd(0x10);

    for (n = pos; n < len; n++)
    {
        write_data(fill_data);
    }
}
/*
    @bref 8*16 typeface
    @param hi_u8 x:write positon start from x axis 
           hi_u8 y:write positon start from y axis
           hi_u8 chr:write data
           hi_u8 char_size:select typeface
 */
hi_void oled_show_char(hi_u8 x, hi_u8 y, hi_u8 chr, hi_u8 char_size)
{
    hi_u8 c = 0;
    hi_u8 i = 0;

    c = chr - ' '; //得到偏移后的值

    if (x > Max_Column - 1)
    {
        x = 0;
        y = y + 2;
    }

    if (char_size == 16)
    {
        oled_set_position(x, y);
        for (i = 0; i < 8; i++)
        {
            write_data(F8X16[c * 16 + i]);
        }

        oled_set_position(x, y + 1);
        for (i = 0; i < 8; i++)
        {
            write_data(F8X16[c * 16 + i + 8]);
        }
    }
    else
    {
        oled_set_position(x, y);
        for (i = 0; i < 6; i++)
        {
            write_data(F6x8[c][i]);
        }
    }
}

/*
    @bref display string
    @param hi_u8 x:write positon start from x axis 
           hi_u8 y:write positon start from y axis
           hi_u8 *chr:write data
           hi_u8 char_size:select typeface
*/
hi_void oled_show_str(hi_u8 x, hi_u8 y, hi_u8 *chr, hi_u8 char_size)
{
    hi_u8 j = 0;

    if (chr == NULL)
    {
        printf("param is NULL,Please check!!!\r\n");
        return;
    }

    while (chr[j] != '\0')
    {
        oled_show_char(x, y, chr[j], char_size);
        x += 8;
        if (x > 120)
        {
            x = 0;
            y += 2;
        }
        j++;
    }
}
/*小数转字符串
 *输入：double 小数
 *输出：转换后的字符串
*/
hi_u8 *flaot_to_string(hi_double d, hi_u8 *str)
{
    hi_u8 str1[40] = {0};
    hi_s32 j = 0;
    hi_s32 k = 0;
    hi_s32 i = 0;

    if (str == NULL)
    {
        return;
    }

    i = (hi_s32)d; //浮点数的整数部分
    while (i > 0)
    {
        str1[j++] = i % 10 + '0';
        i = i / 10;
    }

    for (k = 0; k < j; k++)
    {
        str[k] = str1[j - 1 - k]; //被提取的整数部分正序存放到另一个数组
    }
    str[j++] = '.';

    d = d - (hi_s32)d; //小数部分提取
    for (i = 0; i < 1; i++)
    {
        d = d * 10;
        str[j++] = (hi_s32)d + '0';
        d = d - (hi_s32)d;
    }
    while (str[--j] == '0')
        ;
    str[++j] = '\0';
    return str;
}
/*可燃气体值转化*/
hi_u8 *flaot_to_string_gas(hi_double d, hi_u8 *str)
{
    hi_u8 str1[40] = {0};
    hi_s32 j = 0;
    hi_s32 k = 0;
    hi_s32 i = 0;

    if (str == NULL)
    {
        return;
    }

    i = (hi_s32)d; //浮点数的整数部分
    while (i > 0)
    {
        str1[j++] = i % 10 + '0';
        i = i / 10;
    }

    for (k = 0; k < j; k++)
    {
        str[k] = str1[j - 1 - k]; //被提取的整数部分正序存放到另一个数组
    }
    str[j++] = '.';

    d = d - (hi_s32)d; //小数部分提取
    for (i = 0; i < 3; i++)
    {
        d = d * 10;
        str[j++] = (hi_s32)d + '0';
        d = d - (hi_s32)d;
    }
    while (str[--j] == '0')
        ;
    str[++j] = '\0';
    return str;
}
/*显示屏幕选择菜单*/
hi_void led_show_func_select(hi_void)
{
    switch (g_menu_select)
    {
    case TRAFFIC_LIGHT_MENU:
        oled_show_str(48, 7, "2", 1);
        break;
    default:
        break;
    }
}
/*显示主界面*/
hi_void oled_main_menu_display(hi_void)
{

    oled_fill_screen(0x00); // clear screen
    oled_show_str(0, 1, "2.Traffic light", 1);
    if (g_menu_select == TRAFFIC_LIGHT_MENU)
    {
        oled_show_str(0, 7, "Selet:2   Enter", 1);
    }
}

/*shut down all led*/
hi_void all_led_off(hi_void)
{
    gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
    gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
    gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_12_GPIO);
}
/* display init */
hi_void oled_display_init(hi_void)
{
    oled_fill_screen(OLED_CLEAN_SCREEN); //clean screen
    oled_show_str(15, 3, "Hello Hi3861", 1);
    hi_udelay(DELAY_500_MS); //delay

    oled_fill_screen(OLED_CLEAN_SCREEN); //clean screen
    oled_show_str(0, 1, "2.Traffic light", 1);
    oled_show_str(0, 2, "Selet:1   Enter", 1);
}
/* oled screen demo display */
hi_void *app_i2c_oled_demo(hi_void *param)
{
    /*初始化时屏幕 i2c baudrate setting*/
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400kbps */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);
    /*ssd1306 config init*/
    // oled_init();
    while (HI_ERR_SUCCESS != oled_init())
    {
        printf("Connecting oled board falied!Please access oled board\r\n");
        if (hi3861_board_led_test == 0)
        {
            hi3861_board_led_test = 1;
            /*test HiSpark board*/
            FACTORY_HISPARK_BOARD_TEST("-----------HiSpark board test----------");
        }
        hi_sleep(SLEEP_100_MS);
    }
    /*按键中断初始化*/
    extern hi_void test_gpio_init(hi_void);
    test_gpio_init();
    /*display init*/
    oled_display_init();

    /*根据mode和type在OLED上面显示对应的内容，与硬件同步*/
    while (1)
    {
        /*led menu select*/
        while (g_key_down_flag == KEY_UP)
        {
            hi_sleep(SLEEP_10_MS);
        }

        if (g_key_down_flag == KEY_GPIO_5)
        {
            if (g_menu_mode == MAIN_FUNCTION_SELECT_MODE)
            {
                led_show_func_select();
            }
            g_gpio7_first_key_dwon = HI_TRUE;
            g_key_down_flag = KEY_UP;
        }

        if (g_key_down_flag == KEY_GPIO_7)
        {
            if (g_menu_mode == MAIN_FUNCTION_SELECT_MODE)
            {
                g_menu_mode = SUB_MODE_SELECT_MODE;
                switch (g_menu_select)
                {
                case TRAFFIC_LIGHT_MENU:
                    traffic_light_func();
                    break;
                default:
                    break;
                }
            }
            g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
            g_key_down_flag = KEY_UP;
        }
    }
}
/*
    @bref oled screen demo create task
    @param hi_void 
*/
hi_u32 app_oled_i2c_demo_task(hi_void)
{
    hi_u32 ret = HI_ERR_SUCCESS;
    hi_task_attr attr = {0};
    /*创建互斥锁*/
    hi_mux_create(&g_mux_id);

    hi_task_lock();
    attr.stack_size = OLED_DEMO_TASK_STAK_SIZE;
    attr.task_prio = OLED_DEMO_TASK_PRIORITY;
    attr.task_name = (hi_char *)"app_oled_i2c_demo_task";
    ret = hi_task_create(&g_oled_demo_task_id, &attr, app_i2c_oled_demo, HI_NULL);
    if (ret != HI_ERR_SUCCESS)
    {
        printf("Falied to create i2c sht3x demo task!\n");
    }
    hi_task_unlock();
}
