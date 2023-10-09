/**
  * @file
 *
 * 6LowPAN output for IPv6. Uses ND tables for link-layer addressing. Fragments packets to 6LowPAN units.
 */

/*
 * Copyright (c) 2015 Inico Technologies Ltd.
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

/**
 * @defgroup sixlowpan 6LowPAN netif
 * @ingroup addons
 * 6LowPAN netif implementation
 */

#include "netif/lowpan6.h"

#if LWIP_IPV6 && LWIP_6LOWPAN

#include "lwip/ip.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/nd6.h"
#include "lwip/mem.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"
#include "lwip/snmp.h"
#include "lwip/link_quality.h"
#if ETH_6LOWPAN
#include "lwip/prot/ethernet.h"
#include "netif/ethernet.h"
#endif
#if LWIP_LOWPOWER
#include "lwip/lowpower.h"
#endif
#include <string.h>

#define LOWPAN6_DISPATCH_IPHC 0x60
#define LOWPAN_FRAG1_HDR_LEN 4
#define LOWPAN_FRAGN_HDR_LEN 5
#define LOWPAN_IPv6_SRCADDR_OFFSET 8
#define LOWPAN_IPv6_DSTADDR_OFFSET 24

#define IEEE_EUI64_ADDR_SIZE 8
#define IEEE_EUI48_ADDR_SIZE 6

#if LWIP_PLC
/*
 * plc has two type of broadcast mac address, one is local broadcast,
 * 00:FF:FF:FF:FF:FF, that means one-hop broadcast,
 * the other is entire network broadcast,
 * FF:FF:FF:FF:FF:FF
 */
static unsigned char g_plc_local_broadcast[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static unsigned char g_plc_global_broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define LOWPAN_IS_PLC_BROADCAST_MAC(macaddrbuff, size) \
  ((memcmp(macaddrbuff, g_plc_local_broadcast, size) == 0) || \
   (memcmp(macaddrbuff, g_plc_global_broadcast, size) == 0))
#endif

#if LWIP_IEEE802154
static unsigned char g_rf_long_broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define LOWPAN_IS_RF_BROADCAST_MAC(macaddrbuff, size) \
  (memcmp(macaddrbuff, g_rf_long_broadcast, size) == 0)
#endif

/* This is a helper struct.
 */
struct lowpan6_reass_helper {
  struct pbuf *pbuf;
  struct lowpan6_reass_helper *next_packet;
  u8_t timer;
  struct linklayer_addr sender_addr;
  u16_t datagram_size;
  u16_t datagram_tag;
  struct netif *netifinfo;
};

struct eui64_linklayer_addr {
  u8_t addr[IEEE_EUI64_ADDR_SIZE];
  u8_t addrlen;
};

static struct lowpan6_reass_helper *reass_list = NULL;

#if LWIP_6LOWPAN_NUM_CONTEXTS > 0
static struct lowpan6_context_info lowpan6_context[LWIP_6LOWPAN_NUM_CONTEXTS];
#endif

static err_t dequeue_datagram(struct lowpan6_reass_helper *lrh);

/*
 * Periodic timer for 6LowPAN functions:
 *
 * - Remove incomplete/old packets
 */
void
lowpan6_tmr(void)
{
  struct lowpan6_reass_helper *lrh = NULL;
  struct lowpan6_reass_helper *lrh_temp = NULL;

  lrh = reass_list;
  while (lrh != NULL) {
    lrh_temp = lrh->next_packet;
    if ((--lrh->timer) == 0) {
      (void)dequeue_datagram(lrh);
      (void)pbuf_free(lrh->pbuf);
      mem_free(lrh);
      LOWPAN6_STATS_INC(lowpan6.reass_tout);
      LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_tmr: Freeing the reass context as "
                                  "timeout occurs\n"));
    }
    lrh = lrh_temp;
  }
}

#if LWIP_LOWPOWER
u32_t
lowpan6_tmr_tick()
{
  struct lowpan6_reass_helper *lrh = NULL;
  struct lowpan6_reass_helper *lrh_temp = NULL;
  u32_t tick = 0;

  lrh = reass_list;
  while (lrh != NULL) {
    lrh_temp = lrh->next_packet;
    if (lrh->timer > 0) {
      SET_TMR_TICK(tick, lrh->timer);
    }
    lrh = lrh_temp;
  }

  LOWPOWER_DEBUG(("%s tmr tick: %u\n", __func__, tick));
  return tick;
}
#endif

/*
 * Removes a datagram from the reassembly queue.
 **/
static err_t
dequeue_datagram(struct lowpan6_reass_helper *lrh)
{
  struct lowpan6_reass_helper *lrh_temp = NULL;

  if (reass_list == lrh) {
    reass_list = reass_list->next_packet;
  } else {
    lrh_temp = reass_list;
    while (lrh_temp != NULL) {
      if (lrh_temp->next_packet == lrh) {
        lrh_temp->next_packet = lrh->next_packet;
        break;
      }
      lrh_temp = lrh_temp->next_packet;
    }
  }

  return ERR_OK;
}

void
lowpan6_free_reass_context(struct netif *netif)
{
  struct lowpan6_reass_helper *lrh_temp = NULL;
  struct lowpan6_reass_helper *curlrh = NULL;

  LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_free_reass_context: "
                              "Will free the reassmble context for the netif[%p] \n", netif));
  curlrh = reass_list;
  while (curlrh != NULL) {
    lrh_temp = curlrh->next_packet;
    if (curlrh->netifinfo == netif) {
      (void)dequeue_datagram(curlrh);
      (void)pbuf_free(curlrh->pbuf);
      mem_free(curlrh);
    }

    curlrh = lrh_temp;
  }

  return;
}

static s8_t
lowpan6_context_lookup(const ip6_addr_t *ip6addr)
{
#if (LWIP_6LOWPAN_NUM_CONTEXTS > 0)
  s8_t i;

  for (i = 0; i < LWIP_6LOWPAN_NUM_CONTEXTS; i++) {
    if ((lowpan6_context[i].is_used == 1) &&
        ip6_addr_netcmp(&(lowpan6_context[i].context_prefix), ip6addr)) {
      return lowpan6_context[i].context_id;
    }
  }
#else
  LWIP_UNUSED_ARG(ip6addr);
#endif

  return -1;
}

static
struct lowpan6_context_info *
lowpan6_get_context_byid(u8_t ctxid)
{
#if LWIP_6LOWPAN_NUM_CONTEXTS > 0
  s8_t i;

  for (i = 0; i < LWIP_6LOWPAN_NUM_CONTEXTS; i++) {
    if ((lowpan6_context[i].is_used == 1) &&
        lowpan6_context[i].context_id == ctxid) {
      return &(lowpan6_context[i]);
    }
  }
#else
  LWIP_UNUSED_ARG(ctxid);
#endif

  return NULL;
}

err_t
lowpan6_set_context(struct netif *hwface, u8_t ctxid,
                    const ip6_addr_t *context_prefix)
{
#if LWIP_6LOWPAN_NUM_CONTEXTS > 0
  struct lowpan6_context_info *pst_context = NULL;
  s8_t i;

  if ((hwface == NULL) || (context_prefix == NULL)) {
    return ERR_ARG;
  }
  LWIP_DEBUGF(LOWPAN6_DEBUG, (" CTX to add[%s] with id[%d]\n",
                              ip6addr_ntoa(context_prefix), ctxid));

  IP6_ADDR_ZONECHECK(context_prefix);
  /* Check if we already have the context information with the given ID */
  pst_context = lowpan6_get_context_byid(ctxid);
  if (pst_context != NULL) {
    goto UPDATE_INFO;
  }

  /* Find a free context */
  for (i = 0; i < LWIP_6LOWPAN_NUM_CONTEXTS; i++) {
    if (lowpan6_context[i].is_used == 0) {
      pst_context = &(lowpan6_context[i]);
      break;
    }
  }

  if (pst_context == NULL) {
    LWIP_DEBUGF(LOWPAN6_DEBUG, (" Didn't find free context cb\n"));
    return ERR_MEM;
  }

UPDATE_INFO:
  pst_context->context_id = ctxid;
  pst_context->iface = (void *)hwface;
  pst_context->is_used = 1;
  ip6_addr_set(&(pst_context->context_prefix), context_prefix);

  return ERR_OK;
#else
  LWIP_UNUSED_ARG(hwface);
  LWIP_UNUSED_ARG(ctxid);
  LWIP_UNUSED_ARG(context_prefix);

  /* User gas not set the CB Size so no memory */
  return ERR_MEM;

#endif
}

