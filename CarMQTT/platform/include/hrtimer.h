/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: hrtimer.h header file
 * Author: Hisilicon
 * Create: 2016-07-29
 */

#ifndef __HRTIMER_H__
#define __HRTIMER_H__
#include "hi_types_base.h"


typedef enum {
    HI_HRTIMER_ID_0, /**< time0 */
    HI_HRTIMER_ID_1, /**< time1 */
    HI_HRTIMER_ID_2, /**< time2 */
    HI_HRTIMER_ID_MAX,
} hi_hrtimer_id;

typedef struct {
    hi_u8 create_fail_times;
    hi_bool malloc_fail;
    hi_u8 reserved[2];     /* size 2 */
} hi_hrtimer_diag_info;

HI_EXTERN hi_u32 hi_hrtimer_init(hi_hrtimer_id id, hi_void *timer_array, hi_u8 array_cnt);
HI_EXTERN hi_u32 hi_hrtimer_deinit(hi_void);
HI_EXTERN hi_void hi_hrtimer_refresh_tick(hi_u32 tick);
HI_EXTERN hi_u32 hi_hrtimer_get_current_tick(hi_u32 *tick);

HI_EXTERN hi_hrtimer_diag_info *hi_hrtimer_get_info(hi_void);

#endif



