/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2015. All rights reserved.
 * Description: shell cmds APIs implementation
 * Author: none
 * Create: 2013
 */

#include "lwip/opt.h"
#include "lwip/init.h"
#include "netif/lowpan6_opts.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
#include "lwip/err.h"
#include "lwip/inet.h"
#include "netif/etharp.h"
#include "lwip/ip_addr.h"
#include "lwip/ip6_addr.h"
#if LWIP_IPV4 && LWIP_IGMP
#include "lwip/igmp.h"
#endif /* LWIP_IPV4 && LWIP_IGMP */
#include "lwip/priv/nd6_priv.h"
#include "lwip/sockets.h"
#include "lwip/inet_chksum.h"
#include "lwip/raw.h"
#include "lwip/priv/api_msg.h"
#include "los_config.h"
#include <string.h>
#include "limits.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

#include "lwip/api_shell.h"
#include "lwip/tftpc.h"

#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/udp.h"
#include "lwip/priv/tcp_priv.h"
#if ((LWIP_IPV6 && (LWIP_IPV6_MLD || LWIP_IPV6_MLD_QUERIER)))
#include "lwip/mld6.h"
#endif /* ((LWIP_IPV6 && (LWIP_IPV6_MLD || LWIP_IPV6_MLD_QUERIER))) */

#include "lwip/dhcp.h"
#if LWIP_DHCPS
#include "lwip/dhcps.h"
#endif
#include "lwip/sntp.h"
#include "lwip/netifapi.h"

#ifdef LOSCFG_SHELL
#include "shcmd.h"
#include "shell.h"
#endif
#if LWIP_LITEOS_TASK
#include "hi_time.h"
#endif
#if LWIP_RIPPLE
#include "rpl_mgmt_api.h"
#include "of_helper.h"
#include "dag.h"
#include "pstore.h"
#include "rpl_common.h"
#endif

#if LWIP_RPL
#include "vpprpl-user-interfaces.h"
#include "vpprpl-cross-platform-interfaces.h"
#endif

#if LWIP_NAT64
#include "lwip/nat64.h"
#endif

#if LWIP_MPL
#include "mcast6_table.h"
#endif

#if LWIP_DHCP_COAP_RELAY
#include "dhcp_relay.h"
#endif

#ifdef CUSTOM_AT_COMMAND
#include "hi_at.h"
#endif

#ifdef LWIP_DEBUG_OPEN
#undef PRINTK
#define PRINTK (void)hi_at_printf
#endif

#define PRINT_BUF_LEN 1024
#define BYTE_CONVERT_UNIT 1024

typedef struct shell_cmd {
  int argc;
  char **argv;
  sys_sem_t cb_completed;
} shell_cmd_t;

struct ifconfig_option {
  char            iface[IFNAMSIZ];
  unsigned int    option;
  ip_addr_t       ip_addr;
  ip_addr_t       netmask;
  ip_addr_t       gw;
  unsigned char   ethaddr[NETIF_MAX_HWADDR_LEN];
  u16_t           mtu;
  u8_t            result;
  /*
   * when using telnet, print to the telnet socket will result in system deadlock.
   * So we cahe the prinf data to a buf, and when the tcpip callback returns,
   * then print the data out to the telnet socket
   */
  sys_sem_t       cb_completed;
  char            cb_print_buf[PRINT_BUF_LEN];
  unsigned int    print_len;
};

struct netstat_data {
  s8_t *netstat_out_buf;
  u32_t netstat_out_buf_len;
  u32_t netstat_out_buf_updated_len;
  sys_sem_t cb_completed;
};

struct if_cmd_data {
  char *if_name;
  err_t err;
  sys_sem_t cb_completed;
};

#if LWIP_ARP && LWIP_ENABLE_IP_CONFLICT_SIGNAL
extern sys_sem_t ip_conflict_detect;
extern u32_t is_ip_conflict_signal;
LWIP_STATIC u32_t g_old_ip4addr;
#endif
#if LWIP_IPV6
extern sys_sem_t dup_addr_detect;
extern u32_t is_dup_detect_initialized;
#endif

extern struct etharp_entry arp_table[ARP_TABLE_SIZE];
extern u32_t sys_now(void);
extern const char *const tcp_state_str[];
#if (defined LWIP_TESTBED) || LWIP_ENABLE_BASIC_SHELL_CMD
extern int get_unused_socket_num(void);
#endif

#define API_SHELL_ERRCODE_USAGE 201U
#define API_SHELL_ERRCODE_TCPIP_UNINTED 202U
#define API_SHELL_ERRCODE_MEM_ERR 203U
#define API_SHELL_ERRCODE_SEM_ERR 204U
#define API_SHELL_ERRCODE_DEV_NOT_FOUND 205U
#define API_SHELL_ERRCODE_DEV_NOT_READY 206U
#define API_SHELL_ERRCODE_INVALID 207U
#define API_SHELL_ERRCODE_SERVICE_FAILURE 208U
#define API_SHELL_ERRCODE_IP_CONFLICT 209U
#define API_SHELL_ERRCODE_DUPLICATE_NETWORK 210U
#define API_SHELL_ERRCODE_NO_ROUTE 211U

#ifndef PRINT_ERRCODE
#define PRINT_ERRCODE(x) PRINTK("ERR:%u"CRLF, (x))
#endif

#ifndef PRINT_ERR
#define PRINT_ERR PRINTK
#endif

#ifndef LWIP_IFCONFIG_SHOW_SINGLE
#define LWIP_IFCONFIG_SHOW_SINGLE 1
#endif

#define RESULT_STRING_LEN 320
#define LWIP_SHELL_CMD_PING6_RETRY_TIMES 4

#define MAX_PRINT_SIZE 256
#define MACADDR_BUF_LEN 32

#define IFCONFIG_OPTION_SET_IP          1
#define IFCONFIG_OPTION_SET_NETMASK     (1 << 1)
#define IFCONFIG_OPTION_SET_GW          (1 << 2)
#define IFCONFIG_OPTION_SET_HW          (1 << 3)
#define IFCONFIG_OPTION_SET_UP          (1 << 4)
#define IFCONFIG_OPTION_SET_DOWN        (1 << 5)
#define IFCONFIG_OPTION_SET_MTU         (1 << 6)
#define IFCONFIG_OPTION_DEL_IP          (1 << 7)

#define NETSTAT_ENTRY_SIZE 120
#define MAX_NETSTAT_ENTRY (NETSTAT_ENTRY_SIZE * (MEMP_NUM_TCP_PCB + MEMP_NUM_UDP_PCB  + MEMP_NUM_TCP_PCB_LISTEN + 1))

#define MAX_MACADDR_STRING_LENGTH    18 /* including NULL */

#define CONVERT_STRING_TO_HEX(_src, _dest) do { \
  const char *_src_string = (char *)(_src); \
  (_dest) = 0; \
  while (*_src_string != '\0') { \
    (_dest) = (unsigned char)(((_dest) << 4) & 0xFF); \
    if ((*_src_string >= '0') && (*_src_string <= '9')) { \
      (_dest) |= (unsigned char)(*_src_string - '0'); \
    } else if ((*_src_string >= 'A') && (*_src_string <= 'F')) { \
      (_dest) |= (unsigned char)((*_src_string - 'A') + 10); \
    } else if ((*_src_string >= 'a') && (*_src_string <= 'f')) { \
      (_dest) |= (unsigned char)((*_src_string - 'a') + 10); \
    } else { \
      break; \
    } \
    ++_src_string; \
  } \
} while (0)

#define LWIP_MSECS_TO_SECS(time_in_msecs) ((time_in_msecs) / 1000)

#if LWIP_ENABLE_BASIC_SHELL_CMD

#ifndef LWIP_TESTBED
LWIP_STATIC
#endif
int print_netif(struct netif *netif, char *print_buf, unsigned int buf_len);
/* Forward Declarations [START] */
#ifndef LWIP_TESTBED
LWIP_STATIC
#endif
void lwip_ifconfig_show_internal(void *arg);
#ifndef LWIP_TESTBED
LWIP_STATIC
#endif
void lwip_ifconfig_internal(void *arg);

void lwip_printsize(size_t size);
#ifndef CUSTOM_AT_COMMAND
LWIP_STATIC void lwip_ifconfig_usage(const char *cmd);
#endif

#endif /* LWIP_ENABLE_BASIC_SHELL_CMD */

#if LWIP_ENABLE_LOS_SHELL_CMD
#ifndef LWIP_TESTBED
LWIP_STATIC
#endif
void lwip_arp_show_internal(struct netif *netif, char *printf_buf, unsigned int buf_len);

#ifndef LWIP_TESTBED
LWIP_STATIC
#endif
void lwip_arp_internal(void *arg);
LWIP_STATIC void lwip_arp_usage(const char *cmd);
void ifup_internal(void *arg);
void ifdown_internal(void *arg);
#ifdef LWIP_DEBUG_INFO
LWIP_STATIC u32_t netdebug_memp(int argc, char **argv);
LWIP_STATIC u32_t netdebug_sock(int argc, char **argv);
u32_t os_shell_netdebug(int argc, char **argv);
u32_t os_shell_ipdebug(int argc, char **argv);
#endif /* LWIP_DEBUG_INFO */
#if LWIP_IPV6
#if ICMP6_STATS
void lwip_print_icmpv6_stat(void);
#endif

#endif /* LWIP_IPV6 */
#endif /* LWIP_ENABLE_LOS_SHELL_CMD */

u32_t os_tcpserver(int argc, char **argv);
void udpserver(int argc, char **argv);
#if LWIP_IPV6
int netstat_get_udp_sendq_len6(struct udp_pcb *udppcb, struct pbuf *udpbuf);
int netstat_udp_sendq6(struct udp_pcb *upcb);
#endif
#if LWIP_IPV4
int netstat_get_udp_sendq_len(struct udp_pcb *udppcb, struct pbuf *udpbuf);
#endif
int netstat_tcp_sendq(struct tcp_pcb *tpcb);
int netstat_tcp_recvq(struct tcp_pcb *tpcb);
int netstat_netconn_recvq(const struct netconn *conn);

int netstat_udp_sendq(struct udp_pcb *upcb);
int netstat_netconn_sendq(struct netconn *conn);
/* Forward Declarations [END] */

#ifndef CUSTOM_AT_COMMAND
LWIP_STATIC void
err_ifconfig_string_put(struct ifconfig_option *ifconfig_cmd, u32_t errcode)
{
  int ret;
  ret = snprintf_s(ifconfig_cmd->cb_print_buf + ifconfig_cmd->print_len,
                   PRINT_BUF_LEN - ifconfig_cmd->print_len,
                   ((PRINT_BUF_LEN - ifconfig_cmd->print_len) - 1), "ERR:%u\n", errcode);
  if ((ret > 0) && ((unsigned int)ret < (PRINT_BUF_LEN - ifconfig_cmd->print_len))) {
    ifconfig_cmd->print_len += (unsigned int)ret;
  }
}
#endif

#if LWIP_ENABLE_BASIC_SHELL_CMD
#ifndef LWIP_TESTBED
LWIP_STATIC
#endif
int print_netif_ip(struct netif *netif, char *print_buf, unsigned int buf_len)
{
  int ret;
  char *tmp = print_buf;
  if (buf_len <= 1) {
    return -1;
  }
#if LWIP_IPV4
#ifdef CUSTOM_AT_COMMAND
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "ip=%s,", ipaddr_ntoa(&netif->ip_addr));
#else
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "ip:%s ", ipaddr_ntoa(&netif->ip_addr));
#endif
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

#ifdef CUSTOM_AT_COMMAND
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "netmask=%s,", ipaddr_ntoa(&netif->netmask));
#else
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "netmask:%s ", ipaddr_ntoa(&netif->netmask));
#endif
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

#ifdef CUSTOM_AT_COMMAND
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "gateway=%s,", ipaddr_ntoa(&netif->gw));
#else
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "gateway:%s"CRLF, ipaddr_ntoa(&netif->gw));
#endif
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;
#endif

#if LWIP_IPV6
  char *addr = NULL;
  int i;
  for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
    /* only PREFERRED addresses are displyaed */
    if (!ip6_addr_isvalid(netif->ip6_addr_state[i])) {
      continue;
    }
    addr = ip6addr_ntoa((const ip6_addr_t *)&netif->ip6_addr[i]);
#ifdef CUSTOM_AT_COMMAND
    if (addr != NULL) {
      ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "ip6=%s,", addr);
      if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
        return -1;
      }
      tmp += ret;
      buf_len -= (unsigned int)ret;
    }
#else
    ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "\tip6: %s/64"CRLF, addr ? addr : "::");
    if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
      return -1;
    }
    tmp += ret;
    buf_len -= (unsigned int)ret;
#endif
  }
#endif

  return (tmp - print_buf);
}

LWIP_STATIC int
print_netif_hw(struct netif *netif, char *print_buf, unsigned int buf_len)
{
  int ret, i;
  char *tmp = print_buf;
  if (buf_len <= 1) {
    return -1;
  }
#ifdef CUSTOM_AT_COMMAND
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "%s", "HWaddr=");
#else
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "%s", "\tHWaddr ");
#endif
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  for (i = 0; i < netif->hwaddr_len - 1; i++) {
    ret = snprintf_s(tmp, buf_len, (buf_len - 1), "%02x:", netif->hwaddr[i]);
    if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
      return -1;
    }
    tmp += ret;
    buf_len -= (unsigned int)ret;
  }

#ifdef CUSTOM_AT_COMMAND
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "%02x,", netif->hwaddr[i]);
#else
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "%02x", netif->hwaddr[i]);
#endif
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;

  return (tmp - print_buf);
}

LWIP_STATIC int
print_netif_link(struct netif *netif, char *print_buf, unsigned int buf_len)
{
  int ret;
  char *tmp = print_buf;
  if (buf_len <= 1) {
    return -1;
  }
#ifdef CUSTOM_AT_COMMAND
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "MTU=%d,", netif->mtu);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  /* 1:Link UP: netcard is connencted; 0:Link Down netcard is disconnect */
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "LinkStatus=%d,",
                   ((netif->flags & NETIF_FLAG_LINK_UP) != 0) ? 1 : 0);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;
#else
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), " MTU:%d %s", netif->mtu,
                   ((netif->flags & NETIF_FLAG_UP) != 0) ? "Running" : "Stop");
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  if (netif_default == netif && netif_is_up(netif)) {
    ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), " %s", "Default");
    if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
      return -1;
    }
    tmp += ret;
    buf_len -= (unsigned int)ret;
  }
#endif

#ifdef CUSTOM_AT_COMMAND
  /* 1:Running state; 0:Stop state */
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "RunStatus=%d"CRLF,
                   ((netif->flags & NETIF_FLAG_UP) != 0) ? 1 : 0);
#else
  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), " %s"CRLF,
                   ((netif->flags & NETIF_FLAG_LINK_UP) != 0) ? "Link UP" : "Link Down");
#endif
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;

  return (tmp - print_buf);
}

#if MIB2_STATS
LWIP_STATIC int
print_netif_mib(struct netif *netif, char *print_buf, unsigned int buf_len)
{
  int ret;
  char *tmp = print_buf;

  if (buf_len <= 1) {
    return -1;
  }

  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "\tRX packets:%u ",
                   netif->mib2_counters.ifinucastpkts + netif->mib2_counters.ifinnucastpkts);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "errors:%u ", netif->mib2_counters.ifinerrors);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "dropped:%u ", netif->mib2_counters.ifindiscards);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "overruns:%u"CRLF, netif->mib2_counters.ifinoverruns);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "\tTX packets:%u ",
                   netif->mib2_counters.ifoutucastpkts + netif->mib2_counters.ifoutnucastpkts);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "errors:%u ", netif->mib2_counters.ifouterrors);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "dropped:%u"CRLF, netif->mib2_counters.ifoutdiscards);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "\tRX bytes:%u ", netif->mib2_counters.ifinoctets);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "TX bytes:%u"CRLF, netif->mib2_counters.ifoutoctets);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return -1;
  }
  tmp += ret;
  return (tmp - print_buf);
}
#endif

#ifndef LWIP_TESTBED
LWIP_STATIC
#endif
int
print_netif(struct netif *netif, char *print_buf, unsigned int buf_len)
{
  int ret;
  char *tmp = print_buf;

  if (buf_len <= 1) {
    return -1;
  }
#ifdef CUSTOM_AT_COMMAND
  /* printf +IFCFG */
  (void)hi_at_printf("+IFCFG:");
#endif

  if (netif->link_layer_type == LOOPBACK_IF) {
#ifdef CUSTOM_AT_COMMAND
    ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "%s,", netif->name);
#else
    ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "%s\t", netif->name);
#endif
  } else {
#ifdef CUSTOM_AT_COMMAND
    ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "%s%u,", netif->name, netif->num);
#else
    ret = snprintf_s(tmp, buf_len, (unsigned int)(buf_len - 1), "%s%u\t", netif->name, netif->num);
#endif
  }
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    goto out;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  ret = print_netif_ip(netif, tmp, buf_len);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    goto out;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  ret = print_netif_hw(netif, tmp, buf_len);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    goto out;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  ret = print_netif_link(netif, tmp, buf_len);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    goto out;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

#if MIB2_STATS
  ret = print_netif_mib(netif, tmp, buf_len);
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    goto out;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;
#endif

#if LWIP_IFCONFIG_SHOW_SINGLE
#ifdef CUSTOM_AT_COMMAND
  (void)hi_at_printf("%s", print_buf);
#else
  PRINTK("%s", print_buf);
#endif
#endif
out:
  return (int)(tmp - print_buf);
}

#ifndef LWIP_TESTBED
LWIP_STATIC
#endif
void
lwip_ifconfig_show_internal(void *arg)
{
  struct netif *netif = NULL;
  struct ifconfig_option *ifconfig_cmd = (struct ifconfig_option *)arg;
  int ret;

  if (netif_list == NULL) {
#ifndef CUSTOM_AT_COMMAND
    err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_DEV_NOT_READY);
#endif
    sys_sem_signal(&ifconfig_cmd->cb_completed);
#ifdef CUSTOM_AT_COMMAND
    (void)hi_at_printf("ERROR"CRLF);
#endif
    return;
  }

  if (ifconfig_cmd->iface[0] == '\0') {
    /* display all netif */
    for (netif = netif_list; netif != NULL; netif = netif->next) {
      ret = print_netif(netif,
                        ifconfig_cmd->cb_print_buf + ifconfig_cmd->print_len,
                        PRINT_BUF_LEN - ifconfig_cmd->print_len);
      if (ret == -1) {
        sys_sem_signal(&ifconfig_cmd->cb_completed);
        return;
      }
      ifconfig_cmd->print_len += (unsigned int)ret;
    }
  } else {
    netif = netif_find(ifconfig_cmd->iface);
    if (netif == NULL) {
#ifndef CUSTOM_AT_COMMAND
      err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_DEV_NOT_FOUND);
#endif
      sys_sem_signal(&ifconfig_cmd->cb_completed);
#ifdef CUSTOM_AT_COMMAND
      (void)hi_at_printf("ERROR"CRLF);
#endif
      return;
    }

    ret = print_netif(netif,
                      ifconfig_cmd->cb_print_buf + ifconfig_cmd->print_len,
                      PRINT_BUF_LEN - ifconfig_cmd->print_len);
    if (ret == -1) {
      sys_sem_signal(&ifconfig_cmd->cb_completed);
      return;
    }
    ifconfig_cmd->print_len += (unsigned int)ret;
  }
  sys_sem_signal(&ifconfig_cmd->cb_completed);

#ifdef CUSTOM_AT_COMMAND
  (void)hi_at_printf("OK"CRLF);
#endif
}

LWIP_STATIC int
lwip_ifconfig_ip(struct netif *netif, struct ifconfig_option *ifconfig_cmd)
{
  ip_addr_t ip_addr;
  struct netif *loc_netif = NULL;
  s8_t idx;
  err_t err;

  ip_addr_set_val(&ip_addr, &(ifconfig_cmd->ip_addr));
  if (IP_IS_V4_VAL(ifconfig_cmd->ip_addr)) {
    /* check the address is not multicast/broadcast/0/loopback */
    if (ip_addr_ismulticast_val(&ip_addr) || ip_addr_isbroadcast_val(&ip_addr, netif) ||
        ip_addr_isany(&ip_addr) || ip_addr_isloopback(&ip_addr)) {
#ifndef CUSTOM_AT_COMMAND
      err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_INVALID);
#endif
      return -1;
    }
    /* lwip disallow two netif sit in same net at the same time */
    loc_netif = netif_list;
    while (loc_netif != NULL) {
      if ((loc_netif == netif) || (ip4_addr_isany_val(loc_netif->netmask.u_addr.ip4))) {
        loc_netif = loc_netif->next;
        continue;
      }
      if (ip_addr_cmp(&netif->netmask, &loc_netif->netmask) &&
          ip_addr_netcmp_val(&loc_netif->ip_addr, &ip_addr, ip_2_ip4(&netif->netmask))) {
#ifndef CUSTOM_AT_COMMAND
        err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_DUPLICATE_NETWORK);
#endif
        return -1;
      }
      loc_netif = loc_netif->next;
    }
    /* reset gateway if new and previous ipaddr not in same net */
    if (!ip_addr_netcmp_val(&ip_addr, &netif->ip_addr, ip_2_ip4(&netif->netmask))) {
      ip_addr_set_zero(&netif->gw);
      if (netif == netif_default) {
        (void)netif_set_default(NULL);
      }
    }

#if LWIP_DHCP
    if ((netif_dhcp_data(netif) != NULL) &&
        (netif_dhcp_data(netif)->client.states[LWIP_DHCP_NATIVE_IDX].state != DHCP_STATE_OFF)) {
      (void)netif_dhcp_off(netif);
    }
#endif
    netif_set_ipaddr(netif, ip_2_ip4(&ip_addr));
  } else if (IP_IS_V6_VAL(ifconfig_cmd->ip_addr)) {
    idx = -1;
    err = netif_add_ip6_address(netif, ip_2_ip6(&ip_addr), &idx);
    if ((err != ERR_OK) || (idx == -1)) {
#ifndef CUSTOM_AT_COMMAND
      err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_MEM_ERR);
#endif
      return -1;
    }
  }
  return 0;
}

LWIP_STATIC int
lwip_ifconfig_netmask(struct netif *netif, struct ifconfig_option *ifconfig_cmd)
{
  ip_addr_t netmask;
  struct netif *loc_netif = NULL;
  ip_addr_set_val(&netmask, &(ifconfig_cmd->netmask));
  /* check data valid */
  if (ip_addr_netmask_valid(ip_2_ip4(&netmask)) == 0) {
#ifndef CUSTOM_AT_COMMAND
    err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_INVALID);
#endif
    return -1;
  }

#if LWIP_DHCP
  if ((netif_dhcp_data(netif) != NULL) &&
      (netif_dhcp_data(netif)->client.states[LWIP_DHCP_NATIVE_IDX].state != DHCP_STATE_OFF)) {
    (void)netif_dhcp_off(netif);
  }
#endif
  if (netif_ip4_netmask(netif)->addr != ip_2_ip4(&netmask)->addr) {
    /* lwip disallow two netif sit in same net at the same time */
    loc_netif = netif_list;
    while (loc_netif != NULL) {
      if (loc_netif == netif) {
        loc_netif = loc_netif->next;
        continue;
      }
      if (ip_addr_cmp(&loc_netif->netmask, &netmask) &&
          ip_addr_netcmp(&loc_netif->ip_addr,
                         &netif->ip_addr, ip_2_ip4(&netmask))) {
#ifndef CUSTOM_AT_COMMAND
        err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_DUPLICATE_NETWORK);
#endif
        return -1;
      }
      loc_netif = loc_netif->next;
    }
    netif_set_netmask(netif, ip_2_ip4(&netmask));
    /* check if gateway still reachable */
    if (!ip_addr_netcmp(&netif->gw,
                        &netif->ip_addr, ip_2_ip4(&netmask))) {
      ip_addr_set_zero(&(netif->gw));
      if (netif == netif_default) {
        (void)netif_set_default(NULL);
      }
    }
  }
  return 0;
}

LWIP_STATIC int
lwip_ifconfig_gw(struct netif *netif, struct ifconfig_option *ifconfig_cmd)
{
  ip_addr_t gw;

  ip_addr_set_val(&gw, &ifconfig_cmd->gw);

  /* check the address multicast/0/loopback */
  if (ip_addr_ismulticast_val(&gw) || ip_addr_isbroadcast_val(&gw, netif) ||
      ip_addr_isany(&gw) || ip_addr_isloopback(&gw)) {
#ifndef CUSTOM_AT_COMMAND
    err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_DUPLICATE_NETWORK);
#endif
    return -1;
  }

  /* check if reachable */
  if (!ip_addr_netcmp_val(&gw, &netif->ip_addr, ip_2_ip4(&netif->netmask))) {
#ifndef CUSTOM_AT_COMMAND
    err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_NO_ROUTE);
#endif
    return -1;
  }

  if (netif_default != netif) {
    ip_addr_set_zero(&netif->gw);
    (void)netif_set_default(netif);
  }

#if LWIP_DHCP
  if ((netif_dhcp_data(netif) != NULL) &&
      (netif_dhcp_data(netif)->client.states[LWIP_DHCP_NATIVE_IDX].state != DHCP_STATE_OFF)) {
    (void)netif_dhcp_off(netif);
  }
#endif
  netif_set_gw(netif, ip_2_ip4(&gw));
  return 0;
}

#ifndef LWIP_TESTBED
LWIP_STATIC
#endif
void
lwip_ifconfig_internal(void *arg)
{
  struct ifconfig_option *ifconfig_cmd = NULL;
  struct netif *netif = NULL;

  ifconfig_cmd = (struct ifconfig_option *)arg;
  netif = netif_find(ifconfig_cmd->iface);
  if (netif == NULL) {
#ifndef CUSTOM_AT_COMMAND
    err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_DEV_NOT_FOUND);
#endif
    ifconfig_cmd->result = LOS_NOK;
    goto out;
  }

  if ((ifconfig_cmd->option & IFCONFIG_OPTION_SET_UP) != 0) {
    (void)netif_set_link_up(netif);
    (void)netif_set_up(netif);
    goto out;
  } else if ((ifconfig_cmd->option & IFCONFIG_OPTION_SET_DOWN) != 0) {
    (void)netif_set_down(netif);
    (void)netif_set_link_down(netif);
    goto out;
  }

  if (((ifconfig_cmd->option & IFCONFIG_OPTION_SET_IP) != 0) ||
      ((ifconfig_cmd->option & IFCONFIG_OPTION_SET_NETMASK) != 0) ||
      ((ifconfig_cmd->option & IFCONFIG_OPTION_SET_HW) != 0)) {
    (void)netif_set_down(netif);
  }

  if ((ifconfig_cmd->option & IFCONFIG_OPTION_SET_IP) != 0) {
    if (lwip_ifconfig_ip(netif, ifconfig_cmd) != 0) {
      (void)netif_set_up(netif);
      ifconfig_cmd->result = LOS_NOK;
      goto out;
    }
  }

  if ((ifconfig_cmd->option & IFCONFIG_OPTION_DEL_IP) != 0) {
    (void)netif_do_rmv_ipv6_addr(netif, &ifconfig_cmd->ip_addr);
  }

  if ((ifconfig_cmd->option & IFCONFIG_OPTION_SET_NETMASK) != 0) {
    if (lwip_ifconfig_netmask(netif, ifconfig_cmd) != 0) {
      (void)netif_set_up(netif);
      ifconfig_cmd->result = LOS_NOK;
      goto out;
    }
  }

  if (((ifconfig_cmd->option & IFCONFIG_OPTION_SET_HW) != 0) &&
      (netif_set_hwaddr(netif, ifconfig_cmd->ethaddr, NETIF_MAX_HWADDR_LEN) != ERR_OK)) {
#ifndef CUSTOM_AT_COMMAND
    err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_SERVICE_FAILURE);
