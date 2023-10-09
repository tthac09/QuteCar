#include <steering_engine_control.h>

/*Control function of steering gear*/
hi_void set_angle(hi_s32 duty)
{
    hi_gpio_set_ouput_val(HI_GPIO_IDX_2, HI_GPIO_VALUE1);
    hi_udelay(duty);
    hi_gpio_set_ouput_val(HI_GPIO_IDX_2, HI_GPIO_VALUE0);
    hi_udelay(20000 - duty);
}

/*Steering gear turn left*/
hi_void engine_turn_left(hi_void)
{
    for (int i = 0; i < 10; i++)
    {
        set_angle(1450 + 100 * i);
    }
}

/*Steering gear turn right*/
hi_void engine_turn_right(hi_void)
{
    for (int i = 0; i < 10; i++)
    {
        set_angle(1450 - 100 * i);
    }
}

/*Steering gear return to middle*/
hi_void regress_middle(hi_void)
{
    set_angle(1450);
}
