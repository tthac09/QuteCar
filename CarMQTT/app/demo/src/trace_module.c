#include <trace_module.h>
#include <hi_timer.h>
#include <iot_main.h>
#include <ultrasonic_module.h>
extern hi_u8 g_car_control_mode;
extern hi_u8 g_car_status;
extern hi_u8 g_led_control_module;
extern hi_u16 g_car_modular_control_module;
extern hi_u16 g_car_direction_control_module;
extern hi_u16 global_red_on;

extern hi_void regress_middle(hi_void);
extern hi_void car_go_forward(hi_void);
extern hi_void car_stop(hi_void);
extern hi_void car_turn_left(hi_void);
extern hi_void car_turn_right(hi_void);

#ifndef DISTANCE_BETWEEN_CAR_AND_OBSTACLE
#define DISTANCE_BETWEEN_CAR_AND_OBSTACLE (20)
#endif
/*set gpio input mode*/
hi_void set_gpio_input_mode(hi_io_name id, hi_u8 val, hi_gpio_idx idx, hi_gpio_dir dir)
{
    hi_io_set_func(id, val);
    hi_gpio_set_dir(idx, dir);
}

/*
init gpio11/12 as a input io
GPIO 11 connects the left tracking module
GPIO 11 connects the right tracking module
*/
hi_void trace_module_init(hi_void)
{
    //    set_gpio_input_mode(HI_IO_NAME_GPIO_11,HI_IO_FUNC_GPIO_11_GPIO,HI_GPIO_IDX_11,HI_GPIO_DIR_IN);
    //    set_gpio_input_mode(HI_IO_NAME_GPIO_12,HI_IO_FUNC_GPIO_12_GPIO,HI_GPIO_IDX_12,HI_GPIO_DIR_IN);
}

/*get do value*/
// hi_gpio_value get_do_value(hi_gpio_idx idx)
// {
//     hi_gpio_value val = HI_GPIO_VALUE0;
//     hi_u32 ret = hi_gpio_get_input_val(idx, &val);
//     if (ret != HI_ERR_SUCCESS) {
//         return HI_ERR_FAILURE;
//     }
//     return val;
// }

hi_gpio_value get_do_value(hi_adc_channel_index idx)
{
    // hi_gpio_value val = HI_GPIO_VALUE0;
    // hi_u32 ret = hi_gpio_get_input_val(idx, &val);
    // if (ret != HI_ERR_SUCCESS) {
    //     return HI_ERR_FAILURE;
    // }
    // return val;
    hi_u16 data = 0;
    hi_u32 ret = 0;

    for (int i = 0; i < ADC_TEST_LENGTH; i++)
    {
        //printf("________zyh(%d  %d)____________\n", ADC_TEST_LENGTH, __LINE__);
        ret = hi_adc_read(idx, &data, HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, 0xF0); //ADC_Channal_6  自动识别模式  CNcomment:4次平均算法模式 CNend */
        //printf("________zyh(%d)____________\n", data);
        if (ret != HI_ERR_SUCCESS)
        {
            printf("hi_adc_read failed\n");
            // return  HI_NULL;
        }
    }

    if (idx == HI_ADC_CHANNEL_5)
    {
        printf("gpio11 m_left_value is %d \n", data);
    }
    else if (idx == HI_ADC_CHANNEL_0)
    {
        printf("gpio12 m_right_value is %d \n", data);
    }

    if ((data > 1000) && (data < 1900))
    {
        return HI_GPIO_VALUE0;
    }
    else if ((data > 100) && (data < 200))
    {
        return HI_GPIO_VALUE1;
    }
}

