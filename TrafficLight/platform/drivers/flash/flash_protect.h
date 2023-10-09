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

#ifndef _FLASH_PROTECT_H_
#define _FLASH_PROTECT_H_

#include <hi_types_base.h>
#include <hi_isr.h>
#include <hi_mux.h>

#define UVLO_GRM_INT_EN  7
#ifndef HI3861_32K_DEFAULT_FREQ
#define HI3861_32K_DEFAULT_FREQ 32000
#endif

#define SOFT_RST_GLB_PILSE_MS   4
#define MS_PER_S   1000
#define SOFT_RST_GLB_POS 5
#define UVLO_GRM_LSICK_CLR 7
#define KERNEL_HEAD  0x3c0
#define S19_IDX  3   /* for wb */

/* wb flash reg1 need check: srp sec tb bp2 bp1 bp0 , high 6 bits */
#define WB_REG1_CHECK_MSK 0xFC
/* wb flash reg2 need check: cmp lb3 lb2 lb1 qe srl */
#define WB_REG2_CHECK_MSK 0x7B
#define MAX_CHECK_REG_TIMES 3
/* wb flash reg3 need check: hold drv1 drv0 s19 wps */
#define WB_CHECK_REG3_MSK 0xEC
#define FIRST_DELAY_US  10000
#define NEXT_DELAY_US   1000000
#define WB_REG3_HOLD_OFFSET 7
#define WB_REG3_S19_OFFSET 3
#define WB_REG3_WPS_OFFSET 2
#define WB_REG2_SRL_OFFSET 0
#define WB_REG1_SRP_OFFSET 7
#define READ_REG_DELAY_10US 10
#define READ_REG_DELAY_20US 20

#define FLASH_PROTECT_NEED_CHK  0x55
#define FLASH_PROTECT_NOT_CHK   0xAA

extern hi_u32 __text_rodata_end_;
extern hi_u32 __text_cache_start2_;

extern hi_u32 __ram_text_load;
extern hi_u32 __ram_text_size;

extern hi_u32 __data_load;
extern hi_u32 __data_size;

#if defined(CONFIG_FLASH_ENCRYPT_SUPPORT)
extern hi_u32 __crypto_ram_text_load;
extern hi_u32 __crypto_ram_text_size;
#endif

extern hi_void disable_int_in_flash(hi_void);
extern hi_void enable_int_in_flash(hi_void);
extern hi_u32 hi_get_lowspeed_clk_freq(hi_u32 *freq);
hi_void flash_wb_erase_recovery(const hi_u8 *chip_id);
#endif