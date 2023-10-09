/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description : csr encoding.
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

#ifndef _CSR_ENCODING_H
#define _CSR_ENCODING_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define LREG lw
#define SREG sw
#define REGBYTES 4

/****************** stack size *******************/
#ifdef LOS_COMPILE_LDM
#define INT_SIZE_ON_STACK  (16 * REGBYTES)
#else
#define INT_SIZE_ON_STACK  (32 * REGBYTES)
#endif

/*************** task TCB offset *****************/
#define TASK_CB_KERNEL_SP 0x0
#define TASK_CB_STATUS    0x4

#define UINT32_CUT_MASK         0xFFFFFFFF
#define UINT8_CUT_MASK          0xFF

/************** cache register config *******************/
/**
 * csr_bit[0] ICEN Instruction cache is enabled when this bit is set to 1b1.  Default is disabled.
 * Bit[2:1]   ICS  Instruction cache size: 2b00=4KB, 2b01=8KB, 2b10=16KB, 2b11=32KB.  Default is 32KB.
 */
#define ICCTL                   0x7C0

#define ICCTL_ENABLE            0x1

/**
 * Bit[0]    DCEN Data cache is enabled when this bit is set to 1b1.  Default is disabled.
 * Bit[2:1]  DCS  Data cache size: 2b00=4KB, 2b01=8KB, 2b10=16KB, 2b11=32KB.  Default is 32KB.
 */
#define DCCTL                   0x7C1

#define DCCTL_ENABLE            0x1

/**
 * Bit[0]  VA   Instruction cache invalidation by all or VA: 1b0=all, 1b1=VA.
 * Bit[2]  ICIV Initiate instruction cache invalidation when this bit is set to 1b1.
 *              When the instruction cache invalidation is by VA, the virtual address is specified in icinva CSR.
 */
#define VA                      0x1
#define ICIV                    (0x1 << 2)
#define DCIV                    (0x1 << 2)
#define DCC                     (0x1 << 3)

#define ICMAINT                 0x7C2
#define ICACHE_BY_ALL           ICIV
#define ICACHE_BY_VA            (ICIV | VA)

#define DCMAINT                 0x7C3
#define DCACHE_BY_ALL           (DCC | DCIV)
#define DCACHE_BY_VA            (DCC | DCIV | VA)
#define DCACHE_CLEAN_BY_VA      (DCC | VA)

#define ICINCVA                 0x7C4
#define DCINCVA                 0x7C5

/**
 * Bit[0]   IAPEN  Instruction auto prefetch is initiated when this bit is set to 1b1 and the number of consecutive
 *                 cache lines equal to specified ICL is observed.
 * Bit[3:1] ICL    Number of contiguous instruction cache lines are missed consecutively before instruction cache auto
 *                 prefetch is initiated. Valid values are between 1 (3d0) and 8 (3d7).  Default is 4 (3d3).
 */
#define APREFI                  0x7C6
/**
 * Bit[0]   IAPEN  Data auto prefetch is initiated when this bit is set to 1b1 and the number of consecutive
 *                 cache lines equal to specified ICL is observed.
 * Bit[3:1] ICL    Number of contiguous data cache lines are missed consecutively before data cache auto
 *                 prefetch is initiated. Valid values are between 1 (3d0) and 8 (3d7).  Default is 4 (3d3).
 * Bit[4]   SAPEN  Data auto prefetch is initiated when this set is set to 1b1 and the number of consecutive cache
lines
 *                 of the store accesses missed equal to or larger than specified SCL is observed.
 * Bit[7:5] SCL    Number of contiguous data cache lines of store accesses are missed consecutively before data cache
 *                 auto prefetch is initiated. Valid values are between 1 (3d0) and 8 (3d7).  Default is 4 (3d3).
 */
#define APREFD                  0x7C7

#define IAPEN                   0x1
#define ICL_1                   (0x0 << 1)
#define ICL_2                   (0x1 << 1)
#define ICL_3                   (0x2 << 1)
#define ICL_4                   (0x3 << 1)
#define ICL_5                   (0x4 << 1)
#define ICL_6                   (0x5 << 1)
#define ICL_7                   (0x6 << 1)
#define ICL_8                   (0x7 << 1)

