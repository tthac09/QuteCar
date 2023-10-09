/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL DAG and Instance management
 * Author: NA
 * Create: 2019-04-02
 */

#ifndef _TRICKLE_H_
#define _TRICKLE_H_

#include "rpl_platform_api.h"

typedef void (*rpl_trickle_cb)(void *usp);

typedef struct {
  void *usp;          /* opague pointer */
  rpl_timer_t timer;  /* actual timer */
  rpl_trickle_cb cb;  /* protocol callback */
  uint32_t imin;      /* Min interval */
  uint32_t t;         /* time within current interval in ms */
  uint32_t i;         /* current interval size in ms */
  uint8_t k;          /* redundancy constant */
  uint8_t c;          /* consistency counter */
  uint8_t imax;       /* max number of doublings */
} rpl_trickle_timer_t;

/**
 * @Description: Start the already configured trickle timer
 *
 * @param tt - Trickle timer struct configured using TrickleConfig
 * @param cb - Trickle timer callback
 * @param usp - opague value given out in callback
 * @param cfg - RPL config
 *
 * @return OK/FAIL
 */
uint8_t rpl_trickle_start(rpl_trickle_timer_t *tt, rpl_trickle_cb cb, void *usp, const rpl_cfg_t *cfg);

/**
 * @Description: Stop Trickle timer
 *
 * @param tt - Trickle timer struct
 */
void rpl_trickle_stop(rpl_trickle_timer_t *tt);

/**
 * @Description: Trickle holder should call this whenever it detects a
 * consistent state message in the air.
 *
 * @param tt - Trickle timer
 */
void rpl_trickle_increment(rpl_trickle_timer_t *tt);

/**
 * @Description: Reset trickle. Usually called when trickle holder detects
 * inconsistent state.
 *
 * @param tt - Trickle timer handler
 */
void rpl_trickle_reset(rpl_trickle_timer_t *tt);

#endif /* _TRICKLE_H_ */
