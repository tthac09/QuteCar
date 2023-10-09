/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL notify header
 * Author: NA
 * Create: 2019
 */

#ifndef _RPL_NOTIFY_H_
#define _RPL_NOTIFY_H_

#include "ripple.h"

typedef void (*rpl_notify_cb)(uint8_t type);

typedef struct {
  rpl_notify_cb dao_loop;
  rpl_notify_cb route_full;
  rpl_notify_cb ip_conflict;
} rpl_notify_ops;

int rpl_notify_register(uint16_t id, rpl_notify_ops *ops);

int rpl_notify_unregister(uint16_t id);

uint8_t rpl_notify_init(void);

uint8_t rpl_notify_deinit(void);

#endif