#define SAPEN                   (0x1 << 4)
#define SCL_1                   (0x0 << 5)
#define SCL_2                   (0x1 << 5)
#define SCL_3                   (0x2 << 5)
#define SCL_4                   (0x3 << 5)
#define SCL_5                   (0x4 << 5)
#define SCL_6                   (0x5 << 5)
#define SCL_7                   (0x6 << 5)
#define SCL_8                   (0x7 << 5)

/*************** pmp register config *******************/
/**
 * ATTR[7]  ATTR[6]  ATTR[5]  ATTR[4]  ATTR[3]  ATTR[2]  ATTR[1] ATTR[0]
 * ATTR[15] ATTR[14] ATTR[13] ATTR[12] ATTR[11] ATTR[10] ATTR[9] ATTR[8]
 *    4        4        4        4        4        4        4       4
 *
 * Bit[31:0] ATTR[n] Specify the memory attribute of the lower 8 MPU regions n. The definition of memory attribute is
 *                   the same as AMBA 3.0 AXI ARCACHE(instruction fetch and load execution) and
 *                   AWCACHE(store execution) signals. HiMiDeerV100 only supports the following memory attributes:
 *                   device non-bufferable, normal non-cacheable bufferable, write-back read-allocate,
 *                   write-back write-allocate and write-back read- and write-allocate.
 */
#define MEMATTRL                0x7D8
#define MEMATTRH                0x7D9
/**
 * ME[15]NS[15] ME[14]NS[14] ME[13]NS[13] ... ME[0]NS[0]
 *  1     1      1     1      1     1          1    1
 *
 * Bit[2n]   NS[2n]   Specify if the MPU region 2n is secure or non-secure: 1b0=secure, 1b1=non-secure.
 *                    Default is secure.
 * Bit[2n+1] ME[2n+1] Specify if the MPU region 2n+1 bypasses system MMU: 1b0=disable SoC IOMMU (MMU in system level),
 *                    1b1=enable SoC IOMMU.  Default is IOMMU disabled.
 */
#define SECTL                   0x7DA

/************** mstatus Register *******************/
#define MCAUSE_INSN_MISALIGN    0x0
#define MCAUSE_INSN_FAULT       0x1
#define MCAUSE_INSN_ILLEGAL     0x2
#define MCAUSE_BRKT             0x3
#define MCSUSE_LOAD_MISALIGN    0x4
#define MCAUSE_LOAD_FAULT       0x5
#define MCAUSE_AMO_MISALIGN     0x6
#define MCAUSE_AMO_FAULT        0x7
#define MCAUSE_ECALL_U          0x8
#define MCAUSE_ECALL_M          0xB
#define MCAUSE_INSN_PAGE_FAULT  0xC
#define MCAUSE_LOAD_PAGE_FAULT  0xD
#define MCAUSE_AMO_PAGE_FAULT   0xF

#define EXC_SIZE_ON_STACK       (160)   // 16byte align

#define LOS_MSTATUS_UIE         0x00000001
#define LOS_MSTATUS_SIE         0x00000002
#define LOS_MSTATUS_HIE         0x00000004
#define LOS_MSTATUS_MIE         0x00000008
#define LOS_MSTATUS_UPIE        0x00000010
#define LOS_MSTATUS_SPIE        0x00000020
#define LOS_MSTATUS_HPIE        0x00000040
#define LOS_MSTATUS_MPIE        0x00000080
#define LOS_MSTATUS_SPP         0x00000100
#define LOS_MSTATUS_HPP         0x00000600
#define LOS_MSTATUS_MPP         0x00001800
#define LOS_MSTATUS_FS          0x00006000
#define LOS_MSTATUS_XS          0x00018000
#define LOS_MSTATUS_MPRV        0x00020000
#define LOS_MSTATUS_PUM         0x00040000
#define LOS_MSTATUS_VM          0x1F000000
#define LOS_MSTATUS32_SD        0x80000000
#define LOS_MSTATUS64_SD        0x8000000000000000

