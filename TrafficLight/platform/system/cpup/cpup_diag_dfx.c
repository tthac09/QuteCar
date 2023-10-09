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
#include <hi_mem.h>
#include <hi_stdlib.h>
#include <hi_cpu.h>
#include <hi_task.h>
#include <hi_at.h>

#ifdef LOSCFG_BASE_CORE_CPUP
#define CPUP_DIAG_REPORT_CNT        30
#define PRINT_LINE_SIZE             100
#define CPU_TIME_HIGH_BIT           32
#define DIAG_GET_CPUP_RESULT_LEN    320

void cmd_get_cpup(int argc, unsigned char* argv[])
{
    hi_unref_param(argv);
    hi_u32 ret;
    hi_char printline[PRINT_LINE_SIZE] = {0};
    hi_cpup_item* p_cpup_items = HI_NULL;

    if (argc == 1) {
        hi_cpup_reset_usage();
    }

    p_cpup_items = hi_malloc(HI_MOD_ID_CPUP_DFX, sizeof(hi_cpup_item) * CPUP_DIAG_REPORT_CNT);
    if (p_cpup_items == HI_NULL) {
        goto end;
    }

    ret = hi_cpup_get_usage(CPUP_DIAG_REPORT_CNT, p_cpup_items);
    if (ret != HI_ERR_SUCCESS) {
        printf("get cpup fail\r\n");
        goto end;
    }

    for (hi_u32 i = 0; i < CPUP_DIAG_REPORT_CNT; i++) {
        hi_cpup_item* p_item = &p_cpup_items[i];
        hi_task_info info = {0};
        if (p_item->b_valid == HI_FALSE) {
            continue;
        }
        hi_u32 low_time = (hi_u32)p_item->cpu_time;
        hi_u32 high_time = (hi_u32)(p_item->cpu_time >> CPU_TIME_HIGH_BIT);
        if (p_item->b_task) {
            ret = hi_task_get_info(p_item->id, &info);
            if (ret != HI_ERR_SUCCESS) {
                continue;
            }
            hi_s32 ret_val = snprintf_s(printline, sizeof(printline), sizeof(printline) - 1,
                                        "[%s][permillage=%d][cpu_time=0x%08x%08x]",
                                        info.name, p_item->permillage, high_time, low_time);
            if (ret_val > 0) {
                hi_at_printf("%s\r\n", printline);
            }
        } else {
            snprintf_s(printline, sizeof(printline), sizeof(printline) - 1,
                "[isr%d][permillage=%d][cpu_time=0x%08x%08x]", p_item->id, p_item->permillage, high_time, low_time);
            hi_at_printf("%s\r\n", printline);
        }
    }

end:
    if (p_cpup_items != HI_NULL) {
        hi_free(HI_MOD_ID_CPUP_DFX, p_cpup_items);
    }
    return;
}
#else
void cmd_get_cpup(int argc, unsigned char* argv[])
{
    hi_unref_param(argc);
    hi_unref_param(argv);
}
#endif


void at_cmd_get_cpup(int argc, unsigned char* argv[])
{
    cmd_get_cpup(argc, argv);
}

