/*
 * Copyright (c) 2020 HiHope Community.
 * Description: colorful light demo
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */
#include <app_demo_multi_sample.h>

hi_u8 g_menu_type = 0;
hi_u8 g_menu_select = TRAFFIC_LIGHT_MENU;
hi_u8 g_current_mode = CONTROL_MODE;
hi_u8 g_current_type = 0;
hi_u8 g_someone_walking_flag = 0;
hi_u8 g_with_light_flag = 0;

hi_u8 g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
hi_u8 g_key_down_flag = KEY_UP;

hi_u32 g_gpio5_tick = KEY_UP;
hi_u32 g_gpio7_tick = KEY_UP;
hi_u32 g_gpio8_tick = KEY_UP;
hi_u8 g_gpio9_tick = 0;
hi_u32 g_gpio7_first_key_dwon = HI_TRUE;
hi_u8 g_gpio8_current_type = 0;
hi_u8 g_beep_control_type = 0;
extern hi_u8 oc_beep_status;
extern hi_u8 get_light_status(hi_void);
extern hi_u8 get_gpio5_voltage(hi_void);
/*factory test HiSpark board*/
hi_void hispark_board_test(hi_gpio_value value)
{
    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_9, HI_GPIO_DIR_OUT);
    hi_gpio_set_ouput_val(HI_GPIO_IDX_9, value);
}
/*gpio init*/
hi_void gpio_control(hi_io_name gpio, hi_gpio_idx id, hi_gpio_dir dir, hi_gpio_value gpio_val, hi_u8 val)
{
    hi_io_set_func(gpio, val);
    hi_gpio_set_dir(id, dir);
    hi_gpio_set_ouput_val(id, gpio_val);
}
/*pwm init*/
hi_void pwm_init(hi_io_name id, hi_u8 val, hi_pwm_port port)
{
    hi_io_set_func(id, val); /* PWM0 OUT */
    hi_pwm_init(port);
    hi_pwm_set_clock(PWM_CLK_160M);
}
/* gpio key init*/
hi_void hi_switch_init(hi_io_name id, hi_u8 val, hi_gpio_idx idx, hi_gpio_dir dir, hi_io_pull pval)
{
    hi_io_set_func(id, val);
    hi_gpio_set_dir(idx, dir);
    hi_io_set_pull(id, pval);
}

/*
VCC：5V
blue:  gpio12 yellow
green: gpio11 green 
red:   gpio10 red
switch ：
gpio7 switch1
gpio5 switch2
*/
hi_void test_gpio_init(hi_void)
{
    hi_switch_init(HI_IO_NAME_GPIO_5, HI_IO_FUNC_GPIO_5_GPIO, HI_GPIO_IDX_5, HI_GPIO_DIR_IN, HI_IO_PULL_UP); //switch2
    hi_switch_init(HI_IO_NAME_GPIO_8, HI_IO_FUNC_GPIO_8_GPIO, HI_GPIO_IDX_8, HI_GPIO_DIR_IN, HI_IO_PULL_UP); //switch2

    pwm_init(HI_IO_NAME_GPIO_10, HI_IO_FUNC_GPIO_10_PWM1_OUT, HI_PWM_PORT_PWM1);
    pwm_init(HI_IO_NAME_GPIO_11, HI_IO_FUNC_GPIO_11_PWM2_OUT, HI_PWM_PORT_PWM2);
    pwm_init(HI_IO_NAME_GPIO_12, HI_IO_FUNC_GPIO_12_PWM3_OUT, HI_PWM_PORT_PWM3);
}

