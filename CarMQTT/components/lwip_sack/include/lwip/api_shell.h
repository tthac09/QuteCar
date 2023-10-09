/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2015. All rights reserved.
 * Description: declare shell cmds APIs
 * Author: none
 * Create: 2013
 */

#ifndef LWIP_API_SHELL_H
#define LWIP_API_SHELL_H

#include "lwip/opt.h"
#include "lwip/netif.h"
#if defined (__cplusplus) && __cplusplus
extern "C" {
#endif

extern int tcpip_init_finish;

/* help convert ptr to u32 array(if 64bit platfrom) */
union los_ptr_args {
  void *ptr;
  u32_t args[2];
};

typedef void (*icmp_code_hander)(u8_t code, void *arg);

#ifdef CUSTOM_AT_COMMAND
typedef enum netstat_trans_type {
  TCP_IP6,
  TCP,
  UDP_IP6,
  UDP,
  RAW,
  PKT_RAW,
} netstat_trans_type;
#endif

u32_t lwip_ifconfig(int argc, const char **argv);
u32_t lwip_get_ipv4(char *localIp, unsigned char ipLen, unsigned char *ifname);
u32_t lwip_get_mac_addr(unsigned char *macAddr, unsigned int len, unsigned char *ifname);

u32_t lwip_ifstats(int argc, char **argv);
u32_t lwip_arp(int argc, char **argv);
u32_t at_lwip_arp(int argc, char **argv);
u32_t os_shell_netif_up(int argc, char **argv);
u32_t os_shell_netif_down(int argc, char **argv);
u32_t os_shell_ping(int argc, const char **argv);
#if LWIP_IPV6
u32_t os_shell_ping6(int argc, const char **argv);
u32_t os_shell_ipdebug(int argc, char **argv);
#endif

#if LWIP_RPL || LWIP_RIPPLE
u32_t os_shell_rpl(int argc, char **argv);
u32_t os_rte_debug(int argc, char **argv);
#endif

u32_t os_shell_tftp(int argc, char **argv);
#if LWIP_SNTP
u32_t os_shell_ntpdate(int argc, char **argv);
#endif
#if LWIP_DNS
#ifdef CUSTOM_AT_COMMAND
u32_t os_shell_show_dns(void);
#endif
u32_t os_shell_dns(int argc, const char **argv);
#endif /* LWIP_DNS */
#if LWIP_IPV4 && LWIP_IGMP
u32_t os_shell_igmp(int argc, char **argv);
u32_t at_os_shell_igmp(int argc, char **argv);
#endif /* LWIP_IPV4 && LWIP_IGMP */
#if (LWIP_IPV6 && (LWIP_IPV6_MLD || LWIP_IPV6_MLD_QUERIER))
u32_t os_shell_mld6(int argc, char **argv);
#endif /* (LWIP_IPV6 && (LWIP_IPV6_MLD || LWIP_IPV6_MLD_QUERIER)) */
#if LWIP_DHCP
u32_t os_shell_dhcp(int argc, const char **argv);
void dhcp_clients_info_show(struct netif *netif_p);
#if LWIP_DHCPS
u32_t os_shell_dhcps(int argc, const char **argv);
void dhcps_info_show(struct netif *netif);
#endif /* LWIP_DHCPS */
#endif /* LWIP_DHCP */
u32_t os_shell_netstat(int argc, char **argv);

void netstat_internal(void *ctx);

u32_t os_shell_mcast6(int argc, char **argv);
#if LWIP_RIPPLE
u32_t os_shell_l2test(int argc, char **argv);
#endif /* LWIP_RIPPLE */

u32_t os_shell_netif(int argc, char **argv);

#if defined (__cplusplus) && __cplusplus
}
#endif

#endif