#define LOS_MCAUSE32_CAUSE      0x7FFFFFFF
#define LOS_MCAUSE64_CAUSE      0x7FFFFFFFFFFFFFFF
#define LOS_MCAUSE32_INT        0x80000000
#define LOS_MCAUSE64_INT        0x8000000000000000

#define LOSCFG_MSTATUS_M        (LOS_MSTATUS_MPP | LOS_MSTATUS_MPIE)
#define LOSCFG_MSTATUS_C        (LOS_MSTATUS_MPP | LOS_MSTATUS_MPIE | LOS_MSTATUS_MIE)

#define LOS_SSTATUS_UIE         0x00000001
#define LOS_SSTATUS_SIE         0x00000002
#define LOS_SSTATUS_UPIE        0x00000010
#define LOS_SSTATUS_SPIE        0x00000020
#define LOS_SSTATUS_SPP         0x00000100
#define LOS_SSTATUS_FS          0x00006000
#define LOS_SSTATUS_XS          0x00018000
#define LOS_SSTATUS_PUM         0x00040000
#define LOS_SSTATUS32_SD        0x80000000
#define LOS_SSTATUS64_SD        0x8000000000000000

#define LOS_IRQ_S_SOFT          1
#define LOS_IRQ_H_SOFT          2
#define LOS_IRQ_M_SOFT          3
#define LOS_IRQ_S_TIMER         5
#define LOS_IRQ_H_TIMER         6
#define LOS_IRQ_M_TIMER         7
#define LOS_IRQ_S_EXT           9
#define LOS_IRQ_H_EXT           10
#define LOS_IRQ_M_EXT           11
#define LOS_IRQ_COP             12
#define LOS_IRQ_HOST            13
#define LOS_IRQ_LOCIE0          26
#define LOS_IRQ_LOCIE1          27
#define LOS_IRQ_LOCIE2          28
#define LOS_IRQ_LOCIE3          29
#define LOS_IRQ_LOCIE4          30
#define LOS_IRQ_LOCIE5          31

// rv_custom_local_interrupt 6 -31
#define LOS_IRQ_LOCIE6           0
#define LOS_IRQ_LOCIE7           1
#define LOS_IRQ_LOCIE8           2
#define LOS_IRQ_LOCIE9           3
#define LOS_IRQ_LOCIE10          4
#define LOS_IRQ_LOCIE11          5
#define LOS_IRQ_LOCIE12          6
#define LOS_IRQ_LOCIE13          7
#define LOS_IRQ_LOCIE14          8
#define LOS_IRQ_LOCIE15          9
#define LOS_IRQ_LOCIE16          10
#define LOS_IRQ_LOCIE17          11
#define LOS_IRQ_LOCIE18          12
#define LOS_IRQ_LOCIE19          13
#define LOS_IRQ_LOCIE20          14
#define LOS_IRQ_LOCIE21          15
#define LOS_IRQ_LOCIE22          16
#define LOS_IRQ_LOCIE23          17
#define LOS_IRQ_LOCIE24          18
#define LOS_IRQ_LOCIE25          19
#define LOS_IRQ_LOCIE26          20
#define LOS_IRQ_LOCIE27          21
#define LOS_IRQ_LOCIE28          22
#define LOS_IRQ_LOCIE29          23
#define LOS_IRQ_LOCIE30          24
#define LOS_IRQ_LOCIE31          25

// rv_nmi
#define LOS_IRQ_NMI              12
#define LOS_MIP_NMI             (1 << LOS_IRQ_NMI)