err_t
lowpan6_remove_context(struct netif *hwface, u8_t ctxid)
{
#if LWIP_6LOWPAN_NUM_CONTEXTS > 0
  struct lowpan6_context_info *pst_context = NULL;

  /* Check if we already have the context information with the given ID */
  pst_context = lowpan6_get_context_byid(ctxid);
  if (pst_context == NULL) {
    return ERR_LOWPAN_CTX_NOTFOUND;
  }

  pst_context->is_used = 0;

#else
  LWIP_UNUSED_ARG(ctxid);

#endif

  LWIP_UNUSED_ARG(hwface);
  return ERR_OK;
}

/* Determine compression mode for unicast address. */
static s8_t
lowpan6_get_address_mode(const ip6_addr_t *ip6addr, const struct eui64_linklayer_addr *mac_addr)
{
  if (mac_addr->addrlen == 2) {
    if ((ip6addr->addr[2] == (u32_t)PP_HTONL(0x000000ff)) &&
        ((ip6addr->addr[3]  & PP_HTONL(0xffff0000)) == PP_NTOHL(0xfe000000))) {
      if ((ip6addr->addr[3]  & PP_HTONL(0x0000ffff)) == lwip_ntohl((mac_addr->addr[0] << 8) | mac_addr->addr[1])) {
        return 3;
      }
    }
  } else if (mac_addr->addrlen == 8) {
    if ((ip6addr->addr[2] == lwip_ntohl(((mac_addr->addr[0] ^ 2) << 24) |
                                        (mac_addr->addr[1] << 16) |
                                        (mac_addr->addr[2] << 8) |
                                        mac_addr->addr[3])) &&
        (ip6addr->addr[3] == lwip_ntohl((mac_addr->addr[4] << 24) |
                                        (mac_addr->addr[5] << 16) |
                                        (mac_addr->addr[6] << 8) |
                                        mac_addr->addr[7]))) {
      return 3;
    }
  }

  if ((ip6addr->addr[2] == PP_HTONL(0x000000ffUL)) &&
      ((ip6addr->addr[3] & PP_HTONL(0xffff0000)) == PP_NTOHL(0xfe000000UL))) {
    return 2;
  }

  return 1;
}

/* Determine compression mode for multicast address. */
static s8_t
lowpan6_get_address_mode_mc(const ip6_addr_t *ip6addr)
{
  if ((ip6addr->addr[0] == PP_HTONL(0xff020000)) &&
      (ip6addr->addr[1] == 0) &&
      (ip6addr->addr[2] == 0) &&
      ((ip6addr->addr[3] & PP_HTONL(0xffffff00)) == 0)) {
    return 3;
  } else if (((ip6addr->addr[0] & PP_HTONL(0xff00ffff)) == PP_HTONL(0xff000000)) &&
             (ip6addr->addr[1] == 0)) {
    if ((ip6addr->addr[2] == 0) &&
        ((ip6addr->addr[3] & PP_HTONL(0xff000000)) == 0)) {
      return 2;
    } else if ((ip6addr->addr[2] & PP_HTONL(0xffffff00)) == 0) {
      return 1;
    }
  }

  return 0;
}

static void
lowpan6_eui48_2_eui64(const struct linklayer_addr *eui48, struct eui64_linklayer_addr *eui64)
{
#if LWIP_PLC
  if (LOWPAN_IS_PLC_BROADCAST_MAC(eui48->addr, eui48->addrlen)) {
    (void)memset_s(eui64, sizeof(struct eui64_linklayer_addr), 0, sizeof(struct eui64_linklayer_addr));
    return;
  }
#endif

  eui64->addr[0] = eui48->addr[0];
  eui64->addr[1] = eui48->addr[1];
  eui64->addr[2] = eui48->addr[2];
  eui64->addr[3] = 0xFF;
  eui64->addr[4] = 0xFE;
  eui64->addr[5] = eui48->addr[3];
  eui64->addr[6] = eui48->addr[4];
  eui64->addr[7] = eui48->addr[5];

  eui64->addrlen = IEEE_EUI64_ADDR_SIZE;
  return;
}

/*
 * Encapsulates data into IEEE 802.15.4 frames.
 * Fragments an IPv6 datagram into 6LowPAN units, which fit into IEEE 802.15.4 frames.
 * If configured, will compress IPv6 and or UDP headers.
 * */
static err_t
lowpan6_frag(struct netif *netif, struct pbuf *p, const struct linklayer_addr *src, const struct linklayer_addr *dst)
{
  struct pbuf *p_frag = NULL;
  u16_t frag_len, remaining_len;
  u16_t compressed_hdr_len = 0;
  u16_t macmaxmtu = 0;
  u16_t macoverhead = 0;
  u16_t max_payload;
  u8_t *buffer = NULL;
  u8_t iphc_start_idx;
  u8_t lowpan6_header_len;
  s8_t i;
  static u16_t datagram_tag;
  u16_t datagram_offset;
  err_t err = ERR_IF;
  u16_t p_len;

#if ETH_6LOWPAN
  int ret;
  struct eth_addr eth_dst;
#endif

  struct eui64_linklayer_addr srceu64;
  struct eui64_linklayer_addr desteu64;

  LOWPAN6_STATS_INC(lowpan6.pkt_from_ip);

  if (netif->link_layer_type == PLC_DRIVER_IF) {
    macmaxmtu = LOWPAN6_PLC_MAX_LINK_MTU;
    macoverhead = LOWPAN6_PLC_MAC_FIXED_OVERHEAD;
  } else if (netif->link_layer_type == IEEE802154_DRIVER_IF) {
    macmaxmtu = LOWPAN6_RF_MAX_LINK_MTU;
    macoverhead = LOWPAN6_RF_MAC_FIXED_OVERHEAD;
  } else if (netif->link_layer_type == WIFI_DRIVER_IF) {
#if ETH_6LOWPAN
    macmaxmtu = netif->mtu6;
    macoverhead = 0;
#else
    return ERR_VAL;
#endif
  }

  /* Convert the EUI48 bit to EUI 64 */
  if ((src->addrlen == IEEE_EUI48_ADDR_SIZE) &&
      ((netif->link_layer_type == PLC_DRIVER_IF) || (netif->link_layer_type == WIFI_DRIVER_IF))) {
    lowpan6_eui48_2_eui64(src, &srceu64);
  } else if ((src->addrlen == IEEE_EUI64_ADDR_SIZE) &&
             (NETIF_MAX_HWADDR_LEN == IEEE_EUI64_ADDR_SIZE)) {
    srceu64 = *((struct eui64_linklayer_addr *)src);
  } else {
    LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_frag: Invalid source MAC address\n"));
    return ERR_VAL;
  }

  if ((dst->addrlen == IEEE_EUI48_ADDR_SIZE) &&
      ((netif->link_layer_type == PLC_DRIVER_IF) || (netif->link_layer_type == WIFI_DRIVER_IF))) {
    lowpan6_eui48_2_eui64(dst, &desteu64);
  } else if ((dst->addrlen == IEEE_EUI64_ADDR_SIZE) &&
             (NETIF_MAX_HWADDR_LEN == IEEE_EUI64_ADDR_SIZE)) {
    desteu64 = *((struct eui64_linklayer_addr *)dst);
  } else {
    LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_frag: Invalid dest MAC or driver type address\n"));
    return ERR_VAL;
  }

  /* We'll use a dedicated pbuf for building 6LowPAN fragments. */
#if ETH_6LOWPAN
  p_frag = pbuf_alloc(PBUF_LINK, macmaxmtu, PBUF_RAM);
#else
  p_frag = pbuf_alloc(PBUF_RAW, macmaxmtu, PBUF_POOL);
#endif
  if (p_frag == NULL) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    return ERR_MEM;
  }

  p_frag->flags = p->flags;
#if (LWIP_RPL || LWIP_RIPPLE)
  p_frag->flags |= PBUF_FLAG_6LO_PKT;
#endif /* LWIP_RPL || LWIP_RIPPLE */

#if LWIP_SO_PRIORITY
  p_frag->priority = p->priority;
