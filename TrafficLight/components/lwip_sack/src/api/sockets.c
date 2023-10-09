/**
 * @file
 * Sockets BSD-Like API module
 *
 * @defgroup socket Socket API
 * @ingroup sequential_api
 * BSD-style socket API.\n
 * Thread-safe, to be called from non-TCPIP threads only.\n
 * Can be activated by defining @ref LWIP_SOCKET to 1.\n
 * Header is in posix/sys/socket.h\b
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 * Improved by Marc Boucher <marc@mbsi.ca> and David Haas <dhaas@alum.rpi.edu>
 *
 */

#include "lwip/opt.h"

#if LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */
#include "lwip/sockets.h"
#include "lwip/priv/sockets_priv.h"
#include "netif/ifaddrs.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "lwip/igmp.h"
#include "lwip/inet.h"
#include "lwip/tcp.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/priv/tcpip_priv.h"
#include "lwip/errno.h"
#include "lwip/netif.h"
#include "lwip/mld6.h"

#if LWIP_IPV6_DHCP6
#include "lwip/dhcp6.h"
#include "lwip/prot/dhcp6.h"
#endif /* LWIP_IPV6_DHCP6 */

#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif
#if LWIP_CHECKSUM_ON_COPY
#include "lwip/inet_chksum.h"
#endif
#include "lwip/filter.h"
#include "lwip/priv/api_msg.h"
#include "lwip/tcp_info.h"
#if LWIP_LITEOS_COMPAT
#include <netinet/tcp.h>
#endif

#include <string.h>
#include <stdlib.h>

#if LWIP_NETIF_ETHTOOL
#include "lwip/ethtool.h"
#endif

#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif

#if LWIP_LITEOS_COMPAT
#if LWIP_SOCKET_POLL
#include "liteos/if_packet.h"
#include "fs/fs.h"
#include "poll.h"
#include "linux/wait.h"
#endif /* LWIP_SOCKET_POLL */
#endif

#if LWIP_LOWPOWER
#include "lwip/lowpower.h"
#endif

#if LWIP_IPV6
#if LWIP_IPV6_DUP_DETECT_ATTEMPTS
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCSIPV6DAD(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCGIPV6DAD(struct ifreq *ifr);
#endif
#if LWIP_IOCTL_IPV6DPCTD
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCSIPV6DPCTD(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCGIPV6DPCTD(struct ifreq *ifr);
#endif
#endif

#if LWIP_IPV4
#define IP4ADDR_PORT_TO_SOCKADDR(sin, ipaddr, port) do { \
      (sin)->sin_family = AF_INET; \
      (sin)->sin_port = lwip_htons((port)); \
      inet_addr_from_ip4addr(&(sin)->sin_addr, ipaddr); \
      (void)memset((sin)->sin_zero, 0, SIN_ZERO_LEN); } while (0)

#define SOCKADDR4_TO_IP4ADDR_PORT(sin, ipaddr, port) do { \
    inet_addr_to_ip4addr(ip_2_ip4(ipaddr), &((sin)->sin_addr)); \
    (port) = lwip_ntohs((sin)->sin_port); } while (0)
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
/*
 * SIN6_LEN macro is to differntiate whether stack is using 4.3BSD or 4.4BSD variants of sockaddr_in6
 * structure
 */
#if defined(SIN6_LEN)
#define IP6ADDR_PORT_TO_SOCKADDR(sin6, ipaddr, port) do { \
      (sin6)->sin6_len = sizeof(struct sockaddr_in6); \
      (sin6)->sin6_family = AF_INET6; \
      (sin6)->sin6_port = lwip_htons((port)); \
      (sin6)->sin6_flowinfo = 0; \
      inet6_addr_from_ip6addr(&(sin6)->sin6_addr, ipaddr); \
      (sin6)->sin6_scope_id = ip6_addr_zone(ipaddr); } while (0)
#else
#define IP6ADDR_PORT_TO_SOCKADDR(sin6, ipaddr, port) do { \
      (sin6)->sin6_family = AF_INET6; \
      (sin6)->sin6_port = lwip_htons((port)); \
      (sin6)->sin6_flowinfo = 0; \
      inet6_addr_from_ip6addr(&(sin6)->sin6_addr, ipaddr); \
      (sin6)->sin6_scope_id = ip6_addr_zone(ipaddr); } while (0)
#endif

#define SOCKADDR6_TO_IP6ADDR_PORT(sin6, ipaddr, port) do { \
    inet6_addr_to_ip6addr(ip_2_ip6(ipaddr), &((sin6)->sin6_addr)); \
    (port) = lwip_ntohs((sin6)->sin6_port); } while (0)
#endif /* LWIP_IPV6 */

/**
 * A struct sockaddr replacement that has the same alignment as sockaddr_in/
 *  sockaddr_in6 if instantiated.
 */
union sockaddr_aligned {
  struct sockaddr sa;
#if LWIP_IPV6
  struct sockaddr_in6 sin6;
#endif /* LWIP_IPV6 */
#if LWIP_IPV4
  struct sockaddr_in sin;
#endif /* LWIP_IPV4 */
};

#if LWIP_IPV4 && LWIP_IPV6
static void sockaddr_to_ipaddr_port(const struct sockaddr *sockaddr, ip_addr_t *ipaddr, u16_t *port);
static int lwip_sockopt_check_optlen_conn_pcb(struct lwip_sock *sock, int optlen, int len);
static int lwip_sockopt_check_optlen_conn_pcb_type(struct lwip_sock *sock, int optlen, int len,
                                                   enum netconn_type type, int is_v6);

#define IS_SOCK_ADDR_LEN_VALID(namelen)  (((namelen) == sizeof(struct sockaddr_in)) || \
                                         ((namelen) == sizeof(struct sockaddr_in6)))
#define IS_SOCK_ADDR_TYPE_VALID(name)    (((name)->sa_family == AF_INET) || \
                                         ((name)->sa_family == AF_INET6))
#define SOCK_ADDR_TYPE_MATCH(name, sock) \
       ((((name)->sa_family == AF_INET) && !(NETCONNTYPE_ISIPV6((sock)->conn->type))) || \
       (((name)->sa_family == AF_INET6) && (NETCONNTYPE_ISIPV6((sock)->conn->type))))

static void ipaddr_port_to_sockaddr_safe(union sockaddr_aligned *sockaddr, ip_addr_t *ipaddr, u16_t port)
{
    if (IP_IS_V6_VAL(*(ipaddr))) {
      IP6ADDR_PORT_TO_SOCKADDR((struct sockaddr_in6*)(void*)(sockaddr), ip_2_ip6(ipaddr), port);
    } else {
      IP4ADDR_PORT_TO_SOCKADDR((struct sockaddr_in*)(void*)(sockaddr), ip_2_ip4(ipaddr), port);
    }
}

#define IPADDR_PORT_TO_SOCKADDR_SAFE(sockaddr, ipaddr, port) \
  ipaddr_port_to_sockaddr_safe(sockaddr, ipaddr, port)

#define IPADDR_PORT_TO_SOCKADDR(sockaddr, ipaddr, port) \
  ipaddr_port_to_sockaddr_safe(sockaddr, ipaddr, port)

#define SOCKADDR_TO_IPADDR_PORT(sockaddr, ipaddr, port) sockaddr_to_ipaddr_port(sockaddr, ipaddr, &(port))
#define DOMAIN_TO_NETCONN_TYPE(domain, type) (((domain) == AF_INET6) ? \
  (enum netconn_type)(((uint32_t)(type)) | NETCONN_TYPE_IPV6) : (type))
#elif LWIP_IPV6 /* LWIP_IPV4 && LWIP_IPV6 */
#define IS_SOCK_ADDR_LEN_VALID(namelen)  ((namelen) == sizeof(struct sockaddr_in6))
#define IS_SOCK_ADDR_TYPE_VALID(name)    ((name)->sa_family == AF_INET6)
#define SOCK_ADDR_TYPE_MATCH(name, sock) 1
#define IPADDR_PORT_TO_SOCKADDR(sockaddr, ipaddr, port) \
        IP6ADDR_PORT_TO_SOCKADDR((struct sockaddr_in6*)(void*)(sockaddr), ip_2_ip6(ipaddr), port)
#define IPADDR_PORT_TO_SOCKADDR_SAFE IPADDR_PORT_TO_SOCKADDR
#define SOCKADDR_TO_IPADDR_PORT(sockaddr, ipaddr, port) \
        SOCKADDR6_TO_IP6ADDR_PORT((const struct sockaddr_in6*)(const void*)(sockaddr), ipaddr, port)
#define DOMAIN_TO_NETCONN_TYPE(domain, netconn_type) (netconn_type)
#else /*-> LWIP_IPV4: LWIP_IPV4 && LWIP_IPV6 */
#define IS_SOCK_ADDR_LEN_VALID(namelen)  ((namelen) == sizeof(struct sockaddr_in))
#define IS_SOCK_ADDR_TYPE_VALID(name)    ((name)->sa_family == AF_INET)
#define SOCK_ADDR_TYPE_MATCH(name, sock) 1
#define IPADDR_PORT_TO_SOCKADDR(sockaddr, ipaddr, port) \
        IP4ADDR_PORT_TO_SOCKADDR((struct sockaddr_in*)(void*)(sockaddr), ip_2_ip4(ipaddr), port)
#define IPADDR_PORT_TO_SOCKADDR_SAFE IPADDR_PORT_TO_SOCKADDR
#define SOCKADDR_TO_IPADDR_PORT(sockaddr, ipaddr, port) \
        SOCKADDR4_TO_IP4ADDR_PORT((const struct sockaddr_in*)(const void*)(sockaddr), ipaddr, port)
#define DOMAIN_TO_NETCONN_TYPE(domain, netconn_type) (netconn_type)
#endif /* LWIP_IPV6 */

#define IS_SOCK_ADDR_TYPE_VALID_OR_UNSPEC(name)    (((name)->sa_family == AF_UNSPEC) || \
                                                    IS_SOCK_ADDR_TYPE_VALID(name))
#define SOCK_ADDR_TYPE_MATCH_OR_UNSPEC(name, sock) (((name)->sa_family == AF_UNSPEC) || \
                                                    SOCK_ADDR_TYPE_MATCH(name, sock))
#define IS_SOCK_ADDR_ALIGNED(name)      ((((mem_ptr_t)(name)) % 4) == 0)

#define LWIP_SOCKOPT_CHECK_OPTLEN(optlen, opttype) do { if ((optlen) < (int)sizeof(opttype)) { return EINVAL; } }while (0)
#define LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, optlen, opttype) do { \
  LWIP_SOCKOPT_CHECK_OPTLEN(optlen, opttype); \
  if ((sock)->conn == NULL) { return EINVAL; } } while (0)
#define LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, optlen, opttype) do { \
    int ret = lwip_sockopt_check_optlen_conn_pcb(sock, optlen, sizeof(opttype)); \
    if (ret != 0) { \
      return ret; \
    } \
  } while (0)

#define LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, opttype, netconntype) do { \
  int ret = lwip_sockopt_check_optlen_conn_pcb_type(sock, optlen, sizeof(opttype), netconntype, 0); \
    if (ret != 0) { \
      return ret; \
    } \
  } while (0)

#define LWIP_SOCKOPT_CHECK_IPTYPE_V6(contype) do { if (!(NETCONNTYPE_ISIPV6(contype))) { return ENOPROTOOPT; } }while (0)
#define LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE_IPV6(sock, optlen, opttype, netconntype) do { \
    int ret = lwip_sockopt_check_optlen_conn_pcb_type(sock, optlen, sizeof(opttype), netconntype, 1); \
    if (ret != 0) { \
      return ret; \
    } \
  } while (0)

#define LWIP_SETGETSOCKOPT_DATA_VAR_REF(name)     API_VAR_REF(name)
#define LWIP_SETGETSOCKOPT_DATA_VAR_DECLARE(name) API_VAR_DECLARE(struct lwip_setgetsockopt_data, name)
#define LWIP_SETGETSOCKOPT_DATA_VAR_FREE(name)    API_VAR_FREE(MEMP_SOCKET_SETGETSOCKOPT_DATA, name)
#if LWIP_MPU_COMPATIBLE
#define LWIP_SETGETSOCKOPT_DATA_VAR_ALLOC(name, sock) do { \
  name = (struct lwip_setgetsockopt_data *)memp_malloc(MEMP_SOCKET_SETGETSOCKOPT_DATA); \
  if (name == NULL) { \
    sock_set_errno(sock, ENOMEM); \
    return -1; \
  } } while (0)
#else /* LWIP_MPU_COMPATIBLE */
#define LWIP_SETGETSOCKOPT_DATA_VAR_ALLOC(name, sock)
#endif /* LWIP_MPU_COMPATIBLE */

#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
#define LWIP_SO_SNDRCVTIMEO_OPTTYPE int
#define LWIP_SO_SNDRCVTIMEO_SET(optval, val) (*(int *)(optval) = (val))
#define LWIP_SO_SNDRCVTIMEO_GET_MS(optval)   ((s32_t)*(const int*)(optval))
#define LWIP_SO_SNDRECVTIMO_VALIDATE(optval, _err) (_err) = 0

#else
#define LWIP_SO_SNDRCVTIMEO_OPTTYPE struct timeval
#define LWIP_SO_SNDRCVTIMEO_SET(optval, val)  do { \
  s32_t loc = (val); \
  ((struct timeval *)(optval))->tv_sec = (loc) / MS_PER_SECOND; \
  ((struct timeval *)(optval))->tv_usec = ((loc) % MS_PER_SECOND) * US_PER_MSECOND; } while (0)
#define LWIP_SO_SNDRCVTIMEO_GET_MS(optval) \
  ((((const struct timeval *)(optval))->tv_sec * MS_PER_SECOND) + \
  (((const struct timeval *)(optval))->tv_usec / US_PER_MSECOND))

#define LWIP_MAX_TV_SEC_VAL       ((0x7FFFFFFF / 1000) - 1000)

#define LWIP_SO_SNDRECVTIMO_VALIDATE_RET(optval) do { \
  struct timeval * tv = (struct timeval *)optval;\
  if ((tv->tv_usec < 0) || (tv->tv_sec < 0) || (tv->tv_usec >= 1000000) ||\
      (tv->tv_sec >= LWIP_MAX_TV_SEC_VAL)) {\
        return EDOM;\
      } else if ((tv->tv_sec == 0) && (tv->tv_usec < US_PER_MSECOND) && (tv->tv_usec > 0)){\
        return EINVAL;\
      }\
  } while (0)

#endif


/**
 * This is overridable for the rare case where more than 255 threads
 * select on the same socket...
 */
#ifndef SELWAIT_T
#define SELWAIT_T u8_t
#endif

#if !defined IOV_MAX
#define IOV_MAX 0xFFFF
#elif IOV_MAX > 0xFFFF
#error "IOV_MAX larger than supported by LwIP"
#endif /* IOV_MAX */

/* Contains all internal pointers and states used for a socket */
struct lwip_sock {
  /* sockets currently are built on netconns, each socket has one netconn */
  struct netconn *conn;
  /* data that was left from the previous read */
  void *lastdata;
  /* offset in the data that was left from the previous read */
  u16_t lastoffset;
  /** number of times data was received, set by event_callback(),
      tested by the receive and select functions */
  s16_t rcvevent;
  /** number of times data was ACKed (free send buffer), set by event_callback(),
      tested by select */
  u16_t sendevent;
  /* error happened for this socket, set by event_callback(), tested by select */
  u16_t errevent;
  /* last error that occurred on this socket (in fact, all our errnos fit into an u8_t) */
  atomic_t err;  /* all usage is u8_t , but atomic is int */
  /* counter of how many threads are waiting for this socket using select */
  SELWAIT_T select_waiting;

  atomic_t refcnt;

  atomic_t conn_state;

  /* socket level mutex used for multithread support */
  sys_mutex_t mutex;
#if LWIP_SOCKET_POLL
  /* socket waitqueue */
  wait_queue_head_t wq;
#endif
};

typedef enum _conn_state {
  LWIP_CONN_CLOSED = 0,
  LWIP_CONN_ACTIVE = 1,
  /* move from closing state to active, if closing fails
    [ closing state only used if there are parallel close requests] */
  LWIP_CONN_CLOSING = 2
} lwip_sock_conn_state_e;

#if LWIP_NETCONN_SEM_PER_THREAD
#define SELECT_SEM_T        sys_sem_t*
#define SELECT_SEM_PTR(sem) (sem)
#else /* LWIP_NETCONN_SEM_PER_THREAD */
#define SELECT_SEM_T        sys_sem_t
#define SELECT_SEM_PTR(sem) (&(sem))
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

/* Description for a task waiting in select */
struct lwip_select_cb {
  /* Pointer to the next waiting task */
  struct lwip_select_cb *next;
  /* Pointer to the previous waiting task */
  struct lwip_select_cb *prev;
  /* readset passed to select */
  fd_set *readset;
  /* writeset passed to select */
  fd_set *writeset;
  /* unimplemented: exceptset passed to select */
  fd_set *exceptset;
  /* don't signal the same semaphore twice: set to 1 when signalled */
  int sem_signalled;
  /* semaphore to wake up a task waiting for select */
  SELECT_SEM_T sem;
};

/* Define the number of IPv4/IPV6 multicast memberships, default is one per socket */
#ifndef LWIP_SOCKET_MAX_MEMBERSHIPS
#define LWIP_SOCKET_MAX_MEMBERSHIPS LWIP_CONFIG_NUM_SOCKETS
#endif

#if LWIP_IGMP
/* This is to keep track of IP_ADD_MEMBERSHIP calls to drop the membership when
   a socket is closed */
struct lwip_socket_multicast_pair {
  /* * the socket */
  struct lwip_sock *sock;
  /* * the interface address */
  ip4_addr_t if_addr;
  /* * the group address */
  ip4_addr_t multi_addr;
};

#if LWIP_ALLOW_SOCKET_CONFIG
struct lwip_socket_multicast_pair *socket_ipv4_multicast_memberships = NULL;
#else
struct lwip_socket_multicast_pair socket_ipv4_multicast_memberships[LWIP_SOCKET_MAX_MEMBERSHIPS];
#endif

static int  lwip_socket_register_membership(struct lwip_sock *sock, const ip4_addr_t *if_addr,
                                            const ip4_addr_t *multi_addr);
static void lwip_socket_unregister_membership(struct lwip_sock *sock, const ip4_addr_t *if_addr,
                                              const ip4_addr_t *multi_addr);
static void lwip_socket_drop_registered_memberships(struct lwip_sock *sock);
#endif /* LWIP_IGMP */
#if LWIP_IPV6 && LWIP_IPV6_MLD
/* This is to keep track of IP_JOIN_GROUP calls to drop the membership when a socket is closed */
struct lwip_socket_multicast_mld6_pair {
  /* the socket */
  struct lwip_sock *sock;
  /* the interface index */
  u8_t if_idx;
  /* the group address */
  ip6_addr_t multi_addr;
};

#if LWIP_ALLOW_SOCKET_CONFIG
static struct lwip_socket_multicast_mld6_pair *socket_ipv6_multicast_memberships = NULL;
#else
static struct lwip_socket_multicast_mld6_pair socket_ipv6_multicast_memberships[LWIP_SOCKET_MAX_MEMBERSHIPS];
#endif

static int  lwip_socket_register_mld6_membership(struct lwip_sock *sock, unsigned int if_idx,
                                                 const ip6_addr_t *multi_addr);
static void lwip_socket_unregister_mld6_membership(struct lwip_sock *sock, unsigned int if_idx,
                                                   const ip6_addr_t *multi_addr);
static void lwip_socket_drop_registered_mld6_memberships(struct lwip_sock *sock);
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

/* The global address of available sockets */
#if LWIP_ALLOW_SOCKET_CONFIG
static struct lwip_sock *sockets = NULL;
#else
static struct lwip_sock sockets[LWIP_CONFIG_NUM_SOCKETS];
#endif

#if LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL
#if LWIP_TCPIP_CORE_LOCKING
/* protect the select_cb_list using core lock */
#define LWIP_SOCKET_SELECT_DECL_PROTECT(lev)
#define LWIP_SOCKET_SELECT_PROTECT(lev)   LOCK_TCPIP_CORE()
#define LWIP_SOCKET_SELECT_UNPROTECT(lev) UNLOCK_TCPIP_CORE()
#else /* LWIP_TCPIP_CORE_LOCKING */
/* protect the select_cb_list using SYS_LIGHTWEIGHT_PROT */
#define LWIP_SOCKET_SELECT_DECL_PROTECT(lev)  SYS_ARCH_DECL_PROTECT(lev)
#define LWIP_SOCKET_SELECT_PROTECT(lev)       SYS_ARCH_PROTECT(lev)
#define LWIP_SOCKET_SELECT_UNPROTECT(lev)     SYS_ARCH_UNPROTECT(lev)
/** This counter is increased from lwip_select when the list is changed
    and checked in select_check_waiters to see if it has changed. */
#endif /* LWIP_TCPIP_CORE_LOCKING */
static volatile int select_cb_ctr;
/** The global list of tasks waiting for select */
static struct lwip_select_cb *select_cb_list;
#endif /* LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL */

#define sock_set_errno(sk, e) do { \
  const int sockerr = (e); \
  (void)atomic_set(&sk->err, sockerr);\
  set_errno(sockerr); \
} while (0)

/*
 * Table to quickly map an lwIP error (err_t) to a socket error
 * by using -err as an index
 */
static const int err_to_errno_table[] = {
  0,             /* ERR_OK          0      No error, everything OK. */
  ENOMEM,        /* ERR_MEM        -1      Out of memory error.     */
  ENOBUFS,       /* ERR_BUF        -2      Buffer error.            */
  EWOULDBLOCK,   /* ERR_TIMEOUT    -3      Timeout                  */
  EHOSTUNREACH,  /* ERR_RTE        -4      Routing problem.         */
  EINPROGRESS,   /* ERR_INPROGRESS -5      Operation in progress    */
  EINVAL,        /* ERR_VAL        -6      Illegal value.           */
  EWOULDBLOCK,   /* ERR_WOULDBLOCK -7      Operation would block.   */
  EADDRINUSE,    /* ERR_USE        -8      Address in use.          */
  EALREADY,      /* ERR_ALREADY    -9      Already connecting.      */
  EISCONN,       /* ERR_ISCONN     -10     Conn already established. */
  ENOTCONN,      /* ERR_CONN       -11     Not connected.           */
  -1,            /* ERR_IF         -12     Low-level netif error    */
  EMSGSIZE,      /* ERR_MSGSIZE    -13     Message too long.        */
  ENODEV,        /* ERR_NODEV      -14     No such device.          */
  ENXIO,         /* ERR_NODEVADDR  -15     No such device or address. */
  EDESTADDRREQ,  /* ERR_NODEST     -16     socket is not connection-mode & no peer address is set. */
  ENETDOWN,      /* ERR_NETDOWN    -17     Network is down.         */
  EAFNOSUPPORT,  /* ERR_AFNOSUPP   -18     Address family not supported by protocol. */
  EADDRNOTAVAIL, /* ERR_NOADDR     -19     Cannot assign requested address */
  EOPNOTSUPP,    /* ERR_OPNOTSUPP  -20     Operation not supported on transport endpoint */
  ENETUNREACH,   /* ERR_NETUNREACH -21     No route to the network */
  ETIMEDOUT,     /* ERR_CONNECTIMEOUT -22  Connection request timedout */
  EPIPE,         /* ERR_PIPE       -23     PIPE Error. */
  EAFNOSUPPORT,  /* ERR_AFNOSUPPORT -24     AF not supported */
  ENOPROTOOPT,   /* ERR_NOPROTOOPT -25     Protocol not available */
  EACCES,        /* ERR_ACCESS     -26     Error to access  */
  ENFILE,        /* ERR_NFILE   -27     Socket out of resources */
  EIO,           /* ERR_RESERVE3   -28     Reserved for future use  */
  EIO,           /* ERR_RESERVE4   -29     Reserved for future use  */
  EIO,           /* ERR_RESERVE5   -30     Reserved for future use  */
  EIO,           /* ERR_RESERVE6   -31     Reserved for future use  */
  EIO,           /* ERR_RESERVE7   -32     Reserved for future use  */
  ECONNABORTED,  /* ERR_ABRT       -33     Connection aborted.      */
  ECONNRESET,    /* ERR_RST        -34     Connection reset.        */
  ENOTCONN,      /* ERR_CLSD       -35     Connection closed.       */
  EIO,            /* ERR_ARG        -36    Illegal argument.        */
  ECONNREFUSED,  /* ERR_CONNREFUSED -37     Connection refused.     */
  -2,            /* ERR_MAX_VAL    -38     Invalid error */
};

#define err_to_errno(err) (((err) > 0) || (-(err) >= (err_t)LWIP_ARRAYSIZE(err_to_errno_table)))\
                          ? EIO : err_to_errno_table[-(err)]


#define VALIDATE_SET_RAW_OPTNAME_RET(_sock, _optname) \
    if (((_sock)->conn != NULL && NETCONNTYPE_GROUP((_sock)->conn->type) == NETCONN_RAW) && \
        (((_optname)  != SO_BROADCAST) && ((_optname)  != SO_RCVTIMEO) && \
        ((_optname)  != SO_RCVBUF) && ((_optname) != SO_DONTROUTE) && ((_optname) != SO_BINDTODEVICE) && \
        ((_optname)  != SO_PRIORITY))) { \
      return ENOPROTOOPT; \
    }

#define VALIDATE_GET_RAW_OPTNAME_RET(_sock, _optname) \
    if ((_sock)->conn != NULL && (NETCONNTYPE_GROUP((_sock)->conn->type) == NETCONN_RAW) && \
        (((_optname) != SO_BROADCAST) && ((_optname) != SO_RCVTIMEO) && \
        ((_optname) != SO_RCVBUF) && ((_optname) != SO_TYPE) && \
        ((_optname) != SO_DONTROUTE) && ((_optname) != SO_BINDTODEVICE) && \
        ((_optname) != SO_PRIORITY))) { \
      return ENOPROTOOPT; \
    }


#if PF_PKT_SUPPORT
#define VALIDATE_SET_PF_PKT_OPTNAME_RET(_s, _level, _optname)   \
    if ((_level) == SOL_PACKET && ((_optname) != SO_RCVTIMEO && (_optname) != SO_RCVBUF) &&  \
        (_optname) != SO_ATTACH_FILTER && (_optname) != SO_DETACH_FILTER) { \
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_PACKET, UNIMPL: optname=0x%x, ..)\n",  \
                                 (_s), (_optname)));  \
      return ENOPROTOOPT;  \
    }
#define VALIDATE_GET_PF_PKT_OPTNAME_RET(_s, _level, _optname)   \
    if ((_s)->conn != NULL && (NETCONNTYPE_GROUP((_s)->conn->type) == NETCONN_PKT_RAW) && \
        (_level) == SOL_PACKET && ((_optname) != SO_RCVTIMEO && (_optname) != SO_RCVBUF && (_optname) != SO_TYPE)) { \
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%p, SOL_PACKET, UNIMPL: optname=0x%x, ..)\n",  \
                                 (_s), (_optname)));  \
      return  ENOPROTOOPT;  \
    }

#define VALIDATE_LEVEL_PF_PACKET(_sock, _level)   \
    if ((_sock)->conn != NULL && ((NETCONNTYPE_GROUP((_sock)->conn->type) == NETCONN_PKT_RAW \
      && SOL_SOCKET != (_level) && SOL_PACKET != (_level)) ||  \
      (NETCONNTYPE_GROUP((_sock)->conn->type) != NETCONN_PKT_RAW && SOL_PACKET == (_level))))
#endif

#define LWIP_UPDATE_OPTVAL(_optval_tmp, _optval)   \
    if ((_optval_tmp)) { \
      *(int *)(_optval) = 1;  \
    } else { \
      *(int *)(_optval) = 0; \
    }


/* Forward declaration of some functions */
#if LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL
static void event_callback(struct netconn *conn, enum netconn_evt evt, u32_t len);
#endif

#if !LWIP_TCPIP_CORE_LOCKING
static void lwip_getsockopt_callback(void *arg);
static void lwip_setsockopt_callback(void *arg);
#endif
static int lwip_getsockopt_impl(int s, int level, int optname, void *optval, socklen_t *optlen);
static int lwip_setsockopt_impl(int s, int level, int optname, const void *optval, socklen_t optlen);

