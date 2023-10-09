/**
 * @file
 * Interface Identification APIs from:
 *              RFC 3493: Basic Socket Interface Extensions for IPv6
 *                  Section 4: Interface Identification
 *
 * @defgroup if_api Interface Identification API
 * @ingroup socket
 */

/*
 * Copyright (c) 2017 Joel Cunningham, Garmin International, Inc. <joel.cunningham@garmin.com>
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
 * Author: Joel Cunningham <joel.cunningham@me.com>
 *
 */
/*
 * This file contains the implementation for RFC 2553/3493 section Section 4: Interface Identification
 * The following functions are implemented
 *  lwip_if_indextoname
 *  lwip_if_nametoindex
 *  lwip_if_nameindex
 *  lwip_if_freenameindex
 *  @note: when passing the ifname pointer to the function lwip_if_indextoname:
 *  The ifname argument must point to a buffer of at least IF_NAMESIZE
 *  bytes into which the interface name corresponding to the specified
 *  index is returned.
 *  IF_NAMESIZE is same as IFNAMSIZ and is of 16 bytes including null
 *  The lwip_if_nameindex API can only support max 10 interface indexes. The index starts
 *  from 1 and ends at 10.
 *
 */
#include "lwip/opt.h"

#if LWIP_SOCKET

#include "lwip/errno.h"
#include "lwip/if_api.h"
#include "lwip/netifapi.h"
#include "lwip/priv/sockets_priv.h"

#if LWIP_SOCKET_SET_ERRNO
#ifndef set_errno
#define set_errno(err) do { errno = (err); } while (0)
#endif
#else /* LWIP_SOCKET_SET_ERRNO */
#define set_errno(err)
#endif /* LWIP_SOCKET_SET_ERRNO */

#if LWIP_COMPAT_SOCKETS != 2
#if LWIP_NETIFAPI_IF_INDEX
/**
 *  @page RFC-2553/3493 RFC-2553/3493
 *  @par Compliant Section
 *  Section 4.1 Name-to-Index
 *  @par Behavior Description
 *  if_nametoindex: The function maps an interface name into its corresponding index.\n
 *  Behavior:  lwip_if_nametoindex maps an interface name into its corresponding index
 */
/**
 *  @page RFC-2553/3493 RFC-2553/3493
 *  @par Compliant Section
 *  Section 4.2 Index-to-Name
 *  @par Behavior Description
 *  if_indextoname: The function maps an interface index into its corresponding name.\n
 *  Behavior:  lwip_if_indextoname maps an interface index into its corresponding name. When passing
 *  the ifname pointer to the function lwip_if_indextoname:The ifname argument must point to a
 *  buffer of at least IF_NAMESIZE bytes into which the interface name corresponding to the specified
 *  index is returned.
 */
/**
 *  @page RFC-2553/3493 RFC-2553/3493
 *
 *  @par Compliant Section
 *  Section 4.3 Return All Interface Names and Indexes
 *  @par Behavior Description
 *  if_nameindex: The function returns an array of if_nameindex structures, one structure per interface.\n
 *  Behavior:  lwip_if_nameindex is the mapping function for if_nameindex
 */
/**
 *  @page RFC-2553/3493 RFC-2553/3493
 *
 *  @par Compliant Sections
 *  Section 4.4 Free Memory
 *  @par Behavior Description
 *  if_freenameindex: The following function frees the dynamic memory that was allocated by if_nameindex \n
 *  Behavior:  lwip_if_freenameindex frees the dynamic memory that was allocated by if_freenameindex
 */
/**
 * @ingroup if_api
 * Maps an interface index to its corresponding name.
 * @param ifindex interface index
 * @param ifname shall point to a buffer of at least {IF_NAMESIZE} bytes
 * @return If ifindex is an interface index, then the function shall return the
 * value supplied in ifname, which points to a buffer now containing the interface name.
 * Otherwise, the function shall return a NULL pointer.
 */
char *
lwip_if_indextoname(unsigned int ifindex, char *ifname)
{
  err_t err;
  /*
   * Limiting the max index to LWIP_NETIF_IFINDEX_MAX_EX value to keep it aligned to netif_alloc_ifindex
   * and as the variable ifindex is a uint8 value
   */
  if (ifindex > LWIP_NETIF_IFINDEX_MAX_EX) {
    set_errno(ENXIO);
    return NULL;
  }
  LWIP_ERROR("lwip_if_indextoname:ifname is NULL", (ifname != NULL), set_errno(EFAULT); return NULL);
  err = netifapi_netif_index_to_name((u8_t)ifindex, ifname);
  if ((err == ERR_OK) && (ifname[0] != '\0')) {
    return ifname;
  }
  set_errno(ENXIO);
  return NULL;
}

/**
 * @ingroup if_api
 * Returns the interface index corresponding to name ifname.
 * @param ifname Interface name
 * @return The corresponding index if ifname is the name of an interface;
 * otherwise, zero.
 */
unsigned int
lwip_if_nametoindex(const char *ifname)
{
  err_t err;
  u8_t u_index;
  LWIP_ERROR("lwip_if_nametoindex:ifname is NULL", (ifname != NULL), set_errno(EFAULT); return 0);
  err = netifapi_netif_name_to_index(ifname, &u_index);
  if (err == ERR_OK) {
    return u_index;
  }
  set_errno(ENODEV);
  return 0; /* invalid index */
}

/**
 * @ingroup if_api
 * Returns an array of if_nameindex structures, one structure per interface.
 *
 * @param input None
 * @return if_nameindex* The array of if_nameindex structures, one structure per interface
 *
 */
struct if_nameindex *
lwip_if_nameindex(void)
{
  err_t err;
  struct if_nameindex *pif_index_list = NULL;
  err = netifapi_netif_nameindex_all(&pif_index_list);
  if ((err == ERR_OK) && (pif_index_list != NULL)) {
    return pif_index_list;
  }
  set_errno(ENOBUFS);
  return NULL;
}

/**
 * @ingroup if_api
 * Frees the dynamic memory that was allocated by lwip_if_nameindex().
 *
 * @param if_nameindex* The pointer to if_nameindex structure
 * @return None
 *
 */
void
lwip_if_freenameindex(struct if_nameindex *ptr)
{
  if (ptr == NULL) {
    return;
  }
  mem_free(ptr);
}
#endif /* LWIP_NETIFAPI_IF_INDEX */
#endif /* LWIP_COMPAT_SOCKETS != 2 */
#endif /* LWIP_SOCKET */