#endif
    (void)netif_set_up(netif);
    ifconfig_cmd->result = LOS_NOK;
    goto out;
  }

  if (((ifconfig_cmd->option & IFCONFIG_OPTION_SET_IP) != 0) ||
      ((ifconfig_cmd->option & IFCONFIG_OPTION_SET_NETMASK) != 0) ||
      ((ifconfig_cmd->option & IFCONFIG_OPTION_SET_HW) != 0)) {
    (void)netif_set_up(netif);
  }

  if ((ifconfig_cmd->option & IFCONFIG_OPTION_SET_GW) != 0) {
    if (lwip_ifconfig_gw(netif, ifconfig_cmd) != 0) {
      ifconfig_cmd->result = LOS_NOK;
      goto out;
    }
  }

  if ((ifconfig_cmd->option & IFCONFIG_OPTION_SET_MTU) != 0) {
    if (netif_set_mtu(netif, ifconfig_cmd->mtu) != ERR_OK) {
#ifndef CUSTOM_AT_COMMAND
      err_ifconfig_string_put(ifconfig_cmd, API_SHELL_ERRCODE_INVALID);
#endif
      ifconfig_cmd->result = LOS_NOK;
    }
  }
out:
  sys_sem_signal(&ifconfig_cmd->cb_completed);
}

void
lwip_printsize(size_t size)
{
  static const char *sizes[] = { "", "K", "M", "G" };
  size_t divis = 0;
  size_t rem = 0;
  while ((size >= BYTE_CONVERT_UNIT) && (divis < ((sizeof(sizes) / sizeof(char *)) - 1))) {
    rem = (size % BYTE_CONVERT_UNIT);
    divis++;
    size /= BYTE_CONVERT_UNIT;
  }
  /* 1024.0 : byte in float unit */
  PRINTK("(%.1f %sB) \r\n", (float)size + (float)rem / 1024.0, sizes[divis]);
}

#ifndef CUSTOM_AT_COMMAND
LWIP_STATIC void
lwip_ifconfig_usage(const char *cmd)
{
  PRINTK("Usage:"\
         CRLF"%s [-a] "\
         CRLF"[interface]"\
         CRLF"[interface ipaddr] <netmask mask> <gateway gw>"\
         CRLF"[interface inet6 add|del ipaddr]"\
         CRLF"[interface hw ether MAC]"\
         CRLF"[interface mtu NN]"\
         CRLF"[interface up|down]"CRLF,
         cmd);
}
#endif

u32_t lwip_get_ipv4(char *localIp, unsigned char ipLen, unsigned char *ifname)
{
  struct netif* netif = NULL;
  int ret;

  if (ifname == NULL) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
    return LOS_NOK;
  }

  netif = netif_find((const char *)ifname);
  if (netif == NULL) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_DEV_NOT_FOUND);
    return LOS_NOK;
  }

  ret = snprintf_s(localIp, (size_t)ipLen, (size_t)(ipLen - 1), "%s", ipaddr_ntoa(&netif->ip_addr));
  if ((ret <= 0) || ((unsigned int)ret >= ipLen)) {
    return LOS_NOK;
  }
  return LOS_OK;
}

u32_t lwip_get_mac_addr(unsigned char *macAddr, unsigned int len, unsigned char *ifname)
{
  struct netif* netif = NULL;
  int i;
  int ret;

  if (ifname == NULL) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
    return LOS_NOK;
  }

  netif = netif_find((const char *)ifname);
  if (netif == NULL) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_DEV_NOT_FOUND);
    return LOS_NOK;
  }

  for (i = 0; i < netif->hwaddr_len - 1; i++) {
    ret = snprintf_s((char *)macAddr, len, (len - 1), "%02x:", netif->hwaddr[i]);
    if ((ret <= 0) || ((unsigned int)ret >= len)) {
      return LOS_NOK;
    }
    macAddr += ret;
    len -= (unsigned int)ret;
  }

  ret = snprintf_s((char *)macAddr, len, (len - 1), "%02x", netif->hwaddr[i]);
  if ((ret <= 0) || ((unsigned int)ret >= len)) {
    return LOS_NOK;
  }
  return LOS_OK;
}

#ifndef CUSTOM_AT_COMMAND
LWIP_STATIC u32_t lwip_ifconfig_all(int argc, const char **argv)
{
  /* To support "ifconfig -a" command
   *   RX packets:XXXX errors:X dropped:X overruns:X bytes:XXXX (Human readable format)
   *   TX packets:XXXX errors:X dropped:X overruns:X bytes:XXXX (Human readable format)
   *
   *   Below is assumed for 'overrun' stat.
   *   Linux Kernel:
   *          RX: FIFO overrun
   *              Data structure: net_device->stats->rx_fifo_errors
   *              Flag which is marked when FIFO overrun: ENRSR_FO
   *
   *          Function: ei_receive->ENRSR_FO
   *
   *          TX: A "FIFO underrun" occurred during transmit.
   *              Data structure: net_device->stats->tx_fifo_errors
   *              Flag which is marked when FIFO underrun: ENTSR_FU
   *
   *          Function: ei_tx_intr->ENTSR_FU
   *
   *  LWIP:
   *      So in our case,
   *      while receiving a packet RX case, if the buffer is full (trypost - it is sys_mbox_trypost)
   *      the error will be returned, we can consider that an overflow has happend.
   *      So this can be RX overrun.
   *
   *      But while transmitting a packet TX case, underrun cannot happen because it block on the
   *      message Q if it is full (NOT trypost - it is sys_mbox_post). So TX overrun is always 0.
   */
#if LWIP_STATS
  u32_t stat_err_cnt;
  u32_t stat_drop_cnt;
  u32_t stat_rx_or_tx_cnt;
  u32_t stat_rx_or_tx_bytes;

  stat_rx_or_tx_cnt     = lwip_stats.ip.recv;
  stat_err_cnt          = (u32_t)(lwip_stats.ip.ip_rx_err
                                  + lwip_stats.ip.lenerr
                                  + lwip_stats.ip.chkerr
                                  + lwip_stats.ip.opterr
                                  + lwip_stats.ip.proterr);
  stat_drop_cnt         = (u32_t)(lwip_stats.ip.drop + lwip_stats.link.link_rx_drop);
  stat_rx_or_tx_bytes = lwip_stats.ip.ip_rx_bytes;

  PRINTK("%18s:%u\t errors:%u\t ip dropped:%u\t link dropped:%u\t overrun:%d\t bytes:%u ",
         "RX packets",
         stat_rx_or_tx_cnt,
         stat_err_cnt,
         stat_drop_cnt,
         lwip_stats.link.link_rx_drop,
         lwip_stats.ip.link_rx_overrun,
         stat_rx_or_tx_bytes);

  /* Print in Human readable format of the incoming bytes */
  lwip_printsize(lwip_stats.ip.ip_rx_bytes);
#if IP6_STATS
  stat_rx_or_tx_cnt     = lwip_stats.ip6.recv;
  stat_err_cnt          = (u32_t)(lwip_stats.ip6.ip_rx_err
                                  + lwip_stats.ip6.lenerr
                                  + lwip_stats.ip6.chkerr
                                  + lwip_stats.ip6.opterr
                                  + lwip_stats.ip6.proterr);
  stat_drop_cnt         = lwip_stats.ip6.drop;
  stat_rx_or_tx_bytes = lwip_stats.ip6.ip_rx_bytes;

  PRINTK("%18s:%u\t errors:%u\t dropped:%u\t overrun:%d\t bytes:%u ",
         "RX packets(ip6)",
         stat_rx_or_tx_cnt,
         stat_err_cnt,
         stat_drop_cnt,
         lwip_stats.ip.link_rx_overrun,
         stat_rx_or_tx_bytes);

  /* Print in Human readable format of the incoming bytes */
  lwip_printsize(lwip_stats.ip6.ip_rx_bytes);
#endif
  stat_rx_or_tx_cnt     = (u32_t)(lwip_stats.ip.fw + lwip_stats.ip.xmit);
  stat_err_cnt          = (u32_t)(lwip_stats.ip.rterr + lwip_stats.ip.ip_tx_err);
  /* IP layer drop stat param is not maintained, failure at IP is considered in 'errors' stat */
  stat_drop_cnt         = lwip_stats.link.link_tx_drop;
  stat_rx_or_tx_bytes   = lwip_stats.ip.ip_tx_bytes;

  PRINTK("%18s:%u\t errors:%u\t link dropped:%u\t overrun:0\t bytes:%u",
         "TX packets",
         stat_rx_or_tx_cnt,
         stat_err_cnt,
         stat_drop_cnt,
         stat_rx_or_tx_bytes);

  /* Print in Human readable format of the outgoing bytes */
  lwip_printsize(lwip_stats.ip.ip_tx_bytes);

  stat_rx_or_tx_cnt     = (u32_t)(lwip_stats.ip6.fw + lwip_stats.ip6.xmit);
  stat_err_cnt          = (u32_t)(lwip_stats.ip6.rterr + lwip_stats.ip6.ip_tx_err);
  stat_rx_or_tx_bytes   = lwip_stats.ip6.ip_tx_bytes;

  PRINTK("%18s:%u\t errors:%u\t overrun:0\t bytes:%u",
         "TX packets(ip6)",
         stat_rx_or_tx_cnt,
         stat_err_cnt,
         stat_rx_or_tx_bytes);

  /* Print in Human readable format of the outgoing bytes */
  lwip_printsize(lwip_stats.ip6.ip_tx_bytes);
#endif /* LWIP_STATS */
  return 0;
}
#endif

LWIP_STATIC u32_t lwip_ifconfig_callback(tcpip_callback_fn function, struct ifconfig_option *ifconfig_cmd)
{
  err_t ret;
  if ((ret = tcpip_callback(function, ifconfig_cmd)) != ERR_OK) {
    sys_sem_free(&ifconfig_cmd->cb_completed);
    PRINT_ERRCODE(API_SHELL_ERRCODE_SERVICE_FAILURE);
    return LOS_NOK;
  }
  (void)sys_arch_sem_wait(&ifconfig_cmd->cb_completed, 0);
  sys_sem_free(&ifconfig_cmd->cb_completed);

  ifconfig_cmd->cb_print_buf[PRINT_BUF_LEN - 1] = '\0';
  if (ifconfig_cmd->result != LOS_OK) {
    return LOS_NOK;
  }
  return LOS_OK;
}

LWIP_STATIC u32_t lwip_ifconfig_print_all(struct ifconfig_option *ifconfig_cmd)
{
  u32_t ret = lwip_ifconfig_callback(lwip_ifconfig_show_internal, ifconfig_cmd);
  if (ret != 0) {
    return ret;
  }

#if (LWIP_IFCONFIG_SHOW_SINGLE != lwIP_TRUE)
#ifdef CUSTOM_AT_COMMAND
  (void)hi_at_printf("+IFCONFIG:");
  (void)hi_at_printf("%s", ifconfig_cmd->cb_print_buf);
  (void)hi_at_printf("OK"CRLF);
#else
  PRINTK("%s", ifconfig_cmd->cb_print_buf);
#endif
#endif
  return LOS_OK;
}

LWIP_STATIC u32_t lwip_ifconfig_up_down(struct ifconfig_option *ifconfig_cmd)
{
  u32_t ret = lwip_ifconfig_callback(lwip_ifconfig_internal, ifconfig_cmd);
  if (ret != 0) {
    return ret;
  }

#ifndef CUSTOM_AT_COMMAND
  PRINTK("%s", ifconfig_cmd->cb_print_buf);
  return 0;
#else
  return LOS_NOK;
#endif
}

LWIP_STATIC u32_t
lwip_ifconfig_prase_inet(int *argc, const char **argv, int *idx,
                         struct ifconfig_option *ifconfig_cmd)
{
#if LWIP_ARP
  struct netif *netiftmp = NULL;
#endif /* LWIP_ARP */

  /* check if set the ip address. */
#if LWIP_ARP
  netiftmp = netif_find(ifconfig_cmd->iface);
  if (NULL == netiftmp) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_DEV_NOT_FOUND);
    return EINVAL;
  }
  g_old_ip4addr = ipaddr_addr(ipaddr_ntoa(&netiftmp->ip_addr));
#endif /* LWIP_ARP */
  if ((strcmp(argv[*idx], "inet") == 0) || (ip4addr_aton(argv[*idx], ip_2_ip4(&ifconfig_cmd->ip_addr)) != 0)) {
    if (strcmp(argv[*idx], "inet") == 0) {
      if (*argc <= 1) {
        return EINVAL;
      }

      if (ip4addr_aton(argv[*idx + 1], ip_2_ip4(&ifconfig_cmd->ip_addr)) == 0) {
        PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
        return EINVAL;
      }
      (*argc)--;
      (*idx)++;
    }
    IP_SET_TYPE_VAL((ifconfig_cmd->ip_addr), IPADDR_TYPE_V4);
#if LWIP_ARP
    if (!ip_addr_cmp(&ifconfig_cmd->ip_addr, &netiftmp->ip_addr)) {
      ifconfig_cmd->option |= IFCONFIG_OPTION_SET_IP;
    }
#else
    ifconfig_cmd->option |= IFCONFIG_OPTION_SET_IP;
#endif /* LWIP_ARP */
    (*argc)--;
    (*idx)++;
  } else if (strcmp(argv[*idx], "inet6") == 0) {
    /* 3 : min argc parameter num from command line */
    if (*argc < 3) {
      return LOS_NOK;
    }
    if ((strcmp(argv[*idx + 1], "add") != 0) && (strcmp(argv[*idx + 1], "del") != 0)) {
      return LOS_NOK;
    }
    /* 2 : skip two argv index */
    if (ip6addr_aton(argv[*idx + 2], ip_2_ip6(&ifconfig_cmd->ip_addr)) == 0) {
      PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
      return EINVAL;
    }

    IP_SET_TYPE_VAL((ifconfig_cmd->ip_addr), IPADDR_TYPE_V6);
    ifconfig_cmd->option |= (!strcmp(argv[*idx + 1], "add") ? IFCONFIG_OPTION_SET_IP : IFCONFIG_OPTION_DEL_IP);
    *argc -= 3; /* 3: skip already check three parameter */
    (*idx) += 3; /* 3: skip three argv index */
  }

  if (((ifconfig_cmd->option & IFCONFIG_OPTION_DEL_IP) != 0) && (*argc != 0)) {
    return LOS_NOK;
  }

  return LOS_OK;
}

LWIP_STATIC u32_t
lwip_ifconfig_prase_additional(int *argc, const char **argv, int *idx,
                               struct ifconfig_option *ifconfig_cmd)
{
  while (*argc > 0) {
    /* if set netmask */
    if ((strcmp("netmask", argv[*idx]) == 0) && (*argc > 1) &&
        (ipaddr_addr(argv[*idx + 1]) != IPADDR_NONE)) {
      ip_addr_set_ip4_u32_val(&(ifconfig_cmd->netmask), ipaddr_addr(argv[*idx + 1]));
      ifconfig_cmd->option |= IFCONFIG_OPTION_SET_NETMASK;
      *idx += 2; /* 2: skip two argv index */
      *argc -= 2; /* 2: skip already check two parameter */
    } else if ((strcmp("gateway", argv[*idx]) == 0) && (*argc > 1) && /* if set gateway */
               (ipaddr_addr(argv[*idx + 1]) != IPADDR_NONE)) {
      ip_addr_set_ip4_u32_val(&(ifconfig_cmd->gw), ipaddr_addr(argv[*idx + 1]));
      ifconfig_cmd->option |= IFCONFIG_OPTION_SET_GW;
      *idx += 2; /* 2: skip two argv index */
      *argc -= 2; /* 2: skip already check two parameter */
    } else if ((strcmp("hw", argv[*idx]) == 0) && (*argc > 2) && /* 2 : if set HWaddr and more than two parameter */
               (strcmp("ether", argv[*idx + 1]) == 0)) {
      char *digit = NULL;
      u32_t macaddrlen = strlen(argv[*idx + 2]) + 1; /* 2: skip two argv index */
      char tmp_str[MAX_MACADDR_STRING_LENGTH];
      char *tmp_str1 = NULL;
      char *saveptr = NULL;
      int j;

      if (macaddrlen != MAX_MACADDR_STRING_LENGTH) {
        PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
        return EINVAL;
      }
      /* 2: skip two argv index */
      if (strncpy_s(tmp_str, MAX_MACADDR_STRING_LENGTH, argv[*idx + 2], macaddrlen - 1) != EOK) {
        PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
        return EINVAL;
      }
      /* 6 : max ':' num */
      for (j = 0, tmp_str1 = tmp_str; j < 6; j++, tmp_str1 = NULL) {
        digit = strtok_r(tmp_str1, ":", &saveptr);
        if ((digit == NULL) || (strlen(digit) > 2)) { /* 2 : char len */
          PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
          return EINVAL;
        }
        CONVERT_STRING_TO_HEX(digit, ifconfig_cmd->ethaddr[j]);
      }
      ifconfig_cmd->option |= IFCONFIG_OPTION_SET_HW;
      *idx += 3; /* 3: skip already check three parameter */
      *argc -= 3; /* 3: skip three argv index */
    } else if ((strcmp("mtu", argv[*idx]) == 0) && (*argc > 1)) { /* if set mtu */
      if ((atoi(argv[*idx + 1]) < 0) || (atoi(argv[*idx + 1]) > 0xFFFF)) { /* 0xFFFF : max argv num */
        PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
        return EINVAL;
      }

      ifconfig_cmd->mtu = (u16_t)(atoi(argv[*idx + 1]));
      ifconfig_cmd->option |= IFCONFIG_OPTION_SET_MTU;
      *idx += 2; /* 2: skip two argv index */
      *argc -= 2; /* 2: skip already check two parameter */
    } else {
      return LOS_NOK;
    }
  }

  return LOS_OK;
}

#if LWIP_DHCP && LWIP_DHCP_SUBSTITUTE
LWIP_STATIC err_t
lwip_ifconfig_dhcp_addr_clients_check_fn(struct netif *nif, void *arg)
{
  struct ifconfig_option *ifconfig_cmd = (struct ifconfig_option *)arg;
  struct netif *netif = netif_find(ifconfig_cmd->iface);
  s32_t ret;
  (void)nif;

  if (netif == NULL) {
    ifconfig_cmd->result = lwIP_FALSE;
    return ERR_OK;
  }
  ret = dhcp_netif_addr_clients_check(netif, ip_2_ip4(&ifconfig_cmd->ip_addr));
  if ((ret == lwIP_TRUE) && (!ip_addr_cmp(&ifconfig_cmd->ip_addr, &netif->ip_addr))) {
    ifconfig_cmd->result = lwIP_TRUE;
  } else {
    ifconfig_cmd->result = lwIP_FALSE;
  }

  return ERR_OK;
}

LWIP_STATIC u8_t
lwip_ifconfig_dhcp_addr_clients_check(struct ifconfig_option *ifconfig_cmd)
{
  (void)netifapi_call_argcb(lwip_ifconfig_dhcp_addr_clients_check_fn, (void *)ifconfig_cmd);
  return ifconfig_cmd->result;
}
#endif

LWIP_STATIC u32_t
lwip_ifconfig_prase_args(int *argc, const char **argv, int *idx,
                         struct ifconfig_option *ifconfig_cmd)
{
  u32_t ret = lwip_ifconfig_prase_inet(argc, argv, idx, ifconfig_cmd);
  if (ret != 0) {
    return ret;
  }

  ret = lwip_ifconfig_prase_additional(argc, argv, idx, ifconfig_cmd);
  if (ret != 0) {
    return ret;
  }

#if LWIP_DHCP && LWIP_DHCP_SUBSTITUTE
  if (((ifconfig_cmd->option & IFCONFIG_OPTION_SET_IP) != 0) && IP_IS_V4_VAL((ifconfig_cmd->ip_addr))) {
    if (lwip_ifconfig_dhcp_addr_clients_check(ifconfig_cmd) == lwIP_TRUE) {
      PRINT_ERRCODE(API_SHELL_ERRCODE_IP_CONFLICT);
      return EINVAL;
    }
  }
#endif

#if LWIP_ARP && LWIP_ENABLE_IP_CONFLICT_SIGNAL
  if (((ifconfig_cmd->option & IFCONFIG_OPTION_SET_IP) != 0) && IP_IS_V4_VAL((ifconfig_cmd->ip_addr))) {
    /* Create the semaphore for ip conflict detection. */
    if (sys_sem_new(&ip_conflict_detect, 0) != ERR_OK) {
      PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
      return EINVAL;
    }
    is_ip_conflict_signal = 1;
  }
#endif /* LWIP_ARP && LWIP_ENABLE_IP_CONFLICT_SIGNAL */

#if LWIP_IPV6
  if (((ifconfig_cmd->option & IFCONFIG_OPTION_SET_IP) != 0) && IP_IS_V6_VAL((ifconfig_cmd->ip_addr))) {
    /* Create the semaphore for duplicate address detection. */
    if (sys_sem_new(&dup_addr_detect, 0) != ERR_OK) {
      PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
      return EINVAL;
    }
    is_dup_detect_initialized = 1;
  }
#endif /* LWIP_IPV6 */
  return LOS_OK;
}

LWIP_STATIC void
lwip_ifconfig_conflict_res_free(struct ifconfig_option *ifconfig_cmd)
{
#if LWIP_ARP && LWIP_ENABLE_IP_CONFLICT_SIGNAL
    if (((ifconfig_cmd->option & IFCONFIG_OPTION_SET_IP) != 0) && IP_IS_V4_VAL((ifconfig_cmd->ip_addr))) {
      is_ip_conflict_signal = 0;
      sys_sem_free(&ip_conflict_detect);
    }
#endif /* LWIP_ARP && LWIP_ENABLE_IP_CONFLICT_SIGNAL */

#if LWIP_IPV6
    if (((ifconfig_cmd->option & IFCONFIG_OPTION_SET_IP) != 0) && IP_IS_V6_VAL((ifconfig_cmd->ip_addr))) {
      is_dup_detect_initialized = 0;
      sys_sem_free(&dup_addr_detect);
    }
#endif /* LWIP_IPV6 */
}

LWIP_STATIC u32_t
lwip_ifconfig_conflict_process(struct ifconfig_option *ifconfig_cmd)
{
#if LWIP_ARP && LWIP_ENABLE_IP_CONFLICT_SIGNAL
  err_t err;
  /* Pend 2 seconds for waiting the arp reply if the ip is already in use. */
  if (((ifconfig_cmd->option & IFCONFIG_OPTION_SET_IP) != 0) && IP_IS_V4_VAL((ifconfig_cmd->ip_addr))) {
    err = (err_t)sys_arch_sem_wait(&ip_conflict_detect, DUP_ARP_DETECT_TIME);
    is_ip_conflict_signal = 0;
    sys_sem_free(&ip_conflict_detect);
    if (err < 0) {
      /* The result neither conflict nor timeout. */
      PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
      return LOS_NOK;
    } else if (err < DUP_ARP_DETECT_TIME) {
      /* Duplicate use of new ip, restore it to the old one. */
      PRINT_ERRCODE(API_SHELL_ERRCODE_IP_CONFLICT);
      ip_addr_set_ip4_u32_val(&ifconfig_cmd->ip_addr, g_old_ip4addr);
      if (sys_sem_new(&ifconfig_cmd->cb_completed, 0) != ERR_OK) {
        PRINT_ERRCODE(API_SHELL_ERRCODE_SEM_ERR);
       return LOS_NOK;
      }
      if ((err = tcpip_callback(lwip_ifconfig_internal, ifconfig_cmd)) != ERR_OK) {
        PRINT_ERRCODE(API_SHELL_ERRCODE_SERVICE_FAILURE);
        return LOS_NOK;
      }
      (void)sys_arch_sem_wait(&ifconfig_cmd->cb_completed, 0);
      ifconfig_cmd->cb_print_buf[PRINT_BUF_LEN - 1] = '\0';
#ifndef CUSTOM_AT_COMMAND
      PRINTK("%s", ifconfig_cmd->cb_print_buf);
#endif
      return LOS_NOK;
    }
  }
#endif /* LWIP_ARP && LWIP_ENABLE_IP_CONFLICT_SIGNAL */
#if LWIP_IPV6
  if (((ifconfig_cmd->option & IFCONFIG_OPTION_SET_IP) != 0) && IP_IS_V6_VAL(ifconfig_cmd->ip_addr)) {
    /* Pend 2 seconds for waiting the arp reply if the ip is already in use. */
    u32_t retval = sys_arch_sem_wait(&dup_addr_detect, DUP_ARP_DETECT_TIME);
    is_dup_detect_initialized = 0;
    sys_sem_free(&dup_addr_detect);

    if (retval == SYS_ARCH_ERROR) {
      /* The result neither conflict nor timeout. */
      PRINT_ERRCODE(API_SHELL_ERRCODE_SERVICE_FAILURE);
      return LOS_NOK;
    } else if (retval < DUP_ARP_DETECT_TIME) {
      /* Duplicate use of new ip, restore it to the old one. */
      struct netif *netif = NULL;
      PRINT_ERRCODE(API_SHELL_ERRCODE_IP_CONFLICT);
      netif = netif_find(ifconfig_cmd->iface);
      s8_t i = netif_get_ip6_addr_match(netif, &ifconfig_cmd->ip_addr.u_addr.ip6);
      if (i >= 0) {
        netif->ip6_addr_state[i] = IP6_ADDR_INVALID;
      }

      ifconfig_cmd->cb_print_buf[PRINT_BUF_LEN - 1] = '\0';
#ifndef CUSTOM_AT_COMMAND
      PRINTK("%s", ifconfig_cmd->cb_print_buf);
#endif
      return LOS_NOK;
    }
  }
#endif /* LWIP_IPV6 */

  return LOS_OK;
}

LWIP_STATIC u32_t
lwip_ifconfig_basic(int *argc, const char **argv, int *idx,
                    struct ifconfig_option *ifconfig_cmd)
{
  /* Get the interface */
  if (*argc > 0) {
    if (strlen(argv[*idx]) < IFNAMSIZ) {
      (void)strncpy_s(ifconfig_cmd->iface, IFNAMSIZ, argv[*idx], (strlen(argv[*idx])));
      ifconfig_cmd->iface[IFNAMSIZ - 1] = '\0';
    } else {
      sys_sem_free(&ifconfig_cmd->cb_completed);
      PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
      return 1;
    }
    (*idx)++;
    (*argc)--;
  }

  if (*argc == 0) {
    return lwip_ifconfig_print_all(ifconfig_cmd);
  }

  /* ifup/ifdown */
  if (strcmp("up", argv[*idx]) == 0) {
    ifconfig_cmd->option |= IFCONFIG_OPTION_SET_UP;
  } else if (strcmp("down", argv[*idx]) == 0) {
    ifconfig_cmd->option |= IFCONFIG_OPTION_SET_DOWN;
  }

  if ((ifconfig_cmd->option & (IFCONFIG_OPTION_SET_UP | IFCONFIG_OPTION_SET_DOWN)) != 0) {
    return lwip_ifconfig_up_down(ifconfig_cmd);
  }

  /* not a basic process, continue next process */
  return (u32_t)(-1);
}

u32_t
lwip_ifconfig(int argc, const char **argv)
{
  int i;
  u32_t ret;
  if (argv == NULL) {
    return LOS_NOK;
  }

  if (!tcpip_init_finish) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_TCPIP_UNINTED);
    return LOS_NOK;
  }

#ifndef CUSTOM_AT_COMMAND
  if (argc != 0 && strcmp("-a", argv[0]) == 0) {
    return lwip_ifconfig_all(argc, argv);
  }
