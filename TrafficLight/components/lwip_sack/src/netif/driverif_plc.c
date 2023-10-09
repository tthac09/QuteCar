/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2016. All rights reserved.
 * Description: implementation of driverif PLC APIs
 * Author: none
 * Create: 2013-12-22
 */

#include "lwip/plcip6.h"
#include "netif/driverif_plc.h"
#include "netif/driverif.h"
#include "netif/lowpan6.h"

#if LWIP_PLC

#define IPV6_MTU_OVER_PLC 1280
#define IPV4_MTU_OVER_PLC 576

/**
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
plc_ipv4_output_fn(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
  (void)p;
  (void)(netif);
  (void)ipaddr;

  LWIP_DEBUGF(DRIVERIF_DEBUG, ("plcdriverif_init : IPV4 over PLC not supported now\n"));
  /* Called ARP to resove address and send the packet out etharp_output */
  return ERR_OK;
}

LWIP_STATIC err_t
plc_link_output(struct netif *netif, struct pbuf *p)
{
  (void)netif;
  (void)p;
  LWIP_DEBUGF(DRIVERIF_DEBUG, ("plcdriverif_init : IPV4 over PLC not supported now\n"));
  return ERR_OK;
}

err_t
plcdriverif_init(struct netif *netif)
{
  u16_t link_layer_type;

  LWIP_ERROR("plcdriverif_init : invalid arg netif", (netif != NULL), return ERR_ARG);

  link_layer_type = netif->link_layer_type;

  LWIP_ERROR("plcdriverif_init : invalid link_layer_type in netif",
             (link_layer_type == PLC_DRIVER_IF), return ERR_IF);

  LWIP_ERROR("plcdriverif_init : netif hardware length is greater than maximum supported",
             (netif->hwaddr_len <= NETIF_MAX_HWADDR_LEN), return ERR_IF);

  LWIP_ERROR("plcdriverif_init : drv_send is null", (netif->drv_lln_send != NULL), return ERR_IF);

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  (void)snprintf_s(netif->hostname, sizeof(netif->hostname), sizeof(netif->hostname) - 1, "lwip");
#endif /* LWIP_NETIF_HOSTNAME */

#if LWIP_IPV4
  netif->output = plc_ipv4_output_fn;
  netif->linkoutput = plc_link_output;
#endif

#if LWIP_IPV6
  netif->output_ip6 = output_ip6_onplc;

#if LWIP_6LOWPAN
  if (netif->enabled6lowpan == lwIP_TRUE) {
    lowpan6_if_init(netif);
  }
#endif /* LWIP_6LOWPAN */
#endif /* LWIP_IPV6 */

#if LWIP_MAC_SECURITY
  netif->flags |= NETIF_FLAG_MAC_SECURITY_SUPPORT;
#endif

  (void)strncpy_s(netif->name, IFNAMSIZ, PLC_IFNAME, sizeof(PLC_IFNAME) - 1);

  /* device capabilities */
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP
#if DRIVER_STATUS_CHECK
                  | NETIF_FLAG_DRIVER_RDY
#endif
                  ;
  LWIP_DEBUGF(DRIVERIF_DEBUG, ("plcdriverif_init : Initialized netif 0x%p\n", (void *)netif));
  return ERR_OK;
}
#endif /* LWIP_PLC */
