/*
 * Copyright (c) 2020 HiHope Community.
 * Description: traffic light demo
 * Author: HiSpark Product Team
 * Create: 2020-6-2
 */
#include <hi_io.h>
#include <hi_early_debug.h>
#include <hi_gpio.h>
#include <hi_task.h>
#include <hi_uart.h>
#include <hi_time.h>
#include <hi_sem.h>
#include <hi_watchdog.h>
#include <app_demo_multi_sample.h>
#include <hi_pwm.h>
#include <app_demo_traffic_sample.h>
#include "hi_i2c.h"
#include "iot_profile.h"

hi_u8 g_traffic_human = 0;
hi_u8 g_led_count_flag = 0;

#define HUMAN_MODU_RED_COUNT_SECOND (30)
#define COUNT_FIVE_SECOND (5)
#define COUNT_THREE_SECOND_FLASH (3)

#define COUNT_MAX_SECOND (4294967296)
extern hi_u8 oc_beep_status;
static hi_u32 s_red_led_auto_modu_time_count = 0;
static hi_u32 s_yellow_led_auto_modu_time_count = 0;
static hi_u32 s_green_led_auto_modu_time_count = 0;

static hi_u32 s_red_led_human_modu_time_count = 0;
static hi_u32 s_yellow_led_human_modu_time_count = 0;
static hi_u32 s_green_led_human_modu_time_count = 0;

hi_u32 g_red_led_auto_modu_time_count = 0;
hi_u32 g_yellow_led_auto_modu_time_count = 0;
hi_u32 g_green_led_auto_modu_time_count = 0;

hi_u32 g_red_led_human_modu_time_count = 0;
hi_u32 g_yellow_led_human_modu_time_count = 0;
hi_u32 g_green_led_human_modu_time_count = 0;
static hi_u8 hi3861_board_led_test = 0;
/*
    @bref traffic control mode 
    @param hi_void
*/
hi_void traffic_control_mode_sample(hi_void)
{
    hi_pwm_init(HI_PWM_PORT_PWM0);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_PWM0_OUT);
    hi_gpio_set_dir(HI_IO_NAME_GPIO_9, HI_GPIO_DIR_OUT);

    hi_u8 current_type = g_current_type;
    hi_u8 current_mode = g_current_mode;
    hi_u8 beep = BEEP_OFF;

    switch (g_current_type)
    {
    case RED_ON:
        gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_10_GPIO);
        gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_11_GPIO);
        gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_12_GPIO);
        break;
    case YELLOW_ON:
        gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
        gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_11_GPIO);
        gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_12_GPIO);
        break;
    case GREEN_ON:
        gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_12_GPIO);
        gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
        gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_11_GPIO);
        break;
    default:
        break;
    }

    while (1)
    {
        if ((current_mode != g_current_mode) || (current_type != g_current_type))
        {
            break;
        }
        if (oc_beep_status == BEEP_ON)
        {
            if (beep == BEEP_OFF)
            {
                beep = BEEP_ON;
                hi_pwm_start(HI_PWM_PORT_PWM0, PWM_DUTY, PWM_FULL_DUTY);
                hi_udelay(DELAY_1_s);
            }
            else
            {
                beep = BEEP_OFF;
                hi_pwm_start(HI_PWM_PORT_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY);
                hi_udelay(DELAY_1_s);
            }
        }
        else
        {
            hi_pwm_start(HI_PWM_PORT_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY);
        }

        hi_sleep(SLEEP_1_MS); //1ms
    }
}