#endif

  struct ifconfig_option *ifconfig_cmd = mem_malloc(sizeof(struct ifconfig_option));
  if (ifconfig_cmd == NULL) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_MEM_ERR);
    return LOS_NOK;
  }
  (void)memset_s(ifconfig_cmd, sizeof(struct ifconfig_option), 0, sizeof(struct ifconfig_option));
  if (sys_sem_new(&ifconfig_cmd->cb_completed, 0) != ERR_OK) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_SEM_ERR);
    ret = LOS_NOK;
    goto ifconfig_free_and_return;
  }

  i = 0;
  ret = lwip_ifconfig_basic(&argc, argv, &i, ifconfig_cmd);
  if (ret != (u32_t)(-1)) {
    goto ifconfig_free_and_return;
  }

  ret = lwip_ifconfig_prase_args(&argc, argv, &i, ifconfig_cmd);
  if (ret != 0) {
#ifndef CUSTOM_AT_COMMAND
    if (ret != EINVAL) {
      lwip_ifconfig_usage("ifconfig");
    }
#endif
    sys_sem_free(&ifconfig_cmd->cb_completed);
    ret = LOS_NOK;
    goto ifconfig_free_and_return;
  }

  ret = lwip_ifconfig_callback(lwip_ifconfig_internal, ifconfig_cmd);
  if (ret != 0) {
    lwip_ifconfig_conflict_res_free(ifconfig_cmd);
    PRINT_ERRCODE(API_SHELL_ERRCODE_SERVICE_FAILURE);
    goto ifconfig_free_and_return;
  }
#ifndef CUSTOM_AT_COMMAND
  PRINTK("%s", ifconfig_cmd->cb_print_buf);
#endif

  ret = lwip_ifconfig_conflict_process(ifconfig_cmd);
#ifdef CUSTOM_AT_COMMAND
  if (ret == LOS_OK) {
    (void)hi_at_printf("OK"CRLF);
  }
#endif

ifconfig_free_and_return:
  mem_free(ifconfig_cmd);
  return ret;
}
#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(ifconfig_shellcmd, CMD_TYPE_EX, "ifconfig", XARGS, (CmdCallBackFunc)lwip_ifconfig);
#endif /* LOSCFG_SHELL */

#ifdef CUSTOM_AT_COMMAND
u32_t
ping_get_ip_value(const char *cmd, char *arg, u32_t arg_len, u32_t *cmd_offset)
{
    const char *cmd_copy = cmd;
    u32_t pos = 0;
    if ((cmd == NULL) || (arg == NULL) || (cmd_offset == NULL)) {
      return LOS_NOK;
    }
    while (*cmd_copy == '.') {
        ++cmd_copy;
    }

    while ((*cmd_copy != '.') && (*cmd_copy != '\0')) {
        arg[pos] = *cmd_copy;
        ++pos;
        ++cmd_copy;
        if (pos >= arg_len) {
            return LOS_NOK;
        }
    }
    arg[pos] = '\0';

    if (pos == 0) {
        return LOS_NOK;
    }

    *cmd_offset = (u32_t)(cmd_copy - cmd);
    return LOS_OK;
}
#endif

#if LWIP_DNS
#ifdef CUSTOM_AT_COMMAND
u32_t os_shell_show_dns(void)
{
    ip_addr_t dns;
    err_t err;
    int i;
    (void)hi_at_printf("+DNS:"CRLF);
    for (i = 0; i < DNS_MAX_SERVERS; i++) {
      err = lwip_dns_getserver((u8_t)i, &dns);
      if (err == ERR_OK) {
        (void)hi_at_printf("%s"CRLF, ipaddr_ntoa(&dns));
      } else {
          return LOS_NOK;
      }
    }
    (void)hi_at_printf("OK"CRLF);
    return LOS_OK;
}
#endif
#endif

#if LWIP_DNS
u32_t
os_shell_dns(int argc, const char **argv)
{
  ip_addr_t dns;
  err_t err;
  int i;

  if (tcpip_init_finish == 0) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_TCPIP_UNINTED);
    return LOS_NOK;
  }
  if (argv == NULL) {
    return LOS_NOK;
  }

  if ((argc == 1) && (strcmp(argv[0], "-a") == 0)) {
    for (i = 0; i < DNS_MAX_SERVERS; i++) {
      err = lwip_dns_getserver((u8_t)i, &dns);
      if (err == ERR_OK) {
#ifdef CUSTOM_AT_COMMAND
        (void)hi_at_printf("dns %d: %s"CRLF, i + 1, ipaddr_ntoa(&dns));
#else
        PRINTK("dns %d: %s"CRLF, i + 1, ipaddr_ntoa(&dns));
#endif
      } else {
#ifdef CUSTOM_AT_COMMAND
        (void)hi_at_printf("dns: failed"CRLF);
#else
        PRINTK("dns: failed"CRLF);
#endif
        return LOS_NOK;
      }
    }
#ifdef CUSTOM_AT_COMMAND
    (void)hi_at_printf("OK"CRLF);
#endif
    return LOS_OK;
  } else if (argc == 2) { /* 2 : current argc index */
    i = atoi(argv[0]);
    if ((i <= 0) || (i > DNS_MAX_SERVERS)) {
      goto usage;
    }
#if LWIP_IPV6
    if (ip6addr_aton(argv[1], ((ip6_addr_t *)&dns))) {
#if LWIP_IPV4 && LWIP_IPV6
      dns.type = IPADDR_TYPE_V6;
#endif
#ifdef LWIP_DNS_GLOBAL_ADDR
      if (!ip6_addr_isglobal((ip6_addr_t *)&dns)) {
#ifdef CUSTOM_AT_COMMAND
        (void)hi_at_printf("ip address<%s> is wrong"CRLF, argv[1]);
#else
        PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
#endif
        return LOS_NOK;
      }
#endif
    } else
#endif
    {
#if LWIP_IPV4
      ((ip4_addr_t *)&dns)->addr = ipaddr_addr(argv[1]);
      if (((ip4_addr_t *)&dns)->addr == IPADDR_NONE) {
#ifdef CUSTOM_AT_COMMAND
        (void)hi_at_printf("ip address<%s> is wrong"CRLF, argv[1]);
#else
        PRINT_ERRCODE(API_SHELL_ERRCODE_INVALID);
#endif
        return LOS_NOK;
      }
#if LWIP_IPV4 && LWIP_IPV6
      dns.type = IPADDR_TYPE_V4;
#endif
#endif
    }

    err = lwip_dns_setserver((u8_t)(i - 1), &dns);
    if (err != ERR_OK) {
#ifdef CUSTOM_AT_COMMAND
      (void)hi_at_printf("dns : failed"CRLF);
#else
      PRINTK("dns : failed"CRLF);
#endif
      return LOS_NOK;
    }
#ifdef CUSTOM_AT_COMMAND
    (void)hi_at_printf("OK"CRLF);
#endif
    return LOS_OK;
  }
usage:
#ifndef CUSTOM_AT_COMMAND
  PRINTK("usage:"CRLF);
  PRINTK("\tdns <1-%d> <IP>"CRLF, DNS_MAX_SERVERS);
  PRINTK("\tdns -a"CRLF);
#endif
  return LOS_NOK;
}
#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(dns_shellcmd, CMD_TYPE_EX, "dns", XARGS, (CmdCallBackFunc)os_shell_dns);
#endif /* LOSCFG_SHELL */
#endif /* LWIP_DNS */

#if LWIP_DHCP
err_t dhcp_idx_to_mac(const struct netif *netif, dhcp_num_t mac_idx, u8_t *hwaddr, u8_t *hwaddr_len);
static void
dhcp_client_info_show(struct dhcp_client *dhcp)
{
  ip4_addr_t cli_ip;
  struct dhcp_state *dhcp_state = &((dhcp->states)[dhcp->cli_idx]);
  char macaddr[MACADDR_BUF_LEN];
  char *char_p = macaddr;
  u8_t i;
  u8_t state;

  (void)memset_s(macaddr, sizeof(macaddr), 0, sizeof(macaddr));
  for (i = 0; i < dhcp_state->hwaddr_len; i++) {
    /* 2 : macaddr show in two hex */
    (void)snprintf_s(char_p, (size_t)(MACADDR_BUF_LEN - 2 * i), (size_t)(MACADDR_BUF_LEN - 2 * i - 1),
                     "%02x", dhcp->hwaddr[i]);
    /* 2 : skip two index */
    char_p = char_p + 2;
  }
  DHCP_HOST_TO_IP(cli_ip.addr, ip_2_ip4(&dhcp->server_ip_addr)->addr, dhcp->offered_sn_mask.addr,
                  dhcp_state->offered_ip_addr);
  state = dhcp_state->state;
#ifdef CUSTOM_AT_COMMAND
  (void)hi_at_printf("\t%-8"DHCP_NUM_F"%-16s%-16s%-8hhu%-8hu%-8hhu%-8hu"CRLF, dhcp_state->idx, macaddr,
                     (dhcp_state->offered_ip_addr == 0) ? "0.0.0.0" : ip4addr_ntoa(&cli_ip),
                     state, dhcp_state->lease_used, dhcp_state->tries, dhcp_state->request_timeout);
#else
  PRINTK("\t%-8"DHCP_NUM_F"%-16s%-16s%-8hhu%-8hu%-8hhu%-8hu"CRLF, dhcp_state->idx, macaddr,
         (dhcp_state->offered_ip_addr == 0) ? "0.0.0.0" : ip4addr_ntoa(&cli_ip),
         state, dhcp_state->lease_used, dhcp_state->tries, dhcp_state->request_timeout);
#endif

  return;
}

void
dhcp_clients_info_show(struct netif *netif_p)
{
  struct dhcp *netif_dhcp = NULL;
  struct dhcp_client *dhcp = NULL;
  struct dhcp_state *dhcp_state = NULL;
  int i;
  u8_t hwaddr_len;
  if (netif_p == NULL) {
    return;
  }
  netif_dhcp = netif_dhcp_data(netif_p);
  if (netif_dhcp == NULL) {
#ifdef CUSTOM_AT_COMMAND
    (void)hi_at_printf("dhcpc not start on %s%hhu"CRLF, netif_p->name, netif_p->num);
#else
    PRINT_ERRCODE(API_SHELL_ERRCODE_DEV_NOT_READY);
#endif
    return;
  }
  dhcp = &(netif_dhcp->client);
#ifdef CUSTOM_AT_COMMAND
  (void)hi_at_printf("server :"CRLF);
  (void)hi_at_printf("\tserver_id : %s"CRLF, ip4addr_ntoa(ip_2_ip4(&(dhcp->server_ip_addr))));
  (void)hi_at_printf("\tmask : %s, %hhu"CRLF, ip4addr_ntoa(&(dhcp->offered_sn_mask)), dhcp->subnet_mask_given);
  (void)hi_at_printf("\tgw : %s"CRLF, ip4addr_ntoa(&(dhcp->offered_gw_addr)));
  (void)hi_at_printf("\tT0 : %u"CRLF, dhcp->offered_t0_lease);
  (void)hi_at_printf("\tT1 : %u"CRLF, dhcp->offered_t1_renew);
  (void)hi_at_printf("\tT2 : %u"CRLF, dhcp->offered_t2_rebind);
#else
  PRINTK("server :"CRLF);
  PRINTK("\tserver_id : %s"CRLF, ip4addr_ntoa(ip_2_ip4(&(dhcp->server_ip_addr))));
  PRINTK("\tmask : %s, %hhu"CRLF, ip4addr_ntoa(&(dhcp->offered_sn_mask)), dhcp->subnet_mask_given);
  PRINTK("\tgw : %s"CRLF, ip4addr_ntoa(&(dhcp->offered_gw_addr)));
  PRINTK("\tT0 : %u"CRLF, dhcp->offered_t0_lease);
  PRINTK("\tT1 : %u"CRLF, dhcp->offered_t1_renew);
  PRINTK("\tT2 : %u"CRLF, dhcp->offered_t2_rebind);
#endif
  if (ip4_addr_isany_val(dhcp->server_ip_addr.u_addr.ip4)) {
    return;
  }

#ifdef CUSTOM_AT_COMMAND
  (void)hi_at_printf("clients <%"DHCP_NUM_F"> :"CRLF, dhcp->cli_cnt);
  (void)hi_at_printf("\t%-8s%-16s%-16s%-8s%-8s%-8s%-8s"CRLF,
      "mac_idx", "mac", "addr", "state", "lease", "tries", "rto");
#else
  PRINTK("clients <%"DHCP_NUM_F"> :"CRLF, dhcp->cli_cnt);
  PRINTK("\t%-8s%-16s%-16s%-8s%-8s%-8s%-8s"CRLF, "mac_idx", "mac", "addr", "state", "lease", "tries", "rto");
#endif
  for (i = 0; i < DHCP_CLIENT_NUM; i++) {
    dhcp->cli_idx = (dhcp_num_t)i;
    dhcp_state = &((dhcp->states)[i]);
    if ((i != LWIP_DHCP_NATIVE_IDX) && (dhcp_state->idx == 0)) {
      continue;
    }
    if (dhcp_idx_to_mac(netif_p, dhcp_state->idx, dhcp->hwaddr, &hwaddr_len) != ERR_OK) {
#ifdef CUSTOM_AT_COMMAND
      (void)hi_at_printf("<%d> idx %"DHCP_NUM_F" to mac failed"CRLF, i, dhcp_state->idx);
#else
      PRINTK("<%d> idx %"DHCP_NUM_F" to mac failed"CRLF, i, dhcp_state->idx);
#endif
      continue;
    }
    dhcp_state->hwaddr_len = hwaddr_len;
    dhcp_client_info_show(dhcp);
  }

  return;
}

u32_t
os_shell_dhcp(int argc, const char **argv)
{
  int ret = 0;
  struct netif *netif_p = NULL;

  if (tcpip_init_finish == 0) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_TCPIP_UNINTED);
    return LOS_NOK;
  }

  if ((argc != 2) || (argv == NULL)) { /* 2 : total argc num */
    goto usage;
  }

  netif_p = netif_find(argv[0]);
  if (netif_p == NULL) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_DEV_NOT_FOUND);
    goto usage;
  }
#ifdef CUSTOM_AT_COMMAND
    if (strcmp(argv[1], "1") == 0) {
        ret = netifapi_dhcp_start(netif_p);
    } else if (strcmp(argv[1], "0") == 0) {
        ret = netifapi_dhcp_stop(netif_p);
    } else {
        goto usage;
    }
#else
  if (strcmp(argv[1], "start") == 0) {
    ret = netifapi_dhcp_start(netif_p);
  } else if (strcmp(argv[1], "stop") == 0) {
    ret = netifapi_dhcp_stop(netif_p);
  } else if (strcmp(argv[1], "inform") == 0) {
    ret = netifapi_dhcp_inform(netif_p);
  } else if (strcmp(argv[1], "renew") == 0) {
    ret = netifapi_dhcp_renew(netif_p);
  } else if (strcmp(argv[1], "release") == 0) {
    ret = netifapi_dhcp_release(netif_p);
  } else if (strcmp(argv[1], "show") == 0) {
    ret = netifapi_netif_common(netif_p, dhcp_clients_info_show, NULL);
  } else {
    goto usage;
  }
#endif
  if (ret == LOS_OK) {
    PRINTK("OK"CRLF);
  }

  return (ret == 0) ? LOS_OK : LOS_NOK;
usage:
#ifndef CUSTOM_AT_COMMAND
  PRINTK("dhcp\n\tifname {start | stop | inform | renew | release | show}"CRLF);
#endif
  return LOS_NOK;
}
#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(dhcp_shellcmd, CMD_TYPE_EX, "dhcp", XARGS, (CmdCallBackFunc)os_shell_dhcp);
#endif /* LOSCFG_SHELL */

#if LWIP_DHCPS
void
dhcps_info_show(struct netif *netif)
{
  struct dhcps *dhcps = NULL;
  struct dyn_lease_addr *lease = NULL;
  int i, j, k = 0;
  ip4_addr_t addr;

  /* netif is ensured not to be NULL by the caller */
  LWIP_ASSERT("netif is NULL!", netif != NULL);

  dhcps = netif->dhcps;

  if ((dhcps == NULL) || (dhcps->pcb == NULL)) {
#ifdef CUSTOM_AT_COMMAND
    (void)hi_at_printf("dhcps of %s%hhu not start\r\n", netif->name, netif->num);
#else
    PRINT_ERRCODE(API_SHELL_ERRCODE_DEV_NOT_READY);
#endif
    return;
  }
  addr.addr = lwip_htonl(dhcps->start_addr.addr);
#ifdef CUSTOM_AT_COMMAND
  (void)hi_at_printf("start ip : %s"CRLF, ip4addr_ntoa(&addr));
#else
  PRINTK("start ip : %s"CRLF, ip4addr_ntoa(&addr));
#endif
  addr.addr = lwip_htonl(dhcps->end_addr.addr);
#ifdef CUSTOM_AT_COMMAND
    (void)hi_at_printf("end ip : %s"CRLF, ip4addr_ntoa(&addr));
    (void)hi_at_printf("lease num %hhu"CRLF, dhcps->lease_num);
    (void)hi_at_printf("sys_now %u"CRLF, sys_now());
#else
    PRINTK("end ip : %s"CRLF, ip4addr_ntoa(&addr));
    PRINTK("lease num %hhu"CRLF, dhcps->lease_num);
    PRINTK("sys_now %u"CRLF, sys_now());
#endif

  for (i = 0; i < dhcps->lease_num; i++) {
    if (dhcps->leasearr[i].flags != 0) {
      lease = &(dhcps->leasearr[i]);
#ifdef CUSTOM_AT_COMMAND
      (void)hi_at_printf("%d : ", k++);
#else
      PRINTK("%d : ", k++);
#endif
      for (j = 0; j < NETIF_MAX_HWADDR_LEN; j++) {
#ifdef CUSTOM_AT_COMMAND
        (void)hi_at_printf("%02x", lease->cli_hwaddr[j]);
#else
        PRINTK("%02x", lease->cli_hwaddr[j]);
#endif
      }
      addr.addr = lwip_htonl(lease->cli_addr.addr);
#ifdef CUSTOM_AT_COMMAND
      (void)hi_at_printf(" %s leasetime = %u(%u), proposed_leasetime = %u"CRLF, ip4addr_ntoa(&addr), lease->leasetime,
                         (lease->leasetime - sys_now()) / US_PER_MSECOND, lease->proposed_leasetime);
#else
      PRINTK(" %s leasetime = %u(%u), proposed_leasetime = %u"CRLF, ip4addr_ntoa(&addr), lease->leasetime,
             (lease->leasetime - sys_now()) / US_PER_MSECOND, lease->proposed_leasetime);
#endif
    }
  }
}

u32_t
os_shell_dhcps(int argc, const char **argv)
{
  int ret = 0;
  struct netif *netif_p = NULL;

  if (tcpip_init_finish == 0) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_TCPIP_UNINTED);
    return LOS_NOK;
  }

  if ((argc != 2) || (argv == NULL)) { /* 2 : totol argc num */
    goto usage;
  }

  netif_p = netif_find(argv[0]);
  if (netif_p == NULL) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_DEV_NOT_FOUND);
    goto usage;
  }
#ifdef CUSTOM_AT_COMMAND
    if (strcmp(argv[1], "1") == 0) {
        ret = netifapi_dhcps_start(netif_p, NULL, 0);
    } else if (strcmp(argv[1], "0") == 0) {
        ret = netifapi_dhcps_stop(netif_p);
    } else {
        goto usage;
    }
#else
  if (strcmp(argv[1], "start") == 0) {
    ret = netifapi_dhcps_start(netif_p, NULL, 0);
  } else if (strcmp(argv[1], "stop") == 0) {
    ret = netifapi_dhcps_stop(netif_p);
  } else if (strcmp(argv[1], "show") == 0) {
    ret = netifapi_netif_common(netif_p, dhcps_info_show, NULL);
  } else {
    goto usage;
  }
#endif
  if (ret == LOS_OK) {
    PRINTK("OK"CRLF);
  }

  return (ret == 0) ? LOS_OK : LOS_NOK;
usage:
#ifndef CUSTOM_AT_COMMAND
  PRINTK("dhcps\n\tifname {start | stop | show}"CRLF);
#endif
  return LOS_NOK;
}
#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(dhcps_shellcmd, CMD_TYPE_EX, "dhcps", XARGS, (CmdCallBackFunc)os_shell_dhcps);
#endif /* LOSCFG_SHELL */
#endif /* LWIP_DHCPS */
#endif /* LWIP_DHCP */

#if LWIP_IPV6
extern struct nd6_neighbor_cache_entry neighbor_cache[LWIP_ND6_NUM_NEIGHBORS];
#endif
#if LWIP_IPV6
int
netstat_get_udp_sendq_len6(struct udp_pcb *udppcb, struct pbuf *udpbuf)
{
  int send_len = -1;
  u16_t offset = 0, len;
  struct ip6_hdr *iphdr = NULL;
  struct udp_hdr *udphdr = NULL;
  struct ip6_dest_hdr *dest_hdr = NULL;
  struct ip6_frag_hdr *frag_hdr = NULL;
  u8_t nexth;
  u16_t hlen = 0;

  LWIP_ERROR("netstat_get_udp6_sendQLen: NULL pcb received"CRLF, (udppcb != NULL), return -1);
  LWIP_ERROR("netstat_get_udp6_sendQLen: NULL pbuf received"CRLF, (udpbuf != NULL), return -1);

  iphdr = (struct ip6_hdr *)udpbuf->payload;
  LWIP_ERROR("netstat_get_udp6_sendQLen: NULL iphdr received"CRLF, (iphdr != NULL), return -1);
  if (!(ip6_addr_cmp_zoneless(&iphdr->dest, ip_2_ip6(&udppcb->remote_ip)) &&
        (ip_addr_isany(&udppcb->local_ip) ||
         ip_get_option(udppcb, SOF_BINDNONUNICAST) ||
         ip6_addr_cmp_zoneless(&iphdr->src, ip_2_ip6(&udppcb->local_ip))))) {
    goto FUNC_OUT;
  }

  len = IP6_HLEN;
  if (pbuf_header(udpbuf, (s16_t)(-(s16_t)(len)))) {
    goto FUNC_OUT;
  }

  offset = len;

  nexth = IP6H_NEXTH(iphdr);
  while (offset < udpbuf->tot_len) {
    if ((nexth == IP6_NEXTH_NONE) || (nexth == IP6_NEXTH_UDP) || (nexth == IP6_NEXTH_UDPLITE)) {
      break;
    }
    switch (nexth) {
      case IP6_NEXTH_HOPBYHOP:
      case IP6_NEXTH_ROUTING:
        nexth = *((u8_t *)udpbuf->payload);
        /* 8 : hlen will Multiply by 8 in ipv6 option segment */
        hlen = (u16_t)(8 * (1 + *((u8_t *)udpbuf->payload + 1)));
        break;
      case IP6_NEXTH_DESTOPTS:
        nexth = *((u8_t *)udpbuf->payload);
        dest_hdr = (struct ip6_dest_hdr *)udpbuf->payload;
        /* 8 : hlen will Multiply by 8 in ipv6 option segment */
        hlen = (u16_t)(8 * (1 + dest_hdr->_hlen));
        break;
      case IP6_NEXTH_FRAGMENT:
        frag_hdr = (struct ip6_frag_hdr *)udpbuf->payload;
        nexth = frag_hdr->_nexth;
        hlen = IP6_FRAG_HLEN;
        break;
      default:
        /* Unknown next_header */
        goto FUNC_OUT;
    }

    (void)pbuf_header(udpbuf, (s16_t)(-(s16_t)hlen));
    offset = (u16_t)(offset + hlen);
  }

  /* If the while loop test condition failed , then revert the last offset change */
  if (offset >= udpbuf->tot_len) {
    offset = (u16_t)(offset - hlen);
    goto FUNC_OUT;
  }

  LWIP_ERROR("Transport option should be UDP", (nexth == IP6_NEXTH_UDP || nexth == IP6_NEXTH_UDPLITE), goto FUNC_OUT);

  if (offset > iphdr->_plen) {
    goto FUNC_OUT;
  }

  /* check if there is enough space for atleast udp header available */
  if (udpbuf->tot_len < UDP_HLEN) {
    goto FUNC_OUT;
  }

  udphdr = (struct udp_hdr *)udpbuf->payload;
  if ((ntohs(udphdr->dest) == udppcb->remote_port) && (ntohs(udphdr->src) == udppcb->local_port)) {
    if (ntohs(udphdr->len) > UDP_HLEN) {
      send_len = ntohs(udphdr->len) - UDP_HLEN;
    } else {
      send_len = udpbuf->tot_len - UDP_HLEN;
    }
  }

FUNC_OUT:
  (void)pbuf_header(udpbuf, (s16_t)offset); // canot not cross max limit of s16_t
  return send_len;
}
#endif

#if LWIP_IPV4
int
netstat_get_udp_sendq_len(struct udp_pcb *udppcb, struct pbuf *udpbuf)
{
  int send_len = -1;
  u16_t offset = 0, len;
  struct ip_hdr *iphdr = NULL;
  struct udp_hdr *udphdr = NULL;

  LWIP_ERROR("netstat_get_udp_sendQLen: NULL pcb received"CRLF, (udppcb != NULL), return -1);
  LWIP_ERROR("netstat_get_udp_sendQLen: NULL pbuf received"CRLF, (udpbuf != NULL), return -1);

  iphdr = (struct ip_hdr *)udpbuf->payload;

  if (!(ip4_addr_cmp(&iphdr->dest, ip_2_ip4(&udppcb->remote_ip))
        && (ip_addr_isany(&udppcb->local_ip) ||
            ip_get_option(udppcb, SOF_BINDNONUNICAST) ||
            ip4_addr_cmp(&iphdr->src, ip_2_ip4(&udppcb->local_ip))))) {
    goto FUNC_OUT;
  }
#if LWIP_UDPLITE
  if ((IPH_PROTO(iphdr) != IP_PROTO_UDP) && (IPH_PROTO(iphdr) != IP_PROTO_UDPLITE))
#else
  if (IPH_PROTO(iphdr) != IP_PROTO_UDP)
#endif
  {
    goto FUNC_OUT;
  }

  if ((ntohs(IPH_OFFSET(iphdr)) & IP_OFFMASK) != 0) {
    goto FUNC_OUT;
  }

  len = (u16_t)(IPH_HL(iphdr) * 4); /* 4 ：IP Header Length Multiply by 4 */
  if (pbuf_header(udpbuf, (s16_t)(-len))) {
    goto FUNC_OUT;
  }

  offset = (u16_t)(offset + len);

  udphdr = (struct udp_hdr *)udpbuf->payload;
  if ((ntohs(udphdr->dest) == udppcb->remote_port) && (ntohs(udphdr->src) == udppcb->local_port)) {
    send_len = ntohs(udphdr->len) - UDP_HLEN;
  }

FUNC_OUT:
  (void)pbuf_header(udpbuf, (s16_t)offset);
  return send_len;
}
#endif

int
netstat_tcp_recvq(struct tcp_pcb *tpcb)
{
  unsigned int ret_val = 0;
#if LWIP_SO_RCVBUF
  struct netconn *conn = NULL;
#endif

  LWIP_ERROR("netstat_tcp_recvq: Received NULL pcb"CRLF, (tpcb != NULL), return 0);

#if LWIP_SO_RCVBUF
  conn = (struct netconn *)tpcb->callback_arg;
  if (conn != NULL) {
    switch (conn->type) {
      case NETCONN_TCP:
      case NETCONN_RAW:
#if LWIP_IPV6
      case NETCONN_RAW_IPV6:
      case NETCONN_UDP_IPV6:
#endif
      case NETCONN_UDP:
        SYS_ARCH_GET(((unsigned int)conn->recv_avail + conn->lrcv_left), ret_val);
        break;
      default:
        ret_val = 0; /* ur... very ugly, damn DHCP DNS and SNTP */
    }
  }
#endif

  return (int)ret_val;
}

int
netstat_tcp_sendq(struct tcp_pcb *tpcb)
{
  int ret_val = 0;
  struct tcp_seg *useg = NULL;

  LWIP_ERROR("netstat_tcp_sendq: Received NULL pcb"CRLF, (tpcb != NULL), return 0);

  for (useg = tpcb->unacked; useg != NULL; useg = useg->next) {
    ret_val = ret_val + useg->len;
  }

  return ret_val;
}