#if LWIP_IOCTL_ROUTE
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCADDRT(struct rtentry *rmten);
#endif
#if LWIP_IOCTL_IF
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCGIFCONF(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCGIFADDR(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCSIFADDR(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCSIFADDR_6(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCDIFADDR(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCDIFADDR_6(struct ifreq *ifr);

LWIP_STATIC u8_t lwip_ioctl_internal_SIOCGIFNETMASK(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCSIFNETMASK(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCGIFHWADDR(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCSIFHWADDR(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCGIFFLAGS(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCSIFFLAGS(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCGIFNAME(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCSIFNAME(struct ifreq *ifr);
LWIP_STATIC bool lwip_validate_ifname(const char *name, u8_t *let_pos);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCGIFINDEX(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCSIFMTU(struct ifreq *ifr);
LWIP_STATIC u8_t lwip_ioctl_internal_SIOCGIFMTU(struct ifreq *ifr);
#endif
#if LWIP_NETIF_ETHTOOL
LWIP_STATIC s32_t lwip_ioctl_internal_SIOCETHTOOL(struct ifreq *ifr);
#endif
LWIP_STATIC u8_t lwip_ioctl_internal_FIONBIO(const struct lwip_sock *sock, void *argp);
#if LWIP_SO_RCVBUF || LWIP_FIONREAD_LINUXMODE
LWIP_STATIC u8_t lwip_ioctl_internal_FIONREAD(struct lwip_sock *sock, void *argp);
#endif /* LWIP_SO_RCVBUF || LWIP_FIONREAD_LINUXMODE */
LWIP_STATIC u8_t lwip_ioctl_impl(const struct lwip_sock *sock, long cmd, void *argp);

#if (defined LWIP_TESTBED) || LWIP_ENABLE_BASIC_SHELL_CMD
int get_unused_socket_num(void);
#endif

#if LWIP_IPV4 && LWIP_IPV6
static int lwip_sockopt_check_optlen_conn_pcb(struct lwip_sock *sock, int optlen, int len)
{
  if ((optlen) < len) {
    return EINVAL;
  }

  if (((sock)->conn == NULL) || ((sock)->conn->pcb.tcp == NULL)) {
    return EINVAL;
  }

  return 0;
}

static int lwip_sockopt_check_optlen_conn_pcb_type(struct lwip_sock *sock, int optlen, int len,
                                                   enum netconn_type type, int is_v6)
{
  if ((optlen) < len) {
    return EINVAL;
  }

  if (((sock)->conn == NULL) || ((sock)->conn->pcb.tcp == NULL)) {
    return EINVAL;
  }

  if (is_v6 && !(NETCONNTYPE_ISIPV6(sock->conn->type))) {
    return ENOPROTOOPT;
  }

  if (type != NETCONNTYPE_GROUP(NETCONN_TYPE((sock)->conn))) {
    return ENOPROTOOPT;
  }
  return 0;
}

static void
sockaddr_to_ipaddr_port(const struct sockaddr *sockaddr, ip_addr_t *ipaddr, u16_t *port)
{
  if ((sockaddr == NULL) || (ipaddr == NULL) || (port == NULL)) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("sockaddr_to_ipaddr_port error: invalid args\n"));
    set_errno(EINVAL);
    return;
  }

  if ((sockaddr->sa_family) == AF_INET6) {
    SOCKADDR6_TO_IP6ADDR_PORT((const struct sockaddr_in6 *)(const void *)(sockaddr), ipaddr, *port);
    ipaddr->type = IPADDR_TYPE_V6;
  } else {
    SOCKADDR4_TO_IP4ADDR_PORT((const struct sockaddr_in *)(const void *)(sockaddr), ipaddr, *port);
    ipaddr->type = IPADDR_TYPE_V4;
  }
}
#endif /* LWIP_IPV4 && LWIP_IPV6 */

#if LWIP_NETCONN_SEM_PER_THREAD
/* LWIP_NETCONN_SEM_PER_THREAD==1: initialize thread-local semaphore */
void
lwip_socket_thread_init(void)
{
  netconn_thread_init();
}

/* LWIP_NETCONN_SEM_PER_THREAD==1: destroy thread-local semaphore */
void
lwip_socket_thread_cleanup(void)
{
  netconn_thread_cleanup();
}
#endif

int
sock_init(void)
{
  int i;

#if LWIP_ALLOW_SOCKET_CONFIG
  /* this space will not be free, use it as a static pool. for memory pool configuration purpose */
  if (LWIP_CONFIG_NUM_SOCKETS > LWIP_NUM_SOCKETS_MAX) {
    return ERR_MEM;
  }

  sockets = (struct lwip_sock *)mem_malloc(LWIP_CONFIG_NUM_SOCKETS * sizeof(struct lwip_sock));
  if (sockets == NULL) {
    return ERR_MEM;
  }
  (void)memset_s(sockets, LWIP_CONFIG_NUM_SOCKETS * sizeof(struct lwip_sock),
                 0, LWIP_CONFIG_NUM_SOCKETS * sizeof(struct lwip_sock));
#endif /* LWIP_ALLOW_SOCKET_CONFIG */

  /* allocate a new socket identifier */
  for (i = 0; i < (int)LWIP_CONFIG_NUM_SOCKETS; ++i) {
    if (sys_mutex_new(&sockets[i].mutex) != ERR_OK) {
      goto FAILURE;
    }
  }

  return ERR_OK;
FAILURE:

  /* Free all resources if failure happens */
  for (--i; i >= 0; i--) {
    sys_mutex_free(&sockets[i].mutex);
  }

#if LWIP_ALLOW_SOCKET_CONFIG
  free(sockets);
  sockets = NULL;
#endif
  return ERR_MEM;
}


/*
 * lock lwip_sock parameters from race condition from different thread in parallel
 *
 * @param *sock socket whos paramerters need to be protected.
 * @return void
 */
static inline void
lwip_sock_lock(struct lwip_sock *sock)
{
#if LWIP_COMPAT_MUTEX
  u32_t i_ret;
  i_ret = sys_mutex_lock(&sock->mutex);
  LWIP_ASSERT("sys_mutex_lock failed", (i_ret == 0));
  LWIP_UNUSED_ARG(i_ret);
#else
  sys_mutex_lock(&sock->mutex);
#endif
}


/*
 * unlock lwip_sock parameters for using through different thread in parallel
 *
 * @param *sock socket.
 * @return void
 */
static inline void
lwip_sock_unlock(struct lwip_sock *sock)
{
  sys_mutex_unlock(&sock->mutex);
}


/*
 * Free a socket. The socket's netconn must have been
 * delete before!
 *
 * @param sock the socket to free
 * @param is_tcp != 0 for TCP sockets, used to free lastdata
 */
static void
free_socket(struct lwip_sock *sock, int is_tcp)
{
  void *lastdata = NULL;

  lastdata         = sock->lastdata;
  sock->lastdata   = NULL;
  sock->lastoffset = 0;
  (void)atomic_set(&sock->err, 0);
  (void)atomic_set(&sock->conn_state, LWIP_CONN_CLOSED);

  /* Protect socket array */
  SYS_ARCH_SET(sock->conn, NULL);
  /* don't use 'sock' after this line, as another task might have allocated it */
  if (lastdata != NULL) {
    if (is_tcp) {
      (void)pbuf_free((struct pbuf *)lastdata);
    } else {
      netbuf_delete((struct netbuf *)lastdata);
    }
  }
}

static void
lwip_sock_dec_ref(struct lwip_sock *sock)
{
  int is_tcp = 0;
  if (atomic_dec_and_test(&sock->refcnt)) {
    if (sock->conn != NULL) {
      is_tcp = NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_TCP;
      netconn_finish_delete(sock->conn);
    }
    free_socket(sock, is_tcp);
  }
}


#define SOCKET_REF_MAGIC_NUMBER 0xf0
#define SOCKET_REF_MAGIC_NUMBER_FOR_ADD 0xf1

static inline void
lwip_sock_add_magic_ref(struct lwip_sock *sock)
{
  (void)atomic_add(SOCKET_REF_MAGIC_NUMBER_FOR_ADD, &sock->refcnt);
}

static inline void
lwip_sock_sub_magic_ref(struct lwip_sock *sock)
{
  (void)atomic_sub(SOCKET_REF_MAGIC_NUMBER, &sock->refcnt);
}

/**
 * Map a externally used socket index to the internal socket representation.
 *
 * @param s externally used socket index
 * @return struct lwip_sock for the socket or NULL if not found
 */
static err_t
lwip_sock_check_in(struct lwip_sock *sock)
{
  if (atomic_read(&sock->refcnt) != 0) {
    lwip_sock_add_magic_ref(sock);
    if (atomic_read(&sock->conn_state) == LWIP_CONN_ACTIVE) {
      return ERR_OK;
    }

    lwip_sock_sub_magic_ref(sock);
    lwip_sock_dec_ref(sock);
    return ERR_VAL;
  }
  return ERR_VAL;
}

static void
lwip_sock_check_out(struct lwip_sock *sock)
{
  lwip_sock_sub_magic_ref(sock);
  lwip_sock_dec_ref(sock);
}

#if LWIP_API_RICH
/*
 * Map a externally used socket index to the internal socket representation.
 *
 * @param s externally used socket index
 * @return struct lwip_sock for the socket or NULL if not found
 */
struct lwip_sock *
get_socket(int s)
{
  struct lwip_sock *sock = NULL;

  s -= LWIP_SOCKET_OFFSET;

  if ((s < 0) || (s >= (int)LWIP_CONFIG_NUM_SOCKETS)) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("get_socket(%d): invalid\n", s + LWIP_SOCKET_OFFSET));
    set_errno(EBADF);
    return NULL;
  }

  sock = &sockets[s];

  if (atomic_read(&sock->conn_state) != LWIP_CONN_ACTIVE) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("get_socket(%d): not active\n", s + LWIP_SOCKET_OFFSET));
    set_errno(EBADF);
    return NULL;
  }

  return sock;
}
#endif /* LWIP_API_RICH */

/*
 * Map a externally used socket index to the internal socket representation.
 *
 * @param s externally used socket index
 * @return struct lwip_sock for the socket or NULL if not found
 */
struct lwip_sock *
get_socket_ext(int s)
{
  struct lwip_sock *sock = NULL;

  s -= LWIP_SOCKET_OFFSET;

  if ((s < 0) || (s >= (int)LWIP_CONFIG_NUM_SOCKETS)) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("get_socket(%d): invalid\n", s + LWIP_SOCKET_OFFSET));
    set_errno(EBADF);
    return NULL;
  }

  sock = &sockets[s];

  if (atomic_read(&sock->conn_state) == LWIP_CONN_CLOSED) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("get_socket(%d): not active\n", s + LWIP_SOCKET_OFFSET));
    set_errno(EBADF);
    return NULL;
  }

  return sock;
}

inline static void
lwip_sock_release_socket_reference(struct lwip_sock *sock)
{
  lwip_sock_check_out(sock);
  return;
}

static struct lwip_sock *
lwip_sock_get_socket_reference(int s)
{
  struct lwip_sock *sock = NULL;

  s -= LWIP_SOCKET_OFFSET;

  if ((s < 0) || (s >= (int)LWIP_CONFIG_NUM_SOCKETS)) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("get_socket(%d): invalid\n", s + LWIP_SOCKET_OFFSET));
    set_errno(EBADF);
    return NULL;
  }

  sock = &sockets[s];
  if (lwip_sock_check_in(sock) != ERR_OK) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("get_socket(%d): not active\n", s + LWIP_SOCKET_OFFSET));
    set_errno(EBADF);
    return NULL;
  }

  return sock;
}

/**
 * Same as get_socket but doesn't set errno
 *
 * @param s externally used socket index
 * @return struct lwip_sock for the socket or NULL if not found
 */
static struct lwip_sock *
tryget_socket(int s)
{
  s -= LWIP_SOCKET_OFFSET;
  if ((s < 0) || (s >= (int)LWIP_CONFIG_NUM_SOCKETS)) {
    return NULL;
  }
  if (sockets[s].conn == NULL) {
    return NULL;
  }
  return &sockets[s];
}

#if LWIP_SOCKET_POLL
extern void __init_waitqueue_head(wait_queue_head_t *wait);
#endif

#if (defined LWIP_TESTBED) || LWIP_ENABLE_BASIC_SHELL_CMD
/* get numbers of unused sockets */
int
get_unused_socket_num(void)
{
  int unused = 0, i;
  SYS_ARCH_DECL_PROTECT(lev);

  SYS_ARCH_PROTECT(lev);

  for (i = 0; i < (int)LWIP_CONFIG_NUM_SOCKETS; i++) {
    if (atomic_read(&sockets[i].conn_state) == LWIP_CONN_CLOSED) {
      unused++;
    }
  }

  SYS_ARCH_UNPROTECT(lev);

  return unused;
}
#endif

/*
 * Allocate a new socket for a given netconn.
 *
 * @param newconn the netconn for which to allocate a socket
 * @param accepted 1 if socket has been created by accept(),
 *                 0 if socket has been created by socket()
 * @return the index of the new socket; -1 on error
 */
static int
alloc_socket(struct netconn *newconn, int accepted)
{
  int i;
  SYS_ARCH_DECL_PROTECT(lev);

  /* allocate a new socket identifier */
  for (i = 0; i < (int)LWIP_CONFIG_NUM_SOCKETS; ++i) {
    /* Protect socket array */
    SYS_ARCH_PROTECT(lev);
    if ((sockets[i].conn == NULL) && (sockets[i].select_waiting == 0)) {
      sockets[i].conn       = newconn;
      (void)atomic_set(&sockets[i].conn_state, LWIP_CONN_ACTIVE);
      (void)atomic_set(&sockets[i].refcnt, 1);

      /*
       * The socket is not yet known to anyone, so no need to protect
       *  after having marked it as used.
       */
      SYS_ARCH_UNPROTECT(lev);
      sockets[i].lastdata   = NULL;
      sockets[i].lastoffset = 0;
#if LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL
      sockets[i].rcvevent   = 0;
      /*
       * TCP sendbuf is empty, but the socket is not yet writable until connected
       * (unless it has been created by accept()).
       */
      sockets[i].sendevent  = (NETCONNTYPE_GROUP(newconn->type) == NETCONN_TCP ? (accepted != 0) : 1);
      sockets[i].errevent   = 0;
      (void)atomic_set(&sockets[i].err, 0);
#if LWIP_SOCKET_POLL
      __init_waitqueue_head(&sockets[i].wq);
#endif
#endif
      return i + LWIP_SOCKET_OFFSET;
    }
    SYS_ARCH_UNPROTECT(lev);
  }
  return -1;
}

/*
 * Below this, the well-known socket functions are implemented.
 * Use google.com or opengroup.org to get a good description :-)
 *
 * Exceptions are documented!
 */
int
lwip_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
  struct lwip_sock *sock = NULL;
  struct lwip_sock *nsock = NULL;
  struct netconn *newconn = NULL;
  ip_addr_t naddr;
  u16_t port = 0;
  int newsock;
  err_t err;
  SYS_ARCH_DECL_PROTECT(lev);

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_accept(%d)...\n", s));

  /*
   * Correcting the errno to EINVAL to be consistent with Linux
   * If addr is valid and addrlen is null , stack should return EINVAL
   * If addr is NULL pointer and addrlen is null or valid - ignore the addrlen and function normally as
   * POSIX doesnot mandate addr to be non null pointer
   */
  LWIP_ERROR("Invalid address space\n",
             (((addr != NULL) && (addrlen != NULL)) || (addr == NULL)),
             set_errno(EINVAL); return -1);

  LWIP_ERROR("Invalid address length\n", ((addrlen == NULL) || ((int)(*addrlen) >= 0)),
             set_errno(EINVAL); return -1);

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  LWIP_ERROR("Operation not supported on given socket type\n",
             (NETCONN_TCP == NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn))),
             sock_set_errno(sock, EOPNOTSUPP);
             lwip_sock_release_socket_reference(sock);
             return -1);

  SYS_ARCH_PROTECT(lev);
  if (netconn_is_nonblocking(sock->conn) && (sock->rcvevent <= 0)) {
    SYS_ARCH_UNPROTECT(lev);
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_accept(%d): returning EWOULDBLOCK\n", s));
    sock_set_errno(sock, EWOULDBLOCK);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

  SYS_ARCH_UNPROTECT(lev);

  /* wait for a new connection */
  err = netconn_accept(sock->conn, &newconn);
  if (err != ERR_OK) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_accept(%d): netconn_acept failed, err=%d\n", s, err));
    if (err == ERR_CLSD) {
      sock_set_errno(sock, EINVAL);
    } else {
      sock_set_errno(sock, err_to_errno(err));
    }
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
  LWIP_ASSERT("newconn != NULL", newconn != NULL);

  newsock = alloc_socket(newconn, 1);
  if (newsock == -1) {
    (void)netconn_delete(newconn);
    sock_set_errno(sock, ENFILE);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
  LWIP_ASSERT("invalid socket index",
              (newsock >= LWIP_SOCKET_OFFSET) && (newsock < (int)LWIP_CONFIG_NUM_SOCKETS + LWIP_SOCKET_OFFSET));
  LWIP_ASSERT("newconn->callback == event_callback", newconn->callback == event_callback);
  nsock = &sockets[newsock - LWIP_SOCKET_OFFSET];

  /* See event_callback: If data comes in right away after an accept, even
   * though the server task might not have created a new socket yet.
   * In that case, newconn->socket is counted down (newconn->socket--),
   * so nsock->rcvevent is >= 1 here!
   */
  SYS_ARCH_PROTECT(lev);
  nsock->rcvevent = (s16_t)(nsock->rcvevent - (1 + newconn->socket));
  newconn->socket = newsock;
  SYS_ARCH_UNPROTECT(lev);

  /* Note that POSIX only requires us to check addr is non-NULL. addrlen must
   * not be NULL if addr is valid.
   */
  if ((addr != NULL) && (addrlen != NULL)) {
    union sockaddr_aligned tempaddr;
    socklen_t input_addr_len;
    /* get the IP address and port of the remote host */
    err = netconn_peer(newconn, &naddr, &port);
    if (err != ERR_OK) {
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_accept(%d): netconn_peer failed, err=%d\n", s, err));
      err = ERR_ABRT;
      (void)netconn_delete(newconn);
      free_socket(nsock, 1);
      sock_set_errno(sock, err_to_errno(err));
      lwip_sock_release_socket_reference(sock);
      return -1;
    }
    LWIP_ASSERT("addr valid but addrlen NULL", addrlen != NULL);

    IPADDR_PORT_TO_SOCKADDR_SAFE(&tempaddr, &naddr, port);

    input_addr_len = *addrlen;
    if (IP_IS_V4_VAL(naddr) && (*addrlen > (int)sizeof(struct sockaddr_in))) {
      *addrlen = sizeof(struct sockaddr_in);
    } else if (IP_IS_V6_VAL(naddr) && (*addrlen > (int)sizeof(struct sockaddr_in6))) {
      *addrlen = sizeof(struct sockaddr_in6);
    }

    if (memcpy_s(addr, input_addr_len, &tempaddr, *addrlen) != EOK) {
      err = ERR_MEM;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_accept(%d): memcpy_s failed, err=%d\n", s, err));
      (void)netconn_delete(newconn);
      free_socket(nsock, 1);
      sock_set_errno(sock, err_to_errno(err));
      lwip_sock_release_socket_reference(sock);
      return -1;
    }

    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_accept(%d) returning new sock=%d addr=", s, newsock));
    ip_addr_debug_print_val(SOCKETS_DEBUG, naddr);
    LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F"\n", port));
  } else {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_accept(%d) returning new sock=%d", s, newsock));
  }
  lwip_sock_release_socket_reference(sock);
  return newsock;
}

int
lwip_bind(int s, const struct sockaddr *name, socklen_t namelen)
{
  struct lwip_sock *sock = NULL;
  ip_addr_t local_addr;
  u16_t local_port;
  err_t err;

#if PF_PKT_SUPPORT
  const struct sockaddr_ll *name_ll = NULL;
  u8_t local_ifindex = 0;
#endif

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  /* check size, family and alignment of 'name' */
#if PF_PKT_SUPPORT
  if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) & NETCONN_PKT_RAW) {
    name_ll = (const struct sockaddr_ll *)(void *)name;
    LWIP_ERROR("lwip_bind: invalid address", ((name_ll != NULL) && (namelen == sizeof(struct sockaddr_ll)) &&
                                              ((name_ll->sll_family) == PF_PACKET) && IS_SOCK_ADDR_ALIGNED(name) &&
                                              (name_ll->sll_ifindex <= LWIP_NETIF_IFINDEX_MAX)),
               sock_set_errno(sock, err_to_errno(ERR_VAL));
               lwip_sock_release_socket_reference(sock); return -1);

    ip_addr_set_any(IPADDR_TYPE_V4, &local_addr);
    local_port = name_ll->sll_protocol;
    local_ifindex = (u8_t)name_ll->sll_ifindex;

    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_bind(%d, ", s));
    LWIP_DEBUGF(SOCKETS_DEBUG, (" ifindex=%u proto=%"U16_F")\n", local_ifindex, local_port));
  } else
#endif
  {
    LWIP_ERROR("lwip_bind: invalid address", ((name != NULL) && IS_SOCK_ADDR_LEN_VALID(namelen) &&
                                              IS_SOCK_ADDR_ALIGNED(name)), sock_set_errno(sock, err_to_errno(ERR_VAL));
               lwip_sock_release_socket_reference(sock); return -1);
    LWIP_ERROR("lwip_bind: invalid address",
               (IS_SOCK_ADDR_TYPE_VALID(name)), sock_set_errno(sock, err_to_errno(ERR_AFNOSUPPORT));
               lwip_sock_release_socket_reference(sock); return -1);

    if (!SOCK_ADDR_TYPE_MATCH(name, sock)) {
      /* sockaddr does not match socket type (IPv4/IPv6) */
      sock_set_errno(sock, err_to_errno(ERR_VAL));
      lwip_sock_release_socket_reference(sock);
      return -1;
    }

    LWIP_UNUSED_ARG(namelen);
    SOCKADDR_TO_IPADDR_PORT(name, &local_addr, local_port);

    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_bind(%d, addr=", s));
    ip_addr_debug_print_val(SOCKETS_DEBUG, local_addr);
    LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F")\n", local_port));
  }

#if LWIP_IPV4 && LWIP_IPV6
  /* Dual-stack: Unmap IPv4 mapped IPv6 addresses */
  if (IP_IS_V6_VAL(local_addr) && ip6_addr_isipv4mappedipv6(ip_2_ip6(&local_addr))) {
    unmap_ipv4_mapped_ipv6(ip_2_ip4(&local_addr), ip_2_ip6(&local_addr));
    IP_SET_TYPE_VAL(local_addr, IPADDR_TYPE_V4);
  }
#endif /* LWIP_IPV4 && LWIP_IPV6 */

#if PF_PKT_SUPPORT
  err = netconn_bind(sock->conn, &local_addr, local_port, local_ifindex);
#else
  err = netconn_bind(sock->conn, &local_addr, local_port);
#endif
  if (err != ERR_OK) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_bind(%d) failed, err=%d\n", s, err));
    sock_set_errno(sock, err_to_errno(err));
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_bind(%d) succeeded\n", s));
  lwip_sock_release_socket_reference(sock);
  return 0;
}

int
lwip_close(int s)
{
  struct lwip_sock *sock = NULL;
  err_t err;
  SYS_ARCH_DECL_PROTECT(lev);

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_close(%d)\n", s));

  sock = tryget_socket(s);
  if (sock == NULL) {
    set_errno(EBADF);
    return -1;
  }

  /*
   * Close operation already in progress, can not process multiple close in parallel
   * lock protects - check and set needs to be locked
   * atomic doesn't make sense for close, but make sense for other scenarios,
   * it tell other guys if socket is invalid
   */
  SYS_ARCH_PROTECT(lev);
  if (atomic_read(&sock->conn_state) != LWIP_CONN_ACTIVE) {
    SYS_ARCH_UNPROTECT(lev);
    set_errno(EBADF);
    return -1;
  }

  (void)atomic_set(&sock->conn_state, LWIP_CONN_CLOSING);
  SYS_ARCH_UNPROTECT(lev);

#if LWIP_IGMP
  /* drop all possibly joined IGMP memberships */
  lwip_socket_drop_registered_memberships(sock);
#endif /* LWIP_IGMP */
#if LWIP_IPV6 && LWIP_IPV6_MLD
  /* drop all possibly joined MLD6 memberships */
  lwip_socket_drop_registered_mld6_memberships(sock);
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  LWIP_ASSERT("sock->lastdata == NULL", ((sock->conn != NULL) || (sock->lastdata == NULL)));
  err = netconn_initiate_delete(sock->conn);
  if (err != ERR_OK) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_close(%d) generated internal error, err=%d, \
                but clearing resources and returning SUCCESS\n", s, err));
  }

  /* this may not free socket overall, until all APIs using socket returns */
  lwip_sock_dec_ref(sock);
  return 0;
}

int
lwip_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
  struct lwip_sock *sock = NULL;
  err_t err;

  LWIP_ERROR("lwip_connect: invalid address", (name != NULL),
             set_errno(EINVAL);  return -1);

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

#if PF_PKT_SUPPORT
  if (NETCONN_PKT_RAW & NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn))) {
    sock_set_errno(sock, EOPNOTSUPP);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
#endif

  if (!SOCK_ADDR_TYPE_MATCH_OR_UNSPEC(name, sock)) {
    /* sockaddr does not match socket type (IPv4/IPv6) */
    sock_set_errno(sock, err_to_errno(ERR_VAL));
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

  LWIP_UNUSED_ARG(namelen);
  if (name->sa_family == AF_UNSPEC) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_connect(%d, AF_UNSPEC)\n", s));
    err = netconn_disconnect(sock->conn);
  } else {
    ip_addr_t remote_addr;
    u16_t remote_port;

    /* check size, family and alignment of 'name' */
    LWIP_ERROR("lwip_connect: invalid address", IS_SOCK_ADDR_LEN_VALID(namelen) &&
               IS_SOCK_ADDR_TYPE_VALID_OR_UNSPEC(name) && IS_SOCK_ADDR_ALIGNED(name),
               sock_set_errno(sock, err_to_errno(ERR_VAL));
               lwip_sock_release_socket_reference(sock); return -1);

    LWIP_ERROR("lwip_connect: invalid address family", IS_SOCK_ADDR_TYPE_VALID_OR_UNSPEC(name),
               sock_set_errno(sock, err_to_errno(EAFNOSUPPORT));
               lwip_sock_release_socket_reference(sock); return -1);

    SOCKADDR_TO_IPADDR_PORT(name, &remote_addr, remote_port);
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_connect(%d, addr=", s));
    ip_addr_debug_print_val(SOCKETS_DEBUG, remote_addr);
    LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F")\n", remote_port));

#if LWIP_IPV4 && LWIP_IPV6
    /* Dual-stack: Unmap IPv4 mapped IPv6 addresses */
    if (IP_IS_V6_VAL(remote_addr) && ip6_addr_isipv4mappedipv6(ip_2_ip6(&remote_addr))) {
      unmap_ipv4_mapped_ipv6(ip_2_ip4(&remote_addr), ip_2_ip6(&remote_addr));
      IP_SET_TYPE_VAL(remote_addr, IPADDR_TYPE_V4);
    }
#endif /* LWIP_IPV4 && LWIP_IPV6 */

    err = netconn_connect(sock->conn, &remote_addr, remote_port);
  }

  if (err != ERR_OK) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_connect(%d) failed, err=%d\n", s, err));
    if (err == ERR_RST) {
      sock_set_errno(sock, ECONNREFUSED);
    } else {
      err = (err_t)((err == ERR_CLSD) ? ERR_ABRT : err);
      sock_set_errno(sock, err_to_errno(err));
    }
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_connect(%d) succeeded\n", s));
  lwip_sock_release_socket_reference(sock);
  return 0;
}

/**
 * Set a socket into listen mode.
 * The socket may not have been used for another connection previously.
 *
 * @param s the socket to set to listening mode
 * @param backlog (ATTENTION: needs TCP_LISTEN_BACKLOG=1)
 * @return 0 on success, non-zero on failure
 */
int
lwip_listen(int s, int backlog)
{
  struct lwip_sock *sock = NULL;
  err_t err;

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_listen(%d, backlog=%d)\n", s, backlog));

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_TCP) {
    sock_set_errno(sock, EOPNOTSUPP);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

  /* limit the "backlog" parameter to fit in an u8_t */
  backlog = LWIP_MIN(LWIP_MAX(backlog, 0), TCP_DEFAULT_LISTEN_BACKLOG);

  err = netconn_listen_with_backlog(sock->conn, (u8_t)backlog);
  if (err != ERR_OK) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_listen(%d) failed, err=%d\n", s, err));
    sock_set_errno(sock, err_to_errno(err));
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

  lwip_sock_release_socket_reference(sock);
  return 0;
}

ssize_t
lwip_recvfrom(int s, void *mem, size_t len, int flags,
              struct sockaddr *from, socklen_t *fromlen)
{
  struct lwip_sock *sock = NULL;
  void             *buf = NULL;
  struct pbuf      *p = NULL;
  u16_t            buflen;
  u16_t            copylen;
  int              off = 0;
  u8_t             done = 0;
  err_t            err;
  unsigned int unacceptable_flags;
  ssize_t ret_val = 0;
  SYS_ARCH_DECL_PROTECT(lev);

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom(%d, %p, %"SZT_F", 0x%x, ..)\n", s, mem, len, flags));

  LWIP_ERROR("lwip_recvfrom: invalid arguments", ((mem != NULL)), set_errno(EFAULT); return -1);

  LWIP_ERROR("lwip_recvfrom: invalid arguments",
             ((len != 0) && (flags >= 0) && ((fromlen == NULL) || (*((int *)fromlen) > 0))),
             set_errno(EINVAL); return -1);

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  unacceptable_flags = ~(u32_t)(MSG_PEEK | MSG_DONTWAIT | MSG_NOSIGNAL);
  if (unacceptable_flags & (u32_t)flags) {
    sock_set_errno(sock, EOPNOTSUPP);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

  lwip_sock_lock(sock);

  do {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: top while sock->lastdata=%p\n", sock->lastdata));
    /* Check if there is data left from the last recv operation. */
    if (sock->lastdata != NULL) {
      buf = sock->lastdata;
    } else {
      sock->conn->lrcv_left = 0;

      /* If this is non-blocking call, then check first */
      if ((NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_TCP) &&
          atomic_read(&sock->conn->tcp_connected) != 1) {
        sock_set_errno(sock, ENOTCONN);
        ret_val = -1;
        goto RETURN;
      }
      SYS_ARCH_PROTECT(lev);
      if (((flags & MSG_DONTWAIT) || netconn_is_nonblocking(sock->conn)) &&
          (sock->rcvevent <= 0)) {
        SYS_ARCH_UNPROTECT(lev);
        if (off > 0) {
          /* already received data, return that */
          ret_val =  off;
          goto RETURN;
        }
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom(%d): returning EWOULDBLOCK\n", s));
        sock_set_errno(sock, EWOULDBLOCK);
        ret_val = -1;
        goto RETURN;
      }

      SYS_ARCH_UNPROTECT(lev);

      /* No data was left from the previous operation, so we try to get
         some from the network. */
      if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_TCP) {
        err = netconn_recv_tcp_pbuf(sock->conn, (struct pbuf **)&buf);
      } else {
        err = netconn_recv(sock->conn, (struct netbuf **)&buf);
      }
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: netconn_recv err=%d, netbuf=%p\n",
                                  err, buf));

