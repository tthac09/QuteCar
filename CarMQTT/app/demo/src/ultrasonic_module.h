/*
 * Copyright (c) 2020 HiHope Community. All rights reserved.
 * Description: Ultrasonic module demo
 *              超声波模块数据采集
 * Author: HiSpark Product Team.
 * Create: 2020-06-16
 */
#ifndef __ULTRASONIC_MODULE_H__
#define __ULTRASONIC_MODULE_H__

#include <stdio.h>
#include <hi_io.h>
#include <stdlib.h>
#include <hi_gpio.h>
#include <hi_task.h>
#include <hi_time.h>
#include <hi_watchdog.h>
#include <hi_types_base.h>
#include <hi_early_debug.h>


#define     GPIO_8_IS_LOW_LEVEL               (0)
#define     GPIO_8_IS_HIGH_LEVEL              (1)
#define     DISTANCE_FORMULA                  (58.0) //距离计算公式

hi_float car_get_distance(hi_void);

#endif
