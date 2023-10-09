/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2015. All rights reserved.
 * Description: perf APIs
 * Author: none
 * Create: 2013
 */

#include "arch/perf.h"

#if LWIP_PERF

void
perf_print(u32_t start_ms, u32_t end_ms, char *key)
{
  LWIP_PLATFORM_DIAG(("PERF, %s, %llu, %llu\n", key, start_ms, end_ms));
}

#endif

