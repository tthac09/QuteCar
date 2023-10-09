/*
 * Copyright (c) 2020 HiHope Community.
 * Description:  gl5537_1 serson get data from adc change
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */
#include <hi_watchdog.h>
#include <hi_early_debug.h>
#include <hi_time.h>
#include <hi_io.h>
#include <hi_gpio.h>
#include <hi_adc.h>
#include <hi_stdlib.h>
#include <hi_task.h>
#include <hi_pwm.h>

#define     ADC_TEST_LENGTH             (20)
#define     VLT_MIN                     (100)
#define     OLED_FALG_ON                ((hi_u8)0x01)
#define     OLED_FALG_OFF               ((hi_u8)0x00)

hi_u16  g_adc_buf[ADC_TEST_LENGTH] = { 0 };
hi_u16  g_gpio5_adc_buf[ADC_TEST_LENGTH] = { 0 };
extern hi_void gpio5_isr_func_mode(hi_void);
extern hi_void gpio7_isr_func_type(hi_void);
extern hi_void gpio9_led_light_func(hi_viod) ;
/*get data from adc test*/
hi_u8 get_light_status(hi_void)
{
    int i;
    hi_u16 data;
    hi_u32 ret;
    hi_u16 vlt;
    float voltage;
    float vlt_max = 0;
    float vlt_min = VLT_MIN;
    memset_s(g_adc_buf, sizeof(g_adc_buf), 0x0, sizeof(g_adc_buf));
    for (i = 0; i < ADC_TEST_LENGTH; i++) {
        ret = hi_adc_read(HI_ADC_CHANNEL_4, &data, HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, 0xF0); //ADC_Channal_6  自动识别模式  CNcomment:4次平均算法模式 CNend */
        if (ret != HI_ERR_SUCCESS) {
            printf("ADC Read Fail\n");
            return  HI_NULL;
        }    
        g_adc_buf[i] = data;
    }

    for (i = 0; i < ADC_TEST_LENGTH; i++) {  
        vlt = g_adc_buf[i]; 
        voltage = (float)vlt * 1.8 * 4 / 4096.0;  /* vlt * 1.8 * 4 / 4096.0为将码字转换为电压 */
        vlt_max = (voltage > vlt_max) ? voltage : vlt_max;
        vlt_min = (voltage < vlt_min) ? voltage : vlt_min;
    }

    if (vlt_max >3.0) {
        return OLED_FALG_ON;
    } else {
        return OLED_FALG_OFF;
    }
}
/*get gpio5 Voltage*/
hi_u8 get_gpio5_voltage(hi_void *param)
{
    int i;
    hi_u16 data;
    hi_u32 ret;
    hi_u16 vlt;
    float voltage;
    float vlt_max = 0;
    float vlt_min = VLT_MIN;

    hi_unref_param(param);
    memset_s(g_gpio5_adc_buf, sizeof(g_gpio5_adc_buf), 0x0, sizeof(g_gpio5_adc_buf));
    for (i = 0; i < ADC_TEST_LENGTH; i++) {
        ret = hi_adc_read(HI_ADC_CHANNEL_2, &data, HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, 0xF0); //ADC_Channal_2  自动识别模式  CNcomment:4次平均算法模式 CNend */
        if (ret != HI_ERR_SUCCESS) {
            printf("ADC Read Fail\n");
            return  HI_NULL;
        }    
        g_gpio5_adc_buf[i] = data;
    }

    for (i = 0; i < ADC_TEST_LENGTH; i++) {  
        vlt = g_gpio5_adc_buf[i]; 
        voltage = (float)vlt * 1.8 * 4 / 4096.0;  /* vlt * 1.8* 4 / 4096.0为将码字转换为电压 */
        vlt_max = (voltage > vlt_max) ? voltage : vlt_max;
        vlt_min = (voltage < vlt_min) ? voltage : vlt_min;
    }
    if (vlt_max > 0.6 && vlt_max < 1.0) {
        gpio5_isr_func_mode();
    } else if (vlt_max > 1.0 && vlt_max < 1.5) {
        gpio7_isr_func_type();
    } else if (vlt_max <0.6) {
        printf("gpio9_led_light:vlt_max=%0.2f, vlt_min=%0.2f\r\n", vlt_max, vlt_min);
        gpio9_led_light_func();
    }
    printf("key_5:vlt_max=%0.2f, vlt_min=%0.2f\r\n", vlt_max, vlt_min);
}