#endif /* LWIP_SO_PRIORITY */

  buffer = (u8_t *)p_frag->payload;
  iphc_start_idx = 0;
  p_len = p->tot_len;
  LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_frag: Started compression and frag\n"));

#if LWIP_6LOWPAN_IPHC
  /* Perform 6LowPAN IPv6 header compression according to RFC 6282 */
  {
    struct ip6_hdr *ip6hdr = NULL;
    ip6_addr_t source_ip, dest_ip;
    ip6_addr_t *p_src_ip = &source_ip;

    /* Point to ip6 header and align copies of src/dest addresses. */
    ip6hdr = (struct ip6_hdr *)p->payload;

    ip6_addr_copy(dest_ip, ip6hdr->dest);
    ip6_addr_assign_zone(&dest_ip, IP6_UNKNOWN, netif);
    ip6_addr_copy(source_ip, ip6hdr->src);
    ip6_addr_assign_zone(&source_ip, IP6_UNKNOWN, netif);

    /* Basic length of 6LowPAN header, set dispatch and clear fields. */
    lowpan6_header_len = 2;
    buffer[iphc_start_idx] = LOWPAN6_DISPATCH_IPHC;

    buffer[iphc_start_idx + 1] = 0;

    /* Determine whether there will be a Context Identifier Extension byte or not.
    * If so, set it already. */
#if LWIP_6LOWPAN_NUM_CONTEXTS > 0
    buffer[iphc_start_idx + 2] = 0;

    i = lowpan6_context_lookup(&source_ip);
    if (i >= 0) {
      /* Stateful source address compression. */
      buffer[iphc_start_idx + 1] |= 0x40;
      buffer[iphc_start_idx + 2] |= (i & 0x0f) << 4;
    }

    i = lowpan6_context_lookup(&dest_ip);
    if (i >= 0) {
      /* Stateful destination address compression. */
      buffer[iphc_start_idx + 1] |= 0x04;
      buffer[iphc_start_idx + 2] |= i & 0x0f;
    }

    if (buffer[iphc_start_idx + 2] != 0x00) {
      /* Context identifier extension byte is appended. */
      buffer[iphc_start_idx + 1] |= 0x80;
      lowpan6_header_len++;
    }

#endif /* LWIP_6LOWPAN_NUM_CONTEXTS > 0 */

    /* Determine TF field: Traffic Class, Flow Label */
    if (IP6H_FL(ip6hdr) == 0) {
      /* Flow label is elided. */
      buffer[iphc_start_idx] |= 0x10;
      if (IP6H_TC(ip6hdr) == 0) {
        /* Traffic class (ECN+DSCP) elided too. */
        buffer[iphc_start_idx] |= 0x08;
      } else {
        /* Traffic class (ECN+DSCP) appended. */
        buffer[lowpan6_header_len++] = IP6H_TC(ip6hdr);
      }
    } else {
      if (((IP6H_TC(ip6hdr) & 0x3f) == 0)) {
        /* DSCP portion of Traffic Class is elided, ECN and FL are appended (3 bytes) */
        buffer[iphc_start_idx] |= 0x08;

        buffer[lowpan6_header_len] = IP6H_TC(ip6hdr) & 0xc0;
        buffer[lowpan6_header_len++] |= (IP6H_FL(ip6hdr) >> 16) & 0x0f;
        buffer[lowpan6_header_len++] = (IP6H_FL(ip6hdr) >> 8) & 0xff;
        buffer[lowpan6_header_len++] = IP6H_FL(ip6hdr) & 0xff;
      } else {
        /* Traffic class and flow label are appended (4 bytes) */
        buffer[lowpan6_header_len++] = IP6H_TC(ip6hdr);
        buffer[lowpan6_header_len++] = (IP6H_FL(ip6hdr) >> 16) & 0x0f;
        buffer[lowpan6_header_len++] = (IP6H_FL(ip6hdr) >> 8) & 0xff;
        buffer[lowpan6_header_len++] = IP6H_FL(ip6hdr) & 0xff;
      }
    }

    /* Compress NH?
    * Only if UDP for now. @todo support other NH compression. */
    if (IP6H_NEXTH(ip6hdr) == IP6_NEXTH_UDP) {
      buffer[iphc_start_idx] |= 0x04;
    } else {
      /* append nexth. */
      buffer[lowpan6_header_len++] = IP6H_NEXTH(ip6hdr);
    }

    /* Compress hop limit? */
    if (IP6H_HOPLIM(ip6hdr) == 255) {
      buffer[iphc_start_idx] |= 0x03;
    } else if (IP6H_HOPLIM(ip6hdr) == 64) {
      buffer[iphc_start_idx] |= 0x02;
    } else if (IP6H_HOPLIM(ip6hdr) == 1) {
      buffer[iphc_start_idx] |= 0x01;
    } else {
      /* append hop limit */
      buffer[lowpan6_header_len++] = IP6H_HOPLIM(ip6hdr);
    }

    /* Compress source address */
    if (((buffer[iphc_start_idx + 1] & 0x40) != 0) || (ip6_addr_islinklocal(&source_ip))) {
      /* Context-based or link-local source address compression. */
      i = lowpan6_get_address_mode(&source_ip, &srceu64);
      buffer[iphc_start_idx + 1] |= (i & 0x03) << 4;
      if (i == 1) {
        if (memcpy_s((buffer + lowpan6_header_len), (macmaxmtu - lowpan6_header_len),
                     (u8_t *)p->payload + 16, 8) != EOK) {
          (void)pbuf_free(p_frag);
          return ERR_MEM;
        }
        lowpan6_header_len += 8;
      } else if (i == 2) {
        if (memcpy_s((buffer + lowpan6_header_len), (macmaxmtu - lowpan6_header_len),
                     (u8_t *)p->payload + 22, 2) != EOK) {
          (void)pbuf_free(p_frag);
          return ERR_MEM;
        }
        lowpan6_header_len += 2;
      }
    } else if (ip6_addr_isany(p_src_ip)) {
      /* Special case: mark SAC and leave SAM=0 */
      buffer[iphc_start_idx + 1] |= 0x40;
    } else {
      /* Append full address. */
      if (memcpy_s(buffer + lowpan6_header_len,
                   (macmaxmtu - lowpan6_header_len), (u8_t *)p->payload + 8, 16) != EOK) {
          (void)pbuf_free(p_frag);
          return ERR_MEM;
      }
      lowpan6_header_len += 16;
    }

    /* Compress destination address */
    if (ip6_addr_ismulticast(&dest_ip)) {
      /* @todo support stateful multicast address compression */

      LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_frag: Destination is mcast\n"));

      buffer[iphc_start_idx + 1] |= 0x08;

      i = lowpan6_get_address_mode_mc(&dest_ip);
      buffer[iphc_start_idx + 1] |= i & 0x03;
      if (i == 0) {
        if (memcpy_s(buffer + lowpan6_header_len,
                     (macmaxmtu - lowpan6_header_len), (u8_t *)p->payload + 24, 16) != EOK) {
          (void)pbuf_free(p_frag);
          return ERR_MEM;
        }
        lowpan6_header_len += 16;
      } else if (i == 1) {
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[25];
        if (memcpy_s(buffer + lowpan6_header_len, (macmaxmtu - lowpan6_header_len),
                     (u8_t *)p->payload + 35, 5) != EOK) {
          (void)pbuf_free(p_frag);
          return ERR_MEM;
        }
        lowpan6_header_len += 5;
      } else if (i == 2) {
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[25];
        if (memcpy_s(buffer + lowpan6_header_len, (macmaxmtu - lowpan6_header_len),
                     (u8_t *)p->payload + 37, 3) != EOK) {
          (void)pbuf_free(p_frag);
          return ERR_MEM;
        }
        lowpan6_header_len += 3;
      } else if (i == 3) {
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[39];
      }
    } else if (((buffer[iphc_start_idx + 1] & 0x04) != 0) || (ip6_addr_islinklocal(&dest_ip))) {
      /* Context-based or link-local destination address compression. */
      i = lowpan6_get_address_mode(&dest_ip, &desteu64);
      buffer[iphc_start_idx + 1] |= i & 0x03;
      if (i == 1) {
        if (memcpy_s(buffer + lowpan6_header_len, (macmaxmtu - lowpan6_header_len),
                     (u8_t *)p->payload + 32, 8) != EOK) {
          (void)pbuf_free(p_frag);
          return ERR_MEM;
        }
        lowpan6_header_len += 8;
      } else if (i == 2) {
        if (memcpy_s(buffer + lowpan6_header_len, (macmaxmtu - lowpan6_header_len),
                     (u8_t *)p->payload + 38, 2) != EOK) {
          (void)pbuf_free(p_frag);
          return ERR_MEM;
        }
        lowpan6_header_len += 2;
      }
    } else {
      /* Append full address. */
      if (memcpy_s(buffer + lowpan6_header_len, (macmaxmtu - lowpan6_header_len),
                   (u8_t *)p->payload + 24, 16) != EOK) {
        (void)pbuf_free(p_frag);
        return ERR_MEM;
      }
      lowpan6_header_len += 16;
    }

    /* Move to payload. */
    (void)pbuf_remove_header(p, -IP6_HLEN);
    compressed_hdr_len = (u16_t)(compressed_hdr_len + IP6_HLEN);

    /* Compress UDP header? */
    if (IP6H_NEXTH(ip6hdr) == IP6_NEXTH_UDP) {
      /* @todo support optional checksum compression */

      buffer[lowpan6_header_len] = 0xf0;

      /* determine port compression mode. */
      if ((((u8_t *)p->payload)[0] == 0xf0) && ((((u8_t *)p->payload)[1] & 0xf0) == 0xb0) &&
          (((u8_t *)p->payload)[2] == 0xf0) && ((((u8_t *)p->payload)[3] & 0xf0) == 0xb0)) {
        /* Compress source and dest ports. */
        buffer[lowpan6_header_len++] |= 0x03;
        buffer[lowpan6_header_len++] = ((((u8_t *)p->payload)[1] & 0x0f) << 4) | (((u8_t *)p->payload)[3] & 0x0f);
      } else if (((u8_t *)p->payload)[0] == 0xf0) {
        /* Compress source port. */
        buffer[lowpan6_header_len++] |= 0x02;
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[1];
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[2];
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[3];
      } else if (((u8_t *)p->payload)[2] == 0xf0) {
        /* Compress dest port. */
        buffer[lowpan6_header_len++] |= 0x01;
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[0];
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[1];
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[3];
      } else {
        /* append full ports. */
        lowpan6_header_len++;
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[0];
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[1];
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[2];
        buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[3];
      }

      /* elide length and copy checksum */
      buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[6];
      buffer[lowpan6_header_len++] = ((u8_t *)p->payload)[7];

      (void)pbuf_remove_header(p, UDP_HLEN);
      compressed_hdr_len = (u16_t)(compressed_hdr_len + UDP_HLEN);
    }
  }

