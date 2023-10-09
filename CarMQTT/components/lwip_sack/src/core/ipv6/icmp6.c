/**
 * @file
 *
 * IPv6 version of ICMP, as per RFC 4443.
 */

/*
 * Copyright (c) 2010 Inico Technologies Ltd.
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
 * Author: Ivan Delamer <delamer@inicotech.com>
 *
 *
 * Please coordinate changes and requests with Ivan Delamer
 * <delamer@inicotech.com>
 */

#include "lwip/opt.h"

#if LWIP_ICMP6 && LWIP_IPV6 /* don't build if not configured for use in lwipopts.h */

#include "lwip/icmp6.h"
#include "lwip/prot/icmp6.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/inet_chksum.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/nd6.h"
#include "lwip/mld6.h"
#include "lwip/ip.h"
#include "lwip/stats.h"

#include <string.h>

#if LWIP_RPL || LWIP_RIPPLE
#include "lwip/lwip_rpl.h"
#endif

#if LWIP_MPL
#include "mcast6.h"

#ifndef MCAST6_MPL_IN
#define MCAST6_MPL_IN mcast6_esmrf_icmp_in
#endif /* MCAST6_MPL_IN */
#endif /* LWIP_MPL */

#ifndef LWIP_ICMP6_DATASIZE
#define LWIP_ICMP6_DATASIZE   8
#endif
#if LWIP_ICMP6_DATASIZE == 0
#define LWIP_ICMP6_DATASIZE   8
#endif

#if LWIP_ICMP6_ERR_RT_LMT
/* icmp6 rate limit queue */
struct icmp6_err_bkt icmp6_err_bkt;
#endif


/* Forward declarations */
static void icmp6_send_response(struct pbuf *p, u8_t code, u32_t data, u8_t type);

/**
@page RFC-4443 RFC-4443
@par RFC-4443 Compliance Information
@par Compliant Section
Section 2.1.  Message General Format
@par Behavior Description
ICMPv6 is used by IPv6 nodes to report errors encountered in processing packets, and to
 perform other internet-layer functions, such as diagnostics (ICMPv6 "ping").  ICMPv6 is an
 integral part of IPv6, and the base protocol (all the messages and behavior required by this
 specification) MUST be fully implemented by every IPv6 node.\n
 @verbatim
 ICMPv6 error messages:
          1    Destination Unreachable
          2    Packet Too Big
          3    Time Exceeded
          4    Parameter Problem
  @endverbatim
  @verbatim
 ICMPv6 informational messages:
          128  Echo Request
          129  Echo Reply
 @endverbatim
  Behavior:  Our node conforms to RFC-4443
*/
/*
 * Process an input ICMPv6 message. Called by ip6_input.
 *
 * Will generate a reply for echo requests. Other messages are forwarded
 * to nd6_input, or mld6_input.
 *
 * @param p the mld packet, p->payload pointing to the icmpv6 header
 * @param inp the netif on which this packet was received
 */
