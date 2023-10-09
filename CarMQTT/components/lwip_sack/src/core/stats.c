/**
 * @file
 * Statistics module
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "lwip/opt.h"

#if LWIP_STATS /* don't build if not configured for use in lwipopts.h */

#include "lwip/def.h"
#include "lwip/stats.h"
#include "lwip/mem.h"
#include "lwip/debug.h"
#if LWIP_STATS_API
#include "lwip/tcpip.h"
#endif
#include <string.h>

struct stats_ lwip_stats;

void
stats_init(void)
{
#ifdef LWIP_DEBUG
#if MEM_STATS
  lwip_stats.mem.name = "MEM";
#endif /* MEM_STATS */
#endif /* LWIP_DEBUG */
}

#if LWIP_STATS_DISPLAY
void
stats_display_proto(struct stats_proto *proto, const char *name)
{
  (void)proto;
  (void)name;
  LWIP_PLATFORM_DIAG(("\n%s\n\t", name));
  LWIP_PLATFORM_DIAG(("xmit: %"STAT_COUNTER_F"\n\t", proto->xmit));
  LWIP_PLATFORM_DIAG(("recv: %"STAT_COUNTER_F"\n\t", proto->recv));
  LWIP_PLATFORM_DIAG(("fw: %"STAT_COUNTER_F"\n\t", proto->fw));
  LWIP_PLATFORM_DIAG(("drop: %"STAT_COUNTER_F"\n\t", proto->drop));
  LWIP_PLATFORM_DIAG(("chkerr: %"STAT_COUNTER_F"\n\t", proto->chkerr));
  LWIP_PLATFORM_DIAG(("lenerr: %"STAT_COUNTER_F"\n\t", proto->lenerr));
  LWIP_PLATFORM_DIAG(("memerr: %"STAT_COUNTER_F"\n\t", proto->memerr));
  LWIP_PLATFORM_DIAG(("rterr: %"STAT_COUNTER_F"\n\t", proto->rterr));
  LWIP_PLATFORM_DIAG(("proterr: %"STAT_COUNTER_F"\n\t", proto->proterr));
  LWIP_PLATFORM_DIAG(("opterr: %"STAT_COUNTER_F"\n\t", proto->opterr));
  LWIP_PLATFORM_DIAG(("err: %"STAT_COUNTER_F"\n\t", proto->err));
  LWIP_PLATFORM_DIAG(("cachehit: %"STAT_COUNTER_F"\n\t", proto->cachehit));
  LWIP_PLATFORM_DIAG(("cache_miss: %"STAT_COUNTER_F"\n\t", proto->cache_miss));
  LWIP_PLATFORM_DIAG(("cache_full: %"STAT_COUNTER_F"\n", proto->cache_full));

#ifdef LWIP_IPV6
  LWIP_PLATFORM_DIAG(("destunreachs: %"STAT_COUNTER_F"\n", proto->destunreachs));
  LWIP_PLATFORM_DIAG(("timeexcds: %"STAT_COUNTER_F"\n", proto->timeexcds));
  LWIP_PLATFORM_DIAG(("parmprobs: %"STAT_COUNTER_F"\n", proto->parmprobs));
#endif

#if LWIP_NAT64
  LWIP_PLATFORM_DIAG(("natfw: %"STAT_COUNTER_F"\n", proto->natfw));
  LWIP_PLATFORM_DIAG(("natdrop: %"STAT_COUNTER_F"\n", proto->natdrop));
#endif
}

#if IGMP_STATS || MLD6_STATS
void
stats_display_igmp(struct stats_igmp *igmp, const char *name)
{
  (void)igmp;
  (void)name;
  LWIP_PLATFORM_DIAG(("\n%s\n\t", name));
  LWIP_PLATFORM_DIAG(("xmit: %"STAT_COUNTER_F"\n\t", igmp->xmit));
  LWIP_PLATFORM_DIAG(("recv: %"STAT_COUNTER_F"\n\t", igmp->recv));
  LWIP_PLATFORM_DIAG(("drop: %"STAT_COUNTER_F"\n\t", igmp->drop));
  LWIP_PLATFORM_DIAG(("chkerr: %"STAT_COUNTER_F"\n\t", igmp->chkerr));
  LWIP_PLATFORM_DIAG(("lenerr: %"STAT_COUNTER_F"\n\t", igmp->lenerr));
  LWIP_PLATFORM_DIAG(("memerr: %"STAT_COUNTER_F"\n\t", igmp->memerr));
  LWIP_PLATFORM_DIAG(("proterr: %"STAT_COUNTER_F"\n\t", igmp->proterr));
  LWIP_PLATFORM_DIAG(("rx_v1: %"STAT_COUNTER_F"\n\t", igmp->rx_v1));
  LWIP_PLATFORM_DIAG(("rx_group: %"STAT_COUNTER_F"\n\t", igmp->rx_group));
  LWIP_PLATFORM_DIAG(("rx_general: %"STAT_COUNTER_F"\n\t", igmp->rx_general));
  LWIP_PLATFORM_DIAG(("rx_report: %"STAT_COUNTER_F"\n\t", igmp->rx_report));
  LWIP_PLATFORM_DIAG(("tx_join: %"STAT_COUNTER_F"\n\t", igmp->tx_join));
  LWIP_PLATFORM_DIAG(("tx_leave: %"STAT_COUNTER_F"\n\t", igmp->tx_leave));
  LWIP_PLATFORM_DIAG(("tx_report: %"STAT_COUNTER_F"\n", igmp->tx_report));
}
#endif /* IGMP_STATS || MLD6_STATS */

