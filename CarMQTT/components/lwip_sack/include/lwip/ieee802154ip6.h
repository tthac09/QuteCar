#ifndef LWIP_HDR_IEEE802154SIP6_H
#define LWIP_HDR_IEEE802154SIP6_H

#include "lwip/opt.h"

#if LWIP_IPV6 && LWIP_IEEE802154 /* don't build if not configured for use in lwipopts.h */

#include "lwip/pbuf.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

err_t output_ip6_onieee802154(struct netif *netif, struct pbuf *p,
                              const ip6_addr_t *ipaddr);

void ieee802154_packet_sent_ack_callback(unsigned char *p_dest_addr,
                                         unsigned int u32_addr_mode,
                                         int status,
                                         unsigned int numtx);
#ifdef __cplusplus
}
#endif

#endif /* LWIP_IPV6 && LWIP_ETHERNET */

#endif /* LWIP_HDR_IEEE802154SIP6_H */

