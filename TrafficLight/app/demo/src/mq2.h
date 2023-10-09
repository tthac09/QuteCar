#ifndef __MQ2_H__
#define __MQ2_H__

#include <hi_io.h>

hi_void MQ2_Ppm_Calibration(hi_float RS);
hi_float MQ2_Get_Ppm(hi_u32 data_len);
hi_void convert_to_voltage(hi_u32 data_len);
hi_void *app_demo_mq2(hi_void *param);
hi_void app_demo_mq2_task(hi_void);

#endif