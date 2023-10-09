#include <ultrasonic_module.h>

/*
注意：要在断电的情况下连接好VCC，GND
    trig连接在gpio7，输出模式，低电平，
    echo连接在gpio8，输入模式，低电平
测量前方障碍物的距离
GPIO7/GPIO8都是只能做输出使用的，上电时不允许有高电平输入
上电起来后可以作为输入IO检测
*/

/*Measure distance from obstacles ahead*/
hi_float car_get_distance(hi_void)
{
    static hi_u64 start_time = 0;
    hi_u64 end_time = 0;
    hi_float distance = 0.0;
    hi_gpio_value val = HI_GPIO_VALUE0;
    int gpio_8_level = GPIO_8_IS_LOW_LEVEL;

    /*超声波echo口，设置为输入模式*/
    hi_io_set_func(HI_IO_NAME_GPIO_8, HI_IO_FUNC_GPIO_8_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_8, HI_GPIO_DIR_IN);

    /* 给trig发送至少10us的高电平脉冲，以触发传感器测距 */
    hi_gpio_set_ouput_val(HI_GPIO_IDX_7,HI_GPIO_VALUE1);
    hi_udelay(20);
    hi_gpio_set_ouput_val(HI_GPIO_IDX_7,HI_GPIO_VALUE0);

    /*计算与障碍物之间的距离*/
    while (1) {
        hi_gpio_get_input_val(HI_GPIO_IDX_8,&val);

        /*echo为高电平时开始计时*/
        if ((val == HI_GPIO_VALUE1) && (gpio_8_level == GPIO_8_IS_LOW_LEVEL)) {
            start_time = hi_get_us();
            gpio_8_level = GPIO_8_IS_HIGH_LEVEL;
        }

        /*echo为低电平时纪录结束时间*/
        if ((val == HI_GPIO_VALUE0) && (gpio_8_level == GPIO_8_IS_HIGH_LEVEL)) {
            end_time = hi_get_us() - start_time;
            start_time = 0;
            break;
        }
    }
    distance = end_time/DISTANCE_FORMULA;
    
    return distance;
}