      if (err != ERR_OK) {
        if (off > 0) {
          if (err == ERR_CLSD) {
            /* closed but already received data, ensure select gets the FIN, too */
            event_callback(sock->conn, NETCONN_EVT_RCVPLUS, 0);
          }
          /* already received data, return that */
          ret_val =  off;
          goto RETURN;
        }
        /* We should really do some error checking here. */
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom(%d): buf == NULL, error is \"%s\"!\n",
                                    s, lwip_strerr(err)));
        if (err == ERR_CLSD) {
          ret_val =  0;
          goto RETURN;
        }
        sock_set_errno(sock, err_to_errno(err));
        ret_val =  -1;
        goto RETURN;
      }
      LWIP_ASSERT("buf != NULL", buf != NULL);
      sock->lastdata = buf;
    }

    if (buf == NULL) {
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: buf is NULL\n"));
      ret_val = -1;
      goto RETURN;
    }

    if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_TCP) {
      p = (struct pbuf *)buf;
    } else {
      p = ((struct netbuf *)buf)->p;
    }
    buflen = p->tot_len;
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: buflen=%"U16_F" len=%"SZT_F" off=%d sock->lastoffset=%"U16_F"\n",
                                buflen, len, off, sock->lastoffset));

    buflen = (u16_t)((u32_t)buflen - (u32_t)sock->lastoffset);
    if (len > buflen) {
      copylen = buflen;
    } else {
      copylen = (u16_t)len;
    }

    /* copy the contents of the received buffer into
    the supplied memory pointer mem */
    (void)pbuf_copy_partial(p, (u8_t *)mem + off, copylen, sock->lastoffset);
    off += copylen;

    if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_TCP) {
      LWIP_ASSERT("invalid copylen, len would underflow", len >= copylen);
      len -= copylen;
      SYS_ARCH_PROTECT(lev);
      if ((len <= 0) ||
          (sock->rcvevent <= 0) ||
          ((flags & MSG_PEEK) != 0)) {
        done = 1;
      }

      SYS_ARCH_UNPROTECT(lev);
    } else {
      done = 1;
    }

    /* Check to see from where the data was. */
    if (done) {
      if ((from != NULL) && (fromlen != NULL)) {
        u16_t port = 0;
        ip_addr_t tmpaddr;
        ip_addr_t *fromaddr = NULL;
        union sockaddr_aligned saddr;
#if PF_PKT_SUPPORT
        struct sockaddr_ll sll;
        struct netbuf *nbuf = (struct netbuf *)buf;
#endif

        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom(%d): addr=", s));
#if PF_PKT_SUPPORT
        if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) & NETCONN_PKT_RAW) {
          /* get eth_hdr type, reuse netbuf port */
          port = netbuf_fromport((struct netbuf *)buf);

          if ((size_t) * fromlen > sizeof(sll)) {
            *fromlen = sizeof(sll);
          }

          if (*fromlen) {
            (void)memset_s(&sll, (unsigned int)sizeof(sll), 0, (unsigned int)sizeof(sll));
            sll.sll_family = PF_PACKET;
            sll.sll_protocol = port;
            sll.sll_hatype = netbuf_fromhatype((struct netbuf *)buf);
            sll.sll_ifindex = netbuf_fromifindex((struct netbuf *)buf);

            if (nbuf->p->flags & PBUF_FLAG_LLBCAST) {
              sll.sll_pkttype = PACKET_BROADCAST;
            } else if (nbuf->p->flags & PBUF_FLAG_LLMCAST) {
              sll.sll_pkttype = PACKET_MULTICAST;
            } else if (nbuf->p->flags & PBUF_FLAG_HOST) {
              sll.sll_pkttype = PACKET_HOST;
            } else if (nbuf->p->flags & PBUF_FLAG_OUTGOING) {
              sll.sll_pkttype = PACKET_OUTGOING;
            } else {
              sll.sll_pkttype = PACKET_OTHERHOST;
            }

            (void)memcpy_s(from, *fromlen, &sll, *fromlen);
            *fromlen = sizeof(sll);

            LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom(%d): hatype=%u", s, ntohs(sll.sll_hatype)));
            LWIP_DEBUGF(SOCKETS_DEBUG, (" packet type = %u\n", sll.sll_pkttype));
          }
        } else
#endif
        {
          if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_TCP) {
            fromaddr = &tmpaddr;

            if (!ERR_IS_FATAL(sock->conn->last_err)) {
              /*
               * MT issue fixed: when shutdown() and recv() called in teo threads,
               * sometime remote_ip:port was incorrect.
               */
              (void)memcpy_s(fromaddr, sizeof(ip_addr_t), &sock->conn->remote_ip, sizeof(ip_addr_t));
              port = sock->conn->remote_port;
            } else {
#if LWIP_TCP
              netconn_trygetaddr(sock->conn, fromaddr, &port);
#endif
            }
          } else {
            port = netbuf_fromport((struct netbuf *)buf);
            fromaddr = netbuf_fromaddr((struct netbuf *)buf);
          }

#if LWIP_IPV4 && LWIP_IPV6
          /* Dual-stack: Map IPv4 addresses to IPv4 mapped IPv6 */
          if (NETCONNTYPE_ISIPV6(NETCONN_TYPE(sock->conn)) && IP_IS_V4_VAL(*fromaddr)) {
            ip4_2_ipv4_mapped_ipv6(ip_2_ip6(fromaddr), ip_2_ip4(fromaddr));
            IP_SET_TYPE_VAL(*fromaddr, IPADDR_TYPE_V6);
          }
#endif /* LWIP_IPV4 && LWIP_IPV6 */

          IPADDR_PORT_TO_SOCKADDR(&saddr, fromaddr, port);
          ip_addr_debug_print(SOCKETS_DEBUG, fromaddr);
          LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F" len=%d\n", port, off));
          if ((from != NULL) && (fromlen != NULL)) {
            if (IP_IS_V4(fromaddr) && *fromlen > (int)sizeof(struct sockaddr_in)) {
              *fromlen = sizeof(struct sockaddr_in);
            } else if (IP_IS_V6(fromaddr) && *fromlen > (int)sizeof(struct sockaddr_in6)) {
              *fromlen = sizeof(struct sockaddr_in6);
            }
            if (*fromlen) {
              (void)memcpy_s((void *)from, *fromlen, &saddr, *fromlen);
            }
          }
        }
      }

      /* If we don't peek the incoming message... */
      if ((flags & MSG_PEEK) == 0) {
        /* If this is a TCP socket, check if there is data left in the
        buffer. If so, it should be saved in the sock structure for next
        time around. */
        if ((NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_TCP) && (buflen - copylen > 0)) {
          sock->lastdata = buf;
          sock->lastoffset = (u16_t)(sock->lastoffset + copylen);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: lastdata now netbuf=%p\n", buf));
        } else {
          sock->lastdata = NULL;
          sock->lastoffset = 0;
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: deleting netbuf=%p\n", buf));
          if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_TCP) {
            (void)pbuf_free((struct pbuf *)buf);
          } else {
            netbuf_delete((struct netbuf *)buf);
          }
          buf = NULL;
        }
      }
    } else {
      if ((NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_TCP) && (buflen - copylen == 0)) {
        sock->lastdata = NULL;
        sock->lastoffset = 0;
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: deleting netbuf=%p\n", buf));
        (void)pbuf_free((struct pbuf *)buf);
        buf = NULL;
      }
    }
  } while (!done);

  /* store data left count on this rcv operation */
  if (sock->lastdata != NULL) {
    sock->conn->lrcv_left = (u32_t)(buflen - sock->lastoffset);
  } else {
    sock->conn->lrcv_left = 0;
  }

  lwip_sock_unlock(sock);
  lwip_sock_release_socket_reference(sock);
  return off;

RETURN:

  lwip_sock_unlock(sock);
  lwip_sock_release_socket_reference(sock);
  return ret_val;
}

ssize_t
lwip_read(int s, void *mem, size_t len)
{
  return lwip_recvfrom(s, mem, len, 0, NULL, NULL);
}

ssize_t
lwip_recv(int s, void *mem, size_t len, int flags)
{
  return lwip_recvfrom(s, mem, len, flags, NULL, NULL);
}

ssize_t
lwip_send(int s, const void *data, size_t size, int flags)
{
  struct lwip_sock *sock = NULL;
  err_t err;
  u32_t write_flags;
  size_t written;
  unsigned int acceptable_flags;

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_send(%d, data=%p, size=%"SZT_F", flags=0x%x)\n",
                              s, data, size, flags));

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_TCP) {
    lwip_sock_release_socket_reference(sock);
#if (LWIP_UDP || LWIP_RAW)
    return lwip_sendto(s, data, size, flags, NULL, 0);
#else /* (LWIP_UDP || LWIP_RAW) */
    sock_set_errno(sock, err_to_errno(ERR_ARG));
    lwip_sock_release_socket_reference(sock);
    return -1;
#endif /* (LWIP_UDP || LWIP_RAW) */
  }

  LWIP_ERROR("lwip_send: invalid arguments", ((data != NULL) && (size != 0) && (flags >= 0)),
             sock_set_errno(sock, err_to_errno(ERR_VAL));
             lwip_sock_release_socket_reference(sock); return -1);

  acceptable_flags = MSG_DONTWAIT | MSG_MORE | MSG_NOSIGNAL;
  LWIP_ERROR("lwip_send: unsupported flags", !((u32_t)(~acceptable_flags) & (u32_t)flags),
             sock_set_errno(sock, EOPNOTSUPP);
             lwip_sock_release_socket_reference(sock);
             return -1);

  write_flags = (u32_t)(NETCONN_COPY |
                        (((u32_t)flags & MSG_MORE)     ? NETCONN_MORE      : 0) |
                        (((u32_t)flags & MSG_DONTWAIT) ? NETCONN_DONTBLOCK : 0));
  written = 0;
  err = netconn_write_partly(sock->conn, data, size, (u8_t)write_flags, &written);

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_send(%d) err=%d written=%"SZT_F"\n", s, err, written));
  if (err != ERR_OK) {
    sock_set_errno(sock, err_to_errno(err));
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
  lwip_sock_release_socket_reference(sock);
  return (int)written;
}

#define LWIP_MAX_SSIZE_T       0x7FFFFFFFU

ssize_t
lwip_sendmsg(int s, const struct msghdr *msg, int flags)
{
  struct lwip_sock *sock = NULL;
  size_t i;
#if LWIP_TCP
  u8_t write_flags;
  size_t written = 0;
#endif
  ssize_t size = 0;
  err_t err;
  unsigned int acceptable_flags;

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  LWIP_ERROR("lwip_sendmsg: invalid msghdr", msg != NULL,
             sock_set_errno(sock, err_to_errno(ERR_ARG));
             lwip_sock_release_socket_reference(sock); return -1);

  LWIP_UNUSED_ARG(msg->msg_control);
  LWIP_UNUSED_ARG(msg->msg_controllen);
  LWIP_UNUSED_ARG(msg->msg_flags);
  LWIP_ERROR("lwip_sendmsg: invalid msghdr iov", ((msg->msg_iov != NULL) && (msg->msg_iovlen > 0)),
             sock_set_errno(sock, err_to_errno(ERR_ARG));
             lwip_sock_release_socket_reference(sock); return -1);

  /* validate incoming packets */
  for (i = 0; i < (size_t)msg->msg_iovlen; i++) {
    /* chekck if total sum overflows ssize_t */
    if ((u64_t)(msg->msg_iov[i].iov_len + written) > LWIP_MAX_SSIZE_T) {
      sock_set_errno(sock, EINVAL);
      lwip_sock_release_socket_reference(sock);
      return -1;
    }
    written +=  msg->msg_iov[i].iov_len;
  }

  written = 0;

  if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_TCP) {
#if LWIP_TCP

    acceptable_flags = MSG_DONTWAIT | MSG_MORE | MSG_NOSIGNAL;
    LWIP_ERROR("lwip_send: unsupported flags", !((u32_t)(~acceptable_flags) & (u32_t)flags),
               sock_set_errno(sock, err_to_errno(EOPNOTSUPP));
               lwip_sock_release_socket_reference(sock); return -1);

    write_flags = (u8_t)(NETCONN_COPY |
                         ((flags & MSG_MORE)     ? NETCONN_MORE      : 0) |
                         ((flags & MSG_DONTWAIT) ? NETCONN_DONTBLOCK : 0));

    for (i = 0; i < (size_t)msg->msg_iovlen; i++) {
      u8_t apiflags = write_flags;
      if (i + 1 < (size_t)msg->msg_iovlen) {
        apiflags |= NETCONN_MORE;
      }
      written = 0;
      err = netconn_write_partly(sock->conn, msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len, apiflags, &written);
      if (err == ERR_OK) {
        size += (ssize_t)written;
        /* check that the entire IO vector was accepected, if not return a partial write */
        if (written != msg->msg_iov[i].iov_len) {
          break;
        }
      } else if ((err == ERR_WOULDBLOCK) && (size > 0)) {
        /* none of this IO vector was accepted, but previous was, return partial write and conceal ERR_WOULDBLOCK */
        err = ERR_OK;
        /* let ERR_WOULDBLOCK persist on the netconn since we are returning ERR_OK */
        break;
      } else {
        size = -1;
        break;
      }
    }

    if (err != ERR_OK) {
      sock_set_errno(sock, err_to_errno(err));
    }
    lwip_sock_release_socket_reference(sock);
    return size;
#else /* LWIP_TCP */

    sock_set_errno(sock, err_to_errno(ERR_ARG));
    lwip_sock_release_socket_reference(sock);
    return -1;
#endif /* LWIP_TCP */
  }
  /* else, UDP and RAW NETCONNs */
#if LWIP_UDP || LWIP_RAW
  {
    struct netbuf *chain_buf;

    LWIP_UNUSED_ARG(flags);
    LWIP_ERROR("lwip_sendmsg: invalid msghdr name", (((msg->msg_name == NULL) && (msg->msg_namelen == 0)) ||
                                                     IS_SOCK_ADDR_LEN_VALID(msg->msg_namelen)),
               sock_set_errno(sock, err_to_errno(ERR_ARG));
               lwip_sock_release_socket_reference(sock); return -1);

    /* initialize chain buffer with destination */
    chain_buf = netbuf_new();
    if (chain_buf == NULL) {
      sock_set_errno(sock, err_to_errno(ERR_MEM));
      lwip_sock_release_socket_reference(sock);
      return -1;
    }
    if (msg->msg_name) {
      u16_t remote_port;
      SOCKADDR_TO_IPADDR_PORT((const struct sockaddr *)msg->msg_name, &chain_buf->addr, remote_port);
      netbuf_fromport(chain_buf) = remote_port;
    }
#if LWIP_NETIF_TX_SINGLE_PBUF
    for (i = 0; i < (size_t)msg->msg_iovlen; i++) {
      size += (ssize_t)msg->msg_iov[i].iov_len;
    }

    /* can not send buffer larger than 0xFFFF */
    if ((size_t)size & (MAX_PBUF_RAM_SIZE_TO_ALLOC << 16)) {
      netbuf_delete(chain_buf);
      sock_set_errno(sock, EMSGSIZE);
      lwip_sock_release_socket_reference(sock);
      return -1;
    }

    /* Allocate a new netbuf and copy the data into it. */
    if (netbuf_alloc(chain_buf, (u16_t)size, NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn))) == NULL) {
      err = ERR_MEM;
    } else {
      /* flatten the IO vectors */
      size_t offset = 0;
      for (i = 0; i < (size_t)msg->msg_iovlen; i++) {
        (void)memcpy((void *)&((u8_t *)chain_buf->p->payload)[offset], (void*)msg->msg_iov[i].iov_base,
                     msg->msg_iov[i].iov_len);
        offset += msg->msg_iov[i].iov_len;
      }
#if LWIP_CHECKSUM_ON_COPY
      {
        /* This can be improved by using LWIP_CHKSUM_COPY() and aggregating the checksum for each IO vector */
        u16_t chksum = (u16_t)(~inet_chksum_pbuf(chain_buf->p));
        netbuf_set_chksum(chain_buf, chksum);
      }
#endif /* LWIP_CHECKSUM_ON_COPY */
      err = ERR_OK;
    }
#else /* LWIP_NETIF_TX_SINGLE_PBUF */
    /* create a chained netbuf from the IO vectors. NOTE: we assemble a pbuf chain
       manually to avoid having to allocate, chain, and delete a netbuf for each iov */
    for (i = 0; i < (size_t)msg->msg_iovlen; i++) {
      struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_REF);
      if (p == NULL) {
        err = ERR_MEM; /* let netbuf_delete() cleanup chain_buf */
        break;
      }
      p->payload = msg->msg_iov[i].iov_base;
      LWIP_ASSERT("iov_len < u16_t", msg->msg_iov[i].iov_len <= 0xFFFF);
      p->len = p->tot_len = (u16_t)msg->msg_iov[i].iov_len;
      /* netbuf empty, add new pbuf */
      if (chain_buf->p == NULL) {
        chain_buf->p = chain_buf->ptr = p;
        /* add pbuf to existing pbuf chain */
      } else {
        pbuf_cat(chain_buf->p, p);
      }
    }
    /* save size of total chain */
    if (err == ERR_OK) {
      size = netbuf_len(chain_buf);
    }
#endif /* LWIP_NETIF_TX_SINGLE_PBUF */

    if (err == ERR_OK) {
#if LWIP_IPV4 && LWIP_IPV6
      /* Dual-stack: Unmap IPv4 mapped IPv6 addresses */
      if (IP_IS_V6_VAL(chain_buf->addr) && ip6_addr_isipv4mappedipv6(ip_2_ip6(&chain_buf->addr))) {
        unmap_ipv4_mapped_ipv6(ip_2_ip4(&chain_buf->addr), ip_2_ip6(&chain_buf->addr));
        IP_SET_TYPE_VAL(chain_buf->addr, IPADDR_TYPE_V4);
      }
#endif /* LWIP_IPV4 && LWIP_IPV6 */

      /* send the data */
      err = netconn_send(sock->conn, chain_buf);
    }

    /* deallocated the buffer */
    netbuf_delete(chain_buf);

    if (err != ERR_OK) {
      sock_set_errno(sock, err_to_errno(err));
      lwip_sock_release_socket_reference(sock);
      return -1;
    }
    lwip_sock_release_socket_reference(sock);
    return size;
  }
#else /* LWIP_UDP || LWIP_RAW */

  sock_set_errno(sock, err_to_errno(ERR_ARG));
  lwip_sock_release_socket_reference(sock);
  return -1;
#endif /* LWIP_UDP || LWIP_RAW */
}

#ifndef LWIP_MAX_PF_RAW_SEND_SIZE
#define LWIP_MAX_PF_RAW_SEND_SIZE (0xFFFF - PBUF_IP_HLEN)
#endif


ssize_t
lwip_sendto(int s, const void *data, size_t size, int flags,
            const struct sockaddr *to, socklen_t tolen)
{
  struct lwip_sock *sock = NULL;
  err_t err;
  u16_t short_size;
  u16_t remote_port = 0;
  struct netbuf buf;

#if PF_PKT_SUPPORT
  const struct sockaddr_in *to_in = NULL;
  const struct sockaddr_ll *to_ll = NULL;
#else
  const struct sockaddr_in *to_in = NULL;
#endif

  SYS_ARCH_DECL_PROTECT(lev);
  unsigned int acceptable_flags;

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_TCP) {
#if LWIP_TCP
    lwip_sock_release_socket_reference(sock);
    return lwip_send(s, data, size, flags);
#else /* LWIP_TCP */
    LWIP_UNUSED_ARG(flags);
    sock_set_errno(sock, err_to_errno(ERR_ARG));
    lwip_sock_release_socket_reference(sock);
    return -1;
#endif /* LWIP_TCP */
  }

#if PF_PKT_SUPPORT
  if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn))  & NETCONN_PKT_RAW) {
    to_ll = (const struct sockaddr_ll *)(void *)to;

    LWIP_ERROR("lwip_sendto: invalid address", (((data != NULL) && (size != 0)) &&
                                                (((to_ll != NULL) && (tolen == sizeof(struct sockaddr_ll)))  &&
                                                 ((to_ll->sll_family) == PF_PACKET) &&
                                                 ((((mem_ptr_t)to_ll) % 4) == 0)) &&
                                                (to_ll->sll_ifindex <= LWIP_NETIF_IFINDEX_MAX)),
               sock_set_errno(sock, err_to_errno(ERR_VAL));
               lwip_sock_release_socket_reference(sock); return -1);

    LWIP_ERROR("lwip_sendto: invalid address family", ((to_ll->sll_family) == PF_PACKET),
               sock_set_errno(sock, err_to_errno(ERR_AFNOSUPP));
               lwip_sock_release_socket_reference(sock); return -1);

    LWIP_ERROR("lwip_sendto: invalid flags. Should be 0", (flags == 0),
               sock_set_errno(sock, err_to_errno(ERR_OPNOTSUPP));
               lwip_sock_release_socket_reference(sock); return -1);

    if (size > LWIP_MAX_PF_RAW_SEND_SIZE) {
      sock_set_errno(sock, EMSGSIZE);
      lwip_sock_release_socket_reference(sock);
      return -1;
    }
  } else
#endif
  {
    to_in = (const struct sockaddr_in *)(void *)to;
    acceptable_flags = MSG_MORE | MSG_DONTWAIT | MSG_NOSIGNAL;
    if (((~acceptable_flags) & (unsigned int)flags) != 0) {
      sock_set_errno(sock, err_to_errno(ERR_OPNOTSUPP));
      lwip_sock_release_socket_reference(sock);
      return -1;
    }

    LWIP_ERROR("lwip_sendto: invalid address", ((data != NULL) && (size != 0) && (flags >= 0)),
               sock_set_errno(sock, err_to_errno(ERR_VAL));
               lwip_sock_release_socket_reference(sock); return -1);

    LWIP_ERROR("lwip_sendto: invalid address", (((to_in == NULL) && (tolen == 0)) ||
                                                (((to_in == NULL) && (tolen != 0)) || IS_SOCK_ADDR_LEN_VALID(tolen))),
               sock_set_errno(sock, err_to_errno(ERR_VAL));
               lwip_sock_release_socket_reference(sock);
               return -1);

#if LWIP_CHECK_ADDR_ALIGN
    LWIP_ERROR("lwip_sendto: invalid address", (IS_SOCK_ADDR_ALIGNED(to_in)),
               sock_set_errno(sock, err_to_errno(ERR_VAL));
               lwip_sock_release_socket_reference(sock);
               return -1);
#endif

    LWIP_ERROR("lwip_sendto: invalid address", (to == NULL || IS_SOCK_ADDR_TYPE_VALID(to)),
               sock_set_errno(sock, err_to_errno(ERR_AFNOSUPPORT));
               lwip_sock_release_socket_reference(sock);
               return -1);

    if (size > LWIP_MAX_UDP_RAW_SEND_SIZE) {
      sock_set_errno(sock, EMSGSIZE);
      lwip_sock_release_socket_reference(sock);
      return -1;
    }
  }

  /* size must fit in u16_t */
  short_size = (u16_t)size;

  LWIP_UNUSED_ARG(tolen);

  /* initialize a buffer */
  buf.p = buf.ptr = NULL;
#if LWIP_CHECKSUM_ON_COPY || PF_PKT_SUPPORT
  buf.flags = 0;
#endif /* LWIP_CHECKSUM_ON_COPY */
#if PF_PKT_SUPPORT
  buf.netifindex = 0;
#endif
  if (to_in != NULL) {
    SOCKADDR_TO_IPADDR_PORT(to, &buf.addr, remote_port);
    netbuf_fromport(&buf) = remote_port;
  } else {
#if PF_PKT_SUPPORT
    if (to_ll != NULL) {
      buf.flags |= NETBUF_FLAG_IFINDEX;
      buf.netifindex = (u8_t)to_ll->sll_ifindex;
    } else
#endif
    {
#if LWIP_UDP

      if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) == NETCONN_UDP) {
        SYS_ARCH_PROTECT(lev);
        if (sock->conn->pcb.udp != NULL && (sock->conn->pcb.udp->flags & UDP_FLAGS_CONNECTED) != 0) {
          SYS_ARCH_UNPROTECT(lev);
          buf.addr = sock->conn->pcb.udp->remote_ip;
          remote_port = sock->conn->pcb.udp->remote_port;
          netbuf_fromport(&buf) = remote_port;
        } else {
          SYS_ARCH_UNPROTECT(lev);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("No address provide for UDP unconnect socket\n"));
          sock_set_errno(sock, err_to_errno(ERR_NODEST));
          lwip_sock_release_socket_reference(sock);
          return -1;
        }
      }
#endif
#if LWIP_RAW
      if ((NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) & NETCONN_RAW) != 0) {
        SYS_ARCH_PROTECT(lev);
        if (sock->conn->pcb.raw != NULL && (sock->conn->pcb.raw->flags & RAW_FLAGS_CONNECTED) != 0) {
          SYS_ARCH_UNPROTECT(lev);
          buf.addr = sock->conn->pcb.raw->remote_ip;
        } else {
          SYS_ARCH_UNPROTECT(lev);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("No address provide for unconnect RAW socket\n"));
          sock_set_errno(sock, err_to_errno(ERR_NODEST));
          lwip_sock_release_socket_reference(sock);
          return -1;
        }
      }
#endif
    }
  }

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_sendto(%d, data=%p, short_size=%"U16_F", flags=0x%x to=",
                              s, data, short_size, flags));

#if PF_PKT_SUPPORT
  if (buf.flags & NETBUF_FLAG_IFINDEX) {
    LWIP_DEBUGF(SOCKETS_DEBUG, (" netifindex = %d\n", buf.netifindex));
  } else
#endif
  {
    ip_addr_debug_print(SOCKETS_DEBUG, &buf.addr);
    LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F"\n", remote_port));
  }
  /* make the buffer point to the data that should be sent */
#if LWIP_NETIF_TX_SINGLE_PBUF
  /* Allocate a new netbuf and copy the data into it. */
  if (netbuf_alloc(&buf, short_size, NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn))) == NULL) {
    err = ERR_MEM;
  } else {
#if LWIP_CHECKSUM_ON_COPY
#if PF_PKT_SUPPORT
    if ((NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_PKT_RAW) &&
        (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_RAW)) {
#else
    if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_RAW) {
#endif
      u16_t chksum = LWIP_CHKSUM_COPY(buf.p->payload, data, short_size);
      netbuf_set_chksum(&buf, chksum);
    } else
#endif /* LWIP_CHECKSUM_ON_COPY */
    {
      err = netbuf_take(&buf, data, short_size);
    }
    err = ERR_OK;
  }
#else /* LWIP_NETIF_TX_SINGLE_PBUF */
  err = netbuf_ref(&buf, data, short_size);
#endif /* LWIP_NETIF_TX_SINGLE_PBUF */
  if (err == ERR_OK) {
#if LWIP_IPV4 && LWIP_IPV6
    /* Dual-stack: Unmap IPv4 mapped IPv6 addresses */
    if (to_in && IP_IS_V6_VAL(buf.addr) && ip6_addr_isipv4mappedipv6(ip_2_ip6(&buf.addr))) {
      unmap_ipv4_mapped_ipv6(ip_2_ip4(&buf.addr), ip_2_ip6(&buf.addr));
      IP_SET_TYPE_VAL(buf.addr, IPADDR_TYPE_V4);
    }
#endif /* LWIP_IPV4 && LWIP_IPV6 */

    /* send the data */
    err = netconn_send(sock->conn, &buf);
  }

  /* deallocated the buffer */
  netbuf_free(&buf);

  if (err != ERR_OK) {
    sock_set_errno(sock, err_to_errno(err));
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

  lwip_sock_release_socket_reference(sock);
  return short_size;
}

int
lwip_socket(int domain, int type, int protocol)
{
  struct netconn *conn = NULL;
  int i;

  LWIP_ERROR("domain invalid\n",  (LWIP_IS_VALID_DOMAIN(domain)),
             set_errno(EAFNOSUPPORT); return -1);

#if PF_PKT_SUPPORT
  LWIP_ERROR("Invalid socket type for PF_PACKET domain\n", ((domain != PF_PACKET) || (type == SOCK_RAW)),
             set_errno(ESOCKTNOSUPPORT); return -1);
#endif

  LWIP_ERROR("flag invalid\n", !(type & ~SOCK_TYPE_MASK),
             set_errno(EINVAL); return -1);

  /* create a netconn */
  switch (type) {
    case SOCK_RAW:
#if PF_PKT_SUPPORT
      if (domain == PF_PACKET) {
        conn = netconn_new_with_proto_and_callback(DOMAIN_TO_NETCONN_TYPE(domain, NETCONN_PKT_RAW),
                                                   (u16_t)protocol, event_callback);
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_socket(%s, SOCK_RAW, %d) = ", "PF_PACKET", protocol));
      } else {
        conn = netconn_new_with_proto_and_callback(DOMAIN_TO_NETCONN_TYPE(domain, NETCONN_RAW),
                                                   (u16_t)protocol, event_callback);
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_socket(%s, SOCK_RAW, %d) = ",
                                    domain == PF_INET ? "PF_INET" : "UNKNOWN", protocol));
      }
#else
      conn = netconn_new_with_proto_and_callback(DOMAIN_TO_NETCONN_TYPE(domain, NETCONN_RAW),
                                                 (u8_t)protocol, event_callback);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_socket(%s, SOCK_RAW, %d) = ",
                                  domain == PF_INET ? "PF_INET" : "UNKNOWN", protocol));
#endif
      break;
    case SOCK_DGRAM:
      LWIP_ERROR("protocol invalid\n",
                 (protocol == IPPROTO_UDPLITE || protocol == IPPROTO_UDP || LWIP_IS_IPPROTOCOL(protocol)),
                 set_errno(EPROTONOSUPPORT); return -1);
      conn = netconn_new_with_callback(DOMAIN_TO_NETCONN_TYPE(domain,
                                                              ((protocol == IPPROTO_UDPLITE) ? NETCONN_UDPLITE :
                                                               NETCONN_UDP)),
                                       event_callback);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_socket(%s:%d, SOCK_DGRAM, %d) = ",
                                  (domain == PF_INET || PF_INET6) ? "PF_INET/PF_INET6" : "UNKNOWN", domain, protocol));
      break;
    case SOCK_STREAM:
      LWIP_ERROR("protocol invalid\n", (protocol == IPPROTO_TCP || LWIP_IS_IPPROTOCOL(protocol)),
                 set_errno(EPROTONOSUPPORT); return -1);
      conn = netconn_new_with_callback(DOMAIN_TO_NETCONN_TYPE(domain, NETCONN_TCP), event_callback);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_socket(%s:%d, SOCK_STREAM, %d) = ",
                                  (domain == PF_INET || PF_INET6) ? "PF_INET/PF_INET6" : "UNKNOWN", domain, protocol));
      break;
    default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_socket(%d, %d/UNKNOWN, %d) = -1\n",
                                  domain, type, protocol));
      set_errno(ESOCKTNOSUPPORT);
      return -1;
  }

  if (conn == NULL) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("-1 / ENOBUFS (could not create netconn)\n"));
    set_errno(ENOBUFS);
    return -1;
  }

  i = alloc_socket(conn, 0);
  if (i == -1) {
    (void)netconn_delete(conn);
    set_errno(ENFILE);
    return -1;
  }
  conn->socket = i;
  LWIP_DEBUGF(SOCKETS_DEBUG, ("%d\n", i));
  return i;
}