#if LWIP_IPV6
int
netstat_udp_sendq6(struct udp_pcb *upcb)
{
  int ret_len = 0, ret;
  int idx = 0;
  int i;
#if LWIP_ND6_QUEUEING
  struct nd6_q_entry *neibq = NULL;
#endif

  LWIP_ERROR("netstat_udp_sendq6: Received NULL pcb"CRLF, (upcb != NULL), return 0);

  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    if (neighbor_cache[i].state != ND6_NO_ENTRY) {
      if (ip6_addr_cmp(&upcb->remote_ip.u_addr.ip6, &neighbor_cache[i].next_hop_address)) {
        idx = i;
        break;
      }
    }
  }
#if LWIP_ND6_QUEUEING
  for (neibq = neighbor_cache[idx].q; neibq != NULL; neibq = neibq->next) {
    if (neibq->p == NULL) {
      return 0;
    }
    ret = netstat_get_udp_sendq_len6(upcb, neibq->p);
    if (ret >= 0) {
      ret_len += ret;
    }
  }
#else
  ret = netstat_get_udp_sendq_len6(upcb, neighbor_cache[idx].q);
  if (ret >= 0) {
    ret_len += ret;
  }
#endif
  return ret_len;
}
#endif

#if LWIP_IPV4
int
netstat_udp_sendq(struct udp_pcb *upcb)
{
  int ret_len = 0, ret;
  int arpidx = -1;
  int i;
#if ARP_QUEUEING
  struct etharp_q_entry *arpq = NULL;
#endif

  LWIP_ERROR("netstat_udp_sendq: Received NULL pcb"CRLF, (upcb != NULL), return 0);

  for (i = 0; i < ARP_TABLE_SIZE; ++i) {
    if (arp_table[i].state != ETHARP_STATE_EMPTY) {
      if (ip4_addr_cmp(ip_2_ip4(&upcb->remote_ip), &arp_table[i].ipaddr)) {
        arpidx = i;
        break;
      }
    }
  }

  if (arpidx >= 0) {
#if ARP_QUEUEING
    for (arpq = arp_table[arpidx].q; arpq != NULL; arpq = arpq->next) {
      ret = netstat_get_udp_sendq_len(upcb, arpq->p);
      if (ret <= 0) {
        continue;
      }
      ret_len += ret;
      if (ret_len <= 0) { // overflow, set rteLen = -1 to indicate
        ret_len = -1;
        break;
      }
    }
#else
    ret = netstat_get_udp_sendq_len(upcb, arp_table[arpidx].q);
    if (ret > 0) {
      ret_len += ret;
      if (ret_len <= 0) { // overflow, set rteLen = -1 to indicate
        ret_len = -1;
      }
    }
#endif
  }
  return ret_len;
}
#endif
int
netstat_netconn_recvq(const struct netconn *conn)
{
  unsigned int ret_val = 0;

#if LWIP_SO_RCVBUF
  if (conn == NULL) {
    return 0;
  }

  switch (NETCONNTYPE_GROUP(conn->type)) {
    case NETCONN_TCP:
    case NETCONN_RAW:
#if PF_PKT_SUPPORT
    case NETCONN_PKT_RAW:
#endif
    case NETCONN_UDP:
      SYS_ARCH_GET(((unsigned int)conn->recv_avail + conn->lrcv_left), ret_val);
      break;
    default:
      ret_val = 0; /* ur... very ugly, damn DHCP DNS and SNTP */
  }
#else
  (void)conn;
#endif
  return (int)ret_val;
}
int
netstat_netconn_sendq(struct netconn *conn)
{
  int ret_val;

  if (conn == NULL) {
    return 0;
  }

  switch (NETCONNTYPE_GROUP(conn->type)) {
    case NETCONN_TCP:
      ret_val = netstat_tcp_sendq(conn->pcb.tcp);
      break;
    case NETCONN_RAW:
      ret_val = 0;
      break;
#if PF_PKT_SUPPORT
    case NETCONN_PKT_RAW:
      ret_val = 0; /* always be 0 as frame send to driver directly */
      break;
#endif
    case NETCONN_UDP:
      ret_val = netstat_udp_sendq(conn->pcb.udp);
      break;
    default:
      ret_val = 0; /* ur... very ugly, damn DHCP DNS and SNTP */
  }

  return ret_val;
}

#ifndef CUSTOM_AT_COMMAND
#if PF_PKT_SUPPORT
static s32_t
print_netstat_pkt_raw(struct netstat_data *ndata, u32_t *entry_buf_offset)
{
  u8_t netif_name[IFNAMSIZ];
  struct netif *netif = NULL;
  s8_t *entry_buf = ndata->netstat_out_buf;
  u32_t entry_buf_len = ndata->netstat_out_buf_len;
  int recv_qlen, send_qlen, i_ret;
  u_int proto;
  struct raw_pcb *rpcb = NULL;
  if (entry_buf == NULL) {
    return ERR_VAL;
  }
  if (pkt_raw_pcbs != NULL) {
    i_ret = snprintf_s((char *)(entry_buf + (*entry_buf_offset)), entry_buf_len, entry_buf_len - 1,
                       "\n%-12s%-12s%-12s%-16s%-12s"CRLF, "Type", "Recv-Q", "Send-Q", "Protocol", "netif");
    if ((i_ret <= 0) || ((u32_t)(i_ret) >= entry_buf_len)) {
      return ERR_VAL;
    }
    entry_buf_len -= (u32_t)(i_ret);
    (*entry_buf_offset) += (u32_t)(i_ret);

    for (rpcb = pkt_raw_pcbs; rpcb != NULL; rpcb = rpcb->next) {
      recv_qlen = netstat_netconn_recvq(rpcb->recv_arg);
      send_qlen = netstat_netconn_sendq(rpcb->recv_arg);
      for (netif = netif_list; netif != NULL; netif = netif->next) {
        if (netif->ifindex != rpcb->netifindex) {
          continue;
        }
        i_ret = snprintf_s((char *)netif_name, IFNAMSIZ, IFNAMSIZ - 1, "%s%d", netif->name, netif->num);
        if ((i_ret <= 0) || ((u32_t)(i_ret) >= IFNAMSIZ)) {
          return ERR_VAL;
        }
        break;
      }

      if (netif == NULL) {
        (void)snprintf_s((char *)netif_name, IFNAMSIZ, IFNAMSIZ - 1, "%s", "None");
      }

      proto = ntohs(rpcb->proto.eth_proto);

      i_ret = snprintf_s((char *)(entry_buf + (*entry_buf_offset)), entry_buf_len, entry_buf_len - 1,
                         "%-12s%-12d%-12d%-16x%-12s"CRLF, "pkt-raw", recv_qlen, send_qlen, proto, netif_name);
      if ((i_ret <= 0) || ((u32_t)(i_ret) >= entry_buf_len)) {
        return ERR_VAL;
      }
      entry_buf_len -= (u32_t)(i_ret);
      (*entry_buf_offset) += (u32_t)(i_ret);
    }
  }
  ndata->netstat_out_buf_len = entry_buf_len;
  return ERR_OK;
}
#endif

#if LWIP_RAW
static s32_t
print_netstat_raw(struct netstat_data *ndata, u32_t *entry_buf_offset)
{
  struct raw_pcb *rpcb = NULL;
  s8_t local_ip_port[64] = {0};
  s8_t remote_ip_port[64] = {0};
  s8_t *entry_buf = ndata->netstat_out_buf;
  u32_t entry_buf_len = ndata->netstat_out_buf_len;
  int recv_qlen, send_qlen, i_ret;
  if (entry_buf == NULL) {
    return ERR_VAL;
  }
  if (raw_pcbs != NULL) {
    i_ret = snprintf_s((char *)(entry_buf + (*entry_buf_offset)), entry_buf_len, entry_buf_len - 1,
                       "\n%-8s%-12s%-12s%-20s%-20s%-16s%-16s"CRLF,
                       "Type", "Recv-Q", "Send-Q", "Local Address", "Foreign Address", "Protocol", "HDRINCL");
    if ((i_ret <= 0) || ((u32_t)(i_ret) >= entry_buf_len)) {
      return ERR_VAL;
    }
    entry_buf_len -= (u32_t)(i_ret);
    (*entry_buf_offset) += (u32_t)(i_ret);

    for (rpcb = raw_pcbs; rpcb != NULL; rpcb = rpcb->next) {
      i_ret = snprintf_s((char *)local_ip_port, sizeof(local_ip_port), (sizeof(local_ip_port) - 1), "%s",
                         ipaddr_ntoa(&rpcb->local_ip));
      if ((i_ret <= 0) || ((u32_t)(i_ret) >= sizeof(local_ip_port))) {
        return ERR_VAL;
      }

      i_ret = snprintf_s((char *)remote_ip_port, sizeof(remote_ip_port), (sizeof(remote_ip_port) - 1), "%s",
                         ipaddr_ntoa(&rpcb->remote_ip));
      if ((i_ret <= 0) || ((u32_t)(i_ret) >= sizeof(remote_ip_port))) {
        return ERR_VAL;
      }

      recv_qlen = netstat_netconn_recvq(rpcb->recv_arg);
      send_qlen = netstat_netconn_sendq(rpcb->recv_arg);
      i_ret = snprintf_s((char *)(entry_buf + (*entry_buf_offset)), entry_buf_len, entry_buf_len - 1,
                         "%-8s%-12d%-12d%-20s%-20s%-16u%-16d"CRLF, "raw",
                         recv_qlen, send_qlen, local_ip_port, remote_ip_port,
                         rpcb->raw_proto, rpcb->hdrincl);
      if ((i_ret <= 0) || ((u32_t)(i_ret) >= entry_buf_len)) {
        return ERR_VAL;
      }
      entry_buf_len -= (u32_t)(i_ret);
      (*entry_buf_offset) += (u32_t)(i_ret);
    }
  }
  ndata->netstat_out_buf_len = entry_buf_len;
#if PF_PKT_SUPPORT
  s32_t pkt_ret = print_netstat_pkt_raw(ndata, entry_buf_offset);
  if (pkt_ret == ERR_VAL) {
    return ERR_VAL;
  }
#endif
  return ERR_OK;
}
#endif
#endif

#if LWIP_UDP
static s32_t
print_netstat_udp(struct netstat_data *ndata, u32_t *entry_buf_offset)
{
  s8_t local_ip_port[64] = {0};
  s8_t remote_ip_port[64] = {0};
  struct udp_pcb *upcb = NULL;
  s8_t *entry_buf = ndata->netstat_out_buf;
  u32_t entry_buf_len = ndata->netstat_out_buf_len;
  int recv_qlen, send_qlen, i_ret;
  if (entry_buf == NULL) {
    return ERR_VAL;
  }
  if (udp_pcbs != NULL) {
#ifndef CUSTOM_AT_COMMAND
    i_ret = snprintf_s((char *)(entry_buf + (*entry_buf_offset)), entry_buf_len, entry_buf_len - 1,
                       "\n%-8s%-12s%-12s%-24s%-24s"CRLF, "Proto", "Recv-Q", "Send-Q",
                       "Local Address", "Foreign Address");
    if ((i_ret <= 0) || ((u32_t)(i_ret) >= entry_buf_len)) {
      return ERR_VAL;
    }
    entry_buf_len -= (u32_t)(i_ret);
    (*entry_buf_offset) += (u32_t)(i_ret);
#endif

    for (upcb = udp_pcbs; upcb != NULL; upcb = upcb->next) {
      i_ret = snprintf_s((char *)local_ip_port, sizeof(local_ip_port), (sizeof(local_ip_port) - 1), "%s:%d",
                         ipaddr_ntoa(&upcb->local_ip), upcb->local_port);
      if ((i_ret <= 0) || ((u32_t)(i_ret) >= sizeof(local_ip_port))) {
        return ERR_VAL;
      }

      i_ret = snprintf_s((char *)remote_ip_port, sizeof(remote_ip_port), (sizeof(remote_ip_port) - 1), "%s:%d",
                         ipaddr_ntoa(&upcb->remote_ip), upcb->remote_port);
      if ((i_ret <= 0) || ((u32_t)(i_ret) >= sizeof(remote_ip_port))) {
        return ERR_VAL;
      }

      recv_qlen = (upcb->recv == recv_udp) ? netstat_netconn_recvq(upcb->recv_arg) : 0;
#if LWIP_IPV6
      send_qlen = IP_IS_V6(&upcb->local_ip) ? netstat_udp_sendq6(upcb) : ((upcb->recv == recv_udp) ? \
                                                                          netstat_netconn_sendq(upcb->recv_arg) : 0);
#else
      send_qlen = (upcb->recv == recv_udp) ? netstat_netconn_sendq(upcb->recv_arg) : 0;
#endif
      if (entry_buf_len <= 1) {
        return ERR_VAL;
      }
#ifdef CUSTOM_AT_COMMAND
      /* Proto 2:udp-ip6; 3:udp */
      i_ret = snprintf_s((char *)(entry_buf + (*entry_buf_offset)), entry_buf_len, entry_buf_len - 1,
                         "+NETSTAT:%d,%d,%d,%s,%s,%d"CRLF,
                         (IP_IS_V6(&upcb->local_ip) ? UDP_IP6 : UDP), recv_qlen, send_qlen, local_ip_port,
                         remote_ip_port, 0);
#else
      i_ret = snprintf_s((char *)(entry_buf + (*entry_buf_offset)), entry_buf_len, entry_buf_len - 1,
                         (IP_IS_V6(&upcb->local_ip) ? "%-8s%-12d%-12d%-39s%-39s%-16s\n" :
                          "%-8s%-12d%-12d%-24s%-24s%-16s"CRLF),
                         IP_IS_V6(&upcb->local_ip) ? "udp-ip6" : "udp",
                         recv_qlen, send_qlen, local_ip_port, remote_ip_port, " ");
#endif
      if ((i_ret <= 0) || ((u32_t)(i_ret) >= entry_buf_len)) {
        return ERR_VAL;
      }
      entry_buf_len -= (u32_t)(i_ret);
      (*entry_buf_offset) += (u32_t)(i_ret);
    }
  }
  ndata->netstat_out_buf_len = entry_buf_len;
  return ERR_OK;
}
#endif

#if LWIP_TCP
static s32_t
print_netstat_tcp(struct netstat_data *ndata, u32_t *entry_buf_offset)
{
  s8_t local_ip_port[64] = {0};
  s8_t remote_ip_port[64] = {0};
  struct tcp_pcb *tpcb = NULL;
  u16_t remote_port;
  s8_t *entry_buf = ndata->netstat_out_buf;
  u32_t entry_buf_len = ndata->netstat_out_buf_len;
  int recv_qlen, send_qlen, i_ret;
  if (entry_buf == NULL) {
    return ERR_VAL;
  }
  if ((tcp_active_pcbs == NULL) && (tcp_bound_pcbs == NULL) &&
      (tcp_tw_pcbs == NULL) && (tcp_listen_pcbs.pcbs == NULL)) {
    return ERR_OK;
  }
#ifndef CUSTOM_AT_COMMAND
  i_ret = snprintf_s((char *)(entry_buf + (*entry_buf_offset)), entry_buf_len, entry_buf_len - 1,
                     "%-8s%-12s%-12s%-24s%-24s%-16s"CRLF,
                     "Proto", "Recv-Q", "Send-Q", "Local Address", "Foreign Address", "State");
  if ((i_ret <= 0) || ((u32_t)(i_ret) >= entry_buf_len)) {
    return ERR_VAL;
  }
  entry_buf_len -= (u32_t)(i_ret);
  (*entry_buf_offset) += (u32_t)(i_ret);
#endif

  int i;
  for (i = 0; i < NUM_TCP_PCB_LISTS; i++) {
    struct tcp_pcb *pcblist = *tcp_pcb_lists[i];
    for (tpcb = pcblist; tpcb != NULL; tpcb = tpcb->next) {
      /* DON'T access a tcp_pcb's remote_port if it's casted from a tcp_pcb_listen */
      remote_port = (pcblist == tcp_listen_pcbs.pcbs) ? 0 : tpcb->remote_port;
      i_ret = snprintf_s((char *)local_ip_port, sizeof(local_ip_port), (sizeof(local_ip_port) - 1), "%s:%d",
                         ipaddr_ntoa(&tpcb->local_ip), tpcb->local_port);
      if ((i_ret <= 0) || ((u32_t)(i_ret) >= sizeof(local_ip_port))) {
        return ERR_VAL;
      }
      i_ret = snprintf_s((char *)remote_ip_port, sizeof(remote_ip_port), (sizeof(remote_ip_port) - 1), "%s:%d",
                         ipaddr_ntoa(&tpcb->remote_ip), remote_port);
      if ((i_ret <= 0) || ((u32_t)(i_ret) >= sizeof(remote_ip_port))) {
        return ERR_VAL;
      }

      if (pcblist == tcp_bound_pcbs) {
        send_qlen = 0;
        recv_qlen = 0;
      } else if (pcblist == tcp_listen_pcbs.pcbs) {
        recv_qlen = netstat_netconn_recvq(tpcb->callback_arg);
        send_qlen = 0;
      } else {
        recv_qlen = (tpcb->state == SYN_RCVD) ? 0 : netstat_netconn_recvq(tpcb->callback_arg);
        send_qlen = (tpcb->state == SYN_RCVD) ? 0 : netstat_netconn_sendq(tpcb->callback_arg);
      }

#ifdef CUSTOM_AT_COMMAND
      /* Proto 0:tcp-ip6; 1:tcp .State for tcp_state_str */
      i_ret = snprintf_s((char *)(entry_buf + (*entry_buf_offset)), entry_buf_len, entry_buf_len - 1,
                         "+NETSTAT:%d,%d,%d,%s,%s,%d"CRLF,
                         (IP_IS_V6(&tpcb->local_ip) ? TCP_IP6 : TCP), recv_qlen, send_qlen, local_ip_port,
                         remote_ip_port, tpcb->state);
#else
      i_ret = snprintf_s((char *)(entry_buf + (*entry_buf_offset)), entry_buf_len, entry_buf_len - 1,
                         (IP_IS_V6(&tpcb->local_ip) ? "%-8s%-12d%-12d%-39s%-39s%-16s"CRLF :
                          "%-8s%-12d%-12d%-24s%-24s%-16s"CRLF),
                         IP_IS_V6(&tpcb->local_ip) ? "tcp-ip6" : "tcp",
                         recv_qlen, send_qlen, local_ip_port, remote_ip_port, tcp_state_str[tpcb->state]);
#endif
      if ((i_ret <= 0) || ((u32_t)(i_ret) >= entry_buf_len)) {
        return ERR_VAL;
      }
      entry_buf_len -= (u32_t)(i_ret);
      (*entry_buf_offset) += (u32_t)(i_ret);
    }
  }

  ndata->netstat_out_buf_len = entry_buf_len;
  return ERR_OK;
}
#endif

void
netstat_internal(void *ctx)
{
  s8_t *entry_buf = NULL;
  u32_t entry_buf_offset = 0;
  struct netstat_data *ndata = (struct netstat_data *)ctx;
  entry_buf = ndata->netstat_out_buf;

  if (entry_buf == NULL) {
    goto out;
  }

#ifndef CUSTOM_AT_COMMAND
  int i_ret;
  i_ret = snprintf_s((char *)(entry_buf), ndata->netstat_out_buf_len, ndata->netstat_out_buf_len - 1,
                     "========== total sockets %d ======  unused sockets %d =========="CRLF, LWIP_CONFIG_NUM_SOCKETS,
                     get_unused_socket_num());
  if ((i_ret <= 0) || ((u32_t)(i_ret) >= ndata->netstat_out_buf_len)) {
    goto out;
  }
  ndata->netstat_out_buf_len -= (u32_t)(i_ret);
  entry_buf_offset = (u32_t)(i_ret);
#endif
#if LWIP_TCP
  s32_t tcp_ret = print_netstat_tcp(ndata, &entry_buf_offset);
  if (tcp_ret == ERR_VAL) {
    goto out;
  }
#endif
#if LWIP_UDP
  s32_t udp_ret = print_netstat_udp(ndata, &entry_buf_offset);
  if (udp_ret == ERR_VAL) {
    goto out;
  }
#endif
#ifndef CUSTOM_AT_COMMAND
#if LWIP_RAW
  s32_t raw_ret = print_netstat_raw(ndata, &entry_buf_offset);
  if (raw_ret == ERR_VAL) {
    goto out;
  }
#endif
#endif
out:
  ndata->netstat_out_buf_updated_len = entry_buf_offset;
  sys_sem_signal(&ndata->cb_completed);
  return;
}

static void
print_netstat_out_buf_updated_len(struct netstat_data *ndata)
{
  u32_t print_len = 0;
  char print_out_buff[MAX_PRINT_SIZE] = {0};
  if (ndata->netstat_out_buf_updated_len < MAX_PRINT_SIZE) {
#ifdef CUSTOM_AT_COMMAND
    (void)hi_at_printf("%s", (char *)(ndata->netstat_out_buf));
#else
    PRINTK("%s"CRLF, (char *)(ndata->netstat_out_buf));
#endif
  } else {
    do {
      (void)memset_s(print_out_buff, sizeof(print_out_buff), 0x0, sizeof(print_out_buff));
      (void)memcpy_s(print_out_buff, sizeof(print_out_buff) - 1, ndata->netstat_out_buf + print_len,
                     sizeof(print_out_buff) - 1);
#ifdef CUSTOM_AT_COMMAND
      (void)hi_at_printf("%s", print_out_buff);
#else
       PRINTK("%s", print_out_buff);
#endif
       ndata->netstat_out_buf_updated_len -= sizeof(print_out_buff) - 1;
       print_len += sizeof(print_out_buff) - 1;
    } while (ndata->netstat_out_buf_updated_len >= (MAX_PRINT_SIZE - 1));

    if (ndata->netstat_out_buf_updated_len > 0) {
#ifdef CUSTOM_AT_COMMAND
      (void)hi_at_printf("%s", (char *)(ndata->netstat_out_buf + print_len));
#else
       PRINTK("%s", (char *)(ndata->netstat_out_buf + print_len));
#endif
    }
    PRINTK(CRLF);
  }
#ifdef CUSTOM_AT_COMMAND
  (void)hi_at_printf("OK"CRLF);
#endif
}

u32_t
os_shell_netstat(int argc, char **argv)
{
  struct netstat_data ndata;
  err_t err;

  if (argc > 0) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_USAGE);
    return LOS_NOK;
  }

  if (tcpip_init_finish == 0) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_TCPIP_UNINTED);
    return LOS_NOK;
  }
  (void)memset_s(&ndata, sizeof(struct netstat_data), 0, sizeof(struct netstat_data));
  ndata.netstat_out_buf = mem_malloc(MAX_NETSTAT_ENTRY);
  if (ndata.netstat_out_buf == NULL) {
    PRINT_ERRCODE(API_SHELL_ERRCODE_MEM_ERR);
    return LOS_NOK;
  }
  ndata.netstat_out_buf_len = MAX_NETSTAT_ENTRY;
  ndata.netstat_out_buf_updated_len = 0;

  if (sys_sem_new(&ndata.cb_completed, 0) != ERR_OK) {
    goto err_hand;
  }

  err = tcpip_callback(netstat_internal, &ndata);
  if (err != ERR_OK) {
    sys_sem_free(&ndata.cb_completed);
    goto err_hand;
  }

  (void)sys_arch_sem_wait(&ndata.cb_completed, 0);
  sys_sem_free(&ndata.cb_completed);
  if ((ndata.netstat_out_buf_updated_len > 0) && (ndata.netstat_out_buf_updated_len < MAX_NETSTAT_ENTRY)) {
    print_netstat_out_buf_updated_len(&ndata);
    mem_free(ndata.netstat_out_buf);
    return LOS_OK;
  } else if (ndata.netstat_out_buf_updated_len >= MAX_NETSTAT_ENTRY) {
    goto err_hand;
  } else {
#ifdef CUSTOM_AT_COMMAND
    (void)hi_at_printf("OK"CRLF);
#endif
    mem_free(ndata.netstat_out_buf);
    return LOS_OK;
  }

err_hand:
  mem_free(ndata.netstat_out_buf);
  ndata.netstat_out_buf = NULL;
  (void)(argv);
  return LOS_NOK;
}
#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(netstat_shellcmd, CMD_TYPE_EX, "netstat",  XARGS, (CmdCallBackFunc)os_shell_netstat);
#endif /* LOSCFG_SHELL */

#if LWIP_ENABLE_MESH_SHELL_CMD
#if LWIP_RIPPLE
#define RPL_INSTANCE_ID 99
#define DEFAULT_NONMESH_INTERFACE  "wlan0"
static void
rpl_lbr_start(void)
{
  uint8_t ret;
  rpl_addr_t dag_id;
  rpl_prefix_t prefix;
  struct netif *sta_netif = NULL;

  RPL_SET_ADDR(&dag_id, 0xfd00, 0, 0, 0, 0, 0, 0, 0x1000);
  RPL_SET_ADDR(&prefix.addr, 0xfd00, 0, 0, 0, 0, 0, 0, 0x0000);
  prefix.len = 64; /* 64 : ipv6 prefix len */
  prefix.lifetime = 0xffff; /* 0xffff : max ipv6 prefix lifetime */

  sta_netif = netif_find(DEFAULT_NONMESH_INTERFACE);
  if (sta_netif == NULL) {
    PRINTK("no such netif named %s"CRLF, DEFAULT_NONMESH_INTERFACE);
    return;
  }

  ret = rpl_get_slaac_addr(&dag_id, rpl_config_get_lladdr());
  if (ret != RPL_OK) {
    PRINTK("RplGetSlaacAddr fail"CRLF);
    return;
  }
  ret = rpl_mgmt_set_root(RPL_INSTANCE_ID, NULL, &dag_id);
  if (ret != RPL_OK) {
    PRINTK("RplMgmtSetRoot fail"CRLF);
    return;
  }

  ret = rpl_mgmt_set_prefix(RPL_INSTANCE_ID, &prefix);
  if (ret != RPL_OK) {
    PRINTK("RplMgmtSetPrefix fail"CRLF);
    return;
  }

  ret = rpl_mgmt_start(RPL_MODE_6LBR);
  if (ret != RPL_OK) {
    PRINTK("RplMgmtStart fail"CRLF);
    return;
  }

#if LWIP_NAT64
  (void)nat64_init(sta_netif);
  (void)netif_set_default(sta_netif);
#else
  PRINTK("NAT64 stateful is not started, just using mesh"CRLF);
#endif
}

static void
rpl_lr_start(void)
{
  (void)rpl_mgmt_start(RPL_MODE_6LR);

#if LWIP_NAT64 && defined(LWIP_NAT64_STATELESS)
  (void)nat64_init(NULL);
#else
  PRINTK("NAT64 stateless is not started, just using ipv6 mesh"CRLF);
#endif
}

static void
rpl_cleanup(void)
{
#if LWIP_NAT64
  (void)nat64_deinit();
#endif
  rpl_mgmt_deinit();
}

static void
rpl_parent_info(rpl_parent_t *prnt)
{
  int i;

  if ((prnt == NULL)) {
    return;
  }
  PRINTK("\t\tlocAddr: %s"CRLF, ip6addr_ntoa((const ip6_addr_t *)(&(prnt->loc_addr))));
  PRINTK("\t\tmacAddr: ");
  for (i = 0; i < prnt->mac_addr.len; i++) {
    if (i == 0) {
      PRINTK("%02x", prnt->mac_addr.addr[i]);
    } else {
      PRINTK(":%02x", prnt->mac_addr.addr[i]);
    }
  }
  PRINTK(CRLF);
  PRINTK("\t\tglobalAddr: %s"CRLF, ip6addr_ntoa((const ip6_addr_t *)(&(prnt->global_addr))));

  PRINTK("\t\tmetric:"CRLF);
  PRINTK("\t\t\ttype: %hhu"CRLF, prnt->mc.type);
  PRINTK("\t\t\tP: %hhu"CRLF, prnt->mc.p);
  PRINTK("\t\t\tC: %hhu"CRLF, prnt->mc.c);
  PRINTK("\t\t\tO: %hhu"CRLF, prnt->mc.o);
  PRINTK("\t\t\tR: %hhu"CRLF, prnt->mc.r);
  PRINTK("\t\t\tA: %hhu"CRLF, prnt->mc.a);
  PRINTK("\t\t\tprec: %hhu"CRLF, prnt->mc.prec);
  PRINTK("\t\t\tobj: %hu"CRLF, prnt->mc.obj.num_hops);

  PRINTK("\t\trank: %hu"CRLF, prnt->rank);
  PRINTK("\t\tlinkMetric: %hu"CRLF, prnt->link_metric);
  PRINTK("\t\tdtsn: %hhu"CRLF, prnt->dtsn);
  PRINTK("\t\tsmRssi: %hhd"CRLF, prnt->sm_rssi);
  PRINTK("\t\tisResFull: %hhu"CRLF, prnt->is_res_full);
  PRINTK("\t\tinuse: %hhu"CRLF, prnt->inuse);
  PRINTK("\t\tisPreferred: %hhu"CRLF, prnt->is_preferred);

  PRINTK("\t\tdis_timer_cnt: %hu"CRLF, prnt->dis_timer_cnt);
  PRINTK("\t\tdis_timer_param: %hu"CRLF, prnt->dis_timer_param);
  PRINTK("\t\tdis_retry_cnt: %hhu"CRLF, prnt->dis_retry_cnt);
  PRINTK("\t\tcur_dis_state: %hhu"CRLF, prnt->cur_dis_state);

  return;
}