#define LOS_MIP_SSIP            (1 << LOS_IRQ_S_SOFT)
#define LOS_MIP_HSIP            (1 << LOS_IRQ_H_SOFT)
#define LOS_MIP_MSIP            (1 << LOS_IRQ_M_SOFT)
#define LOS_MIP_STIP            (1 << LOS_IRQ_S_TIMER)
#define LOS_MIP_HTIP            (1 << LOS_IRQ_H_TIMER)
#define LOS_MIP_MTIP            (1 << LOS_IRQ_M_TIMER)
#define LOS_MIP_SEIP            (1 << LOS_IRQ_S_EXT)
#define LOS_MIP_HEIP            (1 << LOS_IRQ_H_EXT)
#define LOS_MIP_MEIP            (1 << LOS_IRQ_M_EXT)
#define LOS_MIP_LOCIE0          (1 << LOS_IRQ_LOCIE0)
#define LOS_MIP_LOCIE1          (1 << LOS_IRQ_LOCIE1)
#define LOS_MIP_LOCIE2          (1 << LOS_IRQ_LOCIE2)
#define LOS_MIP_LOCIE3          (1 << LOS_IRQ_LOCIE3)
#define LOS_MIP_LOCIE4          (1 << LOS_IRQ_LOCIE4)
#define LOS_MIP_LOCIE5          (1 << LOS_IRQ_LOCIE5)

// rv_custom_csr
#define LOCIPD0_LOCIE6           (1 << LOS_IRQ_LOCIE6)
#define LOCIPD0_LOCIE7           (1 << LOS_IRQ_LOCIE7)
#define LOCIPD0_LOCIE8           (1 << LOS_IRQ_LOCIE8)
#define LOCIPD0_LOCIE9           (1 << LOS_IRQ_LOCIE9)
#define LOCIPD0_LOCIE10          (1 << LOS_IRQ_LOCIE10)
#define LOCIPD0_LOCIE11          (1 << LOS_IRQ_LOCIE11)
#define LOCIPD0_LOCIE12          (1 << LOS_IRQ_LOCIE12)
#define LOCIPD0_LOCIE13          (1 << LOS_IRQ_LOCIE13)
#define LOCIPD0_LOCIE14          (1 << LOS_IRQ_LOCIE14)
#define LOCIPD0_LOCIE15          (1 << LOS_IRQ_LOCIE15)
#define LOCIPD0_LOCIE16          (1 << LOS_IRQ_LOCIE16)
#define LOCIPD0_LOCIE17          (1 << LOS_IRQ_LOCIE17)
#define LOCIPD0_LOCIE18          (1 << LOS_IRQ_LOCIE18)
#define LOCIPD0_LOCIE19          (1 << LOS_IRQ_LOCIE19)
#define LOCIPD0_LOCIE20          (1 << LOS_IRQ_LOCIE20)
#define LOCIPD0_LOCIE21          (1 << LOS_IRQ_LOCIE21)
#define LOCIPD0_LOCIE22          (1 << LOS_IRQ_LOCIE22)
#define LOCIPD0_LOCIE23          (1 << LOS_IRQ_LOCIE23)
#define LOCIPD0_LOCIE24          (1 << LOS_IRQ_LOCIE24)
#define LOCIPD0_LOCIE25          (1 << LOS_IRQ_LOCIE25)
#define LOCIPD0_LOCIE26          (1 << LOS_IRQ_LOCIE26)
#define LOCIPD0_LOCIE27          (1 << LOS_IRQ_LOCIE27)
#define LOCIPD0_LOCIE28          (1 << LOS_IRQ_LOCIE28)
#define LOCIPD0_LOCIE29          (1 << LOS_IRQ_LOCIE29)
#define LOCIPD0_LOCIE30          (1 << LOS_IRQ_LOCIE30)
#define LOCIPD0_LOCIE31          (1 << LOS_IRQ_LOCIE31)

// rv_custom_csr
#define LOCIPRI0                (0xBC0)
#define LOCIPRI1                (0xBC1)
#define LOCIPRI2                (0xBC2)
#define LOCIPRI3                (0xBC3)
#define LOCIPRI4                (0xBC4)
#define LOCIPRI5                (0xBC5)

#define LOCIPRI(x)          LOCIPRI##x

#define LOCIEN0                 (0xBE0)
#define LOCIPD0                 (0xBE8)
#define PRITHD                  (0xBFE)

