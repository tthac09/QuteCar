/*
 * Copyright (c) 2020 HiHope Community.
 * Description: app demo robot car
 *              robot 小车的控制
 * Author: HiSpark Product Team.
 * Create: 2020-05-25
 */

#ifndef __APP_DEMO_ROBOT_CAR_H__
#define __APP_DEMO_ROBOT_CAR_H__

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

#define     CAR_CONTROL_DEMO_TASK_STAK_SIZE   (1024*10)
#define     CAR_CONTROL_DEMO_TASK_PRIORITY    (25)
#define     DISTANCE_BETWEEN_CAR_AND_OBSTACLE (20)
#define     KEY_INTERRUPT_PROTECT_TIME        (30)
#define     CAR_TURN_LEFT                     (0)
#define     CAR_TURN_RIGHT                    (1)
#define     MAX_SPEED                         (100)
#define     MIN_SPEED                         (30000)
#define     PWM_DUTY_LEFT_RIGHT               (5000)
#define     PWM_DUTY_FORWARD_BACK             (10000)

typedef enum {
    CAR_STOP_STATUS = 0,
    CAR_RUNNING_STATUS, 
    CAR_TRACE_STATUS
} hi_car_status;

typedef enum {
    CAR_DIRECTION_CONTROL_MODE = 1, //控制小车方向
    CAR_MODULE_CONTROL_MODE,   //控制小车的扩展模块
    CAR_SPEED_CONTROL_MODE     //速度控制
} hi_car_control_mode;

typedef enum {
    CAR_SPEED_UP = 1,
    CAR_SPEED_REDUCTION,
} hi_car_speed_type;

typedef enum {
    CAR_KEEP_GOING_TYPE = 1,
    CAR_KEEP_GOING_BACK_TYPE,
    CAR_KEEP_TURN_LEFT_TYPE,
    CAR_KEEP_TURN_RIGHT_TYPE,
    CAR_GO_FORWARD_TYPE,
    CAR_TURN_LEFT_TYPE,
    CAR_STOP_TYPE,
    CAR_TURN_RIGHT_TYPE,
    CAT_TURN_BACK_TYPE,
} hi_car_direction_control_type;

/*小车扩展模块的几种类型*/
typedef enum {
    CAR_CONTROL_ENGINE_LEFT_TYPE = 1,
    CAR_CONTROL_ENGINE_RIGHT_TYPE,
    CAR_CONTROL_ENGINE_MIDDLE_TYPE,
    CAR_CONTROL_TRACE_TYPE,
    CAR_CONTROL_ULTRASONIC_TYPE,
    CAR_CONTROL_STEER_ENGINE_TYPE,
} hi_car_module_control_type;

hi_void gpio5_switch_init(hi_void);
hi_void app_demo_robot_car_task(hi_void);
hi_void gpio5_interrupt_monitor(hi_void);
hi_void timer_task(hi_void);
#endif