#if LOWPAN6_STATS
void
stats_display_lowpan6(struct stats_lowpan6 *lowpan6, const char *name)
{
  (void)lowpan6;
  (void)name;
  LWIP_PLATFORM_DIAG(("\n%s\n\t", name));
  LWIP_PLATFORM_DIAG(("pkt_from_ip: %"STAT_COUNTER_F"\n\t", lowpan6->pkt_from_ip));
  LWIP_PLATFORM_DIAG(("pkt_xmit: %"STAT_COUNTER_F"\n\t", lowpan6->pkt_xmit));
  LWIP_PLATFORM_DIAG(("xmit_pkt_drop: %"STAT_COUNTER_F"\n\t", (lowpan6->pkt_from_ip - lowpan6->pkt_xmit)));
  LWIP_PLATFORM_DIAG(("comp_fail: %"STAT_COUNTER_F"\n\t", lowpan6->comp_fail));

  LWIP_PLATFORM_DIAG(("pkt_recvd: %"STAT_COUNTER_F"\n\t", lowpan6->pkt_recvd));
  LWIP_PLATFORM_DIAG(("frag_recvd: %"STAT_COUNTER_F"\n\t", lowpan6->frag_recvd));
  LWIP_PLATFORM_DIAG(("pkt_to_ip: %"STAT_COUNTER_F"\n\t", lowpan6->pkt_to_ip));
  LWIP_PLATFORM_DIAG(("recv_pkt_drop: %"STAT_COUNTER_F"\n\t", (lowpan6->pkt_recvd - lowpan6->pkt_to_ip)));

  LWIP_PLATFORM_DIAG(("decomp_fail: %"STAT_COUNTER_F"\n\t", lowpan6->decomp_fail));
  LWIP_PLATFORM_DIAG(("discard_pkt: %"STAT_COUNTER_F"\n\t", lowpan6->discard_pkt));
  LWIP_PLATFORM_DIAG(("reass_tout: %"STAT_COUNTER_F"\n\t", lowpan6->reass_tout));
  LWIP_PLATFORM_DIAG(("dup_frag: %"STAT_COUNTER_F"\n\t", lowpan6->dup_frag));
  LWIP_PLATFORM_DIAG(("oo_frag: %"STAT_COUNTER_F"\n\t", lowpan6->oo_frag));
  LWIP_PLATFORM_DIAG(("rogue_frag: %"STAT_COUNTER_F"\n", lowpan6->rogue_frag));
  LWIP_PLATFORM_DIAG(("corrupt_pkt: %"STAT_COUNTER_F"\n", lowpan6->corrupt_pkt));
}
#endif /* LOWPAN6_STATS */

#if MEM_STATS || MEMP_STATS
void
stats_display_mem(struct stats_mem *mem, const char *name)
{
  (void)mem;
  (void)name;
  LWIP_PLATFORM_DIAG(("\nMEM %s\n\t", name));
  LWIP_PLATFORM_DIAG(("avail: %"U32_F"\n\t", (u32_t)mem->avail));
  LWIP_PLATFORM_DIAG(("used: %"U32_F"\n\t", (u32_t)mem->used));
  LWIP_PLATFORM_DIAG(("max: %"U32_F"\n\t", (u32_t)mem->max));
  LWIP_PLATFORM_DIAG(("err: %"U32_F"\n", (u32_t)mem->err));
}

#if MEMP_STATS
void
stats_display_memp(struct stats_mem *mem, int idx)
{
  if (idx < MEMP_MAX) {
    stats_display_mem(mem, mem->name);
  }
}
#endif /* MEMP_STATS */
#endif /* MEM_STATS || MEMP_STATS */

#if SYS_STATS
void
stats_display_sys(struct stats_sys *sys)
{
  (void)sys;
  LWIP_PLATFORM_DIAG(("\nSYS\n\t"));
  LWIP_PLATFORM_DIAG(("sem.used:  %"U32_F"\n\t", (u32_t)sys->sem.used));
  LWIP_PLATFORM_DIAG(("sem.max:   %"U32_F"\n\t", (u32_t)sys->sem.max));
  LWIP_PLATFORM_DIAG(("sem.err:   %"U32_F"\n\t", (u32_t)sys->sem.err));
  LWIP_PLATFORM_DIAG(("mutex.used: %"U32_F"\n\t", (u32_t)sys->mutex.used));
  LWIP_PLATFORM_DIAG(("mutex.max:  %"U32_F"\n\t", (u32_t)sys->mutex.max));
  LWIP_PLATFORM_DIAG(("mutex.err:  %"U32_F"\n\t", (u32_t)sys->mutex.err));
  LWIP_PLATFORM_DIAG(("mbox.used:  %"U32_F"\n\t", (u32_t)sys->mbox.used));
  LWIP_PLATFORM_DIAG(("mbox.max:   %"U32_F"\n\t", (u32_t)sys->mbox.max));
  LWIP_PLATFORM_DIAG(("mbox.err:   %"U32_F"\n", (u32_t)sys->mbox.err));
}
#endif /* SYS_STATS */

