/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2016. All rights reserved.
 * Description: implementation of driverif ieee802.154 APIs
 * Author: none
 * Create: 2013-12-22
 */
#include "lwip/ieee802154ip6.h"
#include "netif/driverif_ieee802154.h"
#include "netif/driverif.h"
#include "netif/lowpan6.h"

#if LWIP_IPV6 && LWIP_IEEE802154

#define IPV6_MTU_OVER_IEEE802154 1280

err_t
ieee802154_output_fn(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
  (void)netif;
  (void)p;
  (void)ipaddr;

  return ERR_OPNOTSUPP;
}

/*
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this driverif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM on Allocation Failure
 *         any other err_t on error
 */
err_t
ieee802154driverif_init(struct netif *netif)
{
  u16_t link_layer_type;

  LWIP_ERROR("ieee802154driverif_init : invalid arg netif", (netif != NULL), return ERR_ARG);

  link_layer_type = netif->link_layer_type;

  LWIP_ERROR("ieee802154driverif_init : invalid link_layer_type in netif",
             (link_layer_type == IEEE802154_DRIVER_IF), return ERR_IF);

#if NETIF_USE_6BYTE_HWLEN_FOR_IEEE802154
  LWIP_ERROR("ieee802154driverif_init : netif hardware length is greater than maximum supported",
             (netif->hwaddr_len == NETIF_MAX_HWADDR_LEN), return ERR_IF);
#else
  LWIP_ERROR("ieee802154driverif_init : netif hardware length is greater than maximum supported",
             (netif->hwaddr_len == NETIF_802154_MAX_HWADDR_LEN), return ERR_IF);
#endif

  LWIP_ERROR("ieee802154driverif_init : drv_send is null", (netif->drv_lln_send != NULL), return ERR_IF);

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  (void)snprintf_s(netif->hostname, sizeof(netif->hostname), sizeof(netif->hostname) - 1, "lwip");
#endif /* LWIP_NETIF_HOSTNAME */

#if LWIP_IPV4
  netif->output = ieee802154_output_fn;
#endif

#if LWIP_IPV6
  netif->output_ip6 = output_ip6_onieee802154;
#if LWIP_6LOWPAN
  if (netif->enabled6lowpan == lwIP_TRUE) {
    lowpan6_if_init(netif);
  }
#endif /* LWIP_6LOWPAN */
#endif /* LWIP_IPV6 */

  (void)strncpy_s(netif->name, IFNAMSIZ, RF_IFNAME, sizeof(RF_IFNAME) - 1);

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  /* device capabilities */
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP
#if DRIVER_STATUS_CHECK
                  | NETIF_FLAG_DRIVER_RDY
#endif
                  ;

  LWIP_DEBUGF(DRIVERIF_DEBUG, ("plcdriverif_init : Initialized netif 0x%p\n", (void *)netif));
  return ERR_OK;
}
#endif
