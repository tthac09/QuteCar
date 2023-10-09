/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: the APIs of the mcast6 which will be called by rpl.
 * Author: none
 * Create: 2019-07-03
 */

#ifndef __MCAST6_RPL_H__
#define __MCAST6_RPL_H__

void mcast6_dao_handle(const uint8_t *buf, uint16_t buflen, uint16_t lifetimeUnit);
void mcast6_tmr(uint8_t periodSec);
uint16_t mcast6_targets_fill(uint8_t *buf, uint16_t buflen, uint8_t *tgtcnt);
uint8_t mcast6_init(void);
void mcast6_deinit(void);

#endif /* __MCAST6_RPL_H__ */