void
icmp6_input(struct pbuf *p, struct netif *inp)
{
  struct icmpv6_hdr *icmp6hdr = NULL;
  struct pbuf *r = NULL;
  const ip6_addr_t *reply_src = NULL;

  ICMP6_STATS_INC(icmp6.recv);

  /*
   * Check that ICMPv6 header fits in payload
   * Will remove this check
   * if (p->len < sizeof(struct icmpv6_hdr))
   * as struct icmpv6_hdr size is 8 bytes. It assumes a 4 bytes code always.
   * We will check only for ICMP_HEADER size
   */
  if (p->len < ICMP6_HLEN) {
    /* drop short packets */
    LWIP_DEBUGF(ICMP_DEBUG, ("Drop the packet Invalid length [%d][%d] \n",
                             p->len, ICMP6_HLEN));
    (void)pbuf_free(p);
    ICMP6_STATS_INC(icmp6.lenerr);
    ICMP6_STATS_INC(icmp6.drop);
    return;
  }

  icmp6hdr = (struct icmpv6_hdr *)p->payload;
#if LWIP_RIPPLE && defined(LWIP_NA_PROXY) && LWIP_NA_PROXY
  if ((p->na_proxy == lwIP_TRUE) && (icmp6hdr->type != ICMP6_TYPE_NS)) {
    (void)pbuf_free(p);
    return;
  }
#endif
#if CHECKSUM_CHECK_ICMP6
  IF__NETIF_CHECKSUM_ENABLED(inp, NETIF_CHECKSUM_CHECK_ICMP6) {
    if (ip6_chksum_pseudo(p, IP6_NEXTH_ICMP6, p->tot_len, ip6_current_src_addr(),
                          ip6_current_dest_addr()) != 0) {
      /* Checksum failed */
      (void)pbuf_free(p);
      ICMP6_STATS_INC(icmp6.chkerr);
      ICMP6_STATS_INC(icmp6.drop);
      return;
    }
  }
#endif /* CHECKSUM_CHECK_ICMP6 */

#if LWIP_MAC_SECURITY
  /*
   * If the raw socket pcb will accept only secure packets then we will
   * drop the packet reeived without mac layer encryption
   */
  if (((inp->flags & NETIF_FLAG_MAC_SECURITY_SUPPORT) != 0) &&
      (((p->flags & PBUF_FLAG_WITH_ENCRYPTION)) == 0) &&
      (((inp->is_auth_sucess == 0) && (icmp6hdr->type != ICMP6_TYPE_NS) && (icmp6hdr->type != ICMP6_TYPE_NA) &&
      ((icmp6hdr->type == ICMP6_TYPE_EREQ) && !ip6_addr_islinklocal(ip6_current_dest_addr()))) ||
      ((inp->is_auth_sucess != 0) && (icmp6hdr->type != ICMP6_TYPE_NS)))) {
    LWIP_DEBUGF(IP6_DEBUG, ("Drop the packet as its not secure\r\n"));
    ICMP6_STATS_INC(icmp6.drop);
    (void)pbuf_free(p);
    LWIP_DEBUGF(IP6_DEBUG, ("%s:%d is_auth_sucess:%d \r\nLWIP_MAC_SECURITY error\r\n",
                            __FUNCTION__, __LINE__, inp->is_auth_sucess));
    return;
  }
#endif

  switch (icmp6hdr->type) {
    case ICMP6_TYPE_NA: /* Neighbor advertisement */
    case ICMP6_TYPE_NS: /* Neighbor solicitation */
    case ICMP6_TYPE_RA: /* Router advertisement */
    case ICMP6_TYPE_RD: /* Redirect */
    case ICMP6_TYPE_PTB: /* Packet too big */
      nd6_input(p, inp);
      return;
    case ICMP6_TYPE_RS:
#if LWIP_ND6_ROUTER
#if (LWIP_RPL || LWIP_RIPPLE) && LWIP_RPL_RS_DAO
      /*
       * just send DAO message
       * when the node is mesh gate
       */
      if (lwip_rpl_is_router()) {
        nd6_rs_input(p, inp, ND6_RS_FLAG_DAO);
        return;
      }
#endif
      nd6_rs_input(p, inp, ND6_RS_FLAG_RA);
#endif
      return;
#if (LWIP_IPV6_MLD || LWIP_IPV6_MLD_QUERIER)
    case ICMP6_TYPE_MLQ:
    case ICMP6_TYPE_MLR:
    case ICMP6_TYPE_MLD:
      mld6_input(p, inp);
      return;
#endif
    case ICMP6_TYPE_EREQ:
#if !LWIP_MULTICAST_PING
      /* multicast destination address? */
      if (ip6_addr_ismulticast(ip6_current_dest_addr())) {
        /* drop */
        (void)pbuf_free(p);
        ICMP6_STATS_INC(icmp6.drop);
        return;
      }
#endif /* LWIP_MULTICAST_PING */

      /* Allocate reply. */
      r = pbuf_alloc(PBUF_IP, p->tot_len, PBUF_RAM);
      if (r == NULL) {
        /* drop */
        (void)pbuf_free(p);
        ICMP6_STATS_INC(icmp6.memerr);
        return;
      }

      /* Copy echo request. */
      if (pbuf_copy(r, p) != ERR_OK) {
        /* drop */
        (void)pbuf_free(p);
        (void)pbuf_free(r);
        ICMP6_STATS_INC(icmp6.err);
        return;
      }

      /* Determine reply source IPv6 address. */
#if LWIP_MULTICAST_PING
      if (ip6_addr_ismulticast(ip6_current_dest_addr())) {
        reply_src = ip_2_ip6(ip6_select_source_address(inp, ip6_current_src_addr()));
        if (reply_src == NULL) {
          /* drop */
          (void)pbuf_free(p);
          (void)pbuf_free(r);
          ICMP6_STATS_INC(icmp6.rterr);
          return;
        }
      } else
#endif /* LWIP_MULTICAST_PING */
      {
        reply_src = ip6_current_dest_addr();
      }

      /* Set fields in reply. */
      ((struct icmp6_echo_hdr *)(r->payload))->type = ICMP6_TYPE_EREP;
      ((struct icmp6_echo_hdr *)(r->payload))->chksum = 0;
#if CHECKSUM_GEN_ICMP6
      IF__NETIF_CHECKSUM_ENABLED(inp, NETIF_CHECKSUM_GEN_ICMP6) {
        ((struct icmp6_echo_hdr *)(r->payload))->chksum = ip6_chksum_pseudo(r,
                                                                            IP6_NEXTH_ICMP6,
                                                                            r->tot_len,
                                                                            reply_src,
                                                                            ip6_current_src_addr());
      }
#endif /* CHECKSUM_GEN_ICMP6 */

      /* Send reply. */
      ICMP6_STATS_INC(icmp6.xmit);
#if LWIP_MAC_SECURITY
      if ((p->flags & PBUF_FLAG_WITH_ENCRYPTION) != 0) {
        r->flags |= PBUF_FLAG_WITH_ENCRYPTION;
      }
#endif

      (void)ip6_output_if(r, reply_src, ip6_current_src_addr(),
                          LWIP_ICMP6_HL, 0, IP6_NEXTH_ICMP6, inp);
      (void)pbuf_free(r);

      break;

#if LWIP_RPL || LWIP_RIPPLE
    case ICMP6_TYPE_RPL:
      lwip_handle_rpl_ctrl_msg(icmp6hdr, p, ip6_current_src_addr(), ip6_current_dest_addr(), inp);

      break;
#endif /* LWIP_RPL || LWIP_RIPPLE */
    case ICMP6_TYPE_EREP:
      ICMP6_STATS_INC(icmp6.drop);
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp6_input: ICMPv6 type %"S16_F" code %"S16_F" is dropped.\n",
                               (s16_t)icmp6hdr->type, (s16_t)icmp6hdr->code));
      break;