static void
prefix_list_info(rpl_prefix_t *list, u8_t list_len)
{
  u8_t i;

  for (i = 0; i < list_len; i++) {
    if (list[i].len == 0) {
      continue;
    }
    PRINTK("\t\t\t%s/%hhu, lifetime %u, autoAddrConf %hhu, isRouterAddr %hhu"CRLF,
           ip6addr_ntoa((const ip6_addr_t *)(&(list[i].addr))), list[i].len, list[i].lifetime,
           list[i].auto_addr_conf, list[i].is_router_addr);
  }

  return;
}

static void
rpl_instance_info(struct rpl_instance_s *inst)
{
  PRINTK("\tinstID: %"RPL_INST_F CRLF, inst->inst_id);
  if (inst->obj_func) {
    PRINTK("\t\tocp: %hu"CRLF, inst->obj_func->ocp);
  }
  PRINTK("\t\tmode: %hhu"CRLF, inst->mode);
  PRINTK("\t\tisroot: %hhu"CRLF, inst->isroot);
  PRINTK("\t\ttarget:"CRLF);
  prefix_list_info(inst->target, RPL_CONF_MAX_TARGETS);
  PRINTK("\t\tprefix:"CRLF);
  prefix_list_info(inst->prefix, RPL_CONF_MAX_PREFIXES);
  PRINTK("\t\tcfg:"CRLF);
  PRINTK("\t\t\trackTimerVal: %u"CRLF, inst->cfg.rack_timer_val);
  PRINTK("\t\t\tlifetimeUnit: %hu"CRLF, inst->cfg.lifetime_unit);
  PRINTK("\t\t\tminRankInc: %hu"CRLF, inst->cfg.min_rank_inc);
  PRINTK("\t\t\tmaxRankInc: %hu"CRLF, inst->cfg.max_rank_inc);
  PRINTK("\t\t\tocp: %hu"CRLF, inst->cfg.ocp);
  PRINTK("\t\t\tlifetime: %hhu"CRLF, inst->cfg.lifetime);
  PRINTK("\t\t\tdioImin: %hhu"CRLF, inst->cfg.dio_imin);
  PRINTK("\t\t\tdioRed: %hhu"CRLF, inst->cfg.dio_red);
  PRINTK("\t\t\tdioIdbl: %hhu"CRLF, inst->cfg.dio_idbl);
  PRINTK("\t\t\tmop: %hhu"CRLF, inst->cfg.mop);
  PRINTK("\t\t\trackRetry: %hhu"CRLF, inst->cfg.rack_retry);

  return;
}

static void
rpl_dag_info(rpl_dag_t *dag)
{
  int         state = 0;
  rpl_parent_t *prnt = NULL;

  if (dag == NULL) {
    return;
  }
  if (dag->inst != NULL) {
    rpl_instance_info(dag->inst);
  }
  PRINTK("\tdodagID: %s"CRLF, ip6addr_ntoa((const ip6_addr_t *)(&(dag->dodag_id))));
  PRINTK("\tmnid: %hhu"CRLF, dag->mnid);
  PRINTK("\tdaoSeq: %hhu"CRLF, dag->dao_seq);
  PRINTK("\tlastJoinStatus: %hhu"CRLF, dag->last_join_status);
  PRINTK("\tcurDaoState: %hhu"CRLF, dag->cur_dao_state);
  PRINTK("\tdaoRetryCnt: %hhu"CRLF, dag->dao_retry_cnt);
  PRINTK("\trank: %hu"CRLF, dag->rank);
  PRINTK("\toldrank: %hu"CRLF, dag->old_rank);
  PRINTK("\tdodagVerNum: %hhu"CRLF, dag->dodag_ver_num);
  PRINTK("\tdtsnOut: %hhu"CRLF, dag->dtsn_out);
  PRINTK("\tpreference: %hhu"CRLF, dag->preference);
  PRINTK("\tpathSeq: %hhu"CRLF, dag->path_seq);
  PRINTK("\tgrounded: %hhu"CRLF, dag->grounded);
  PRINTK("\tmetricUpdated: %hhu"CRLF, dag->metric_updated);
  PRINTK("\tstate: %hhu"CRLF, dag->state);
  PRINTK("\tinuse: %hhu"CRLF, dag->inuse);
  PRINTK("\tis_prefer: %hhu"CRLF, dag->is_prefer);
  PRINTK("\tparent:"CRLF);
  while ((prnt = rpl_get_next_parent(dag, &state))) {
    PRINTK("\t\t[%d]:"CRLF, state - 1);
    rpl_parent_info(prnt);
  }
  return;
}

static void
rpl_dags_info(void)
{
  rpl_dag_t *dag = NULL;
  int state = 0;
  u8_t i, j;

  i = rpl_config_get_rinited();
  j = is_rpl_running();

  PRINTK("rpl status:");
  PRINTK("%s ", (i == RPL_TRUE) ? "inited" : "uninit");
  PRINTK("%s"CRLF, (j == RPL_TRUE) ? "running" : "stopped");

  PRINTK("[%s][%d] start"CRLF, __FUNCTION__, __LINE__);
  while ((dag = rpl_get_next_inuse_dag(&state))) {
    i = (uint8_t)((u32_t)state & 0x0000ffff);
    j = (uint8_t)(((u32_t)state & 0x7fff0000) >> 16);
    PRINTK("[%hhu][%hhu]:"CRLF, i, (u8_t)(j - 1));
    rpl_dag_info(dag);
  }
  PRINTK("[%s][%d] end"CRLF, __FUNCTION__, __LINE__);

  return;
}

static void
os_shell_rpl_internal(void *arg)
{
  struct netif *netif_p = NULL;
  int node_br;
  shell_cmd_t *rpl_cmd = (shell_cmd_t *)arg;
  int argc = rpl_cmd->argc;
  char **argv = rpl_cmd->argv;

  if ((argc == 3) && (strcmp(argv[1], "start") == 0)) {
    netif_p = netif_find(argv[0]);
    if (netif_p == NULL) {
      PRINTK("no such netif named %s"CRLF, argv[0]);
      goto usage;
    }
    node_br = !!atoi(argv[2]);
    if (RPL_OK != rpl_mgmt_init((rpl_netdev_t *)netif_p)) {
      PRINTK("RplMgmtInit fail"CRLF);
      sys_sem_signal(&rpl_cmd->cb_completed);
      return;
    }

    if (node_br) {
      rpl_lbr_start();
    } else {
      rpl_lr_start();
      (void)netif_set_default(netif_p);
    }
    netif_p->flags |= NETIF_IS_RPL_UP;
  } else if ((argc == 2) && (strcmp(argv[1], "stop") == 0)) {
    netif_p = netif_find(argv[0]);
    if (netif_p == NULL) {
      goto usage;
    }
    rpl_cleanup();
    netif_p->flags &= (~NETIF_IS_RPL_UP);
  } else if ((argc == 1) && (strcmp(argv[0], "dag") == 0)) {
    rpl_dags_info();
  } else {
    goto usage;
  }

  sys_sem_signal(&rpl_cmd->cb_completed);
  return;
usage:
  PRINTK("rpl"CRLF"\tifname start isBr"CRLF"\tifname stop"CRLF"\tdag\tprint dags information"CRLF);
  sys_sem_signal(&rpl_cmd->cb_completed);
  return;
}

u32_t
os_shell_rpl(int argc, char **argv)
{
  shell_cmd_t rpl_cmd = {0};
  err_t ret;
  if (argv == NULL) {
    return LOS_NOK;
  }
  if (sys_sem_new(&rpl_cmd.cb_completed, 0) != ERR_OK) {
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("%s: sys_sem_new fail"CRLF, __FUNCTION__);
#else
    PRINTK("%s: sys_sem_new fail"CRLF, __FUNCTION__);
#endif
    return LOS_NOK;
  }

  rpl_cmd.argc = argc;
  rpl_cmd.argv = argv;

  ret = tcpip_callback(os_shell_rpl_internal, &rpl_cmd);
  if (ret != ERR_OK) {
    sys_sem_free(&rpl_cmd.cb_completed);
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("rpl : internal error, ret:%d"CRLF, ret);
#else
    PRINTK("rpl : internal error, ret:%d"CRLF, ret);
#endif
    return LOS_NOK;
  }
  (void)sys_arch_sem_wait(&rpl_cmd.cb_completed, 0);
  sys_sem_free(&rpl_cmd.cb_completed);

  return LOS_OK;
}
#endif

#if LWIP_RPL
#define DEFAULT_INTERFACE_NAME  "wlan0"
#define DEFAULT_RPL_INSTANCE_ID (99)
#define DEFAULT_IPV6_IID_LEN  (64)

static struct netif *
find_rpl_netif(char *iface)
{
  struct netif *nif = NULL;
  if (iface[0] == '\0') {
    if (strncpy_s(iface, IFNAMSIZ, DEFAULT_INTERFACE_NAME, (strlen(DEFAULT_INTERFACE_NAME))) == EOK) {
      iface[IFNAMSIZ - 1] = '\0';
    } else {
      return NULL;
    }
  }
  nif = netif_find(iface);
  if (nif == NULL) {
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("netif %s is not exist."CRLF, iface);
#else
    PRINTK("netif %s is not exist."CRLF, iface);
#endif
    return NULL;
  } else {
    return nif;
  }
}

static int
start_rpl_network(struct netif *nif, const int is_node_br)
{
  VpprplWSNParametersS wsnparams;
  (void)memset_s(&wsnparams, sizeof(wsnparams), 0, sizeof(wsnparams));

  if (is_node_br == 1) {
    wsnparams.nodeType = VPPRPL_NODE_TYPE_BR;
    wsnparams.instance_id = DEFAULT_RPL_INSTANCE_ID;
    wsnparams.wsnPrefixLen = DEFAULT_IPV6_IID_LEN;
    vpprpl_ip6addr(&wsnparams.wsnPrefix, 0xfd00, 0, 0, 0, 0, 0, 0, 0);
    vpprpl_ip6addr(&wsnparams.dodagID, 0, 0, 0, 0, 0, 0, 0, 0);
  } else {
    wsnparams.nodeType = VPPRPL_NODE_TYPE_NON_BR;
  }

  ret = vpprpl_iface_start_rplnetwork(&wsnparams, (iface_t)nif);
  if (ret != VPPRPL_SUCCESS) {
    PRINTK("vpprpl_iface_start_rplnetwork Failed!\n");
    return LOS_NOK;
  }
  return LOS_OK;
}