int
lwip_write(int s, const void *data, size_t size)
{
  return lwip_send(s, data, size, 0);
}

int
lwip_writev(int s, const struct iovec *iov, int iovcnt)
{
  struct msghdr msg;

  LWIP_ERROR("iovcnt invalid\n", (iovcnt >= 0),
             set_errno(EINVAL); return -1);
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  /* Hack: we have to cast via number to cast from 'const' pointer to non-const.
     Blame the opengroup standard for this inconsistency. */
  msg.msg_iov = LWIP_CONST_CAST(struct iovec *, iov);
  msg.msg_iovlen = (size_t)(unsigned int)iovcnt;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;
  return lwip_sendmsg(s, &msg, 0);
}

#if LWIP_SOCKET_SELECT
/*
 * Go through the readset and writeset lists and see which socket of the sockets
 * set in the sets has events. On return, readset, writeset and exceptset have
 * the sockets enabled that had events.
 *
 * @param maxfdp1 the highest socket index in the sets
 * @param readset_in    set of sockets to check for read events
 * @param writeset_in   set of sockets to check for write events
 * @param exceptset_in  set of sockets to check for error events
 * @param readset_out   set of sockets that had read events
 * @param writeset_out  set of sockets that had write events
 * @param exceptset_out set os sockets that had error events
 * @return number of sockets that had events (read/write/exception) (>= 0)
 */
static int
lwip_selscan(int maxfdp1, fd_set *readset_in, fd_set *writeset_in, fd_set *exceptset_in,
             fd_set *readset_out, fd_set *writeset_out, fd_set *exceptset_out, int *pinvalid)
{
  unsigned int i;
  int nready = 0;
  fd_set lreadset;
  fd_set lwriteset;
  fd_set lexceptset;
  struct lwip_sock *sock = NULL;
  SYS_ARCH_DECL_PROTECT(lev);
  int invalid = 0;

  FD_ZERO(&lreadset);
  FD_ZERO(&lwriteset);
  FD_ZERO(&lexceptset);

  /* Go through each socket in each list to count number of sockets which
     currently match */
  for (i = LWIP_SOCKET_OFFSET; i < (unsigned int)maxfdp1; i++) {
    /* if this FD is not in the set, continue */
    if (!(readset_in && FD_ISSET(i, readset_in)) &&
        !(writeset_in && FD_ISSET(i, writeset_in)) &&
        !(exceptset_in && FD_ISSET(i, exceptset_in))) {
      continue;
    }
    /* First get the socket's status (protected)... */
    SYS_ARCH_PROTECT(lev);
    sock = get_socket_ext((int)i);
    if (sock != NULL) {
      void *lastdata = sock->lastdata;
      s16_t rcvevent = sock->rcvevent;
      u16_t sendevent = sock->sendevent;
      u16_t errevent = sock->errevent;
      SYS_ARCH_UNPROTECT(lev);

      /* ... then examine it: */
      /* See if netconn of this socket is ready for read */
      if (readset_in && FD_ISSET(i, readset_in) && ((lastdata != NULL) || (rcvevent > 0))) {
        FD_SET(i, &lreadset);
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_selscan: fd=%d ready for reading\n", i));
        nready++;
      }
      /* See if netconn of this socket is ready for write */
      if (writeset_in && FD_ISSET(i, writeset_in) && (sendevent != 0)) {
        FD_SET(i, &lwriteset);
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_selscan: fd=%d ready for writing\n", i));
        nready++;
      }
      /* See if netconn of this socket had an error */
      if (exceptset_in && FD_ISSET(i, exceptset_in) && (errevent != 0)) {
        FD_SET(i, &lexceptset);
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_selscan: fd=%d ready for exception\n", i));
        nready++;
      }
    } else {
      SYS_ARCH_UNPROTECT(lev);
      invalid++;
      /* continue on to next FD in list */
    }
  }
  /* copy local sets to the ones provided as arguments */
  (void)memcpy_s(readset_out, sizeof(fd_set), &lreadset, sizeof(fd_set));
  (void)memcpy_s(writeset_out, sizeof(fd_set), &lwriteset, sizeof(fd_set));
  (void)memcpy_s(exceptset_out, sizeof(fd_set), &lexceptset, sizeof(fd_set));

  LWIP_ASSERT("nready >= 0", nready >= 0);
  *pinvalid = invalid;
  return nready;
}

#define LWIP_MAX_SELECT_TIMEOUT_VAL_MSEC 0xFFFFFFFEU /* in milliseconds */

/* Let go all sockets that were marked as used when starting select */
static void
lwip_select_release_sockets_sets_reference(int maxfdp, fd_set *used_sockets)
{
  unsigned int i;
  for (i = LWIP_SOCKET_OFFSET; i < (unsigned int)maxfdp; i++) {
    /* if this FD is not in the set, continue */
    if (FD_ISSET(i, used_sockets)) {
      struct lwip_sock *sock = tryget_socket((int)i);
      LWIP_ASSERT("socket gone at the end of select", sock != NULL);
      if (sock != NULL) {
        lwip_sock_release_socket_reference(sock);
      }
    }
  }
}

static err_t
lwip_select_get_sockets_list_reference(int maxfdp, fd_set *fdset, fd_set *used_sockets)
{
  SYS_ARCH_DECL_PROTECT(lev);
  if (fdset != NULL) {
    unsigned int i;
    for (i = LWIP_SOCKET_OFFSET; i < (unsigned int)maxfdp; i++) {
      /* if this FD is in the set, lock it (unless already done) */
      if (FD_ISSET(i, fdset) && !FD_ISSET(i, used_sockets)) {
        struct lwip_sock *sock = NULL;
        SYS_ARCH_PROTECT(lev);
        sock = lwip_sock_get_socket_reference((int)i);
        if (sock != NULL) {
          /* leave the socket used until released by lwip_select_dec_sockets_used */
          FD_SET(i, used_sockets);
        } else {
          SYS_ARCH_UNPROTECT(lev);
          lwip_select_release_sockets_sets_reference(maxfdp, used_sockets);
          return ERR_VAL;
        }

        SYS_ARCH_UNPROTECT(lev);
      }
    }
  }

  return ERR_OK;
}

/*
 * Mark all sockets passed to select as used to prevent them from being freed
 * from other threads while select is running.
 * Marked sockets are added to 'used_sockets' to mark them only once an be able
 * to unmark them correctly.
 */
static err_t
lwip_select_get_sockets_sets_reference(int maxfdp, fd_set *fdset1, fd_set *fdset2,
                                       fd_set *fdset3, fd_set *used_sockets)
{
  err_t retval;
  FD_ZERO(used_sockets);
  retval = lwip_select_get_sockets_list_reference(maxfdp, fdset1, used_sockets);
  if (retval != ERR_OK) {
    return retval;
  }

  retval = lwip_select_get_sockets_list_reference(maxfdp, fdset2, used_sockets);
  if (retval != ERR_OK) {
    return retval;
  }

  return lwip_select_get_sockets_list_reference(maxfdp, fdset3, used_sockets);
}

int
lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset,
            struct timeval *timeout)
{
  u32_t waitres = 0;
  int nready;
  fd_set lreadset, lwriteset, lexceptset;
  u32_t msectimeout;
  u64_t msectimeout_temp;
  struct lwip_select_cb select_cb;
  fd_set used_set;

  unsigned int i;
  int maxfdp2;
  int invalidfdcnt = 0;
#if LWIP_NETCONN_SEM_PER_THREAD
  int waited = 0;
#endif
  SYS_ARCH_DECL_PROTECT(lev);
  FD_ZERO(&lreadset);
  FD_ZERO(&lwriteset);
  FD_ZERO(&lexceptset);
  LWIP_ERROR("lwip_select: invalid maxfdp1",
             (maxfdp1 > (int)LWIP_SOCKET_OFFSET) && (maxfdp1 <= (int)(LWIP_SOCKET_OFFSET + LWIP_CONFIG_NUM_SOCKETS)),
             set_errno(EINVAL); return -1);

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_select(%d, %p, %p, %p, tvsec=%"S32_F" tvusec=%"S32_F")\n",
                              maxfdp1, (void *)readset, (void *) writeset, (void *) exceptset,
                              timeout ? (s32_t)timeout->tv_sec : (s32_t) -1,
                              timeout ? (s32_t)timeout->tv_usec : (s32_t) -1));
  if (timeout != NULL) {
    /* avoid overflow of timeout duration with very large value of sec or usec */
    if ((timeout->tv_sec < 0) || (timeout->tv_usec < 0)) {
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_select: Invalid value of timeout"));
      set_errno(EINVAL);
      return -1;
    }

    msectimeout_temp = ((u64_t)(unsigned long)timeout->tv_sec) * MS_PER_SECOND;
    if (msectimeout_temp > LWIP_MAX_SELECT_TIMEOUT_VAL_MSEC) {
      set_errno(EINVAL);
      return -1;
    }
    /* 500 : add 500us base on timeout->tv_usec */
    if (((((u64_t)(unsigned long)timeout->tv_usec + 500) / US_PER_MSECOND) + msectimeout_temp) >
        LWIP_MAX_SELECT_TIMEOUT_VAL_MSEC) {
      set_errno(EINVAL);
      return -1;
    }
    /* 500: when tv_usec is less than 500 us, we set is msectimeout 1ms */
    msectimeout = (u32_t)(msectimeout_temp + (((unsigned long)timeout->tv_usec + 500) / US_PER_MSECOND));
    if (msectimeout == 0) {
      /* wait for at least 1 ms */
      msectimeout = 1;
    }
  }

  nready = lwip_select_get_sockets_sets_reference(maxfdp1, readset, writeset, exceptset, &used_set);
  if (nready != ERR_OK) {
    set_errno(EBADF);
    return -1;
  }

  /* Go through each socket in each list to count number of sockets which
     currently match */
  nready = lwip_selscan(maxfdp1, readset, writeset, exceptset, &lreadset, &lwriteset, &lexceptset, &invalidfdcnt);
  if (invalidfdcnt != 0) {
    lwip_select_release_sockets_sets_reference(maxfdp1, &used_set);
    set_errno(EBADF);
    return -1;
  }

  /* If we don't have any current events, then suspend if we are supposed to */
  if (!nready) {
    if ((timeout != NULL) && (timeout->tv_sec == 0) && (timeout->tv_usec == 0)) {
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_select: no timeout, returning 0\n"));
      /* This is OK as the local fdsets are empty and nready is zero,
         or we would have returned earlier. */
      goto return_copy_fdsets;
    }

    /* None ready: add our semaphore to list:
       We don't actually need any dynamic memory. Our entry on the
       list is only valid while we are in this function, so it's ok
       to use local variables. */
    select_cb.next = NULL;
    select_cb.prev = NULL;
    select_cb.readset = readset;
    select_cb.writeset = writeset;
    select_cb.exceptset = exceptset;
    select_cb.sem_signalled = 0;
#if LWIP_NETCONN_SEM_PER_THREAD
    select_cb.sem = LWIP_NETCONN_THREAD_SEM_GET();
#else /* LWIP_NETCONN_SEM_PER_THREAD */
    if (sys_sem_new(&select_cb.sem, 0) != ERR_OK) {
      /* failed to create semaphore */
      lwip_select_release_sockets_sets_reference(maxfdp1, &used_set);
      set_errno(ENOMEM);
      return -1;
    }
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

    /* Protect the select_cb_list */
    SYS_ARCH_PROTECT(lev);

    /* Put this select_cb on top of list */
    select_cb.next = select_cb_list;
    if (select_cb_list != NULL) {
      select_cb_list->prev = &select_cb;
    }
    select_cb_list = &select_cb;
    /* Increasing this counter tells event_callback that the list has changed. */
    select_cb_ctr++;

    /* Now we can safely unprotect */
    SYS_ARCH_UNPROTECT(lev);

    /* Increase select_waiting for each socket we are interested in */
    maxfdp2 = maxfdp1;
    for (i = LWIP_SOCKET_OFFSET; i < (unsigned int)maxfdp1; i++) {
      if ((readset && FD_ISSET(i, readset)) ||
          (writeset && FD_ISSET(i, writeset)) ||
          (exceptset && FD_ISSET(i, exceptset))) {
        struct lwip_sock *sock;
        SYS_ARCH_PROTECT(lev);
        sock = get_socket_ext((int)i);
        if (sock != NULL) {
          sock->select_waiting++;
          LWIP_ASSERT("sock->select_waiting > 0", sock->select_waiting > 0);
        } else {
          /* Not a valid socket */
          nready = -1;
          maxfdp2 = (int)i;
          SYS_ARCH_UNPROTECT(lev);
          break;
        }
        SYS_ARCH_UNPROTECT(lev);
      }
    }

    if (nready >= 0) {
      /* Call lwip_selscan again: there could have been events between
         the last scan (without us on the list) and putting us on the list! */
      nready = lwip_selscan(maxfdp1, readset, writeset, exceptset, &lreadset, &lwriteset, &lexceptset, &invalidfdcnt);
      if (!nready) {
        /* Still none ready, just wait to be woken */
        if (timeout == 0) {
          /* Wait forever */
          msectimeout = 0;
        } else {
          msectimeout = (u32_t)((timeout->tv_sec * MS_PER_SECOND) + ((timeout->tv_usec + 500) / US_PER_MSECOND));
          if (msectimeout == 0) {
            /* Wait 1ms at least (0 means wait forever) */
            msectimeout = 1;
          }
        }

        waitres = sys_arch_sem_wait(SELECT_SEM_PTR(select_cb.sem), msectimeout);
#if LWIP_NETCONN_SEM_PER_THREAD
        waited = 1;
#endif
      }
    }

    /* Decrease select_waiting for each socket we are interested in */
    for (i = LWIP_SOCKET_OFFSET; i < (unsigned int)maxfdp2; i++) {
      if ((readset && FD_ISSET(i, readset)) ||
          (writeset && FD_ISSET(i, writeset)) ||
          (exceptset && FD_ISSET(i, exceptset))) {
        struct lwip_sock *sock;
        SYS_ARCH_PROTECT(lev);
        sock = get_socket_ext((int)i);
        if (sock != NULL) {
          /* for now, handle select_waiting==0... */
          LWIP_ASSERT("sock->select_waiting > 0", sock->select_waiting > 0);
          if (sock->select_waiting > 0) {
            sock->select_waiting--;
          }
        } else {
          /* Not a valid socket */
          nready = -1;
        }
        SYS_ARCH_UNPROTECT(lev);
      }
    }
    /* Take us off the list */
    SYS_ARCH_PROTECT(lev);
    if (select_cb.next != NULL) {
      select_cb.next->prev = select_cb.prev;
    }
    if (select_cb_list == &select_cb) {
      LWIP_ASSERT("select_cb.prev == NULL", select_cb.prev == NULL);
      select_cb_list = select_cb.next;
    } else {
      LWIP_ASSERT("select_cb.prev != NULL", select_cb.prev != NULL);
      select_cb.prev->next = select_cb.next;
    }
    /* Increasing this counter tells event_callback that the list has changed. */
    select_cb_ctr++;
    SYS_ARCH_UNPROTECT(lev);

#if LWIP_NETCONN_SEM_PER_THREAD
    if (select_cb.sem_signalled && (!waited || (waitres == SYS_ARCH_TIMEOUT))) {
      /* don't leave the thread-local semaphore signalled */
      (void)sys_arch_sem_wait(select_cb.sem, 1);
    }
#else /* LWIP_NETCONN_SEM_PER_THREAD */
    sys_sem_free(&select_cb.sem);
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

    if (nready < 0) {
      lwip_select_release_sockets_sets_reference(maxfdp1, &used_set);
      /* This happens when a socket got closed while waiting */
      set_errno(EBADF);
      return -1;
    }

    if (waitres == SYS_ARCH_TIMEOUT) {
      /* Timeout */
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_select: timeout expired\n"));
      /* This is OK as the local fdsets are empty and nready is zero,
         or we would have returned earlier. */
      goto return_copy_fdsets;
    }

    /* See what's set */
    nready = lwip_selscan(maxfdp1, readset, writeset, exceptset, &lreadset, &lwriteset, &lexceptset, &invalidfdcnt);
  }

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_select: nready=%d\n", nready));
return_copy_fdsets:
  if (readset != NULL) {
    (void)memcpy_s(readset, sizeof(fd_set), &lreadset, sizeof(fd_set));
  }
  if (writeset != NULL) {
    (void)memcpy_s(writeset, sizeof(fd_set), &lwriteset, sizeof(fd_set));
  }
  if (exceptset != NULL) {
    (void)memcpy_s(exceptset, sizeof(fd_set), &lexceptset, sizeof(fd_set));
  }

  lwip_select_release_sockets_sets_reference(maxfdp1, &used_set);
  return nready;
}
#endif /* LWIP_SOCKET_SELECT */

#if LWIP_SOCKET_POLL
extern void poll_wait(struct file *filp, wait_queue_head_t *wait_address, poll_table *p);
extern void __wake_up_interruptible_poll(wait_queue_head_t *wait, pollevent_t key);

int
lwip_poll(int sockfd, poll_table *wait)
{
  struct lwip_sock *sock = NULL;
  struct netconn *conn = NULL;
  pollevent_t mask = 0;
  pollevent_t ret;

  SYS_ARCH_DECL_PROTECT(lev);
  LWIP_ASSERT("wait!= NULL", wait != NULL);

  sock = lwip_sock_get_socket_reference(sockfd);
  if (sock == NULL) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_poll: Invalid socket"));
    set_errno(EBADF);
    return -EBADF; /* compatible with file poll */
  }

  conn = sock->conn;

  SYS_ARCH_PROTECT(lev);

  /* listen socket need special poll impl */
  if ((conn->type == SOCK_STREAM) && (conn->state == NETCONN_LISTEN)) {
    /* POLLIN if new connection establised by tcp stack */
    if (sock->rcvevent > 0) {
      mask |= POLLIN | POLLRDNORM;
      goto done;
    }
  }

  /* SHUTDOWN_MASK means both direction was closed due to some errors, eg. RST recved */
  if (conn->shutdown == SHUTDOWN_MASK) {
    mask |= POLLHUP | POLLIN | POLLRDNORM | POLLRDHUP | POLLOUT | POLLWRNORM;
  }

  /* FIN segment recved */
  if (conn->shutdown == RCV_SHUTDOWN) {
    mask |= POLLIN | POLLRDNORM | POLLRDHUP;
  }

  /* normal data readable, socket option SO_RCVLOWAT not support now */
  if ((sock->lastdata != NULL) || (sock->rcvevent > 0)) {
    mask |= POLLIN | POLLRDNORM;
  }

  /* make socket writeable if any space usable */
  if ((conn->shutdown == SND_SHUTDOWN) || (sock->sendevent != 0)) {
    mask |= POLLOUT | POLLWRNORM;
  }

  if (sock->errevent != 0) {
    mask |= POLLERR;
  }

done:

  SYS_ARCH_UNPROTECT(lev);
  ret = mask & wait->key;
  if (ret == 0) {
    /* add socket to wait queue if no event got */
    poll_wait(NULL, &sock->wq, wait);
  }

  lwip_sock_release_socket_reference(sock);
  return ret;
}
#endif /* LWIP_SOCKET_POLL */

#if LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL
/**
 * Callback registered in the netconn layer for each socket-netconn.
 * Processes recvevent (data available) and wakes up tasks waiting for select.
 */
static void
event_callback(struct netconn *conn, enum netconn_evt evt, u32_t len)
{
  int s;
  struct lwip_sock *sock = NULL;
  struct lwip_select_cb *scb = NULL;
  int last_select_cb_ctr;
#if LWIP_SOCKET_POLL
  unsigned int mask = 0;
#endif
  SYS_ARCH_DECL_PROTECT(lev);

  LWIP_UNUSED_ARG(len);

  /* Get socket */
  if (conn != NULL) {
    s = conn->socket;
    if (s < 0) {
      /* Data comes in right away after an accept, even though
       * the server task might not have created a new socket yet.
       * Just count down (or up) if that's the case and we
       * will use the data later. Note that only receive events
       * can happen before the new socket is set up. */
      SYS_ARCH_PROTECT(lev);
      if (conn->socket < 0) {
        if (evt == NETCONN_EVT_RCVPLUS) {
          conn->socket--;
        }
        SYS_ARCH_UNPROTECT(lev);
        return;
      }
      s = conn->socket;
      SYS_ARCH_UNPROTECT(lev);
    }

    sock = get_socket_ext(s);
    if (sock == NULL) {
      return;
    }
  } else {
    return;
  }

  SYS_ARCH_PROTECT(lev);
  /* Set event as required */
  switch (evt) {
    case NETCONN_EVT_RCVPLUS:
      sock->rcvevent++;
      break;
    case NETCONN_EVT_RCVMINUS:
      sock->rcvevent--;
      break;
    case NETCONN_EVT_SENDPLUS:
      sock->sendevent = 1;
      break;
    case NETCONN_EVT_SENDMINUS:
      sock->sendevent = 0;
      break;
    case NETCONN_EVT_ERROR:
      sock->errevent = 1;
      break;
    default:
      LWIP_ASSERT("unknown event", 0);
      break;
  }

  /* Now decide if anyone is waiting for this socket */
  /* NOTE: This code goes through the select_cb_list list multiple times
     ONLY IF a select was actually waiting. We go through the list the number
     of waiting select calls + 1. This list is expected to be small. */
  /* At this point, SYS_ARCH is still protected! */
again:

  if (sock->select_waiting != 0) {
    for (scb = select_cb_list; scb != NULL; scb = scb->next) {
      /* remember the state of select_cb_list to detect changes */
      last_select_cb_ctr = select_cb_ctr;
      if (scb->sem_signalled == 0) {
        /* semaphore not signalled yet */
        int do_signal = 0;
        /* Test this select call for our socket */
        if (sock->rcvevent > 0) {
          if (scb->readset && FD_ISSET((unsigned int)s, scb->readset)) {
            do_signal = 1;
          }
        }
        if (sock->sendevent != 0) {
          if (!do_signal && scb->writeset && FD_ISSET((unsigned int)s, scb->writeset)) {
            do_signal = 1;
          }
        }
        if (sock->errevent != 0) {
          if (!do_signal && scb->exceptset && FD_ISSET((unsigned int)s, scb->exceptset)) {
            do_signal = 1;
          }
        }
        if (do_signal) {
          scb->sem_signalled = 1;
          /* Don't call SYS_ARCH_UNPROTECT() before signaling the semaphore, as this might
             lead to the select thread taking itself off the list, invalidating the semaphore. */
          sys_sem_signal(SELECT_SEM_PTR(scb->sem));
        }
      }
      /* unlock interrupts with each step */
      SYS_ARCH_UNPROTECT(lev);
      /* this makes sure interrupt protection time is short */
      SYS_ARCH_PROTECT(lev);
      if (last_select_cb_ctr != select_cb_ctr) {
        /* someone has changed select_cb_list, restart at the beginning */
        goto again;
      }
    }
  }

#if LWIP_SOCKET_POLL
  /* check shutdown mask firstly */
  switch (conn->shutdown) {
    case SHUTDOWN_MASK:
      mask |= POLLHUP | POLLIN | POLLRDNORM | POLLRDHUP | POLLOUT | POLLWRNORM;
      break;
    case RCV_SHUTDOWN:
      mask |= POLLIN | POLLRDNORM | POLLRDHUP;
      break;
    case SND_SHUTDOWN:
      mask |= POLLOUT | POLLWRNORM;
      break;
    default:
      mask = 0;
  }

  /* wakeup poll task if any data readable, for both listen socket and connected socket */
  if (sock->rcvevent > 0) {
    mask |= (POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND);
  }

  if (sock->sendevent != 0) {
    mask |= (POLLOUT | POLLWRNORM | POLLWRBAND);
  }

  if (sock->errevent != 0) {
    mask |= POLLERR;
  }

  /* try to wakeup the pending task if any */
  if (mask && !LOS_ListEmpty(&(sock->wq.poll_queue))) {
    wake_up_interruptible_poll(&sock->wq, mask);
  }
#endif
  SYS_ARCH_UNPROTECT(lev);
}
#endif /* LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL */

/**
 * Close one end of a full-duplex connection.
 */
int
lwip_shutdown(int s, int how)
{
  struct lwip_sock *sock = NULL;
  err_t err;
  u8_t shut_rx = 0, shut_tx = 0;
  int err_tmp;

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_shutdown(%d, how=%d)\n", s, how));

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  if (sock->conn != NULL) {
    if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_TCP) {
      err_tmp = ENOTCONN;
      goto SYM_ERROR;
    }
  } else {
    err_tmp = ENOTCONN;
    goto SYM_ERROR;
  }

  if (how == SHUT_RD) {
    shut_rx = 1;
  } else if (how == SHUT_WR) {
    shut_tx = 1;
  } else if (how == SHUT_RDWR) {
    shut_rx = 1;
    shut_tx = 1;
  } else {
    err_tmp = EINVAL;
    goto SYM_ERROR;
  }
  err = netconn_shutdown(sock->conn, shut_rx, shut_tx);
  if (err != ERR_OK) {
    err_tmp = err_to_errno(err);
    goto SYM_ERROR;
  }
  lwip_sock_release_socket_reference(sock);
  return 0;

SYM_ERROR:
  sock_set_errno(sock, err_tmp);
  lwip_sock_release_socket_reference(sock);
  return -1;
}
#if LWIP_GETPEERNAME || LWIP_GETSOCKNAME || (LWIP_COMPAT_SOCKETS != 2)
static int
lwip_getaddrname(int s, struct sockaddr *name, socklen_t *namelen, u8_t local)
{
  struct lwip_sock *sock = NULL;
  union sockaddr_aligned saddr;
  ip_addr_t naddr;
  u16_t port;
  err_t err;
  socklen_t outlen;

#if PF_PKT_SUPPORT
  struct sockaddr_ll addr_sin;
  struct netif *netif = NULL;
  struct sockaddr_ll *name_ll = NULL;
#endif

  LWIP_ERROR("lwip_getaddrname: invalid arguments", ((name != NULL) && (namelen != NULL)),
             set_errno(EINVAL); return -1);

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  /* get the IP address and port */
  err = netconn_getaddr(sock->conn, &naddr, &port, local);
  if (err != ERR_OK) {
    sock_set_errno(sock, err_to_errno(err));
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

#if PF_PKT_SUPPORT
  if (NETCONN_PKT_RAW & NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn))) {
    name_ll = (struct sockaddr_ll *)&addr_sin;
    (void)memset_s(&addr_sin, sizeof(struct sockaddr_ll), 0, sizeof(struct sockaddr_ll));
    addr_sin.sll_family = PF_PACKET;
    name_ll->sll_protocol = sock->conn->pcb.pkt_raw->proto.eth_proto;
    name_ll->sll_pkttype = 0;
    name_ll->sll_ifindex = (int)ip4_addr_get_u32(ip_2_ip4(&naddr));
    netif = netif_get_by_index((u8_t)name_ll->sll_ifindex);
    if (netif == NULL) {
      LWIP_DEBUGF(SOCKETS_DEBUG,
                  ("lwip_getaddrname:netif not found for given ifindex (%u)\n", ip4_addr_get_u32(ip_2_ip4(&naddr))));
      name_ll->sll_halen = 0;
      name_ll->sll_hatype = 0;
    } else {
      name_ll->sll_hatype = port;
      name_ll->sll_halen = NETIF_MAX_HWADDR_LEN;
      if (memcpy_s(name_ll->sll_addr, name_ll->sll_halen, netif->hwaddr, name_ll->sll_halen) != EOK) {
        sock_set_errno(sock, err_to_errno(ERR_MEM));
        lwip_sock_release_socket_reference(sock);
        return -1;
      }
    }
    outlen = sizeof(struct sockaddr_ll);
    if (memcpy_s(name, *namelen, &addr_sin, ((*namelen > outlen) ? outlen : *namelen)) != EOK) {
      sock_set_errno(sock, err_to_errno(ERR_MEM));
      lwip_sock_release_socket_reference(sock);
      return -1;
    }
    *namelen = sizeof(struct sockaddr_ll);
    lwip_sock_release_socket_reference(sock);

    return 0;
  }
#endif /* PF_PKT_SUPPORT */

#if LWIP_IPV4 && LWIP_IPV6
  /* Dual-stack: Map IPv4 addresses to IPv4 mapped IPv6 */
  if (NETCONNTYPE_ISIPV6(NETCONN_TYPE(sock->conn)) &&
      IP_IS_V4_VAL(naddr)) {
    ip4_2_ipv4_mapped_ipv6(ip_2_ip6(&naddr), ip_2_ip4(&naddr));
    IP_SET_TYPE_VAL(naddr, IPADDR_TYPE_V6);
  }
#endif /* LWIP_IPV4 && LWIP_IPV6 */

  IPADDR_PORT_TO_SOCKADDR_SAFE(&saddr, &naddr, port);

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getaddrname(%d, addr=", s));
  ip_addr_debug_print_val(SOCKETS_DEBUG, naddr);
  LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F")\n", port));

  if (IP_IS_V6_VAL(naddr)) {
    outlen = sizeof(struct sockaddr_in6);
  } else {
    outlen = sizeof(struct sockaddr_in);
  }

  if (memcpy_s(name, *namelen, &saddr, *namelen > outlen ? outlen : *namelen) != EOK) {
    sock_set_errno(sock, err_to_errno(ERR_MEM));
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
  *namelen = outlen;

  lwip_sock_release_socket_reference(sock);
  return 0;
}
#endif /* LWIP_GETPEERNAME || LWIP_GETSOCKNAME || (LWIP_COMPAT_SOCKETS != 2) */

