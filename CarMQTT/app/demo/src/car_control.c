#include <car_control.h>
#include <app_demo_robot_car.h>

extern hi_u32  g_car_speed;

/*gpio control func*/
hi_void gpio_control(hi_io_name gpio, hi_gpio_idx id, hi_gpio_dir  dir,  hi_gpio_value gpio_val, hi_u8 val)
{
    hi_io_set_func(gpio, val);
    hi_gpio_set_dir(id, dir);
    hi_gpio_set_ouput_val(id, gpio_val);
}

/*pwm control func*/
hi_void pwm_control(hi_io_name id, hi_u8 val, hi_pwm_port port, hi_u16 duty)
{
    hi_io_set_func(id, val);
    hi_pwm_init(port);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_pwm_start(port, duty, PWM_FREQ_FREQUENCY);
}

/*car speed up */
hi_void car_speed_up(hi_void)
{
    g_car_speed = g_car_speed - CAR_SPEED_CHANGE_VALUE;
    correct_car_speed();
    printf("g_car_speed is %d\n",(int)g_car_speed);
}

/*car speed reduction*/
hi_void car_speed_reduction(hi_void)
{
    g_car_speed = g_car_speed + CAR_SPEED_CHANGE_VALUE;
    correct_car_speed();
    printf("g_car_speed is %d\n",(int)g_car_speed);
}

/*correct car speed*/
hi_void correct_car_speed(hi_void)
{
    if (g_car_speed < MAX_SPEED) {
        g_car_speed = MAX_SPEED;
    }

    if (g_car_speed > MIN_SPEED) {
        g_car_speed = MIN_SPEED;
    }
}

/*car go forward */
hi_void car_go_forward(hi_void)
{
    correct_car_speed();
    gpio_control(HI_IO_NAME_GPIO_0, HI_GPIO_IDX_0, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_0_GPIO);
    pwm_control(HI_IO_NAME_GPIO_1,HI_IO_FUNC_GPIO_1_PWM4_OUT,HI_PWM_PORT_PWM4, g_car_speed);
    gpio_control(HI_IO_NAME_GPIO_9, HI_GPIO_IDX_9, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_9_GPIO);
    pwm_control(HI_IO_NAME_GPIO_10,HI_IO_FUNC_GPIO_10_PWM1_OUT,HI_PWM_PORT_PWM1, g_car_speed);
}

/*car go back */
hi_void car_trace_back(hi_void)
{
    correct_car_speed();
    pwm_control(HI_IO_NAME_GPIO_0,HI_IO_FUNC_GPIO_0_PWM3_OUT,HI_PWM_PORT_PWM3, 12000); 
    gpio_control(HI_IO_NAME_GPIO_1, HI_GPIO_IDX_1, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_1_GPIO);
    pwm_control(HI_IO_NAME_GPIO_9,HI_IO_FUNC_GPIO_9_PWM0_OUT,HI_PWM_PORT_PWM0, 12000); 
    gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_10_GPIO);
}
/*car stop */
hi_void car_stop(hi_void)
{
    correct_car_speed();
    pwm_control(HI_IO_NAME_GPIO_0,HI_IO_FUNC_GPIO_0_PWM3_OUT,HI_PWM_PORT_PWM3, PWM_DUTY_STOP); 
    gpio_control(HI_IO_NAME_GPIO_1, HI_GPIO_IDX_1, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_1_GPIO);
    pwm_control(HI_IO_NAME_GPIO_9,HI_IO_FUNC_GPIO_9_PWM0_OUT,HI_PWM_PORT_PWM0, PWM_DUTY_STOP);
    gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
}

