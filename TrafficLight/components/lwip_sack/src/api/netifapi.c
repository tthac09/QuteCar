/**
 * @file
 * Network Interface Sequential API module
 *
 * @defgroup netifapi NETIF API
 * @ingroup sequential_api
 * Thread-safe functions to be called from non-TCPIP threads
 *
 * @defgroup netifapi_netif NETIF related
 * @ingroup netifapi
 * To be called from non-TCPIP threads
 */

/*
 * Copyright (c) <2013-2015>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
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
 */

/**
 * @defgroup netifapi NETIF API
 * @ingroup threadsafe_api
 * Thread-safe functions to be called from non-TCPIP threads
 *
 * @defgroup netifapi_netif NETIF related
 * @ingroup netifapi
 * To be called from non-TCPIP threads
 */

#include "lwip/opt.h"

#if LWIP_NETIF_API /* don't build if not configured for use in lwipopts.h */

#include "lwip/etharp.h"
#include "lwip/netifapi.h"
#include "lwip/memp.h"
#if LWIP_IPV6_MLD && LWIP_NETIFAPI_MLD6
#include "lwip/mld6.h"
#endif
#include "lwip/nd6.h"
#include "lwip/priv/tcpip_priv.h"
#include "netif/lowpan6.h"

#define NETIFAPI_VAR_REF(name)      API_VAR_REF(name)
#define NETIFAPI_VAR_DECLARE(name)  API_VAR_DECLARE(struct netifapi_msg, name)
#define NETIFAPI_VAR_ALLOC(name)    API_VAR_ALLOC(struct netifapi_msg, MEMP_NETIFAPI_MSG, name, ERR_MEM)
#define NETIFAPI_VAR_FREE(name)     API_VAR_FREE(MEMP_NETIFAPI_MSG, name)

#if LWIP_DHCP_VENDOR_CLASS_IDENTIFIER
LWIP_STATIC err_t do_netifapi_set_vci(struct tcpip_api_call_data *m);
err_t netifapi_set_vci(char *vci, u8_t vci_len);
#if LWIP_DHCP_GET_VENDOR_CLASS_IDENTIFIER
LWIP_STATIC err_t do_netifapi_get_vci(struct tcpip_api_call_data *m);
err_t netifapi_get_vci(char *vci, u8_t *vci_len);
#endif
#endif /* LWIP_DHCP_VENDOR_CLASS_IDENTIFIER */

/*
 * Call netif_add() inside the tcpip_thread context.
 */
static err_t
netifapi_do_netif_add(struct tcpip_api_call_data *m)
{
  /* cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  if (!netif_add(msg->netif
#if LWIP_IPV4
                 , API_EXPR_REF(msg->msg.add.ipaddr),
                 API_EXPR_REF(msg->msg.add.netmask),
                 API_EXPR_REF(msg->msg.add.gw)
#endif /* LWIP_IPV4 */
                )) {
    return ERR_IF;
  } else {
    return ERR_OK;
  }
}

/*
 * Call the "errtfunc" (or the "voidfunc" if "errtfunc" is NULL) inside the
 * tcpip_thread context.
 */
static err_t
netifapi_do_netif_common(struct tcpip_api_call_data *m)
{
  /* cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  if (msg->msg.common.errtfunc != NULL) {
    return msg->msg.common.errtfunc(msg->netif);
  } else {
    msg->msg.common.voidfunc(msg->netif);
    return ERR_OK;
  }
}

/*
 * Call the "argfunc" inside the tcpip_thread context.
 */
static err_t
netifapi_do_netif_argcb(struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  return msg->msg.arg_cb.argfunc(msg->netif, msg->msg.arg_cb.arg);
}

#if LWIP_ARP && LWIP_IPV4
/**
 * @ingroup netifapi_arp
 * Add or update an entry in the ARP cache.
 * For an update, ipaddr is used to find the cache entry.
 *
 * @param ipaddr IPv4 address of cache entry
 * @param ethaddr hardware address mapped to ipaddr
 * @param type type of ARP cache entry
 * @return ERR_OK: entry added/updated, else error from err_t
 */
err_t
netifapi_arp_add(const ip4_addr_t *ipaddr, struct eth_addr *ethaddr, enum netifapi_arp_entry type)
{
  err_t err;

  /* We only support permanent entries currently */
  LWIP_UNUSED_ARG(type);

#if ETHARP_SUPPORT_STATIC_ENTRIES && LWIP_TCPIP_CORE_LOCKING
  LOCK_TCPIP_CORE();
  err = etharp_add_static_entry(ipaddr, ethaddr);
  UNLOCK_TCPIP_CORE();
#else
  /* @todo add new vars to struct netifapi_msg and create a 'do' func */
  LWIP_UNUSED_ARG(ipaddr);
  LWIP_UNUSED_ARG(ethaddr);
  err = ERR_VAL;
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES && LWIP_TCPIP_CORE_LOCKING */

  return err;
}

/**
 * @ingroup netifapi_arp
 * Remove an entry in the ARP cache identified by ipaddr
 *
 * @param ipaddr IPv4 address of cache entry
 * @param type type of ARP cache entry
 * @return ERR_OK: entry removed, else error from err_t
 */
err_t
netifapi_arp_remove(const ip4_addr_t *ipaddr, enum netifapi_arp_entry type)
{
  err_t err;

  /* We only support permanent entries currently */
  LWIP_UNUSED_ARG(type);

#if ETHARP_SUPPORT_STATIC_ENTRIES && LWIP_TCPIP_CORE_LOCKING
  LOCK_TCPIP_CORE();
  err = etharp_remove_static_entry(ipaddr);
  UNLOCK_TCPIP_CORE();
#else
  /* @todo add new vars to struct netifapi_msg and create a 'do' func */
  LWIP_UNUSED_ARG(ipaddr);
  err = ERR_VAL;
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES && LWIP_TCPIP_CORE_LOCKING */

  return err;
}
#endif /* LWIP_ARP && LWIP_IPV4 */

/*
 * @ingroup netifapi_netif
 * Call netif_add() in a thread-safe way by running that function inside the
 * tcpip_thread context.
 *
 * @note for params @see netif_add()
 */
err_t
netifapi_netif_add(struct netif *netif
#if LWIP_IPV4
                   , const ip4_addr_t *ipaddr, const ip4_addr_t *netmask, const ip4_addr_t *gw
#endif /* LWIP_IPV4 */
                  )
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

#if LWIP_IPV4
  if (ipaddr == NULL) {
    ipaddr = IP4_ADDR_ANY4;
  }
  if (netmask == NULL) {
    netmask = IP4_ADDR_ANY4;
  }
  if (gw == NULL) {
    gw = IP4_ADDR_ANY4;
  }
#endif /* LWIP_IPV4 */

  NETIFAPI_VAR_REF(msg).netif = netif;
#if LWIP_IPV4
  NETIFAPI_VAR_REF(msg).msg.add.ipaddr  = NETIFAPI_VAR_REF(ipaddr);
  NETIFAPI_VAR_REF(msg).msg.add.netmask = NETIFAPI_VAR_REF(netmask);
  NETIFAPI_VAR_REF(msg).msg.add.gw      = NETIFAPI_VAR_REF(gw);
#endif /* LWIP_IPV4 */
  err = tcpip_api_call(netifapi_do_netif_add, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

#ifdef LWIP_TESTBED
err_t
netifapi_netif_reset(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, netif_reset);
}
#endif

/*
 * Remove a network interface from the list of lwIP netifs.
 *
 * @param netif the network interface to remove
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 *  - ERR_NODEV: On failure due to No such netif
 */
err_t
netifapi_netif_remove(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, netif_remove);
}

