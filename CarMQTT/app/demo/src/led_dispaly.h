#ifndef __LED_DISPLAY_H__
#define __LED_DISPLAY_H__

#include <hi_stdlib.h>

#define     LED_DEMO_TASK_STAK_SIZE           (1024*2)
#define     LED_DEMO_TASK_PRIORITY            (25)

#define     NFC_DEMO_TASK_STAK_SIZE           (1024*2)
#define     NFC_DEMO_TASK_PRIORITY            (25)
#define     COLORFUL_FUNC                     (hi_u8(0x01))
#define     TRAFFIC_FUNC                      (hi_u8(0x01))
#define     ENVIRONMENT_FUNC                  (hi_u8(0x01))
#define     KEY_INTERRUPT_PROTECT_TIME        (30)

#define     SECOND_100_TIME                   (100)
#define     SECOND_200_TIME                   (200)
#define     SECOND_300_TIME                   (300)
#define     SECOND_400_TIME                   (400)
#define     SECOND_500_TIME                   (499)
#define     SECOND_600_TIME                   (600)
#define     SECOND_700_TIME                   (700)
#define     SECOND_800_TIME                   (800)
#define     SECOND_900_TIME                   (900)
#define     SECOND_1000_TIME                  (1000)
#define     SECOND_1100_TIME                  (1100)
#define     SECOND_1200_TIME                  (1200)
#define     SECOND_1300_TIME                  (1300)
#define     SECOND_1400_TIME                  (1400)
#define     SECOND_1500_TIME                  (1500)
#define     SECOND_1600_TIME                  (1600)
#define     SECOND_1700_TIME                  (1700)
#define     SECOND_1800_TIME                  (1800)
#define     SECOND_1900_TIME                  (1900)
#define     SECOND_2000_TIME                  (2000)
#define     SECOND_2100_TIME                  (2100)
#define     SECOND_2200_TIME                  (2200)
#define     SECOND_2300_TIME                  (2300)
#define     SECOND_2400_TIME                  (2400)
#define     SECOND_2500_TIME                  (2500)
#define     SECOND_2600_TIME                  (2600)
#define     SECOND_2700_TIME                  (2700)
#define     SECOND_2800_TIME                  (2800)
#define     SECOND_2900_TIME                  (2900)
#define     SECOND_3000_TIME                  (2999)

extern hi_u8  led_display_mode;
extern hi_u8 g_menu_select;
extern hi_u8 g_traffic_human;

typedef enum{
    COLORFUL_FUNC_SELECT =1,
    TRAFFIC_FUNC_SELECT,
    ENVIRONMENT_FUNC_SELECT
}select_key;

typedef enum{
    COLORFUL_MENU_SELECT =1,
    TRAFFIC_MENU_SELECT,
    ENVIRONMENT_MENU_SELECT
}confir_menu;

typedef enum{
    SINGAL_LED_LIGHT_ON =1,
    LED_ON_AUTO_MODE,
    LED_ON_HUMAN_MODE,
    TRAFFIC_MODE_EXTIC
}traffic_light_mode;

typedef enum{
    COLORFUL_MENU=1,
    TRAFFIC_MENU,
    ENVIRONMENT_MENU
}led_menu_type;

hi_u32 traffic_light_flash(hi_u8 model);
hi_u32 traffic_light_yellow_led_flash(hi_u8 model);
hi_u32 traffic_light_green_led_flash(hi_u8 model);
hi_void traffic_light_singal_led(hi_void);
hi_void traffic_light_auto_mode_led(hi_void);
hi_void traffic_light_human_mode_led(hi_void);
hi_void traffic_singal_mode_func(hi_void);
hi_void traffic_human_mode_func(hi_void);
hi_void OLED_ShowStr(hi_u8 x,hi_u8 y,hi_u8 *chr,hi_u8 Char_Size);

#endif