#else /* LWIP_6LOWPAN_HC */
  /* Send uncompressed IPv6 header with appropriate dispatch byte. */
  lowpan6_header_len = 1;
  buffer[iphc_start_idx] = 0x41; /* IPv6 dispatch */
#endif /* LWIP_6LOWPAN_HC */

  /* Calculate remaining packet length */
  remaining_len = p->tot_len;

  if (remaining_len > 0x7FF) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    /* datagram_size must fit into 11 bit */
    LOWPAN6_STATS_INC(lowpan6.comp_fail);
    (void)pbuf_free(p_frag);
    return ERR_VAL;
  }
#if ETH_6LOWPAN
  ret = memcpy_s(eth_dst.addr, sizeof(eth_dst.addr), dst->addr, dst->addrlen);
  if (ret != EOK) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    /* datagram_size must fit into 11 bit */
    LOWPAN6_STATS_INC(lowpan6.comp_fail);
    (void)pbuf_free(p_frag);
    return ERR_VAL;
  }
#endif
  /* Fragment, or 1 packet? */
  if (remaining_len > (macmaxmtu - (lowpan6_header_len + macoverhead))) {
    /* We must move the 6LowPAN header to make room for the FRAG header. */
    LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_frag: This packet will be fragmented\n"));

    max_payload = macmaxmtu - macoverhead;

    i = lowpan6_header_len;
    while (i-- != 0) {
      buffer[iphc_start_idx + i + LOWPAN_FRAG1_HDR_LEN] = buffer[iphc_start_idx + i];
    }

    /* Now we need to fragment the packet. FRAG1 header first */
    buffer[iphc_start_idx] = 0xc0 | (((p->tot_len + compressed_hdr_len) >> 8) & 0x7);
    buffer[iphc_start_idx + 1] = (p->tot_len + compressed_hdr_len) & 0xff;

    datagram_tag++;
    buffer[iphc_start_idx + 3] = (datagram_tag >> 8) & 0xff;
    buffer[iphc_start_idx + 2] = datagram_tag & 0xff;

    /* Fragment follows. */
    frag_len = (max_payload - (lowpan6_header_len + LOWPAN_FRAG1_HDR_LEN)) & 0xfff8;

    (void)pbuf_copy_partial(p, (buffer + (lowpan6_header_len + LOWPAN_FRAG1_HDR_LEN)), frag_len, 0);
    remaining_len = remaining_len - frag_len;
    datagram_offset = frag_len + compressed_hdr_len;

    /* Calculate frame length */
    p_frag->tot_len = frag_len + lowpan6_header_len + LOWPAN_FRAG1_HDR_LEN;
    p_frag->len = p_frag->tot_len;

    /* send the packet */
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p_frag->tot_len);

    LWIP_DEBUGF(LOWPAN6_DEBUG | LWIP_DBG_TRACE,
                ("lowpan6_send: sending fragment %p lenghth[%u]\n", (void *)p_frag, p_frag->tot_len));
#if ETH_6LOWPAN
    err = ethernet_output(netif, p_frag, (const struct eth_addr *)(netif->hwaddr), &eth_dst, ETHTYPE_6LOWPAN);
#else
    err = netif->drv_lln_send(netif, p_frag, (struct linklayer_addr *)dst);
#endif
    while ((remaining_len > 0) && (err == ERR_OK)) {
      buffer[iphc_start_idx] |= 0x20; /* Change FRAG1 to FRAGN */
      /* datagram offset in FRAGN header (datagram_offset is max. 11 bit) */
      buffer[iphc_start_idx + 4] = (u8_t)(datagram_offset >> 3);

      frag_len = (max_payload - LOWPAN_FRAGN_HDR_LEN) & 0xfff8;
      if (frag_len > remaining_len) {
        frag_len = remaining_len;
      }

      (void)pbuf_copy_partial(p, buffer + LOWPAN_FRAGN_HDR_LEN, frag_len, p->tot_len - remaining_len);
      remaining_len -= frag_len;
      datagram_offset += frag_len;

      /* Calculate frame length */
      p_frag->payload = buffer;
      p_frag->tot_len = frag_len + LOWPAN_FRAGN_HDR_LEN;
      p_frag->len = p_frag->tot_len;

      /* send the packet */
      MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p_frag->tot_len);
      LWIP_DEBUGF(LOWPAN6_DEBUG | LWIP_DBG_TRACE,
                  ("lowpan6_send: sending fragment %p length[%u]\n", (void *)p_frag, p_frag->tot_len));
#if ETH_6LOWPAN
      err = ethernet_output(netif, p_frag, (const struct eth_addr *)(netif->hwaddr), &eth_dst, ETHTYPE_6LOWPAN);
#else
      err = netif->drv_lln_send(netif, p_frag, (struct linklayer_addr *)dst);
#endif
    }
  } else {
    /* It fits in one frame. */
    frag_len = remaining_len;

    /* Copy IPv6 packet */
    (void)pbuf_copy_partial(p, buffer + lowpan6_header_len, frag_len, 0);
    remaining_len = 0;

    /* Calculate frame length */
    p_frag->tot_len = frag_len + lowpan6_header_len;
    p_frag->len = p_frag->tot_len;

    /* send the packet */
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p_frag->tot_len);
    LWIP_DEBUGF(LOWPAN6_DEBUG | LWIP_DBG_TRACE,
                ("lowpan6_send: sending complete packet %p length[%u]\n", (void *)p_frag, p_frag->tot_len));
#if ETH_6LOWPAN
    err = ethernet_output(netif, p_frag, (const struct eth_addr *)(netif->hwaddr), &eth_dst, ETHTYPE_6LOWPAN);
#else
    err = netif->drv_lln_send(netif, p_frag, (struct linklayer_addr *)dst);