#if LWIP_GETPEERNAME || (LWIP_COMPAT_SOCKETS != 2)
int
lwip_getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
  return lwip_getaddrname(s, name, namelen, 0);
}
#endif /* LWIP_GETPEERNAME || (LWIP_COMPAT_SOCKETS != 2) */

#if LWIP_GETSOCKNAME || (LWIP_COMPAT_SOCKETS != 2)
int
lwip_getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
  return lwip_getaddrname(s, name, namelen, 1);
}
#endif /* LWIP_GETSOCKNAME || (LWIP_COMPAT_SOCKETS != 2) */

int
lwip_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
  int err;
  struct lwip_sock *sock = NULL;
#if !LWIP_TCPIP_CORE_LOCKING
  err_t cberr;
  LWIP_SETGETSOCKOPT_DATA_VAR_DECLARE(data);
#endif /* !LWIP_TCPIP_CORE_LOCKING */

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  /* values are same as linux */
  if ((optval == NULL) || (optlen == NULL)) {
    sock_set_errno(sock, EFAULT);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

#if PF_PKT_SUPPORT
  VALIDATE_LEVEL_PF_PACKET(sock, level) {
    sock_set_errno(sock, EINVAL);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
#endif /* PF_PKT_SUPPORT */

#if LWIP_TCPIP_CORE_LOCKING
  /* core-locking can just call the -impl function */
  LOCK_TCPIP_CORE();
  err = lwip_getsockopt_impl(s, level, optname, optval, optlen);
  UNLOCK_TCPIP_CORE();

#else /* LWIP_TCPIP_CORE_LOCKING */

#if LWIP_MPU_COMPATIBLE
  /* MPU_COMPATIBLE copies the optval data, so check for max size here */
  if (*optlen > LWIP_SETGETSOCKOPT_MAXOPTLEN) {
    sock_set_errno(sock, ENOBUFS);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
#endif /* LWIP_MPU_COMPATIBLE */

  LWIP_SETGETSOCKOPT_DATA_VAR_ALLOC(data, sock);
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).s = s;
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).level = level;
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).optname = optname;
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).optlen = *optlen;
#if !LWIP_MPU_COMPATIBLE
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).optval.p = optval;
#endif /* !LWIP_MPU_COMPATIBLE */
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).err = 0;
#if LWIP_NETCONN_SEM_PER_THREAD
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).completed_sem = LWIP_NETCONN_THREAD_SEM_GET();
#else
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).completed_sem = &sock->conn->op_completed;
#endif
  cberr = tcpip_callback(lwip_getsockopt_callback, &LWIP_SETGETSOCKOPT_DATA_VAR_REF(data));
  if (cberr != ERR_OK) {
    LWIP_SETGETSOCKOPT_DATA_VAR_FREE(data);
    sock_set_errno(sock, err_to_errno(cberr));
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
  (void)sys_arch_sem_wait((sys_sem_t *)(LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).completed_sem), 0);

  /* write back optlen and optval */
  *optlen = LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).optlen;
#if LWIP_MPU_COMPATIBLE
  (void)memcpy(optval, LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).optval,
               LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).optlen);
#endif /* LWIP_MPU_COMPATIBLE */

  /* maybe lwip_getsockopt_internal has changed err */
  err = LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).err;
  LWIP_SETGETSOCKOPT_DATA_VAR_FREE(data);
#endif /* LWIP_TCPIP_CORE_LOCKING */
  if (err != ERR_OK) {
    sock_set_errno(sock, err);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
  lwip_sock_release_socket_reference(sock);
  return 0;
}

#if !LWIP_TCPIP_CORE_LOCKING
/** lwip_getsockopt_callback: only used without CORE_LOCKING
 * to get into the tcpip_thread
 */
static void
lwip_getsockopt_callback(void *arg)
{
  struct lwip_setgetsockopt_data *data = NULL;
  LWIP_ASSERT("arg != NULL", arg != NULL);
  data = (struct lwip_setgetsockopt_data *)arg;

  data->err = lwip_getsockopt_impl(data->s, data->level, data->optname,
#if LWIP_MPU_COMPATIBLE
                                   data->optval,
#else /* LWIP_MPU_COMPATIBLE */
                                   data->optval.p,
#endif /* LWIP_MPU_COMPATIBLE */
                                   &data->optlen);

  sys_sem_signal((sys_sem_t *)(data->completed_sem));
}
#endif /* LWIP_TCPIP_CORE_LOCKING */

/** lwip_getsockopt_impl: the actual implementation of getsockopt:
 * same argument as lwip_getsockopt, either called directly or through callback
 */
static int
lwip_getsockopt_impl(int s, int level, int optname, void *optval, socklen_t *optlen)
{
  u8_t err = ERR_OK;
  int optval_tmp;
  struct lwip_sock *sock = tryget_socket(s);
  if (sock == NULL) {
    return EBADF;
  }

#ifdef LWIP_HOOK_SOCKETS_GETSOCKOPT
  if (LWIP_HOOK_SOCKETS_GETSOCKOPT(s, sock, level, optname, optval, optlen, &err)) {
    return err;
  }
#endif

  switch (level) {
    /* Level: SOL_SOCKET */
    case SOL_SOCKET:
#if PF_PKT_SUPPORT
    case SOL_PACKET:
      VALIDATE_GET_PF_PKT_OPTNAME_RET(sock, level, optname);
#endif /* PF_PKT_SUPPORT  */
      VALIDATE_GET_RAW_OPTNAME_RET(sock, optname);
      switch (optname) {
#if LWIP_SO_BINDTODEVICE
        case SO_BINDTODEVICE:
          if ((*optlen < IFNAMSIZ) || (sock->conn == NULL) || (sock->conn->pcb.ip == NULL)) {
            return EINVAL;
          }
          if (sock->conn->pcb.ip->ifindex) {
            struct netif *netif = netif_get_by_index(sock->conn->pcb.ip->ifindex);
            if (netif == NULL) {
              *(optlen) = 0;
              return ENODEV;
            } else {
              *(optlen) = 1 + (socklen_t)snprintf_s((char *)optval, IFNAMSIZ, IFNAMSIZ - 1, "%s%d",
                                                    netif->name, netif->num);
            }
          } else {
            *(optlen) = 0;
            return EINVAL;
          }
          break;
#endif /* LWIP_SO_BINDTODEVICE */
#if LWIP_TCP
        case SO_ACCEPTCONN:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, *optlen, int);
          if ((sock->conn != NULL) && NETCONNTYPE_GROUP(sock->conn->type) != NETCONN_TCP) {
            return ENOPROTOOPT;
          }
          if ((sock->conn->pcb.tcp != NULL) && (sock->conn->pcb.tcp->state == LISTEN)) {
            *(int *)optval = 1;
          } else {
            *(int *)optval = 0;
          }
          break;
#endif /* LWIP_TCP */
        /* The option flags */
        case SO_BROADCAST:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, *optlen, int);

          optval_tmp = (int)ip_get_option(sock->conn->pcb.ip, SOF_BROADCAST);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, SOL_SOCKET, optname=0x%x, ..) = %s\n",
                                      s, SOF_BROADCAST, (*(int *)optval ? "on" : "off")));
          LWIP_UPDATE_OPTVAL(optval_tmp, optval);
          break;
#if LWIP_SO_DONTROUTE
        case SO_DONTROUTE:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, *optlen, int);
          optval_tmp = (int)ip_get_option(sock->conn->pcb.ip, SOF_DONTROUTE);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, SOL_SOCKET, optname=0x%x, ..) = %s\n",
                                      s, SOF_DONTROUTE, (optval_tmp ? "on" : "off")));
          LWIP_UPDATE_OPTVAL(optval_tmp, optval);
          break;
#endif /* LWIP_SO_DONTROUTE */

        case SO_KEEPALIVE:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, *optlen, int);

          optval_tmp = (int)ip_get_option(sock->conn->pcb.ip, SOF_KEEPALIVE);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, SOL_SOCKET, optname=0x%x, ..) = %s\n",
                                      s, SOF_KEEPALIVE, (*(int *)optval ? "on" : "off")));
          LWIP_UPDATE_OPTVAL(optval_tmp, optval);
          break;
#if SO_REUSE
        case SO_REUSEADDR:
#endif /* SO_REUSE */
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, *optlen, int);

          optval_tmp = (int)ip_get_option(sock->conn->pcb.ip, SOF_REUSEADDR);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, SOL_SOCKET, optname=0x%x, ..) = %s\n",
                                      s, SOF_REUSEADDR, (*(int *)optval ? "on" : "off")));
          LWIP_UPDATE_OPTVAL(optval_tmp, optval);
          break;

        case SO_TYPE:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, *optlen, int);
          switch (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn))) {
#if PF_PKT_SUPPORT
            case NETCONN_PKT_RAW:
#endif
            case NETCONN_RAW:
              *(int *)optval = SOCK_RAW;
              break;
            case NETCONN_TCP:
              *(int *)optval = SOCK_STREAM;
              break;
            case NETCONN_UDP:
              *(int *)optval = SOCK_DGRAM;
              break;
            default: /* unrecognized socket type */
              *(int *)optval = (int)NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn));

#if PF_PKT_SUPPORT
              LWIP_DEBUGF(SOCKETS_DEBUG,
                          ("lwip_getsockopt(%d, %s, SO_TYPE): unrecognized socket type %u\n",
                           s, (NETCONN_PKT_RAW != NETCONNTYPE_GROUP(sock->conn->type)) ? "SOL_SOCKET" : "SOL_PACKET",
                           *(u32_t *)optval));
#else
              LWIP_DEBUGF(SOCKETS_DEBUG,
                          ("lwip_getsockopt(%d, SOL_SOCKET, SO_TYPE): unrecognized socket type %u\n",
                           s, *(u32_t *)optval));
#endif
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, SOL_SOCKET, SO_TYPE) = %d\n",
                                      s, *(int *)optval));
          break;

        case SO_ERROR:
          LWIP_SOCKOPT_CHECK_OPTLEN(*optlen, int);
          /* only overwrite ERR_OK or temporary errors */
          if (((atomic_read(&sock->err) == 0) || (atomic_read(&sock->err) == EINPROGRESS)) && (sock->conn != NULL)) {
            sock_set_errno(sock, err_to_errno(sock->conn->last_err));
          }
          *(int *)optval = (atomic_read(&sock->err) == 0xFF ? (int) -1 : (int)atomic_read(&sock->err));
          (void)atomic_set(&sock->err, 0);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, SOL_SOCKET, SO_ERROR) = %d\n",
                                      s, *(int *)optval));
          break;

#if LWIP_SO_SNDBUF
        case SO_SNDBUF:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, *optlen, int, NETCONN_TCP);
          *(u32_t *)optval = tcp_get_sendbufsize(sock->conn->pcb.tcp);
          break;
#endif /* LWIP_SO_SNDBUF */

#if LWIP_SO_SNDTIMEO
        case SO_SNDTIMEO:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, *optlen, LWIP_SO_SNDRCVTIMEO_OPTTYPE);
          LWIP_SO_SNDRCVTIMEO_SET(optval, netconn_get_sendtimeout(sock->conn));
          break;
#endif /* LWIP_SO_SNDTIMEO */
#if LWIP_SO_RCVTIMEO
        case SO_RCVTIMEO:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, *optlen, LWIP_SO_SNDRCVTIMEO_OPTTYPE);
          LWIP_SO_SNDRCVTIMEO_SET(optval, netconn_get_recvtimeout(sock->conn));
          break;
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
        case SO_RCVBUF:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, *optlen, int);
          *(int *)optval = netconn_get_recvbufsize(sock->conn);
          break;
#endif /* LWIP_SO_RCVBUF */
#if LWIP_SO_LINGER
        case SO_LINGER: {
          int conn_linger;
          struct linger *linger = (struct linger *)optval;
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, *optlen, struct linger);
          conn_linger = sock->conn->linger;
          if (conn_linger >= 0) {
            linger->l_onoff = 1;
            linger->l_linger = conn_linger;
          } else {
            linger->l_onoff = 0;
            linger->l_linger = 0;
          }
        }
        break;
#endif /* LWIP_SO_LINGER */
#if LWIP_UDP
        case SO_NO_CHECK:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, *optlen, int, NETCONN_UDP);
#if LWIP_UDPLITE
          if ((udp_flags(sock->conn->pcb.udp) & UDP_FLAGS_UDPLITE) != 0) {
            /* this flag is only available for UDP, not for UDP lite */
            return ENOPROTOOPT;
          }
#endif /* LWIP_UDPLITE */
          *(int *)optval = (udp_flags(sock->conn->pcb.udp) & UDP_FLAGS_NOCHKSUM) ? 1 : 0;
          break;
#endif /* LWIP_UDP */
#if LWIP_SO_PRIORITY
        case SO_PRIORITY: {
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, *optlen, prio_t);

#if LWIP_UDP
          if (NETCONNTYPE_GROUP(sock->conn->type) == NETCONN_UDP) {
            *(prio_t *)optval = udp_getpriority(sock->conn->pcb.udp);
          } else
#endif
#if LWIP_TCP
            if (NETCONNTYPE_GROUP(sock->conn->type) == NETCONN_TCP) {
              *(prio_t *)optval = tcp_getpriority(sock->conn->pcb.tcp);
            } else
#endif
#if LWIP_RAW
              if (NETCONNTYPE_GROUP(sock->conn->type) == NETCONN_RAW) {
                *(prio_t *)optval = raw_getpriority(sock->conn->pcb.raw);
              } else
#endif
              {
                LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt Invalid socket type \n"));
                return ENOPROTOOPT;
              }
          break;
        }

#endif /* LWIP_SO_PRIORITY */

        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, SOL_SOCKET, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;

    /* Level: IPPROTO_IP */
    case IPPROTO_IP:
      switch (optname) {
        case IP_TTL:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, *optlen, int);
          *(int *)optval = (int)sock->conn->pcb.ip->ttl;
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, IP_TTL) = %d\n",
                                      s, *(int *)optval));
          break;
        case IP_TOS:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, *optlen, int);
          *(int *)optval = sock->conn->pcb.ip->tos;
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, IP_TOS) = %d\n",
                                      s, *(int *)optval));
          break;
#if LWIP_RAW
        case IP_HDRINCL:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, *optlen, int, NETCONN_RAW);
          if (NETCONNTYPE_ISIPV6(sock->conn->type)) {
            return ENOPROTOOPT;
          }

          *(int *)optval = sock->conn->pcb.raw->hdrincl;
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IP_HDRINCL) = %d\n",
                                      s, *(int *)optval));
          break;
#endif

#if LWIP_IPV4 && LWIP_MULTICAST_TX_OPTIONS
        case IP_MULTICAST_TTL:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, *optlen, u8_t);
          if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_UDP) {
            return ENOPROTOOPT;
          }
          *(u8_t *)optval = udp_get_multicast_ttl(sock->conn->pcb.udp);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, IP_MULTICAST_TTL) = %d\n",
                                      s, *(int *)optval));
          break;
        case IP_MULTICAST_IF:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, *optlen, struct in_addr);
          if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_UDP) {
            return ENOPROTOOPT;
          }
          inet_addr_from_ip4addr((struct in_addr *)optval, udp_get_multicast_netif_addr(sock->conn->pcb.udp));
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, IP_MULTICAST_IF) = 0x%"X32_F"\n",
                                      s, *(u32_t *)optval));
          break;
        case IP_MULTICAST_LOOP:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, *optlen, u8_t, NETCONN_UDP);
          if ((sock->conn->pcb.udp->flags & UDP_FLAGS_MULTICAST_LOOP) != 0) {
            *(u8_t *)optval = 1;
          } else {
            *(u8_t *)optval = 0;
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, IP_MULTICAST_LOOP) = %d\n",
                                      s, *(int *)optval));
          break;
#endif /* LWIP_IPV4 && LWIP_MULTICAST_TX_OPTIONS */
        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;

#if LWIP_TCP
    /* Level: IPPROTO_TCP */
    case IPPROTO_TCP:

#if LWIP_TCP_INFO
      if (optname == TCP_INFO) {
        LWIP_SOCKOPT_CHECK_OPTLEN(*optlen, struct tcp_info);
        if (sock->conn == NULL) {
          return EINVAL;
        }

        if (NETCONNTYPE_GROUP(NETCONN_TYPE((sock)->conn)) != NETCONN_TCP) {
          return ENOPROTOOPT;
        }

        if ((sock->conn == NULL) || (sock->conn->pcb.tcp == NULL)) {
          ((struct tcp_info *)optval)->tcpi_state = TCP_CLOSE;
          break;
        }

        tcp_get_info(sock->conn->pcb.tcp, (struct tcp_info *)optval);
        break;
      }
#endif

      /* Special case: all IPPROTO_TCP option take an int */
      LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, *optlen, int, NETCONN_TCP);
      if (sock->conn->pcb.tcp->state == LISTEN) {
        return EINVAL;
      }
      switch (optname) {
        case TCP_NODELAY:
          *(int *)optval = tcp_nagle_disabled(sock->conn->pcb.tcp);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_TCP, TCP_NODELAY) = %s\n",
                                      s, (*(int *)optval) ? "on" : "off"));
          break;
        case TCP_KEEPALIVE:
          *(int *)optval = (int)sock->conn->pcb.tcp->keep_idle;
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_TCP, TCP_KEEPALIVE) = %d\n",
                                      s, *(int *)optval));
          break;
#if LWIP_TCP_KEEPALIVE
        case TCP_KEEPIDLE:
          *(int *)optval = (int)(sock->conn->pcb.tcp->keep_idle / MS_PER_SECOND);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_TCP, TCP_KEEPIDLE) = %d\n",
                                      s, *(int *)optval));
          break;
        case TCP_KEEPINTVL:
          *(int *)optval = (int)(sock->conn->pcb.tcp->keep_intvl / MS_PER_SECOND);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_TCP, TCP_KEEPINTVL) = %d\n",
                                      s, *(int *)optval));
          break;
        case TCP_KEEPCNT:
          *(int *)optval = (int)sock->conn->pcb.tcp->keep_cnt;
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_TCP, TCP_KEEPCNT) = %d\n",
                                      s, *(int *)optval));
          break;
#endif /* LWIP_TCP_KEEPALIVE */
#if LWIP_SOCK_OPT_TCP_QUEUE_SEQ
        case TCP_QUEUE_SEQ:
          if (*(unsigned int *)optval == TCP_RECV_QUEUE) {
            *(unsigned int *)optval = sock->conn->pcb.tcp->rcv_nxt;
          } else if (*(int *)optval == TCP_SEND_QUEUE) {
            *(unsigned int *)optval = sock->conn->pcb.tcp->snd_nxt;
          } else {
            return EINVAL;
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_TCP, TCP_QUEUE_SEQ) = %d\n",
                                      s, *(int *)optval));
          break;
#endif /* LWIP_SOCK_OPT_TCP_QUEUE_SEQ */
        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_TCP, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;
#endif /* LWIP_TCP */

#if LWIP_IPV6
    /* Level: IPPROTO_IPV6 */
    case IPPROTO_IPV6:
      /* this check is added to ensure that the socket created should be only of type AF_INET6 */
      switch (optname) {
        case IPV6_V6ONLY:

          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, *optlen, int);
          LWIP_SOCKOPT_CHECK_IPTYPE_V6(sock->conn->type);

          *(int *)optval = (netconn_get_ipv6only(sock->conn) ? 1 : 0);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IPV6, IPV6_V6ONLY) = %d\n",
                                      s, *(int *)optval));
          break;
#if LWIP_IPV6 && LWIP_RAW
        case IPV6_CHECKSUM:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE_IPV6(sock, *optlen, int, NETCONN_RAW);
          if (sock->conn->pcb.raw->chksum_reqd == 0) {
            *(int *)optval = -1;
          } else {
            *(int *)optval = sock->conn->pcb.raw->chksum_offset;
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IPV6, IPV6_CHECKSUM) = %d\n",
                                      s, (*(int *)optval)));
          break;
#endif
#if LWIP_SOCK_OPT_IPV6_UNICAST_HOPS
        case IPV6_UNICAST_HOPS:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, *optlen, u8_t);
          LWIP_SOCKOPT_CHECK_IPTYPE_V6(sock->conn->type);

          *(u8_t *)optval = sock->conn->pcb.ip->ttl;
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IPV6, IPV6_UNICAST_HOPS) = %d\n",
                                      s, *(u8_t *)optval));
          break;
#endif
#if LWIP_MULTICAST_TX_OPTIONS
        case IPV6_MULTICAST_HOPS:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE_IPV6(sock, *optlen, u8_t, NETCONN_UDP);

          *(u8_t *)optval = udp_get_multicast_ttl(sock->conn->pcb.udp);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IPV6, IPV6_MULTICAST_HOPS) = %d\n",
                                      s, *(u8_t *)optval));
          break;
#endif /* LWIP_MULTICAST_TX_OPTIONS */
        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IPV6, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;
#endif /* LWIP_IPV6 */

#if LWIP_UDP && LWIP_UDPLITE
    /* Level: IPPROTO_UDPLITE */
    case IPPROTO_UDPLITE:
      /* Special case: all IPPROTO_UDPLITE option take an int */
      LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, *optlen, int);
      /* If this is no UDP lite socket, ignore any options. */
      if (!NETCONNTYPE_ISUDPLITE(NETCONN_TYPE(sock->conn))) {
        return ENOPROTOOPT;
      }
      switch (optname) {
        case UDPLITE_SEND_CSCOV:
          *(int *)optval = sock->conn->pcb.udp->chksum_len_tx;
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_UDPLITE, UDPLITE_SEND_CSCOV) = %d\n",
                                      s, (*(int *)optval)));
          break;
        case UDPLITE_RECV_CSCOV:
          *(int *)optval = sock->conn->pcb.udp->chksum_len_rx;
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_UDPLITE, UDPLITE_RECV_CSCOV) = %d\n",
                                      s, (*(int *)optval)));
          break;
        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_UDPLITE, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;
#endif /* LWIP_UDP */
    /* Level: IPPROTO_RAW */
    case IPPROTO_RAW:
      switch (optname) {
#if LWIP_IPV6 && LWIP_RAW
        case IPV6_CHECKSUM:

          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE_IPV6(sock, *optlen, int, NETCONN_RAW);

          if (sock->conn->pcb.raw->chksum_reqd == 0) {
            *(int *)optval = -1;
          } else {
            *(int *)optval = sock->conn->pcb.raw->chksum_offset;
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_RAW, IPV6_CHECKSUM) = %d\n",
                                      s, (*(int *)optval)));
          break;
#endif /* LWIP_IPV6 && LWIP_RAW */
        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_RAW, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;

      /* [rfc2292 section3.2][12-06-2018][Start] */
      /* Below feature is enabled  only if ipv6 raw socket is enabled */
#if LWIP_IPV6 && LWIP_RAW && LWIP_SOCK_OPT_ICMP6_FILTER
    case IPPROTO_ICMPV6:
      switch (optname) {
        case ICMP6_FILTER:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE_IPV6(sock, *optlen, struct icmp6_filter, NETCONN_RAW);

          if (memcpy_s(optval, *optlen, &sock->conn->pcb.raw->icmp6_filter, sizeof(struct icmp6_filter)) != EOK) {
            err = ENOMEM;
          }
          break;

        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_ICMPV6, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;
#endif

    default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, level=0x%x, UNIMPL: optname=0x%x, ..)\n",
                                  s, level, optname));
      err = ENOPROTOOPT;
      break;
  }

  return err;
}
/**
 *  @page RFC-2553_RFC-3493 RFC-2553/3493
 *
 *  @par Compliant Section
 *  Section 5.1 Unicast Hop Limit
 *  @par Behavior Description
 *  This option controls the hop limit used in outgoing unicast IPv6 packets.\n
 *  Stack allows the IPV6_UNICAST_HOPS value to be retrieved using the lwip_getsockopt() function.\n
 *  Currently, the stack does not allow the Set option for the same. The stack sets the error as ENOPROTOOPT.
 */
/**
 *  @page RFC-2553_RFC-3493 RFC-2553/3493
 *
 *  @par Compliant Section
 *  Section 5.2 Sending and Receiving Multicast Packets
 *
 *  @par Behavior Description
 *  IPV6_MULTICAST_HOPS - Set the hop limit to use for outgoing multicast packets.\n
 *  IPV6_JOIN_GROUP - Join a multicast group on a specified local interface. If the interface index is specified as 0,
 *  no interface will be selected
 *  and EADDRNOTAVAIL will be set. The maximum value of index supported is 254 as of now. \n
 *  IPV6_LEAVE_GROUP - Leave a multicast group on a specified interface.\n
 *  The stack allows the IPV6_MULTICAST_HOPS value to be retrieved using the lwip_getsockopt() function.
 *  Currently, the stack does not allow Set option for the same. Stack will set error as ENOPROTOOPT. \n
 *
 */
/**
 *  @page RFC-2553_RFC-3493 RFC-2553/3493
 *  @par Non-Compliant Section
 *  Section 5.2 Sending and Receiving Multicast Packets
 *  @par Behavior Description
 *  IPV6_MULTICAST_IF - Set the interface to use for outgoing multicast packets.\n
 *  IPV6_MULTICAST_LOOP - If a multicast datagram is sent to a group to which the sending
 *  host itself belongs (on the outgoing interface), a copy of the   datagram is looped back by
 *  the IP layer for local delivery if this option is set to 1.  If this option is set to 0 a copy is
 *  not looped back.  Other option values return an error of EINVAL.\n
 *  Behavior: IPV6_MULTICAST_IF - Stack is not compliant to this option and it is not handled. Stack will return
 *  ENOPROTOOPT for the same.\n
 *  IPV6_MULTICAST_LOOP - Stack is not compliant to this option and it is not handled. Stack will return
 *  ENOPROTOOPT for the same.
 */
/**
 *  @page RFC-2553_RFC-3493 RFC-2553/3493
 *  @par Non-Compliant Section
 *  Section 6.1 Nodename-to-Address Translation
 *
 *  @par Behavior Description
 *  getipnodebyname:\n
 *  The commonly used function gethostbyname() is inadequate for many applications due to the following reasons:\n
 *  - It provides no way for the caller to specify anything about the types of\n
 *  addresses desired (IPv4 only,   IPv6 only, IPv4-mapped IPv6 are OK)\n
 *  - Many   implementations of this function are not thread safe.\n
 *  The stack does not implement the function. RFC-2553 introduces the\n
 *  functions ,but RFC-3493 deprecates the same in favor of getaddrinfo() and getnameinfo().
 */
/**
 *  @page RFC-2553_RFC-3493 RFC-2553/3493
 *  @par Non-Compliant Section
 *  Section 6.2 Address-To-Nodename Translation
 *  @par Behavior Description
 *  getipnodebyaddr:\n
 *  As with getipnodebyname(), getipnodebyaddr() must be thread safe and is an alternative to
 *  gethostbyaddr() \n
 *  The stack does not implement the function. RFC-2553 introduces the
 *  functions, but RFC-3493 deprecates the same in favor of getaddrinfo() and getnameinfo().
 */
/**
 *  @page RFC-2553_RFC-3493 RFC-2553/3493
 *
 *  @par Non-Compliant Section
 *  Section 6.3 Freeing memory for getipnodebyname and getipnodebyaddr
 *  @par Behavior Description
 *  freehostent:\n
 *  The function frees the memory dynamically allocated by getipnodebyname and getipnodebyaddr\n
 *  The stack does not implement the function. RFC-2553 introduces the\n
 *  functions, but RFC-3493 deprecates the same in favor of getaddrinfo() and getnameinfo().
 */
/**
 *  @page RFC-3493 RFC-3493
 *  @par RFC-3493 Compliance Information
 *  @par Compliant Section
 *  Section 5.3  IPV6_V6ONLY option for AF_INET6 Sockets
 *  @par Behavior Description
 *  This socket option restricts AF_INET6 sockets to IPv6 communications only.\n
 *  Stack allows this option for all sock types and is not restricted to TCP type.
 */
int
lwip_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
  int err;
  const void *optvalue = optval;
#if LWIP_SOCK_FILTER && PF_PKT_SUPPORT
  int detachfilteroverride = 0;
#endif
  struct lwip_sock *sock = NULL;
#if !LWIP_TCPIP_CORE_LOCKING
  err_t cberr;
  LWIP_SETGETSOCKOPT_DATA_VAR_DECLARE(data);
#endif /* !LWIP_TCPIP_CORE_LOCKING */

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

#if LWIP_SOCK_FILTER && PF_PKT_SUPPORT
  if (optname == SO_DETACH_FILTER) {
    optvalue = (void *)&detachfilteroverride;
  }
#endif

  if (optvalue == NULL) {
    sock_set_errno(sock, EFAULT);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

#if PF_PKT_SUPPORT
  VALIDATE_LEVEL_PF_PACKET(sock, level) {
    sock_set_errno(sock, EINVAL);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
#endif /* PF_PKT_SUPPORT  */

#if LWIP_TCPIP_CORE_LOCKING
  /* core-locking can just call the -impl function */
  LOCK_TCPIP_CORE();
  err = lwip_setsockopt_impl(s, level, optname, optvalue, optlen);
  UNLOCK_TCPIP_CORE();
#if LWIP_LOWPOWER
  tcpip_send_msg_na(LOW_NON_BLOCK);
#endif

#else /* LWIP_TCPIP_CORE_LOCKING */

#if LWIP_MPU_COMPATIBLE
  /* MPU_COMPATIBLE copies the optval data, so check for max size here */
  if (optlen > LWIP_SETGETSOCKOPT_MAXOPTLEN) {
    sock_set_errno(sock, ENOBUFS);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
#endif /* LWIP_MPU_COMPATIBLE */

  LWIP_SETGETSOCKOPT_DATA_VAR_ALLOC(data, sock);
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).s = s;
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).level = level;
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).optname = optname;
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).optlen = optlen;
#if LWIP_MPU_COMPATIBLE
  (void)memcpy(LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).optval, optvalue, optlen);
