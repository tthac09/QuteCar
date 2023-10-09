/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2015. All rights reserved.
 * Description: This is the IPv4 & IPv6 address tools implementation.
 * Author: none
 * Create: 2013
 */

#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#if LWIP_IPV4 || LWIP_IPV6

#include "lwip/ip_addr.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"


#if LWIP_SOCKET_SET_ERRNO
#ifndef set_errno
#define set_errno(err) do { errno = (err); } while (0)
#endif
#else /* LWIP_SOCKET_SET_ERRNO */
#define set_errno(err)
#endif /* LWIP_SOCKET_SET_ERRNO */


#if LWIP_INET_ADDR_FUNC
in_addr_t
inet_addr(const char *cp)
{
  LWIP_ERROR("inet_aton:cp is NULL", (cp != NULL), return (INADDR_NONE));
  return ipaddr_addr(cp);
}
#endif

#if LWIP_INET_ATON_FUNC
int
inet_aton(const char *cp, struct in_addr *inp)
{
  LWIP_ERROR("inet_aton:cp is NULL", (cp != NULL), return 0);
  return ip4addr_aton(cp, (ip4_addr_t *)inp);
}
#endif

#if LWIP_INET_NTOA_FUNC
char *
inet_ntoa(struct in_addr in)
{
  return ip4addr_ntoa((const ip4_addr_t *)&in);
}
#endif


const char *
lwip_inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
  const char *ret = NULL;
  if ((src == NULL) || (dst == NULL)) {
    set_errno(EINVAL);
    return NULL;
  }
  switch (af) {
#if LWIP_IPV4
    case AF_INET:
      ret = lwip_inet_ntop4(src, dst, size);
      if (ret == NULL) {
        set_errno(ENOSPC);
      }
      break;
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
    case AF_INET6:
      ret = ip6addr_ntoa_r((const ip6_addr_t *)(src), (dst), INET6_ADDRSTRLEN);
      if (ret == NULL) {
        set_errno(ENOSPC);
      }
      break;
#endif /* LWIP_IPV6 */
    default:
      set_errno(EAFNOSUPPORT);
      break;
  }

  return ret;
}

int
lwip_inet_pton(int af, const char *src, void *dst)
{
  int err;
  if ((src == NULL) || (dst == NULL)) {
    set_errno(EINVAL);
    return -1;
  }
  switch (af) {
#if LWIP_IPV4
    case AF_INET:
      return inet_pton4(src, dst);
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
    case AF_INET6: {
      /* convert into temporary variable since ip6_addr_t might be larger
         than in6_addr when scopes are enabled */
      ip6_addr_t addr;
      err = ip6addr_aton((src), &addr);
      if (err) {
        (void)memcpy_s(dst, sizeof(addr.addr), addr.addr, sizeof(addr.addr));
      }

      return err;
#endif /* LWIP_IPV6 */
    }
    default:
      set_errno(EAFNOSUPPORT);
      break;
  }

  return -1;
}
#endif /* LWIP_IPV4 || LWIP_IPV6 */

#if LWIP_IPV4 && LWIP_IPV6
#if LWIP_SMALL_SIZE
int ip_addr_cmp(const ip_addr_t *addr1, const ip_addr_t *addr2)
{
  if (addr1 == NULL || addr2 == NULL) {
    return 0;
  }

  return ((IP_GET_TYPE(addr1) != IP_GET_TYPE(addr2)) ? 0 : (IP_IS_V6_VAL(*(addr1)) ? \
          ip6_addr_cmp(ip_2_ip6(addr1), ip_2_ip6(addr2)) : \
          ip4_addr_cmp(ip_2_ip4(addr1), ip_2_ip4(addr2))));
}

int ip_addr_isany(const ip_addr_t *ipaddr)
{
  return (((ipaddr) == NULL) ? 1 : ((IP_IS_V6(ipaddr)) ? ip6_addr_isany(ip_2_ip6(ipaddr)) :
          ip4_addr_isany(ip_2_ip4(ipaddr))));
}

int ip_addr_isloopback(const ip_addr_t* ipaddr)
{
  if (ipaddr == NULL) {
    return 0;
  }
  return ((IP_IS_V6(ipaddr)) ? ip6_addr_isloopback(ip_2_ip6(ipaddr)) :
          ip4_addr_isloopback(ip_2_ip4(ipaddr)));
}
#endif
#endif