#endif
  }

  if (err == ERR_OK) {
    LOWPAN6_STATS_INC(lowpan6.pkt_xmit);
  }

  if ((p_len > p->tot_len) && (p_len <= (p->tot_len + UDP_HLEN + IP6_HLEN))) {
    (void)pbuf_header(p, p_len - p->tot_len);
  }

  (void)pbuf_free(p_frag);
  return err;
}

/**
 * Resolve and fill-in IEEE 802.15.4 address header for outgoing IPv6 packet.
 *
 * Perform Header Compression and fragment if necessary.
 *
 * @param netif The lwIP network interface which the IP packet will be sent on.
 * @param q The pbuf(s) containing the IP packet to be sent.
 * @param ip6addr The IP address of the packet destination.
 *
 * @return err_t
 */
err_t
lowpan6_output(struct netif *netif, struct pbuf *q, const struct linklayer_addr *dstlinkaddr)
{
  struct linklayer_addr src;

  LWIP_DEBUGF(LOWPAN6_DEBUG | LWIP_DBG_TRACE, ("lowpan6_output: processing IPv6"
                                               " packet %p with secflag[%x]\n", (void *)q, q->flags));

  src.addrlen = netif->hwaddr_len;
  (void)memcpy_s(src.addr, netif->hwaddr_len, netif->hwaddr, netif->hwaddr_len);

  /* Send out the packet using the returned hardware address. */
  MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);

  return lowpan6_frag(netif, q, &src, dstlinkaddr);
}

#define SICSLOWPAN_VALIDATE_LENGTH(aval, reqd) do { \
  if ((aval) < (reqd)) { \
    (void)pbuf_free(p); \
    (void)pbuf_free(q); \
    LOWPAN6_STATS_INC(lowpan6.corrupt_pkt); \
    return NULL; \
  } \
} while (0)

#define SICSLOWPAN_REDUCE_LEN(len, factor) do { \
  (len) = (uint16_t)((len) - (factor)); \
} while (0)

LWIP_STATIC struct pbuf *
lowpan6_decompress(struct pbuf *p, u16_t datagram_size,
                   struct linklayer_addr *src, struct linklayer_addr *dest, u16_t link_layer_type)
{
  struct pbuf *q = NULL;
  u8_t *lowpan6_buffer = NULL;
  u16_t lowpan6_offset;
  struct ip6_hdr *ip6hdr = NULL;
  s8_t i;
  s8_t ip6_offset = IP6_HLEN;
  u16_t ip6buffsize;
  u16_t len2decomp;
  struct eui64_linklayer_addr srceu64;
  struct eui64_linklayer_addr desteu64;
  struct lowpan6_context_info *ctxinfo = NULL;

  /* Concert the EUI48 bit to EUI 64 */
  /* Source MAC address can't be a MULTICAST address */
  if ((src->addrlen == IEEE_EUI48_ADDR_SIZE) &&
      ((link_layer_type == PLC_DRIVER_IF) || (link_layer_type == WIFI_DRIVER_IF))) {
    lowpan6_eui48_2_eui64(src, &srceu64);
  } else if ((src->addrlen == IEEE_EUI64_ADDR_SIZE) &&
             (NETIF_MAX_HWADDR_LEN == IEEE_EUI64_ADDR_SIZE)) {
    srceu64 = *((struct eui64_linklayer_addr *)src);
  } else {
    LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_decompress: Not PLC driver or Invalid source MAC address \n"));
    (void)pbuf_free(p);
    return NULL;
  }

  if ((dest->addrlen == IEEE_EUI48_ADDR_SIZE) &&
      ((link_layer_type == PLC_DRIVER_IF) || (link_layer_type == WIFI_DRIVER_IF))) {
    lowpan6_eui48_2_eui64(dest, &desteu64);
  } else if ((dest->addrlen == IEEE_EUI64_ADDR_SIZE) &&
             (NETIF_MAX_HWADDR_LEN == IEEE_EUI64_ADDR_SIZE)) {
    desteu64 = *((struct eui64_linklayer_addr *)dest);
  } else {
    LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_decompress: Not PLC driver or Invalid dest MAC address \n"));
    (void)pbuf_free(p);
    return NULL;
  }

  ip6buffsize = p->len + IP6_HLEN + UDP_HLEN;
#if ETH_6LOWPAN
  q = pbuf_alloc(PBUF_LINK, ip6buffsize, PBUF_RAM);
#else
  q = pbuf_alloc(PBUF_IP, ip6buffsize, PBUF_POOL);
#endif
  if (q == NULL) {
    (void)pbuf_free(p);
    return NULL;
  }
#if LWIP_RPL || LWIP_RIPPLE
#if LWIP_USE_L2_METRICS
  PBUF_SET_RSSI(q, PBUF_GET_RSSI(p));
  PBUF_SET_LQI(q, PBUF_GET_LQI(p));
#endif
#endif
  lowpan6_buffer = (u8_t *)p->payload;

  ip6hdr = (struct ip6_hdr *)q->payload;

  len2decomp = p->len;

  SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 2);
  lowpan6_offset = 2;
  SICSLOWPAN_REDUCE_LEN(len2decomp, 2);

  if (lowpan6_buffer[1] & 0x80) {
    SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 1);
    lowpan6_offset++;
    SICSLOWPAN_REDUCE_LEN(len2decomp, 1);
  }

  /* Set IPv6 version, traffic class and flow label. */
  if ((lowpan6_buffer[0] & 0x18) == 0x00) {
    SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 4);
    IP6H_VTCFL_SET(ip6hdr, 6, lowpan6_buffer[lowpan6_offset],
                   ((lowpan6_buffer[lowpan6_offset + 1] & 0x0f) << 16) | (lowpan6_buffer[lowpan6_offset + 2] << 8) |
                   lowpan6_buffer[lowpan6_offset + 3]);
    lowpan6_offset += 4;
    SICSLOWPAN_REDUCE_LEN(len2decomp, 4);
  } else if ((lowpan6_buffer[0] & 0x18) == 0x08) {
    SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 3);
    IP6H_VTCFL_SET(ip6hdr, 6, lowpan6_buffer[lowpan6_offset] & 0xc0,
                   ((lowpan6_buffer[lowpan6_offset] & 0x0f) << 16) | (lowpan6_buffer[lowpan6_offset + 1] << 8) |
                   lowpan6_buffer[lowpan6_offset + 2]);
    lowpan6_offset += 3;
    SICSLOWPAN_REDUCE_LEN(len2decomp, 3);
  } else if ((lowpan6_buffer[0] & 0x18) == 0x10) {
    SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 1);
    IP6H_VTCFL_SET(ip6hdr, 6, lowpan6_buffer[lowpan6_offset], 0);
    lowpan6_offset += 1;
    SICSLOWPAN_REDUCE_LEN(len2decomp, 1);
  } else if ((lowpan6_buffer[0] & 0x18) == 0x18) {
    IP6H_VTCFL_SET(ip6hdr, 6, 0, 0);
  }

  /* Set Next Header */
  if ((lowpan6_buffer[0] & 0x04) == 0x00) {
    SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 1);
    IP6H_NEXTH_SET(ip6hdr, lowpan6_buffer[lowpan6_offset++]);
    SICSLOWPAN_REDUCE_LEN(len2decomp, 1);
  } else {
    /* We should fill this later with NHC decoding */
    IP6H_NEXTH_SET(ip6hdr, 0);
  }

  /* Set Hop Limit */
  if ((lowpan6_buffer[0] & 0x03) == 0x00) {
    SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 1);
    IP6H_HOPLIM_SET(ip6hdr, lowpan6_buffer[lowpan6_offset++]);
    SICSLOWPAN_REDUCE_LEN(len2decomp, 1);
  } else if ((lowpan6_buffer[0] & 0x03) == 0x01) {
    IP6H_HOPLIM_SET(ip6hdr, 1);
  } else if ((lowpan6_buffer[0] & 0x03) == 0x02) {
    IP6H_HOPLIM_SET(ip6hdr, 64);
  } else if ((lowpan6_buffer[0] & 0x03) == 0x03) {
    IP6H_HOPLIM_SET(ip6hdr, 255);
  }

  /* Source address decoding. */
  if ((lowpan6_buffer[1] & 0x40) == 0x00) {
    /* Stateless compression */
    if ((lowpan6_buffer[1] & 0x30) == 0x00) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 16);
      /* copy full address */
      if (memcpy_s(&ip6hdr->src.addr[0],
                   (ip6buffsize - LOWPAN_IPv6_SRCADDR_OFFSET),
                   lowpan6_buffer + lowpan6_offset, 16) != EOK) {
        (void)pbuf_free(p);
        (void)pbuf_free(q);
        return NULL;
      }
      lowpan6_offset += 16;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 16);
    } else if ((lowpan6_buffer[1] & 0x30) == 0x10) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 8);
      ip6hdr->src.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->src.addr[1] = 0;
      if (memcpy_s(&ip6hdr->src.addr[2], (ip6buffsize - LOWPAN_IPv6_SRCADDR_OFFSET),
                   lowpan6_buffer + lowpan6_offset, 8) != EOK) {
        (void)pbuf_free(p);
        (void)pbuf_free(q);
        return NULL;
      }
      lowpan6_offset += 8;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 8);
    } else if ((lowpan6_buffer[1] & 0x30) == 0x20) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 2);
      ip6hdr->src.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->src.addr[1] = 0;
      ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
      ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL | (lowpan6_buffer[lowpan6_offset] << 8) |
                                       lowpan6_buffer[lowpan6_offset + 1]);
      lowpan6_offset += 2;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 2);
    } else if ((lowpan6_buffer[1] & 0x30) == 0x30) {
      ip6hdr->src.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->src.addr[1] = 0;
      if (srceu64.addrlen == 2) {
        ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
        ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL | (src->addr[0] << 8) | src->addr[1]);
      } else {
        ip6hdr->src.addr[2] = lwip_htonl(((srceu64.addr[0] ^ 2) << 24) |
                                         (srceu64.addr[1] << 16) | (srceu64.addr[2] << 8) | srceu64.addr[3]);
        ip6hdr->src.addr[3] = lwip_htonl((srceu64.addr[4] << 24) | (srceu64.addr[5] << 16) |
                                         (srceu64.addr[6] << 8) | srceu64.addr[7]);
      }
    }
  } else {
    /* Stateful compression */
    if ((lowpan6_buffer[1] & 0x30) == 0x00) {
      /* ANY address */
      ip6hdr->src.addr[0] = 0;
      ip6hdr->src.addr[1] = 0;
      ip6hdr->src.addr[2] = 0;
      ip6hdr->src.addr[3] = 0;
    } else {
      /* Set prefix from context info */
      if (lowpan6_buffer[1] & 0x80) {
        /* This access we are already validated at the begining */
        i = (lowpan6_buffer[2] >> 4) & 0x0f;
      } else {
        i = 0;
      }

      ctxinfo = lowpan6_get_context_byid(i);
      if (ctxinfo == NULL) {
        LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_decompress: invalid src context ID=%d \n", i));
        (void)pbuf_free(p);
        (void)pbuf_free(q);
        return NULL;
      }

      ip6hdr->src.addr[0] = ctxinfo->context_prefix.addr[0];
      ip6hdr->src.addr[1] = ctxinfo->context_prefix.addr[1];
    }

    if ((lowpan6_buffer[1] & 0x30) == 0x10) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 8);
      if (memcpy_s(&ip6hdr->src.addr[2], (ip6buffsize - LOWPAN_IPv6_SRCADDR_OFFSET),
                   lowpan6_buffer + lowpan6_offset, 8) != EOK) {
        (void)pbuf_free(p);
        (void)pbuf_free(q);
        return NULL;
      }
      lowpan6_offset += 8;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 8);
    } else if ((lowpan6_buffer[1] & 0x30) == 0x20) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 2);
      ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
      ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL |
                                       (lowpan6_buffer[lowpan6_offset] << 8) |
                                       lowpan6_buffer[lowpan6_offset + 1]);
      lowpan6_offset += 2;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 2);
    } else if ((lowpan6_buffer[1] & 0x30) == 0x30) {
      if (srceu64.addrlen == 2) {
        ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
        ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL | (srceu64.addr[0] << 8) | srceu64.addr[1]);
      } else {
        ip6hdr->src.addr[2] = lwip_htonl(((srceu64.addr[0] ^ 2) << 24) |
                                         (srceu64.addr[1] << 16) | (srceu64.addr[2] << 8) | srceu64.addr[3]);
        ip6hdr->src.addr[3] = lwip_htonl((srceu64.addr[4] << 24) | (srceu64.addr[5] << 16) |
                                         (srceu64.addr[6] << 8) | srceu64.addr[7]);
      }
    }
  }

  /* Destination address decoding. */
  if (lowpan6_buffer[1] & 0x08) {
    /* Multicast destination */
    if (lowpan6_buffer[1] & 0x04) {
      /* @todo support stateful multicast addressing */
      LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_decompress: Stateful multicast addressing not supported \n"));
      (void)pbuf_free(p);
      (void)pbuf_free(q);
      return NULL;
    }

    if ((lowpan6_buffer[1] & 0x03) == 0x00) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 16);
      /* copy full address */
      if (memcpy_s(&ip6hdr->dest.addr[0], (ip6buffsize - LOWPAN_IPv6_DSTADDR_OFFSET),
                   lowpan6_buffer + lowpan6_offset, 16) != EOK) {
        (void)pbuf_free(p);
        (void)pbuf_free(q);
        return NULL;
      }
      lowpan6_offset += 16;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 16);
    } else if ((lowpan6_buffer[1] & 0x03) == 0x01) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 4);
      ip6hdr->dest.addr[0] = lwip_htonl(0xff000000UL | (lowpan6_buffer[lowpan6_offset++] << 16));
      ip6hdr->dest.addr[1] = 0;
      ip6hdr->dest.addr[2] = lwip_htonl(lowpan6_buffer[lowpan6_offset++]);
      ip6hdr->dest.addr[3] = lwip_htonl((lowpan6_buffer[lowpan6_offset] << 24) |
                                        (lowpan6_buffer[lowpan6_offset + 1] << 16) |
                                        (lowpan6_buffer[lowpan6_offset + 2] << 8) |
                                         lowpan6_buffer[lowpan6_offset + 3]);
      lowpan6_offset += 4;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 4);
    } else if ((lowpan6_buffer[1] & 0x03) == 0x02) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 3);
      ip6hdr->dest.addr[0] = lwip_htonl(0xff000000UL | (lowpan6_buffer[lowpan6_offset++] << 16));
      ip6hdr->dest.addr[1] = 0;
      ip6hdr->dest.addr[2] = 0;
      ip6hdr->dest.addr[3] = lwip_htonl((lowpan6_buffer[lowpan6_offset] << 16) |
                                        (lowpan6_buffer[lowpan6_offset + 1] << 8) |
                                         lowpan6_buffer[lowpan6_offset + 2]);
      lowpan6_offset += 3;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 3);
    } else if ((lowpan6_buffer[1] & 0x03) == 0x03) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 1);
      ip6hdr->dest.addr[0] = PP_HTONL(0xff020000UL);
      ip6hdr->dest.addr[1] = 0;
      ip6hdr->dest.addr[2] = 0;
      ip6hdr->dest.addr[3] = lwip_htonl(lowpan6_buffer[lowpan6_offset++]);
      SICSLOWPAN_REDUCE_LEN(len2decomp, 1);
    }

  } else {
    if (lowpan6_buffer[1] & 0x04) {
      /* Stateful destination compression */
      /* Set prefix from context info */
      if (lowpan6_buffer[1] & 0x80) {
        i = lowpan6_buffer[2] & 0x0f;
      } else {
        i = 0;
      }

      ctxinfo = lowpan6_get_context_byid(i);
      if (ctxinfo == NULL) {
        LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_decompress: invalid src context ID=%d \n", i));
        (void)pbuf_free(p);
        (void)pbuf_free(q);
        return NULL;
      }

      ip6hdr->dest.addr[0] = ctxinfo->context_prefix.addr[0];
      ip6hdr->dest.addr[1] = ctxinfo->context_prefix.addr[1];
    } else {
      /* Link local address compression */
      ip6hdr->dest.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->dest.addr[1] = 0;
    }

    if ((lowpan6_buffer[1] & 0x03) == 0x00) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 16);
      /* copy full address */
      if (memcpy_s(&ip6hdr->dest.addr[0], (ip6buffsize - LOWPAN_IPv6_DSTADDR_OFFSET),
                   lowpan6_buffer + lowpan6_offset, 16) != EOK) {
        (void)pbuf_free(q);
        (void)pbuf_free(p);
        return NULL;
      }
      lowpan6_offset += 16;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 16);
    } else if ((lowpan6_buffer[1] & 0x03) == 0x01) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 8);
      if (memcpy_s(&ip6hdr->dest.addr[2], (ip6buffsize - LOWPAN_IPv6_DSTADDR_OFFSET),
                   lowpan6_buffer + lowpan6_offset, 8) != EOK) {
        (void)pbuf_free(q);
        (void)pbuf_free(p);
        return NULL;
      }
      lowpan6_offset += 8;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 8);
    } else if ((lowpan6_buffer[1] & 0x03) == 0x02) {
      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 2);
      ip6hdr->dest.addr[2] = PP_HTONL(0x000000ffUL);
      ip6hdr->dest.addr[3] = lwip_htonl(0xfe000000UL |
                                        (lowpan6_buffer[lowpan6_offset] << 8) |
                                         lowpan6_buffer[lowpan6_offset + 1]);
      lowpan6_offset += 2;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 2);
    } else if ((lowpan6_buffer[1] & 0x03) == 0x03) {
      if (desteu64.addrlen == 2) {
        ip6hdr->dest.addr[2] = PP_HTONL(0x000000ffUL);
        ip6hdr->dest.addr[3] = lwip_htonl(0xfe000000UL | (desteu64.addr[0] << 8) |
                                          desteu64.addr[1]);
      } else {
        ip6hdr->dest.addr[2] = lwip_htonl(((desteu64.addr[0] ^ 2) << 24) |
                                          (desteu64.addr[1] << 16) | (desteu64.addr[2] << 8) | desteu64.addr[3]);
        ip6hdr->dest.addr[3] = lwip_htonl((desteu64.addr[4] << 24) | (desteu64.addr[5] << 16) |
                                          (desteu64.addr[6] << 8) | desteu64.addr[7]);
      }
    }
  }

  /* Next Header Compression (NHC) decoding? */
  if (lowpan6_buffer[0] & 0x04) {
    SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 1);
    if ((lowpan6_buffer[lowpan6_offset] & 0xf8) == 0xf0) {
      struct udp_hdr *udphdr = NULL;

      /* UDP compression */
      IP6H_NEXTH_SET(ip6hdr, IP6_NEXTH_UDP);
      udphdr = (struct udp_hdr *)((u8_t *)q->payload + ip6_offset);

      if (lowpan6_buffer[lowpan6_offset] & 0x04) {
        /* @todo support checksum decompress */
        LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_decompress: chksum decompress not supported \n"));
        (void)pbuf_free(p);
        (void)pbuf_free(q);
        return NULL;
      }

      /* Decompress ports */
      i = lowpan6_buffer[lowpan6_offset++] & 0x03;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 1);
      if (i == 0) {
        SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 4);
        udphdr->src = lwip_htons(lowpan6_buffer[lowpan6_offset] << 8 | lowpan6_buffer[lowpan6_offset + 1]);
        udphdr->dest = lwip_htons(lowpan6_buffer[lowpan6_offset + 2] << 8 | lowpan6_buffer[lowpan6_offset + 3]);
        lowpan6_offset += 4;
        SICSLOWPAN_REDUCE_LEN(len2decomp, 4);
      } else if (i == 0x01) {
        SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 3);
        udphdr->src = lwip_htons(lowpan6_buffer[lowpan6_offset] << 8 | lowpan6_buffer[lowpan6_offset + 1]);
        udphdr->dest = lwip_htons(0xf000 | lowpan6_buffer[lowpan6_offset + 2]);
        lowpan6_offset += 3;
        SICSLOWPAN_REDUCE_LEN(len2decomp, 3);
      } else if (i == 0x02) {
        SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 3);
        udphdr->src = lwip_htons(0xf000 | lowpan6_buffer[lowpan6_offset]);
        udphdr->dest = lwip_htons(lowpan6_buffer[lowpan6_offset + 1] << 8 | lowpan6_buffer[lowpan6_offset + 2]);
        lowpan6_offset += 3;
        SICSLOWPAN_REDUCE_LEN(len2decomp, 3);
      } else if (i == 0x03) {
        SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 1);
        udphdr->src = lwip_htons(0xf0b0 | ((lowpan6_buffer[lowpan6_offset] >> 4) & 0x0f));
        udphdr->dest = lwip_htons(0xf0b0 | (lowpan6_buffer[lowpan6_offset] & 0x0f));
        lowpan6_offset += 1;
        SICSLOWPAN_REDUCE_LEN(len2decomp, 1);
      }

      SICSLOWPAN_VALIDATE_LENGTH(len2decomp, 2);
      udphdr->chksum = lwip_htons(lowpan6_buffer[lowpan6_offset] << 8 | lowpan6_buffer[lowpan6_offset + 1]);
      lowpan6_offset += 2;
      SICSLOWPAN_REDUCE_LEN(len2decomp, 2);

      ip6_offset += UDP_HLEN;

      if (datagram_size == 0) {
        datagram_size = p->tot_len - lowpan6_offset + ip6_offset;
      }

      udphdr->len = lwip_htons(datagram_size - IP6_HLEN);
    } else {
      /* @todo support NHC other than UDP */
      LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_decompress: Unsupported msg \n"));
      (void)pbuf_free(p);
      (void)pbuf_free(q);
      return NULL;
    }
  }

  if (datagram_size == 0) {
    datagram_size = p->tot_len - lowpan6_offset + ip6_offset;
  }
  /* Now we copy leftover contents from p to q, so we have all L2 and L3 headers (and L4?) in a single PBUF.
  * Replace p with q, and free p */
  (void)pbuf_remove_header(p, lowpan6_offset);

  /* Chek we have bytes remaining in the payload to copy */
  if (p->len > 0) {
    if (memcpy_s((u8_t *)q->payload + ip6_offset, (ip6buffsize - ip6_offset), p->payload, p->len) != EOK) {
      (void)pbuf_free(p);
      (void)pbuf_free(q);
      return NULL;
    }
  }

  q->tot_len = ip6_offset + p->len;
  q->len = q->tot_len;
  if (p->next != NULL) {
    pbuf_cat(q, p->next);
  }

  p->next = NULL;

