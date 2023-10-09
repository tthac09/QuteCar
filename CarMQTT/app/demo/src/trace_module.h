/*
 * Copyright (c) 2020 HiHope Community. 
 * Description: app trace module demo
 *              红外循迹模块
 * Author: HiSpark Product Team.
 * Create: 2020-05-26
 */

#ifndef __TRACE_MODULE_H__
#define __TRACE_MODULE_H__

#include <hi_adc.h>
#include <app_demo_robot_car.h>

#define     ADC_TEST_LENGTH             (20)

hi_void set_gpio_input_mode(hi_io_name id, hi_u8 val, hi_gpio_idx idx, hi_gpio_dir dir);
hi_void trace_module_init(hi_void);
//hi_gpio_value get_do_value(hi_gpio_idx idx);
hi_gpio_value get_do_value(hi_adc_channel_index idx);
hi_void trace_module(hi_void);

#endif