void
stats_display(void)
{
  s16_t i;

  LINK_STATS_DISPLAY();
  ETHARP_STATS_DISPLAY();
  IPFRAG_STATS_DISPLAY();
  IP6_FRAG_STATS_DISPLAY();
  IP_STATS_DISPLAY();
  ND6_STATS_DISPLAY();
  IP6_STATS_DISPLAY();
  DHCP6_STATS_DISPLAY();
  IGMP_STATS_DISPLAY();
  MLD6_STATS_DISPLAY();
  ICMP_STATS_DISPLAY();
  ICMP6_STATS_DISPLAY();
  UDP_STATS_DISPLAY();
  TCP_STATS_DISPLAY();
  LOWPAN6_STATS_DISPLAY();
  MEM_STATS_DISPLAY();
  for (i = 0; i < MEMP_MAX; i++) {
    MEMP_STATS_DISPLAY(i);
  }
  SYS_STATS_DISPLAY();
}
#endif /* LWIP_STATS_DISPLAY */

#if LWIP_STATS_API
#define VALIDATE_STACK_STATE \
  if(tcpip_init_finish == 0) { \
    return -1; \
  }

#define VALIDATE_PARAM(para) \
  if (para == NULL) { \
    return ERR_ARG; \
  }

err_t
lwip_statsapi_get_ipv6_stats(struct stats_proto *ipv6)
{
  VALIDATE_STACK_STATE;
  VALIDATE_PARAM(ipv6);

#if IP6_STATS
  (void)memcpy_s(ipv6, sizeof(struct stats_proto), &lwip_stats.ip6,
                 sizeof(struct  stats_proto));
#endif

  return ERR_OK;
}

err_t
lwip_statsapi_get_icmpv6_stats(struct stats_proto *icmpv6)
{
  VALIDATE_STACK_STATE;
  VALIDATE_PARAM(icmpv6);

#if ICMP6_STATS
  (void)memcpy_s(icmpv6, sizeof(struct stats_proto), &lwip_stats.icmp6,
                 sizeof(struct  stats_proto));

  /* Since Incase of ND6/MLD6, stats are not increased by handling function, Need to add manually. */
#if ND6_STATS
  icmpv6->xmit += lwip_stats.nd6.xmit;
#endif
#if LWIP_IPV6_MLD
  icmpv6->xmit += lwip_stats.mld6.xmit;
#endif
#endif

  return ERR_OK;
}

err_t
lwip_statsapi_get_nd6_stats(struct stats_proto *nd6)
{
  VALIDATE_STACK_STATE;
  VALIDATE_PARAM(nd6);

#if ND6_STATS
  (void)memcpy_s(nd6, sizeof(struct stats_proto), &lwip_stats.nd6,
                 sizeof(struct  stats_proto));
#endif

  return ERR_OK;
}

err_t
lwip_statsapi_get_tcp_stats(struct stats_proto *tcp)
{
  VALIDATE_STACK_STATE;
  VALIDATE_PARAM(tcp);

#if TCP_STATS
  (void)memcpy_s(tcp, sizeof(struct stats_proto), &lwip_stats.tcp,
                 sizeof(struct  stats_proto));
#endif

  return ERR_OK;
}

err_t
lwip_statsapi_get_udp_stats(struct stats_proto *udp)
{
  VALIDATE_STACK_STATE;
  VALIDATE_PARAM(udp);

#if UDP_STATS
  (void)memcpy_s(udp, sizeof(struct stats_proto), &lwip_stats.udp,
                 sizeof(struct  stats_proto));
#endif

  return ERR_OK;
}

err_t
lwip_statsapi_get_lowpan6_stats(struct stats_lowpan6 *lowpan6)
{
  VALIDATE_STACK_STATE;
  VALIDATE_PARAM(lowpan6);

#if LOWPAN6_STATS
  (void)memset_s(lowpan6, sizeof(struct stats_lowpan6), &lwip_stats.lowpan6,
                 sizeof(struct stats_lowpan6));
#endif

  return ERR_OK;
}
err_t
lwip_statsapi_clear_lowpan6_stats(void)
{
  VALIDATE_STACK_STATE;

#if LOWPAN6_STATS
  (void)memcpy_s(&lwip_stats.lowpan6, sizeof(struct stats_lowpan6), 0,
                 sizeof(struct stats_lowpan6));
#endif

  return ERR_OK;
}
#endif

#endif /* LWIP_STATS */

