#ifndef __APP_DEMO_TRAFFIC_SAMPLE_H__
#define __APP_DEMO_TRAFFIC_SAMPLE_H__

#include <hi_types_base.h>
#include <hi_gpio.h>
#include <hi_io.h>
#include <app_demo_multi_sample.h>

extern hi_u8 g_current_type;
extern hi_u8 g_current_mode;
extern hi_u8 g_menu_mode;
extern hi_u8 g_menu_select;

extern hi_void all_led_off(hi_void);
extern hi_u8 delay_check_key(hi_u32 delay_time);
extern hi_void gpio_control(hi_io_name gpio, hi_gpio_idx id, hi_gpio_dir  dir,  hi_gpio_value gpio_val, hi_u8 val);
extern hi_void oled_show_str(hi_u8 x, hi_u8 y, hi_u8 *chr, hi_u8 char_size);
extern hi_void oled_position_clean_screen(hi_u8 fill_data, hi_u8 line, hi_u8 pos, hi_u8 len);
extern hi_u32 oled_init(hi_void);

hi_void traffic_control_mode_sample(hi_void);
hi_u8 delay_and_fresh_screen(hi_u32 delay_time, hi_u8 beep_status);
hi_void traffic_auto_mode_sample(hi_void);
hi_void traffic_normal_type(hi_void);
hi_void traffic_human_type(hi_void);
hi_void traffic_human_mode_sample(hi_void);
hi_void traffic_light_func(hi_void);
#endif