#if LWIP_STATS
    case ICMP6_TYPE_DUR:
      MIB2_STATS_INC(mib2.icmpindestunreachs);
      ICMP6_STATS_INC(icmp6.destunreachs);
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp6_input: ICMPv6 type %"S16_F" code %"S16_F" not supported.\n",
                               (s16_t)icmp6hdr->type, (s16_t)icmp6hdr->code));

      ICMP6_STATS_INC(icmp6.drop);
      break;

    case ICMP6_TYPE_TE:
      MIB2_STATS_INC(mib2.icmpintimeexcds);
      ICMP6_STATS_INC(icmp6.timeexcds);
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp6_input: ICMPv6 type %"S16_F" code %"S16_F" not supported.\n",
                               (s16_t)icmp6hdr->type, (s16_t)icmp6hdr->code));

      ICMP6_STATS_INC(icmp6.drop);
      break;

    case ICMP6_TYPE_PP:
      MIB2_STATS_INC(mib2.icmpinparmprobs);
      ICMP6_STATS_INC(icmp6.parmprobs);
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp6_input: ICMPv6 type %"S16_F" code %"S16_F" not supported.\n",
                               (s16_t)icmp6hdr->type, (s16_t)icmp6hdr->code));

      ICMP6_STATS_INC(icmp6.drop);
      break;
#endif

#if LWIP_MPL
    case ICMP6_TYPE_ESMRF:
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp6_input: ICMP6_TYPE_ESMRF\n"));
      MCAST6_MPL_IN(p, (u8_t *)(ip6_current_src_addr()));
      break;
#endif /* LWIP_MPL */

    default:
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp6_input: ICMPv6 type %"S16_F" code %"S16_F" not supported.\n",
                               (s16_t)icmp6hdr->type, (s16_t)icmp6hdr->code));

      ICMP6_STATS_INC(icmp6.proterr);
      ICMP6_STATS_INC(icmp6.drop);
      break;
  }

  (void)pbuf_free(p);
}

