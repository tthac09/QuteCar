#ifndef __APP_DEMO_MULTI_SAMPLE_H__
#define __APP_DEMO_MULTI_SAMPLE_H__

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <hi_task.h>
#include <hi_io.h>
#include <hi_gpio.h>
#include <hi_pwm.h>
#include <hi_sem.h>
#include <hi_time.h>
#include <hi_watchdog.h>
#include <hi_types_base.h>
#include <ssd1306_oled.h>

#define KEY_INTERRUPT_PROTECT_TIME (30)
#define KEY_DOWN_INTERRUPT (1)
#define GPIO_DEMO_TASK_STAK_SIZE (1024 * 2)
#define GPIO_DEMO_TASK_PRIORITY (25)

#define MAX_FUNCTION_SINGLE_DEMO (1)
#define MAX_FUNCTION_DEMO_NUM (1)
#define MAX_TRAFFIC_LIGHT_MODE (4)

#define MAX_CONTROL_MODE_TYPE (2)
#define MAX_PWM_CONTROL_TYPE (4)
#define MAX_BIRGHTNESS_TYPE (2)
#define RETURN_TYPE_MODE (2)

#define MAX_TRAFFIC_CONTROL_TYPE (3)
#define MAX_TRAFFIC_AUTO_TYPE (2)
#define MAX_TRAFFIC_HUMAN_TYPE (2)

#define OLED_FALG_ON ((hi_u8)0x01)
#define OLED_FALG_OFF ((hi_u8)0x00)

#define KEY_GPIO_5 (1)
#define KEY_GPIO_7 (2)

#define LIGHT_ONE_SECOND_INTERVAL (60)
#define BEEP_ONE_SECOND_INTERVAL (15)
#define BEEP_HALF_SECOND_INTERVAL (8)
#define BEEP_QUARTER_SECOND_INTERVAL (4)

#define RED_LIGHT_FLASH_TIME (3)    //红灯闪3秒
#define YELLOW_LIGHT_FLASH_TIME (3) //黄灯闪3秒
#define GREEN_LIGHT_FLASH_TIME (3)  //绿灯闪3秒

#define TRAFFIC_AUTO_MODE_TIME_COUNT (3)
#define TRAFFIC_HUMAN_MODE_NORMAL_TIME_COUNT (3)
#define TRAFFIC_HUMAN_MODE_TIME_CONUT (3)

#define FACTORY_HISPARK_BOARD_TEST(fmt, ...)    \
    do                                          \
    {                                           \
        printf(fmt, ##__VA_ARGS__);             \
        for (hi_s32 i = 0; i < 3; i++)          \
        {                                       \
            hispark_board_test(HI_GPIO_VALUE0); \
            hi_udelay(DELAY_250_MS);            \
            hispark_board_test(HI_GPIO_VALUE1); \
            hi_udelay(DELAY_250_MS);            \
        }                                       \
    } while (0)

#define FACTORY_HISPARK_BOARD_CL_TL_TEST(fmt, ...) \
    do                                             \
    {                                              \
        printf(fmt, ##__VA_ARGS__);                \
        for (hi_s32 i = 0; i < 3; i++)             \
        {                                          \
            hispark_board_test(HI_GPIO_VALUE0);    \
            hi_udelay(DELAY_500_MS);               \
            hispark_board_test(HI_GPIO_VALUE1);    \
            hi_udelay(DELAY_500_MS);               \
        }                                          \
    } while (0)

typedef enum
{
    BEEP_BY_ONE_SECOND,
    BEEP_BY_HALF_SECOND,
    BEEP_BY_QUARTER_SECOND
} hi_beep_status;

typedef enum
{
    RED_COUNT = 0,
    YELLOW_COUNT,
    GREEN_COUNT,
    DEFAULT
} hi_led_time_count;

typedef enum
{
    MAIN_FUNCTION_SELECT_MODE = 0,
    SUB_MODE_SELECT_MODE,
} hi_menu_mode;

typedef enum
{
    TRAFFIC_LIGHT_FUNC,
} hi_select_func;

typedef enum
{
    TRAFFIC_LIGHT_MENU,
} hi_select_menu_mode;

typedef enum
{
    CONTROL_MODE = 0,
    RETURN_MODE,
} hi_colorful_light_mode;

typedef enum
{
    TRAFFIC_CONTROL_MODE = 0,
    TRAFFIC_AUTO_MODE,
    TRAFFIC_HUMAN_MODE,
    TRAFFIC_RETURN_MODE,
} hi_traffic_light_mode;

typedef enum
{
    TRAFFIC_NORMAL_TYPE = 0,
    TRAFFIC_HUMAN_TYPE
} hi_traffic_light_type;

typedef enum
{
    RED_ON = 0,
    YELLOW_ON,
    GREEN_ON,
} hi_control_mode_type;

typedef enum
{
    BEEP_ON = 0,
    BEEP_OFF,
} hi_beep_status_type;

typedef enum
{
    KEY_UP = 0,
    KEY_DOWN,
} hi_return_select_type;

typedef enum
{
    ONE = 1,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
} time_count;

hi_void all_light_out(hi_void);
hi_void control_mode_sample(hi_void);
hi_void return_enum_sample(hi_void);
hi_void gpio5_isr_func_mode(hi_void);
hi_void gpio7_isr_func_type(hi_void);
hi_void app_multi_sample_demo(hi_void);
hi_void return_main_enum_sample(hi_void);
hi_void test_gpio_init(hi_void);

#endif