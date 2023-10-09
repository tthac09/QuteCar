/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: implementation for NAT64 dhcp proxy
 * Author: NA
 * Create: 2019
 */
#include "lwip/opt.h"
#include "lwip/dhcp.h"
#if LWIP_NAT64
#include "arch/sys_arch.h"
#include "lwip/nat64.h"
#include "lwip/nat64_v4_dhcpc.h"

err_t
nat64_dhcp_request_ip(struct netif *ntf, const linklayer_addr_t *lladdr)
{
  err_t ret;
#if LWIP_DHCP_SUBSTITUTE
  dhcp_num_t index = 0;
#endif
  if ((ntf == NULL) || (lladdr == NULL)) {
    return ERR_ARG;
  }

#if LWIP_DHCP_SUBSTITUTE
  ret = nat64_entry_mac_to_idx(lladdr->addr, lladdr->addrlen, &index);
  if (ret != ERR_OK) {
    return ERR_VAL;
  }

  ret = dhcp_substitute_start(ntf, index);
#else
  ret = ERR_ARG;
#endif

  return ret;
}

err_t
nat64_dhcp_stop(struct netif *ntf, const linklayer_addr_t *lladdr)
{
  err_t ret;
#if LWIP_DHCP_SUBSTITUTE
  dhcp_num_t index = 0;
#endif
  if ((ntf == NULL) || (lladdr == NULL)) {
    return ERR_ARG;
  }
#if LWIP_DHCP_SUBSTITUTE
  ret = nat64_entry_mac_to_idx(lladdr->addr, lladdr->addrlen, &index);
  if (ret != ERR_OK) {
    return ERR_VAL;
  }

  dhcp_substitute_stop(ntf, index);
  ret = ERR_OK;
#else
  ret = ERR_ARG;
#endif

  return ret;
}


void
nat64_dhcp_ip4_event(const u8_t *mac, u8_t maclen, const ip4_addr_t *ipaddr, int event)
{
  nat64_entry_t *entry = NULL;
  int ret;
  linklayer_addr_t llmac;
  if (mac == NULL) {
    return;
  }

  ret = memset_s(&llmac, sizeof(linklayer_addr_t), 0, sizeof(linklayer_addr_t));
  if (ret != EOK) {
    LWIP_DEBUGF(NAT64_DEBUG, ("%s:memset_s fail(%d)\n", __FUNCTION__, ret));
    return;
  }
  ret = memcpy_s(llmac.addr, sizeof(llmac.addr), mac, maclen);
  if (ret != EOK) {
    LWIP_DEBUGF(NAT64_DEBUG, ("%s:memcpy_s fail(%d)\n", __FUNCTION__, ret));
    return;
  }
  llmac.addrlen = maclen > sizeof(llmac.addr) ? sizeof(llmac.addr) : maclen;
  entry = nat64_entry_lookup_by_mac(&llmac);
  if (entry == NULL) {
    return;
  }

  if (((event == NAT64_DHCP_EVENT_OFFER) || (event == NAT64_DHCP_EVENT_RENEW)) &&
      (ipaddr != NULL)) {
    if ((entry->state == NAT64_STATE_DHCP_REQUEST) ||
        (entry->state == NAT64_STATE_ESTABLISH) ||
        (entry->state == NAT64_STATE_STALE)) {
#if !LWIP_NAT64_MIN_SUBSTITUTE
      entry->ip = *ipaddr;
#else
      (void)ipaddr;
#endif
      entry->state = NAT64_STATE_ESTABLISH;
      return;
    }
  }

  if (event == NAT64_DHCP_EVENT_RELEASE) {
    entry->state = NAT64_STATE_STALE;
  }
}
#endif