#if LWIP_MAC_SECURITY
  q->flags = p->flags;
  LWIP_DEBUGF(LOWPAN6_DEBUG, ("decompressed [%x]\n", q->flags));
#endif

  (void)pbuf_free(p);

  /* Infer IPv6 payload length for header */
  IP6H_PLEN_SET(ip6hdr, datagram_size - (u16_t)IP6_HLEN);

  /* all done */
  return q;
}

err_t
lowpan6_input(struct pbuf *p, struct netif *netif, const struct linklayer_addr *sendermac,
              const struct linklayer_addr *recvrmac)
{
  u8_t *puc = NULL;
  s8_t i;
  u16_t datagram_size = 0;
  u16_t datagram_offset;
  u16_t datagram_tag;
  struct lowpan6_reass_helper *lrh = NULL;
  struct lowpan6_reass_helper *lrh_temp = NULL;

  LWIP_DEBUGF(LOWPAN6_DEBUG | LWIP_DBG_TRACE, ("%s in!!\n", __FUNCTION__));
  MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);

  LWIP_DEBUGF(LOWPAN6_DEBUG, ("Lowpan buf flags[%x]\n", p->flags));

  /* Check dispatch. */
  puc = (u8_t *)p->payload;

  if ((*puc & 0xf8) == 0xc0) {
    /* FRAG1 dispatch. add this packet to reassembly list. */
    LOWPAN6_STATS_INC(lowpan6.frag_recvd);
    LOWPAN6_STATS_INC(lowpan6.pkt_recvd);

    /* We are checking additional 1 bit to handle the non-compressed IPv6 packets */
    if (p->len < LOWPAN_FRAG1_HDR_LEN + 1) {
      (void)pbuf_free(p);
      LOWPAN6_STATS_INC(lowpan6.corrupt_pkt);
      return ERR_OK;
    }

    datagram_size = ((u16_t)(puc[0] & 0x07) << 8) | (u16_t)puc[1];
    datagram_tag = ((u16_t)puc[2] << 8) | (u16_t)puc[3];

    /* check for duplicate */
    lrh = reass_list;
    while (lrh != NULL) {
      if ((lrh->sender_addr.addrlen == sendermac->addrlen) &&
          (memcmp(lrh->sender_addr.addr, sendermac->addr, sendermac->addrlen) == 0)) {
        /* address match with packet in reassembly. */
        if ((datagram_tag == lrh->datagram_tag) && (datagram_size == lrh->datagram_size)) {
          MIB2_STATS_NETIF_INC(netif, ifindiscards);
          LOWPAN6_STATS_INC(lowpan6.dup_frag);
          /* duplicate fragment. */
          (void)pbuf_free(p);
          return ERR_OK;
        } else {
          /* We are receiving the start of a new datagram. Discard old one (incomplete). */
          lrh_temp = lrh->next_packet;
          (void)dequeue_datagram(lrh);
          (void)pbuf_free(lrh->pbuf);
          mem_free(lrh);
          LOWPAN6_STATS_INC(lowpan6.discard_pkt);
          LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_input: Received first frag of "
                                      "new packet discarding the old fragments from same source \n"));

          /* Check next datagram in queue. */
          lrh = lrh_temp;
        }
      } else {
        /* Check next datagram in queue. */
        lrh = lrh->next_packet;
      }
    }

    (void)pbuf_remove_header(p, 4); /* hide frag1 dispatch */

    lrh = (struct lowpan6_reass_helper *)mem_malloc(sizeof(struct lowpan6_reass_helper));
    if (lrh == NULL) {
      MIB2_STATS_NETIF_INC(netif, ifindiscards);
      (void)pbuf_free(p);
      return ERR_MEM;
    }

    lrh->sender_addr.addrlen = sendermac->addrlen;
    for (i = 0; i < sendermac->addrlen; i++) {
      lrh->sender_addr.addr[i] = sendermac->addr[i];
    }
    lrh->datagram_size = datagram_size;
    lrh->datagram_tag = datagram_tag;
    lrh->netifinfo = netif;

    /* Decompress the first fragment */
    if (*(u8_t *)p->payload == 0x41) {
      /* This is a complete IPv6 packet, just skip dispatch byte. */
      (void)pbuf_header(p, -1); /* hide dispatch byte. */
      lrh->pbuf = p;
    } else if ((*(u8_t *)p->payload & 0xe0) == 0x60) {
      lrh->pbuf = lowpan6_decompress(p, datagram_size, (struct linklayer_addr *)sendermac,
                                     (struct linklayer_addr *)recvrmac, netif->link_layer_type);
      if (lrh->pbuf == NULL) {
        /* decompression failed */
        mem_free(lrh);
        MIB2_STATS_NETIF_INC(netif, ifindiscards);
        LOWPAN6_STATS_INC(lowpan6.decomp_fail);
        return ERR_OK;
      }
    }

    lrh->next_packet = reass_list;
    lrh->timer = LOWPAN6_REASS_TIMEOUT;
    reass_list = lrh;

    return ERR_OK;
  } else if ((*puc & 0xf8) == 0xe0) {
    /* FRAGN dispatch, find packet being reassembled. */
    LOWPAN6_STATS_INC(lowpan6.frag_recvd);

    if (p->len < LOWPAN_FRAGN_HDR_LEN) {
      (void)pbuf_free(p);
      LOWPAN6_STATS_INC(lowpan6.corrupt_pkt);
      return ERR_OK;
    }

    datagram_size = ((u16_t)(puc[0] & 0x07) << 8) | (u16_t)puc[1];
    datagram_tag = ((u16_t)puc[2] << 8) | (u16_t)puc[3];
    datagram_offset = (u16_t)puc[4] << 3;
    (void)pbuf_remove_header(p, 5); /* hide frag1 dispatch */

    for (lrh = reass_list; lrh != NULL; lrh = lrh->next_packet) {
      if ((lrh->sender_addr.addrlen == sendermac->addrlen) &&
          (memcmp(lrh->sender_addr.addr, sendermac->addr, sendermac->addrlen) == 0) &&
          (datagram_tag == lrh->datagram_tag) &&
          (datagram_size == lrh->datagram_size)) {
        break;
      }
    }

    if (lrh == NULL) {
      /* rogue fragment */
      LWIP_DEBUGF(LOWPAN6_DEBUG, ("lowpan6_input: Received a frag "
                                  "of a packet without receiving the first fragment\n"));
      MIB2_STATS_NETIF_INC(netif, ifindiscards);
      LOWPAN6_STATS_INC(lowpan6.rogue_frag);
      (void)pbuf_free(p);
      return ERR_OK;
    }

    if (lrh->pbuf->tot_len > datagram_offset) {
      /* duplicate, ignore. */
      LOWPAN6_STATS_INC(lowpan6.dup_frag);
      (void)pbuf_free(p);
      return ERR_OK;
    } else if (lrh->pbuf->tot_len < datagram_offset) {
      MIB2_STATS_NETIF_INC(netif, ifindiscards);
      LOWPAN6_STATS_INC(lowpan6.oo_frag);
      /* We have missed a fragment. Delete whole reassembly. */
      (void)dequeue_datagram(lrh);
      (void)pbuf_free(lrh->pbuf);
      mem_free(lrh);
      (void)pbuf_free(p);
      return ERR_OK;
    }

    pbuf_cat(lrh->pbuf, p);
    p = NULL;

    /* is packet now complete? */
    if (lrh->pbuf->tot_len >= lrh->datagram_size) {
      /* dequeue from reass list. */
      (void)dequeue_datagram(lrh);

      /* get pbuf */
      p = lrh->pbuf;

      /* release helper */
      mem_free(lrh);
    } else {
      return ERR_OK;
    }
  } else {
    if (p->len < 1) {
      (void)pbuf_free(p);
      LOWPAN6_STATS_INC(lowpan6.corrupt_pkt);
      return ERR_OK;
    }

    puc = (u8_t *)p->payload;
    if (*puc == 0x41) {
      /* This is a complete IPv6 packet, just skip dispatch byte. */
      LOWPAN6_STATS_INC(lowpan6.pkt_recvd);
      (void)pbuf_remove_header(p, 1); /* hide dispatch byte. */
    } else if ((*puc & 0xe0) == 0x60) {
      LOWPAN6_STATS_INC(lowpan6.pkt_recvd);
      /* IPv6 headers are compressed using IPHC. */
      p = lowpan6_decompress(p, datagram_size, (struct linklayer_addr *)sendermac,
                             (struct linklayer_addr *)recvrmac, netif->link_layer_type);
      if (p == NULL) {
        MIB2_STATS_NETIF_INC(netif, ifindiscards);
        LOWPAN6_STATS_INC(lowpan6.decomp_fail);
        return ERR_OK;
      }
    } else {
      MIB2_STATS_NETIF_INC(netif, ifindiscards);
      LOWPAN6_STATS_INC(lowpan6.discard_pkt);
      (void)pbuf_free(p);
      return ERR_OK;
    }
  }

  if (p == NULL) {
    return ERR_OK;
  }

  /* We have a complete packet, check dispatch for headers. */
  MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
  LOWPAN6_STATS_INC(lowpan6.pkt_to_ip);

  LWIP_DEBUGF(LOWPAN6_DEBUG | LWIP_DBG_TRACE, ("IPv6 Packet of size[%u]"
                                               " flags[%x]", p->tot_len, p->flags));
#if LWIP_RPL || LWIP_RIPPLE
  if (memcpy_s(p->mac_address, sizeof(p->mac_address), sendermac->addr,
               sendermac->addrlen > sizeof(p->mac_address) ?
               sizeof(p->mac_address) : sendermac->addrlen) != EOK) {
    (void)pbuf_free(p);
    return ERR_MEM;
  }
#endif
  return ip6_input(p, netif);
}

err_t
lowpan6_if_init(struct netif *netif)
{
  if (netif == NULL) {
    return ERR_ARG;
  }
  netif->lowpan_output = lowpan6_output;

  MIB2_INIT_NETIF(netif, snmp_ifType_other, 0);

  /* maximum transfer unit for IPv6 */
  if (netif->link_layer_type == PLC_DRIVER_IF) {
    netif->mtu = LOWPAN6_IPV6_MAX_MTU_OVER_PLC;
  } else if (netif->link_layer_type == IEEE802154_DRIVER_IF) {
    netif->mtu = LOWPAN6_IPV6_MAX_MTU_OVER_IEE802154;
  }

  /* broadcast capability */
  netif->flags |= NETIF_FLAG_BROADCAST;

  return ERR_OK;
}
#endif /* LWIP_IPV6 && LWIP_6LOWPAN */
