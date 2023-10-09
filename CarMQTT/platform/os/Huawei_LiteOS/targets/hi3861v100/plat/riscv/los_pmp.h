/* ----------------------------------------------------------------------------
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
 * ---------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 * --------------------------------------------------------------------------- */

#ifndef _LOS_PMP_H
#define _LOS_PMP_H

#include "los_base.h"
#include "los_task.h"
#include "encoding.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * @ingroup los_pmp
 * Locking PMP entry.
 */
#define PMP_RGN_LOCK                0x80            // lock pmp

/**
 * @ingroup los_pmp
 * Address Matching.
 */
#define PMP_RGN_ADDR_MATCH_OFF      0               // Null region (disabled)
#define PMP_RGN_ADDR_MATCH_TOR      0x8             // Top of range
#define PMP_RGN_ADDR_MATCH_NA4      0x10            // Naturally aligned four-byte region
#define PMP_RGN_ADDR_MATCH_NAPOT    0x18            // Naturally aligned power-of-two region, >= 8 bytes
#define PMP_RGN_ADDR_MATCH_MASK     0x18
#define PMP_ADDR_ID0                0
#define PMP_ADDR_ID1                1
#define PMP_ADDR_ID2                2
#define PMP_ADDR_ID3                3
#define PMP_ADDR_ID4                4
#define PMP_ADDR_ID5                5
#define PMP_ADDR_ID6                6
#define PMP_ADDR_ID7                7
#define PMP_ADDR_ID8                8
#define PMP_ADDR_ID9                9
#define PMP_ADDR_ID10               10
#define PMP_ADDR_ID11               11
#define PMP_ADDR_ID12               12
#define PMP_ADDR_ID13               13
#define PMP_ADDR_ID14               14
#define PMP_ADDR_ID15               15
#define LOS_BIT0                    1
#define LOS_BIT1                    (1 << 1)
#define LOS_BIT2                    (1 << 2)
#define LOS_BIT3                    (1 << 3)
#define LOS_BIT4                    (1 << 4)
#define PMP_CONFIG_NUMBER_0         0
#define PMP_CONFIG_NUMBER_1         1
#define PMP_CONFIG_NUMBER_2         2
#define PMP_CONFIG_NUMBER_3         3


/**
 * @ingroup los_pmp
 * Access permission.
 */
#define PMPCFG_R_BIT    0
#define PMPCFG_W_BIT    1
#define PMPCFG_X_BIT    2

enum MemReadAcc {
    E_MEM_RD_ACC_NON_RD = 0,
    E_MEM_RD_ACC_RD = 1,
};

enum MemWriteReadAcc {
    E_MEM_WR_ACC_NON_WR = 0,
    E_MEM_WR_ACC_WR = 1,
};

enum MemExecuteAcc {
    E_MEM_EX_ACC_NON_EX = 0,
    E_MEM_EX_ACC_EX = 1,
};

typedef struct {
    enum MemReadAcc       readAcc;
    enum MemWriteReadAcc  writeAcc;
    enum MemExecuteAcc    excuteAcc;
} PmpAccInfo;

/**
 * @ingroup los_pmp
 * memory attribute.
 */
typedef enum {
    MEM_ATTR_DEV_NON_BUF = 0,                 // 4b0000, device non-bufferable
    MEM_ATTR_NORM_NON_CA_NON_BUF = 2,         // 4b0010, normal non-cacheable non-bufferable
    MEM_ATTR_NORM_NON_CA_BUF = 3,             // 4b0011, normal non-cacheable bufferable
    MEM_ATTR_WB_RD_ALLOC = 7,                 // 4b0111, write-back read-only allocate
    MEM_ATTR_WB_WR_ALLOC = 11,                // 4b1011, write-back write-only allocate
    MEM_ATTR_WB_RW_ALLOC = 15,                // 4b1111, write-back write and read allocate
} PmpMemAttrInfo;

typedef enum {
    SEC_CONTRLO_SECURE_NOMMU = 0,             // secure and disable SoC IOMMU
    SEC_CONTRLO_NOSECURE_NOMMU = 1,           // no-secure and disable SoC IOMMU
    SEC_CONTRLO_SECURE_MMU = 2,               // secure and enable SoC IOMMU
    SEC_CONTRLO_NOSECURE_MMU = 3,             // no-secure and enable SoC IOMMU
} PmpSeControlInfo;

typedef enum {
    CACHE_4KB  = 0,
    CACHE_8KB  = 2,
    CACHE_16KB = 4,
    CACHE_32KB = 6,
} CacheSize;

