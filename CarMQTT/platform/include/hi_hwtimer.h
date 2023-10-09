/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: hi_hwtimer.h.
 * Author: Hisilicon
 * Create: 2012-12-22
 */

/**
* @file hi_hwtimer.h
*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.  \n
*
* Description: hwtimer interfaces. \n
*/

#ifndef __HI_HWTIMER_H__
#define __HI_HWTIMER_H__

#include <hi_types_base.h>


typedef enum {
    HI_RTC_CLK_32K = 32,
    HI_RTC_CLK_24M = 24,
    HI_RTC_CLK_40M = 40,
} hi_rtc_clk; /**< 低功耗深睡场景下，24M/40M时钟不可用 */

typedef void (*hi_hwtimer_callback)(hi_u32 data);

typedef void (*hi_hwrtc_callback)(hi_u32 data);

typedef void (*hwtimer_clken_callback)(hi_void);

/**
 * @ingroup hw_timer
 *
 * Timer mode control. CNcomment:定时器模式控制。CNend
 */
typedef enum {
    TIMER_MODE_FREE = 0,   /**< 自由模式 */
    TIMER_MODE_CYCLE = 1,  /**< 周期模式 */
} timer_mode;

/**
 * @ingroup hw_timer
 *
 * Timer interrupt mask control. CNcomment:定时器中断模式控制。CNend
 */
typedef enum {
    TIMER_INT_UNMASK = 0,  /**< 不屏蔽 */
    TIMER_INT_MASK = 1,    /**< 屏蔽 */
} timer_int_mask;

/**
 * @ingroup hw_timer
 *
 * hwtimer ID. CNcomment:硬件定时器ID。CNend
 */
typedef enum {
    HI_TIMER_ID_0,
    HI_TIMER_ID_1,
    HI_TIMER_ID_2,
    HI_TIMER_ID_MAX, /* 无效值 */
} hi_timer_id;

/**
 * @ingroup hw_timer
 *
 * hwrtc ID. CNcomment:硬件RTC ID。CNend
 */
typedef enum {
    HI_RTC_ID_0 = HI_TIMER_ID_MAX,
    HI_RTC_ID_1,
    HI_RTC_ID_2,
    HI_RTC_ID_3,
    HI_RTC_ID_MAX, /* 无效值 */
} hi_rtc_id;

/**
 * @ingroup hw_timer
 *
 * hwtimer working mode. CNcomment:硬件定时器工作模式。CNend
 */
typedef enum {
    HI_HWTIMER_MODE_ONCE,    /**< 单次模式 */
    HI_HWTIMER_MODE_PERIOD,  /**< 周期模式 */
    HI_HWTIMER_MODE_INVALID,
} hi_hwtimer_mode;

/**
 * @ingroup hw_timer
 *
 * hwrtc working mode. CNcomment:硬件RTC工作模式。CNend
 */
typedef enum {
    HI_HWRTC_MODE_ONCE,    /**< 单次模式 */
    HI_HWRTC_MODE_PERIOD,  /**< 周期模式 */
    HI_HWRTC_MODE_INVALID,
} hi_hwrtc_mode;

/**
 * @ingroup hw_timer
 *
 * hwtimer handle structure. CNcomment:硬件定时器句柄结构。CNend
 */
typedef struct {
    hi_hwtimer_mode mode;       /**< 工作模式 */
    hi_u32 expire;              /**< 超时时间（单位：微秒）。最大超时时间为hi_u32(-1)/CLK，其中CLK为工作时钟频率，
                                     即若工作时钟为24Mhz，则CLK=24。 */
    hi_u32 data;                /**< 回调函数参数 */
    hi_hwtimer_callback func;   /**< 回调函数 */
    hi_timer_id timer_id;       /**< 定时器ID */
} hi_hwtimer_ctl;

/**
 * @ingroup hw_timer
 *
 * hwrtc handle structure. CNcomment:硬件RTC句柄结构。CNend
 */
typedef struct {
    hi_hwrtc_mode mode;         /**< 工作模式 */
    hi_u32 expire;              /**< 超时时间，时钟源若为32K，单位为ms；若为24M或40M，单位为us。 */
    hi_u32 data;                /**< 回调函数参数 */
    hi_hwrtc_callback func;     /**< 回调函数 */
    hi_rtc_id rtc_id;           /**< RTC ID */
} hi_hwrtc_ctl;


 /* 在真正配置时钟源选择、分频的接口里调用该接口，即保持寄存器值和软件的变量值一致 */
hi_void hi_hwrtc_set_clk(hi_rtc_clk clk);

hi_u32 hi_hwtimer_init_new(hi_timer_id timer_id);
hi_u32 hi_hwtimer_start(const hi_hwtimer_ctl *timer);
hi_u32 hi_hwtimer_stop(hi_timer_id timer_id);
hi_u32 hi_hwtimer_destroy_new(hi_timer_id timer_id);
hi_u32 hi_hwtimer_get_cur_val(hi_timer_id timer_id, hi_u32 *val); /* 获取timer当前值 */
hi_u32 hi_hwtimer_get_load(hi_timer_id timer_id, hi_u32 *load);   /* 获取timer初值 */

hi_u32 hi_hwrtc_start(const hi_hwrtc_ctl *rtc);
hi_u32 hi_hwrtc_init(hi_rtc_id timer_id);
hi_u32 hi_hwrtc_stop(hi_rtc_id rtc_id);
hi_u32 hi_hwrtc_destroy(hi_rtc_id rtc_id);
hi_u32 hi_hwrtc_get_cur_val(hi_rtc_id rtc_id, hi_u32 *val); /* 获取rtc当前值 */
hi_u32 hi_hwrtc_get_load(hi_rtc_id rtc_id, hi_u32 *load);   /* 获取rtc初值 */
hi_u32 hwtimer_start(hi_u32 tick_cnt, const hi_hwtimer_ctl *timer);

#endif