#else /* LWIP_MPU_COMPATIBLE */
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).optval.pc = (const void *)optvalue;
#endif /* LWIP_MPU_COMPATIBLE */
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).err = 0;
#if LWIP_NETCONN_SEM_PER_THREAD
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).completed_sem = LWIP_NETCONN_THREAD_SEM_GET();
#else
  LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).completed_sem = &sock->conn->op_completed;
#endif
  cberr = tcpip_callback(lwip_setsockopt_callback, &LWIP_SETGETSOCKOPT_DATA_VAR_REF(data));
  if (cberr != ERR_OK) {
    LWIP_SETGETSOCKOPT_DATA_VAR_FREE(data);
    sock_set_errno(sock, err_to_errno(cberr));
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
  (void)sys_arch_sem_wait((sys_sem_t *)(LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).completed_sem), 0);

  /* maybe lwip_getsockopt_internal has changed err */
  err = LWIP_SETGETSOCKOPT_DATA_VAR_REF(data).err;
  LWIP_SETGETSOCKOPT_DATA_VAR_FREE(data);
#endif /* LWIP_TCPIP_CORE_LOCKING */

  if (err != ERR_OK) {
    sock_set_errno(sock, err);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
  lwip_sock_release_socket_reference(sock);
  return 0;
}

#if !LWIP_TCPIP_CORE_LOCKING
/** lwip_setsockopt_callback: only used without CORE_LOCKING
 * to get into the tcpip_thread
 */
static void
lwip_setsockopt_callback(void *arg)
{
  struct lwip_setgetsockopt_data *data = NULL;
  LWIP_ASSERT("arg != NULL", arg != NULL);
  data = (struct lwip_setgetsockopt_data *)arg;

  data->err = lwip_setsockopt_impl(data->s, data->level, data->optname,
#if LWIP_MPU_COMPATIBLE
                                   data->optval,
#else /* LWIP_MPU_COMPATIBLE */
                                   data->optval.pc,
#endif /* LWIP_MPU_COMPATIBLE */
                                   data->optlen);

  sys_sem_signal((sys_sem_t *)(data->completed_sem));
}
#endif /* LWIP_TCPIP_CORE_LOCKING */

/** lwip_setsockopt_impl: the actual implementation of setsockopt:
 * same argument as lwip_setsockopt, either called directly or through callback
 */
static int
lwip_setsockopt_impl(int s, int level, int optname, const void *optval, socklen_t optlen)
{
  u8_t err = 0;
#if LWIP_SO_BINDTODEVICE
  char ifname[IFNAMSIZ] = {0};
  u8_t namelen;
#endif
  struct lwip_sock *sock = tryget_socket(s);
  if (sock == NULL) {
    return EBADF;
  }

#ifdef LWIP_HOOK_SOCKETS_SETSOCKOPT
  if (LWIP_HOOK_SOCKETS_SETSOCKOPT(s, sock, level, optname, optval, optlen, &err)) {
    return err;
  }
#endif

  switch (level) {
    /* Level: SOL_SOCKET */
    case SOL_SOCKET:
#if PF_PKT_SUPPORT
    case SOL_PACKET:
      VALIDATE_SET_PF_PKT_OPTNAME_RET(s, level, optname);
#endif /* PF_PKT_SUPPORT */
      VALIDATE_SET_RAW_OPTNAME_RET(sock, optname);
      switch (optname) {
          /* SO_ACCEPTCONN is get-only */
          /* The option flags */
#if LWIP_SO_BINDTODEVICE
        case SO_BINDTODEVICE: {
          struct netif *netif = NULL;
          if ((sock->conn == NULL) || (sock->conn->pcb.ip == NULL) || (strlen(optval) > (IFNAMSIZ - 1))) {
            return EINVAL;
          }

          if (optlen > (IFNAMSIZ - 1)) {
            optlen = IFNAMSIZ - 1;
          }

          if (memcpy_s(ifname, IFNAMSIZ - 1, optval, optlen) != EOK) {
            LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_SOCKET, optname=%d, %s, ..) memcpy_s error\n",
                                        s, SO_BINDTODEVICE, ifname));
            return EINVAL;
          }
          namelen = (u8_t)LWIP_MIN(IFNAMSIZ - 1, optlen);
          ifname[namelen] = '\0';
          if (ifname[0] != '\0') {
            netif = netif_find(ifname);
            if (netif != NULL) {
              sock->conn->pcb.ip->ifindex = netif->ifindex;
              LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_SOCKET, optname=%d, %s, ..)\n",
                                          s, SO_BINDTODEVICE, ifname));
            } else {
              return ENODEV;
            }
          } else {
            sock->conn->pcb.ip->ifindex = 0;
          }
          break;
        }
#endif /* LWIP_SO_BINDTODEVICE */
        case SO_BROADCAST:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, optlen, int);
          if (*(const int *)optval) {
            ip_set_option(sock->conn->pcb.ip, (u8_t)SOF_BROADCAST);
          } else {
            ip_reset_option(sock->conn->pcb.ip, (u8_t)SOF_BROADCAST);
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_SOCKET, optname=0x%x, ..) -> %s\n",
                                      s, SOF_BROADCAST, (*(const int *)optval ? "on" : "off")));
          break;
#if LWIP_SOCK_FILTER && PF_PKT_SUPPORT
        case SO_ATTACH_FILTER: {
          struct sock_fprog *filter = NULL;
          /* only AF_PACKET RAW socket support sock filter now */
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, struct sock_fprog, NETCONN_PKT_RAW);
          filter = (struct sock_fprog *)optval;
          err = (u8_t)sock_attach_filter(filter, sock->conn);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_SOCKET, optname=0x%x, ..) = %u\n",
                                      s, SO_ATTACH_FILTER, err));
          break;
        }
        case SO_DETACH_FILTER:
          /* only AF_PACKET RAW socket support sock filter now */
          if (sock->conn == NULL) {
            return EINVAL;
          }
          if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_PKT_RAW) {
            return ENOPROTOOPT;
          }
          err = (u8_t)sock_detach_filter(sock->conn);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_SOCKET, optname=0x%x, ..) = %u\n",
                                      s, SO_DETACH_FILTER, err));
          break;
#endif /* LWIP_SOCK_FILTER && PF_PKT_SUPPORT */

          /* UNIMPL case SO_DEBUG: */
#if LWIP_SO_DONTROUTE
        case SO_DONTROUTE:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, optlen, int);
          if (*(int *)optval) {
            ip_set_option(sock->conn->pcb.ip, SOF_DONTROUTE);
          } else {
            ip_reset_option(sock->conn->pcb.ip, SOF_DONTROUTE);
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_SOCKET, optname=%d, ..) -> %s\n",
                                      s, SO_DONTROUTE, (*(int *)optval ? "on" : "off")));
          break;
#endif /* LWIP_SO_DONTROUTE */
        case SO_KEEPALIVE:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, optlen, int);
          if (*(const int *)optval) {
            ip_set_option(sock->conn->pcb.ip, (u8_t)SOF_KEEPALIVE);
          } else {
            ip_reset_option(sock->conn->pcb.ip, (u8_t)SOF_KEEPALIVE);
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_SOCKET, optname=0x%x, ..) -> %s\n",
                                      s, SOF_KEEPALIVE, (*(const int *)optval ? "on" : "off")));
          break;

#if SO_REUSE
        case SO_REUSEADDR:
#endif /* SO_REUSE */
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, optlen, int);
          if (*(const int *)optval) {
            ip_set_option(sock->conn->pcb.ip, (u8_t)SOF_REUSEADDR);
          } else {
            ip_reset_option(sock->conn->pcb.ip, (u8_t)SOF_REUSEADDR);
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_SOCKET, optname=0x%x, ..) -> %s\n",
                                      s, SOF_REUSEADDR, (*(const int *)optval ? "on" : "off")));
          break;
          /* SO_TYPE is get-only */
          /* SO_ERROR is get-only */
#if LWIP_SO_SNDTIMEO
        case SO_SNDTIMEO:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, optlen, LWIP_SO_SNDRCVTIMEO_OPTTYPE);
          LWIP_SO_SNDRECVTIMO_VALIDATE_RET(optval);
          netconn_set_sendtimeout(sock->conn, LWIP_SO_SNDRCVTIMEO_GET_MS(optval));
          break;
#endif /* LWIP_SO_SNDTIMEO */
#if LWIP_SO_RCVTIMEO
        case SO_RCVTIMEO:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, optlen, LWIP_SO_SNDRCVTIMEO_OPTTYPE);
          LWIP_SO_SNDRECVTIMO_VALIDATE_RET(optval);
          netconn_set_recvtimeout(sock->conn, (int)LWIP_SO_SNDRCVTIMEO_GET_MS(optval));
          break;
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
        case SO_RCVBUF:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, optlen, int);
          if ((*(int *)optval) < RECV_BUFSIZE_MIN) {
            LWIP_DEBUGF(SOCKETS_DEBUG,
                        ("lwip_setsockopt(%d, SOL_SOCKET, optname=0x%x) Invalid value \n", s, SO_RCVBUF));
            return EINVAL;
          }
          netconn_set_recvbufsize(sock->conn, *(const int *)optval);
          break;
#endif /* LWIP_SO_RCVBUF */

#if LWIP_SO_SNDBUF
        case SO_SNDBUF:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, int, NETCONN_TCP);
          if (((sock->conn->pcb.tcp->state != CLOSED) && (sock->conn->pcb.tcp->state != LISTEN))) {
            LWIP_DEBUGF(SOCKETS_DEBUG,
                        ("lwip_setsockopt(%d, SOL_SOCKET, optname=0x%x) Unsupported state \n", s, SO_SNDBUF));
            return EOPNOTSUPP;
          }

          if (((*(int *)optval) < SEND_BUFSIZE_MIN) || ((*(int *)optval) > SEND_BUFSIZE_MAX)) {
            LWIP_DEBUGF(SOCKETS_DEBUG,
                        ("lwip_setsockopt(%d, SOL_SOCKET, optname=0x%x) Invalid value \n", s, SO_SNDBUF));
            return EINVAL;
          } else {
            tcp_set_sendbufsize(sock->conn->pcb.tcp, *(tcpwnd_size_t *)optval);

            if (sock->conn->pcb.tcp->state == CLOSED) {
              tcp_sndbuf(sock->conn->pcb.tcp) = *(tcpwnd_size_t *)optval;
            }
          }
          break;
#endif /* LWIP_SO_SNDBUF */

#if LWIP_SO_LINGER
        case SO_LINGER: {
          const struct linger *linger = (const struct linger *)optval;
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, optlen, struct linger);
          if (linger->l_onoff) {
            int lingersec = linger->l_linger;
            if (lingersec < 0) {
              return EINVAL;
            }
            if (lingersec > 0xFFFF) {
              lingersec = 0xFFFF;
            }
            sock->conn->linger = (s16_t)lingersec;
          } else {
            sock->conn->linger = -1;
          }
          break;
        }

#endif /* LWIP_SO_LINGER */
#if LWIP_UDP
        case SO_NO_CHECK:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, int, NETCONN_UDP);
#if LWIP_UDPLITE
          if ((udp_flags(sock->conn->pcb.udp) & UDP_FLAGS_UDPLITE) != 0) {
            /* this flag is only available for UDP, not for UDP lite */
            return EAFNOSUPPORT;
          }
#endif /* LWIP_UDPLITE */
          if (*(const int *)optval) {
            udp_setflags(sock->conn->pcb.udp, udp_flags(sock->conn->pcb.udp) | UDP_FLAGS_NOCHKSUM);
          } else {
            udp_setflags(sock->conn->pcb.udp, udp_flags(sock->conn->pcb.udp) & (u8_t) ~UDP_FLAGS_NOCHKSUM);
          }
          break;
#endif /* LWIP_UDP */
#if LWIP_SO_PRIORITY
        case SO_PRIORITY: {
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, optlen, prio_t);
          if ((*(prio_t *)optval) > LWIP_PKT_PRIORITY_MAX
#if LWIP_PLC
              || (*(prio_t *)optval) < LWIP_PKT_PRIORITY_MIN
#endif /* LWIP_PLC */
             ) {
            LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SO_PRIORITY, optname=0x%x) Invalid value \n", s,
                                        SO_PRIORITY));
            return EINVAL;
          }
#if LWIP_UDP
          if (NETCONNTYPE_GROUP(sock->conn->type) == NETCONN_UDP) {
            sock->conn->pcb.udp->priority = *(const prio_t *)optval;
          } else
#endif
#if LWIP_TCP
            if (NETCONNTYPE_GROUP(sock->conn->type) == NETCONN_TCP) {
              sock->conn->pcb.tcp->priority = *(const prio_t *)optval;
            } else
#endif
#if LWIP_RAW
              if (NETCONNTYPE_GROUP(sock->conn->type) == NETCONN_RAW) {
                sock->conn->pcb.raw->priority = *(const prio_t *)optval;
              } else
#endif
              {
                LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt Invalid socket type \n"));
                return ENOPROTOOPT;
              }
          break;
        }

#endif /* LWIP_SO_PRIORITY */

        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_SOCKET, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;

    /* Level: IPPROTO_IP */
    case IPPROTO_IP:
      switch (optname) {
        case IP_TTL:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, optlen, int);
          sock->conn->pcb.ip->ttl = (u8_t)(*(const int *)optval);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_IP, IP_TTL, ..) -> %d\n",
                                      s, sock->conn->pcb.ip->ttl));
          break;
        case IP_TOS:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, optlen, int);
          sock->conn->pcb.ip->tos = (u8_t)(*(const int *)optval);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_IP, IP_TOS, ..)-> %d\n",
                                      s, sock->conn->pcb.ip->tos));
          break;
#if LWIP_RAW
        case IP_HDRINCL:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, int, NETCONN_RAW);
          if (NETCONNTYPE_ISIPV6(sock->conn->type)) {
            return ENOPROTOOPT;
          }

          sock->conn->pcb.raw->hdrincl = (*(int *)optval) ? 1 : 0;
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IP_HDRINCL)-> %d\n",
                                      s, sock->conn->pcb.raw->hdrincl));
          break;
#endif

#if LWIP_NETBUF_RECVINFO
        case IP_PKTINFO:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, int, NETCONN_UDP);
          if (*(const int *)optval) {
            sock->conn->flags |= NETCONN_FLAG_PKTINFO;
          } else {
            sock->conn->flags = sock->conn->flags & (~NETCONN_FLAG_PKTINFO);
          }
          break;
#endif /* LWIP_NETBUF_RECVINFO */

#if LWIP_IPV4 && LWIP_MULTICAST_TX_OPTIONS
        case IP_MULTICAST_TTL:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, u8_t, NETCONN_UDP);
          udp_set_multicast_ttl(sock->conn->pcb.udp, (u8_t)(*(const u8_t *)optval));
          break;
        case IP_MULTICAST_IF: {
          struct netif *netif = NULL;
          ip_addr_t if_addr;
          (void)memset_s(&if_addr, sizeof(ip_addr_t), 0, sizeof(ip_addr_t));
          IP_SET_TYPE_VAL(if_addr, IPADDR_TYPE_V4);
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, struct in_addr, NETCONN_UDP);
          inet_addr_to_ip4addr(ip_2_ip4(&if_addr), (const struct in_addr *)optval);
          netif = netif_find_by_ipaddr(&if_addr);
          if (netif == NULL) {
            err = EADDRNOTAVAIL;
          } else if (sock->conn->pcb.ip->ifindex && (sock->conn->pcb.ip->ifindex != netif->ifindex)) {
            err = EADDRNOTAVAIL;
          } else {
            udp_set_multicast_netif_addr(sock->conn->pcb.udp, ip_2_ip4(&if_addr));
          }
          break;
        }

        case IP_MULTICAST_LOOP:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, u8_t, NETCONN_UDP);
          if (*(const u8_t *)optval) {
            udp_setflags(sock->conn->pcb.udp, udp_flags(sock->conn->pcb.udp) | UDP_FLAGS_MULTICAST_LOOP);
          } else {
            udp_setflags(sock->conn->pcb.udp, udp_flags(sock->conn->pcb.udp) & ~UDP_FLAGS_MULTICAST_LOOP);
          }
          break;
#endif /* LWIP_IPV4 && LWIP_MULTICAST_TX_OPTIONS */
#if LWIP_IGMP
        case IP_ADD_MEMBERSHIP:
        case IP_DROP_MEMBERSHIP: {
          /* If this is a TCP or a RAW socket, ignore these options. */
          /* @todo: assign membership to this socket so that it is dropped when closing the socket */
          err_t igmp_err;
          const struct ip_mreq *imr = (const struct ip_mreq *)optval;
          ip4_addr_t if_addr;
          ip4_addr_t multi_addr;
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, struct ip_mreq, NETCONN_UDP);
          inet_addr_to_ip4addr(&if_addr, &imr->imr_interface);
          inet_addr_to_ip4addr(&multi_addr, &imr->imr_multiaddr);
          if (optname == IP_ADD_MEMBERSHIP) {
            if (!lwip_socket_register_membership(sock, &if_addr, &multi_addr)) {
              /* cannot track membership (out of memory) */
              err = ENOMEM;
              igmp_err = ERR_OK;
            } else {
              igmp_err = igmp_joingroup(&if_addr, &multi_addr);
            }
          } else {
            igmp_err = igmp_leavegroup(&if_addr, &multi_addr);
            lwip_socket_unregister_membership(sock, &if_addr, &multi_addr);
          }
          if (igmp_err != ERR_OK) {
            err = EADDRNOTAVAIL;
          }
          break;
        }

#endif /* LWIP_IGMP */

        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_IP, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;

#if LWIP_TCP
    /* Level: IPPROTO_TCP */
    case IPPROTO_TCP:
      /* Special case: all IPPROTO_TCP option take an int */
      LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, int, NETCONN_TCP);
      if (sock->conn->pcb.tcp->state == LISTEN) {
        return EINVAL;
      }
      switch (optname) {
        case TCP_NODELAY:
          if (*(const int *)optval) {
            tcp_nagle_disable(sock->conn->pcb.tcp);
          } else {
            tcp_nagle_enable(sock->conn->pcb.tcp);
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, TCP_NODELAY) -> %s\n",
                                      s, (*(const int *)optval) ? "on" : "off"));
          break;
        case TCP_KEEPALIVE:
          sock->conn->pcb.tcp->keep_idle = (u32_t)(*(const int *)optval);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, TCP_KEEPALIVE) -> %"U32_F"\n",
                                      s, sock->conn->pcb.tcp->keep_idle));
          break;

#if LWIP_TCP_KEEPALIVE
        case TCP_KEEPIDLE:
          sock->conn->pcb.tcp->keep_idle = MS_PER_SECOND * (u32_t)(*(const int *)optval);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, TCP_KEEPIDLE) -> %"U32_F"\n",
                                      s, sock->conn->pcb.tcp->keep_idle));
          break;
        case TCP_KEEPINTVL:
          sock->conn->pcb.tcp->keep_intvl = MS_PER_SECOND * (u32_t)(*(const int *)optval);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, TCP_KEEPINTVL) -> %"U32_F"\n",
                                      s, sock->conn->pcb.tcp->keep_intvl));
          break;
        case TCP_KEEPCNT:
          sock->conn->pcb.tcp->keep_cnt = (u32_t)(*(const int *)optval);
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, TCP_KEEPCNT) -> %"U32_F"\n",
                                      s, sock->conn->pcb.tcp->keep_cnt));
          break;
#endif /* LWIP_TCP_KEEPALIVE */

        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;
#endif /* LWIP_TCP */

#if LWIP_IPV6
    /* Level: IPPROTO_IPV6 */
    case IPPROTO_IPV6:
      /* this check is added to ensure that the socket created should be only of type AF_INET6 */
      switch (optname) {
        case IPV6_V6ONLY:
          /* removed the TCP type to support the option for all sock types */
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, optlen, int);
          /* The below check is to ensure that the option is used for AF_INET6 only */
          LWIP_SOCKOPT_CHECK_IPTYPE_V6(sock->conn->type);

          if (*(const int *)optval) {
            netconn_set_ipv6only(sock->conn, 1);
          } else {
            netconn_set_ipv6only(sock->conn, 0);
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_IPV6, IPV6_V6ONLY, ..) -> %d\n",
                                      s, (netconn_get_ipv6only(sock->conn) ? 1 : 0)));
          break;
#if LWIP_IPV6 && LWIP_RAW
        case IPV6_CHECKSUM:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE_IPV6(sock, optlen, int, NETCONN_RAW);
          if (sock->conn->pcb.raw->raw_proto == IPPROTO_ICMPV6) {
            LWIP_DEBUGF(SOCKETS_DEBUG,
                        ("lwip_setsockopt(%d, socket type IPPROTO_ICMPV6, \
                        opttype - IPV6_CHECKSUM, ..) -> %d Not allowed \n",
                         s, sock->conn->pcb.raw->chksum_reqd));
            return EINVAL;
          }

          if (*(const int *)optval < 0) {
            sock->conn->pcb.raw->chksum_reqd = 0;
            /* reseting the offset of icmp value to 2 */
            sock->conn->pcb.raw->chksum_offset = 2;
          } else if (*(const int *)optval & 1) {
            /* Per RFC3542, odd offsets are not allowed */
            return EINVAL;
          } else if (*(const int *)optval > 0xFFFF) {
            /* Check added to avoid overflow or avoid big invalid value. */
            return EINVAL;
          } else {
            sock->conn->pcb.raw->chksum_reqd = 1;
            sock->conn->pcb.raw->chksum_offset = (u16_t) * (const int *)optval;
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_IPV6, IPV6_CHECKSUM, ..) -> %d\n",
                                      s, sock->conn->pcb.raw->chksum_reqd));
          break;
#endif
#if LWIP_IPV6_MLD
        case IPV6_JOIN_GROUP:
        case IPV6_LEAVE_GROUP: {
          /* If this is a TCP or a RAW socket, ignore these options. */
          err_t mld6_err;
          struct netif *netif = NULL;
          ip6_addr_t multi_addr;
          const struct ipv6_mreq *imr = (const struct ipv6_mreq *)optval;
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE_IPV6(sock, optlen, struct ipv6_mreq, NETCONN_UDP);
          inet6_addr_to_ip6addr(&multi_addr, &imr->ipv6mr_multiaddr);

          /* Check if the address is multicast or not */
          if (!ip6_addr_ismulticast(&multi_addr)) {
            err = EADDRNOTAVAIL;
            break;
          }
          /* Stack as of now doesnot handle the 0 interface index scenario */
          if (imr->ipv6mr_interface > LWIP_NETIF_IFINDEX_MAX_EX) {
            LWIP_DEBUGF(SOCKETS_DEBUG,
                        ("lwip_setsockopt(%d, Invalid IPPROTO_IPV6, IPV6_JOIN_GROUP/IPV6_LEAVE_GROUP) -> %d\n",
                         s, (imr->ipv6mr_interface)));
            err = ENXIO;
            break;
          }
          netif = netif_get_by_index((u8_t)imr->ipv6mr_interface);
          /* Whether the netif is null */
          if (netif == NULL) {
            err = EADDRNOTAVAIL;
            break;
          }

          if (optname == IPV6_JOIN_GROUP) {
            if (!lwip_socket_register_mld6_membership(sock, imr->ipv6mr_interface, &multi_addr)) {
              /* cannot track membership (out of memory) */
              err = ENOMEM;
              mld6_err = ERR_OK;
            } else {
              mld6_err = mld6_joingroup_netif(netif, &multi_addr);
            }
          } else {
            mld6_err = mld6_leavegroup_netif(netif, &multi_addr);
            lwip_socket_unregister_mld6_membership(sock, imr->ipv6mr_interface, &multi_addr);
          }
          if (mld6_err != ERR_OK) {
            err = EADDRNOTAVAIL;
          }
          break;
        }

#endif /* LWIP_IPV6_MLD */
#if LWIP_MAC_SECURITY
        case IP_MACSEC: {
          LWIP_SOCKOPT_CHECK_IPTYPE_V6(sock->conn->type);

          LWIP_SOCKOPT_CHECK_OPTLEN_CONN(sock, optlen, int);
#if LWIP_UDP
          if (NETCONNTYPE_GROUP(NETCONN_TYPE((sock)->conn)) == NETCONN_UDP) {
            sock->conn->pcb.udp->macsec_reqd = (u8_t)(*(const int *)optval) ? 1 : 0;
          } else
#endif
#if LWIP_RAW
            if (NETCONNTYPE_GROUP(NETCONN_TYPE((sock)->conn)) == NETCONN_RAW) {
              sock->conn->pcb.raw->macsec_reqd = (u8_t)(*(const int *)optval) ? 1 : 0;
            } else
#endif
            {
              err = ENOPROTOOPT;
            }
          break;
        }

#endif /* LWIP_MAC_SECURITY */
#if LWIP_NETBUF_RECVINFO
        case IPV6_RECVPKTINFO:
          LWIP_SOCKOPT_CHECK_IPTYPE_V6(sock->conn->type);
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE(sock, optlen, int, NETCONN_UDP);
          if (*(const int *)optval) {
            sock->conn->flags |= NETCONN_FLAG_PKTINFO;
          } else {
            sock->conn->flags &= ~NETCONN_FLAG_PKTINFO;
          }
          break;
#endif /* LWIP_NETBUF_RECVINFO */

        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_IPV6, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;
#endif /* LWIP_IPV6 */

#if LWIP_UDP && LWIP_UDPLITE
    /* Level: IPPROTO_UDPLITE */
    case IPPROTO_UDPLITE:
      /* Special case: all IPPROTO_UDPLITE option take an int */
      LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB(sock, optlen, int);
      /* If this is no UDP lite socket, ignore any options. */
      if (!NETCONNTYPE_ISUDPLITE(NETCONN_TYPE(sock->conn))) {
        return ENOPROTOOPT;
      }
      switch (optname) {
        case UDPLITE_SEND_CSCOV:
          if ((*(const int *)optval != 0) && ((*(const int *)optval < 8) || (*(const int *)optval > 0xffff))) {
            /* don't allow illegal values! */
            sock->conn->pcb.udp->chksum_len_tx = 8;
          } else {
            sock->conn->pcb.udp->chksum_len_tx = (u16_t) * (const int *)optval;
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_UDPLITE, UDPLITE_SEND_CSCOV) -> %d\n",
                                      s, (*(const int *)optval)));
          break;
        case UDPLITE_RECV_CSCOV:
          if ((*(const int *)optval != 0) && ((*(const int *)optval < 8) || (*(const int *)optval > 0xffff))) {
            /* don't allow illegal values! */
            sock->conn->pcb.udp->chksum_len_rx = 8;
          } else {
            sock->conn->pcb.udp->chksum_len_rx = (u16_t) * (const int *)optval;
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_UDPLITE, UDPLITE_RECV_CSCOV) -> %d\n",
                                      s, (*(const int *)optval)));
          break;

        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_UDPLITE, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;
#endif /* LWIP_UDP */
    /* Level: IPPROTO_RAW */
    case IPPROTO_RAW:
      switch (optname) {
#if LWIP_IPV6 && LWIP_RAW
        case IPV6_CHECKSUM:

          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE_IPV6(sock, optlen, int, NETCONN_RAW);

          /* It should not be possible to disable the checksum generation with ICMPv6
           * as per RFC 3542 chapter 3.1 */
          if (sock->conn->pcb.raw->raw_proto == IPPROTO_ICMPV6) {
            return EINVAL;
          }

          if (*(const int *)optval < 0) {
            sock->conn->pcb.raw->chksum_reqd = 0;
            sock->conn->pcb.raw->chksum_offset = IPV6_ICMP_CHKSUM_OFFSET;
          } else if (*(const int *)optval & 1) {
            /* Per RFC3542, odd offsets are not allowed */
            return EINVAL;
          } else if (*(const int *)optval > 0xFFFF) {
            sock->conn->pcb.raw->chksum_reqd = 1;
            /* Check added to avoid overflow or avoid  big invalid value. */
            return EINVAL;
          } else {
            sock->conn->pcb.raw->chksum_reqd = 1;
            sock->conn->pcb.raw->chksum_offset = (u16_t) * (const int *)optval;
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_RAW, IPV6_CHECKSUM, ..) -> %d\n",
                                      s, sock->conn->pcb.raw->chksum_reqd));
          break;
#endif /* LWIP_IPV6 && LWIP_RAW */
        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_RAW, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;

      /* Below feature is enabled  only if ipv6 raw socket is enabled */
#if LWIP_IPV6 && LWIP_RAW && LWIP_SOCK_OPT_ICMP6_FILTER
    /* Level: IPPROTO_ICMPV6 */
    case IPPROTO_ICMPV6:
      switch (optname) {
        case ICMP6_FILTER:
          LWIP_SOCKOPT_CHECK_OPTLEN_CONN_PCB_TYPE_IPV6(sock, optlen, struct icmp6_filter, NETCONN_RAW);
          if (optlen > (int)sizeof(struct icmp6_filter)) {
            optlen = sizeof(struct icmp6_filter);
          }

          if (memcpy_s(&sock->conn->pcb.raw->icmp6_filter, sizeof(struct icmp6_filter),
                       optval, optlen) != EOK) {
            LWIP_DEBUGF(SOCKETS_DEBUG,
                        ("lwip_setsockopt(%d, IPPROTO_ICMPV6, ICMP6_FILTER:  ..) mem error\n", s));
            err = EINVAL;
            break;
          }
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_ICMPV6, ICMP6_FILTER:  ..)\n", s));
          break;

        default:
          LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_ICMPV6, UNIMPL: optname=0x%x, ..)\n",
                                      s, optname));
          err = ENOPROTOOPT;
          break;
      }
      break;
#endif

    default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, level=0x%x, UNIMPL: optname=0x%x, ..)\n",
                                  s, level, optname));
      err = ENOPROTOOPT;
      break;
  }

  return err;
}