/*
    @bref delay time and fresh screen 
    @param hi_u32 delay_time :delay unit us
           hi_u8 beep_status: 0/1, 0:beep off, 1:beep on
*/
hi_u8 delay_and_fresh_screen(hi_u32 delay_time, hi_u8 beep_status)
{
    hi_u32 i, cycle_count;
    hi_u8 current_mode;
    hi_u8 current_type;
    hi_u8 beep = BEEP_OFF;
    hi_u8 time_count = delay_time / DELAY_1_s;
    hi_u8 time_count_buff[2] = {0};

    cycle_count = delay_time / 20000;
    current_mode = g_current_mode;
    current_type = g_current_type;
    for (i = 0; i < cycle_count; i++)
    {
        hi_udelay(DELAY_10_MS); //10ms
        switch (g_led_count_flag)
        {
        case RED_COUNT:
            if ((i % LIGHT_ONE_SECOND_INTERVAL) == DEFAULT_TYPE)
            {
                if (time_count >= TIME_PEROID_COUNT)
                {
                    if ((current_mode != g_current_mode) || (current_type != g_current_type))
                    {
                        return KEY_DOWN_INTERRUPT;
                    }
                    time_count_buff[0] = (time_count / 10 + 0x30);
                    time_count_buff[1] = time_count % 10 + 0x30;
                    if (oc_beep_status == BEEP_ON)
                    {
                        oled_show_str(120, 7, "1", 1);
                    }
                    else
                    {
                        oled_show_str(120, 7, "0", 1);
                    }
                    oled_show_str(16, 7, time_count_buff, 1);
                    hi_sleep(SLEEP_1_MS); //1ms
                }
                else
                {
                    switch (time_count)
                    {
                    case NINE:
                        if (oc_beep_status == BEEP_ON)
                        {
                            oled_show_str(0, 7, "R:9 Y:0 G:0 B:1 ", 1);
                        }
                        else
                        {
                            oled_show_str(0, 7, "R:9 Y:0 G:0 B:0 ", 1);
                        }
                        break;
                    case EIGHT:
                        if (oc_beep_status == BEEP_ON)
                        {
                            oled_show_str(0, 7, "R:8 Y:0 G:0 B:1 ", 1);
                        }
                        else
                        {
                            oled_show_str(0, 7, "R:8 Y:0 G:0 B:0 ", 1);
                        }
                        break;
                    case SEVEN:
                        if (oc_beep_status == BEEP_ON)
                        {
                            oled_show_str(0, 7, "R:7 Y:0 G:0 B:1 ", 1);
                        }
                        else
                        {
                            oled_show_str(0, 7, "R:7 Y:0 G:0 B:0 ", 1);
                        }
                        break;
                    case SIX:
                        if (oc_beep_status == BEEP_ON)
                        {
                            oled_show_str(0, 7, "R:6 Y:0 G:0 B:1 ", 1);
                        }
                        else
                        {
                            oled_show_str(0, 7, "R:6 Y:0 G:0 B:0 ", 1);
                        }
                        break;
                    case FIVE:
                        if (oc_beep_status == BEEP_ON)
                        {
                            oled_show_str(0, 7, "R:5 Y:0 G:0 B:1 ", 1);
                        }
                        else
                        {
                            oled_show_str(0, 7, "R:5 Y:0 G:0 B:0 ", 1);
                        }
                        break;
                    case FOUR:
                        if (oc_beep_status == BEEP_ON)
                        {
                            oled_show_str(0, 7, "R:4 Y:0 G:0 B:1 ", 1);
                        }
                        else
                        {
                            oled_show_str(0, 7, "R:4 Y:0 G:0 B:0 ", 1);
                        }
                        break;
                    case THREE:
                        if (oc_beep_status == BEEP_ON)
                        {
                            oled_show_str(0, 7, "R:3 Y:0 G:0 B:1 ", 1);
                        }
                        else
                        {
                            oled_show_str(0, 7, "R:3 Y:0 G:0 B:0 ", 1);
                        }
                        break;
                    case TWO:
                        if (oc_beep_status == BEEP_ON)
                        {
                            oled_show_str(0, 7, "R:2 Y:0 G:0 B:1 ", 1);
                        }
                        else
                        {
                            oled_show_str(0, 7, "R:2 Y:0 G:0 B:0 ", 1);
                        }
                        break;
                    case ONE:
                        if (oc_beep_status == BEEP_ON)
                        {
                            oled_show_str(0, 7, "R:1 Y:0 G:0 B:1 ", 1);
                        }
                        else
                        {
                            oled_show_str(0, 7, "R:1 Y:0 G:0 B:0 ", 1);
                        }
                        break;
                    default:
                        break;
                    }
                }
                if (current_mode != g_current_mode)
                {
                    return KEY_DOWN_INTERRUPT;
                }
                hi_sleep(SLEEP_1_MS); //1ms
                if (time_count > INIT_TIME_COUNT)
                {
                    time_count--;
                }
            }
            break;
        case YELLOW_COUNT:
            if (current_mode != g_current_mode)
            {
                return KEY_DOWN_INTERRUPT;
            }
            hi_sleep(SLEEP_1_MS); //1ms
            break;
        case GREEN_COUNT:
            if (current_mode != g_current_mode)
            {
                return KEY_DOWN_INTERRUPT;
            }
            hi_sleep(SLEEP_1_MS); //1ms
            if ((i % LIGHT_ONE_SECOND_INTERVAL) == DEFAULT_TYPE)
            {
                time_count_buff[0] = time_count + 0x30;
                oled_position_clean_screen(0x00, 7, 81, 8);
                oled_show_str(81, 7, &time_count_buff[0], 1);
                if (time_count > INIT_TIME_COUNT)
                {
                    time_count--;
                }
            }
            break;
        case DEFAULT:
            if (current_mode != g_current_mode)
            {
                return KEY_DOWN_INTERRUPT;
            }
            hi_sleep(SLEEP_1_MS); //1ms
            break;
        default:
            break;
        }

        if ((current_mode != g_current_mode) || (current_type != g_current_type))
        {
            return KEY_DOWN_INTERRUPT;
        }
        if (oc_beep_status == BEEP_ON)
        {
            switch (beep_status)
            {
            case BEEP_BY_ONE_SECOND:
                if ((i % BEEP_ONE_SECOND_INTERVAL) == DEFAULT_TYPE)
                {
                    if (beep == BEEP_ON)
                    {
                        hi_pwm_start(HI_PWM_PORT_PWM0, PWM_DUTY, PWM_FULL_DUTY);
                        beep = BEEP_OFF;
                    }
                    else
                    {
                        hi_pwm_start(HI_PWM_PORT_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY);
                        beep = BEEP_ON;
                    }
                }
                break;
            case BEEP_BY_HALF_SECOND:
                if ((i % BEEP_HALF_SECOND_INTERVAL) == DEFAULT_TYPE)
                {
                    if (beep == BEEP_ON)
                    {
                        hi_pwm_start(HI_PWM_PORT_PWM0, PWM_DUTY, PWM_FULL_DUTY);
                        beep = BEEP_OFF;
                    }
                    else
                    {
                        hi_pwm_start(HI_PWM_PORT_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY);
                        beep = BEEP_ON;
                    }
                }
                break;
            case BEEP_BY_QUARTER_SECOND:
                if ((i % BEEP_QUARTER_SECOND_INTERVAL) == DEFAULT_TYPE)
                {
                    if (beep == BEEP_ON)
                    {
                        hi_pwm_start(HI_PWM_PORT_PWM0, PWM_DUTY, PWM_FULL_DUTY);
                        beep = BEEP_OFF;
                    }
                    else
                    {
                        hi_pwm_start(HI_PWM_PORT_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY);
                        beep = BEEP_ON;
                    }
                }
                break;
            default:
                break;
            }
        }
        else
        {
            hi_pwm_start(HI_PWM_PORT_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY);
        }

        if (g_current_mode == TRAFFIC_AUTO_MODE)
        {
            current_type = g_current_type;
        }

        if ((current_mode != g_current_mode) || (current_type != g_current_type))
        {
            return KEY_DOWN_INTERRUPT;
        }
    }
    return HI_NULL;
}
hi_u8 traffic_auto_mode_current = TRAFFIC_AUTO_MODE;
/*traffic auto mode */
hi_void traffic_auto_mode_sample(hi_void)
{
    hi_s32 i = 0;
    hi_u8 _time_count = TRAFFIC_AUTO_MODE_TIME_COUNT;

    hi_pwm_init(HI_PWM_PORT_PWM0);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_PWM0_OUT);
    hi_gpio_set_dir(HI_IO_NAME_GPIO_9, HI_GPIO_DIR_OUT);

    while (1)
    {
        //红灯亮5秒
        g_led_count_flag = RED_COUNT;
        gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_10_GPIO);
        if (delay_and_fresh_screen(DELAY_5_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
        {
            gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
            return;
        }
        if (s_red_led_auto_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_red_led_auto_modu_time_count = 0;
        }
        s_red_led_auto_modu_time_count += COUNT_FIVE_SECOND;
        g_red_led_auto_modu_time_count = s_red_led_auto_modu_time_count;
        //红灯闪3秒
        g_led_count_flag = DEFAULT;
        _time_count = TRAFFIC_AUTO_MODE_TIME_COUNT;
        for (i = 0; i < RED_LIGHT_FLASH_TIME; i++)
        {
            switch (_time_count)
            {
            case 3:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:3 Y:0 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:3 Y:0 G:0 B:0 ", 1);
                }
                break;
            case 2:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:2 Y:0 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:2 Y:0 G:0 B:0 ", 1);
                }

                break;
            case 1:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:1 Y:0 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:1 Y:0 G:0 B:0 ", 1);
                }
                break;
            default:
                break;
            }
            if (_time_count > RED_COUNT)
            {
                _time_count--;
            }

            gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_10_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
                return;
            }

            gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                return;
            }
        }
        hi_sleep(SLEEP_1_MS); //1ms
        if (oc_beep_status == BEEP_ON)
        {
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:1 ", 1);
        }
        else
        {
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:0 ", 1);
        }

        if (s_red_led_auto_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_red_led_auto_modu_time_count = 0;
        }
        s_red_led_auto_modu_time_count += COUNT_THREE_SECOND_FLASH;
        g_red_led_auto_modu_time_count = s_red_led_auto_modu_time_count;
        //黄灯闪3秒
        g_led_count_flag = DEFAULT;
        _time_count = TRAFFIC_AUTO_MODE_TIME_COUNT;
        for (i = 0; i < YELLOW_LIGHT_FLASH_TIME; i++)
        {
            switch (_time_count)
            {
            case THREE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:3 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:3 G:0 B:0 ", 1);
                }
                break;
            case TWO:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:2 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:2 G:0 B:0 ", 1);
                }
                break;
            case ONE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:1 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:1 G:0 B:0 ", 1);
                }
                break;
            default:
                break;
            }

            if (_time_count > RED_COUNT)
            {
                _time_count--;
            }

            gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_12_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_12_GPIO);
                return;
            }

            gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_12_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                return;
            }
        }

        if (s_yellow_led_auto_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_yellow_led_auto_modu_time_count = 0;
        }
        s_yellow_led_auto_modu_time_count += COUNT_THREE_SECOND_FLASH;
        g_yellow_led_auto_modu_time_count = s_yellow_led_auto_modu_time_count;
        hi_sleep(SLEEP_1_MS); //1ms
        if (oc_beep_status == BEEP_ON)
        {
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:1 ", 1);
        }
        else
        {
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:0 ", 1);
        }

        //绿灯亮5秒
        g_led_count_flag = GREEN_COUNT;
        gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_11_GPIO);
        if (delay_and_fresh_screen(DELAY_5_s, BEEP_BY_ONE_SECOND) == KEY_DOWN_INTERRUPT)
        {
            gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_11_GPIO);
            return;
        }

        if (s_green_led_auto_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_green_led_auto_modu_time_count = 0;
        }
        s_green_led_auto_modu_time_count += COUNT_FIVE_SECOND;
        g_green_led_auto_modu_time_count = s_green_led_auto_modu_time_count;

        hi_sleep(SLEEP_1_MS); //1ms
        //绿灯闪3秒
        g_led_count_flag = DEFAULT;
        _time_count = TRAFFIC_AUTO_MODE_TIME_COUNT;
        for (i = 0; i < GREEN_LIGHT_FLASH_TIME; i++)
        {
            switch (_time_count)
            {
            case THREE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:3 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:3 B:0 ", 1);
                }

                break;
            case TWO:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:2 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:2 B:0 ", 1);
                }
                break;
            case ONE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:1 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:1 B:0 ", 1);
                }
                break;
            default:
                break;
            }

            if (_time_count > RED_COUNT)
            {
                _time_count--;
            }

            gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_11_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_QUARTER_SECOND) == KEY_DOWN_INTERRUPT)
            {
                gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_11_GPIO);
                return;
            }

            gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_11_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_QUARTER_SECOND) == KEY_DOWN_INTERRUPT)
            {
                return;
            }
        }
        if (s_green_led_auto_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_green_led_auto_modu_time_count = 0;
        }
        s_green_led_auto_modu_time_count += COUNT_THREE_SECOND_FLASH;
        g_green_led_auto_modu_time_count = s_green_led_auto_modu_time_count;
        hi_sleep(SLEEP_1_MS); //1ms
    }
}
/* traffic normal type */
hi_void traffic_normal_type(hi_void)
{
    hi_u8 normal_time_count = TRAFFIC_HUMAN_MODE_NORMAL_TIME_COUNT;

    while (1)
    {
        //红灯亮30秒
        g_led_count_flag = RED_COUNT;
        gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_10_GPIO);
        if (delay_and_fresh_screen(DELAY_30_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
        {
            gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
            return;
        }

        if (s_red_led_human_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_red_led_human_modu_time_count = 0;
        }
        s_red_led_human_modu_time_count += HUMAN_MODU_RED_COUNT_SECOND;
        g_red_led_human_modu_time_count = s_red_led_human_modu_time_count;
        hi_sleep(SLEEP_1_MS); //1ms
        //红灯闪3秒
        g_led_count_flag = DEFAULT;
        normal_time_count = TRAFFIC_HUMAN_MODE_NORMAL_TIME_COUNT;
        for (int i = 0; i < RED_LIGHT_FLASH_TIME; i++)
        {
            switch (normal_time_count)
            {
            case THREE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:3 Y:0 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:3 Y:0 G:0 B:0 ", 1);
                }
                break;
            case TWO:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:2 Y:0 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:2 Y:0 G:0 B:0 ", 1);
                }
                break;
            case ONE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:1 Y:0 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:1 Y:0 G:0 B:0 ", 1);
                }
                break;
            default:
                break;
            }

            if (normal_time_count > RED_COUNT)
            {
                normal_time_count--;
            }

            gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_10_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
                return;
            }

            gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                break;
            }
        }
        if (s_red_led_human_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_red_led_human_modu_time_count = 0;
        }
        s_red_led_human_modu_time_count += COUNT_THREE_SECOND_FLASH;
        g_red_led_human_modu_time_count = s_red_led_human_modu_time_count;
        hi_sleep(SLEEP_1_MS); //1ms
        //黄灯闪3秒
        g_led_count_flag = DEFAULT;
        normal_time_count = TRAFFIC_HUMAN_MODE_NORMAL_TIME_COUNT;
        for (int i = 0; i < YELLOW_LIGHT_FLASH_TIME; i++)
        {
            switch (normal_time_count)
            {
            case THREE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:3 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:3 G:0 B:0 ", 1);
                }

                break;
            case TWO:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:2 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:2 G:0 B:0 ", 1);
                }

                break;
            case ONE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:1 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:1 G:0 B:0 ", 1);
                }

                break;
            default:
                break;
            }

            if (normal_time_count > RED_COUNT)
            {
                normal_time_count--;
            }

            gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_12_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_12_GPIO);
                return;
            }

            gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_12_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                return;
            }
        }
        if (s_yellow_led_human_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_yellow_led_human_modu_time_count = 0;
        }
        s_yellow_led_human_modu_time_count += COUNT_THREE_SECOND_FLASH;
        g_yellow_led_human_modu_time_count = s_yellow_led_human_modu_time_count;
        if (oc_beep_status == BEEP_ON)
        {
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:1 ", 1);
        }
        else
        {
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:0 ", 1);
        }

        hi_sleep(SLEEP_1_MS); //10ms
        //绿灯亮5秒
        g_led_count_flag = GREEN_COUNT;
        gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_11_GPIO);
        if (delay_and_fresh_screen(DELAY_5_s, BEEP_BY_ONE_SECOND) == KEY_DOWN_INTERRUPT)
        {
            gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_11_GPIO);
            break;
        }
        if (s_green_led_human_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_green_led_human_modu_time_count = 0;
        }
        s_green_led_human_modu_time_count += COUNT_FIVE_SECOND;
        g_green_led_human_modu_time_count = s_green_led_human_modu_time_count;
        hi_sleep(SLEEP_1_MS); //10ms
        //绿灯闪3秒
        g_led_count_flag = DEFAULT;
        normal_time_count = TRAFFIC_HUMAN_MODE_NORMAL_TIME_COUNT;
        for (int i = 0; i < GREEN_LIGHT_FLASH_TIME; i++)
        {
            switch (normal_time_count)
            {
            case THREE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:3 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:3 B:0 ", 1);
                }

                break;
            case TWO:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:2 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:2 B:0 ", 1);
                }

                break;
            case ONE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:1 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:1 B:0 ", 1);
                }

                break;
            default:
                break;
            }
            if (normal_time_count > RED_COUNT)
            {
                normal_time_count--;
            }

            hi_sleep(SLEEP_1_MS); //10ms

            gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_11_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_QUARTER_SECOND) == KEY_DOWN_INTERRUPT)
            {
                gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_11_GPIO);
                return;
            }

            gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_11_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_QUARTER_SECOND) == KEY_DOWN_INTERRUPT)
            {
                return;
            }
        }

        if (s_green_led_human_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_green_led_human_modu_time_count = 0;
        }
        s_green_led_human_modu_time_count += COUNT_THREE_SECOND_FLASH;
        g_green_led_human_modu_time_count = s_green_led_human_modu_time_count;
        if (oc_beep_status == BEEP_ON)
        {
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:1 ", 1);
        }
        else
        {
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:0 ", 1);
        }
    }
}
/*
    @bref traffic human type
    @param hi_void
*/
hi_void traffic_human_type(hi_void)
{
    hi_u8 current_mode = g_current_mode;
    hi_u8 current_type = g_current_type;

    while (1)
    {
        hi_u8 human_time_count = TRAFFIC_HUMAN_MODE_TIME_CONUT;
        //红灯闪3秒
        g_led_count_flag = DEFAULT;
        for (int i = 0; i < RED_LIGHT_FLASH_TIME; i++)
        {
            switch (human_time_count)
            {
            case THREE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:3 Y:0 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:3 Y:0 G:0 B:0 ", 1);
                }
                break;
            case TWO:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:2 Y:0 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:2 Y:0 G:0 B:0 ", 1);
                }
                break;
            case ONE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:1 Y:0 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:1 Y:0 G:0 B:0 ", 1);
                }
                break;
            default:
                break;
            }

            if (human_time_count > RED_COUNT)
            {
                human_time_count--;
            }

            hi_sleep(SLEEP_1_MS); //1ms

            gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_10_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
                return;
            }

            gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                return;
            }
        }

        if (s_red_led_human_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_red_led_human_modu_time_count = 0;
        }
        s_red_led_human_modu_time_count += COUNT_THREE_SECOND_FLASH;
        g_red_led_human_modu_time_count = s_red_led_human_modu_time_count;

        //黄灯闪3秒
        g_led_count_flag = DEFAULT;
        human_time_count = TRAFFIC_HUMAN_MODE_TIME_CONUT;
        for (int i = 0; i < YELLOW_LIGHT_FLASH_TIME; i++)
        {
            if ((current_mode != g_current_mode) || (current_type != g_current_type))
            {
                return;
            }
            switch (human_time_count)
            {
            case THREE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:3 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:3 G:0 B:0 ", 1);
                }
                break;
            case TWO:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:2 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:2 G:0 B:0 ", 1);
                }
                break;
            case ONE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:1 G:0 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:1 G:0 B:0 ", 1);
                }
                break;
            default:
                break;
            }
            if (human_time_count > RED_COUNT)
            {
                human_time_count--;
            }

            hi_sleep(SLEEP_1_MS); //10ms

            gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_12_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_12_GPIO);
                return;
            }

            gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_12_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_HALF_SECOND) == KEY_DOWN_INTERRUPT)
            {
                return;
            }
        }

        if (s_yellow_led_human_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_yellow_led_human_modu_time_count = 0;
        }
        s_yellow_led_human_modu_time_count += COUNT_THREE_SECOND_FLASH;
        g_yellow_led_human_modu_time_count = s_yellow_led_human_modu_time_count;
        if (oc_beep_status == BEEP_ON)
        {
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:1 ", 1);
        }
        else
        {
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:0 ", 1);
        }

        //绿灯亮5秒
        g_led_count_flag = GREEN_COUNT;
        gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_11_GPIO);
        if (delay_and_fresh_screen(DELAY_5_s, BEEP_BY_ONE_SECOND) == KEY_DOWN_INTERRUPT)
        {
            gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_11_GPIO);
            return;
        }

        if (s_green_led_human_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_green_led_human_modu_time_count = 0;
        }
        s_green_led_human_modu_time_count += COUNT_FIVE_SECOND;
        g_green_led_human_modu_time_count = s_green_led_human_modu_time_count;
        //绿灯闪3秒
        g_led_count_flag = DEFAULT;
        human_time_count = TRAFFIC_HUMAN_MODE_TIME_CONUT;
        for (int i = 0; i < GREEN_LIGHT_FLASH_TIME; i++)
        {
            switch (human_time_count)
            {
            case THREE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:3 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:3 B:0 ", 1);
                }

                break;
            case TWO:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:2 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:2 B:0 ", 1);
                }

                break;
            case ONE:
                oled_show_str(0, 7, "                ", 1);
                if (oc_beep_status == BEEP_ON)
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:1 B:1 ", 1);
                }
                else
                {
                    oled_show_str(0, 7, "R:0 Y:0 G:1 B:0 ", 1);
                }

                break;
            default:
                break;
            }

            if (human_time_count > RED_COUNT)
            {
                human_time_count--;
            }

            hi_sleep(SLEEP_1_MS); //10ms

            gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_11_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_QUARTER_SECOND) == KEY_DOWN_INTERRUPT)
            {
                gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_11_GPIO);
                return;
            }

            gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_11_GPIO);
            if (delay_and_fresh_screen(DELAY_1_s, BEEP_BY_QUARTER_SECOND) == KEY_DOWN_INTERRUPT)
            {
                return;
            }
        }

        if (s_green_led_human_modu_time_count >= COUNT_MAX_SECOND)
        {
            s_green_led_human_modu_time_count = 0;
        }
        s_green_led_human_modu_time_count += COUNT_THREE_SECOND_FLASH;
        g_green_led_human_modu_time_count = s_green_led_human_modu_time_count;
        /*回到正常模式*/
        // g_current_type = TRAFFIC_NORMAL_TYPE;
        // return;
    }
}
/* traffic human mode */
hi_void traffic_human_mode_sample(hi_void)
{
    hi_pwm_init(HI_PWM_PORT_PWM0);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_PWM0_OUT);
    hi_gpio_set_dir(HI_IO_NAME_GPIO_9, HI_GPIO_DIR_OUT);

    switch (g_current_type)
    {
    case TRAFFIC_NORMAL_TYPE:
        traffic_normal_type();
        break;
    case TRAFFIC_HUMAN_TYPE:
        traffic_human_type();
        break;
    default:
        break;
    }
}

