/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: the main APIs of the mcast6.
 * Author: none
 * Create: 2019-07-03
 */

#ifndef __MCAST6_H__
#define __MCAST6_H__

#include <stdint.h>
#include <mcast6_conf.h>
#include <lwip/pbuf.h>
#include <lwip/ip.h>

typedef union {
  uint8_t  a8[MCAST6_ADDR_U8_NUM];
  uint16_t a16[MCAST6_ADDR_U16_NUM];
  uint32_t a32[MCAST6_ADDR_U32_NUM];
} mcast6_addr_t;

#if MCAST6_CONF_BIG_ENDIAN
#define MCAST6_HTONS(x) (x)
#define MCAST6_NTOHS(x) (x)
#define MCAST6_HTONL(x) (x)
#define MCAST6_NTOHL(x) (x)
#else /* MCAST6_CONF_BIG_ENDIAN == 0 */
#define MCAST6_HTONS(x) ((((x) & 0x00ff) << 8) | \
  (((x) & 0xff00) >> 8))
#define MCAST6_NTOHS(x) MCAST6_HTONS(x)
#define MCAST6_HTONL(x) ((((x) & 0x000000ffUL) << 24) | \
  (((x) & 0x0000ff00UL) << 8) |                         \
  (((x) & 0x00ff0000UL) >> 8) |                         \
  (((x) & 0xff000000UL) >> 24))
#define MCAST6_NTOHL(x) MCAST6_HTONL(x)
#endif /* MCAST6_CONF_BIG_ENDIAN */

#define MCAST6_ISANY_VAL(A) \
  (((A).a32[0] == 0) &&     \
  ((A).a32[1] == 0) &&      \
  ((A).a32[2] == 0) &&      \
  ((A).a32[3] == 0))

#define MCAST6_IS_MCAST(A) ((A)->a8[0] == 0xff)
#define MCAST6_IS_LINKLOCAL(A) (((A)->a16[0] & MCAST6_HTONL(0xffc0)) == MCAST6_HTONL(0xfe80))

#define MCAST6_SCOPE(A) (((A)->a8[1]) & 0xf)
#define MCAST6_SCOPE_RESERVED0           0x0
#define MCAST6_SCOPE_INTERFACE_LOCAL     0x1
#define MCAST6_SCOPE_LINK_LOCAL          0x2
#define MCAST6_SCOPE_REALM_LOCAL         0x3
#define MCAST6_SCOPE_ADMIN_LOCAL         0x4
#define MCAST6_SCOPE_SITE_LOCAL          0x5
#define MCAST6_SCOPE_ORGANIZATION_LOCAL  0x8
#define MCAST6_SCOPE_GLOBAL              0xe
#define MCAST6_SCOPE_RESERVEDF           0xf

#define MCAST6_MATCH(A, B)                                            \
  (((A)->a32[0] == (B)->a32[0]) &&                                    \
  ((A)->a32[1] == (B)->a32[1]) &&                                     \
  ((((A)->a32[2] == (B)->a32[2]) && ((A)->a32[3] == (B)->a32[3])) ||  \
  (((B)->a32[2] == 0) && ((B)->a32[3] == 0))))

#define MCAST6_NIPQUAD_FMT "****:%02x%02x:%02x%02x:%02x%02x:%02x%02x"
#define MCAST6_NIPQUAD(IP)                                                                   \
  (uint8_t)(IP)->a8[8], (uint8_t)(IP)->a8[9], (uint8_t)(IP)->a8[10], (uint8_t)(IP)->a8[11],  \
  (uint8_t)(IP)->a8[12], (uint8_t)(IP)->a8[13], (uint8_t)(IP)->a8[14], (uint8_t)(IP)->a8[15]

#define MCAST6_IP4_NIPQUAD_FMT "*.*.%02x.%02x"
#define MCAST6_IP4_NIPQUAD(IP) (uint8_t)(((IP) & 0x0000ff00UL) >> 8), (uint8_t)((IP) & 0x000000ffUL)

void mcast6_esmrf_in(struct pbuf *p, const struct ip6_hdr *iphdr);
void mcast6_ipv4_in(struct pbuf *p, const struct netif *inp);
void mcast6_esmrf_icmp_out(const struct pbuf *p, const ip_addr_t *ip, u16_t port);
void mcast6_ipv4_out(const struct pbuf *p, const ip_addr_t *ip, u16_t port);
void mcast6_esmrf_icmp_in(struct pbuf *p, uint8_t *sendaddr);
uint8_t mcast6_esmrf_from_conn_peer(const struct pbuf *p);
void lwip_mld6_join_group(const ip6_addr_t *groupaddr);
void mcast6_deinit(void);
uint8_t mcast6_init(void);

#endif /* __MCAST6_H__ */