/**
 * @ingroup los_pmp
 * PMP entry info
 * uwBaseAddress must be in the range of RAM, and
 * uwRegionSize granularity must be the multiple of cache line size
 * if addrress matching mode  equals to NPAOT, the base address and size should match the type and size
 * uwAccessPermission is not arbitrary input
 */
typedef struct {
    // PMP address register information
    UINT32 uwBaseAddress; // set the base address of the protected region, the base address must be in the range of RAM
    UINT32 uwRegionSize;  // PMP entry region size

    // PMP memory attribute register information
    PmpMemAttrInfo memAttr;
    PmpSeControlInfo  seCtl;

    // PMP configuration register information
    BOOL  bLocked;                    // whether lock
    PmpAccInfo accPermission;         // access permission of read, write and instruction execution
    UINT8 ucAddressMatch;             // address matching mode of PMP configuration

    // Index of PMP entry
    UINT8  ucNumber;                  // number of PMP register to be checked
} PMP_ENTRY_INFO;

/**
 * @ingroup los_pmp
 * Region size
 * not including TOR capacity, for pmpaddr(i-1) <= a < pmpaddr(i),
 * i.e. Capacity(TOR) = pmpaddr(i) - pmpaddr(i-1)
 * if address matching mode equals to NAPOT or NA4,
 * we define the shift bit number to represent the capacity of PMP entry
 * region size
 * range: PMP_REGION_SIZE_8B ~ PMP_REGION_SIZE_16GB
 */
#define PMP_REGION_SIZE_4B                                     (1 << 2)   // 4-byte NAPOT range


/*
 * When address matching = OFF , it will return one invalid value 0xFFFFFFFF
 */
#define PMP_ADDR_MATCH_OFF                                     0xFFFFFFFF

/*
 * The PMP entry size granularity must be the multiple of cache line size.
 * Besides, the PMP region start address and end address must be cache line aligned.
 */
/* set pmpcfg.A to NAPOT */     /* letf-shift bit num */   /* PMP region size */
#define PMP_REGION_SIZE_8B                 (1 << 3)        // 8-byte NAPOT range
#define PMP_REGION_SIZE_16B                (1 << 4)        // 16-byte NAPOT range
#define PMP_REGION_SIZE_32B                (1 << 5)        // 32-byte NAPOT range
#define PMP_REGION_SIZE_64B                (1 << 6)        // 64-byte NAPOT range
#define PMP_REGION_SIZE_128B               (1 << 7)        // 128-byte NAPOT range
#define PMP_REGION_SIZE_256B               (1 << 8)        // 256-byte NAPOT range
#define PMP_REGION_SIZE_512B               (1 << 9)        // 512-byte NAPOT range
#define PMP_REGION_SIZE_1KB                (1 << 10)       // 1K-byte NAPOT range
#define PMP_REGION_SIZE_2KB                (1 << 11)       // 2K-byte NAPOT range
#define PMP_REGION_SIZE_4KB                (1 << 12)       // 4K-byte NAPOT range
#define PMP_REGION_SIZE_8KB                (1 << 13)       // 8K-byte NAPOT range
#define PMP_REGION_SIZE_16KB               (1 << 14)       // 16K-byte NAPOT range
#define PMP_REGION_SIZE_32KB               (1 << 15)       // 32K-byte NAPOT range
#define PMP_REGION_SIZE_64KB               (1 << 16)       // 64K-byte NAPOT range
#define PMP_REGION_SIZE_128KB              (1 << 17)       // 128K-byte NAPOT range
#define PMP_REGION_SIZE_256KB              (1 << 18)       // 256K-byte NAPOT range
#define PMP_REGION_SIZE_512KB              (1 << 19)       // 512K-byte NAPOT range
#define PMP_REGION_SIZE_1MB                (1 << 20)       // 1M-byte NAPOT range
#define PMP_REGION_SIZE_2MB                (1 << 21)       // 2M-byte NAPOT range
#define PMP_REGION_SIZE_4MB                (1 << 22)       // 4M-byte NAPOT range
#define PMP_REGION_SIZE_8MB                (1 << 23)       // 8M-byte NAPOT range
#define PMP_REGION_SIZE_16MB               (1 << 24)       // 16M-byte NAPOT range
#define PMP_REGION_SIZE_32MB               (1 << 25)       // 32M-byte NAPOT range
#define PMP_REGION_SIZE_64MB               (1 << 26)       // 64M-byte NAPOT range
#define PMP_REGION_SIZE_128MB              (1 << 27)       // 128M-byte NAPOT range
#define PMP_REGION_SIZE_256MB              (1 << 28)       // 256M-byte NAPOT range
#define PMP_REGION_SIZE_512MB              (1 << 29)       // 512M-byte NAPOT range
#define PMP_REGION_SIZE_1GB                (1 << 30)       // 1G-byte NAPOT range
#define PMP_REGION_SIZE_2GB                (1 << 31)       // 2G-byte NAPOT range
#define PMP_REGION_SIZE_4GB                (1 << 32)       // 4G-byte NAPOT range
#define PMP_REGION_SIZE_8GB                (1 << 33)       // 8G-byte NAPOT range
#define PMP_REGION_SIZE_16GB               (1 << 34)       // 16G-byte NAPOT range


