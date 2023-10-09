/*
 * Copyright (c) 2020 HiHope Community.
 * Description: app steering engine control demo
 *              舵机控制模块
 * Author: HiSpark Product Team. 
 * Create: 2020-05-26
 */

#ifndef __STEERING_ENGINE_CONTROL_H__
#define __STEERING_ENGINE_CONTROL_H__

#include <stdio.h>
#include <stdlib.h>
#include <hi_gpio.h>

hi_void set_angle(hi_s32 duty);
hi_void engine_turn_left(hi_void);
hi_void engine_turn_right(hi_void);
hi_void regress_middle(hi_void);

#endif
