/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2018. All rights reserved.
 * Description: Init redirect
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

#include "securec.h"
#include "los_config.h"
#include "los_base.h"
#include "los_sem_pri.h"
#include "los_mux_pri.h"
#include "los_queue_pri.h"
#include "los_memory_pri.h"
#include "los_memstat_pri.h"
#include "los_swtmr_pri.h"
#include "los_tick_pri.h"
#include "los_cpup_pri.h"
#include "los_timeslice_pri.h"
#include "los_exc.h"
#include "los_hal.h"
#include "hi_clock.h"
#include "hi3861_platform_base.h"

#define TIMER_LOCK_BIT 0x08
#define OS_SYS_CLOCK_IN_M        (OS_SYS_CLOCK / 1000000)

#if (LOSCFG_LIB_CONFIGURABLE == YES)
#include "los_build.h"
#endif

#if (LOSCFG_BASE_MISC_TRACK == YES)
#include "los_track.h"
#endif

#if (LOSCFG_LIB_CONFIGURABLE == YES)
#if (LOSCFG_LIB_LIBC == YES)
extern int *errno_array;
extern CHAR LOS_BLOCK_START(errno);
extern CHAR LOS_BLOCK_END(errno);
#endif

extern CHAR LOS_BLOCK_START(sysmemused);
extern CHAR LOS_BLOCK_END(sysmemused);
#endif

extern HWI_PROC_FUNC g_intEntry;

LITE_OS_SEC_TEXT_INIT_REDIRECT VOID OsBoardConfig(VOID)
{
    g_sysMemAddrEnd = (UINTPTR)g_halSectorsRamStart + g_halSectorsRamSize - 1;
}

static VOID OsErrHandle(const CHAR *fileName,
                        UINT32     lineNo,        /* Line number of the erroneous line. */
                        UINT32     errorNo,       /* Error code. */
                        UINT32     paraLen,       /* Length of the input parameter para. */
                        const VOID *para)
{
    UINT32 paraData = 0;
    if (paraLen && (para != NULL)) {
        paraData = *(UINT32*)para;
    }
#ifndef HI1131TEST
    if ((errorNo == OS_ERRNO_MEMCHECK_ERR) ||
        (errorNo == LOS_ERRNO_TSK_STKAREA_TOO_SMALL) ||
        (errorNo == LOS_ERRNO_TSK_STACK_POINTER_ERROR)) {
        LOS_Panic("[%u] %s line:%d errno:0x%x %u %x\r\n",
            ERR_PANIC_MAGIC1, fileName, lineNo, errorNo, paraLen, paraData);
    }
#else
    (VOID)printf("ERROR:%s line:%u errno:0x%x %u %x\r\n", fileName, lineNo, errorNo, paraLen, paraData);
#endif
}

/* get circle which is 64 bits */
LITE_OS_RAM_SECTION VOID OsPlatformGetCyclePatch(UINT32 *cntHigh, UINT32 *cntLow)
{
    UINT64 swTick;
    UINT64 cycle;
    UINT32 intSta;
    UINT32 hwCycle;
    UINTPTR intSave;
    hi_xtal_clock xtalClock;

    intSave = LOS_IntLock();
    swTick = LOS_TickCountGet();

    SysTick->CONTROLREG |= TIMER_LOCK_BIT;
    while (SysTick->CONTROLREG & TIMER_LOCK_BIT) { } // need wait until the bit unlocked
    hwCycle = SysTick->CURRENTVALUE;

    intSta  = SysTick->INTSTATUS;
    if (((intSta & 0x0000001) != 0)) {
        swTick++;
        SysTick->CONTROLREG |= TIMER_LOCK_BIT;
        while (SysTick->CONTROLREG & TIMER_LOCK_BIT) { } // need wait until the bit unlocked
        hwCycle = SysTick->CURRENTVALUE;
    }

    xtalClock = hi_get_xtal_clock();
    if (xtalClock == HI_XTAL_CLOCK_24M) {
        hwCycle = hwCycle / SYSTEM_CLOCK_FREQ_24M * OS_SYS_CLOCK_IN_M;
    } else {
        hwCycle = hwCycle / SYSTEM_CLOCK_FREQ_40M * OS_SYS_CLOCK_IN_M;
    }
    cycle = (((swTick) * g_cyclesPerTick) + (g_cyclesPerTick - hwCycle));
    *cntHigh = cycle >> 32; // get high 32 bit
    *cntLow = cycle & 0xFFFFFFFFU;
    LOS_IntRestore(intSave);
    return;
}