/*
According to the data received by the infrared tracking module, 
send the corresponding instructions to the car
*/
#define car_speed_left 100
#define car_speed_right 100
hi_u32 g_car_speed_left = car_speed_left;
hi_u32 g_car_speed_right = car_speed_right;
hi_u8 count = 0;
hi_gpio_value io_status_left;
hi_gpio_value io_status_right;
hi_void timer1_callback(hi_u32 arg)
{
    hi_gpio_value io_status;
    if (g_car_speed_left != car_speed_left)
    {
        count++;
        if (count >= 2)
        {
            hi_gpio_get_input_val(HI_GPIO_IDX_11, &io_status);
            if (io_status != HI_GPIO_VALUE0)
            {
                g_car_speed_left = car_speed_left;
                printf("left speed change \r\n");
                count = 0;
            }
        }
    }
    if (g_car_speed_right != car_speed_right)
    {
        count++;
        if (count >= 2)
        {
            hi_gpio_get_input_val(HI_GPIO_IDX_12, &io_status);
            if (io_status != HI_GPIO_VALUE0)
            {
                g_car_speed_right = car_speed_right;
                printf("right speed change \r\n");
                count = 0;
            }
        }
    }
    if (g_car_speed_left != car_speed_left && g_car_speed_right != car_speed_right)
    {
        g_car_speed_left = car_speed_left;
        g_car_speed_right = car_speed_right;
    }
    hi_gpio_get_input_val(HI_GPIO_IDX_11, &io_status_left);
    hi_gpio_get_input_val(HI_GPIO_IDX_12, &io_status_right);
    // 等于HI_GPIO_VALUE0,灯不亮
    if (io_status_right == HI_GPIO_VALUE0 && io_status_left != HI_GPIO_VALUE0) // 右边识别到黑线，右转
    {
        //printf("========  1  ========\n");
        g_car_speed_left = car_speed_left;
        //g_car_speed_right = 60000;
        g_car_speed_right = 30000;
    }
    if (io_status_right != HI_GPIO_VALUE0 && io_status_left == HI_GPIO_VALUE0) // 左边识别到黑线，左转
    {
        //printf("========  2  ========\n");
        //g_car_speed_left = 60000;
        g_car_speed_left = 30000;
        g_car_speed_right = car_speed_right;
    }
    // if (io_status_right == HI_GPIO_VALUE0 && io_status_left == HI_GPIO_VALUE0) // 直行
    // {
    //     g_car_speed_left = car_speed_left;
    //     g_car_speed_right = car_speed_right;
    // }
    if (io_status_right == HI_GPIO_VALUE0 && io_status_left == HI_GPIO_VALUE0) // 两边都识别到黑线，则识别到停止线
    {
        printf("============== global_red_on: %d=======\n",global_red_on);
        //printf("========  3  ========\n");
        if (global_red_on == HI_TRUE)
        {
            g_car_speed_left = 60000;
            g_car_speed_right = 60000;
        }
    }
}

hi_void trace_module(hi_void)
{

    extern void DemoMsgRcvCallBack(int qos, const char *topic, const char *payload);
    extern int IoTSetMsgCallback(fnMsgCallBack msgCallback);
    extern void cJsonInit(void);
    //cJsonInit();
    //IoTMain();


    hi_u8 current_car_modular_control_module = g_car_modular_control_module;
    hi_u8 current_car_control_mode = g_car_control_mode;
    hi_gpio_value m_left_value = HI_GPIO_VALUE0;
    hi_gpio_value m_right_value = HI_GPIO_VALUE0;

    printf("###\n");

    trace_module_init();
    hi_u32 timer_id1;
    hi_timer_create(&timer_id1);
    hi_timer_start(timer_id1, HI_TIMER_TYPE_PERIOD, 1, timer1_callback, 0);
    gpio_control(HI_IO_NAME_GPIO_0, HI_GPIO_IDX_0, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_0_GPIO);
    pwm_control(HI_IO_NAME_GPIO_1, HI_IO_FUNC_GPIO_1_PWM4_OUT, HI_PWM_PORT_PWM4, g_car_speed_left);
    gpio_control(HI_IO_NAME_GPIO_9, HI_GPIO_IDX_9, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_9_GPIO);
    pwm_control(HI_IO_NAME_GPIO_10, HI_IO_FUNC_GPIO_10_PWM1_OUT, HI_PWM_PORT_PWM1, g_car_speed_right);

    hi_u16 flag = 0;
    while (1)
    {
        hi_pwm_start(HI_PWM_PORT_PWM4, g_car_speed_left, 60000);
        hi_pwm_start(HI_PWM_PORT_PWM1, g_car_speed_right, 60000);

        hi_udelay(1);

        // 超声波模块
        hi_float m_distance = 0.0;
        m_distance = car_get_distance();
        printf("=============get distance:%lf ============\n", m_distance);
        if (m_distance < DISTANCE_BETWEEN_CAR_AND_OBSTACLE)
        {
            printf("*************there is a car ahead**********\n");
            printf("=============get distance:%lf ============\n", m_distance);
            //g_car_speed_left = 60000;
            //g_car_speed_right = 60000;
            flag = 1;    
            break;
        }

        if ((current_car_modular_control_module != g_car_modular_control_module) || (current_car_control_mode != g_car_control_mode))
        {
            printf("===========trace off=============\n");
            break;
        }
        hi_watchdog_feed();
        hi_sleep(20);
    }
    hi_timer_delete(timer_id1);
    hi_sleep(50);

    if(flag == 1)           // 跳转到超声波
    {
        printf("===========switch to STEER==========\n");
        g_car_control_mode = CAR_MODULE_CONTROL_MODE;
        g_car_modular_control_module = CAR_CONTROL_STEER_ENGINE_TYPE;
        g_car_status = CAR_RUNNING_STATUS;
        hi_watchdog_feed();
        hi_sleep(50);
    }
}

inline hi_void car_before_line_check()
{
    return 1;
}