/*car turn left */
hi_void car_trace_left(hi_void)
{
    correct_car_speed();
    gpio_control(HI_IO_NAME_GPIO_0, HI_GPIO_IDX_0, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_0_GPIO);
    pwm_control(HI_IO_NAME_GPIO_1,HI_IO_FUNC_GPIO_1_PWM4_OUT,HI_PWM_PORT_PWM4, 5000);
    gpio_control(HI_IO_NAME_GPIO_9, HI_GPIO_IDX_9, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_9_GPIO);
    pwm_control(HI_IO_NAME_GPIO_10,HI_IO_FUNC_GPIO_10_PWM1_OUT,HI_PWM_PORT_PWM1, 13000);
    /*pwm_control(HI_IO_NAME_GPIO_0,HI_IO_FUNC_GPIO_0_PWM3_OUT,HI_PWM_PORT_PWM3, PWM_DUTY_LEFT_RIGHT); 
    gpio_control(HI_IO_NAME_GPIO_1, HI_GPIO_IDX_1, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_1_GPIO);
    gpio_control(HI_IO_NAME_GPIO_9, HI_GPIO_IDX_9, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_9_GPIO);
    pwm_control(HI_IO_NAME_GPIO_10,HI_IO_FUNC_GPIO_10_PWM1_OUT,HI_PWM_PORT_PWM1, 7000);*/
}

/*car turn right */
hi_void car_trace_right(hi_void)
{
    correct_car_speed();
    gpio_control(HI_IO_NAME_GPIO_0, HI_GPIO_IDX_0, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_0_GPIO);
    pwm_control(HI_IO_NAME_GPIO_1,HI_IO_FUNC_GPIO_1_PWM4_OUT,HI_PWM_PORT_PWM4, 13000);
    gpio_control(HI_IO_NAME_GPIO_9, HI_GPIO_IDX_9, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_9_GPIO);
    pwm_control(HI_IO_NAME_GPIO_10,HI_IO_FUNC_GPIO_10_PWM1_OUT,HI_PWM_PORT_PWM1, 5000);
    /*gpio_control(HI_IO_NAME_GPIO_0, HI_GPIO_IDX_0, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_0_GPIO);
    pwm_control(HI_IO_NAME_GPIO_1,HI_IO_FUNC_GPIO_1_PWM4_OUT,HI_PWM_PORT_PWM4, PWM_DUTY_LEFT_RIGHT);
    pwm_control(HI_IO_NAME_GPIO_9,HI_IO_FUNC_GPIO_9_PWM0_OUT,HI_PWM_PORT_PWM0,7000);
    gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_10_GPIO);*/
}

/*car turn right */
hi_void car_turn_right(hi_void)
{
    correct_car_speed();
    gpio_control(HI_IO_NAME_GPIO_0, HI_GPIO_IDX_0, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_0_GPIO);
    pwm_control(HI_IO_NAME_GPIO_1,HI_IO_FUNC_GPIO_1_PWM4_OUT,HI_PWM_PORT_PWM4, PWM_DUTY_LEFT_RIGHT);
    pwm_control(HI_IO_NAME_GPIO_9,HI_IO_FUNC_GPIO_9_PWM0_OUT,HI_PWM_PORT_PWM0,PWM_DUTY_LEFT_RIGHT);
    gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_10_GPIO);
}

/*car go back */
hi_void car_go_back(hi_void)
{
    correct_car_speed();
    pwm_control(HI_IO_NAME_GPIO_0,HI_IO_FUNC_GPIO_0_PWM3_OUT,HI_PWM_PORT_PWM3, g_car_speed); 
    gpio_control(HI_IO_NAME_GPIO_1, HI_GPIO_IDX_1, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_1_GPIO);
    pwm_control(HI_IO_NAME_GPIO_9,HI_IO_FUNC_GPIO_9_PWM0_OUT,HI_PWM_PORT_PWM0, g_car_speed); 
    gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_10_GPIO);
}

/*car turn left */
hi_void car_turn_left(hi_void)
{
    correct_car_speed();
    pwm_control(HI_IO_NAME_GPIO_0,HI_IO_FUNC_GPIO_0_PWM3_OUT,HI_PWM_PORT_PWM3, PWM_DUTY_LEFT_RIGHT); 
    gpio_control(HI_IO_NAME_GPIO_1, HI_GPIO_IDX_1, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0, HI_IO_FUNC_GPIO_1_GPIO);
    gpio_control(HI_IO_NAME_GPIO_9, HI_GPIO_IDX_9, HI_GPIO_DIR_OUT, HI_GPIO_VALUE1, HI_IO_FUNC_GPIO_9_GPIO);
    pwm_control(HI_IO_NAME_GPIO_10,HI_IO_FUNC_GPIO_10_PWM1_OUT,HI_PWM_PORT_PWM1, PWM_DUTY_LEFT_RIGHT);
}

