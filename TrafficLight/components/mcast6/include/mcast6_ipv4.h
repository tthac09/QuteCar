/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: the APIs of the mcast6 IPv4 support.
 * Author: none
 * Create: 2019-07-03
 */

#ifndef __MCAST6_IPV4_H__
#define __MCAST6_IPV4_H__

#include "mcast6.h"

#ifdef WITH_LWIP
void mcast6_ipv4_in(struct pbuf *p, const struct netif *inp);

void mcast6_ipv4_out(const struct pbuf *p, const ip_addr_t *ip, u16_t port);
#endif

#endif /* __MCAST6_IPV4_H__ */