#define LOS_SIP_SSIP            LOS_MIP_SSIP
#define LOS_SIP_STIP            LOS_MIP_STIP

#define LOS_PRV_U               0
#define LOS_PRV_S               1
#define LOS_PRV_H               2
#define LOS_PRV_M               3

#define LOS_VM_MBARE            0
#define LOS_VM_MBB              1
#define LOS_VM_MBBID            2
#define LOS_VM_SV32             8
#define LOS_VM_SV39             9
#define LOS_VM_SV48             10

#define DEFAULT_RSTVEC          0x00001000
#define DEFAULT_NMIVEC          0x00001004
#define DEFAULT_MTVEC           0x00001010
#define CONFIG_STRING_ADDR      0x0000100C
#define EXT_IO_BASE             0x40000000
#define DRAM_BASE               0x80000000

// page table entry (PTE) fields
#define LOS_PTE_V               0x001 // Valid
#define LOS_PTE_TYPE            0x01E // Type
#define LOS_PTE_R               0x020 // Referenced
#define LOS_PTE_D               0x040 // Dirty
#define LOS_PTE_SOFT            0x380 // Reserved for Software

#define LOS_PTE_TYPE_TABLE        0x00
#define LOS_PTE_TYPE_TABLE_GLOBAL 0x02
#define LOS_PTE_TYPE_URX_SR       0x04
#define LOS_PTE_TYPE_URWX_SRW     0x06
#define LOS_PTE_TYPE_UR_SR        0x08
#define LOS_PTE_TYPE_URW_SRW      0x0A
#define LOS_PTE_TYPE_URX_SRX      0x0C
#define LOS_PTE_TYPE_URWX_SRWX    0x0E
#define LOS_PTE_TYPE_SR           0x10
#define LOS_PTE_TYPE_SRW          0x12
#define LOS_PTE_TYPE_SRX          0x14
#define LOS_PTE_TYPE_SRWX         0x16
#define LOS_PTE_TYPE_SR_GLOBAL    0x18
#define LOS_PTE_TYPE_SRW_GLOBAL   0x1A
#define LOS_PTE_TYPE_SRX_GLOBAL   0x1C
#define LOS_PTE_TYPE_SRWX_GLOBAL  0x1E

#define LOS_PTE_PPN_SHIFT       10

#define LOS_PTE_TABLE(PTE)      ((0x0000000AU >> ((PTE) & 0x1F)) & 1)
#define LOS_PTE_UR(PTE)         ((0x0000AAA0U >> ((PTE) & 0x1F)) & 1)
#define LOS_PTE_UW(PTE)         ((0x00008880U >> ((PTE) & 0x1F)) & 1)
#define LOS_PTE_UX(PTE)         ((0x0000A0A0U >> ((PTE) & 0x1F)) & 1)
#define LOS_PTE_SR(PTE)         ((0xAAAAAAA0U >> ((PTE) & 0x1F)) & 1)
#define LOS_PTE_SW(PTE)         ((0x88888880U >> ((PTE) & 0x1F)) & 1)
#define LOS_PTE_SX(PTE)         ((0xA0A0A000U >> ((PTE) & 0x1F)) & 1)

#define LOS_PTE_CHECK_PERM(PTE, SUPERVISOR, STORE, FETCH) \
    ((STORE) ? ((SUPERVISOR) ? LOS_PTE_SW(PTE) : LOS_PTE_UW(PTE)) : \
        ((FETCH) ? ((SUPERVISOR) ? LOS_PTE_SX(PTE) : LOS_PTE_UX(PTE)) : \
            ((SUPERVISOR) ? LOS_PTE_SR(PTE) : LOS_PTE_UR(PTE))))

# define LOS_MSTATUS_SD         LOS_MSTATUS32_SD
# define LOS_SSTATUS_SD         LOS_SSTATUS32_SD
# define LOS_PGLEVEL_BITS 10
# define MCAUSE_INT             LOS_MCAUSE32_INT
# define MCAUSE_CAUSE           LOS_MCAUSE32_CAUSE

