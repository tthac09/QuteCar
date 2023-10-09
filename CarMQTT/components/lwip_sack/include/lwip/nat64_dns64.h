/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: DNS64 header
 * Author: NA
 * Create: 2019
 */
#ifndef LWIP_HDR_NAT64_DNS64_H
#define LWIP_HDR_NAT64_DNS64_H

#include "lwip/lwip_rpl.h"

#if LWIP_DNS64

#define NAT64_DNS_PORT 53

err_t nat64_dns64_6to4(struct pbuf *p);
err_t nat64_dns64_4to6(const struct pbuf *p4, struct pbuf *p6);
err_t nat64_dns64_extra_count(struct pbuf *p, u16_t *count);
err_t nat64_dns64_get_prefix(ip6_addr_t *ip6addr, u8_t *len);
#endif
#endif
