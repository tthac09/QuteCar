/*
 * Copyright (c) 2020 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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