#define LOS_PGSHIFT       12
#define LOS_PGSIZE        (1 << LOS_PGSHIFT)

#ifndef __ASSEMBLER__
#ifdef __GNUC__

#define READ_CSR(csrReg) ({                             \
    UINT32 tmp_;                                        \
    __asm__ volatile ("csrr %0, " #csrReg : "=r"(tmp_));    \
    tmp_;                                               \
})

#define WRITE_CSR(csrReg, csrVal) do {                              \
    if (__builtin_constant_p(csrVal) && ((UINT32)(csrVal) < 32)) {  \
        __asm__ volatile ("csrw " #csrReg ", %0" :: "i"(csrVal));       \
    } else {                                                        \
        __asm__ volatile ("csrw " #csrReg ", %0" :: "r"(csrVal));       \
    }                                                               \
} while (0)

#define SET_CSR(csrReg, csrBit) do {                                            \
    UINT32 tmp_;                                                                \
    if (__builtin_constant_p(csrBit) && ((UINT32)(csrBit) < 32)) {              \
        __asm__ volatile ("csrrs %0, " #csrReg ", %1" : "=r"(tmp_) : "i"(csrBit));  \
    } else {                                                                    \
        __asm__ volatile ("csrrs %0, " #csrReg ", %1" : "=r"(tmp_): "r"(csrBit));   \
    }                                                                           \
    (VOID)tmp_;                                                                 \
} while (0)

#define CLEAR_CSR(csrReg, csrBit) do {                                          \
    UINT32 tmp_;                                                                \
    if (__builtin_constant_p(csrBit) && ((UINT32)(csrBit) < 32)) {              \
        __asm__ volatile ("csrrc %0, " #csrReg ", %1" : "=r"(tmp_) : "i"(csrBit));  \
    } else {                                                                    \
        __asm__ volatile ("csrrc %0, " #csrReg ", %1" : "=r"(tmp_) : "r"(csrBit));  \
    }                                                                           \
    (VOID)tmp_;                                                                 \
} while (0)

// rv_custom_csr
#define READ_CUSTOM_CSR(csrReg) ({                                              \
    UINT32 tmp_;                                                                \
    __asm__ volatile ("csrr %0, %1" : "=r"(tmp_) : "i"(csrReg));                    \
    tmp_;                                                                       \
})

#define WRITE_CUSTOM_CSR_VAL(csrRegAddr, csrVal) do {                           \
    if (__builtin_constant_p(csrVal))  {                                        \
        __asm__ volatile("li t0," "%0" : : "i"(csrVal));                            \
    } else {                                                                    \
        __asm__ volatile("mv t0," "%0" : : "r"(csrVal));                            \
    }                                                                           \
    __asm__ volatile("csrw %0, t0" :: "i"(csrRegAddr));                             \
} while (0)

#define SET_CUSTOM_CSR(csrRegAddr, csrBit) do {                                 \
    if (__builtin_constant_p(csrBit) && ((UINT32)(csrBit) < 32)) {              \
        __asm__ volatile("li t0," "%0" : : "i"(csrBit));                            \
    } else {                                                                    \
        __asm__ volatile("mv t0," "%0" : : "r"(csrBit));                            \
    }                                                                           \
    __asm__ volatile("csrs %0, t0" :: "i"(csrRegAddr));                             \
} while (0)

#define CLEAR_CUSTOM_CSR(csrRegAddr, csrBit) do {                               \
    if (__builtin_constant_p(csrBit) && ((UINT32)(csrBit) < 32)) {              \
        __asm__ volatile("li t0," "%0" : : "i"(csrBit));                            \
    } else {                                                                    \
        __asm__ volatile("mv t0," "%0" : : "r"(csrBit));                            \
    }                                                                           \
    __asm__ volatile("csrc %0, t0" :: "i"(csrRegAddr));                             \
} while (0)

#endif  /*__GNUC__*/
#endif  /*__ASSEMBLER__*/
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
#endif /* _CSR_ENCODING_H */