/**
 * @page RFC-4443 RFC-4443
 * @par Non-Compliant Section
 * Section 3. ICMPv6 Error Messages
 * Upper Layer Notification:
 * A node receiving the ICMPv6 Destination Unreachable message MUST notify the upper-layer process\n
 * if the relevant process can be identified.
 * @par Behavior Description
 * Upper Layer Notification is not supported
*/
/*
 * Send an icmpv6 'destination unreachable' packet.
 *
 * @param p the input packet for which the 'unreachable' should be sent,
 *          p->payload pointing to the IPv6 header
 * @param c ICMPv6 code for the unreachable type
 */
void
icmp6_dest_unreach(struct pbuf *p, enum icmp6_dur_code c)
{
  icmp6_send_response(p, c, 0, ICMP6_TYPE_DUR);
}

/**
 * Send an icmpv6 'packet too big' packet.
 *
 * @param p the input packet for which the 'packet too big' should be sent,
 *          p->payload pointing to the IPv6 header
 * @param mtu the maximum mtu that we can accept
 */
void
icmp6_packet_too_big(struct pbuf *p, u32_t mtu)
{
  icmp6_send_response(p, 0, mtu, ICMP6_TYPE_PTB);
}

/**
 * Send an icmpv6 'time exceeded' packet.
 *
 * @param p the input packet for which the 'unreachable' should be sent,
 *          p->payload pointing to the IPv6 header
 * @param c ICMPv6 code for the time exceeded type
 */
void
icmp6_time_exceeded(struct pbuf *p, enum icmp6_te_code c)
{
  icmp6_send_response(p, c, 0, ICMP6_TYPE_TE);
}

/**
 * Send an icmpv6 'parameter problem' packet.
 *
 * @param p the input packet for which the 'param problem' should be sent,
 *          p->payload pointing to the IP header
 * @param c ICMPv6 code for the param problem type
 * @param pointer the pointer to the byte where the parameter is found
 */
void
icmp6_param_problem(struct pbuf *p, enum icmp6_pp_code c, u32_t pointer)
{
  icmp6_send_response(p, c, pointer, ICMP6_TYPE_PP);
}


#if LWIP_ICMP6_ERR_RT_LMT

/* Invoked when the error message is receieved */
u8_t
icmp6_err_update_rate_lmt()
{
  struct icmp6_err_bkt *p = &icmp6_err_bkt;
  u32_t avg = 0;
  u16_t i;

  p->icmp_bkt[p->bkt_index]++;
  for (i = 0; i < ICMP6_ERR_BKT_SIZE; i++) {
    avg += p->icmp_bkt[i];
  }

  /* calculate the running average and compare with the threshold */
  p->avg = avg / ICMP6_ERR_BKT_SIZE;

  /* drop the packet */
  return (p->avg >= (u32_t)ICMP6_ERR_THRESHOLD_LEVEL);
}


/* invoked through nd6 timer. No need to create one more timer for this */
void
icmp6_err_rate_calc(void)
{
  struct icmp6_err_bkt *p = &icmp6_err_bkt;

  p->bkt_index++;
  if (p->bkt_index >= ICMP6_ERR_BKT_SIZE) {
    p->bkt_index = 0;
  }

  p->icmp_bkt[p->bkt_index] = 0;

  return ;
}
#endif

/**
 * Send an ICMPv6 packet in response to an incoming packet.
 *
 * @param p the input packet for which the response should be sent,
 *          p->payload pointing to the IPv6 header
 * @param code Code of the ICMPv6 header
 * @param data Additional 32-bit parameter in the ICMPv6 header
 * @param type Type of the ICMPv6 header
 */