/**
* @ingroup los_cpup
* PMP error code: The pointer to an input parameter is NULL.
*
* Value: 0x02001200
*
* Solution: Check whether the pointer to the input parameter is usable.
*/
#define LOS_ERRNO_PMP_PTR_NULL                    LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x00)

/**
* @ingroup los_pmp
* PMP error code: The base address is not aligned to the boundary of the region capacity.
*
* Value: 0x02001201
*
* Solution: Check base address.
*/
#define LOS_ERRNO_PMP_INVALID_BASE_ADDRESS        LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x01)

/**
* @ingroup los_pmp
* PMP error code: Capacity less than cacheline bytes.
* The PMP entry size granularity must be the multiple of cache line size.
*
* Value: 0x02001202
*
* Solution: Guaranteed that the capacity is greater than or equal to cacheline, supporting cacheline granularity.
*/
#define LOS_ERRNO_PMP_INVALID_CAPACITY            LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x02)

/**
* @ingroup los_cpup
* PMP error code: Type extension that given is invalid.
*
* Value: 0x0200120a
*
* Solution: memory attribute configuration only support 0000,0010,0011,0111,,1011,1111
*/
#define LOS_ERRNO_PMP_ATTR_INVALID                LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x03)

/**
* @ingroup los_pmp
* PMP error code: Invalid number.
*
* Value: 0x02001204
*
* Solution: Enter a valid number.
*/
#define LOS_ERRNO_PMP_INVALID_NUMBER              LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x04)

/**
* @ingroup los_cpup
* PMP error code: Region has already been enabled.
*
* Value: 0x02001205
*
* Solution: If you want to re enable the region, please first in addition to the region.
*/
#define LOS_ERRNO_PMP_REGION_IS_ENABLED           LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x05)

/**
* @ingroup los_cpup
* PMP error code: Region has already been disabled.
*
* Value: 0x02001206
*
* Solution: If you want to re enable the region, please first in addition to the region.
*/
#define LOS_ERRNO_PMP_REGION_IS_DISABLED          LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x06)

/**
* @ingroup los_cpup
* PMP error code: Invalid access.
*
* Value: 0x02001207
*
* Solution: Checking whether the access is correct.
*/
#define LOS_ERRNO_PMP_REGION_INVALID_ACCESS       LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x07)

/**
* @ingroup los_cpup
* PMP error code: Base address is not in range.
*
* Value: 0x02001208
*
* Solution: Checking base address
*/
#define LOS_ERRNO_PMP_BASE_ADDRESS_NOT_IN_RANGE     LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x08)

/**
* @ingroup los_cpup
* PMP error code: According to the current base address, the size of the application is too big.
*
* Value: 0x02001209
*
* Solution: baseAddress + regionSize must not exceed RAM Max address
*/
#define LOS_ERRNO_PMP_REGION_SIZE_IS_TOO_BIG      LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x09)

/**
* @ingroup los_cpup
* PMP error code: According to the current configuration, the PMP entry is locked.
*
* Value: 0x0200120a
*
* Solution: if PMP entry is locked, then write configuration and address register will be ignored,
* you should reset the system.
*/
#define LOS_ERRNO_PMP_ENTRY_IS_LOCKED             LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x0a)

/**
* @ingroup los_cpup
* PMP error code: Invalid configuration register of PMP entry.
*
* Value: 0x0200120b
*
* Solution: Checking whether register can be written.
*/
#define LOS_ERRNO_PMP_INVALID_CONFIG              LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x0b)