#if LWIP_IOCTL_ROUTE
LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCADDRT(struct rtentry *rmten)
{
  struct netif *netif = NULL;
  ip_addr_t rtgw_addr;
  u16_t rtgw_port;

  SOCKADDR_TO_IPADDR_PORT(&rmten->rt_gateway, &rtgw_addr, rtgw_port);

  if (!IP_IS_V4_VAL(rtgw_addr)) {
    return EINVAL;
  }

  /* check if multicast/0/loopback */
  if (ip_addr_ismulticast(&rtgw_addr) || ip_addr_isany(&rtgw_addr) ||
      ip_addr_isloopback(&rtgw_addr)) {
    return EINVAL;
  }

  /* check if reachable */
  for (netif = netif_list; netif != NULL; netif = netif->next) {
    if (ip_addr_netcmp(&rtgw_addr, &netif->ip_addr, ip_2_ip4(&netif->netmask))) {
      break;
    }
  }

  if (netif == NULL) {
    return EHOSTUNREACH;
  }

  /* check if broadcast */
  if (ip_addr_isbroadcast(&rtgw_addr, netif) != 0) {
    return EINVAL;
  }

  /* Check flags */
  if ((rmten->rt_flags & RTF_GATEWAY) == 0) {
    return EINVAL;
  }

  /* Add validation */
  if ((netif_default != NULL) && (netif_default != netif)) {
    ip_addr_set_zero(&netif_default->gw);
    (void)netif_set_default(netif);
  }
  netif_set_gw(netif, ip_2_ip4(&rtgw_addr));

  return 0;
}
#endif

#if LWIP_IOCTL_IF
LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCGIFCONF(struct ifreq *ifr)
{
  struct ifconf *ifc = NULL;
  struct netif *netif = NULL;
  struct ifreq ifreq;
  struct sockaddr_in *sock_in = NULL;
  int pos;
  int len;
  int ret;

  /* Format the caller's buffer. */
  ifc = (struct ifconf *)ifr;
  len = ifc->ifc_len;

  /* Loop over the interfaces, and write an info block for each. */
  pos = 0;
  for (netif = netif_list; netif != NULL; netif = netif->next) {
    if (ifc->ifc_buf == NULL) {
      pos =  (pos + (int)sizeof(struct ifreq));
      continue;
    }

    if (len < (int)sizeof(ifreq)) {
      break;
    }
    (void)memset_s(&ifreq, sizeof(struct ifreq), 0, sizeof(struct ifreq));
    if (netif->link_layer_type == LOOPBACK_IF) {
      ret = snprintf_s(ifreq.ifr_name, IFNAMSIZ, (IFNAMSIZ - 1), "%s", netif->name);
      if ((ret <= 0) || (ret >= IFNAMSIZ)) {
        LWIP_DEBUGF(NETIF_DEBUG, ("lwip_ioctl: snprintf_s ifr_name failed."));
        return ENOBUFS;
      }
    } else {
      ret = snprintf_s(ifreq.ifr_name, IFNAMSIZ, (IFNAMSIZ - 1), "%s%"U8_F, netif->name, netif->num);
      if ((ret <= 0) || (ret >= IFNAMSIZ)) {
        LWIP_DEBUGF(NETIF_DEBUG, ("lwip_ioctl: snprintf_s ifr_name failed."));
        return ENOBUFS;
      }
    }

    sock_in = (struct sockaddr_in *)&ifreq.ifr_addr;
    sock_in->sin_family = AF_INET;
    sock_in->sin_addr.s_addr = ip_2_ip4(&netif->ip_addr)->addr;
    if (memcpy_s(ifc->ifc_buf + pos, sizeof(struct ifreq), &ifreq, sizeof(struct ifreq)) != EOK) {
      return ENOBUFS;
    }
    pos = pos + (int)sizeof(struct ifreq);
    len = len - (int)sizeof(struct ifreq);
  }

  ifc->ifc_len = pos;

  return 0;
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCGIFADDR(struct ifreq *ifr)
{
  struct netif *netif = NULL;
  struct sockaddr_in *sock_in = NULL;

  /* get netif ipaddr */
  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  } else {
    sock_in = (struct sockaddr_in *)&ifr->ifr_addr;
    sock_in->sin_family = AF_INET;
    sock_in->sin_addr.s_addr = ip_2_ip4(&netif->ip_addr)->addr;
    return 0;
  }
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCSIFADDR_6(struct ifreq *ifr)
{
  struct netif *netif = NULL;
  err_t err;
  struct in6_ifreq *ifr6 = NULL;
  ip_addr_t target_addr;
  s8_t idx;

  ifr6 = (struct in6_ifreq *)ifr;

  /* Supports only fixed length LWIP_IPV6_PREFIX_LEN of prefix */
  if ((ifr6->ifr6_prefixlen != LWIP_IPV6_PREFIX_LEN) ||
      (ifr6->ifr6_ifindex > LWIP_NETIF_IFINDEX_MAX_EX)) {
    return EINVAL;
  }

  netif = netif_get_by_index((u8_t)ifr6->ifr6_ifindex);
  if (netif == NULL) {
    return ENODEV;
  }
#if LWIP_HAVE_LOOPIF
  /* Most likely unhittable code.. since loopif is global address.. remove code after confirmation */
  else if (netif->link_layer_type == LOOPBACK_IF) {
    return EPERM;
  }
#endif

  ip_addr_set_zero_ip6(&target_addr);
#if LWIP_LITEOS_COMPAT
  (void)memcpy_s((ip_2_ip6(&target_addr))->addr, sizeof(struct ip6_addr),
                 (&ifr6->ifr6_addr)->s6_addr, sizeof(struct ip6_addr));
#else
  inet6_addr_to_ip6addr(ip_2_ip6(&target_addr), &ifr6->ifr6_addr);
#endif

  /* Do no allow multicast or any ip address or loopback address to be set as interface address */
  if (ip_addr_ismulticast(&target_addr) ||
      ip_addr_isany(&target_addr) ||
      ip_addr_isloopback(&target_addr)) {
    return EINVAL;
  }

  idx = netif_get_ip6_addr_match(netif, ip_2_ip6(&target_addr));
  if (idx >= 0) {
    return EEXIST;
  }

#if LWIP_IPV6_DHCP6
  if ((netif_dhcp6_data(netif) != NULL) && (netif_dhcp6_data(netif)->state != DHCP6_STATE_OFF)) {
    dhcp6_disable(netif);
  }
#endif

  err = netif_add_ip6_address(netif, ip_2_ip6(&target_addr), &idx);
  if ((err != ERR_OK) || (idx == -1)) {
    return ENOBUFS;
  }

  netif_ip6_addr_set_state(netif, idx, IP6_ADDR_PREFERRED);

  return ERR_OK;
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCSIFADDR(struct ifreq *ifr)
{
  struct netif *netif = NULL;

  struct netif *loc_netif = NULL;
  ip_addr_t taget_addr;
  u16_t taget_port;
  SOCKADDR_TO_IPADDR_PORT(&ifr->ifr_addr, &taget_addr, taget_port);

  /* set netif ipaddr */
  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  }
#if LWIP_HAVE_LOOPIF
  else if (netif->link_layer_type == LOOPBACK_IF) {
    return EPERM;
  }
#endif
  else {
    /* check the address is not multicast/broadcast/0/loopback */
    if (!IP_IS_V4(&taget_addr) || ip_addr_ismulticast(&taget_addr) ||
        ip_addr_isbroadcast(&taget_addr, netif) ||
        ip_addr_isany(&taget_addr) ||
        ip_addr_isloopback(&taget_addr)) {
      return EINVAL;
    }

    /* reset gateway if new and previous ipaddr not in same net */
    if (ip_addr_netcmp(&taget_addr, &netif->ip_addr, ip_2_ip4(&netif->netmask)) == 0) {
      ip_addr_set_zero(&netif->gw);
      if (netif == netif_default) {
        (void)netif_set_default(NULL);
      }
    }

    /* lwip disallow two netif sit in same net at the same time */
    loc_netif = netif_list;
    while (loc_netif != NULL) {
      if (loc_netif == netif) {
        loc_netif = loc_netif->next;
        continue;
      }
      if (ip_addr_cmp(&netif->netmask, &loc_netif->netmask) &&
          ip_addr_netcmp(&loc_netif->ip_addr, &taget_addr,
                         ip_2_ip4(&netif->netmask))) {
        return EINVAL;
      }
      loc_netif = loc_netif->next;
    }

#if LWIP_DHCP
    if ((netif_dhcp_data(netif) != NULL) &&
        (netif_dhcp_data(netif)->client.states[LWIP_DHCP_NATIVE_IDX].state != DHCP_STATE_OFF)) {
      (void)netif_dhcp_off(netif);
    }
#endif

#if LWIP_ARP
    /* clear ARP cache when IP address changed */
    if ((netif->flags & NETIF_FLAG_ETHARP) != 0) {
      etharp_cleanup_netif(netif);
    }
#endif /* LWIP_ARP */

    netif_set_ipaddr(netif, ip_2_ip4(&taget_addr));

    return 0;
  }
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCDIFADDR_6(struct ifreq *ifr)
{
  struct netif *netif = NULL;
  err_t err;
  struct in6_ifreq *ifr6 = NULL;
  ip_addr_t target_addr;
  s8_t idx;

  ifr6 = (struct in6_ifreq *)ifr;

  /* Supports only fixed length LWIP_IPV6_PREFIX_LEN of prefix */
  if ((ifr6->ifr6_prefixlen != LWIP_IPV6_PREFIX_LEN) ||
      (ifr6->ifr6_ifindex > LWIP_NETIF_IFINDEX_MAX_EX)) {
    return EINVAL;
  }

  netif = netif_get_by_index((u8_t)ifr6->ifr6_ifindex);
  if (netif == NULL) {
    return ENODEV;
  }
#if LWIP_HAVE_LOOPIF
  /* Most likely unhittable code.. since loopif is global address.. remove code after confirmation */
  else if (netif->link_layer_type == LOOPBACK_IF) {
    return EPERM;
  }
#endif

  ip_addr_set_zero_ip6(&target_addr);
#if LWIP_LITEOS_COMPAT
  (void)memcpy_s((ip_2_ip6(&target_addr))->addr, sizeof(struct ip6_addr),
                 (&ifr6->ifr6_addr)->s6_addr, sizeof(struct ip6_addr));
#else
  inet6_addr_to_ip6addr(ip_2_ip6(&target_addr), &ifr6->ifr6_addr);
#endif
  /* It should not be loopback address */
  if (ip_addr_isloopback(&target_addr)) {
    return EINVAL;
  }

  /* Allow deletion of existing IPv6 address.... only limitation is, it does not allow removal of link local address */
  idx = netif_get_ip6_addr_match(netif, ip_2_ip6(&target_addr));
  if (idx < 0) {
    return EADDRNOTAVAIL;
  }

  /* Removal of link local address not permitted */
  if (idx == 0) {
    return EPERM;
  }

#if LWIP_IPV6_DHCP6
  if (netif_dhcp6_data(netif) && netif_dhcp6_data(netif)->state != DHCP6_STATE_OFF) {
    dhcp6_disable(netif);
  }
#endif

  err = netif_do_rmv_ipv6_addr(netif, ip_2_ip6(&target_addr));
  if ((err != ERR_OK) || (idx == -1)) {
    return ENOBUFS;
  }

  return ERR_OK;
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCDIFADDR(struct ifreq *ifr)
{
  struct netif *netif = NULL;

  ip_addr_t target_addr;
  u16_t target_port;

  SOCKADDR_TO_IPADDR_PORT(&ifr->ifr_addr, &target_addr, target_port);

  /* set netif ipaddr */
  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  }
#if LWIP_HAVE_LOOPIF
  else if (netif->link_layer_type == LOOPBACK_IF) {
    return EPERM;
  }
#endif

  /* check the address is not loopback */
  if (!IP_IS_V4(&target_addr) || ip_addr_isloopback(&target_addr)) {
    return EINVAL;
  }

#if LWIP_DHCP
  if ((netif_dhcp_data(netif) != NULL) &&
      (netif_dhcp_data(netif)->client.states[LWIP_DHCP_NATIVE_IDX].state != DHCP_STATE_OFF)) {
    (void)netif_dhcp_off(netif);
  }
#endif

  ip_addr_set_zero(&netif->gw);
  ip_addr_set_zero(&netif->ip_addr);
  ip_addr_set_zero(&netif->netmask);
  if (netif == netif_default) {
    (void)netif_set_default(NULL);
  }

#if LWIP_IPV4 && LWIP_ARP
  if ((netif->flags & NETIF_FLAG_ETHARP) != 0) {
    etharp_cleanup_netif(netif);
  }
#endif /* LWIP_IPV4 && LWIP_ARP */

  return ERR_OK;
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCGIFNETMASK(struct ifreq *ifr)
{
  struct netif *netif = NULL;
  struct sockaddr_in *sock_in = NULL;

  /* get netif netmask */
  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  } else {
    sock_in = (struct sockaddr_in *)&ifr->ifr_netmask;
    sock_in->sin_family = AF_INET;
    sock_in->sin_addr.s_addr = ip_2_ip4(&netif->netmask)->addr;
    return 0;
  }
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCSIFNETMASK(struct ifreq *ifr)
{
  struct netif *netif = NULL;

  struct netif *loc_netif = NULL;
  ip_addr_t taget_addr;
  u16_t taget_port;
  SOCKADDR_TO_IPADDR_PORT(&ifr->ifr_addr, &taget_addr, taget_port);
  if (!IP_IS_V4(&taget_addr)) {
    return EINVAL;
  }

  /* set netif netmask */
  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  }
#if LWIP_HAVE_LOOPIF
  else if (netif->link_layer_type == LOOPBACK_IF) {
    return EPERM;
  }
#endif
  else {
    if (ip_addr_cmp(&netif->netmask, &taget_addr)) {
      return 0;
    }
    /* check data valid */
    if (ip_addr_netmask_valid(ip_2_ip4(&taget_addr)) != 0) {
      return EINVAL;
    }

    /* lwip disallow two netif sit in same net at the same time */
    loc_netif = netif_list;
    while (loc_netif != NULL) {
      if (loc_netif == netif) {
        loc_netif = loc_netif->next;
        continue;
      }
      if (ip_addr_cmp(&loc_netif->netmask, &taget_addr) &&
          ip_addr_netcmp(&loc_netif->ip_addr,
                         &netif->ip_addr, ip_2_ip4(&loc_netif->netmask))) {
        return EINVAL;
      }
      loc_netif = loc_netif->next;
    }

#if LWIP_DHCP // LWIP_DHCP
    if ((netif_dhcp_data(netif) != NULL) &&
        (netif_dhcp_data(netif)->client.states[LWIP_DHCP_NATIVE_IDX].state != DHCP_STATE_OFF)) {
      (void)netif_dhcp_off(netif);
    }
#endif

    netif_set_netmask(netif, ip_2_ip4(&taget_addr));

    /* check if gateway still reachable */
    if (!ip_addr_netcmp(&netif->gw, &netif->ip_addr, ip_2_ip4(&taget_addr))) {
      ip_addr_set_zero(&(netif->gw));
      if (netif == netif_default) {
        (void)netif_set_default(NULL);
      }
    }
    return 0;
  }
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCSIFHWADDR(struct ifreq *ifr)
{
  struct netif *netif = NULL;
  err_t ret;

  /* set netif hw addr */
  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  }
#if LWIP_HAVE_LOOPIF
  else if (netif->link_layer_type == LOOPBACK_IF) {
    return EPERM;
  }
#endif
  else {

    /* bring netif down to clear all Neighbor Cache Entry */
    (void)netif_set_down(netif);

    ret = netif_set_hwaddr(netif, (const unsigned char *)ifr->ifr_hwaddr.sa_data, netif->hwaddr_len);

    if (ret != ERR_OK) {
      (void)netif_set_up(netif);
      return err_to_errno(ret);
    }

    /*
     * bring netif up to try to send GARP/IGMP/NA/MLD/RS. GARP and NA would
     * make the neighboring nodes update their Neighbor Cache immediately.
     */
    (void)netif_set_up(netif);
    return 0;
  }
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCGIFHWADDR(struct ifreq *ifr)
{
  struct netif *netif = NULL;

  /* get netif hw addr */
  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  }
#if LWIP_HAVE_LOOPIF
  else if (netif->link_layer_type == LOOPBACK_IF) {
    return EPERM;
  }
#endif /* LWIP_HAVE_LOOPIF */
  else {
    if (memcpy_s((void *)ifr->ifr_hwaddr.sa_data, sizeof(ifr->ifr_hwaddr.sa_data),
                 (void *)netif->hwaddr, netif->hwaddr_len) != EOK) {
      return EINVAL;
    }
    return 0;
  }
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCSIFFLAGS(struct ifreq *ifr)
{
  struct netif *netif = NULL;

  /* set netif hw addr */
  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  }
#if LWIP_HAVE_LOOPIF
  else if (netif->link_layer_type == LOOPBACK_IF) {
    return EPERM;
  }
#endif /* LWIP_HAVE_LOOPIF */
  else {
    if ((ifr->ifr_flags & IFF_UP) && !(netif->flags & NETIF_FLAG_UP)) {
      (void)netif_set_up(netif);
    } else if (!(ifr->ifr_flags & IFF_UP) && (netif->flags & NETIF_FLAG_UP)) {
      (void)netif_set_down(netif);
    }
    if ((ifr->ifr_flags & IFF_RUNNING) && !(netif->flags & NETIF_FLAG_LINK_UP)) {
      (void)netif_set_link_up(netif);
    } else if (!(ifr->ifr_flags & IFF_RUNNING) && (netif->flags & NETIF_FLAG_LINK_UP)) {
      (void)netif_set_link_down(netif);
    }

    if (ifr->ifr_flags & IFF_BROADCAST) {
      netif->flags |= NETIF_FLAG_BROADCAST;
    } else {
      netif->flags = netif->flags & (~NETIF_FLAG_BROADCAST);
    }
    if (ifr->ifr_flags & IFF_NOARP) {
      netif->flags = (netif->flags & (~NETIF_FLAG_ETHARP));
    } else {
      netif->flags |= NETIF_FLAG_ETHARP;
    }

    if (ifr->ifr_flags & IFF_MULTICAST) {
#if LWIP_IGMP
      netif->flags |= NETIF_FLAG_IGMP;
#endif /* LWIP_IGMP */
#if LWIP_IPV6 && LWIP_IPV6_MLD
      netif->flags |= NETIF_FLAG_MLD6;
#endif /* LWIP_IPV6_MLD */
    } else {
#if LWIP_IGMP
      netif->flags = (netif->flags &  ~NETIF_FLAG_IGMP);
#endif /* LWIP_IGMP */
#if LWIP_IPV6 && LWIP_IPV6_MLD
      netif->flags = (netif->flags &  ~NETIF_FLAG_MLD6);
#endif /* LWIP_IPV6_MLD */
    }

#if LWIP_DHCP
    if (ifr->ifr_flags & IFF_DYNAMIC) {
      (void)dhcp_start(netif);
    } else {
      dhcp_stop(netif);
#if !LWIP_DHCP_SUBSTITUTE
      dhcp_cleanup(netif);
#endif
    }
#endif

#if LWIP_NETIF_PROMISC
    if ((ifr->ifr_flags & IFF_PROMISC)) {
      netif_update_promiscuous_mode_status(netif, 1);
    } else {
      netif_update_promiscuous_mode_status(netif, 0);
    }
#endif /* LWIP_NETIF_PROMISC */
    return 0;
  }
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCGIFFLAGS(struct ifreq *ifr)
{
  struct netif *netif = NULL;

  /* set netif hw addr */
  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  } else {
    if (netif->flags & NETIF_FLAG_UP) {
      ifr->ifr_flags |= IFF_UP;
    } else {
      ifr->ifr_flags &= ~IFF_UP;
    }
    if (netif->flags & NETIF_FLAG_LINK_UP) {
      ifr->ifr_flags |= IFF_RUNNING;
    } else {
      ifr->ifr_flags &= ~IFF_RUNNING;
    }
    if (netif->flags & NETIF_FLAG_BROADCAST) {
      ifr->ifr_flags |= IFF_BROADCAST;
    } else {
      ifr->ifr_flags &= ~IFF_BROADCAST;
    }
    if (netif->flags & NETIF_FLAG_ETHARP) {
      ifr->ifr_flags &= ~IFF_NOARP;
    } else {
      ifr->ifr_flags |= IFF_NOARP;
    }

#if LWIP_IGMP || LWIP_IPV6_MLD
    if (
#if LWIP_IGMP
      (netif->flags & NETIF_FLAG_IGMP)
#endif /* LWIP_IGMP */
#if LWIP_IGMP && LWIP_IPV6_MLD
      ||
#endif /* LWIP_IGMP && LWIP_IPV6_MLD */
#if LWIP_IPV6_MLD
      (netif->flags & NETIF_FLAG_MLD6)
#endif /* LWIP_IPV6_MLD */
    ) {
      ifr->ifr_flags = (short)((unsigned short)ifr->ifr_flags | IFF_MULTICAST);
    } else {
      ifr->ifr_flags = (short)((unsigned short)ifr->ifr_flags & (~IFF_MULTICAST));
    }
#endif /* LWIP_IGMP || LWIP_IPV6_MLD */

#if LWIP_DHCP
    if ((netif->flags & NETIF_FLAG_DHCP) != 0) {
      ifr->ifr_flags = (short)((unsigned short)ifr->ifr_flags | IFF_DYNAMIC);
    } else {
      ifr->ifr_flags = (short)((unsigned short)ifr->ifr_flags & (~IFF_DYNAMIC));
    }
#endif

#if LWIP_HAVE_LOOPIF
    if (netif->link_layer_type == LOOPBACK_IF) {
      ifr->ifr_flags |= IFF_LOOPBACK;
    }
#endif

#if LWIP_NETIF_PROMISC
    if (atomic_read(&(netif->flags_ext)) == NETIF_FLAG_PROMISC) {
      ifr->ifr_flags |= IFF_PROMISC;
    } else {
      ifr->ifr_flags &= ~IFF_PROMISC;
    }
#endif /* LWIP_NETIF_PROMISC */

    return 0;
  }
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCGIFNAME(struct ifreq *ifr)
{
  struct netif *netif = NULL;
  int ret;

  for (netif = netif_list; netif != NULL; netif = netif->next) {
    if (ifr->ifr_ifindex == netif->ifindex) {
      break;
    }
  }

  if (netif == NULL) {
    return ENODEV;
  } else {
    if (netif->link_layer_type == LOOPBACK_IF) {
      ret = snprintf_s(ifr->ifr_name, IFNAMSIZ, (IFNAMSIZ - 1), "%s", netif->name);
      if ((ret <= 0) || (ret >= IFNAMSIZ)) {
        return ENOBUFS;
      }
    } else {
      ret = snprintf_s(ifr->ifr_name, IFNAMSIZ, (IFNAMSIZ - 1), "%s%"U8_F, netif->name, netif->num);
      if ((ret <= 0) || (ret >= IFNAMSIZ)) {
        return ENOBUFS;
      }
    }
    return 0;
  }
}

LWIP_STATIC bool
lwip_validate_ifname(const char *name, u8_t *let_pos)
{
  unsigned short num_pos = 0;
  unsigned short letter_pos = 0;
  unsigned short pos = 0;
  bool have_num = 0;

  /* if the first position of variable name is not letter, such as '6eth2' */
  if (((*name >= 'a') && (*name <= 'z')) || ((*name >= 'A') && (*name <= 'Z'))) {
    return 0;
  }

  /* check if the position of letter is bigger than the the position of digital */
  while (*name != '\0') {
    if ((*name >= '0') && (*name <= '9')) {
      num_pos = pos;
      have_num = 1;
    } else if (((*name >= 'a') && (*name <= 'z')) || ((*name >= 'A') && (*name <= 'Z'))) {
      letter_pos = pos;
      if (have_num != 0) {
        return 0;
      }
    } else {
      return 0;
    }
    pos++;
    name++;
  }

  /* for the speacil case as all position of variable name is letter, such as 'ethabc' */
  if (num_pos == 0) {
    return 0;
  }

  /* cheak if the digital in the variable name is bigger than 255, such as 'eth266' */
  if (atoi(name - (pos - letter_pos - 1)) > 255) {
    return 0;
  }

  *let_pos = (u8_t)letter_pos;

  return 1;
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCSIFNAME(struct ifreq *ifr)
{
  struct netif *netif = NULL;
  char name[IFNAMSIZ];
  s32_t num = 0;
  u8_t letter_pos = 0;

  netif = netif_find(ifr->ifr_name);

  if (netif == NULL) {
    return ENODEV;
  } else if (netif->link_layer_type == LOOPBACK_IF) {
    return EPERM;
  } else if ((netif->flags & IFF_UP) != 0) {
    return EBUSY;
  } else {
    ifr->ifr_newname[IFNAMSIZ - 1] = '\0';
    if ((lwip_validate_ifname(ifr->ifr_newname, &letter_pos) == 0) || (strlen(ifr->ifr_newname) > (IFNAMSIZ - 1))) {
      return EINVAL;
    }

    (void)memset_s(name, IFNAMSIZ, 0, IFNAMSIZ);
    if (strncpy_s(name, IFNAMSIZ, ifr->ifr_newname, strlen(ifr->ifr_newname)) != EOK) {
      return EPERM;
    }

    num = atoi(name + letter_pos + 1);
    if (num > 0xFF || num < 0) {
      return EINVAL;
    }

    name[letter_pos + 1] = '\0';
    if (netif_check_num_isusing(name, (u8_t)num) == 1) {
      return EINVAL;
    }

    if (strncpy_s(netif->name, sizeof(netif->name), name, strlen(name)) != EOK) {
      return EINVAL;
    }
    netif->name[strlen(name)] = '\0';
    netif->num = (u8_t)num;
  }

  return 0;
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCGIFINDEX(struct ifreq *ifr)
{
  struct netif *netif = NULL;

  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  } else {
    ifr->ifr_ifindex = netif->ifindex;
    return 0;
  }
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCSIFMTU(struct ifreq *ifr)
{
  struct netif *netif = NULL;

  /* set netif hw addr */
  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  }
#if LWIP_HAVE_LOOPIF
  /* the mtu of loopif is not used. */
  else if (netif->link_layer_type == LOOPBACK_IF) {
    return EPERM;
  }
#endif
  else {
    if (ERR_OK != netif_set_mtu(netif, (u16_t)ifr->ifr_mtu)) {
      return EINVAL;
    }

    return 0;
  }
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCGIFMTU(struct ifreq *ifr)
{
  struct netif *netif = NULL;

  /* get netif hw addr */
  netif = netif_find(ifr->ifr_name);

  if (netif == NULL) {
    return ENODEV;
  } else {
    ifr->ifr_mtu = netif->mtu;
    return 0;
  }
}
#endif /* LWIP_IOCTL_IF */

#if LWIP_NETIF_ETHTOOL
LWIP_STATIC s32_t
lwip_ioctl_internal_SIOCETHTOOL(struct ifreq *ifr)
{
  struct netif *netif;

  netif = netif_find(ifr->ifr_name);
  if (netif == NULL) {
    return ENODEV;
  } else {
    return dev_ethtool(netif, ifr);
  }
}
#endif

#if LWIP_FIONREAD_LINUXMODE
LWIP_STATIC u8_t
lwip_ioctl_internal_NOTCP_FIONREAD(struct lwip_sock *sock, void *argp)
{
  SYS_ARCH_DECL_PROTECT(lev);
  if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_TCP) {
    struct pbuf *p = NULL;
    if (sock->lastdata != NULL) {
      p = ((struct netbuf *)sock->lastdata)->p;
      *((int *)argp) = p->tot_len - sock->lastoffset;
    } else {
      struct netbuf *rxbuf = NULL;
      err_t err;

      SYS_ARCH_PROTECT(lev);
      if (sock->rcvevent <= 0) {
        *((int *)argp) = 0;
        SYS_ARCH_UNPROTECT(lev);
      } else {
        SYS_ARCH_UNPROTECT(lev);
        err = netconn_recv(sock->conn, &rxbuf);
        if (err != ERR_OK) {
          *((int *)argp) = 0;
        } else {
          sock->lastdata = rxbuf;
          sock->lastoffset = 0;
          *((int *)argp) = rxbuf->p->tot_len;
        }
      }
    }
    return lwIP_TRUE;
  }
  return lwIP_FALSE;
}
#endif

#if LWIP_SO_RCVBUF || LWIP_FIONREAD_LINUXMODE
LWIP_STATIC u8_t
lwip_ioctl_internal_FIONREAD(struct lwip_sock *sock, void *argp)
{
#if LWIP_SO_RCVBUF
  u16_t buflen = 0;
  int recv_avail;
#endif
  lwip_sock_lock(sock);
#if LWIP_FIONREAD_LINUXMODE
  u8_t ret;
  ret = lwip_ioctl_internal_NOTCP_FIONREAD(sock, argp);
  if (ret == lwIP_TRUE) {
    lwip_sock_unlock(sock);
    return 0;
  }
#endif

#if LWIP_SO_RCVBUF
  /* we come here if either LWIP_FIONREAD_LINUXMODE==0 or this is a TCP socket */
  SYS_ARCH_GET(sock->conn->recv_avail, recv_avail);
  if (recv_avail < 0) {
    recv_avail = 0;
  }
  *((int *)argp) = recv_avail;

  /* Check if there is data left from the last recv operation. /maq 041215 */
  if (sock->lastdata != NULL) {
    struct pbuf *p = (struct pbuf *)sock->lastdata;
    if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_TCP) {
      p = ((struct netbuf *)p)->p;
    }
    buflen = p->tot_len;
    buflen = (u16_t)(buflen - sock->lastoffset);

    *((int *)argp) += buflen;
  }

  lwip_sock_unlock(sock);
  return 0;

#else /* LWIP_SO_RCVBUF */
  (void)argp;
  lwip_sock_unlock(sock);
  return ENOSYS;

#endif /* !LWIP_SO_RCVBUF */
}
#endif /* LWIP_SO_RCVBUF || LWIP_FIONREAD_LINUXMODE */

LWIP_STATIC u8_t
lwip_ioctl_internal_FIONBIO(const struct lwip_sock *sock, void *argp)
{
  u8_t val;
  SYS_ARCH_DECL_PROTECT(lev);

  val = 0;
  if (*(u32_t *)argp) {
    val = 1;
  }

  SYS_ARCH_PROTECT(lev);
  netconn_set_nonblocking(sock->conn, val);
  SYS_ARCH_UNPROTECT(lev);
  return 0;
}

#if LWIP_IPV6
#if LWIP_IPV6_DUP_DETECT_ATTEMPTS
LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCSIPV6DAD(struct ifreq *ifr)
{
  struct netif *tmpnetif = netif_find(ifr->ifr_name);
  if (tmpnetif == NULL) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("Interface not found.\n"));
    return ENODEV;
  }

  if ((ifr->ifr_ifru.ifru_ivalue != 0) && (ifr->ifr_ifru.ifru_ivalue != 1)) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("Invalid ioctl argument(ifru_ivalue 0/1).\n"));
    return EBADRQC;
  }

  if (ifr->ifr_ifru.ifru_ivalue == 1) {
    tmpnetif->ipv6_flags = (tmpnetif->ipv6_flags | LWIP_IPV6_ND6_FLAG_DAD);

    LWIP_DEBUGF(SOCKETS_DEBUG, ("DAD turned on through ioctl for  %s iface index %u \n",
                                tmpnetif->name, tmpnetif->num));
  } else {
    tmpnetif->ipv6_flags = (tmpnetif->ipv6_flags & ((~LWIP_IPV6_ND6_FLAG_DAD) & 0xffU));

    LWIP_DEBUGF(SOCKETS_DEBUG, ("DAD turned off through ioctl for  %s iface index %u \n",
                                tmpnetif->name, tmpnetif->num));
  }
  return 0;
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCGIPV6DAD(struct ifreq *ifr)
{
  struct netif *tmpnetif = netif_find(ifr->ifr_name);
  if (tmpnetif == NULL) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("Interface not found.\n"));
    return ENODEV;
  }
  ifr->ifr_ifru.ifru_ivalue = (tmpnetif->ipv6_flags & LWIP_IPV6_ND6_FLAG_DAD) ? 1 : 0;
  return 0;
}