u32_t
os_shell_rpl(int argc, char **argv)
{
  uint8_t ret;
  char iface[IFNAMSIZ] = {0};
  int is_node_br = 0;
  struct netif *nif = NULL;
  int i = 0;
  int j = argc;

  if (argc < 1) {
    goto usage;
  }

  while (j > 0) {
    if (strcmp(argv[i], "-b") == 0) {
      is_node_br = 1;
      i++;
      j--;
    } else if (strcmp(argv[i], "-I") == 0) {
      if (strlen(argv[i + 1]) < IFNAMSIZ) {
        (void)strncpy_s(iface, IFNAMSIZ, argv[i + 1], (strlen(argv[i + 1])));
        iface[IFNAMSIZ - 1] = '\0';
      }

      i += 2; /* 2: skip two argv index */
      j -= 2; /* 2: skip already check two parameter */
    } else {
      goto usage;
    }
  }

  nif = find_rpl_netif(iface);
  if (nif == NULL) {
    return LOS_NOK;
  }

  ret = start_rpl_network(nif, is_node_br);
  if (ret == LOS_NOK) {
    return LOS_NOK;
  }

#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("rpl %s started!"CRLF, is_node_br ? "MBR" : "MG");
#else
  PRINTK("rpl %s started!"CRLF, is_node_br ? "MBR" : "MG");
#endif
  return LOS_OK;

usage:
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf(CRLF"Usage:"CRLF);
    (void)hi_at_printf("rpl [-b] [-I interface] ..."CRLF);
#else
  PRINTK(CRLF"Usage:"CRLF);
  PRINTK("rpl [-b] [-I interface] ..."CRLF);
#endif
  return LOS_NOK;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(rpl_shellcmd, CMD_TYPE_EX, "rpl", XARGS, (CmdCallBackFunc)os_shell_rpl);
#endif /* LOSCFG_SHELL */

#endif /* LWIP_RPL */

#define MCAST6_TEST_PORT (5000)
#define MCAST6_TEST_TASK_NAME "mcast6"
#define MCAST6_TEST_TASK_PRIORITY 25
#define MCAST6_TEST_TASK_STACK 4096
#define MCAST6_RECV_BUF_LEN (128)
#define MCAST6_TASK_DELAY (200) // ms
#define MCAST6_TASK_STOP_DELAY (1000) // ms

static int g_ser_fd = -1;
static int g_cli_fd = -1;
static u8_t g_mcast6_cli_task_finish = lwIP_FALSE;
static u8_t g_mcast6_ser_ip_type = IPADDR_TYPE_V6;

static void
os_shell_mcast6_usage(void)
{
  PRINTK("mcast6\n\tser {start srcAddr | stop}\n\tser send destAddr message\n"
         "\tcli {start | stop}\n\ttable show"CRLF
#if LWIP_DHCP_COAP_RELAY
         "\tcoap {mg ifname | mbr dhcpServAddr coapIf dhcpIf}"CRLF
#endif
         "\tesmrf {init | deinit}"CRLF);
}

static int
mcast6_ser_socket_op(struct sockaddr *sockaddr, const socklen_t addr_len)
{
  int opt, ret;
  opt = lwIP_TRUE;
  ret = setsockopt(g_ser_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (ret != ERR_OK) {
    PRINTK("setsockopt fail"CRLF);
    return -1;
  }

  ret = setsockopt(g_ser_fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
  if (ret != ERR_OK) {
    PRINTK("setsockopt bcast fail"CRLF);
    return -1;
  }

  ret = bind(g_ser_fd, sockaddr, addr_len);
  if (ret != ERR_OK) {
    PRINTK("bind src_addr fail"CRLF);
    return -1;
  }
  return 0;
}

static err_t
mcast6_ser_start(char *src_addr)
{
  int ret;
  int domain;
  struct sockaddr_in6 addr6 = {0};
  struct sockaddr_in addr = {0};
  struct sockaddr *sockaddr = NULL;
  socklen_t addr_len;
  ip_addr_t sip = {0};

  if (g_ser_fd >= 0) {
    PRINTK("ser have started"CRLF);
    return ERR_OK;
  }

  if (ipaddr_aton(src_addr, &sip) == 0) {
    PRINTK("invalid ip6 addr"CRLF);
    return ERR_ARG;
  }

  if (sip.type == IPADDR_TYPE_V6) {
    domain = AF_INET6;
    addr6.sin6_family = AF_INET6;
#if LWIP_LITEOS_COMPAT
    (void)memcpy_s((&addr6.sin6_addr)->s6_addr, sizeof(struct ip6_addr),
                   (&sip.u_addr.ip6)->addr, sizeof(struct ip6_addr));
#else
    inet6_addr_from_ip6addr(&addr6.sin6_addr, &sip.u_addr.ip6);
#endif
    addr6.sin6_port = 0;
    addr6.sin6_scope_id = 0;
    sockaddr = (struct sockaddr *)&addr6;
    addr_len = sizeof(addr6);
    g_mcast6_ser_ip_type = IPADDR_TYPE_V6;
  } else {
    domain = AF_INET;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = sip.u_addr.ip4.addr;
    sockaddr = (struct sockaddr *)&addr;
    addr_len = sizeof(addr);
    g_mcast6_ser_ip_type = IPADDR_TYPE_V4;
  }

  g_ser_fd = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
  if (g_ser_fd < 0) {
    PRINTK("socket fail"CRLF);
    return -1;
  }

  ret = mcast6_ser_socket_op(sockaddr, addr_len);
  if (ret == -1) {
    goto failure;
  }

  return ERR_OK;
failure:
  (void)closesocket(g_ser_fd);
  g_ser_fd = -1;
  return -1;
}

static err_t
mcast6_ser_stop(void)
{
  if (g_ser_fd >= 0) {
    (void)closesocket(g_ser_fd);
    g_ser_fd = -1;
  }
  PRINTK("stop success"CRLF);

  return ERR_OK;
}

static err_t
mcast6_ser_send(char *groupaddr, char *msg)
{
  struct sockaddr_in addr;
  struct sockaddr_in6 addr6 = {0};
  struct sockaddr *to = NULL;
  socklen_t tolen;
  ip_addr_t dip = {0};
  ssize_t actual_send;

  if (g_ser_fd < 0) {
    PRINTK("ser not started"CRLF);
    return ERR_VAL;
  }

  if (ipaddr_aton(groupaddr, &dip) == 0) {
    PRINTK("invalid groupaddr %s"CRLF, groupaddr);
    return ERR_ARG;
  }
  if (dip.type != g_mcast6_ser_ip_type) {
    PRINTK("invalid ip ver"CRLF);
    return ERR_ARG;
  }
  if (g_mcast6_ser_ip_type == IPADDR_TYPE_V6) {
    if (!ip6_addr_ismulticast(&dip.u_addr.ip6)) {
      PRINTK("not mcast6 addr"CRLF);
      return ERR_ARG;
    }
    addr6.sin6_family = AF_INET6;
#if LWIP_LITEOS_COMPAT
    (void)memcpy_s((&addr6.sin6_addr)->s6_addr, sizeof(struct ip6_addr),
                   (&dip.u_addr.ip6)->addr, sizeof(struct ip6_addr));
#else
    inet6_addr_from_ip6addr(&addr6.sin6_addr, &dip.u_addr.ip6);
#endif
    addr6.sin6_port = htons(MCAST6_TEST_PORT);
    to = (struct sockaddr *)&addr6;
    tolen = sizeof(addr6);
  } else {
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MCAST6_TEST_PORT);
    addr.sin_addr.s_addr = dip.u_addr.ip4.addr;
    to = (struct sockaddr *)&addr;
    tolen = sizeof(addr);
  }
  PRINTK("send %s to %s"CRLF, msg, ipaddr_ntoa(&dip));

  actual_send = sendto(g_ser_fd, msg, strlen(msg), 0, to, tolen);
  PRINTK("[%zu] actual_send %zd"CRLF, strlen(msg), actual_send);
  if (actual_send < 0) {
    (void)closesocket(g_ser_fd);
    g_ser_fd = -1;
    PRINTK("udp abort"CRLF);
    return ERR_NETUNREACH;
  }

  return ERR_OK;
}

static err_t
mcast6_ser_ctrl(int argc, char **argv)
{
  err_t ret;

  if ((argc == 3) && (strcmp(argv[1], "start") == 0)) {
    ret = mcast6_ser_start(argv[2]);
  } else if (strcmp(argv[1], "stop") == 0) {
    ret = mcast6_ser_stop();
  } else if ((argc == 4) && (strcmp(argv[1], "send") == 0)) {
    ret = mcast6_ser_send(argv[2], argv[3]);
  } else {
    goto failure;
  }

  return ret;

failure:
  os_shell_mcast6_usage();
  return ERR_ARG;
}

static int
mcast_cli_task_socket_op(void)
{
  int opt;
  int ret;
  g_cli_fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
  if (g_cli_fd < 0) {
    PRINTK("socket fail"CRLF);
    return -1;
  }

  opt = lwIP_TRUE;
  ret = setsockopt(g_cli_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (ret != ERR_OK) {
    PRINTK("setsockopt reuse fail"CRLF);
    return -1;
  }

  ret = setsockopt(g_cli_fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
  if (ret != ERR_OK) {
    PRINTK("setsockopt bcast fail"CRLF);
    return -1;
  }
  return 0;
}

static err_t
mcast6_cli_task_start(void)
{
  int ret, actual_recv;
  struct sockaddr *sockaddr = NULL;
  socklen_t addr_len;
  struct sockaddr_in6 addr6 = {0};
  char buffer[MCAST6_RECV_BUF_LEN] = {0};

  ret = mcast_cli_task_socket_op();
  if (ret == -1) {
    goto failure;
  }

  addr6.sin6_family = AF_INET6;
#if LWIP_LITEOS_COMPAT
  (void)memcpy_s((&addr6.sin6_addr)->s6_addr, sizeof(struct ip6_addr),
                 (&(IP6_ADDR_ANY->u_addr.ip6))->addr, sizeof(struct ip6_addr));
#else
  inet6_addr_from_ip6addr(&addr6.sin6_addr, &(IP6_ADDR_ANY->u_addr.ip6));
#endif
  addr6.sin6_port = htons(MCAST6_TEST_PORT);
  sockaddr = (struct sockaddr *)&addr6;
  addr_len = sizeof(addr6);

  if (bind(g_cli_fd, sockaddr, addr_len) != 0) {
    PRINTK("bind fail"CRLF);
    goto failure;
  }

  PRINTK("mcast6 start to recv"CRLF);
  while (g_mcast6_cli_task_finish == lwIP_FALSE) {
    actual_recv = recvfrom(g_cli_fd, buffer, MCAST6_RECV_BUF_LEN, 0, sockaddr, &addr_len);
    if (actual_recv < 0) {
      PRINTK("recvfrom failed"CRLF);
      break;
    } else {
      PRINTK("[Mcast6CliRecv]recv len : %d, data : %.*s, from %s:%hu"CRLF, actual_recv, actual_recv, buffer,
             ip6addr_ntoa((const ip6_addr_t *)(&addr6.sin6_addr)), ntohs(addr6.sin6_port));
#if (LWIP_LITEOS_COMPAT == 0)
      fflush(NULL);
#endif
    }
    sys_msleep(MCAST6_TASK_DELAY);
  }

  g_mcast6_cli_task_finish = lwIP_FALSE;
  (void)closesocket(g_cli_fd);
  g_cli_fd = -1;
  PRINTK("task exit"CRLF);
  return 0;
failure:
  if (g_cli_fd >= 0) {
    (void)closesocket(g_cli_fd);
    g_cli_fd = -1;
  }
  return -1;
}

static err_t
mcast6_cli_stop(void)
{
  if (g_cli_fd < 0) {
    PRINTK("ser have stopped"CRLF);
    return LOS_OK;
  }
  g_mcast6_cli_task_finish = lwIP_TRUE;
  (void)closesocket(g_cli_fd);
  g_cli_fd = -1;
  PRINTK("wait task to stop"CRLF);
  sys_msleep(MCAST6_TASK_STOP_DELAY);
  PRINTK("task stop success"CRLF);

  return LOS_OK;
}

static err_t
mcast6_cli_start(void)
{
  u32_t ret;

  if (g_cli_fd >= 0) {
    PRINTK("mcast6 cli is running"CRLF);
    return LOS_OK;
  }

  ret = sys_thread_new(MCAST6_TEST_TASK_NAME, (lwip_thread_fn)mcast6_cli_task_start, NULL, MCAST6_TEST_TASK_STACK,
                       MCAST6_TEST_TASK_PRIORITY);
  if (ret == SYS_ARCH_ERROR) {
    PRINTK("create task %s failed", MCAST6_TEST_TASK_NAME);
    return LOS_NOK;
  }

  return LOS_OK;
}

static err_t
mcast6_cli_ctrl(char **argv)
{
  err_t ret;

  if (strcmp(argv[1], "start") == 0) {
    ret = mcast6_cli_start();
  } else if (strcmp(argv[1], "stop") == 0) {
    ret = mcast6_cli_stop();
  } else {
    goto failure;
  }

  return ret;

failure:
  os_shell_mcast6_usage();
  return ERR_ARG;
}

#if LWIP_MPL
static void
mcast6_table_print(void *argv)
{
  sys_sem_t *cb_completed = (sys_sem_t *)argv;
  mcast6_table_t *list = mcast6_get_table_list();

  PRINTK("%-50s%-16s"CRLF, "mcastAddr", "lifetime");
  while (list != NULL) {
    PRINTK("%-50s%-16u"CRLF, ip6addr_ntoa((const ip6_addr_t *)(&(list->addr))), list->lifetime);
    list = list->next;
  }

  sys_sem_signal(cb_completed);
  return;
}

static err_t
mcast6_table_ctrl(void)
{
  sys_sem_t cb_completed;
  err_t ret;

  if (sys_sem_new(&cb_completed, 0) != ERR_OK) {
    PRINTK("%s: sys_sem_new fail"CRLF, __FUNCTION__);
    return LOS_NOK;
  }

  ret = tcpip_callback(mcast6_table_print, &cb_completed);
  if (ret != ERR_OK) {
    sys_sem_free(&cb_completed);
    PRINTK("l2test : internal error, ret:%d"CRLF, ret);
    return LOS_NOK;
  }
  (void)sys_arch_sem_wait(&cb_completed, 0);
  sys_sem_free(&cb_completed);

  return ERR_OK;
}
#else
static err_t
mcast6_table_ctrl(void)
{
  return ERR_VAL;
}
#endif /* LWIP_MPL */

static err_t
mcast6_coap_mg_ctrl(char **argv)
{
#if LWIP_DHCP_COAP_RELAY
  struct netif *netif = NULL;

  netif = netif_find(argv[2]);
  if (netif == NULL) {
    PRINTK("no netif named %s"CRLF, argv[2]);
    return ERR_VAL;
  }

  return dhcp_relay_fake_server_start(netif, netif);
#else
  (void)argv;
  return ERR_VAL;
#endif
}

static err_t
mcast6_coap_mbr_ctrl(char **argv)
{
#if LWIP_DHCP_COAP_RELAY
  ip_addr_t dhcps_ip_addr = IPADDR4_INIT(0);
  struct netif *netif_dhcp = NULL;
  struct netif *netif_coap = NULL;

  if (ip4addr_aton((const char *)(argv[2]), ip_2_ip4(&(dhcps_ip_addr))) == 0) {
    PRINTK("invalid ip4addr %s"CRLF, argv[2]);
    return ERR_VAL;
  }

  netif_coap = netif_find(argv[3]);
  if (netif_coap == NULL) {
    PRINTK("no netif named %s"CRLF, argv[3]);
    return ERR_VAL;
  }

  netif_dhcp = netif_find(argv[4]);
  if (netif_dhcp == NULL) {
    PRINTK("no netif named %s"CRLF, argv[4]);
    return ERR_VAL;
  }

  return dhcp_relay_fake_client_start(dhcps_ip_addr, netif_dhcp, netif_coap);
#else
  (void)argv;
  return ERR_VAL;
#endif
}

static err_t
mcast6_coap_ctrl(int argc, char **argv)
{
  err_t ret;

  if ((strcmp(argv[1], "mg") == 0) && (argc == 3)) {
    ret = mcast6_coap_mg_ctrl(argv);
  } else if ((strcmp(argv[1], "mbr") == 0) && (argc == 5)) {
    ret = mcast6_coap_mbr_ctrl(argv);
  } else {
    goto failure;
  }

  return ret;

failure:
  os_shell_mcast6_usage();
  return ERR_ARG;
}

static err_t
mcast6_esmrf_ctrl(int argc, char **argv)
{
#if LWIP_MPL
  err_t ret;

  if ((strcmp(argv[1], "init") == 0) && (argc == 2)) {
    ret = mcast6_init();
  } else if ((strcmp(argv[1], "deinit") == 0) && (argc == 2)) {
    mcast6_deinit();
    ret = 0;
  } else {
    goto failure;
  }

  return ret;

failure:
  os_shell_mcast6_usage();
  return ERR_ARG;
#else
  (void)argc;
  (void)argv;
  return ERR_ARG;
#endif
}

u32_t
os_shell_mcast6(int argc, char **argv)
{
  int ret = -1;
  if (argv == NULL) {
    return LOS_NOK;
  }
  if (argc < 2) { /* 2 : min argc num */
    goto failure;
  }
  if (strcmp(argv[0], "ser") == 0) {
    ret = mcast6_ser_ctrl(argc, argv);
  } else if (strcmp(argv[0], "cli") == 0) {
    ret = mcast6_cli_ctrl(argv);
  } else if ((strcmp(argv[0], "table") == 0) && (strcmp(argv[1], "show") == 0)) {
    ret = mcast6_table_ctrl();
  } else if (strcmp(argv[0], "coap") == 0) {
    ret = mcast6_coap_ctrl(argc, argv);
  } else if (strcmp(argv[0], "esmrf") == 0) {
    ret = mcast6_esmrf_ctrl(argc, argv);
  } else {
    goto failure;
  }
#ifdef LWIP_DEBUG_OPEN
  (void)hi_at_printf("mcast6 ret %d"CRLF, ret);
#else
  PRINTK("mcast6 ret %d"CRLF, ret);
#endif
  return ((ret == LOS_OK) ? LOS_OK : LOS_NOK);

failure:
  os_shell_mcast6_usage();
  return LOS_NOK;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(mcast6_shellcmd, CMD_TYPE_EX, "mcast6", XARGS, (CmdCallBackFunc)os_shell_mcast6);
#endif /* LOSCFG_SHELL */

#if (LWIP_IPV6 && (LWIP_IPV6_MLD || LWIP_IPV6_MLD_QUERIER))
static void
os_shell_mld6_usage(void)
{
#ifdef LWIP_DEBUG_OPEN
  (void)hi_at_printf("mld6\r\n\tshow\r\n\tifname {report | stop}\r\n\tifname {join | leave} groupaddr\r\n\t\
      ifname querier {start | stop | show}"CRLF);
#else
  PRINTK("mld6\n\tshow\n\tifname {report | stop}\n\tifname {join | leave} groupaddr\n\tifname querier \
      {start | stop | show}"CRLF);
#endif
}

static void
mld6_netif_show(void)
{
  struct netif *netif_p = NULL;
  struct mld_group *group = NULL;

#ifdef LWIP_DEBUG_OPEN
  (void)hi_at_printf("\t%-50s%-16s%-16s%-16s%-16s"CRLF, "groupaddr", "reporter", "state", "timer(100ms)", "use");
#else
  PRINTK("\t%-50s%-16s%-16s%-16s%-16s"CRLF, "groupaddr", "reporter", "state", "timer(100ms)", "use");
#endif
  for (netif_p = netif_list; netif_p != NULL; netif_p = netif_p->next) {
    if ((netif_p->flags & NETIF_FLAG_MLD6) == 0) {
      continue;
    }
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("%s%d"CRLF, netif_p->name, netif_p->num);
#else
    PRINTK("%s%d"CRLF, netif_p->name, netif_p->num);
#endif
    group = netif_mld6_data(netif_p);
    if (group == NULL) {
#ifdef LWIP_DEBUG_OPEN
      (void)hi_at_printf(CRLF);
#else
      PRINTK("\n");
#endif
      continue;
    }
    while (group != NULL) {
#ifdef LWIP_DEBUG_OPEN
      (void)hi_at_printf("\t%-50s%-16d%-16d%-16d%-16d"CRLF, ip6addr_ntoa(&(group->group_address)),
                         group->last_reporter_flag, group->group_state,
                         group->timer, group->use);
#else
      PRINTK("\t%-50s%-16d%-16d%-16d%-16d"CRLF, ip6addr_ntoa(&(group->group_address)),
             group->last_reporter_flag, group->group_state,
             group->timer, group->use);
#endif
      group = group->next;
    }
  }
}

static s32_t
mld6_netif_ctrl(char **argv)
{
  s32_t ret = 0;
  struct netif *netif_p = NULL;

  netif_p = netif_find(argv[0]);
  if (netif_p == NULL) {
    PRINTK("no such netif named %s"CRLF, argv[0]);
    goto usage;
  }
  if (strcmp(argv[1], "stop") == 0) {
    ret = mld6_stop(netif_p);
  } else if (strcmp(argv[1], "report") == 0) {
    mld6_report_groups(netif_p);
  } else {
    goto usage;
  }

  return ret;
usage:
  os_shell_mld6_usage();
  return -1;
}

static s32_t
mld6_membership_ctrl(char **argv)
{
  s32_t ret = 0;
  struct netif *netif_p = NULL;
  ip6_addr_t groupaddr = {0};

  if (!ip6addr_aton(argv[2], &(groupaddr))) {
    PRINTK("Invalid IPv6 Address : %s"CRLF, argv[2]);
    goto usage;
  }

  if (ip6_addr_isany_val(groupaddr) || ip6_addr_isnone_val(groupaddr) || !(ip6_addr_ismulticast(&groupaddr))) {
    PRINTK("invalid groupaddr %s"CRLF, argv[2]);
    goto usage;
  }
  netif_p = netif_find(argv[0]);
  if (netif_p == NULL) {
    PRINTK("no such netif named %s"CRLF, argv[0]);
    goto usage;
  }
  if (strcmp(argv[1], "join") == 0) {
    ret = mld6_joingroup_netif(netif_p, &groupaddr);
  } else if (strcmp(argv[1], "leave") == 0) {
    ret = mld6_leavegroup_netif(netif_p, &groupaddr);
  } else {
    goto usage;
  }

  return ret;
usage:
  os_shell_mld6_usage();
  return -1;
}

#if LWIP_IPV6_MLD_QUERIER
static void
mld6_querier_status_show(struct mld6_querier *querier)
{
  struct mld6_listener *listener = NULL;

#ifdef LWIP_DEBUG_OPEN
  (void)hi_at_printf("\t%-16s%-16s%-16s"CRLF, "state", "count", "timer");
  (void)hi_at_printf("\t%-16hhu%-16hhu%-16hu"CRLF, querier->querier_state, querier->query_count, querier->timer);
  (void)hi_at_printf("listeners:"CRLF);
#else
  PRINTK("\t%-16s%-16s%-16s"CRLF, "state", "count", "timer");
  PRINTK("\t%-16hhu%-16hhu%-16hu"CRLF, querier->querier_state, querier->query_count, querier->timer);
  PRINTK("listeners:"CRLF);
#endif

  if (querier->listeners != NULL) {
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("\t%-50s%-16s%-16s%-16s%-16s"CRLF, "addr", "state", "count", "timer", "rexmt_timer");
#else
    PRINTK("\t%-50s%-16s%-16s%-16s%-16s"CRLF, "addr", "state", "count", "timer", "rexmt_timer");
#endif
    listener = querier->listeners;
    do {
#ifdef LWIP_DEBUG_OPEN
      (void)hi_at_printf("\t%-50s%-16hhu%-16hhu%-16hu%-16hu"CRLF, ip6addr_ntoa(&(listener->group_address)),
                         listener->state, listener->query_count, listener->timer, listener->rexmt_timer);
#else
      PRINTK("\t%-50s%-16hhu%-16hhu%-16hu%-16hu"CRLF, ip6addr_ntoa(&(listener->group_address)),
             listener->state, listener->query_count, listener->timer, listener->rexmt_timer);
#endif
      listener = listener->next;
    } while (listener != NULL);
  }

  return;
}
#endif /* LWIP_IPV6_MLD_QUERIER */

static s32_t
mld6_querier_ctrl(char **argv)
{
#if LWIP_IPV6_MLD_QUERIER
  s32_t ret = 0;
  struct netif *netif_p = NULL;
  struct mld6_querier *querier = NULL;

  netif_p = netif_find(argv[0]);
  if (netif_p == NULL) {
    PRINTK("no such netif named %s"CRLF, argv[0]);
    goto usage;
  }
  if (strcmp(argv[2], "start") == 0) {
    ret = mld6_querier_start(netif_p);
  } else if (strcmp(argv[2], "stop") == 0) {
    mld6_querier_stop(netif_p);
  } else if (strcmp(argv[2], "show") == 0) {
    querier = netif_mld6_querier_data(netif_p);
    if (querier == NULL) {
      PRINTK("mld6 querier not start"CRLF);
    } else {
      mld6_querier_status_show(querier);
    }
  } else {
    goto usage;
  }

  return ret;
usage:
  os_shell_mld6_usage();
  return -1;
#else
  (void)argv;
  PRINTK("mld6 querier not support"CRLF);
  return -1;
#endif /* LWIP_IPV6_MLD_QUERIER */
}

u32_t
os_shell_mld6(int argc, char **argv)
{
  s32_t ret = 0;

  if (tcpip_init_finish == 0) {
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("%s: tcpip_init have not been called"CRLF, __FUNCTION__);
#else
    PRINTK("%s: tcpip_init have not been called"CRLF, __FUNCTION__);
#endif
    return LOS_NOK;
  }
  if (argv == NULL) {
    return LOS_NOK;
  }
  if ((argc == 1) && (strcmp(argv[0], "show") == 0)) {
    mld6_netif_show();
  } else if (argc == 2) { /* 2 : argc index */
    ret = mld6_netif_ctrl(argv);
  } else if (argc == 3) { /* 3 : argc index */
    if (strcmp(argv[1], "querier") == 0) {
      ret = mld6_querier_ctrl(argv);
    } else {
      ret = mld6_membership_ctrl(argv);
    }
  } else {
    os_shell_mld6_usage();
  }
#ifdef LWIP_DEBUG_OPEN
  (void)hi_at_printf("mld6 ret %d"CRLF, ret);
#else
  PRINTK("mld6 ret %d"CRLF, ret);
#endif
  return LOS_OK;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(mld6_shellcmd, CMD_TYPE_EX, "mld6", XARGS, (CmdCallBackFunc)os_shell_mld6);
#endif /* LOSCFG_SHELL */
#endif /* (LWIP_IPV6 && (LWIP_IPV6_MLD || LWIP_IPV6_MLD_QUERIER)) */

#if LWIP_RIPPLE & RPL_CONF_IP6
u32_t
os_rte_debug(int argc, char **argv)
{
  s32_t state = 0;
  rpl_addr_t tgt;
  rpl_addr_t nhop;
  rpl_mnode_id_t mnid;
  u16_t lt;
  s32_t cnt = 0;
  ip6_addr_t ipv6_addr;
  char ac_ipv6_addr[IP6ADDR_STRLEN_MAX + 1] = {0};
  char *tmp = NULL;
  (void)argc;
  (void)argv;
  if (tcpip_init_finish == 0) {
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("%s: tcpip_init have not been called"CRLF, __FUNCTION__);
#else
    PRINTK("%s: tcpip_init have not been called"CRLF, __FUNCTION__);
#endif
    goto exit;
  }
  LOCK_TCPIP_CORE();
#ifdef LWIP_DEBUG_OPEN
  (void)hi_at_printf("%s, %s, %s, %s, %s"CRLF, "Idx", "RplAddrS", "Nhop", "MNID", "Ltime");
#else
  LWIP_PLATFORM_PRINT("%s, %s, %s, %s, %s"CRLF, "Idx", "RplAddrS", "Nhop", "MNID", "Ltime");
#endif
  while (rpl_platform_get_next_rte(&state, &tgt, &nhop, &lt, &mnid) == RPL_OK) {
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("%d,", ++cnt);
#else
    LWIP_PLATFORM_PRINT("%d,", ++cnt);
#endif
    if (memcpy_s(ipv6_addr.addr, sizeof(ipv6_addr.addr), tgt.a8, sizeof(tgt.a8)) != EOK) {
#ifdef LWIP_DEBUG_OPEN
      (void)hi_at_printf("rtedebug memcpy_s fail"CRLF);
#else
      LWIP_PLATFORM_PRINT("rtedebug memcpy_s fail"CRLF);
#endif
      goto exit;
    }
    tmp = ip6addr_ntoa_r((const ip6_addr_t *)ipv6_addr.addr, ac_ipv6_addr, INET6_ADDRSTRLEN);
    if (tmp == NULL) {
#ifdef LWIP_DEBUG_OPEN
      (void)hi_at_printf("rtedebug ip6addr_ntoa_r is null"CRLF);
#else
      LWIP_PLATFORM_PRINT("rtedebug ip6addr_ntoa_r is null"CRLF);
#endif
      goto exit;
    } else {
#ifdef LWIP_DEBUG_OPEN
      (void)hi_at_printf("%s,", ac_ipv6_addr);
#else
      LWIP_PLATFORM_PRINT("%s,", ac_ipv6_addr);
#endif
    }
    if (memcpy_s(ipv6_addr.addr, sizeof(ipv6_addr.addr), nhop.a8, sizeof(nhop.a8)) != EOK) {
#ifdef LWIP_DEBUG_OPEN
      (void)hi_at_printf("rtedebug memcpy_s fail"CRLF);
#else
      LWIP_PLATFORM_PRINT("rtedebug memcpy_s fail"CRLF);
#endif
      goto exit;
    }
    tmp = ip6addr_ntoa_r((const ip6_addr_t *)ipv6_addr.addr, ac_ipv6_addr, INET6_ADDRSTRLEN);
    if (tmp == NULL) {
#ifdef LWIP_DEBUG_OPEN
      (void)hi_at_printf("rtedebug ip6addr_ntoa_r is null"CRLF);
#else
      LWIP_PLATFORM_PRINT("rtedebug ip6addr_ntoa_r is null"CRLF);
#endif
      goto exit;
    } else {
#ifdef LWIP_DEBUG_OPEN
      (void)hi_at_printf("%s,", ac_ipv6_addr);
#else
      LWIP_PLATFORM_PRINT("%s,", ac_ipv6_addr);
#endif
    }
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("%u,", mnid);
    (void)hi_at_printf("%u"CRLF, lt);
#else
    LWIP_PLATFORM_PRINT("%u,", mnid);
    LWIP_PLATFORM_PRINT("%u"CRLF, lt);
#endif
  }
  UNLOCK_TCPIP_CORE();
exit:
  return LOS_OK;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(rte_shellcmd, CMD_TYPE_EX, "rtedebug", XARGS, (CmdCallBackFunc)os_rte_debug);
#endif /* LOSCFG_SHELL */
#endif /* LWIP_RIPPLE & RPL_CONF_IP6 */

#if LWIP_RIPPLE
#define L2_TEST_PORT (12345)
#define BYTE_IN_HEX_LEN (2)
#define L2_PBUF_MSG_LEN (32)

static struct udp_pcb *g_test_serv_pcb = NULL;

static void
l2test_pbuf_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  u16_t len = p->len;
  unsigned char *data = (unsigned char *)(p->payload);
  int i;

  (void)arg;
  (void)pcb;
  (void)addr;
  (void)port;

  PRINTK("recv from ");
  for (i = 0; i < NETIF_MAX_HWADDR_LEN; i++) {
    if (i == 0) {
      PRINTK("%02x", p->mac_address[i]);
    } else {
      PRINTK(":%02x", p->mac_address[i]);
    }
  }
  PRINTK(" %s:%hu"CRLF, ip6addr_ntoa(&(addr->u_addr.ip6)), port);
  PRINTK("RSSI : %hhd"CRLF, PBUF_GET_RSSI(p));
  PRINTK("recv len : %hu, data : %.*s"CRLF, len, len, data);

  return;
}

static err_t
l2test_pbuf_stop(void)
{
  if (g_test_serv_pcb != NULL) {
    udp_remove(g_test_serv_pcb);
    g_test_serv_pcb = NULL;
  }

  return LOS_OK;
}

static err_t
l2test_pbuf_start(struct netif *netif)
{
  err_t result;

  if (g_test_serv_pcb != NULL) {
    PRINTK("already start"CRLF);
    return ERR_OK;
  }
  g_test_serv_pcb = udp_new_ip6();
  if (g_test_serv_pcb == NULL) {
    PRINTK("udp_new_ip6 failed"CRLF);
    return ERR_MEM;
  }
  ip_set_option(g_test_serv_pcb, SOF_BROADCAST);
  result = udp_bind(g_test_serv_pcb, IP6_ADDR_ANY, L2_TEST_PORT);
  if (result != ERR_OK) {
    PRINTK("udp_bind failed"CRLF);
    udp_remove(g_test_serv_pcb);
    g_test_serv_pcb = NULL;
    return result;
  }

  result = udp_connect(g_test_serv_pcb, IP6_ADDR_ANY, L2_TEST_PORT);
  if (result != ERR_OK) {
    PRINTK("udp_connect failed"CRLF);
    udp_remove(g_test_serv_pcb);
    g_test_serv_pcb = NULL;
    return result;
  }
  g_test_serv_pcb->ifindex = netif->ifindex;
  udp_recv(g_test_serv_pcb, l2test_pbuf_recv, NULL);

  return result;
}

static err_t
l2test_pbuf_send(struct netif *netif, int argc, char **argv)
{
  err_t err;
  ip_addr_t dst_addr = {0};
  char msg[L2_PBUF_MSG_LEN] = {0};
  struct pbuf *p_out = NULL;
  int pri;

  if (g_test_serv_pcb == NULL) {
    PRINTK("pcb not init"CRLF);
    return ERR_VAL;
  }

  if (ip6addr_aton(argv[3], &(dst_addr.u_addr.ip6)) == ERR_OK) {
    PRINTK("invalid ip6 addr"CRLF);
    return ERR_ARG;
  }
  PRINTK("dst ip6 %s, priority %s"CRLF, ip6addr_ntoa(&(dst_addr.u_addr.ip6)), argv[4]);
  if (sprintf_s(msg, sizeof(msg), "send msg priority %s", argv[4]) == -1) {
    return ERR_MEM;
  }
  IP_SET_TYPE_VAL(dst_addr, IPADDR_TYPE_V6);
  p_out = pbuf_alloc(PBUF_TRANSPORT, (u16_t)(strlen(msg)), PBUF_RAM);
  if (p_out == NULL) {
    PRINTK("pbuf_alloc failed"CRLF);
    return ERR_MEM;
  }
  err = memcpy_s(p_out->payload, (u16_t)(strlen(msg)), msg, (u16_t)(strlen(msg)));
  if (err != EOK) {
    PRINTK("memcpy_s failed\n");
    (void)pbuf_free(p_out);
    return err;
  }

  pri = atoi(argv[4]);
  /* 3 : beacon priority */
  g_test_serv_pcb->priority = (pri > 3) ? 3 : pri;
  /* 6 argc index is six and argv is five */
  if ((argc == 6) && (strcmp(argv[5], "ctrl") == ERR_OK)) {
    p_out->flags |= PBUF_FLAG_CTRL_PKT;
  }

  pbuf_realloc(p_out, (u16_t)(strlen(msg)));

  err = udp_sendto_if(g_test_serv_pcb, p_out, &dst_addr, L2_TEST_PORT, netif);
  PRINTK("send [%s] %d"CRLF, msg, err);
  (void)pbuf_free(p_out);

  return err;
}

static err_t
l2test_remove_peer(struct netif *netif, char *mac_str)
{
  err_t ret;
  struct linklayer_addr peer_addr = {0};
  char *digit = NULL;
  u32_t mac_addr_len = strlen(mac_str) + 1;
  char tmp_str[MAX_MACADDR_STRING_LENGTH];
  char *tmp_str1 = NULL;
  char *save_ptr = NULL;
  int j;

  if (mac_addr_len != MAX_MACADDR_STRING_LENGTH) {
    PRINTK("wrong MAC address format"CRLF);
    return LOS_NOK;
  }

  ret = strncpy_s(tmp_str, mac_addr_len, mac_str, mac_addr_len - 1);
  if (ret != EOK) {
    PRINTK("strncpy_s failed"CRLF);
    return LOS_NOK;
  }
  for (j = 0, tmp_str1 = tmp_str; j < NETIF_MAX_HWADDR_LEN; j++, tmp_str1 = NULL) {
    digit = strtok_r(tmp_str1, ":", &save_ptr);
    if ((digit == NULL) || (strlen(digit) > BYTE_IN_HEX_LEN)) {
      PRINTK("wrong MAC address format"CRLF);
      return LOS_NOK;
    }
    CONVERT_STRING_TO_HEX(digit, peer_addr.addr[j]);
  }
  peer_addr.addrlen = NETIF_MAX_HWADDR_LEN;
  ret = netif_remove_peer(netif, &peer_addr);

  return ret;
}

static err_t
l2test_callback_status(struct netif *netif)
{
#if LWIP_IPV4 && LWIP_IGMP && LWIP_LINK_MCAST_FILTER
  PRINTK("igmp_mac_filter %s"CRLF, ((netif->igmp_mac_filter == NULL) ? "null" : "not null"));
#endif /* LWIP_IPV4 && LWIP_IGMP && LWIP_LINK_MCAST_FILTER */
#if LWIP_IPV6 && LWIP_IPV6_MLD && LWIP_LINK_MCAST_FILTER
  PRINTK("mld_mac_filter %s"CRLF, ((netif->mld_mac_filter == NULL) ? "null" : "not null"));
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD && LWIP_LINK_MCAST_FILTER */
  PRINTK("remove_peer %s"CRLF, ((netif->remove_peer == NULL) ? "null" : "not null"));
  PRINTK("set_beacon_prio %s"CRLF, ((netif->set_beacon_prio == NULL) ? "null" : "not null"));
  PRINTK("set_unique_id %s"CRLF, ((netif->set_unique_id == NULL) ? "null" : "not null"));
  PRINTK("linklayer_event %s"CRLF, ((netif->linklayer_event == NULL) ? "null" : "not null"));

  return LOS_OK;
}

static void
os_shell_l2test_usage(void)
{
  PRINTK("l2test"CRLF);
  PRINTK("\t%-32s%s"CRLF, "ifname remove mac_addr_str", "disassociate peer through MAC address. {l2test wlan0 remove \
      11:22:33:44:55:66}");
  PRINTK("\t%-32s%s"CRLF, "ifname prio priority", "set beacon priority, value of priority is 0~255. {l2test wlan0 prio \
      56}");
  PRINTK("\t%-32s%s"CRLF, "ifname mnid mnid_value", "set mnid of mesh node, value of mnid is 0~127. {l2test wlan0 mnid \
      2}");
  PRINTK("\t%-32s%s"CRLF, "ifname all", "show callback functions of netif registered status. {l2test wlan0 all}");
  PRINTK("\t%-32s%s"CRLF, "ifname pbuf {start | stop}", "start/stop a udp pcb");
  PRINTK("\t%-32s"CRLF, "ifname pbuf send dst_ip6_ip priority_value(0,1,2,3) [ctrl]");
  PRINTK("\t%-32s%s"CRLF, "", "send a message to dst_ip6_ip, priority_value can be one in (0,1,2,3), 'ctrl' is \
      optional");
  PRINTK("\t%-32s"CRLF, "store {read | write} {ver | flag | dtsn | dodagVerNum | pathSeq | daoSeq | dcoSeq} [value]");
  PRINTK("\t%-32s%s"CRLF, "", "range of value is 0~255");

  return;
}

static err_t
l2test_store_read_or_write(char *nv_ctrl, u8_t *is_read)
{
  if (strcmp(nv_ctrl, "read") == ERR_OK) {
    *is_read = lwIP_TRUE;
  } else if (strcmp(nv_ctrl, "write") == ERR_OK) {
    *is_read = lwIP_FALSE;
  } else {
    os_shell_l2test_usage();
    goto failure;
  }
  return ERR_OK;
failure:
  return ERR_ARG;
}

static err_t
l2test_store_key_id(char *nv_key, u8_t *nv_key_id)
{
  if (strcmp(nv_key, "ver") == ERR_OK) {
    *nv_key_id = PS_VER;
  } else if (strcmp(nv_key, "flag") == ERR_OK) {
    *nv_key_id = PS_FLAG;
  } else if (strcmp(nv_key, "dtsn") == ERR_OK) {
    *nv_key_id = PS_DTSN;
  } else if (strcmp(nv_key, "dodagVerNum") == ERR_OK) {
    *nv_key_id = PS_DODAGVERNUM;
  } else if (strcmp(nv_key, "pathSeq") == ERR_OK) {
    *nv_key_id = PS_PATHSEQ;
  } else if (strcmp(nv_key, "daoSeq") == ERR_OK) {
    *nv_key_id = PS_DAOSEQ;
  } else if (strcmp(nv_key, "dcoSeq") == ERR_OK) {
    *nv_key_id = PS_DCOSEQ;
  } else {
    os_shell_l2test_usage();
    goto failure;
  }
  return ERR_OK;
failure:
  return ERR_ARG;
}

static err_t
l2test_store(int argc, char **argv)
{
  err_t ret;
  u8_t is_read = lwIP_TRUE;
  u8_t nv_key_id;
  u8_t value;
  if (argv == NULL) {
    return LOS_NOK;
  }
  ret = l2test_store_read_or_write(argv[1], &is_read);
  if (ret != ERR_OK) {
    goto failure;
  }
  if ((is_read == lwIP_FALSE) && (argc != 4)) { /* 4 ：argc index */
    os_shell_l2test_usage();
    goto failure;
  }
  ret = l2test_store_key_id(argv[2], &nv_key_id);
  if (ret != ERR_OK) {
    goto failure;
  }

  if (is_read == lwIP_TRUE) {
    ret = rpl_pstore_read(nv_key_id, &value, sizeof(value));
    if (ret == ERR_OK) {
      PRINTK("read %s %hhu"CRLF, argv[2], value);
    }
  } else {
    value = (u8_t)atoi(argv[3]);
    ret = rpl_pstore_write(nv_key_id, &value, sizeof(value));
    if (ret == ERR_OK) {
      PRINTK("write %s %hhu"CRLF, argv[2], value);
    }
  }

  return ret;
failure:
  return ERR_ARG;
}

static void
os_shell_l2test_internal(void *arg)
{
  err_t ret = ERR_VAL;
  struct netif *netif = NULL;
  u8_t prio;
  uniqid_t id;
  shell_cmd_t *l2test_cmd = (shell_cmd_t *)arg;
  int argc = l2test_cmd->argc;
  char **argv = l2test_cmd->argv;

  if (argc < 1) {
    os_shell_l2test_usage();
    sys_sem_signal(&l2test_cmd->cb_completed);
    return;
  }
  if ((argc >= 3) && (strcmp(argv[0], "store") == ERR_OK)) {
    ret = l2test_store(argc, argv);
    goto funcRet;
  }
  netif = netif_find(argv[0]);
  if (netif == NULL) {
    PRINTK("not find %s"CRLF, argv[0]);
    sys_sem_signal(&l2test_cmd->cb_completed);
    return;
  }
  if ((argc == 3) && (strcmp(argv[1], "remove") == ERR_OK)) {
    ret = l2test_remove_peer(netif, argv[2]);
  } else if ((argc == 3) && (strcmp(argv[1], "prio") == ERR_OK)) {
    /* value of priority is 0~255 */
    if ((atoi(argv[2]) < 0) || (atoi(argv[2]) > 255)) {
      PRINTK("invalid prio"CRLF);
      ret = -1;
      goto funcRet;
    }
    prio = (u8_t)(atoi(argv[2]));
    ret = netif_set_beacon_prio(netif, prio);
  } else if ((argc == 3) && (strcmp(argv[1], "mnid") == ERR_OK)) {
    /* value of mnid is 0~127 */
    if ((atoi(argv[2]) < 0) || (atoi(argv[2]) > 127)) {
      PRINTK("invalid mnid"CRLF);
      ret = -1;
      goto funcRet;
    }
    id = (uniqid_t)(atoi(argv[2]));
    ret = netif_set_unique_id(netif, id);
  } else if ((argc == 2) && (strcmp(argv[1], "all") == ERR_OK)) {
    ret = l2test_callback_status(netif);
  } else if ((argc >= 3) && (strcmp(argv[1], "pbuf") == ERR_OK)) {
    if (strcmp(argv[2], "start") == ERR_OK) {
      ret = l2test_pbuf_start(netif);
    } else if (strcmp(argv[2], "stop") == ERR_OK) {
      ret = l2test_pbuf_stop();
    } else if ((argc >= 5) && (strcmp(argv[2], "send") == ERR_OK)) {
      ret = l2test_pbuf_send(netif, argc, argv);
    } else {
      os_shell_l2test_usage();
    }
  } else {
    os_shell_l2test_usage();
  }

funcRet:
  PRINTK("ret %d"CRLF, ret);

  sys_sem_signal(&l2test_cmd->cb_completed);
  return;
}

u32_t
os_shell_l2test(int argc, char **argv)
{
  shell_cmd_t l2test_cmd = {0};
  err_t ret;
  if (argv == NULL) {
    return LOS_NOK;
  }
  if (sys_sem_new(&l2test_cmd.cb_completed, 0) != ERR_OK) {
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("%s: sys_sem_new fail"CRLF, __FUNCTION__);
#else
    PRINTK("%s: sys_sem_new fail"CRLF, __FUNCTION__);
#endif
    return LOS_NOK;
  }

  l2test_cmd.argc = argc;
  l2test_cmd.argv = argv;

  ret = tcpip_callback(os_shell_l2test_internal, &l2test_cmd);
  if (ret != ERR_OK) {
    sys_sem_free(&l2test_cmd.cb_completed);
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("l2test : internal error, ret:%d"CRLF, ret);
#else
    PRINTK("l2test : internal error, ret:%d"CRLF, ret);
#endif
    return LOS_NOK;
  }
  (void)sys_arch_sem_wait(&l2test_cmd.cb_completed, 0);
  sys_sem_free(&l2test_cmd.cb_completed);

  return LOS_OK;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(l2test_shellcmd, CMD_TYPE_EX, "l2test", XARGS, (CmdCallBackFunc)os_shell_l2test);
#endif /* LOSCFG_SHELL */
#endif /* LWIP_RIPPLE */

#endif /* LWIP_ENABLE_MESH_SHELL_CMD */

#endif /* LWIP_ENABLE_BASIC_SHELL_CMD */

#if LWIP_ENABLE_LOS_SHELL_CMD
static void
lwip_ifstats_usage(const char *cmd)
{
  PRINTK("Usage:"\
         CRLF"%s [-p [IPV6 | ICMPV6 | ND6 | 6LOWPAN | TCP |UDP]]"CRLF, cmd);
}

#define PRINT_STAT(stats_info) \
  PRINTK("packets sent:%u \t recvd:%u \t drop:%u \t"CRLF,stats_info->xmit, \
          stats_info->recv, stats_info->drop);

#if IP6_STATS
#define PRINT_IPV6_STAT() do { \
  stats_info = &lwip_stats.ip6; \
  PRINTK("IPv6 "); \
  PRINT_STAT(stats_info) \
} while (0)
#else
#define PRINT_IPV6_STAT()
#endif

#if ICMP6_STATS
#define PRINT_ICMPV6_STAT()  lwip_print_icmpv6_stat()
#else
#define PRINT_ICMPV6_STAT()
#endif

#if ND6_STATS
#define PRINT_ND6_STAT() do { \
  stats_info = &lwip_stats.nd6; \
  PRINTK("ND6 "); \
  PRINT_STAT(stats_info) \
} while (0)
#else
#define PRINT_ND6_STAT()
#endif

#if TCP_STATS
#define PRINT_TCP_STAT() do { \
  stats_info = &lwip_stats.tcp; \
  PRINTK("TCP "); \
  PRINT_STAT(stats_info) \
} while (0)
#else
#define PRINT_TCP_STAT()
#endif

#if UDP_STATS
#define PRINT_UDP_STAT() do { \
  stats_info = &lwip_stats.udp; \
  PRINTK("UDP "); \
  PRINT_STAT(stats_info) \
} while (0)
#else
#define PRINT_UDP_STAT()
#endif

#if LOWPAN6_STATS
#define PRINT_6LOWPAN_STAT() do { \
  struct stats_lowpan6 *stats = &lwip_stats.lowpan6; \
  PRINTK("6LOWPAN "); \
  PRINTK("pkt_xmit_succ:%u \t xmit_pkt_drop:%u " \
         "\t pkt_decomp_succ:%u \t recv_pkt_drop:%u \t"CRLF, \
         stats->pkt_xmit,  (stats->pkt_from_ip - stats->pkt_xmit), \
         stats->pkt_to_ip, (stats->pkt_recvd - stats->pkt_to_ip)); \
} while (0)
#else
#define PRINT_6LOWPAN_STAT()
#endif

#if ICMP6_STATS
void
lwip_print_icmpv6_stat(void)
{
  struct stats_proto stats_info = lwip_stats.icmp6;
#if ND6_STATS
  stats_info.xmit = (STAT_COUNTER)(stats_info.xmit + lwip_stats.nd6.xmit);
#endif
#if LWIP_IPV6_MLD
  stats_info.xmit = (STAT_COUNTER)(stats_info.xmit + lwip_stats.mld6.xmit);
#endif

  PRINTK("ICMPv6 ");
  PRINT_STAT((&stats_info))
}
#endif

u32_t
lwip_ifstats(int argc, char **argv)
{
  if (!tcpip_init_finish) {
    PRINTK("%s: tcpip_init have not been called"CRLF, __FUNCTION__);
    return LOS_NOK;
  }

  if (argc) {
    if ((argc < 2) || !argv) {
      goto SHOW_RETURN;
    }

    if (argv[0] && argv[1] && strcasecmp("-p", argv[0]) == 0) {
#if IP6_STATS || ICMP6_STATS || TCP_STATS || UDP_STATS
      struct stats_proto *stats_info = NULL;
#endif
      if (strcasecmp("IPV6", argv[1]) == 0) {
#if IP6_STATS
        PRINT_IPV6_STAT();
#endif
      } else if (strcasecmp("ICMPV6", argv[1]) == 0) {
        PRINT_ICMPV6_STAT();
      } else if (strcasecmp("ND6", argv[1]) == 0) {
        PRINT_ND6_STAT();
      } else if (strcasecmp("TCP", argv[1]) == 0) {
        PRINT_TCP_STAT();
      } else if (strcasecmp("UDP", argv[1]) == 0) {
        PRINT_UDP_STAT();
      } else if (strcasecmp("6LOWPAN", argv[1]) == 0) {
        PRINT_6LOWPAN_STAT();
      } else {
        goto SHOW_RETURN;
      }
    } else {
      goto SHOW_RETURN;
    }
  } else {
#if IP6_STATS || ICMP6_STATS || TCP_STATS || UDP_STATS
    struct stats_proto *stats_info = NULL;
#endif
    /* Show Statistics for IPv6, ICMPv6, ND6, 6LOWPAN, UDP and TCP */
    PRINT_IPV6_STAT();
    PRINT_ICMPV6_STAT();
    PRINT_ND6_STAT();
    PRINT_TCP_STAT();
    PRINT_UDP_STAT();
    PRINT_6LOWPAN_STAT();
  }

  return 0;

SHOW_RETURN:
  lwip_ifstats_usage("ifstats");
  return 1;
}

/* add arp entry to arp cache */
#define ARP_OPTION_ADD      1
/* delete arp entry to arp cache */
#define ARP_OPTION_DEL      2
/* print all arp entry in arp cache */
#define ARP_OPTION_SHOW     3

struct arp_option {
  /* see the ARP_OPTION_ above */
  int             option;
  /* descriptive abbreviation of network interface */
  char            iface[IFNAMSIZ];
  /* ip addr */
  unsigned int    ipaddr;
  /* hw addr */
  unsigned char ethaddr[ETH_HWADDR_LEN];
  /* when using telnet, printf to the telnet socket will result in system  */
  /* deadlock.so don't do it.cahe the data to prinf to a buf, and when     */
  /* callback returns, then printf the data out to the telnet socket       */
  sys_sem_t       cb_completed;
  char            cb_print_buf[PRINT_BUF_LEN];
  int             print_buf_len;
};

#ifndef LWIP_TESTBED
LWIP_STATIC
#endif
void
lwip_arp_show_internal(struct netif *netif, char *printf_buf, unsigned int buf_len)
{
  u8_t state, i, num;
  int ret;
  char *name = NULL;
  char *tmp = printf_buf;
  if (buf_len <= 1) {
    return;
  }
  ret = snprintf_s(tmp, buf_len, (buf_len - 1), "%-24s%-24s%-12s%-12s"CRLF, "Address", "HWaddress", "Iface", "Type");
  if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
    return;
  }
  tmp += ret;
  buf_len -= (unsigned int)ret;

  for (i = 0; i < ARP_TABLE_SIZE; ++i) {
    state = arp_table[i].state;
    if (((state == ETHARP_STATE_STABLE)
#if ETHARP_SUPPORT_STATIC_ENTRIES
         || (state == ETHARP_STATE_STATIC)
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
        ) && arp_table[i].netif) {
      name = arp_table[i].netif->name;
      num = arp_table[i].netif->num;
      if ((netif != NULL) && ((strncmp(name, netif->name, IFNAMSIZ) != 0) || (num != netif->num))) {
        continue;
      }

      ret = snprintf_s(tmp, buf_len, (buf_len - 1),
                       "%-24s%02X:%02X:%02X:%02X:%02X:%02X       %s%u        %s"CRLF,
                       ip4addr_ntoa(&arp_table[i].ipaddr),
                       arp_table[i].ethaddr.addr[0], arp_table[i].ethaddr.addr[1],
                       arp_table[i].ethaddr.addr[2], arp_table[i].ethaddr.addr[3],
                       arp_table[i].ethaddr.addr[4], arp_table[i].ethaddr.addr[5],
                       name, num,
#if ETHARP_SUPPORT_STATIC_ENTRIES
                       ((state == ETHARP_STATE_STATIC) ? "static" : "dynamic")
#else
                       "dynamic"
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
                      );
      if ((ret <= 0) || ((unsigned int)ret >= buf_len)) {
        return;
      }
      tmp += ret;
      buf_len -= (unsigned int)ret;
    }
  }
}

static int
lwip_arp_add_internal(struct netif *netif, struct arp_option *arp_cmd)
{
  err_t ret = 0;
  struct eth_addr ethaddr;
  ip4_addr_t ipaddr;
  ipaddr.addr = arp_cmd->ipaddr;
  (void)memcpy_s(ethaddr.addr, sizeof(ethaddr.addr), arp_cmd->ethaddr, sizeof(ethaddr.addr));
  if (netif != NULL) {
    /* If  in the same subnet */
    if (ip4_addr_netcmp(&ipaddr, ip_2_ip4(&(netif->ip_addr)), ip_2_ip4(&(netif->netmask)))) {
      ret = etharp_update_arp_entry(netif, &ipaddr, &ethaddr, ETHARP_FLAG_TRY_HARD);
    } else {
      (void)snprintf_s(arp_cmd->cb_print_buf, PRINT_BUF_LEN, (PRINT_BUF_LEN - 1),
                       "Network is unreachable"CRLF);
      sys_sem_signal(&arp_cmd->cb_completed);
      return ERR_NETUNREACH;
    }
  } else {
    for (netif = netif_list; netif != NULL; netif = netif->next) {
      /* If  in the same subnet */
      if (ip4_addr_netcmp(&ipaddr, ip_2_ip4(&(netif->ip_addr)), ip_2_ip4(&(netif->netmask)))) {
        ret = etharp_update_arp_entry(netif, &ipaddr, &ethaddr, ETHARP_FLAG_TRY_HARD);
        if (ret == ERR_OK) {
          /* only can add success one time */
          break;
        }
      }
      /* The netif is last netif and cannot add this arp entry on any netif */
      if (netif->next == NULL) {
        (void)snprintf_s(arp_cmd->cb_print_buf, PRINT_BUF_LEN, (PRINT_BUF_LEN - 1),
                         "Network is unreachable"CRLF);
        sys_sem_signal(&arp_cmd->cb_completed);
        return ERR_NETUNREACH;
      }
    }
  }
  return ret;
}

static int
lwip_arp_del_internal(struct netif *netif, struct arp_option *arp_cmd)
{
  err_t ret = 0;
  struct eth_addr ethaddr;
  ip4_addr_t ipaddr;
  ipaddr.addr = arp_cmd->ipaddr;
  (void)memcpy_s(ethaddr.addr, sizeof(ethaddr.addr), arp_cmd->ethaddr, sizeof(ethaddr.addr));
  if (netif != NULL) {
    ret = etharp_delete_arp_entry(netif, &ipaddr);
  } else {
    for (netif = netif_list; netif != NULL; netif = netif->next) {
      ret = etharp_delete_arp_entry(netif, &ipaddr);
      if (ret == ERR_OK) {
        /* only can del success one time */
        break;
      }
    }
  }
  return ret;
}

#ifndef LWIP_TESTBED
LWIP_STATIC
#endif
void
lwip_arp_internal(void *arg)
{
#if LWIP_IPV4
  struct arp_option *arp_cmd = (struct arp_option *)arg;
  struct netif *netif = NULL;
  err_t ret = 0;
  int type = 0;

  if (arp_cmd->iface[0] == 'd' && arp_cmd->iface[1] == 'e') {
    netif = NULL;
  } else {
    /* find the specified netif by it's name */
    netif = netif_find(arp_cmd->iface);
    if (netif == NULL) {
      (void)snprintf_s(arp_cmd->cb_print_buf, PRINT_BUF_LEN, (PRINT_BUF_LEN - 1), "No such device"CRLF);
      goto out;
    }
  }

  type = arp_cmd->option;
  switch (type) {
    case ARP_OPTION_SHOW:
      lwip_arp_show_internal(netif, arp_cmd->cb_print_buf, PRINT_BUF_LEN);
      break;

    case ARP_OPTION_ADD:
      ret = lwip_arp_add_internal(netif, arp_cmd);
      break;

    case ARP_OPTION_DEL:
      ret = lwip_arp_del_internal(netif, arp_cmd);
      break;

    default:
      (void)snprintf_s(arp_cmd->cb_print_buf, PRINT_BUF_LEN, (PRINT_BUF_LEN - 1), "Error"CRLF);
      goto out;
  }

out:
  if (type == ARP_OPTION_ADD || type == ARP_OPTION_DEL) {
    if (ret == ERR_NETUNREACH) {
      return;
    } else if (ret == ERR_MEM) {
      (void)snprintf_s(arp_cmd->cb_print_buf, PRINT_BUF_LEN, (PRINT_BUF_LEN - 1), "Out of memory error"CRLF);
    } else if (ret == ERR_ARG) {
      (void)snprintf_s(arp_cmd->cb_print_buf, PRINT_BUF_LEN, (PRINT_BUF_LEN - 1), "Illegal argument"CRLF);
    } else {
      (void)snprintf_s(arp_cmd->cb_print_buf, PRINT_BUF_LEN, (PRINT_BUF_LEN - 1), "Successed"CRLF);
    }
  }
#endif

  sys_sem_signal(&arp_cmd->cb_completed);
}

LWIP_STATIC void
lwip_arp_usage(const char *cmd)
{
  PRINTK("Usage:"\
         CRLF"%s"
         CRLF"%s [-i IF] -s IPADDR HWADDR"\
         CRLF"%s [-i IF] -d IPADDR"CRLF,
         cmd, cmd, cmd);
}

static void
arp_cmd_init(struct arp_option *arp_cmd_p)
{
  (void)memset_s(arp_cmd_p, sizeof(struct arp_option), 0, sizeof(struct arp_option));
  arp_cmd_p->iface[0] = 'd';
  arp_cmd_p->iface[1] = 'e';
  arp_cmd_p->iface[2] = '0';
  arp_cmd_p->option = ARP_OPTION_SHOW;
  arp_cmd_p->print_buf_len = 0;
}

static int
arp_parse_get(const char *ifname, const int iface_len, struct arp_option *arp_cmd_p)
{
  if (iface_len >= IFNAMSIZ) {
    PRINTK("Iface name is big "CRLF);
    return LOS_NOK;
  }
  if (strncmp(ifname, "lo", (sizeof("lo") - 1)) == 0) {
    PRINTK("Illegal operation\n");
    return LOS_NOK;
  }
  (void)strncpy_s(arp_cmd_p->iface, IFNAMSIZ, ifname, iface_len);
  arp_cmd_p->iface[iface_len] = '\0';
  return LOS_OK;
}

static int
arp_parse_add(const unsigned int ipaddr, const unsigned char *macaddr, struct arp_option *arp_cmd_p)
{
  /* arp add */
  char *digit = NULL;
  u32_t macaddrlen = strlen(macaddr) + 1;
  char tmp_str[MAX_MACADDR_STRING_LENGTH];
  char *tmp_str1 = NULL;
  char *saveptr1 = NULL;
  char *temp = NULL;
  int j;

  arp_cmd_p->option = ARP_OPTION_ADD;
  arp_cmd_p->ipaddr = ipaddr;

  if (arp_cmd_p->ipaddr == IPADDR_NONE) {
    PRINTK("IP address is not correct!"CRLF);
    return LOS_NOK;
  }

  /* cannot add an arp entry of 127.*.*.* */
  if ((arp_cmd_p->ipaddr & (u32_t)0x0000007fUL) == (u32_t)0x0000007fUL) {
    PRINTK("IP address is not correct!"CRLF);
    return LOS_NOK;
  }

  if (macaddrlen != MAX_MACADDR_STRING_LENGTH) {
    PRINTK("Wrong MAC address length"CRLF);
    return LOS_NOK;
  }

  if (strncpy_s(tmp_str, MAX_MACADDR_STRING_LENGTH, macaddr, macaddrlen - 1) != EOK) {
    PRINTK("Wrong MAC address"CRLF);
    return LOS_NOK;
  }
  /* 6 : the : index in mac address */
  for (j = 0, tmp_str1 = tmp_str; j < 6; j++, tmp_str1 = NULL) {
    digit = strtok_r(tmp_str1, ":", &saveptr1);
    if ((digit == NULL) || (strlen(digit) > 2)) { /* 2 : Addresses are classify in two hexadecimal */
      PRINTK("MAC address is not correct"CRLF);
      return LOS_NOK;
    }

    for (temp = digit; *temp != '\0'; temp++) {
      if (!isxdigit(*temp)) {
        PRINTK("MAC address is not correct"CRLF);
        return LOS_NOK;
      }
    }

    CONVERT_STRING_TO_HEX(digit, arp_cmd_p->ethaddr[j]);
  }
  return LOS_OK;
}

static int
arp_cmd_exec_callback(struct arp_option *arp_cmd_p)
{
  err_t ret;
  if (sys_sem_new(&arp_cmd_p->cb_completed, 0) != ERR_OK) {
    PRINTK("%s: sys_sem_new fail\n", __FUNCTION__);
    return LOS_NOK;
  }

  if ((ret = tcpip_callback(lwip_arp_internal, arp_cmd_p)) != ERR_OK) {
    PRINTK("%s : tcpip_callback failed in line %d : errnu %d", __FUNCTION__, __LINE__, ret);
    sys_sem_free(&arp_cmd_p->cb_completed);
    return LOS_NOK;
  }
  (void)sys_arch_sem_wait(&arp_cmd_p->cb_completed, 0);
  sys_sem_free(&arp_cmd_p->cb_completed);
  arp_cmd_p->cb_print_buf[PRINT_BUF_LEN - 1] = '\0';
  PRINTK("%s", arp_cmd_p->cb_print_buf);
  return LOS_OK;
}

u32_t
lwip_arp(int argc, char **argv)
{
  int i = 0;
  struct arp_option arp_cmd;
  err_t ret;
  char ifname[IFNAMSIZ] = {0};
  int inf_len;
  unsigned int ipaddr;
  unsigned char macaddr[MACADDR_BUF_LEN] = {0};

  arp_cmd_init(&arp_cmd);
  if (tcpip_init_finish == 0) {
    PRINTK("%s: tcpip_init have not been called"CRLF, __FUNCTION__);
    return LOS_NOK;
  }

  while (argc > 0) {
    if (strcmp("-i", argv[i]) == 0 && (argc > 1)) {
      inf_len = strlen(argv[i + 1]);
      (void)strncpy_s(ifname, inf_len, argv[i + 1], inf_len);
      ret = arp_parse_get(ifname, inf_len, &arp_cmd);
      if (ret == LOS_NOK) {
        goto arp_error;
      }

      i += 2;
      argc -= 2;
    } else if (strcmp("-d", argv[i]) == 0 && (argc > 1)) {
      /* arp delete */
      arp_cmd.option = ARP_OPTION_DEL;
      arp_cmd.ipaddr = inet_addr(argv[i + 1]);

      if (arp_cmd.ipaddr == IPADDR_NONE) {
        PRINTK("IP address is not correct!"CRLF);
        goto arp_error;
      }

      i += 2;
      argc -= 2;
    } else if (strcmp("-s", argv[i]) == 0 && (argc > 2)) {
      ipaddr = inet_addr(argv[i + 1]);
      (void)strncpy_s(macaddr, strlen(argv[i + 2]), argv[i + 2], strlen(argv[i + 2]));
      ret = arp_parse_add(ipaddr, macaddr, &arp_cmd);
      if (ret == LOS_NOK) {
        goto arp_error;
      }

      i += 3;
      argc -= 3;
    } else {
      goto arp_error;
    }
  }

  ret = arp_cmd_exec_callback(&arp_cmd);
  return ret;

arp_error:
  lwip_arp_usage("arp");
  return LOS_NOK;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(arp_shellcmd, CMD_TYPE_EX, "arp", 1, (CmdCallBackFunc)lwip_arp);
#endif /* LOSCFG_SHELL */

void
ifup_internal(void *arg)
{
  struct netif *netif = NULL;
  struct if_cmd_data *ifcmd_data = NULL;

  ifcmd_data = (struct if_cmd_data *)arg;
  netif = netif_find(ifcmd_data->if_name);
  if (netif == NULL) {
    ifcmd_data->err = ERR_VAL;
  } else {
    (void)netif_set_up(netif);
    ifcmd_data->err = ERR_OK;
  }

  sys_sem_signal(&ifcmd_data->cb_completed);
  return;
}

void
ifdown_internal(void *arg)
{
  struct netif *netif = NULL;
  struct if_cmd_data *ifcmd_data = NULL;

  ifcmd_data = (struct if_cmd_data *)arg;
  netif = netif_find(ifcmd_data->if_name);
  if (netif == NULL) {
    ifcmd_data->err = ERR_VAL;
  } else {
    (void)netif_set_down(netif);
    ifcmd_data->err = ERR_OK;
  }

  sys_sem_signal(&ifcmd_data->cb_completed);
  return;
}

#if LWIP_SNTP
u32_t
os_shell_ntpdate(int argc, char **argv)
{
  int server_num;
  char *ret = NULL;
  struct timeval get_time;

  (void)memset_s(&get_time, sizeof(struct timeval), 0, sizeof(struct timeval));

  if (tcpip_init_finish == 0) {
    PRINTK("%s: tcpip_init have not been called"CRLF, __FUNCTION__);
    return LOS_NOK;
  }
  if (argv == NULL) {
    return LOS_NOK;
  }
  if (argc < 1) {
    goto usage;
  }

  server_num = lwip_sntp_start(argc, argv, &get_time);
  if (server_num >= 0 && server_num < argc) {
    ret = ctime((time_t *)&get_time.tv_sec);
    if (ret) {
      PRINTK("time server %s: %s"CRLF, argv[server_num], ret);
    } else {
      PRINTK("ctime return null error"CRLF);
    }
  } else {
    PRINTK("no server suitable for synchronization found"CRLF);
  }

  return LOS_OK;

usage:
  PRINTK("\nUsage:\n");
  PRINTK("ntpdate [SERVER_IP1] [SERVER_IP2] ..."CRLF);
  return LOS_NOK;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(ntpdate_shellcmd, CMD_TYPE_EX, "ntpdate",  XARGS, (CmdCallBackFunc)os_shell_ntpdate);
#endif /* LOSCFG_SHELL */

#endif /* LWIP_SNTP */

#ifdef LOSCFG_NET_LWIP_SACK_TFTP
static char *tftp_error[] = {
  "TFTP transfer finish"CRLF,
  "Error while creating UDP socket"CRLF,
  "Error while binding to the UDP socket"CRLF,
  "Error returned by lwip_select() system call"CRLF,
  "Error while receiving data from the peer"CRLF,
  "Error while sending data to the peer"CRLF,
  "Requested file is not found"CRLF,
  "This is the error sent by the server when hostname cannot be resolved"CRLF,
  "Input paramters passed to TFTP interfaces are invalid"CRLF,
  "Error detected in TFTP packet or the error received from the TFTP server"CRLF,
  "Error during packet synhronization while sending or unexpected packet is received"CRLF,
  "File size limit crossed, Max block can be 0xFFFF, each block containing 512 bytes"CRLF,
  "File name lenght greater than 256"CRLF,
  "Hostname IP is not valid"CRLF,
  "TFTP server returned file access error"CRLF,
  "TFTP server returned error signifying that the DISK is full to write"CRLF,
  "TFTP server returned error signifying that the file exist"CRLF,
  "The source file name do not exisits"CRLF,
  "Memory allocaion failed in TFTP client"CRLF,
  "File open failed"CRLF,
  "File read error"CRLF,
  "File create error"CRLF,
  "File write error"CRLF,
  "Max time expired while waiting for file to be recived"CRLF,
  "Error when the received packet is less than 4bytes(error lenght) or greater than 512bytes"CRLF,
  "Returned by TFTP server for protocol user error"CRLF,
  "The destination file path length greater than 256"CRLF,
  "Returned by TFTP server for undefined transfer ID"CRLF,
  "IOCTL fucntion failed at TFTP client while setting the socket to non-block"CRLF,
};

#if LWIP_TFTP
static u32_t
tftp_invoke_operation(u8_t uc_tftp_get, u32_t ul_remote_addr, s8_t *sz_local_file_name, s8_t *sz_remote_file_name)
{
  u16_t us_tftp_serv_port = 69;
  u32_t ret;
  if (sz_local_file_name == NULL || sz_remote_file_name == NULL) {
    return LOS_NOK;
  }

  if (uc_tftp_get) {
    ret = lwip_tftp_get_file_by_filename(ntohl(ul_remote_addr), us_tftp_serv_port,
                                         TRANSFER_MODE_BINARY, sz_remote_file_name, sz_local_file_name);
  } else {
    ret = lwip_tftp_put_file_by_filename(ntohl(ul_remote_addr), us_tftp_serv_port,
                                         TRANSFER_MODE_BINARY, sz_local_file_name, sz_remote_file_name);
  }

  LWIP_ASSERT("TFTP UNKNOW ERROR!", ret < LWIP_ARRAYSIZE(tftp_error));
  PRINTK("%s", tftp_error[ret]);
  if (ret) {
    return LOS_NOK;
  } else {
    return LOS_OK;
  }
}

u32_t
os_shell_tftp(int argc, char **argv)
{
  /* log off temporary for upgrade lwip to 2.0 */
  u32_t  ul_remote_addr = IPADDR_NONE;
  u8_t   uc_tftp_get = 0;
  s8_t   *sz_local_file_name = NULL;
  s8_t   *sz_remote_file_name = NULL;
  u32_t  ret;
  int i = 0;
  if (argv == NULL) {
    return LOS_NOK;
  }
  if (!tcpip_init_finish) {
    PRINTK("%s: tcpip_init have not been called"CRLF, __FUNCTION__);
    return LOS_NOK;
  }

  while (i < argc) {
    if (strcmp(argv[i], "-p") == 0) {
      uc_tftp_get = 0;
      i++;
      continue;
    }

    if (strcmp(argv[i], "-g") == 0) {
      uc_tftp_get = 1;
      i++;
      continue;
    }

    if (strcmp(argv[i], "-l") == 0 && ((i + 1) < argc)) {
      sz_local_file_name = (s8_t *)argv[i + 1];
      i += 2;
      continue;
    }

    if (strcmp(argv[i], "-r") == 0 && ((i + 1) < argc)) {
      sz_remote_file_name = (s8_t *)argv[i + 1];
      i += 2;
      continue;
    }

    if ((i + 1) == argc) {
      ul_remote_addr = inet_addr(argv[i]);
      break;
    }

    goto usage;
  }

  if (ul_remote_addr == IPADDR_NONE || sz_local_file_name == NULL || sz_remote_file_name == NULL) {
    goto usage;
  }

  return tftp_invoke_operation(uc_tftp_get, ul_remote_addr, sz_local_file_name, sz_remote_file_name);

usage:
  PRINTK("usage:\nTransfer a file from/to tftp server"CRLF);
  PRINTK("tftp <-g/-p> -l FullPathLocalFile -r RemoteFile Host"CRLF);
  return LOS_NOK;
}
#endif /* LWIP_TFTP */

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(tftp_shellcmd, CMD_TYPE_EX, "tftp", XARGS, (CmdCallBackFunc)os_shell_tftp);
#endif /* LOSCFG_SHELL */
#endif /* LOSCFG_NET_LWIP_SACK_TFTP */

#define MAX_SIZE 1024
void
tcp_access(int sockfd)
{
  size_t n, i;
  ssize_t ret;
  char msg[MAX_SIZE] = {0};
  while (1) {
    PRINTK("waiting for recv"CRLF);
    (void)memset_s(msg, MAX_SIZE, 0, MAX_SIZE);
    ret = recv(sockfd, msg, MAX_SIZE - 1, 0);
    if (ret < 0) {
      PRINTK("recv failed, %d."CRLF, (u32_t)ret);
      (void)closesocket(sockfd);
      return;
    } else if (ret == 0) {
      (void)closesocket(sockfd);
      PRINTK("client disconnect."CRLF);
      return;
    }

    n = strlen(msg);
    for (i = 0; i < n; ++i) {
      if ((msg[i] >= 'a') && (msg[i] <= 'z')) {
        msg[i] = (char)(msg[i] + ('A' - 'a'));
      } else if ((msg[i] >= 'A') && (msg[i] <= 'Z')) {
        msg[i] = (char)(msg[i] + ('a' - 'A'));
      }
    }

    if (send(sockfd, msg, n, 0) < 0) {
      PRINTK("send failed!"CRLF);
      continue;
    }
  }
}

#ifdef LWIP_DEBUG_TCPSERVER
u32_t
os_tcpserver(int argc, char **argv)
{
  uint16_t port;
  int sockfd;
  int ret;
  struct sockaddr_in seraddr;
  struct sockaddr_in cliaddr;
  u32_t cliaddr_size = (u32_t)sizeof(cliaddr);
  int reuse, i_port_val;

  if (tcpip_init_finish == 0) {
    PRINTK("tcpip_init have not been called"CRLF);
    return LOS_NOK;
  }

  if (argc < 1) {
    PRINTK(CRLF"Usage: tcpserver <port>"CRLF);
    return LOS_NOK;
  }

  i_port_val = atoi(argv[0]);
  /* Port 0 not supported , negative values not supported , max port limit is 65535 */
  if (i_port_val <= 0 || i_port_val > 65535) {
    PRINTK(CRLF"Usage: Invalid port"CRLF);
    return LOS_NOK;
  }

  port = (uint16_t)i_port_val;

  /*
   * removed the print of argv[1] as its accessing argv[1] without verifying argc and argv[1] not used anywhere else
   */
  PRINTK("argv[0]:%s, argc:%d, port:%d"CRLF, argv[0], argc, port);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  reuse = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                 (const char *)&reuse, sizeof(reuse)) != 0) {
    (void)closesocket(sockfd);
    PRINTK("set SO_REUSEADDR failed"CRLF);
    return LOS_NOK;
  }

  (void)memset_s(&seraddr, sizeof(seraddr), 0, sizeof(seraddr));
  seraddr.sin_family = AF_INET;
  seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  seraddr.sin_port = htons(port);

  ret = bind(sockfd, (struct sockaddr *)&seraddr, sizeof(seraddr));
  if (ret < 0) {
    PRINTK("bind ip and port failed");
    (void)closesocket(sockfd);
    return LOS_NOK;
  }

  ret = listen(sockfd, 5); /* 5 : max tcp listen backlog queue */
  if (ret < 0) {
    (void)closesocket(sockfd);
    PRINTK("listen failed"CRLF);
    return LOS_NOK;
  }
  while (1) {
    PRINTK("waiting for accept"CRLF);
    (void)memset_s(&cliaddr, sizeof(struct sockaddr_in), 0, sizeof(struct sockaddr_in));
    ret = (int)accept(sockfd, (struct sockaddr *)&cliaddr, &cliaddr_size);
    if (ret < 0) {
      (void)closesocket(sockfd);
      PRINTK("Accept failed, %d"CRLF, ret);
      break ;
    }
    tcp_access(ret);
  }
  return LOS_NOK;            // Hits Only If Accept Fails
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(tcpserver_shellcmd, CMD_TYPE_EX, "tcpserver",  XARGS, (CmdCallBackFunc)os_tcpserver);
#endif /* LOSCFG_SHELL */
#endif /* LWIP_DEBUG_TCPSERVER */

#ifdef LWIP_DEBUG_UDPSERVER
void
udpserver(int argc, char **argv)
{
  int sockfd, fromlen;
  int ret, i_port_val;
  struct sockaddr_in seraddr;
  struct sockaddr_in cliaddr;
  size_t n, i;

  char msg[MAX_SIZE];
  uint16_t port;
  if (argv == NULL) {
    return;
  }
  if (argc < 1) {
    PRINTK(CRLF"Usage: udpserver <port>"CRLF);
    return;
  }

  i_port_val = atoi(argv[0]);
  /* Port 0 not supported , negative values not supported , max port limit is 65535 */
  if ((i_port_val <= 0) || (i_port_val > 65535)) {
    PRINTK(CRLF"Usage: Invalid Port"CRLF);
    return;
  }

  port = (uint16_t)i_port_val;

  PRINTK("port:%d"CRLF, port);

  sockfd = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  (void)memset_s(&seraddr, sizeof(seraddr), 0, sizeof(seraddr));
  (void)memset_s(&cliaddr, sizeof(cliaddr), 0, sizeof(cliaddr));
  seraddr.sin_family = AF_INET;
  seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  seraddr.sin_port = htons(port);
  ret = lwip_bind(sockfd, (struct sockaddr *)&seraddr, sizeof(seraddr));
  if (ret < 0) {
    PRINTK("bind ip and port failed:%d"CRLF, errno);
    (void)closesocket(sockfd);
    return;
  }

  while (1) {
    ret = recvfrom(sockfd, msg, MAX_SIZE - 1, 0, (struct sockaddr *)&cliaddr, (socklen_t *)&fromlen);
    if (ret < 0) {
      if ((errno == EAGAIN) || (errno == EINTR)) {
        continue;
      } else {
        break;
      }
    }
    n = strlen(msg);
    for (i = 0; i < n; ++i) {
      if (msg[i] >= 'a' && msg[i] <= 'z') {
        msg[i] = (char)(msg[i] + 'A' - 'a');
      } else if (msg[i] >= 'A' && msg[i] <= 'Z') {
        msg[i] = (char)(msg[i] + 'a' - 'A');
      }
    }
    ret = sendto(sockfd, msg, n + 1, 0, (struct sockaddr *)&cliaddr, (socklen_t)fromlen);
    if ((ret <= 0) && (errno == EPIPE)) {
      break;
    }
  }

  (void)closesocket(sockfd);
  return;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(udpserver_shellcmd, CMD_TYPE_EX, "udpserver",  XARGS, (CmdCallBackFunc)udpserver);
#endif /* LOSCFG_SHELL */
#endif /* LWIP_DEBUG_UDPSERVER */

#ifdef LWIP_DEBUG_INFO
LWIP_STATIC
u32_t
netdebug_memp(int argc, char **argv)
{
  u32_t ret = LOS_OK;
  int type;

  if (argc == 2) {
    if (!strcmp("-i", argv[1])) {
      debug_memp_info();
    } else if (!strcmp("-udp", argv[1])) {
      debug_memp_type_info(MEMP_UDP_PCB);
    } else if (!strcmp("-tcp", argv[1])) {
      debug_memp_type_info(MEMP_TCP_PCB);
    } else if (!strcmp("-raw", argv[1])) {
      debug_memp_type_info(MEMP_RAW_PCB);
    } else if (!strcmp("-conn", argv[1])) {
      debug_memp_type_info(MEMP_NETCONN);
    } else {
      ret = LOS_NOK;
    }
  } else if (argc == 3) {
    if (!strcmp("-d", argv[1])) {
      type = atoi(argv[2]);
      if (type >= 0) {
        debug_memp_detail(type);
      } else {
        PRINTK("Error: type < 0"CRLF);
        ret = LOS_NOK;
      }
    } else {
      ret = LOS_NOK;
    }
  } else {
    ret = LOS_NOK;
  }

  return ret;
}

LWIP_STATIC
u32_t
netdebug_sock(int argc, char **argv)
{
  int idx;
  u32_t ret = LOS_NOK;

  if (argc == 2) {
    if (strcmp("-i", argv[1]) == 0) {
      /* netdebug sock -i */
      for (idx = 0; idx < (int)LWIP_CONFIG_NUM_SOCKETS; idx++) {
        debug_socket_info(idx, 1, 0);
      }
      ret = LOS_OK;
    }
  } else if (argc == 3) {
    if (strcmp("-d", argv[1]) == 0) {
      /* netdebug sock -d <idx> */
      idx = atoi(argv[2]);
      if (idx >= 0) {
        debug_socket_info(idx, 1, 1);
        ret = LOS_OK;
      } else {
        PRINTK("Error: idx < 0"CRLF);
      }
    }
  }

  return ret;
}

u32_t
os_shell_netdebug(int argc, char **argv)
{
  u32_t ret = LOS_NOK;
  if (argv == NULL) {
    return LOS_NOK;
  }
  if (argc < 1) {
    goto usage;
  }
  if (strcmp("memp", argv[0]) == 0) {
    LOCK_TCPIP_CORE();
    ret = netdebug_memp(argc, argv);
    UNLOCK_TCPIP_CORE();
    if (ret != LOS_OK) {
      goto usage_memp;
    }
  } else if (strcmp("sock", argv[0]) == 0) {
    /* netdebug sock {-i | -d <idx>} */
    LOCK_TCPIP_CORE();
    ret = netdebug_sock(argc, argv);
    UNLOCK_TCPIP_CORE();
    if (ret != LOS_OK) {
      goto usage_sock;
    }
  } else {
    goto usage;
  }
  return ret;

usage:
  /* Cmd help */
  PRINTK(CRLF"Usage:"CRLF);
  PRINTK("netdebug memp {-i | -d <type> | -udp | -tcp | -raw |-conn}"CRLF);
  PRINTK("netdebug sock {-i | -d <idx>}"CRLF);
  return LOS_NOK;

usage_memp:
  /* netdebug memp help */
  PRINTK(CRLF"Usage:"CRLF);
  PRINTK("netdebug memp {-i | -d <type> | -udp | -tcp | -raw |-conn}"CRLF);
  return LOS_NOK;

usage_sock:
  /* netdebug sock help */
  PRINTK(CRLF"Usage:"CRLF);
  PRINTK("netdebug sock {-i | -d <idx>}"CRLF);
  return LOS_NOK;
}
#endif /* LWIP_DEBUG_INFO */

#if defined(LOSCFG_SHELL) && defined(LWIP_DEBUG_INFO)
SHELLCMD_ENTRY(netdebug_shellcmd, CMD_TYPE_EX, "netdebug",  XARGS, (CmdCallBackFunc)os_shell_netdebug);
#endif /* LOSCFG_SHELL && LWIP_DEBUG_INFO */

u32_t
os_shell_netif(int argc, char **argv)
{
  struct netif *netif = NULL;
  if (argv == NULL) {
    return LOS_NOK;
  }
  if (argc < 1) {
    PRINTK("netif_default wlan0"CRLF);
    return LOS_NOK;
  }

  netif = netif_find(argv[0]);
  if (netif == NULL) {
    PRINTK("not find %s"CRLF, argv[0]);
    return LOS_NOK;
  }

  (void)netifapi_netif_set_default(netif);
  return LOS_OK;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(netif_shellcmd, CMD_TYPE_EX, "netif_default", XARGS, (CmdCallBackFunc)os_shell_netif);
#endif /* LOSCFG_SHELL */

#ifdef LWIP_TESTBED
extern void cmd_reset(void);
u32_t
os_shell_reboot(int argc, const char **argv)
{
  cmd_reset();
  return LOS_OK;
}
#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(reboot_shellcmd, CMD_TYPE_EX, "reboot", XARGS, (CmdCallBackFunc)os_shell_reboot);
#endif /* LOSCFG_SHELL */
#endif

#endif /* LWIP_ENABLE_LOS_SHELL_CMD */

u32_t
os_shell_display_version(int argc, char **argv)
{
  (void)argc;
  (void)argv;
#ifdef CUSTOM_AT_COMMAND
  (void)hi_at_printf("+Base LwIP %s, %s"CRLF, LWIP_VERSION_STRING, NSTACK_VERSION_STR);
  (void)hi_at_printf("OK"CRLF);
#else
  PRINTK("Base LwIP %s, %s"CRLF, LWIP_VERSION_STRING, NSTACK_VERSION_STR);
#endif
  return LOS_OK;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(lwip_version_shellcmd, CMD_TYPE_EX, "lwip_version", XARGS, (CmdCallBackFunc)os_shell_display_version);
#endif /* LOSCFG_SHELL */

#if LWIP_IPV6
static void
display_ipv6_prefix(void)
{
  u8_t i;
  char ac_ipv6_addr[IP6ADDR_STRLEN_MAX + 1] = {0};
  u8_t atleast_one_entry = 0;
  /* Display prefix */
  PRINTK("================="CRLF);
  PRINTK("|| Prefix List ||"CRLF);
  PRINTK("================="CRLF);
  PRINTK("%-50s %-16s %-20s"CRLF,
         "Prefix", "netif", "validLifetime");
  PRINTK("---------------------------------------------------------------------------------"CRLF);
  /* Display neighbour Cache Entry */
  for (i = 0; i < LWIP_ND6_NUM_PREFIXES; i++) {
    if (prefix_list[i].netif != NULL && prefix_list[i].invalidation_timer > 0) {
      atleast_one_entry = 1;
      (void)ip6addr_ntoa_r((const ip6_addr_t *)(prefix_list[i].prefix.addr), (ac_ipv6_addr), INET6_ADDRSTRLEN);
      PRINTK("%-50s ", ac_ipv6_addr);
      PRINTK("%s%-13u ", prefix_list[i].netif->name, prefix_list[i].netif->num);
      PRINTK("%-20u"CRLF, prefix_list[i].invalidation_timer);
    }
  }
  if (atleast_one_entry == 0) {
    PRINTK("**** NO VALID PREFIXES FOUND CONFIGURED ****"CRLF);
  }
  PRINTK("---------------------------------------------------------------------------------"CRLF);
}

static void
display_ipv6_neighbor_cache_entry(void)
{
  u8_t i;
  u8_t atleast_one_entry = 0;
  char ac_ipv6_addr[IP6ADDR_STRLEN_MAX + 1] = {0};
  char aclladdr[20] = {0};
  const char *ac_states[] = {"NO_ENTRY", "INCOMPLETE", "REACHABLE", "STALE", "DELAY", "PROBE"};

  PRINTK(CRLF CRLF);
  PRINTK("============================"CRLF);
  PRINTK("|| Neighbor Cache Entries ||"CRLF);
  PRINTK("============================"CRLF);
  PRINTK("%-50s %-25s %-16s %-15s %-10s"CRLF,
         "Neighbor", "MAC", "netif", "state", "IsRouter");
  PRINTK("------------------------------------------------------------"
         "------------------------------------------------------------"CRLF);

  /* Display neighbour Cache Entry */
  for (i = 0; i < LWIP_ND6_NUM_NEIGHBORS; i++) {
    if (neighbor_cache[i].state != ND6_NO_ENTRY) {
      atleast_one_entry = 1;
      (void)ip6addr_ntoa_r((const ip6_addr_t *)(neighbor_cache[i].next_hop_address.addr), (ac_ipv6_addr),
                           INET6_ADDRSTRLEN);
      PRINTK("%-50s ", ac_ipv6_addr);

      if (snprintf_s(aclladdr, sizeof(aclladdr), sizeof(aclladdr) - 1, "%02X:%02X:%02X:%02X:%02X:%02X",
                     neighbor_cache[i].lladdr[0], neighbor_cache[i].lladdr[1], neighbor_cache[i].lladdr[2],
                     neighbor_cache[i].lladdr[3], neighbor_cache[i].lladdr[4], neighbor_cache[i].lladdr[5]) < 0) {
        return;
      }
      PRINTK("%-25s ", aclladdr);
      PRINTK("%s%-13u ", neighbor_cache[i].netif->name, neighbor_cache[i].netif->num);
      PRINTK("%-15s ", ac_states[neighbor_cache[i].state]);
      PRINTK("%-10s"CRLF, neighbor_cache[i].isrouter ? "Yes" : "No");
    }
  }
  if (atleast_one_entry == 0) {
    PRINTK("**** NO NEIGHBOURS FOUND ****\n");
  }
  PRINTK("------------------------------------------------------------"
         "------------------------------------------------------------"CRLF);
}

static void
display_ipv6_des_cache_entry(void)
{
  u8_t i;
  u8_t atleast_one_entry = 0;
  char ac_ipv6_addr[IP6ADDR_STRLEN_MAX + 1] = {0};
  PRINTK(CRLF CRLF);
  PRINTK("==============================="CRLF);
  PRINTK("|| Destination Cache Entries ||"CRLF);
  PRINTK("==============================="CRLF);
  PRINTK("%-50s %-50s %-10s %-10s"CRLF,
         "Destination", "NextHop", "PMTU", "age");
  PRINTK("------------------------------------------------------------"
         "------------------------------------------------------------"CRLF);
  /* Display neighbour Cache Entry */
  for (i = 0; i < LWIP_ND6_NUM_DESTINATIONS; i++) {
    if (!ip6_addr_isany(&(destination_cache[i].destination_addr))) {
      atleast_one_entry = 1;
      (void)ip6addr_ntoa_r((const ip6_addr_t *)(destination_cache[i].destination_addr.addr), (ac_ipv6_addr),
                           INET6_ADDRSTRLEN);
      PRINTK("%-50s ", ac_ipv6_addr);
      (void)ip6addr_ntoa_r((const ip6_addr_t *)(destination_cache[i].next_hop_addr.addr), (ac_ipv6_addr),
                           INET6_ADDRSTRLEN);
      PRINTK("%-50s ", ac_ipv6_addr);
      PRINTK("%-10u ", destination_cache[i].pmtu);
      PRINTK("%-10u"CRLF, destination_cache[i].age);
    }
  }
  if (atleast_one_entry == 0) {
    PRINTK("**** NO DESTINATION CACHE FOUND ****"CRLF);
  }
  PRINTK("------------------------------------------------------------"
         "------------------------------------------------------------"CRLF);
}

static void
display_default_router_entry(void)
{
  u8_t i;
  u8_t atleast_one_entry = 0;
  char ac_ipv6_addr[IP6ADDR_STRLEN_MAX + 1] = {0};
  PRINTK(CRLF CRLF);
  PRINTK("============================"CRLF);
  PRINTK("|| Default Router Entries ||"CRLF);
  PRINTK("============================"CRLF);
  PRINTK("%-50s %-20s %-10s"CRLF,
         "Router", "invalidation_timer", "flags");
  PRINTK("-----------------------------------------------------------------------------"CRLF);
  /* Display Default Router Cache Entry */
  for (i = 0; i < LWIP_ND6_NUM_ROUTERS; i++) {
    if (default_router_list[i].neighbor_entry) {
      atleast_one_entry = 1;
      (void)ip6addr_ntoa_r((const ip6_addr_t *)((default_router_list[i].neighbor_entry)->next_hop_address.addr),
                           (ac_ipv6_addr), INET6_ADDRSTRLEN);
      PRINTK("%-50s ", ac_ipv6_addr);
      PRINTK("%-20u ", default_router_list[i].invalidation_timer);
      PRINTK("%-10u"CRLF, default_router_list[i].flags);
    }
  }
  if (atleast_one_entry == 0) {
    PRINTK("**** NO DEFAULT ROUTERS FOUND ****"CRLF);
  }
  PRINTK("-----------------------------------------------------------------------------"CRLF);
}

u32_t
os_shell_ipdebug(int argc, char **argv)
{
  LWIP_UNUSED_ARG(argc);
  LWIP_UNUSED_ARG(argv);

  if (tcpip_init_finish == 0) {
    PRINTK("%s: tcpip_init have not been called"CRLF, __FUNCTION__);
    goto exit;
  }

  display_ipv6_prefix();
  display_ipv6_neighbor_cache_entry();
  display_ipv6_des_cache_entry();
  display_default_router_entry();

exit:
  return LOS_OK;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(ipdebug_shellcmd, CMD_TYPE_EX, "ipdebug", XARGS, (CmdCallBackFunc)os_shell_ipdebug);
#endif /* LOSCFG_SHELL */
#endif /* LWIP_IPV6 */

#if LWIP_IPV4 && LWIP_IGMP
static void
os_shell_igmp_usage(void)
{
  PRINTK("igmp\tshow\tifname {start | stop | report}\t{ifname | ifaddr} {join | leave} groupaddr"CRLF);
}

static void
igmp_netif_show(void)
{
  struct netif *netif_p = NULL;
  struct igmp_group *igmp_group_p = NULL;
#ifdef LWIP_DEBUG_OPEN
  (void)hi_at_printf("\t%-16s%-16s%-16s%-16s%-16s"CRLF, "groupaddr", "reporter", "state", "timer(100ms)", "use");
#else
  PRINTK("\t%-16s%-16s%-16s%-16s%-16s"CRLF, "groupaddr", "reporter", "state", "timer(100ms)", "use");
#endif
  for (netif_p = netif_list; netif_p != NULL; netif_p = netif_p->next) {
    if ((netif_p->flags & NETIF_FLAG_IGMP) == 0) {
      continue;
    }
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("%s%d"CRLF, netif_p->name, netif_p->num);
#else
    PRINTK("%s%d"CRLF, netif_p->name, netif_p->num);
#endif
    igmp_group_p = netif_igmp_data(netif_p);
    if (igmp_group_p == NULL) {
#ifdef LWIP_DEBUG_OPEN
      (void)hi_at_printf(CRLF);
#else
      PRINTK(CRLF);
#endif
      continue;
    }
    while (igmp_group_p != NULL) {
#ifdef LWIP_DEBUG_OPEN
      (void)hi_at_printf("\t%-16s%-16d%-16d%-16d%-16d"CRLF, ip4addr_ntoa(&(igmp_group_p->group_address)),
                         igmp_group_p->last_reporter_flag, igmp_group_p->group_state,
                         igmp_group_p->timer, igmp_group_p->use);
#else
      PRINTK("\t%-16s%-16d%-16d%-16d%-16d"CRLF, ip4addr_ntoa(&(igmp_group_p->group_address)),
             igmp_group_p->last_reporter_flag, igmp_group_p->group_state,
             igmp_group_p->timer, igmp_group_p->use);
#endif
      igmp_group_p = igmp_group_p->next;
    }
  }
}

static s32_t
igmp_netif_control(char **argv)
{
  s32_t ret = 0;
  struct netif *netif_p = NULL;

  netif_p = netif_find(argv[0]);
  if (netif_p == NULL) {
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("no such netif named %s"CRLF, argv[0]);
#else
    PRINTK("no such netif named %s"CRLF, argv[0]);
#endif
    goto usage;
  }
  if (strcmp(argv[1], "start") == 0) {
    ret = igmp_start(netif_p);
  } else if (strcmp(argv[1], "stop") == 0) {
    ret = igmp_stop(netif_p);
  } else if (strcmp(argv[1], "report") == 0) {
    igmp_report_groups(netif_p);
  } else {
    goto usage;
  }

  return ret;
usage:
  os_shell_igmp_usage();
  return -1;
}

static s32_t
igmp_membership_netif_ctrl(struct netif *netif_p,
                           ip4_addr_t groupaddr, char **argv)
{
  s32_t ret;

  if (strcmp(argv[1], "join") == 0) {
    ret = igmp_joingroup_netif(netif_p, &groupaddr);
  } else if (strcmp(argv[1], "leave") == 0) {
    ret = igmp_leavegroup_netif(netif_p, &groupaddr);
  } else {
    ret = -1;
    os_shell_igmp_usage();
  }

  return ret;
}

static s32_t
igmp_membership_addr_ctrl(ip4_addr_t groupaddr,
                          char **argv)
{
  s32_t ret = 0;
  ip4_addr_t ifaddr = {0};

  ifaddr.addr = inet_addr(argv[0]);
  if (ifaddr.addr == IPADDR_NONE) {
    goto usage;
  }
  if (strcmp(argv[1], "join") == 0) {
    ret = igmp_joingroup(&ifaddr, &groupaddr);
  } else if (strcmp(argv[1], "leave") == 0) {
    ret = igmp_leavegroup(&ifaddr, &groupaddr);
  } else {
    goto usage;
  }

  return ret;
usage:
  os_shell_igmp_usage();
  return -1;
}

static s32_t
igmp_membership_ctrl(char **argv)
{
  s32_t ret;
  struct netif *netif_p = NULL;
  ip4_addr_t groupaddr = {0};

  groupaddr.addr = inet_addr(argv[2]);
  if ((groupaddr.addr == IPADDR_NONE) || !(ip4_addr_ismulticast(&groupaddr))) {
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("invalid groupaddr %s"CRLF, argv[2]); // 2 is cast addr
#else
    PRINTK("invalid groupaddr %s"CRLF, argv[2]); // 2 is cast addr
#endif
    os_shell_igmp_usage();
    return -1;
  }
  netif_p = netif_find(argv[0]);
  if (netif_p == NULL) {
    ret = igmp_membership_addr_ctrl(groupaddr, argv);
  } else {
    ret = igmp_membership_netif_ctrl(netif_p, groupaddr, argv);
  }

  return ret;
}

u32_t
os_shell_igmp(int argc, char **argv)
{
  s32_t ret = 0;

  if (tcpip_init_finish == 0) {
#ifdef LWIP_DEBUG_OPEN
    (void)hi_at_printf("%s: tcpip_init have not been called"CRLF, __FUNCTION__);
#else
    PRINTK("%s: tcpip_init have not been called"CRLF, __FUNCTION__);
#endif
    return LOS_NOK;
  }
  if (argv == NULL) {
    return LOS_NOK;
  }
  LOCK_TCPIP_CORE();
  if ((argc == 1) && (strcmp(argv[0], "show") == 0)) {
    igmp_netif_show();
  } else if (argc == 2) {
    ret = igmp_netif_control(argv);
  } else if (argc == 3) {
    ret = igmp_membership_ctrl(argv);
  } else {
    os_shell_igmp_usage();
  }
  UNLOCK_TCPIP_CORE();
#ifdef LWIP_DEBUG_OPEN
  (void)hi_at_printf("igmp ret %d"CRLF, ret);
#else
  PRINTK("igmp ret %d"CRLF, ret);
#endif
  return LOS_OK;
}

u32_t
at_os_shell_igmp(int argc, char **argv)
{
  u32_t ret = os_shell_igmp(argc, argv);

  return ret;
}

#ifdef LOSCFG_SHELL
SHELLCMD_ENTRY(igmp_shellcmd, CMD_TYPE_EX, "igmp", XARGS, (CmdCallBackFunc)os_shell_igmp);
#endif /* LOSCFG_SHELL */
#endif /* LWIP_IPV4 && LWIP_IGMP */