/**
* @ingroup los_cpup
* PMP error code: According to the current configuration,
* the NAPOT type and size is not matched in NAPOT range encoding.
*
* Value: 0x0200120c
*
* Solution: baseAddress + regionSize must obey the NAPOT range encoding rules.
*/
#define LOS_ERRNO_PMP_NAPOT_ENCODING_NON_MATCHED      LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x0c)

/**
* @ingroup los_cpup
* PMP error code: According to the current configuration, if pmp(i + 1)cfg.A is set to TOR,
* writes to pmpaddr(i) are ignored.
*
* Value: 0x0200120d
*
* Solution: check if the lower bound of region is locked of this region when setting address matching mode to TOR.
*/
#define LOS_ERRNO_PMP_TOR_LOWER_BOUND_LOCKED          LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x0d)

/**
* @ingroup los_cpup
* PMP error code: According to the current configuration, check whether the lower bound of TOR region equals to
* the pre-region end address
*
* Value: 0x0200120e
*
* Solution: set the lower bound of TOR i-th region to the pre-region, i.e. (i-1)'th region end address
*/
#define LOS_ERRNO_PMP_TOR_LOWER_BOUND_NONMATCHED      LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0x0e)

/**
* @ingroup los_cpup
* PMP error code: Type extension that given is invalid.
*
* Value: 0x0200120f
*
* Solution: memory sectl configuration only support 00,01,10,11
*/
#define LOS_ERRNO_PMP_SECTL_INVALID                   LOS_ERRNO_OS_ERROR(LOS_MOD_MPU, 0xf)

/*
* @ingroup los_pmp
* @brief pmp region init.
*
* @par Description:
* This API is used to init the pmp Minimum protected address and Maximum protected address.
* @attention
* <ul>
* @param  minAddr [IN] Minimum protected address
* @param  maxAddr [IN] Maximum protected address
* @par Dependency
* <ul><li>los_mpu.h: the header file that contains the API declaration.</li></ul>
* @since Huawei LiteOS V200R002C00B051
 */
UINT32 LOS_PmpInit(UINT64 minAddr, UINT64 maxAddr);

/*
* @ingroup los_pmp
* @brief pmp Sectl region get.
*
* @par Description:
* This API is used to get Sectl region value.
* @attention
* <ul>
* @param  NO
* @par Dependency
* <ul><li>los_pmp.h: the header file that contains the API declaration.</li></ul>
* @since Huawei LiteOS V200R002C00B051
 */
UINT32 LOS_PmpGetSectl(VOID);

/*
 * @ingroup los_pmp
 * @brief pmp region disable.
 *
 * @par Description:
 * This API is used to disable the pmp entry.
 * @attention
 * <ul>
 * <li>the number must be in the range of [0, LOSCFG_PMP_MAX_SUPPORT).</li>
 * <li>.</li>
 * </ul>
 * @param  number     [IN] The pmp entry number
 * @par Dependency:
 * <ul><li>los_mpu.h: the header file that contains the API declaration.</li></ul>
 * @since Huawei LiteOS V200R002C00B051
 */
UINT32 LOS_PmpRegionDisable(UINT8 number);

/**
 * @ingroup los_pmp
 * @brief enable ICache.
 *
 * @par Description:
 * This API is used to enable ICache as 4K 8K 16K or 32K.
 * @attention
 * <ul>
 * <li>The API is just enable ICache as 4K 8K 16K or 32K.</li>
 * <li>.</li>
 * </ul>
 * @par Dependency:
 * <ul><li>los_mpu.h: the header file that contains the API declaration.</li></ul>
 * @since Huawei LiteOS V200R002C00B051
 */
UINT32 LOS_ICacheEnable(CacheSize icclSize);  // default cache size = 32K

/**
 * @ingroup los_pmp
 * @brief enable DCache.
 *
 * @par Description:
 * This API is used to enable DCache as 4K 8K 16K or 32K.
 * @attention
 * <ul>
 * <li>The API is just enable DCache as 4K 8K 16K or 32K.</li>
 * <li>.</li>
 * </ul>
 * @par Dependency:
 * <ul><li>los_mpu.h: the header file that contains the API declaration.</li></ul>
 * @since Huawei LiteOS V200R002C00B051
 */
UINT32 LOS_DCacheEnable(CacheSize dcclSize);  // default cache size = 32K