#endif

#if LWIP_IOCTL_IPV6DPCTD
LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCSIPV6DPCTD(struct ifreq *ifr)
{
  struct netif *tmpnetif = netif_find(ifr->ifr_name);
  if (tmpnetif == NULL) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("Interface not found.\n"));
    return ENODEV;
  }
  if ((ifr->ifr_ifru.ifru_ivalue != 0) && (ifr->ifr_ifru.ifru_ivalue != 1)) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("Invalid ioctl argument(ifru_ivalue 0/1).\n"));
    return EBADRQC;
  }
  if (ifr->ifr_ifru.ifru_ivalue == 1) {
    tmpnetif->ipv6_flags = (tmpnetif->ipv6_flags | LWIP_IPV6_ND6_FLAG_DEPRECATED);
    LWIP_DEBUGF(SOCKETS_DEBUG, ("Deprecation turned on through ioctl for  %s iface index %u \n",
                                tmpnetif->name, tmpnetif->num));
  } else {
    tmpnetif->ipv6_flags = (tmpnetif->ipv6_flags & ((~LWIP_IPV6_ND6_FLAG_DEPRECATED) & 0xffU));
    LWIP_DEBUGF(SOCKETS_DEBUG, ("Deprecation turned off through ioctl for  %s iface index %u \n",
                                tmpnetif->name, tmpnetif->num));
  }
  return 0;
}

LWIP_STATIC u8_t
lwip_ioctl_internal_SIOCGIPV6DPCTD(struct ifreq *ifr)
{
  struct netif *tmpnetif = netif_find(ifr->ifr_name);
  if (tmpnetif == NULL) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("Interface not found.\n"));
    return ENODEV;
  }

  ifr->ifr_ifru.ifru_ivalue = (tmpnetif->ipv6_flags & LWIP_IPV6_ND6_FLAG_DEPRECATED) ? 1 : 0;
  return 0;
}
#endif /* LWIP_IOCTL_IPV6DPCTD */
#endif

LWIP_STATIC u8_t
lwip_ioctl_impl(const struct lwip_sock *sock, long cmd, void *argp)
{
  u8_t err = 0;
#if LWIP_NETIF_ETHTOOL
  s32_t ret;
#endif
#if LWIP_IPV6_DUP_DETECT_ATTEMPTS || LWIP_IOCTL_IPV6DPCTD || LWIP_IOCTL_IF || LWIP_NETIF_ETHTOOL
  struct ifreq *ifr = (struct ifreq *)argp;
#endif
#if LWIP_IOCTL_ROUTE
  struct rtentry *rmten = (struct rtentry *)argp;
#endif
#if LWIP_IPV6_DUP_DETECT_ATTEMPTS || LWIP_IOCTL_IPV6DPCTD || LWIP_IOCTL_ROUTE || LWIP_IOCTL_IF
  bool is_ipv6 = 0;

  /* allow it only on IPv6 sockets... */
  is_ipv6 = NETCONNTYPE_ISIPV6(sock->conn->type);
#endif

  switch ((u32_t)cmd) {
#if LWIP_IPV6
#if LWIP_IPV6_DUP_DETECT_ATTEMPTS
    case  SIOCSIPV6DAD:
      /* allow it only on IPv6 sockets... */
      if (is_ipv6 == 0) {
        err = EINVAL;
      } else {
        err = lwip_ioctl_internal_SIOCSIPV6DAD(ifr);
      }
      break;
    case  SIOCGIPV6DAD:
      /* allow it only on IPv6 sockets... */
      if (is_ipv6 == 0) {
        err = EINVAL;
      } else {
        err = lwip_ioctl_internal_SIOCGIPV6DAD(ifr);
      }
      break;
#endif /* LWIP_IPV6_DUP_DETECT_ATTEMPTS */
#if LWIP_IOCTL_IPV6DPCTD
    case SIOCSIPV6DPCTD:
      /* allow it only on IPv6 sockets... */
      if (is_ipv6 == 0) {
        err = EINVAL;
      } else {
        err = lwip_ioctl_internal_SIOCSIPV6DPCTD(ifr);
      }
      break;
    case SIOCGIPV6DPCTD:
      /* allow it only on IPv6 sockets... */
      if (is_ipv6 == 0) {
        err = EINVAL;
      } else {
        err = lwip_ioctl_internal_SIOCGIPV6DPCTD(ifr);
      }
      break;
#endif
#endif /* LWIP_IPV6 */
#if LWIP_IOCTL_ROUTE
    case SIOCADDRT:
      /* Do not allow if socket is AF_INET6 */
      if (is_ipv6 != 0) {
        err = EINVAL;
      } else {
        err = lwip_ioctl_internal_SIOCADDRT(rmten);
      }
      break;
#endif
#if LWIP_IOCTL_IF
    case SIOCGIFCONF:
      /* Do not allow if socket is AF_INET6 */
      if (is_ipv6 != 0) {
        err = EINVAL;
      } else {
        err = lwip_ioctl_internal_SIOCGIFCONF(ifr);
      }
      break;
    case SIOCGIFADDR:
      if (is_ipv6 != 0) {
        err = EINVAL;
      } else {
        err = lwip_ioctl_internal_SIOCGIFADDR(ifr);
      }
      break;
    case SIOCSIFADDR:
      if (is_ipv6 != 0) {
        err = lwip_ioctl_internal_SIOCSIFADDR_6(ifr);
      } else {
        err = lwip_ioctl_internal_SIOCSIFADDR(ifr);
      }
      break;
    case SIOCDIFADDR:
      /* Delete interface address */
      if (is_ipv6 != 0) {
        err = lwip_ioctl_internal_SIOCDIFADDR_6(ifr);
      } else {
        err = lwip_ioctl_internal_SIOCDIFADDR(ifr);
      }
      break;
    case SIOCGIFNETMASK:
      if (is_ipv6 != 0) {
        err = EINVAL;
      } else {
        err = lwip_ioctl_internal_SIOCGIFNETMASK(ifr);
      }
      break;
    case SIOCSIFNETMASK:
      if (is_ipv6 != 0) {
        err = EINVAL;
      } else {
        err = lwip_ioctl_internal_SIOCSIFNETMASK(ifr);
      }
      break;
    case SIOCSIFHWADDR:
      err = lwip_ioctl_internal_SIOCSIFHWADDR(ifr);
      break;
    case SIOCGIFHWADDR:
      err = lwip_ioctl_internal_SIOCGIFHWADDR(ifr);
      break;
    case SIOCSIFFLAGS:
      err = lwip_ioctl_internal_SIOCSIFFLAGS(ifr);
      break;
    case SIOCGIFFLAGS:
      err = lwip_ioctl_internal_SIOCGIFFLAGS(ifr);
      break;
    case SIOCGIFNAME:
      err = lwip_ioctl_internal_SIOCGIFNAME(ifr);
      break;
    case SIOCSIFNAME:
      err = lwip_ioctl_internal_SIOCSIFNAME(ifr);
      break;
    /* Need to support the get index through ioctl
     * As of now the options is restricted to PF_PACKET scenario , so removed the compiler flag Begin
     */
    case SIOCGIFINDEX:
      err = lwip_ioctl_internal_SIOCGIFINDEX(ifr);
      break;
    case SIOCGIFMTU:
      err = lwip_ioctl_internal_SIOCGIFMTU(ifr);
      break;
    case SIOCSIFMTU:
      err = lwip_ioctl_internal_SIOCSIFMTU(ifr);
      break;
#endif /* LWIP_IOCTL_IF */
#if LWIP_NETIF_ETHTOOL
    case SIOCETHTOOL:
      ret = lwip_ioctl_internal_SIOCETHTOOL(ifr);
      if (ret != 0) {
        /* an IO error happened */
        err = EIO;
      }
      break;
#endif
    case FIONBIO:
      err = lwip_ioctl_internal_FIONBIO(sock, argp);
      break;
    /* START For cmd = -1 stack has to treat it as Invalid Input and return EINVAL */
    case 0xFFFFFFFF:
      err = EINVAL;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_ioctl_impl(INVALID: 0x%lx)\n", cmd));
      break;
    default:
      err = ENOSYS;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_ioctl(UNIMPL: 0x%lx)\n", cmd));
      break;
  }

  return err;
}

int
lwip_ioctl(int s, long cmd, void *argp)
{
  struct lwip_sock *sock = NULL;
#if LWIP_TCPIP_CORE_LOCKING
  u8_t err;
#else /* LWIP_TCPIP_CORE_LOCKING */
  u8_t val;
#if LWIP_SO_RCVBUF
  u16_t buflen;
  int recv_avail;
#endif /* LWIP_SO_RCVBUF */
#endif /* LWIP_TCPIP_CORE_LOCKING */

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  if (argp == NULL) {
    sock_set_errno(sock, EFAULT);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }

#if LWIP_TCPIP_CORE_LOCKING
#if LWIP_SO_RCVBUF || LWIP_FIONREAD_LINUXMODE
  if (cmd == FIONREAD) {
    err = lwip_ioctl_internal_FIONREAD(sock, argp);
  } else
#endif /* LWIP_SO_RCVBUF || LWIP_FIONREAD_LINUXMODE */
  {
    LOCK_TCPIP_CORE();
    err = lwip_ioctl_impl(sock, cmd, argp);
    UNLOCK_TCPIP_CORE();
  }
  if (err != ERR_OK) {
    sock_set_errno(sock, err);
    lwip_sock_release_socket_reference(sock);
    return -1;
  }
  lwip_sock_release_socket_reference(sock);
  return 0;
#else
  switch ((u32_t)cmd) {
#if LWIP_SO_RCVBUF || LWIP_FIONREAD_LINUXMODE
    case FIONREAD:
#if LWIP_FIONREAD_LINUXMODE
      if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_TCP) {
        struct pbuf *p = NULL;
        if (sock->lastdata != NULL) {
          p = ((struct netbuf *)sock->lastdata)->p;
          *((int *)argp) = p->tot_len - sock->lastoffset;
        } else {
          struct netbuf *rxbuf = NULL;
          err_t err;
          if (sock->rcvevent <= 0) {
            *((int *)argp) = 0;
          } else {
            err = netconn_recv(sock->conn, &rxbuf);
            if (err != ERR_OK) {
              *((int *)argp) = 0;
            } else {
              sock->lastdata = rxbuf;
              sock->lastoffset = 0;
              *((int *)argp) = rxbuf->p->tot_len;
            }
          }
        }
        if (err != ERR_OK) {
          sock_set_errno(sock, err_to_errno(err));
          lwip_sock_release_socket_reference(sock);
          return -1;
        }
        lwip_sock_release_socket_reference(sock);
        return 0;
      }
#endif /* LWIP_FIONREAD_LINUXMODE */

#if LWIP_SO_RCVBUF
      /* we come here if either LWIP_FIONREAD_LINUXMODE==0 or this is a TCP socket */
      SYS_ARCH_GET(sock->conn->recv_avail, recv_avail);
      if (recv_avail < 0) {
        recv_avail = 0;
      }
      *((int *)argp) = recv_avail;

      /* Check if there is data left from the last recv operation. /maq 041215 */
      if (sock->lastdata != NULL) {
        struct pbuf *p = (struct pbuf *)sock->lastdata;
        if (NETCONNTYPE_GROUP(NETCONN_TYPE(sock->conn)) != NETCONN_TCP) {
          p = ((struct netbuf *)p)->p;
        }
        buflen = p->tot_len;
        buflen = (u16_t)(buflen - sock->lastoffset);

        *((int *)argp) += buflen;
      }

      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_ioctl(%d, FIONREAD, %p) = %"U16_F"\n", s, argp, *((u16_t *)argp)));
      lwip_sock_release_socket_reference(sock);
      return 0;
#else /* LWIP_SO_RCVBUF */
      break;
#endif /* LWIP_SO_RCVBUF */
#endif /* LWIP_SO_RCVBUF || LWIP_FIONREAD_LINUXMODE */

    case FIONBIO:
      val = 0;
      if ((argp != NULL) && (*(u32_t *)argp != 0)) {
        val = 1;
      }
      netconn_set_nonblocking(sock->conn, val);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_ioctl(%d, FIONBIO, %d)\n", s, val));
      lwip_sock_release_socket_reference(sock);
      return 0;

#if LWIP_IPV6
#if LWIP_IPV6_DUP_DETECT_ATTEMPTS
    case SIOCSIPV6DAD:
      lwip_ioctl_internal_SIOCSIPV6DAD(argp);
      return 0;

    case SIOCGIPV6DAD:
      lwip_ioctl_internal_SIOCGIPV6DAD(argp);
      return 0;
#endif
#if LWIP_IOCTL_IPV6DPCTD
    case SIOCSIPV6DPCTD:
      lwip_ioctl_internal_SIOCSIPV6DPCTD(argp);
      return 0;

    case SIOCGIPV6DPCTD:
      lwip_ioctl_internal_SIOCGIPV6DPCTD(argp);
      return 0;
#endif
#endif

    case 0xFFFFFFFF:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_ioctl(%d, INVALID: 0x%lx, %p)\n", s, cmd, argp));
      sock_set_errno(sock, EINVAL);
      lwip_sock_release_socket_reference(sock);
      return -1;

    default:
      break;
  }
  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_ioctl(%d, UNIMPL: 0x%lx, %p)\n", s, cmd, argp));
  sock_set_errno(sock, ENOSYS); /* not yet implemented */
  lwip_sock_release_socket_reference(sock);
  return -1;
#endif
}

#ifdef LWIP_FCNTL
/** A minimal implementation of fcntl.
 * Currently only the commands F_GETFL and F_SETFL are implemented.
 * Only the flag O_NONBLOCK is implemented.
 */
int
lwip_fcntl(int s, int cmd, int val)
{
  struct lwip_sock *sock = NULL;
  int ret = -1;

  sock = lwip_sock_get_socket_reference(s);
  if (sock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  switch (cmd) {
    case F_GETFL:
      ret = netconn_is_nonblocking(sock->conn) ? O_NONBLOCK : 0;
      break;
    case F_SETFL:
      if (val < 0) {
        lwip_sock_release_socket_reference(sock);
        set_errno(EINVAL);
        return -1;
      }

      if (((u32_t)val & ~(u32_t)O_NONBLOCK) == 0) {
        /* only O_NONBLOCK, all other bits are zero */
        netconn_set_nonblocking(sock->conn, (u32_t)val & O_NONBLOCK);
        ret = 0;
      } else {
        sock_set_errno(sock, EINVAL); /* not yet implemented */
      }
      break;
    default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_fcntl(%d, UNIMPL: %d, %d)\n", s, cmd, val));
      sock_set_errno(sock, ENOSYS); /* not yet implemented */
      break;
  }

  lwip_sock_release_socket_reference(sock);
  return ret;
}
#endif

#if LWIP_IGMP
/** Register a new IGMP membership. On socket close, the membership is dropped automatically.
 *
 * ATTENTION: this function is called from tcpip_thread (or under CORE_LOCK).
 *
 * @return 1 on success, 0 on failure
 */
static int
lwip_socket_register_membership(struct lwip_sock *sock, const ip4_addr_t *if_addr, const ip4_addr_t *multi_addr)
{
  int i;

  SYS_ARCH_DECL_PROTECT(lev);

  if (sock == NULL) {
    return 0;
  }

  SYS_ARCH_PROTECT(lev);
#if LWIP_ALLOW_SOCKET_CONFIG
  if (socket_ipv4_multicast_memberships == NULL) {
    socket_ipv4_multicast_memberships =
      mem_malloc(sizeof(struct lwip_socket_multicast_pair) * LWIP_SOCKET_MAX_MEMBERSHIPS);
    if (socket_ipv4_multicast_memberships == NULL) {
      SYS_ARCH_UNPROTECT(lev);
      return 0;
    }
    (void)memset_s(socket_ipv4_multicast_memberships,
                   sizeof(struct lwip_socket_multicast_pair) * LWIP_SOCKET_MAX_MEMBERSHIPS,
                   0,
                   sizeof(struct lwip_socket_multicast_pair) * LWIP_SOCKET_MAX_MEMBERSHIPS);
  }
#endif

  for (i = 0; i < (int)LWIP_SOCKET_MAX_MEMBERSHIPS; i++) {
    if (socket_ipv4_multicast_memberships[i].sock == NULL) {
      socket_ipv4_multicast_memberships[i].sock = sock;
      ip4_addr_copy(socket_ipv4_multicast_memberships[i].if_addr, *if_addr);
      ip4_addr_copy(socket_ipv4_multicast_memberships[i].multi_addr, *multi_addr);
      SYS_ARCH_UNPROTECT(lev);
      return 1;
    }
  }
  SYS_ARCH_UNPROTECT(lev);

  return 0;
}

/* Unregister a previously registered membership. This prevents dropping the membership
 * on socket close.
 *
 * ATTENTION: this function is called from tcpip_thread (or under CORE_LOCK).
 */
static void
lwip_socket_unregister_membership(struct lwip_sock *sock, const ip4_addr_t *if_addr, const ip4_addr_t *multi_addr)
{
  int i;

  SYS_ARCH_DECL_PROTECT(lev);
  if (sock == NULL) {
    return;
  }
  SYS_ARCH_PROTECT(lev);
#if LWIP_ALLOW_SOCKET_CONFIG
  if (socket_ipv4_multicast_memberships == NULL) {
    SYS_ARCH_UNPROTECT(lev);
    return;
  }
#endif

  for (i = 0; i < (int)LWIP_SOCKET_MAX_MEMBERSHIPS; i++) {
    if ((socket_ipv4_multicast_memberships[i].sock == sock) &&
        ip4_addr_cmp(&socket_ipv4_multicast_memberships[i].if_addr, if_addr) &&
        ip4_addr_cmp(&socket_ipv4_multicast_memberships[i].multi_addr, multi_addr)) {
      socket_ipv4_multicast_memberships[i].sock = NULL;
      ip4_addr_set_zero(&socket_ipv4_multicast_memberships[i].if_addr);
      ip4_addr_set_zero(&socket_ipv4_multicast_memberships[i].multi_addr);
      break;
    }
  }
  SYS_ARCH_UNPROTECT(lev);
}

/** Drop all memberships of a socket that were not dropped explicitly via setsockopt.
 *
 * ATTENTION: this function is NOT called from tcpip_thread (or under CORE_LOCK).
 */
static void
lwip_socket_drop_registered_memberships(struct lwip_sock *sock)
{
  int i;

  SYS_ARCH_DECL_PROTECT(lev);

  if (sock == NULL) {
    return;
  }

  SYS_ARCH_PROTECT(lev);
#if LWIP_ALLOW_SOCKET_CONFIG
  if (socket_ipv4_multicast_memberships == NULL) {
    SYS_ARCH_UNPROTECT(lev);
    return;
  }
#endif

  SYS_ARCH_UNPROTECT(lev);

  for (i = 0; i < (int)LWIP_SOCKET_MAX_MEMBERSHIPS; i++) {
    SYS_ARCH_PROTECT(lev);

    if (socket_ipv4_multicast_memberships[i].sock == sock) {
      ip_addr_t multi_addr, if_addr;
      ip_addr_copy_from_ip4(multi_addr, socket_ipv4_multicast_memberships[i].multi_addr);
      ip_addr_copy_from_ip4(if_addr, socket_ipv4_multicast_memberships[i].if_addr);
      socket_ipv4_multicast_memberships[i].sock = NULL;
      ip4_addr_set_zero(&socket_ipv4_multicast_memberships[i].if_addr);
      ip4_addr_set_zero(&socket_ipv4_multicast_memberships[i].multi_addr);

      SYS_ARCH_UNPROTECT(lev);

      (void)netconn_leave_group(sock->conn, &multi_addr, &if_addr, NETCONN_LEAVE);
    } else {
      SYS_ARCH_UNPROTECT(lev);
    }
  }
}
#endif /* LWIP_IGMP */

#ifdef LWIP_GET_CONN_INFO
/* Get dst_mac TCP / IP connections, src_ip, dst_ip, ipid, srcport, dstport,
 * Seq, ack, tcpwin, tsval, tsecr information
 *
 * @param s socket descriptor for which the details are required
 * @param conn parameter in which the acquired TCP / IP connection information is to be saved
 *
 * @note If the UDP connection, seq, ack, tcpwin, tsval, tsecr is set to 0
 * @return 0 if successful; failure to return -1
 */
int
lwip_get_conn_info(int s, void *conninfo)
{
  struct lwip_sock *isock = NULL;
  err_t err;
  int retval = 0;
  int errno_tmp;

  isock = lwip_sock_get_socket_reference(s);
  if (isock == NULL) {
    /* get socket updates errno */
    return -1;
  }

  err = netconn_getconninfo(isock->conn, conninfo);
  if (err != ERR_OK) {
    retval = -1;
    goto RETURN;
  }

RETURN:
  if (err == ERR_VAL) {
    set_errno(EOPNOTSUPP);
  } else {
    errno_tmp =  err_to_errno(err);
    if (errno_tmp) {
      set_errno(errno_tmp);
    }
  }

  lwip_sock_release_socket_reference(isock);
  return retval;
}
#endif

#ifdef LWIP_DEBUG_INFO
void
debug_socket_info(int idx, int filter, int expend)
{
  if ((idx >= (int)LWIP_CONFIG_NUM_SOCKETS) || (idx < 0)) {
    LWIP_PLATFORM_PRINT("problem:idx >= LWIP_CONFIG_NUM_SOCKETS || idx < 0\n");
    return;
  }

  if ((filter != 0) && (atomic_read(&sockets[idx].conn_state) != LWIP_CONN_CLOSED)) {
    LWIP_PLATFORM_PRINT("\n---------------SOCKET\n");
    LWIP_PLATFORM_PRINT("idx      :%d\n", idx);
    LWIP_PLATFORM_PRINT("conn     :%p\n", sockets[idx].conn);
    LWIP_PLATFORM_PRINT("r_event  :%d\n", sockets[idx].rcvevent);
    LWIP_PLATFORM_PRINT("s_event  :%d\n", sockets[idx].sendevent);
    LWIP_PLATFORM_PRINT("e_event  :%d\n", sockets[idx].errevent);
    LWIP_PLATFORM_PRINT("err      :%d\n", atomic_read(&sockets[idx].err));
    LWIP_PLATFORM_PRINT("conn_state :%d\n", atomic_read(&sockets[idx].conn_state));
    LWIP_PLATFORM_PRINT("refcnt :%d\n", atomic_read(&sockets[idx].refcnt));
    LWIP_PLATFORM_PRINT("select_w :%d\n", sockets[idx].select_waiting);
    LWIP_PLATFORM_PRINT("last_offset :%d\n", sockets[idx].lastoffset);
#if LWIP_SOCKET_POLL
    LWIP_PLATFORM_PRINT("poll_prev:%p\n", sockets[idx].wq.poll_queue.pstPrev);
    LWIP_PLATFORM_PRINT("poll_next:%p\n", sockets[idx].wq.poll_queue.pstNext);
#endif/* LWIP_SOCKET_POLL */

    if ((atomic_read(&sockets[idx].conn_state) != LWIP_CONN_CLOSED) && (sockets[idx].conn != NULL) && (expend)) {
      debug_netconn_info((void *)sockets[idx].conn, expend);
    }
  }
}
#endif /* LWIP_DEBUG_INFO */

#if LWIP_IPV6 && LWIP_IPV6_MLD
/* Register a new MLD6 membership. On socket close, the membership is dropped automatically.
 *
 * ATTENTION: this function is called from tcpip_thread (or under CORE_LOCK).
 *
 * @return 1 on success, 0 on failure
 */
static int
lwip_socket_register_mld6_membership(struct lwip_sock *sock, unsigned int if_idx, const ip6_addr_t *multi_addr)
{
  u8_t i;
  /* sock is validated here , multi_addr is  passed a valid pointer by the caller  */
  SYS_ARCH_DECL_PROTECT(lev);

  if (sock == NULL) {
    return 0;
  }
  SYS_ARCH_PROTECT(lev);
#if LWIP_ALLOW_SOCKET_CONFIG
  if (socket_ipv6_multicast_memberships == NULL) {
    socket_ipv6_multicast_memberships =
      mem_malloc(sizeof(struct lwip_socket_multicast_mld6_pair) * LWIP_SOCKET_MAX_MEMBERSHIPS);
    if (socket_ipv6_multicast_memberships == NULL) {
      SYS_ARCH_UNPROTECT(lev);
      return 0;
    }
    (void)memset_s(socket_ipv6_multicast_memberships,
                   sizeof(struct lwip_socket_multicast_mld6_pair) * LWIP_SOCKET_MAX_MEMBERSHIPS,
                   0,
                   sizeof(struct lwip_socket_multicast_mld6_pair) * LWIP_SOCKET_MAX_MEMBERSHIPS);
  }
#endif

  for (i = 0; i < LWIP_SOCKET_MAX_MEMBERSHIPS; i++) {
    if (socket_ipv6_multicast_memberships[i].sock == NULL) {
      socket_ipv6_multicast_memberships[i].sock   = sock;
      socket_ipv6_multicast_memberships[i].if_idx = (u8_t)if_idx;
      ip6_addr_copy(socket_ipv6_multicast_memberships[i].multi_addr, *multi_addr);
      SYS_ARCH_UNPROTECT(lev);

      return 1;
    }
  }

  SYS_ARCH_UNPROTECT(lev);

  return 0;
}

/* Unregister a previously registered MLD6 membership. This prevents dropping the membership
 * on socket close.
 *
 * ATTENTION: this function is called from tcpip_thread (or under CORE_LOCK).
 */
static void
lwip_socket_unregister_mld6_membership(struct lwip_sock *sock, unsigned int if_idx, const ip6_addr_t *multi_addr)
{
  u8_t i;

  SYS_ARCH_DECL_PROTECT(lev);

  /* sock is validated here ,multi_addr is  passed a valid pointer by the caller */
  if (sock == NULL) {
    return;
  }

  SYS_ARCH_PROTECT(lev);
#if LWIP_ALLOW_SOCKET_CONFIG
  if (socket_ipv6_multicast_memberships == NULL) {
    SYS_ARCH_UNPROTECT(lev);
    return;
  }
#endif

  for (i = 0; i < LWIP_SOCKET_MAX_MEMBERSHIPS; i++) {
    if ((socket_ipv6_multicast_memberships[i].sock == sock) &&
        (socket_ipv6_multicast_memberships[i].if_idx == if_idx) &&
        ip6_addr_cmp(&socket_ipv6_multicast_memberships[i].multi_addr, multi_addr)) {
      socket_ipv6_multicast_memberships[i].sock   = NULL;
      socket_ipv6_multicast_memberships[i].if_idx = NETIF_NO_INDEX;
      ip6_addr_set_zero(&socket_ipv6_multicast_memberships[i].multi_addr);
      break;
    }
  }
  SYS_ARCH_UNPROTECT(lev);
}

/* Drop all MLD6 memberships of a socket that were not dropped explicitly via setsockopt.
 *
 * ATTENTION: this function is NOT called from tcpip_thread (or under CORE_LOCK).
 */
static void
lwip_socket_drop_registered_mld6_memberships(struct lwip_sock *sock)
{
  u8_t i;

  SYS_ARCH_DECL_PROTECT(lev);

  if ((sock == NULL) || (sock->conn == NULL)) {
    return;
  }

  SYS_ARCH_PROTECT(lev);
#if LWIP_ALLOW_SOCKET_CONFIG
  if (socket_ipv6_multicast_memberships == NULL) {
    SYS_ARCH_UNPROTECT(lev);
    return;
  }
#endif

  SYS_ARCH_UNPROTECT(lev);

  for (i = 0; i < LWIP_SOCKET_MAX_MEMBERSHIPS; i++) {
    SYS_ARCH_PROTECT(lev);

    if (socket_ipv6_multicast_memberships[i].sock == sock) {
      ip_addr_t multi_addr;
      u8_t if_idx;

      ip_addr_copy_from_ip6(multi_addr, socket_ipv6_multicast_memberships[i].multi_addr);
      if_idx = socket_ipv6_multicast_memberships[i].if_idx;

      socket_ipv6_multicast_memberships[i].sock   = NULL;
      socket_ipv6_multicast_memberships[i].if_idx = NETIF_NO_INDEX;
      ip6_addr_set_zero(&socket_ipv6_multicast_memberships[i].multi_addr);
      SYS_ARCH_UNPROTECT(lev);

      (void)netconn_leave_group_netif(sock->conn, &multi_addr, if_idx, NETCONN_LEAVE);
    } else {
      SYS_ARCH_UNPROTECT(lev);
    }
  }
}
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD  */

#endif /* LWIP_SOCKET */
