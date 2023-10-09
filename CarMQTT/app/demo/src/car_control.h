/*
 * Copyright (c) 2020 HiHope Community.
 * Description: app car control demo
 *              小车电机控制 方向控制
 * Author: HiSpark Product Team.
 * Create: 2020-05-25
 */
#ifndef __CAR_CONTROL_H__
#define __CAR_CONTROL_H__

#include <stdio.h>
#include <hi_io.h>
#include <hi_pwm.h>
#include <stdlib.h>
#include <memory.h>
#include <hi_gpio.h>
#include <hi_task.h>
#include <hi_time.h>
#include <hi_watchdog.h>
#include <hi_types_base.h>
#include <hi_early_debug.h>

#define     PWM_FREQ_FREQUENCY                (60000)
#define     CAR_SPEED_CHANGE_VALUE            (500)
#define     PWM_DUTY_STOP                     (100)

hi_void  gpio_control(hi_io_name gpio, hi_gpio_idx id, hi_gpio_dir  dir,  hi_gpio_value gpio_val, hi_u8 val);
hi_void  pwm_control(hi_io_name id, hi_u8 val, hi_pwm_port port, hi_u16 duty);
hi_void car_speed_up(hi_void);
hi_void car_speed_reduction(hi_void);
hi_void correct_car_speed(hi_void);
hi_void  car_go_forward(hi_void);
hi_void  car_go_back(hi_void);
hi_void  car_stop(hi_void);
hi_void  car_turn_left(hi_void);
hi_void  car_turn_right(hi_void);
hi_void  car_trace_back(hi_void);
hi_void  car_trace_left(hi_void);
hi_void  car_trace_right(hi_void);

#endif
