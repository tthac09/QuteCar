/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: app pwm demo
 * Author: Hisilicon
 * Create: 2019-12-09
 */
#include <app_demo_pwm.h>

hi_void pwm_0_demo(hi_void)
{
    hi_pwm_init(HI_PWM_PORT_PWM0);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_pwm_start(HI_PWM_PORT_PWM0, 750, 1500); /* duty: 750 freq:1500 */
}

hi_void pwm_1_demo(hi_void)
{
    hi_pwm_init(HI_PWM_PORT_PWM1);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_pwm_start(HI_PWM_PORT_PWM1, 750, 1500); /* duty: 750 freq:1500 */
}

hi_void pwm_2_demo(hi_void)
{
    hi_pwm_init(HI_PWM_PORT_PWM2);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_pwm_start(HI_PWM_PORT_PWM2, 750, 1500); /* duty: 750 freq:1500 */
}

hi_void pwm_3_demo(hi_void)
{
    hi_pwm_init(HI_PWM_PORT_PWM3);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_pwm_start(HI_PWM_PORT_PWM3, 750, 1500); /* duty: 750 freq:1500 */
}

hi_void pwm_4_demo(hi_void)
{
    hi_pwm_init(HI_PWM_PORT_PWM4);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_pwm_start(HI_PWM_PORT_PWM4, 750, 1500); /* duty: 750 freq:1500 */
}

hi_void pwm_5_demo(hi_void)
{
    hi_pwm_init(HI_PWM_PORT_PWM5);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_pwm_start(HI_PWM_PORT_PWM5, 750, 1500); /* duty: 750 freq:1500 */
}

hi_void app_demo_pwm(hi_void)
{
    printf("start test pwm");

    pwm_0_demo();
    pwm_1_demo();
    pwm_2_demo();
    pwm_3_demo();
    pwm_4_demo();
    pwm_5_demo();

    printf("please use an oscilloscope to check the output waveform!");
}