/*****************************************************************************
 Function    : OsRegister
 Description : Configuring the maximum number of tasks
 Input       : None
 Output      : None
 Return      : None
 *****************************************************************************/
LITE_OS_SEC_TEXT_INIT_REDIRECT static VOID OsRegister(VOID)
{
#if (LOSCFG_LIB_CONFIGURABLE == YES)
    g_taskLimit = LOSCFG_BASE_CORE_TSK_LIMIT_CONFIG;
    g_taskMaxNum = g_taskLimit + 1;

#if (LOSCFG_LIB_LIBC == YES)
    errno_array = (INT32 *)&LOS_BLOCK_START(errno);
    (VOID)memset_s(errno_array, (size_t)(&LOS_BLOCK_END(errno) - &LOS_BLOCK_START(errno)), 0,
                   (size_t)(&LOS_BLOCK_END(errno) - &LOS_BLOCK_START(errno)));
#endif

    g_tskMemUsedInfo = (TskMemUsedInfo *)&LOS_BLOCK_START(sysmemused);
    (VOID)memset_s(g_tskMemUsedInfo, (size_t)(&LOS_BLOCK_END(sysmemused) - &LOS_BLOCK_START(sysmemused)), 0,
                   (size_t)(&LOS_BLOCK_END(sysmemused) - &LOS_BLOCK_START(sysmemused)));

    g_osSysClock            = OS_SYS_CLOCK_CONFIG;
    g_minusOneTickPerSecond = LOSCFG_BASE_CORE_TICK_PER_SECOND_CONFIG - 1; /* tick per sencond minus one */
    g_semLimit              = LOSCFG_BASE_IPC_SEM_LIMIT_CONFIG;
    g_muxLimit              = LOSCFG_BASE_IPC_MUX_LIMIT_CONFIG;
    g_queueLimit            = LOSCFG_BASE_IPC_QUEUE_LIMIT_CONFIG;
    g_swtmrLimit            = LOSCFG_BASE_CORE_SWTMR_LIMIT_CONFIG;
    g_taskMinStkSize        = LOSCFG_BASE_CORE_TSK_MIN_STACK_SIZE_CONFIG;
    g_taskIdleStkSize       = LOSCFG_BASE_CORE_TSK_IDLE_STACK_SIZE_CONFIG;
    g_taskDfltStkSize       = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE_CONFIG;
    g_taskSwtmrStkSize      = LOSCFG_BASE_CORE_TSK_SWTMR_STACK_SIZE_CONFIG;
    g_timeSliceTimeOut      = LOSCFG_BASE_CORE_TIMESLICE_TIMEOUT_CONFIG;
    g_userErrFunc.pfnHook   = (LOS_ERRORHANDLE_FUNC)OsErrHandle;
#else
    g_taskMaxNum            = LOSCFG_BASE_CORE_TSK_LIMIT + 1; /* Reserved 1 for IDLE */
#endif
    g_intEntry              = (HWI_PROC_FUNC)OsInterrupt;
#if (LOSCFG_BASE_CORE_TICK_HW_TIME == YES)
    g_tickHandler           = (PLATFORM_TICK_HANDLER)OsPlatformTickHandler;
    g_cycleGetFunc          = (PLATFORM_GET_CYCLE_FUNC)OsPlatformGetCyclePatch;
#endif
#if (LOSCFG_BASE_MISC_TRACK == YES)
    g_trackHook             = (TRACK_PROC_HOOK)OsTrackhook;
#endif

#if (LOSCFG_PLATFORM_EXC == YES)
    g_osBackTraceFunc = OsBackTraceHook;
#endif

#if (LOSCFG_MEM_ENABLE_ALLOC_CHECK == YES)
    LOS_MemEnableIntegrityCheck();
#else
    LOS_MemDisableIntegrityCheck();
#endif

#ifdef LOSCFG_DEBUG_KASAN
    g_kasanLimit = LOSCFG_KASAN_TASK_STACK_LIMIT_NUM;
#endif
    return;
}

