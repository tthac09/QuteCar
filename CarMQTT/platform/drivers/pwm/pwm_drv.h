/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: pwm driver implementatioin.
 * Author: wangjun
 * Create: 2019-06-03
 */

#ifndef __PWM_DRV_H__
#define __PWM_DRV_H__

#include <hi3861_platform.h>
#include <hi_pwm.h>

#define PWM_BASE_ADDR_STEP 0x100
#define pwm_base_addr(pwm_num) (HI_PWM_REG_BASE + (pwm_num) * PWM_BASE_ADDR_STEP)

#define pwm_en_reg(base_addr)    ((base_addr)+ 0x0)
#define pwm_start_reg(base_addr) ((base_addr) + 0x4)
#define pwm_freq_reg(base_addr)  ((base_addr) + 0x8)
#define pwm_duty_reg(base_addr)  ((base_addr) + 0xC)

typedef struct {
    hi_bool is_init;
    hi_u8 reserved[3]; /* 3bytes reserved */
    hi_u32 pwm_sem;
}pwm_ctl;

typedef struct {
    pwm_ctl pwm[HI_PWM_PORT_MAX];
}pwm_ctx;

hi_u32 pwm_check_port(hi_pwm_port port);
hi_void pwm_set_enable(hi_pwm_port port, hi_bool flag);
hi_void pwm_set_freq(hi_pwm_port port, hi_u16 freq);
hi_void pwm_set_duty(hi_pwm_port port, hi_u16 duty);
hi_void pwm_take_effect(hi_pwm_port port);
hi_bool pwm_is_init(hi_pwm_port port);
hi_u32 pwm_lock(hi_pwm_port port);
hi_u32 pwm_unlock(hi_pwm_port port);
pwm_ctl *pwm_get_ctl(hi_pwm_port port);


#endif