static void
icmp6_send_response(struct pbuf *p, u8_t code, u32_t data, u8_t type)
{
  struct pbuf *q = NULL;
  struct icmpv6_hdr *icmp6hdr = NULL;
  const ip6_addr_t *reply_src = NULL;
  ip6_addr_t *reply_dest = NULL;
  ip6_addr_t reply_src_local, reply_dest_local;
  struct ip6_hdr *ip6hdr = NULL;
  struct netif *netif = NULL;
  u32_t icmplen;

#if LWIP_ICMP6_ERR_RT_LMT
  u8_t ret = icmp6_err_update_rate_lmt();
  if (ret != 0) {
    LWIP_DEBUGF(ICMP_DEBUG,
                ("icmp_time_exceeded: rate limit execceded for the packet so dropping the error icmp packet.\n"));
    ICMP6_STATS_INC(icmp6.drop);
    return;
  }
#endif

  /* Get the destination address and netif for this ICMP message. */
  if ((ip_current_netif() == NULL) ||
      ((code == ICMP6_TE_FRAG) && (type == ICMP6_TYPE_TE))) {
    /* Special case, as ip6_current_xxx is either NULL, or points
     * to a different packet than the one that expired.
     * We must use the addresses that are stored in the expired packet.
     */
    ip6hdr = (struct ip6_hdr *)p->payload;
    /* copy from packed address to aligned address */
    ip6_addr_copy(reply_dest_local, ip6hdr->src);
    ip6_addr_copy(reply_src_local, ip6hdr->dest);
    reply_dest = &reply_dest_local;
    reply_src = &reply_src_local;
#if LWIP_SO_DONTROUTE
    netif = ip6_route(reply_src, reply_dest, RT_SCOPE_UNIVERSAL);
#else
    netif = ip6_route(reply_src, reply_dest);
#endif
    if (netif == NULL) {
      ICMP6_STATS_INC(icmp6.rterr);
      return;
    }
  } else {
    netif = ip_current_netif();
    reply_dest = ip6_current_src_addr();

    /* Select an address to use as source. */
    reply_src = ip_2_ip6(ip6_select_source_address(netif, reply_dest));
    if (reply_src == NULL) {
      /* drop */
      ICMP6_STATS_INC(icmp6.rterr);
      return;
    }
  }

  /*
   * RFC compliance : rfc 4443: section:2.4
   * Every ICMPv6 error message (type < 128) MUST include as much of
   * the IPv6 offending (invoking) packet (the packet that caused the
   * error) as possible without making the error message packet exceed
   * the minimum IPv6 MTU [IPv6].
   */
  icmplen = LWIP_MIN(p->tot_len, (NETIF_MTU_MIN - (IP6_HLEN + sizeof(struct icmpv6_hdr))));

  q = pbuf_alloc(PBUF_IP, (u16_t)(sizeof(struct icmpv6_hdr) + icmplen), PBUF_RAM);
  if (q == NULL) {
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_time_exceeded: failed to allocate pbuf for ICMPv6 packet.\n"));
    ICMP6_STATS_INC(icmp6.memerr);
    return;
  }

  icmp6hdr = (struct icmpv6_hdr *)q->payload;
  icmp6hdr->type = type;
  icmp6hdr->code = code;
  icmp6hdr->data = lwip_htonl(data);

  /* copy fields from original packet */
  if (memcpy_s((u8_t *)q->payload + sizeof(struct icmpv6_hdr), icmplen, (u8_t *)p->payload, icmplen) != EOK) {
    LWIP_DEBUGF(ICMP_DEBUG, ("memcpy_s error.\n"));
    ICMP6_STATS_INC(icmp6.memerr);
    (void)pbuf_free(q);
    return;
  }

  /* calculate checksum */
  icmp6hdr->chksum = 0;
#if CHECKSUM_GEN_ICMP6
  IF__NETIF_CHECKSUM_ENABLED(netif, NETIF_CHECKSUM_GEN_ICMP6) {
    icmp6hdr->chksum = ip6_chksum_pseudo(q, IP6_NEXTH_ICMP6, q->tot_len, reply_src, reply_dest);
  }
#endif /* CHECKSUM_GEN_ICMP6 */

#if LWIP_MAC_SECURITY
  if ((p->flags & PBUF_FLAG_WITH_ENCRYPTION) != 0) {
    q->flags |= PBUF_FLAG_WITH_ENCRYPTION;
  }
#endif

  ICMP6_STATS_INC(icmp6.xmit);
  (void)ip6_output_if(q, reply_src, reply_dest, LWIP_ICMP6_HL, 0, IP6_NEXTH_ICMP6, netif);
  (void)pbuf_free(q);
}

#endif /* LWIP_ICMP6 && LWIP_IPV6 */
