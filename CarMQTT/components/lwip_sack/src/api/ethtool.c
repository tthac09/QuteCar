/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2016. All rights reserved.
 * Description: implementation of ethtool APIs
 * Author: none
 * Create: 2013-12-22
 */
#include "lwip/opt.h"

#if LWIP_NETIF_ETHTOOL /* don't build if not configured for use in lwipopts.h */
#include "lwip/ethtool.h"
static s32_t
ethtool_get_link(struct netif *netif, struct ethtool_value *edata)
{
  LWIP_ASSERT("netif != NULL", netif != NULL);
  LWIP_ASSERT("edata != NULL", edata != NULL);

  edata->cmd = ETHTOOL_GLINK;

  if ((netif->ethtool_ops == NULL) || (netif->ethtool_ops->get_link == NULL)) {
    edata->data = netif_is_up(netif) && netif_is_link_up(netif);
    return 0;
  }

  edata->data = netif_is_up(netif) && netif->ethtool_ops->get_link(netif);
  return 0;
}

static s32_t
ethtool_get_settings(struct netif *netif, struct ethtool_cmd *edata)
{
  LWIP_ASSERT("netif != NULL", netif != NULL);
  LWIP_ASSERT("edata != NULL", edata != NULL);

  edata->cmd = ETHTOOL_GSET;

  if ((netif->ethtool_ops == NULL) || (netif->ethtool_ops->get_settings == NULL)) {
    return EOPNOTSUPP;
  }

  return netif->ethtool_ops->get_settings(netif, edata);
}

static s32_t
ethtool_set_settings(struct netif *netif, struct ethtool_cmd *edata)
{
  LWIP_ASSERT("netif != NULL", netif != NULL);
  LWIP_ASSERT("edata != NULL", edata != NULL);

  edata->cmd = ETHTOOL_SSET;

  if ((netif->ethtool_ops == NULL) || (netif->ethtool_ops->set_settings == NULL)) {
    return EOPNOTSUPP;
  }

  return netif->ethtool_ops->set_settings(netif, edata);
}

s32_t
dev_ethtool(struct netif *netif, struct ifreq *ifr)
{
  u32_t ethcmd;
  s32_t retval;

  LWIP_ASSERT("netif != NULL", netif != NULL);
  LWIP_ASSERT("ifr != NULL", ifr != NULL);

  if ((netif->ethtool_ops != NULL) && (netif->ethtool_ops->begin != NULL)) {
    if (netif->ethtool_ops->begin(netif) < 0) {
      return EINVAL;
    }
  }

  ethcmd = *(u32_t *)ifr->ifr_data;

  switch (ethcmd) {
    case ETHTOOL_GLINK:
      retval = ethtool_get_link(netif, (struct ethtool_value *)ifr->ifr_data);
      break;
    case ETHTOOL_GSET:
      retval = ethtool_get_settings(netif, (struct ethtool_cmd *)ifr->ifr_data);
      break;
    case ETHTOOL_SSET:
      retval = ethtool_set_settings(netif, (struct ethtool_cmd *)ifr->ifr_data);
      break;
    default:
      retval = EOPNOTSUPP;
      break;
  }

  if ((netif->ethtool_ops != NULL) && (netif->ethtool_ops->complete != NULL)) {
    netif->ethtool_ops->complete(netif);
  }
  return retval;
}
#endif /* LWIP_NETIF_ETHTOOL */