/**
 * @ingroup los_pmp
 * @brief flush DCache.
 *
 * @par Description:
 * This API is used to flush DCache by Va.
 * @param  baseAddr       [IN] The start address need flush.
 * @param  size           [IN] The size of flush memory.
 * @param  needInvalid    [IN] Only clean the cache or clean and invalid cache.
 * @retval UINT32       return LOS_OK, or return failed.
 * @par Dependency:
 * <ul><li>los_mpu.h: the header file that contains the API declaration.</li></ul>
 * @since Huawei LiteOS V200R002C00B051
 */
LITE_OS_SEC_TEXT UINT32 LOS_FlushDCacheByVa(UINT32 baseAddr, UINT32 size, BOOL needInvalid);

/**
 * @ingroup los_pmp
 * @brief flush ICache.
 *
 * @par Description:
 * This API is used to flush ICache by Va.
 * @param  baseAddr     [IN] The start address need flush.
 * @param  size         [IN] The size of flush memory.
 * @retval UINT32       return LOS_OK, or return failed.
 * @par Dependency:
 * <ul><li>los_mpu.h: the header file that contains the API declaration.</li></ul>
 * @since Huawei LiteOS V200R002C00B051
 */
UINT32 LOS_FlushICacheByVa(UINT32 baseAddr, UINT32 size);

/**
 * @ingroup los_pmp
 * @brief flush ICache.
 *
 * @par Description:
 * This API is used to flush ICache.
 * @attention
 * <ul>
 * <li>The API will flush all ICache, the size is 32K.</li>
 * <li>.</li>
 * </ul>
 * @par Dependency:
 * <ul><li>los_mpu.h: the header file that contains the API declaration.</li></ul>
 * @since Huawei LiteOS V200R002C00B051
 */
VOID LOS_FlushICacheByAll(VOID);

/**
 * @ingroup los_pmp
 * @brief flush DCache.
 *
 * @par Description:
 * This API is used to flush DCache.
 * @attention
 * <ul>
 * <li>The API will flush all DCache, the size is 32K.</li>
 * <li>.</li>
 * </ul>
 * @par Dependency:
 * <ul><li>los_mpu.h: the header file that contains the API declaration.</li></ul>
 * @since Huawei LiteOS V200R002C00B051
 */
VOID LOS_FlushDCacheByAll(VOID);

/**
 * @ingroup los_pmp
 * @brief Obtain the set protection memory region.
 *
 * @par Description:
 * This API is used to set protection memory region.
 * @attention
 * <ul>
 * <li>the base address must be in the range of [LOSCFG_MPU_MIN_ADDRESS, LOSCFG_MPU_MAX_ADDRESS].</li>
 * <li>the input pmpInfo should be filled by using the defined macro in los_pmp.h,
 * the size of pmpInfo is less than or equal to LOSCFG_MPU_MAX_SUPPORT.</li>
 * <li>if the value of the input param pmpInfo->bLocked is 0, the access permission of machine mode will passed.</li>
 * <li>you can setup the region when the region size is greater than CACHE_LINE_SIZE, otherwise,
 * it may produce an UNALIGNED exception.</li>
 * <li>you can setup the region when memory attribute is belong to the enum struct of PmpMemAttrInfo, otherwise,
 * it may produce unpredictable results.</li>
 * <li>.</li>
 * <li>.</li>
 * </ul>
 *
 * @param  pmpInfo          [IN] Set the related configuration information for the protected area
 * @param  pmpInfo->bLocked [IN] lock region in machine mode.
                                 if 1, lock region, validating access permission in machine mode, otherwise, disable it.
 * @retval UINT32           return LOS_OK, or return the index of pstPmpInfo which set up failed.
 * @par Dependency:
 * <ul><li>los_mpu.h: the header file that contains the API declaration.</li></ul>
 * @since Huawei LiteOS V200R002C00B051
 */
UINT32 LOS_ProtectionRegionSet(const PMP_ENTRY_INFO *pmpInfo);

/**
 * @ingroup los_pmp
 * @brief fence memory.
 *
 * @par Description:
 * This API is used to fence memory.
 * @par Dependency:
 * <ul><li>los_mpu.h: the header file that contains the API declaration.</li></ul>
 * @since Huawei LiteOS V200R002C00B051
 */
LITE_OS_SEC_ALW_INLINE STATIC_INLINE VOID Mb(VOID)
{
    __asm__ __volatile__("fence":::"memory");
}

#ifdef LOSCFG_RAM_MONITOR
UINT32 LOS_RamMonitorInit(VOID);
VOID LOS_RamMonitorSet(UINT32 startAddr, UINT32 endAddr);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* LOS_PMP_H */