/*****************************************************************************
 Function    : LOS_Start
 Description : Task start function
 Input       : None
 Output      : None
 Return      : LOS_OK on success or error code on failure
 *****************************************************************************/
LITE_OS_SEC_TEXT_INIT_REDIRECT UINT32 LOS_Start(VOID)
{
    UINT32 ret;

#if (LOSCFG_BASE_CORE_TICK_HW_TIME == NO)
    ret = OsTickStart();
#else
    ret = OsTimerInit();
#endif
    if (ret != LOS_OK) {
        PRINT_ERR("Liteos system timer init error\n");
        return ret;
    }

    LOS_StartToRun();

    return ret;
}

/*****************************************************************************
 Function    : LOS_KernelInit
 Description : System kernel initialization function, configure all system modules
 Input       : None
 Output      : None
 Return      : LOS_OK on success or error code on failure
 *****************************************************************************/
LITE_OS_SEC_TEXT_INIT_REDIRECT UINT32 LOS_KernelInit(VOID)
{
    UINT32 ret;

    OsRegister();
    ret = OsMemPoolInit();
    if (ret != LOS_OK) {
        PRINT_ERR("OsMemPoolInit error %u\n", ret);
        return ret;
    }

    m_aucSysMem0 = OS_SYS_MEM_ADDR;
    ret = OsMemSystemInit();
    if (ret != LOS_OK) {
        PRINT_ERR("OsMemSystemInit error %u\n", ret);
        return ret;
    }

#if (LOSCFG_PLATFORM_HWI == YES)
    {
        OsHwiInit();
    }
#endif

    ret = OsTaskInit(NULL);
    if (ret != LOS_OK) {
        PRINT_ERR("osTaskInit error\n");
        return ret;
    }

#if (LOSCFG_BASE_CORE_TSK_MONITOR == YES)
    {
        OsTaskMonInit();
    }
#endif

#if (LOSCFG_BASE_CORE_CPUP == YES)
    {
        ret = OsCpupInit(NULL);
        if (ret != LOS_OK) {
            PRINT_ERR("OsCpupInit error\n");
        }
    }
#endif

#if (LOSCFG_BASE_IPC_SEM == YES)
    {
        ret = OsSemInit(NULL);
        if (ret != LOS_OK) {
            return ret;
        }
    }
#endif

#if (LOSCFG_BASE_IPC_MUX == YES)
    {
        ret = OsMuxInit(NULL);
        if (ret != LOS_OK) {
            return ret;
        }
    }
#endif

#if (LOSCFG_BASE_IPC_QUEUE == YES)
    {
        ret = OsQueueInit(NULL);
        if (ret != LOS_OK) {
            PRINT_ERR("osQueueInit error\n");
            return ret;
        }
    }
#endif

#if (LOSCFG_BASE_CORE_SWTMR == YES)
    {
        ret = OsSwtmrInit(NULL);
        if (ret != LOS_OK) {
            PRINT_ERR("OsSwtmrInit error\n");
            return ret;
        }
        ret = OsSwtmrSaveInit();
        if (ret != LOS_OK) {
            PRINT_ERR("OsSwtmrSaveInit error\n");
            return ret;
        }
    }
#endif

#if (LOSCFG_BASE_MISC_TRACK == YES)
    {
        ret = LOS_TrackInit(LOSCFG_BASE_MISC_TRACK_COUNT_CONFIG);
        if (ret != LOS_OK) {
            PRINT_ERR("LOS_TrackInit error\n");
            return ret;
        }
        (VOID)LOS_TrackStart((UINT16)TRACK_TASK_FLAG | (UINT16)TRACK_ISR_FLAG);
    }
#endif

#if(LOSCFG_BASE_CORE_TIMESLICE == YES)
    OsTimesliceInit();
#endif

    ret = OsIdleTaskCreate();
    if (ret != LOS_OK) {
        return ret;
    }

#if (LOSCFG_TEST == YES)
    ret = LOS_TestInit();
    if (ret != LOS_OK) {
        return ret;
    }
#endif

    return LOS_OK;
}