/* traffic light function handle and  display */
hi_void traffic_light_func(hi_void)
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
            FACTORY_HISPARK_BOARD_CL_TL_TEST("-----------HiSpark board test----------");
        }
        hi_sleep(SLEEP_1S);
    }
    /*按键中断初始化*/
    extern hi_void test_gpio_init(hi_void);
    test_gpio_init();
    //clean screen
    oled_fill_screen(OLED_CLEAN_SCREEN);
    oled_show_str(0, 0, "WiFi-AP  ON  U:1", 1);
    oled_show_str(0, 1, "                ", 1);
    oled_show_str(0, 2, " Traffic Light  ", 1);
    oled_show_str(0, 3, "                ", 1);
    oled_show_str(0, 4, "Mode:           ", 1);
    oled_show_str(0, 5, "1.Control mode: ", 1);
    oled_show_str(0, 6, "                ", 1);
    oled_show_str(0, 7, "                ", 1);
    g_current_type = KEY_UP;
    while (1)
    {
        switch (g_current_mode)
        {
        case TRAFFIC_CONTROL_MODE:                          // use this control mode ======================================
            oled_show_str(0, 0, "WiFi-AP OFF  U:0", 1);
            oled_show_str(0, 1, "                ", 1);
            oled_show_str(0, 2, " Traffic Light  ", 1);
            oled_show_str(0, 3, "                ", 1);
            oled_show_str(0, 4, "Mode:           ", 1);
            oled_show_str(0, 5, "1.Control mode: ", 1);
            oled_show_str(0, 6, "                ", 1);
            oled_show_str(0, 7, "                ", 1);
            switch (g_current_type)
            {
            case RED_ON:
                oled_show_str(0, 7, "1.Red On       ", 1);  // 第0列、第7行开始写
                break;
            case YELLOW_ON:
                oled_show_str(0, 7, "2.Yellow On     ", 1);
                break;
            case GREEN_ON:
                oled_show_str(0, 7, "3.Green On      ", 1);
                break;
            default:
                break;
            }
            traffic_control_mode_sample();
            break;
        case TRAFFIC_AUTO_MODE:
            oled_show_str(0, 4, "Mode:           ", 1);
            oled_show_str(0, 5, "2.Auto mode:    ", 1);
            oled_show_str(0, 6, "                ", 1);
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:0 ", 1);
            all_led_off();
            traffic_auto_mode_sample();
            break;
        case TRAFFIC_HUMAN_MODE:
            oled_show_str(0, 4, "Mode:           ", 1);
            oled_show_str(0, 5, "3.Human mode:   ", 1);
            oled_show_str(0, 6, "                ", 1);
            oled_show_str(0, 7, "R:0 Y:0 G:0 B:0 ", 1);
            all_led_off();
            traffic_human_mode_sample();
            break;
        case TRAFFIC_RETURN_MODE:
            oled_show_str(0, 4, "Return Menu     ", 1);
            oled_show_str(0, 5, "                ", 1);
            oled_show_str(0, 6, "                ", 1);
#ifdef CONFIG_ALL_DEMO
            oled_show_str(0, 7, "Continue   Exit ", 1);
#else
            oled_show_str(0, 7, "Continue        ", 1);
#endif
            all_led_off();
            hi_pwm_start(HI_PWM_PORT_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY); //关闭beep
#ifdef CONFIG_ALL_DEMO
            return_main_enum_sample();
            if (g_current_type == KEY_DOWN)
            {
                g_current_mode = MAIN_FUNCTION_SELECT_MODE;
                all_led_off();
                hi_pwm_start(HI_PWM_PORT_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY); //关闭beep

                oled_main_menu_display();
                return;
            }
            g_current_mode = MAIN_FUNCTION_SELECT_MODE;
#endif
            break;
        default:
            break;
        }
    }
}