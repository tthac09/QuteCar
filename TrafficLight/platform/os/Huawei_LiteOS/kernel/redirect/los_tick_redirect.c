/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2018. All rights reserved.
 * Description: Tick redirect
 * Author: PanBin
 * Create: 2013-01-01
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 * --------------------------------------------------------------------------- */

#include "los_hal.h"
#include "los_exc.h"
#include "los_task_pri.h"
#include "los_tick_pri.h"
#if (LOSCFG_BASE_CORE_TICK_HW_TIME == YES)
#include "hi_hwtimer.h"
#include "hi3861_platform_base.h"
#include "los_bsp_adapter.h"
#include <hi_clock.h>
#include "los_pmp.h"
#include "los_hal.h"

hi_void_callback g_tick_callback = HI_NULL;
UINT32 g_firstTick = 0; /* first tick cycle after wakeup in sys cycle */
UINT32 g_reloadValue = 0;
VOID OsPlatformTickHandler(VOID)
{
    if (g_firstTick != 0) {
        Timer_Type *timerx = (Timer_Type *)TIMER_3_BASE_ADDR;
        timerx->LOADCOUNT  = g_reloadValue;
        g_firstTick = 0;
    }
    hi_reg_read_val32(TIMER_3_BASE_ADDR + TIMER_EOI);
    Mb();
    /* call registerd tick callbak. */
    if (g_tick_callback != NULL) {
        g_tick_callback();
    }
}

UINT32* OsTimerGetFirstTick(VOID)
{
    return &g_firstTick;
}

UINT32 OsTimerGetReloadValue(VOID)
{
    return g_reloadValue;
}

LITE_OS_SEC_TEXT_INIT_REDIRECT VOID OsTimerDisable(VOID)
{
    Timer_Type *timerx = (Timer_Type *)TIMER_3_BASE_ADDR;
    timerx->CONTROLREG = 0x00000000;
    return;
}

LITE_OS_SEC_TEXT_INIT_REDIRECT VOID OsTimerStop(VOID)
{
    Timer_Type *timerx = (Timer_Type *)TIMER_3_BASE_ADDR;
    timerx->CONTROLREG &= ~0x00000001;
    return;
}
LITE_OS_SEC_TEXT_INIT_REDIRECT VOID OsTimerStart(VOID)
{
    Timer_Type *timerx = (Timer_Type *)TIMER_3_BASE_ADDR;
    if (g_firstTick != 0) {
        timerx->LOADCOUNT = g_firstTick;
    }
    timerx->CONTROLREG |= 0x00000001;
    return;
}

LITE_OS_SEC_TEXT_INIT_REDIRECT UINT32 OsTimerInit(VOID)
{
    UINT32 reloadValue;
    UINT32 systemClock = SYSTEM_CLOCK_FREQ_24M; /* SYSTEM CLOCK FREQ 24M */
    Timer_Type *timerx = (Timer_Type *)TIMER_3_BASE_ADDR;
    if (LOSCFG_BASE_CORE_TICK_PER_SECOND == 0) {
        return LOS_ERRNO_TICK_CFG_INVALID;
    }

    LOS_AdapIrqDisable((UINT32)MACHINE_TIMER_IRQ);
    hi_reg_write32((TIMER_3_BASE_ADDR + TIMER_CONTROL_REG), TIMER_CTL_CFG_DISABLE);

    hi_xtal_clock xtalClock = hi_get_xtal_clock();
    if (xtalClock == HI_XTAL_CLOCK_40M) {
        systemClock = SYSTEM_CLOCK_FREQ_40M; /* SYSTEM CLOCK FREQ 40M */
    }
    g_cyclesPerTick = OS_SYS_CLOCK / LOSCFG_BASE_CORE_TICK_PER_SECOND;
    reloadValue = systemClock * 1000000 / LOSCFG_BASE_CORE_TICK_PER_SECOND; /* 1000000: Mhz convert to hz. */
    g_reloadValue = reloadValue;
    timerx->CONTROLREG = 0x00000000;
    timerx->LOADCOUNT  = reloadValue;

    timerx->CONTROLREG |= (UINT32)TIMER_MODE_CYCLE << 1; /* 1: timer for cycle */
    timerx->CONTROLREG |= (UINT32)TIMER_INT_UNMASK << 2; /* 2: timer for unmask */

    OS_SET_VECTOR(MACHINE_TIMER_IRQ, (HWI_PROC_FUNC)OsTickHandler, 0);
    OsIrqEnable((HWI_HANDLE_T)MACHINE_TIMER_IRQ);

    timerx->CONTROLREG |= 0x00000001;
    return LOS_OK;
}
#endif