/*
三色灯控制模式：每按一下类型S1按键，在绿灯亮、黄灯亮、红灯亮，三个状态之间切换
mode:
1.control mode

type:
1.red on
2.yellow on
3.green on
RED_ON = 1,
    YELLOW_ON,
    GREEN_ON,
*/
hi_void control_mode_sample(hi_void)
{
    hi_u8 current_type = g_current_type;
    hi_u8 current_mode = g_current_mode;
    while (1)
    {
        switch (g_current_type)
        {
        case RED_ON:
            hi_pwm_start(HI_PWM_PORT_PWM1, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);
            hi_pwm_start(HI_PWM_PORT_PWM2, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            hi_pwm_start(HI_PWM_PORT_PWM3, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            break;
        case YELLOW_ON:
            hi_pwm_start(HI_PWM_PORT_PWM1, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            hi_pwm_start(HI_PWM_PORT_PWM2, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);
            hi_pwm_start(HI_PWM_PORT_PWM3, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            break;
        case GREEN_ON:
            hi_pwm_start(HI_PWM_PORT_PWM1, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            hi_pwm_start(HI_PWM_PORT_PWM2, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            hi_pwm_start(HI_PWM_PORT_PWM3, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);
            break;
        default:
            break;
        }

        if ((current_mode != g_current_mode) || (current_type != g_current_type))
        {
            all_light_out();
            break;
        }
        hi_sleep(SLEEP_1_MS);
    }
}

/*中断检测函数，每10ms检测一次*/
hi_u8 delay_and_check_key_interrupt(hi_u32 delay_time)
{
    hi_u32 i = 0;
    hi_u32 cycle_count = 0;
    hi_u8 current_mode = 0;
    hi_u8 current_type = 0;

    cycle_count = delay_time / DELAY_10_MS;
    current_mode = g_current_mode;
    current_type = g_current_type;

    for (i = 0; i < cycle_count; i++)
    {
        if ((current_mode != g_current_mode) || (current_type != g_current_type))
        {
            return KEY_DOWN_INTERRUPT;
        }
        hi_udelay(DELAY_5_MS); //10ms
        hi_sleep(SLEEP_1_MS);
    }
    return HI_NULL;
}

/*红、黄、绿每1秒轮流亮*/
hi_void cycle_for_one_second(hi_void)
{
    while (1)
    {
        hi_pwm_start(HI_PWM_PORT_PWM1, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);

        if (delay_and_check_key_interrupt(DELAY_1_s) == KEY_DOWN_INTERRUPT)
        {
            hi_pwm_start(HI_PWM_PORT_PWM1, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            break;
        }

        hi_pwm_start(HI_PWM_PORT_PWM1, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
        hi_pwm_start(HI_PWM_PORT_PWM2, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);

        if (delay_and_check_key_interrupt(DELAY_1_s) == KEY_DOWN_INTERRUPT)
        {
            hi_pwm_start(HI_PWM_PORT_PWM2, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            break;
        }

        hi_pwm_start(HI_PWM_PORT_PWM2, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
        hi_pwm_start(HI_PWM_PORT_PWM3, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);

        if (delay_and_check_key_interrupt(DELAY_1_s) == KEY_DOWN_INTERRUPT)
        {
            hi_pwm_start(HI_PWM_PORT_PWM3, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            break;
        }
        hi_pwm_start(HI_PWM_PORT_PWM3, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
        hi_sleep(SLEEP_1_MS);
    }
}

/*红、黄、绿每0.5秒轮流亮*/
hi_void cycle_for_half_second(hi_void)
{
    while (1)
    {
        hi_pwm_start(HI_PWM_PORT_PWM1, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);

        if (delay_and_check_key_interrupt(DELAY_500_MS) == KEY_DOWN_INTERRUPT)
        {
            hi_pwm_start(HI_PWM_PORT_PWM1, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            break;
        }

        hi_pwm_start(HI_PWM_PORT_PWM1, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
        hi_pwm_start(HI_PWM_PORT_PWM2, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);

        if (delay_and_check_key_interrupt(DELAY_500_MS) == KEY_DOWN_INTERRUPT)
        {
            hi_pwm_start(HI_PWM_PORT_PWM2, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            break;
        }

        hi_pwm_start(HI_PWM_PORT_PWM2, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
        hi_pwm_start(HI_PWM_PORT_PWM3, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);

        if (delay_and_check_key_interrupt(DELAY_500_MS) == KEY_DOWN_INTERRUPT)
        {
            hi_pwm_start(HI_PWM_PORT_PWM3, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            break;
        }
        hi_pwm_start(HI_PWM_PORT_PWM3, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
        hi_sleep(SLEEP_1_MS);
    }
}

/*红、黄、绿每0.25秒轮流亮。*/
hi_void cycle_for_quarter_second(hi_void)
{
    while (1)
    {
        hi_pwm_start(HI_PWM_PORT_PWM1, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);

        if (delay_and_check_key_interrupt(DELAY_250_MS) == KEY_DOWN_INTERRUPT)
        {
            hi_pwm_start(HI_PWM_PORT_PWM1, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            break;
        }

        hi_pwm_start(HI_PWM_PORT_PWM1, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
        hi_pwm_start(HI_PWM_PORT_PWM2, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);

        if (delay_and_check_key_interrupt(DELAY_250_MS) == KEY_DOWN_INTERRUPT)
        {
            hi_pwm_start(HI_PWM_PORT_PWM2, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            break;
        }

        hi_pwm_start(HI_PWM_PORT_PWM2, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
        hi_pwm_start(HI_PWM_PORT_PWM3, PWM_SMALL_DUTY, PWM_MIDDLE_DUTY);

        if (delay_and_check_key_interrupt(DELAY_250_MS) == KEY_DOWN_INTERRUPT)
        {
            hi_pwm_start(HI_PWM_PORT_PWM3, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
            break;
        }
        hi_pwm_start(HI_PWM_PORT_PWM3, PWM_LOW_DUTY, PWM_MIDDLE_DUTY);
        hi_sleep(SLEEP_1_MS);
    }
}

/*
mode:8.Creturn
模式8：返回主界面
*/
hi_void return_main_enum_sample(hi_void)
{
    hi_u8 current_type = g_current_type;
    hi_u8 current_mode = g_current_mode;
    while (1)
    {
        switch (g_current_type)
        {
        case KEY_UP:
            break;
        case KEY_DOWN:
            return;
        default:
            break;
        }

        if (current_mode != g_current_mode)
        {
            break;
        }
        hi_sleep(SLEEP_1_MS);
    }
}
hi_void gpio9_led_light_func(hi_viod)
{
    g_gpio9_tick++;
    if (g_gpio9_tick % 2 == 0)
    {
        printf("led off\r\n");
        hispark_board_test(HI_GPIO_VALUE1);
    }
    else
    {
        printf("led on\r\n");
        hispark_board_test(HI_GPIO_VALUE0);
    }
}
/*
按键5每按下一次，就会产生中断并调用该函数
用来调整模式
*/
hi_void gpio5_isr_func_mode(hi_void)
{
    hi_u32 tick_interval = 0;
    hi_u32 current_gpio5_tick = 0;

    current_gpio5_tick = hi_get_tick();
    tick_interval = current_gpio5_tick - g_gpio5_tick;

    if (tick_interval < KEY_INTERRUPT_PROTECT_TIME)
    {
        return HI_NULL;
    }

    g_gpio5_tick = current_gpio5_tick;
    g_key_down_flag = KEY_GPIO_5;

    printf("gpio5 key down; g_menu_mode = %d, g_current_mode = %d\n", g_menu_mode, g_current_mode);

    if (g_menu_mode == MAIN_FUNCTION_SELECT_MODE)
    {
        g_menu_select++;
        if (g_menu_select >= MAX_FUNCTION_DEMO_NUM)
        {
            g_menu_select = TRAFFIC_LIGHT_MENU;
        }
        return HI_ERR_SUCCESS;
    }

    if (g_menu_mode == SUB_MODE_SELECT_MODE)
    {
        g_current_mode++;
        g_current_type = CONTROL_MODE;
        switch (g_menu_select)
        {
        case TRAFFIC_LIGHT_MENU:
            if (g_current_mode >= MAX_TRAFFIC_LIGHT_MODE)
            {
                g_current_mode = CONTROL_MODE;
            }
            break;
        default:
            break;
        }
    }
}

/*
按键7每按下一次，就会产生中断并调用该函数
用来调整类型
*/
hi_void gpio7_isr_func_type(hi_void)
{
    hi_u32 tick_interval;
    hi_u32 current_gpio7_tick;

    /*按键防抖*/
    current_gpio7_tick = hi_get_tick();
    tick_interval = current_gpio7_tick - g_gpio7_tick;
    printf("gpio7 start = %d, end = %d end-start = %d\n", g_gpio7_tick, current_gpio7_tick, tick_interval);
    if (tick_interval < KEY_INTERRUPT_PROTECT_TIME)
    {
        return HI_NULL;
    }

    g_gpio7_tick = current_gpio7_tick;
    g_key_down_flag = KEY_GPIO_7;

#ifdef CONFIG_COLORFUL_LIGHT
    g_current_type++;
#elif CONFIG_TRAFFIC_LIGHT
    g_current_type++;
#else
    if (g_gpio7_first_key_dwon != HI_TRUE)
    {
        g_current_type++;
    }
    g_gpio7_first_key_dwon = HI_FALSE;
#endif

    if (g_menu_select == TRAFFIC_LIGHT_MENU)
    {
        switch (g_current_mode)
        {
        case TRAFFIC_CONTROL_MODE:
            if (g_current_type >= MAX_TRAFFIC_CONTROL_TYPE)
            {
                g_current_type = TRAFFIC_NORMAL_TYPE;
            }
            break;
        case TRAFFIC_AUTO_MODE:
            if (g_current_type > MAX_TRAFFIC_AUTO_TYPE)
            {
                g_current_type = TRAFFIC_NORMAL_TYPE;
            }
            break;
        case TRAFFIC_HUMAN_MODE:
            if (g_current_type > MAX_TRAFFIC_HUMAN_TYPE)
            {
                g_current_type = TRAFFIC_NORMAL_TYPE;
            }
            break;
        case TRAFFIC_RETURN_MODE:
            if (g_current_type > RETURN_TYPE_MODE)
            {
                g_current_type = KEY_UP;
            }
            break;
        default:
            break;
        }
    }

    printf("gpio7 current mode is %d, type is %d\n", g_current_mode, g_current_type);
}
hi_void gpio8_interrupt(hi_void)
{
    hi_u32 tick_interval = 0;
    hi_u32 current_gpio8_tick = 0;

    current_gpio8_tick = hi_get_tick();
    tick_interval = current_gpio8_tick - g_gpio8_tick;
    printf("gpio8 interrupt \r\n");

    if (tick_interval < KEY_INTERRUPT_PROTECT_TIME)
    {
        printf("gpio8 interrupt return  \r\n");
        return HI_NULL;
    }
    g_gpio8_current_type++;

    if (g_gpio8_current_type % 2 == 0)
    {
        printf("beep off\r\n", g_gpio8_current_type);
        oc_beep_status = BEEP_OFF;
    }
    else
    {
        printf("beep on %d\r\n", g_gpio8_current_type);
        oc_beep_status = BEEP_ON;
    }

    if (g_gpio8_current_type >= 255)
    {
        g_gpio8_current_type = 0;
    }
}
/*
S2(gpio5)是模式按键，S1(gpio7)是类型按键。
1、三色灯控制模式：每按一下类型S1按键，在绿灯亮、黄灯亮、红灯亮，三个状态之间切换
2、炫彩模式，每按一下S2，不同的炫彩类型
3、PWM无极调光模式，每按一下S2，不同的调光类型
4、人体红外检测模式（设置一个好看的灯亮度）
5、光敏检测模式
6、人体红外+光敏联合检测模式。
7、返回模式
创建两个按键中断，按键响应后会调用对应的回调函数
*/
hi_void app_multi_sample_demo(hi_void)
{
    hi_u32 ret = HI_ERR_SUCCESS;

    (hi_void) hi_gpio_init();
    g_gpio5_tick = hi_get_tick();
    ret = hi_gpio_register_isr_function(HI_GPIO_IDX_5, HI_INT_TYPE_EDGE, HI_GPIO_EDGE_FALL_LEVEL_LOW, get_gpio5_voltage, HI_NULL);
    if (ret == HI_ERR_SUCCESS)
    {
        printf(" register gpio 5\r\n");
    }

    g_gpio8_tick = hi_get_tick();
    ret = hi_gpio_register_isr_function(HI_GPIO_IDX_8, HI_INT_TYPE_EDGE, HI_GPIO_EDGE_FALL_LEVEL_LOW, gpio8_interrupt, HI_NULL);
    if (ret == HI_ERR_SUCCESS)
    {
        printf(" register gpio8\r\n");
    }
}