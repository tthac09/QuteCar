/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: Platform specific data structures
 * Author: NA
 * Create: 2019-04-02
 */

#ifndef _RPL_PLATFORM_H_
#define _RPL_PLATFORM_H_

typedef void (*rpl_timeout_cb)(void *);

typedef struct rpl_timer_s {
  rpl_timeout_cb tofunc;
  void *arg;
  uint32_t duration;
  uint8_t isactive : 1;
  uint8_t isperiodic : 1;
} rpl_timer_t;

#endif /* _RPL_PLATFORM_H_ */
