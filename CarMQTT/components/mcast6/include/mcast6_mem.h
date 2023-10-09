/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: the memory APIs of the mcast6.
 * Author: none
 * Create: 2019-07-03
 */

#ifndef __MCAST6_MEM_H__
#define __MCAST6_MEM_H__

#include <stdlib.h>
#include <lwip/mem.h>

static inline void *
mcast6_malloc(size_t size)
{
  return mem_malloc((mem_size_t)size);
}

static inline void
mcast6_free(void **mem)
{
  if ((mem == NULL) || (*mem == NULL)) {
    return;
  }
  mem_free(*mem);
  *mem = NULL;
  return;
}

#endif /* __MCAST6_MEM_H__ */