#if LWIP_NETIFAPI_IF_HW
LWIP_STATIC err_t
netifapi_do_netif_set_hwaddr(struct tcpip_api_call_data *m)
{
  err_t ret;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  ret = netif_set_hwaddr(msg->netif, msg->msg.hw.hw_addr, msg->msg.hw.hw_len);
  return ret;
}

err_t
netifapi_netif_set_hwaddr(struct netif *netif, unsigned char *hw_addr, int hw_len)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.hw.hw_addr = hw_addr;
  NETIFAPI_VAR_REF(msg).msg.hw.hw_len = hw_len;
  err = tcpip_api_call(netifapi_do_netif_set_hwaddr, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

LWIP_STATIC err_t
netifapi_do_netif_get_hwaddr(struct tcpip_api_call_data *m)
{
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  netif_get_hwaddr(msg->netif, msg->msg.hw.hw_addr, msg->msg.hw.hw_len);
  return ERR_OK;
}

void
netifapi_netif_get_hwaddr(struct netif *netif, unsigned char *hw_addr, int hw_len)
{
  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.hw.hw_addr = hw_addr;
  NETIFAPI_VAR_REF(msg).msg.hw.hw_len = hw_len;
  (void)tcpip_api_call(netifapi_do_netif_get_hwaddr, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return;
}
#endif /* LWIP_NETIFAPI_IF_HW */

#if LWIP_IPV4
/*
 * Call netif_set_addr() inside the tcpip_thread context.
 */
static err_t
netifapi_do_netif_set_addr(struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  return netif_set_addr(msg->netif,
                        API_EXPR_REF(msg->msg.add.ipaddr),
                        API_EXPR_REF(msg->msg.add.netmask),
                        API_EXPR_REF(msg->msg.add.gw));
}

/*
 * @ingroup netifapi_netif
 * Call netif_set_addr() in a thread-safe way by running that function inside the
 * tcpip_thread context.
 *
 * @note for params @see netif_set_addr()
 */
err_t
netifapi_netif_set_addr(struct netif *netif,
                        const ip4_addr_t *ipaddr,
                        const ip4_addr_t *netmask,
                        const ip4_addr_t *gw)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

  if (ipaddr == NULL) {
    ipaddr = IP4_ADDR_ANY4;
  }
  if (netmask == NULL) {
    netmask = IP4_ADDR_ANY4;
  }
  if (gw == NULL) {
    gw = IP4_ADDR_ANY4;
  }

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.add.ipaddr  = NETIFAPI_VAR_REF(ipaddr);
  NETIFAPI_VAR_REF(msg).msg.add.netmask = NETIFAPI_VAR_REF(netmask);
  NETIFAPI_VAR_REF(msg).msg.add.gw      = NETIFAPI_VAR_REF(gw);
  err = tcpip_api_call(netifapi_do_netif_set_addr, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

#if LWIP_NETIFAPI_GET_ADDR
/*
 * Call netif_get_addr() inside the tcpip_thread context.
 */
static err_t
netifapi_do_netif_get_addr(struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  return netif_get_addr(msg->netif,
                        msg->msg.add_get.ipaddr,
                        msg->msg.add_get.netmask,
                        msg->msg.add_get.gw);
}

/*
 * @ingroup netifapi_netif
 * Call netif_get_addr() in a thread-safe way by running that function inside the
 * tcpip_thread context.
 *
 * @note for params @see netif_get_addr()
 */
err_t
netifapi_netif_get_addr(struct netif *netif,
                        ip4_addr_t *ipaddr,
                        ip4_addr_t *netmask,
                        ip4_addr_t *gw)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.add_get.ipaddr  = ipaddr;
  NETIFAPI_VAR_REF(msg).msg.add_get.netmask = netmask;
  NETIFAPI_VAR_REF(msg).msg.add_get.gw      = gw;
  err = tcpip_api_call(netifapi_do_netif_get_addr, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif /* LWIP_NETIFAPI_GET_ADDR */
#endif /* LWIP_IPV4 */

/**
 * call the "errtfunc" (or the "voidfunc" if "errtfunc" is NULL) in a thread-safe
 * way by running that function inside the tcpip_thread context.
 *
 * @note use only for functions where there is only "netif" parameter.
 */
err_t
netifapi_netif_common(struct netif *netif, netifapi_void_fn voidfunc,
                      netifapi_errt_fn errtfunc)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_netif_common : invalid arguments", (!((voidfunc == NULL) && (errtfunc == NULL))),
             return ERR_VAL);

  NETIFAPI_VAR_ALLOC(msg);
  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.common.voidfunc = voidfunc;
  NETIFAPI_VAR_REF(msg).msg.common.errtfunc = errtfunc;
  err = tcpip_api_call(netifapi_do_netif_common, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

/*
 * Bring an interface up, available for processing
 * traffic.
 *
 * @note: Enabling DHCP on a down interface will make it come
 * up once configured.
 *
 * @param netif the network interface
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_netif_set_up(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, netif_set_up);
}

/*
 * Bring an interface down, disabling any traffic processing.
 *
 * @note: Enabling DHCP on a down interface will make it come
 * up once configured.
 *
 * @param netif the network interface
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_netif_set_down(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, netif_set_down);
}

static err_t
netif_find_call(struct netif *nif, void *data)
{
  (void)nif;
  name_for_netif_t *arg = (struct name_for_netif *)(data);
  arg->p_netif = netif_find(arg->name);
  return ERR_OK;
}

/*
 * Called from the caller when find interface
 *
 * @param name The network interface name
 *
 * @return
 *  - NULL: On fail
 *  - netif: On success
 */
struct netif *
netifapi_netif_find(const char *name)
{
  struct netif *t_netif = NULL;
  name_for_netif_t arg;
  if ((name == NULL) || (strlen(name) >= IFNAMSIZ) || (*name == '\0')) {
    return NULL;
  }
  (void)memset_s(&arg, sizeof(struct name_for_netif), 0x0, sizeof(struct name_for_netif));
  if (memcpy_s(arg.name, sizeof(arg.name), name, strlen(name)) != EOK) {
    return NULL;
  }
  (void)netifapi_call_argcb(netif_find_call, &arg);
  t_netif = arg.p_netif;
  return t_netif;
}

/*
 * Called by a driver when its link goes up
 *
 * @param netif The network interface
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_netif_set_link_up(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, netif_set_link_up);
}

/*
 * Called by a driver when its link goes down
 *
 * @param netif The network interface
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_netif_set_link_down(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, netif_set_link_down);
}

#if LWIP_DHCP
/*
 * Start DHCP negotiation for a network interface.
 *
 * If no DHCP client instance was attached to this interface,
 * a new client is created first. If a DHCP client instance
 * was already present, it restarts negotiation.
 *
 * @param netif The lwIP network interface
 *
 * @return lwIP error code
 * - ERR_OK - No error
 * - ERR_MEM - Out of memory
 */
err_t
netifapi_dhcp_start(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, dhcp_start);
}

/*
 * Remove the DHCP client from the interface.
 *
 * @param netif The network interface to stop DHCP on
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_dhcp_stop(struct netif *netif)
{
  return netifapi_netif_common(netif, dhcp_stop, NULL);
}

/*
 * Inform a DHCP server of our manual configuration.
 *
 * This informs DHCP servers of our fixed IP_add configuration
 * by sending an INFORM message. It does not involve DHCP address
 * configuration, it is just here to be nice to the network.
 *
 * @param netif The lwIP network interface
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_dhcp_inform(struct netif *netif)
{
  return netifapi_netif_common(netif, dhcp_inform, NULL);
}

/*
 * Renew an existing DHCP lease at the involved DHCP server.
 *
 * @param netif The lwIP network interface
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_dhcp_renew(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, dhcp_renew);
}

/*
 * Release a DHCP lease (usually called before @ref dhcp_stop).
 *
 * @param netif The lwIP network interface
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_dhcp_release(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, dhcp_release);
}

/*
 * Removes a struct dhcp from a netif.
 *
 * ATTENTION: Only use this when not using dhcp_set_struct() to allocate the
 * struct dhcp since the memory is passed back to the heap.
 *
 * @param netif the netif from which to remove the struct dhcp
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_dhcp_cleanup(struct netif *netif)
{
  return netifapi_netif_common(netif, dhcp_cleanup, NULL);
}

/*
 * Check DHCP negotiation is done for a network interface.
 *
 * @param netif The lwIP network interface
 *
 * @return
 * - ERR_OK - if DHCP is bound
 * - ERR_MEM - if DHCP bound is still progressing
 */
err_t
netifapi_dhcp_is_bound(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, dhcp_is_bound);
}

#if LWIP_API_RICH
/**
 * To call dhcp_set_struct() inside the tcpip_thread context.
 */
static err_t
netifapi_do_dhcp_set_struct(struct tcpip_api_call_data *m)
{
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  dhcp_set_struct(msg->netif, msg->msg.dhcp_struct.dhcp);
  return ERR_OK;
}

/*
 * Set a statically allocated struct dhcp to work with.
 * Using this prevents dhcp_start to allocate it using mem_malloc.
 *
 * @param netif the netif for which to set the struct dhcp
 * @param dhcp (uninitialised) dhcp struct allocated by the application
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_dhcp_set_struct(struct netif *netif, struct dhcp *dhcp)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  NETIFAPI_VAR_ALLOC(msg);
  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.dhcp_struct.dhcp = dhcp;
  err = tcpip_api_call(netifapi_do_dhcp_set_struct, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

/*
 * To remove the statically assigned dhcp structure
 *
 * @param netif the network interface
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_dhcp_remove_struct(struct netif *netif)
{
  return netifapi_netif_common(netif, dhcp_remove_struct, NULL);
}
#endif
#if LWIP_DHCP_SUBSTITUTE
err_t
netifapi_dhcp_clients_info_get(struct netif *netif, struct dhcp_clients_info **clis_info)
{
  return netifapi_netif_call_argcb(netif, dhcp_clients_info_get, (void *)clis_info);
}

err_t
netifapi_dhcp_clients_info_free(struct netif *netif, struct dhcp_clients_info **clis_info)
{
  return netifapi_netif_call_argcb(netif, dhcp_clients_info_free, (void *)clis_info);
}
#endif /* LWIP_DHCP_SUBSTITUTE */
#endif /* LWIP_DHCP */

/*
 * call the "argfunc" with argument arg in a thread-safe
 * way by running that function inside the tcpip_thread context.
 *
 * @note use only for functions where there is only "netif" parameter.
 */
err_t
netifapi_netif_call_argcb(struct netif *netif, netifapi_arg_fn argfunc, void *arg)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_netif_call_argcb : invalid arguments", ((netif != NULL)), return ERR_VAL);
  LWIP_ERROR("netifapi_netif_call_argcb : invalid arguments", ((argfunc != NULL)), return ERR_VAL);

  NETIFAPI_VAR_ALLOC(msg);
  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.argfunc = argfunc;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.arg = arg;
  err = tcpip_api_call(netifapi_do_netif_argcb, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

/*
 * call the "argfunc" with argument arg in a thread-safe
 * way by running that function inside the tcpip_thread context.
 */
err_t
netifapi_call_argcb(netifapi_arg_fn argfunc, void *arg)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_netif_call_argcb : invalid arguments", ((argfunc != NULL)), return ERR_VAL);

  NETIFAPI_VAR_ALLOC(msg);
  NETIFAPI_VAR_REF(msg).netif = NULL;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.argfunc = argfunc;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.arg = arg;
  err = tcpip_api_call(netifapi_do_netif_argcb, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

/*
 * Set a network interface as the default network interface
 * (used to output all packets for which no specific route is found)
 *
 * @param netif the default network interface
 *
 * @return
 *  - ERR_OK: On success
 *  - ERR_MEM: On failure due to memory
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_netif_set_default(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, netif_set_default);
}

#if LWIP_NETIFAPI_IF_GET_DEFAULT
static err_t
do_netifapi_netif_get_default(struct tcpip_api_call_data *m)
{
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  if ((netif_default != NULL) && netif_is_up(netif_default)) {
    msg->netif = netif_default;
  } else {
    msg->netif = NULL;
  }

  return ERR_OK;
}


/*
 * Get the default network interface
 *
 * @return
 *  - NULL if either the default netif was NOT exist or the default netif was down,
 *  - else return the default netif pointer
 */
struct netif *
netifapi_netif_get_default(void)
{
  struct netif *netif_def = NULL;
  NETIFAPI_VAR_DECLARE(msg);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = NULL;
  (void)tcpip_api_call(do_netifapi_netif_get_default, &API_VAR_REF(msg).call);
  netif_def = NETIFAPI_VAR_REF(msg).netif;
  NETIFAPI_VAR_FREE(msg);
  return netif_def;
}
#endif /* LWIP_NETIFAPI_IF_GET_DEFAULT */

LWIP_STATIC err_t
netif_create_ip6_linklocal_address_wrapper(struct netif *netif, void *arg)
{
  u32_t *tmp = (u32_t *)arg;
  u32_t from_mac_48bit = *tmp;
  return netif_create_ip6_linklocal_address(netif, (u8_t)from_mac_48bit);
}

/*
 * @ingroup netif_ip6
 * This function allows for the easy addition of a new IPv6 address to an interface.
 * It takes care of finding an empty slot and then sets the address tentative
 * (to make sure that all the subsequent processing happens).
 *
 * @param netif netif to add the address on
 * @param ip6addr address to add
 * @param chosen_idx if != NULL, the chosen IPv6 address index will be stored here
 */
err_t
netifapi_netif_add_ip6_linklocal_address(struct netif *netif, u8_t from_mac_48bit)
{
  err_t err;
  u32_t is_from_mac_48bit = from_mac_48bit;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_netif_add_ip6_linklocal_address : invalid arguments", ((netif != NULL)), return ERR_VAL);
  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.argfunc = netif_create_ip6_linklocal_address_wrapper;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.arg = &is_from_mac_48bit;
  err = tcpip_api_call(netifapi_do_netif_argcb, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

#if LWIP_NETIFAPI_IP6_ADDR
/*
 * @ingroup netif_ip6
 * This function allows for the easy addition of a new IPv6 address to an interface.
 * It takes care of finding an empty slot and then sets the address tentative
 * (to make sure that all the subsequent processing happens).
 *
 * @param netif netif to add the address on
 * @param ip6addr address to add
 * @param chosen_idx if != NULL, the chosen IPv6 address index will be stored here
 */
err_t
netifapi_netif_add_ip6_address(struct netif *netif, ip_addr_t *ipaddr)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_netif_add_ip6_address : invalid arguments", ((ipaddr != NULL)), return ERR_VAL);
  LWIP_ERROR("netifapi_netif_add_ip6_address : invalid arguments", ((netif != NULL)), return ERR_VAL);

  /* Do no allow multicast or any ip address or loopback address to be set as interface address */
  if (!IP_IS_V6(ipaddr) ||
      ip_addr_ismulticast(ipaddr) ||
      ip_addr_isany(ipaddr) ||
      ip_addr_isloopback(ipaddr)) {
    return ERR_VAL;
  }

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.argfunc = netif_do_add_ipv6_addr;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.arg = (void *)ipaddr;
  err = tcpip_api_call(netifapi_do_netif_argcb, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif /* LWIP_NETIFAPI_IP6_ADDR */

#if LWIP_NETIFAPI_IP6_ADDR || LWIP_ENABLE_BASIC_SHELL_CMD
/*
 * @ingroup netif_ip6
 * This function allows for the easy removal of a existing IPv6 address to an interface.
 *
 * @param netif netif to add the address on
 * @param ip6addr address to add
 * @param chosen_idx if != NULL, the chosen IPv6 address index will be removed from here
 */
void
netifapi_netif_rmv_ip6_address(struct netif *netif, ip_addr_t *ipaddr)
{
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_netif_add_ip6_address : invalid arguments", ((ipaddr != NULL)), return);
  LWIP_ERROR("netifapi_netif_add_ip6_address : invalid arguments", ((netif != NULL)), return);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.argfunc = netif_do_rmv_ipv6_addr;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.arg = (void *)ipaddr;
  (void)tcpip_api_call(netifapi_do_netif_argcb, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return;
}
#endif /* LWIP_NETIFAPI_IP6_ADDR */

#if LWIP_IPV6_AUTOCONFIG
err_t
netifapi_set_ip6_autoconfig_enabled(struct netif *netif)
{
  return netifapi_netif_common(netif, netif_set_ip6_autoconfig_enabled, NULL);
}


err_t
netifapi_set_ip6_autoconfig_disabled(struct netif *netif)
{
  return netifapi_netif_common(netif, netif_set_ip6_autoconfig_disabled, NULL);
}
#endif /* LWIP_IPV6_AUTOCONFIG */


#if LWIP_IPV6_MLD && LWIP_NETIFAPI_MLD6
err_t
netif_do_join_ip6_multicastgroup(struct netif *netif, void *arguments)
{
  ip6_addr_t *ipaddr = (ip6_addr_t *)arguments;
  err_t err;

  err = mld6_join_staticgroup_netif(netif, ipaddr);
  return err;
}


err_t
netifapi_netif_join_ip6_multicastgroup(struct netif *netif, ip6_addr_t *ip6addr)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_netif_add_ip6_address : invalid arguments", ((ip6addr != NULL)), return ERR_VAL);
  LWIP_ERROR("netifapi_netif_add_ip6_address : invalid arguments", ((netif != NULL)), return ERR_VAL);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.argfunc = netif_do_join_ip6_multicastgroup;
  NETIFAPI_VAR_REF(msg).msg.arg_cb.arg = (void *)ip6addr;
  err = tcpip_api_call(netifapi_do_netif_argcb, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

#endif

#if LWIP_NETIF_LINK_CALLBACK && LWIP_NETIFAPI_LINK_CALLBACK
static err_t
netifapi_do_netif_set_link_callback(struct tcpip_api_call_data *m)
{
  /* cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  err_t err;

  err = netif_set_link_callback(msg->netif, msg->msg.netif_link_cb.link_callback);
  return err;
}

err_t
netifapi_netif_set_link_callback(struct netif *netif, netif_status_callback_fn link_callback)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_netif_set_link_callback : invalid arguments", (netif != NULL), return ERR_VAL);
  NETIFAPI_VAR_ALLOC(msg);

  (void)memset_s(&msg, sizeof(struct netifapi_msg), 0, sizeof(struct netifapi_msg));
  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.netif_link_cb.link_callback = link_callback;
  err = tcpip_api_call(netifapi_do_netif_set_link_callback, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif

#if LWIP_NETIF_EXT_STATUS_CALLBACK
static err_t
netifapi_do_netif_add_ext_callback(struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  netif_add_ext_callback(msg->msg.netif_ext_cb.cb, msg->msg.netif_ext_cb.fn);
  return ERR_OK;
}

err_t
netifapi_netif_add_ext_callback(netif_ext_callback_t *callback, netif_ext_callback_fn fn)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  LWIP_ERROR("netifapi_netif_add_ext_callback : invalid arguments", (callback != NULL), return ERR_VAL);
  LWIP_ERROR("netifapi_netif_add_ext_callback : invalid arguments", (fn != NULL), return ERR_VAL);
  NETIFAPI_VAR_ALLOC(msg);
  NETIFAPI_VAR_REF(msg).msg.netif_ext_cb.cb = callback;
  NETIFAPI_VAR_REF(msg).msg.netif_ext_cb.fn = fn;
  err = tcpip_api_call(netifapi_do_netif_add_ext_callback, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

static err_t
netifapi_do_netif_remove_ext_callback(struct tcpip_api_call_data *m)
{
  /* cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  netif_remove_ext_callback(msg->msg.netif_ext_cb.cb);
  return ERR_OK;
}

err_t
netifapi_netif_remove_ext_callback(netif_ext_callback_t *callback)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  LWIP_ERROR("netifapi_netif_remove_ext_callback : invalid arguments", (callback != NULL), return ERR_VAL);
  NETIFAPI_VAR_ALLOC(msg);
  NETIFAPI_VAR_REF(msg).msg.netif_ext_cb.cb = callback;
  err = tcpip_api_call(netifapi_do_netif_remove_ext_callback, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif /* LWIP_NETIF_EXT_STATUS_CALLBACK */

#if LWIP_NETIFAPI_MTU
static err_t
netifapi_do_netif_set_mtu (struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  err_t ret;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  ret = netif_set_mtu(msg->netif, msg->msg.netif_mtu.mtu);
  return ret;
}

err_t
netifapi_netif_set_mtu(struct netif *netif, u16_t mtu)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_netif_set_mtu : invalid arguments", (netif != NULL), return ERR_VAL);
  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.netif_mtu.mtu = mtu;
  err = tcpip_api_call(netifapi_do_netif_set_mtu, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif

#if LWIP_NETIFAPI_IF_NUM
static err_t
netifapi_do_netif_change_if_num (struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  err_t ret;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  ret = netif_change_if_num(msg->netif, msg->msg.netif_num.num);
  return ret;
}

err_t
netifapi_netif_change_if_num(struct netif *netif, u8_t num)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_netif_set_mtu : invalid arguments", (netif != NULL), return ERR_VAL);
  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.netif_num.num = num;
  err = tcpip_api_call(netifapi_do_netif_change_if_num, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif

#if LWIP_DHCPS
static err_t
netifapi_do_dhcps_start (struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  err_t ret;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  ret = dhcps_start(msg->netif, msg->msg.dhcp_start_params.start_ip, msg->msg.dhcp_start_params.ip_num);
  return ret;
}

err_t
netifapi_dhcps_start(struct netif *netif, char *start_ip, u16_t ip_num)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_dhcps_start : invalid arguments", (netif != NULL), return ERR_VAL);
  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.dhcp_start_params.start_ip = start_ip ;
  NETIFAPI_VAR_REF(msg).msg.dhcp_start_params.ip_num = ip_num ;

  err = tcpip_api_call(netifapi_do_dhcps_start, &API_VAR_REF(msg).call);

  NETIFAPI_VAR_FREE(msg);
  return err;
}

err_t
netifapi_dhcps_stop(struct netif *netif)
{
  LWIP_ERROR("netifapi_dhcps_stop : invalid arguments", (netif != NULL), return ERR_VAL);

  return netifapi_netif_common(netif, dhcps_stop, NULL);
}
#if LWIP_NETIFAPI_DHCPS_IP
static err_t
netifapi_do_dhcps_get_client_ip (struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  err_t ret;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  ret = dhcps_find_client_lease(msg->netif, msg->msg.dhcp_get_ip_params.mac,
                                msg->msg.dhcp_get_ip_params.maclen, msg->msg.dhcp_get_ip_params.ip);
  return ret;
}

err_t
netifapi_dhcps_get_client_ip(struct netif *netif, u8_t *mac, u8_t maclen, ip_addr_t *ip)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_dhcps_get_client_ip : invalid arguments", (netif != NULL), return ERR_VAL);
  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.dhcp_get_ip_params.mac = mac ;
  NETIFAPI_VAR_REF(msg).msg.dhcp_get_ip_params.maclen = maclen ;
  NETIFAPI_VAR_REF(msg).msg.dhcp_get_ip_params.ip = ip ;

  err = tcpip_api_call(netifapi_do_dhcps_get_client_ip, &API_VAR_REF(msg).call);

  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif
#endif /* LWIP_DHCPS */

#if LWIP_AUTOIP
/*
 * Call autoip_start() in a thread-safe way by running that function inside the
 * tcpip_thread context.
 *
 * @note use only for functions where there is only "netif" parameter.
 */
err_t
netifapi_autoip_start(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, autoip_start);
}

/*
 * Call autoip_stop() in a thread-safe way by running that function inside the
 * tcpip_thread context.
 *
 * @note use only for functions where there is only "netif" parameter.
 */
err_t
netifapi_autoip_stop(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, autoip_stop);
}
#endif /* LWIP_AUTOIP */

#if DRIVER_STATUS_CHECK

err_t
netifapi_wake_queue(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, netif_wake_queue);
}

err_t
netifapi_stop_queue(struct netif *netif)
{
  return netifapi_netif_common(netif, NULL, netif_stop_queue);
}
#endif

#if LWIP_IPV6_DHCP6
err_t
netifapi_dhcp6_enable_stateful(struct netif *netif)
{
  LWIP_ERROR("netifapi_dhcp6_enable_stateful : netif == NULL", ((netif != NULL)), return ERR_VAL);
  return netifapi_netif_common(netif, NULL, dhcp6_enable_stateful);
}

err_t
netifapi_dhcp6_enable_stateless(struct netif *netif)
{
  LWIP_ERROR("netifapi_dhcp6_enable_stateless : netif == NULL", ((netif != NULL)), return ERR_VAL);
  return netifapi_netif_common(netif, NULL, dhcp6_enable_stateless);
}

err_t
netifapi_dhcp6_disable(struct netif *netif)
{
  LWIP_ERROR("netifapi_dhcp6_disable : netif == NULL", ((netif != NULL)), return ERR_VAL);
  return netifapi_netif_common(netif, dhcp6_disable, NULL);
}

err_t
netifapi_dhcp6_release_stateful(struct netif *netif)
{
  LWIP_ERROR("netifapi_dhcp6_release_stateful : netif == NULL", ((netif != NULL)), return ERR_VAL);
  return netifapi_netif_common(netif, NULL, dhcp6_release_stateful);
}

err_t
netifapi_dhcp6_cleanup(struct netif *netif)
{
  LWIP_ERROR("netifapi_dhcp6_cleanup : netif == NULL", ((netif != NULL)), return ERR_VAL);
  return netifapi_netif_common(netif, dhcp6_cleanup, NULL);
}
#endif /* LWIP_IPV6_DHCP6 */

#if LWIP_NETIFAPI_IF_INDEX
/*
 * Call netif_name_to_index() inside the tcpip_thread context.
 */
static err_t
netifapi_do_name_to_index(struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  u8_t index;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  index = netif_name_to_index(msg->msg.ifs.name);
  msg->msg.ifs.index = index;

  if (index == 0) {
    return ERR_VAL;
  } else {
    return ERR_OK;
  }
}

/*
 * Call netif_index_to_name() inside the tcpip_thread context.
 */
static err_t
netifapi_do_index_to_name(struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  if (!netif_index_to_name(msg->msg.ifs.index, msg->msg.ifs.name)) {
    /* return failure via empty name */
    msg->msg.ifs.name[0] = '\0';
  }
  return ERR_OK;
}

/*
 * Call netif_get_nameindex_all() inside the tcpip_thread context.
 */
static err_t
netifapi_netif_get_nameindex_all(struct tcpip_api_call_data *m)
{
  err_t err;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  err = netif_get_nameindex_all(&msg->msg.p_if_list);
  return err;
}

/*
 * @ingroup netifapi_netif
 * Call netifapi_do_name_to_index() in a thread-safe way by running that function inside the
 * tcpip_thread context.
 *
 * @param name the interface name of the netif
 * @param index output index of the found netif
 */
err_t
netifapi_netif_name_to_index(const char *name, u8_t *index)
{
  err_t err;
#if LWIP_MPU_COMPATIBLE
  size_t namelen;
#endif

  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

  *index = 0;

#if LWIP_MPU_COMPATIBLE
  namelen = strlen(name);
  if (namelen <= (IF_NAMESIZE - 1)) {
    if (strncpy_s(NETIFAPI_VAR_REF(msg).msg.ifs.name, IF_NAMESIZE, name, namelen) == EOK) {
      NETIFAPI_VAR_REF(msg).msg.ifs.name[namelen] = '\0';
    } else {
      return ERR_VAL;
    }
  } else {
    return ERR_ARG;
  }
#else
  NETIFAPI_VAR_REF(msg).msg.ifs.name = (char *)name;
#endif /* LWIP_MPU_COMPATIBLE */
  err = tcpip_api_call(netifapi_do_name_to_index, &API_VAR_REF(msg).call);
  if (!err) {
    *index = NETIFAPI_VAR_REF(msg).msg.ifs.index;
  }
  NETIFAPI_VAR_FREE(msg);
  return err;
}

/*
 * @ingroup netifapi_netif
 * Call netifapi_do_index_to_name() in a thread-safe way by running that function inside the
 * tcpip_thread context.
 *
 * @param index the interface index of the netif
 * @param name output name of the found netif, empty '\0' string if netif not found.
 *             name should be of at least IF_NAMESIZE bytes
 */
err_t
netifapi_netif_index_to_name(u8_t index, char *name)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).msg.ifs.index = index;
#if !LWIP_MPU_COMPATIBLE
  NETIFAPI_VAR_REF(msg).msg.ifs.name = name;
#endif /* LWIP_MPU_COMPATIBLE */
  err = tcpip_api_call(netifapi_do_index_to_name, &API_VAR_REF(msg).call);
#if LWIP_MPU_COMPATIBLE
  if (err == ERR_OK) {
    /*
     * here stack is assuming that the input buffer is of minimum size IFNAMSIZ.
     * This is a limitation of the API
     */
    if (strncpy_s(name, IFNAMSIZ, NETIFAPI_VAR_REF(msg).msg.ifs.name,
                  strlen(NETIFAPI_VAR_REF(msg).msg.ifs.name)) == EOK) {
      name[strlen(NETIFAPI_VAR_REF(msg).msg.ifs.name)] = '\0';
    } else {
      err = ERR_VAL;
    }
  }
#endif /* LWIP_MPU_COMPATIBLE */
  NETIFAPI_VAR_FREE(msg);
  return err;
}

/*
 * @ingroup netifapi_netif
 * Call netifapi_netif_get_nameindex_all() in a thread-safe way by running that function inside the
 * tcpip_thread context.
 *
 * @param pp_if_list , the pointer to all the interface list in name index format,if_nameindex structure.
 *
 */
err_t
netifapi_netif_nameindex_all(struct if_nameindex **pp_if_list)
{
  err_t err;

  NETIFAPI_VAR_DECLARE(msg);
  NETIFAPI_VAR_ALLOC(msg);

  err = tcpip_api_call(netifapi_netif_get_nameindex_all, &API_VAR_REF(msg).call);
  *pp_if_list = NETIFAPI_VAR_REF(msg).msg.p_if_list;

  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif /* LWIP_NETIFAPI_IF_INDEX */

#if LWIP_NETIF_HOSTNAME
static err_t
do_netifapi_set_hostname(struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  if (strncpy_s(msg->netif->hostname, NETIF_HOSTNAME_MAX_LEN,
                msg->msg.hostname.name, NETIF_HOSTNAME_MAX_LEN - 1) == EOK) {
    if (msg->msg.hostname.namelen < (NETIF_HOSTNAME_MAX_LEN - 1)) {
      msg->netif->hostname[msg->msg.hostname.namelen] = '\0';
    } else {
      msg->netif->hostname[NETIF_HOSTNAME_MAX_LEN - 1] = '\0';
    }
    return ERR_OK;
  }
  return ERR_VAL;
}

err_t
netifapi_set_hostname(struct netif *netif, char *hostname, u8_t namelen)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_set_hostname:netif is NULL", (netif != NULL), return ERR_ARG);
  LWIP_ERROR("netifapi_set_hostname:hostname is NULL", (hostname != NULL), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.hostname.name  = hostname;
  NETIFAPI_VAR_REF(msg).msg.hostname.namelen = namelen;

  err = tcpip_api_call(do_netifapi_set_hostname, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

#if LWIP_NETIF_GET_HOSTNAME

static err_t
do_netifapi_get_hostname(struct tcpip_api_call_data *m)
{
  /*
   * cast through void* to silence alignment warnings.
   * We know it works because the structs have been instantiated as struct netifapi_msg
   */
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  if (strncpy_s(msg->msg.hostname.name, msg->msg.hostname.namelen,
                msg->netif->hostname, (size_t)(msg->msg.hostname.namelen - 1)) == EOK) {
    msg->msg.hostname.name[msg->msg.hostname.namelen - 1] = '\0';
    return ERR_OK;
  }
  return ERR_VAL;
}

err_t
netifapi_get_hostname(struct netif *netif, char *hostname, u8_t namelen)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_get_hostname:netif is NULL", (netif != NULL), return ERR_ARG);
  LWIP_ERROR("netifapi_get_hostname:hostname is NULL", (hostname != NULL), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.hostname.name  = hostname;
  NETIFAPI_VAR_REF(msg).msg.hostname.namelen = namelen;

  err = tcpip_api_call(do_netifapi_get_hostname, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif /* LWIP_NETIF_GET_HOSTNAME */
#endif /* LWIP_NETIF_HOSTNAME */

#if LWIP_DHCP_VENDOR_CLASS_IDENTIFIER
LWIP_STATIC err_t
do_netifapi_set_vci(struct tcpip_api_call_data *m)
{
  err_t ret;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  ret = dhcp_set_vci(msg->msg.vci.vci, *(msg->msg.vci.vci_len));
  return ret;
}

err_t
netifapi_set_vci(char *vci, u8_t vci_len)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_set_vci:vci is NULL", (vci != NULL), return ERR_ARG);
  LWIP_ERROR("netifapi_set_vci:vci_len > DHCP_VCI_MAX_LEN", (vci_len <= DHCP_VCI_MAX_LEN), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = NULL;
  NETIFAPI_VAR_REF(msg).msg.vci.vci = vci;
  NETIFAPI_VAR_REF(msg).msg.vci.vci_len = &vci_len;

  err = tcpip_api_call(do_netifapi_set_vci, &API_VAR_REF(msg).call);

  NETIFAPI_VAR_FREE(msg);
  return err;
}

#if LWIP_DHCP_GET_VENDOR_CLASS_IDENTIFIER
LWIP_STATIC err_t
do_netifapi_get_vci(struct tcpip_api_call_data *m)
{
  err_t ret;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  ret = dhcp_get_vci(msg->msg.vci.vci, msg->msg.vci.vci_len);
  return ret;
}

err_t
netifapi_get_vci(char *vci, u8_t *vci_len)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_get_vci:vci is NULL", (vci != NULL), return ERR_ARG);
  LWIP_ERROR("netifapi_get_vci:vci_len is NULL", (vci_len != NULL), return ERR_ARG);
  LWIP_ERROR("netifapi_get_vci:*vci_len < DHCP_VCI_MAX_LEN", (*vci_len >= DHCP_VCI_MAX_LEN), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = NULL;
  NETIFAPI_VAR_REF(msg).msg.vci.vci = vci;
  NETIFAPI_VAR_REF(msg).msg.vci.vci_len = vci_len;

  err = tcpip_api_call(do_netifapi_get_vci, &API_VAR_REF(msg).call);

  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif /* LWIP_DHCP_GET_VENDOR_CLASS_IDENTIFIER */
#endif /* LWIP_DHCP_VENDOR_CLASS_IDENTIFIER */

#if LWIP_IP_FILTER || LWIP_IPV6_FILTER
static err_t
do_netifapi_set_ip_filter(struct tcpip_api_call_data *m)
{
  err_t ret = ERR_VAL;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
#if LWIP_IP_FILTER
  if (msg->msg.ip_filter.type == IPADDR_TYPE_V4) {
    ret = set_ip4_filter(msg->msg.ip_filter.filter_fn);
  }
#endif /* LWIP_IP_FILTER */
#if LWIP_IPV6_FILTER
  if (msg->msg.ip_filter.type == IPADDR_TYPE_V6) {
    ret = set_ip6_filter(msg->msg.ip_filter.filter_fn);
  }
#endif /* LWIP_IPV6_FILTER */
  return ret;
}

/*
 * Set ip filter_fn, the filter_fn will be called when pass a packet to ip_input.
 *
 * @param filter_fn The filter implement function, NULL indicate disable the filter.
 * @return
 *  - ERR_OK: On success
 *  - ERR_VAL: On failure due to Illegal value
 */
err_t
netifapi_set_ip_filter(ip_filter_fn filter_fn, int type)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  NETIFAPI_VAR_ALLOC(msg);
  (void)memset_s(&msg, sizeof(struct netifapi_msg), 0, sizeof(struct netifapi_msg));

  NETIFAPI_VAR_REF(msg).msg.ip_filter.filter_fn = filter_fn;
  NETIFAPI_VAR_REF(msg).msg.ip_filter.type      = type;

  err = tcpip_api_call(do_netifapi_set_ip_filter, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif /* LWIP_IP_FILTER || LWIP_IPV6_FILTER */

#if LWIP_IPV6
#if LWIP_ND6_ROUTER
static err_t
netifapi_do_set_ra_enable(struct tcpip_api_call_data *m)
{
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  struct netif *nif = msg->netif;
  u8_t ra_enable = msg->msg.ip6_state.state;

  nif->ra_enable = (ra_enable > 0) ? lwIP_TRUE : lwIP_FALSE;
  return ERR_OK;
}

err_t
netifapi_set_ra_enable(struct netif *netif, u8_t ra_enable)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_set_ipv6_forwarding : invalid arguments \n", (netif != NULL), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.ip6_state.state = ra_enable;

  err = tcpip_api_call(netifapi_do_set_ra_enable, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

static err_t
netifapi_do_set_ipv6_forwarding(struct tcpip_api_call_data *m)
{
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  struct netif *nif = msg->netif;
  u8_t forwarding = msg->msg.ip6_state.state;

  nif->forwarding = forwarding;
  return ERR_OK;
}

err_t
netifapi_set_ipv6_forwarding(struct netif *netif, u8_t forwarding)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_set_ipv6_forwarding : invalid arguments \n", (netif != NULL), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.ip6_state.state = forwarding;

  err = tcpip_api_call(netifapi_do_set_ipv6_forwarding, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

static err_t
netifapi_do_set_accept_ra(struct tcpip_api_call_data *m)
{
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  struct netif *nif = msg->netif;
  u8_t accept_ra = msg->msg.ip6_state.state;

  nif->accept_ra = accept_ra;
  return ERR_OK;
}

err_t
netifapi_set_accept_ra(struct netif *netif, u8_t accept_ra)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_set_ipv6_forwarding : invalid arguments \n", (netif != NULL), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.ip6_state.state = accept_ra;

  err = tcpip_api_call(netifapi_do_set_accept_ra, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif

#if LWIP_6LOWPAN
static err_t
netifapi_do_set_6lowpan_enable(struct tcpip_api_call_data *m)
{
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  struct netif *nif = msg->netif;
  u8_t enable = msg->msg.ip6_state.state;

  nif->enabled6lowpan = (enable > 0) ? lwIP_TRUE : lwIP_FALSE;
  return ERR_OK;
}

err_t
netifapi_set_6lowpan_enable(struct netif *netif, u8_t enable)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_set_6lowpan_enable : invalid arguments \n", (netif != NULL), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.ip6_state.state = enable;

  err = tcpip_api_call(netifapi_do_set_6lowpan_enable, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif

/*
 * These APIs will be responsible for adding , deleting and querying the
 * neighbor cache when IPv6 is enabled
 */
#if LWIP_NETIF_NBR_CACHE_API
static err_t
netifapi_do_add_ipv6_neighbor(struct tcpip_api_call_data *m)
{
  err_t err;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  struct ipv6_neighbor *nb = msg->msg.ipv6_tables_access_params.nbrinfo;
  struct nd6_neighbor_info nd6nbr;

  nd6nbr.curstate = nb->curstate;
  nd6nbr.hwlen = nb->hwlen;
  nd6nbr.reachabletime = nb->reachabletime;

  if (memcpy_s(nd6nbr.hwaddr, NETIF_MAX_HWADDR_LEN, nb->hwaddr, nb->hwlen) != EOK) {
    return ERR_MEM;
  }
  ip6_addr_set(&(nd6nbr.nbripaddr), &(nb->nbripaddr));

  err = nd6_add_neighbor_cache_entry_manually(msg->netif, &nd6nbr);
  return err;
}

static err_t
netifapi_do_del_ipv6_neighbor(struct tcpip_api_call_data *m)
{
  err_t err;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  struct ip6_addr *nbrip = msg->msg.ipv6_tables_access_params.nodeip;

  err = nd6_del_neighbor_cache_entry_manually(msg->netif, nbrip);
  return err;
}

static err_t
netifapi_do_query_ipv6_neighbor(struct tcpip_api_call_data *m)
{
  err_t err;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  struct ipv6_neighbor *nb = msg->msg.ipv6_tables_access_params.nbrinfo;
  struct ip6_addr *nbrip = msg->msg.ipv6_tables_access_params.nodeip;

  struct nd6_neighbor_info nd6nbr;

  err = nd6_get_neighbor_cache_info(msg->netif, nbrip, &nd6nbr);
  if (err == ERR_OK) {
    /* Copy the info */
    nb->curstate = nd6nbr.curstate;
    nb->hwlen = nd6nbr.hwlen;
    nb->reachabletime = nd6nbr.reachabletime;

    if (memcpy_s(nb->hwaddr, NETIF_MAX_HWADDR_LEN, nd6nbr.hwaddr, nd6nbr.hwlen) != EOK) {
      return ERR_MEM;
    }
    nb->hwlen = nd6nbr.hwlen;

    ip6_addr_set(&(nb->nbripaddr), &(nd6nbr.nbripaddr));
  }

  return err;
}

err_t
netifapi_add_ipv6_neighbor(struct netif *netif, struct ipv6_neighbor *nbr)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_add_ipv6_neighbor : invalid arguments \n", \
             ((netif != NULL) && (nbr != NULL)), return ERR_ARG);

  LWIP_ERROR("netifapi_add_ipv6_neighbor : MAC len is invalid \n", \
             ((nbr->hwlen <= NETIF_MAX_HWADDR_LEN)), return ERR_VAL);

  NETIFAPI_VAR_ALLOC(msg);

  if ((nbr->curstate != ND6_STATE_REACHABLE)
#if LWIP_ND6_STATIC_NBR
      && (nbr->curstate != ND6_STATE_PERMANENT)
#endif
     ) {
    LWIP_ERROR("netifapi_add_ipv6_neighbor : invalid sate \n", 0, return ERR_VAL);
  }

#if LWIP_ND6_STATIC_NBR
  if ((nbr->curstate == ND6_STATE_PERMANENT) && (nbr->reachabletime != 0)) {
    LWIP_ERROR("netifapi_add_ipv6_neighbor : For static neighbors rechable time " \
               "must be set to 0 \n", 0, return ERR_VAL);
  }
#endif

  if ((nbr->curstate == ND6_STATE_REACHABLE) &&
      (nbr->reachabletime > LWIP_ND6_REACHABLE_TIME)) {
    LWIP_DEBUGF(NETIF_DEBUG, ("netifapi: Trying to set the reachable time[%u] " \
                              "greater than configured[%d] after first expiry it will use the " \
                              "configured one\n", nbr->reachabletime, LWIP_ND6_REACHABLE_TIME));
  }

  (void)memset_s(&msg, sizeof(struct netifapi_msg), 0, sizeof(struct netifapi_msg));
  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.ipv6_tables_access_params.nbrinfo = nbr;

  err = tcpip_api_call(netifapi_do_add_ipv6_neighbor, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;

  /* Validate the input parameter */
}

err_t
netifapi_del_ipv6_neighbor(struct netif *netif, struct ip6_addr *ipaddr)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_del_ipv6_neighbor : invalid arguments \n", \
             ((netif != NULL) && (ipaddr != NULL)), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  (void)memset_s(&msg, sizeof(struct netifapi_msg), 0, sizeof(struct netifapi_msg));
  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.ipv6_tables_access_params.nodeip = ipaddr;

  err = tcpip_api_call(netifapi_do_del_ipv6_neighbor, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

err_t
netifapi_query_ipv6_neighbor(struct netif *netif, struct ip6_addr *ipaddr,
                             struct ipv6_neighbor *nbrinfo)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_del_ipv6_neighbor : invalid arguments \n", \
             ((netif != NULL) && (ipaddr != NULL) && (nbrinfo != NULL)), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  (void)memset_s(&msg, sizeof(struct netifapi_msg), 0, sizeof(struct netifapi_msg));
  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.ipv6_tables_access_params.nodeip = ipaddr;
  NETIFAPI_VAR_REF(msg).msg.ipv6_tables_access_params.nbrinfo = nbrinfo;

  err = tcpip_api_call(netifapi_do_query_ipv6_neighbor, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif /* LWIP_NETIF_NBR_CACHE_API */
#if LWIP_6LOWPAN

static err_t
netifapi_do_set_6lowpan_context(struct tcpip_api_call_data *m)
{
  err_t err;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  struct ip6_addr *ctxprefix = msg->msg.lowpanctx.ctxprefix;
  u8_t ctxidx = msg->msg.lowpanctx.index;

  err = lowpan6_set_context(msg->netif, ctxidx, ctxprefix);
  return err;
}

static err_t
netifapi_do_remove_6lowpan_context(struct tcpip_api_call_data *m)
{
  err_t err;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  u8_t ctxidx = msg->msg.lowpanctx.index;

  err = lowpan6_remove_context(msg->netif, ctxidx);
  return err;
}

err_t
netifapi_set_6lowpan_context(struct netif *netif, u8_t ctxid, struct ip6_addr *context_prefix)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  /* LWIP_6LOWPAN_NUM_CONTEXTS */
  LWIP_ERROR("netifapi_set_6lowpan_context : invalid arguments \n", \
             ((netif != NULL) && (ctxid <= LWIP_6LOWPAN_MAX_CTXID) && (context_prefix != NULL)), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.lowpanctx.ctxprefix = context_prefix;
  NETIFAPI_VAR_REF(msg).msg.lowpanctx.index = ctxid;

  err = tcpip_api_call(netifapi_do_set_6lowpan_context, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

err_t
netifapi_remove_6lowpan_context(struct netif *netif, u8_t ctxid)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  /* LWIP_6LOWPAN_NUM_CONTEXTS */
  LWIP_ERROR("netifapi_remove_6lowpan_context : invalid arguments \n", \
             ((netif != NULL) && (ctxid <= LWIP_6LOWPAN_MAX_CTXID)), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.lowpanctx.index = ctxid;

  err = tcpip_api_call(netifapi_do_remove_6lowpan_context, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

#endif /* LWIP_6LOWPAN */

#if LWIP_ND6_STATIC_DEFAULT_ROUTE
static err_t
netifapi_do_netif_add_ipv6_default_route(struct tcpip_api_call_data *m)
{
  err_t err;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  struct ip6_addr *router_addr = msg->msg.ipv6_tables_access_params.nodeip;

  err = lwip_nd6_add_static_default_route_manually(msg->netif,  router_addr);
  return err;
}

static err_t
netifapi_do_netif_del_ipv6_default_route(struct tcpip_api_call_data *m)
{
  err_t err;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  ip6_addr_t *nodeip = msg->msg.ipv6_tables_access_params.nodeip;

  err = lwip_nd6_del_static_default_route(msg->netif, (const ip6_addr_t *)nodeip);
  return err;
}

err_t
netifapi_add_ipv6_static_default_route
(
  struct netif *netif,
  struct ip6_addr *router_addr
)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_add_ipv6_default_route: invalid arguments", \
             ((router_addr != NULL) && (netif != NULL)), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);
  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.ipv6_tables_access_params.nodeip = router_addr;
  err = tcpip_api_call(netifapi_do_netif_add_ipv6_default_route, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

err_t
netifapi_del_ipv6_static_default_route(struct netif *netif,
                                       struct ip6_addr *router_addr)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_del_ipv6_default_route: invalid arguments", \
             ((router_addr != NULL) && (netif != NULL)), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.ipv6_tables_access_params.nodeip = router_addr;
  err = tcpip_api_call(netifapi_do_netif_del_ipv6_default_route, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);

  return err;
}
#endif /* LWIP_ND6_STATIC_DEFAULT_ROUTE */

#if LWIP_ND6_STATIC_PREFIX
static err_t
netifapi_do_add_static_prefix(struct tcpip_api_call_data *m)
{
  err_t err;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  err = nd6_add_static_prefix(msg->netif,
                              msg->msg.prefix_params.prefix,
                              msg->msg.prefix_params.auto_config_flags);
  return err;
}

static err_t
netifapi_do_del_static_prefix(struct tcpip_api_call_data *m)
{
  err_t err;
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;

  err = nd6_del_static_prefix(msg->netif,
                              msg->msg.prefix_params.prefix);
  return err;
}

err_t
netifapi_add_static_prefix(struct netif *netif, struct ip6_addr *prefix,
                           u8_t flags)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_add_prefix: invalid parameter.\n", \
             ((NULL != netif) && (NULL != prefix)), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.prefix_params.prefix = prefix;
  NETIFAPI_VAR_REF(msg).msg.prefix_params.auto_config_flags = flags;

  err = tcpip_api_call(netifapi_do_add_static_prefix, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

err_t
netifapi_del_static_prefix(struct netif *netif, struct ip6_addr *prefix)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_del_prefix: invalid parameter.\n", \
             ((NULL != netif) && (NULL != prefix)), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).msg.prefix_params.prefix = prefix;
  err = tcpip_api_call(netifapi_do_del_static_prefix, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

#endif /* LWIP_ND6_STATIC_PREFIX */

#endif /* LWIP_IPV6 */

#if LWIP_MAC_SECURITY
static err_t
netifapi_do_set_authentication_status(struct tcpip_api_call_data *m)
{
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  u8_t authstatus = msg->authstatus;

  if ((authstatus != AUTHENTICATION_SUCCESS) && (authstatus != AUTHENTICATION_FAILURE)) {
    return -1;
  }

  msg->netif->is_auth_sucess = authstatus;

  return ERR_OK;
}

err_t
netifapi_set_authentication_status(struct netif *netif, u8_t authstatus)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  /* LWIP_6LOWPAN_NUM_CONTEXTS */
  LWIP_ERROR("netifapi_del_ipv6_neighbor : invalid arguments \n", \
             ((netif != NULL)), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).netif = netif;
  NETIFAPI_VAR_REF(msg).authstatus = authstatus;

  err = tcpip_api_call(netifapi_do_set_authentication_status, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif /* LWIP_MAC_SECURITY */

#if LWIP_LOWPOWER
static err_t
netifapi_do_set_lowpower_mod(struct tcpip_api_call_data *m)
{
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  enum lowpower_mod mod = msg->msg.lp.mod;
  set_lowpower_mod(mod);
  return ERR_OK;
}

err_t
netifapi_enable_lowpower(void)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).msg.lp.mod = LOW_TMR_LOWPOWER_MOD;

  err = tcpip_api_call(netifapi_do_set_lowpower_mod, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}

err_t
netifapi_disable_lowpower(void)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  NETIFAPI_VAR_ALLOC(msg);

  NETIFAPI_VAR_REF(msg).msg.lp.mod = LOW_TMR_NORMAL_MOD;

  err = tcpip_api_call(netifapi_do_set_lowpower_mod, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif

#if LWIP_IPV4 && LWIP_ARP
static err_t
netifapi_do_ip_to_mac(struct tcpip_api_call_data *m)
{
  struct netifapi_msg *msg = (struct netifapi_msg *)(void *)m;
  return etharp_ip_to_mac((const ip4_addr_t *)(msg->msg.arp.ip), msg->msg.arp.mac, msg->msg.arp.maclen);
}

err_t
netifapi_ip_to_mac(const ip4_addr_t *ip, u8_t *mac, u8_t *maclen)
{
  err_t err;
  NETIFAPI_VAR_DECLARE(msg);

  LWIP_ERROR("netifapi_ip_to_mac : invalid arguments \n", ((ip != NULL)), return ERR_ARG);
  LWIP_ERROR("netifapi_ip_to_mac : invalid arguments \n", ((mac != NULL)), return ERR_ARG);
  LWIP_ERROR("netifapi_ip_to_mac : invalid arguments \n", ((maclen != NULL)), return ERR_ARG);

  NETIFAPI_VAR_ALLOC(msg);
  NETIFAPI_VAR_REF(msg).msg.arp.ip = (ip4_addr_t *)ip;
  NETIFAPI_VAR_REF(msg).msg.arp.mac = mac;
  NETIFAPI_VAR_REF(msg).msg.arp.maclen = maclen;
  err = tcpip_api_call(netifapi_do_ip_to_mac, &API_VAR_REF(msg).call);
  NETIFAPI_VAR_FREE(msg);
  return err;
}
#endif

#endif /* LWIP_NETIF_API */
