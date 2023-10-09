/*----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description : LiteOS pmp module implemention.
 * Author : HuangYanhui
 * Create : 2018-03-07
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
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

#ifndef LOS_HAL_H
#define LOS_HAL_H

#include "encoding.h"
#include "los_typedef.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

typedef struct {
    volatile UINT32 LOADCOUNT;                              /*!< Offset: 0x00 */
    volatile UINT32 CURRENTVALUE;                           /*!< Offset: 0x04 */
    volatile UINT32 CONTROLREG;                             /*!< Offset: 0x08 */
    volatile UINT32 EOI;                                    /*!< Offset: 0x0C */
    volatile UINT32 INTSTATUS;                              /*!< Offset: 0x10 */
} Timer_Type;

#define CFG_TIMER_CTRL                  0xDA
#define CFG_TIMER_CTRL_WITH_INTERRUPT   0xE2
#define REG_TIMER_LOAD                  0x0
#define REG_TIMER_VALUE                 0x4
#define REG_TIMER_CONTROL               0x8
#define REG_TIMER_INTCLR                0xC
#define REG_TIMER_RIS                   0x10
#define REG_TIMER_MIS                   0x14
#define REG_TIMER_BGLOAD                0x18

#define REG_BASE_PERI_CTRL              0x20000
#define EXT_INT_REG_BASE                (REG_BASE_PERI_CTRL + 0xc20)
#define NMI_REG_BASE                    0x40010258
#define NMI_REG_BASE_NEW                NMI_REG_BASE

/*------------------------------------------------------------------------------
 * mtimer register & config.
 */
#if   defined(LOS_HIMIDEER_RV32)
#define REG_BASE_MTIMER               0x23000
#elif defined(LOS_HIFONEV320_RV32)
#define REG_BASE_MTIMER               0xF8B32000
#endif
#define REG_TIMER_CTRL                0x0
#define REG_TIMER_DIV                 0x04
#define REG_TIMER_MTIME               0x8
#define REG_TIMER_MTIME_H             0xC
#define REG_TIMER_MTIMECMP            0x10
#define REG_TIMER_MTIMECMP_H          0x14
#define TIMER_CTRL_ENABLE             0
#define TIMER_CTRL_CLK                1
#define TIMER_CTRL_STOP               2

#define OS_TIMER_TICK_BASE_REG ((Timer_Type *)(TIMER_3_BASE_ADDR))

#define SysTick OS_TIMER_TICK_BASE_REG

/*------------------------------------------------------------------------------
 * Interrupt enable/disable.
 */
VOID __disable_irq(VOID);
VOID __enable_irq(VOID);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif  /* LOS_HAL_H */
