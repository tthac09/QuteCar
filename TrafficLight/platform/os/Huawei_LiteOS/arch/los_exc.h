/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description : LiteOS exception module.
 * Author : HuangYanhui
 * Create : 2018-03-07
 * Histroy : 2018-12-27 ZhuShengle Support for interrupt input parameters.
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
 * ---------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 * --------------------------------------------------------------------------- */

#ifndef _LOS_EXC_H
#define _LOS_EXC_H

#include "los_typedef.h"
#include "los_hwi.h"

#ifdef LOS_BACKTRACE
extern UINT32 __text_cache_start1_;
extern UINT32 __text_cache_end1_;
extern UINT32 __text_cache_start2_;
extern UINT32 __text_cache_end2_;
extern UINT32 __ram_text_start;
extern UINT32 __ram_text_end;

#endif

#define STACK_CHK_FAIL_MAGIC    100U
#define MEM_PANIC_MAGIC1        101U
#define MEM_PANIC_MAGIC2        102U
#define MEM_PANIC_MAGIC3        103U
#define MEM_PANIC_MAGIC4        104U
#define MEM_PANIC_MAGIC5        105U
#define MEM_PANIC_MAGIC6        106U
#define ERR_PANIC_MAGIC1        107U
#define SYS_PANIC_MAGIC         108U

typedef struct {
        UINT32 mepc;
        UINT32 ra;
        UINT32 sp;
        UINT32 gp;
        UINT32 tp;
        UINT32 t0;
        UINT32 t1;
        UINT32 t2;
        UINT32 s0;
        UINT32 s1;
        UINT32 a0;
        UINT32 a1;
        UINT32 a2;
        UINT32 a3;
        UINT32 a4;
        UINT32 a5;
        UINT32 a6;
        UINT32 a7;
        UINT32 s2;
        UINT32 s3;
        UINT32 s4;
        UINT32 s5;
        UINT32 s6;
        UINT32 s7;
        UINT32 s8;
        UINT32 s9;
        UINT32 s10;
        UINT32 s11;
        UINT32 t3;
        UINT32 t4;
        UINT32 t5;
        UINT32 t6;
        /* Machine CSRs */
        UINT32 mstatus;
        UINT32 mtval;
        UINT32 mcause;
        UINT32 ccause;
        /* a0 value before the syscall */
        // unsigned long orig_a0;
} ExcContext;

/**
 * @ingroup los_exc
 * @brief Kernel panic function.
 *
 * @par Description:
 * Stack function that prints kernel panics.
 * @attention After this function is called and stack information is printed, the system will fail to respond.
 * @attention The input parameter can be NULL.
 * @param fmt   [IN] Type #char* : variadic argument.
 *
 * @retval #None.
 *
 * @par Dependency:
 * <ul><li>los_exc.h: the header file that contains the API declaration.</li></ul>
 * @see None.
 * @since Huawei LiteOS V100R001C00
*/
VOID LOS_Panic(const char *fmt, ...);

/**
 * @ingroup los_exc
 * Exception information structure
 *
 * Description: exception information stored when an exception occurs.
 *
 */
typedef struct {
    UINT16     phase;            /**< Phase in which an exception occurs:
                                      0 means that the exception occurred during initialization,
                                      1 means that the exception occurred in the task,
                                      2 means that the exception occurred in the interrupt. */
    UINT16     type;             /**< Exception type */
    UINT32     faultAddr;        /**< The wrong access address when the exception occurred: */
    UINT32     thrdPid;          /**< An exception occurred during the interrupt indicating the interrupt number,
                                      An exception occurs in the task, indicating the task id,
                                      If it occurs in the initialization, it is 0xffffffff */
    UINT16     nestCnt;          /**< Count of nested exception */
    UINT16     reserved;         /**< Reserved for alignment */
    ExcContext *context;         /**< Hardware context when an exception occurs */
} ExcInfo;

typedef VOID (*OsBackTraceHandler)(VOID);
extern OsBackTraceHandler g_osBackTraceFunc;

typedef VOID (*OsExcCheckHandler)(VOID);
extern OsExcCheckHandler g_excCheckFunc;

/**
 * @ingroup los_exc
 * @brief Register Exc Check Hook.
 *
 * @par Description:
 * Register an exc check hook which will be called when enter exc handler.
 * @attention This Function is make sure for the system to enter in exc handler.
 * @param fmt   [IN] Type #OsExcCheckHandler : exc check hook.
 *
 * @retval #None.
 *
 * @par Dependency:
 * <ul><li>los_exc.h: the header file that contains the API declaration.</li></ul>
 * @see None.
 * @since Huawei LiteOS V100R001C00
*/
VOID LOS_RegExcCheckHook(OsExcCheckHandler func);
VOID LOS_TriggerNMI(VOID);

VOID OsBackTrace(VOID);
VOID OsBackTraceHook(VOID);
#endif
