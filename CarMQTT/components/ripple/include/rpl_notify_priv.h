/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: RPL notify tigger header
 * Author: NA
 * Create: 2019
 */

#include "rpl_notify.h"

#ifndef _RPL_NOTIFY_INNER_H_
#define _RPL_NOTIFY_INNER_H_

enum rpl_notify_event {
  RPL_NOTIFY_DAO_LOOP,
  RPL_NOTIFY_ROUTE_FULL,
  RPL_NOTIFY_IP_CONFLICT,
  RPL_NOTIFY_MAX
};

void rpl_notify_trigger(enum rpl_notify_event event, uint8_t type);

#endif
