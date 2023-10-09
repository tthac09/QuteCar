/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: the APIs of the mcast6 to handle Multicast Route Table.
 * Author: none
 * Create: 2019-07-03
 */

#ifndef __MCAST6_TABLE_H__
#define __MCAST6_TABLE_H__

#include "mcast6.h"

typedef struct mcast6_table {
  struct mcast6_table *next;
  mcast6_addr_t addr;
  uint32_t lifetime;
} mcast6_table_t;

mcast6_table_t *mcast6_get_table_list(void);
uint8_t mcast6_table_lookup(const mcast6_addr_t *addr);
uint16_t mcast6_table_entry_cnt(void);
void mcast6_table_clear(void);

#endif /* __MCAST6_TABLE_H__ */

