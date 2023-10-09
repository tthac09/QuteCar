/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL Exported constants and structures
 * Author: NA
 * Create: 2019-04-02
 */

#ifndef _RIPPLE_H_
#define _RIPPLE_H_

#include <stdint.h>
#include "rpl_conf.h"

#define RPL_OK 0              /* success */
#define RPL_FAIL 1            /* general failure */
#define RPL_ALREADY_PRESENT 2 /* Search resulted in info already present for e.g. search prefix/target */
#define RPL_MALFORMED 3       /* Malformed header or packet detected */
#define RPL_ROUTE_LOOP 4

#define RPL_INSTID_NA 0xff

#define RPL_OCP_OF0 0       /* objective function 0 (RFC 6552) */
#define RPL_OCP_MRHOF 1     /* min-rank-with-hysteresis OF (RFC 6719) */
#define RPL_OCP_UNKNOWN 255 /* OCP not set */

/* Used with RplMgmtStart() */
#define RPL_MODE_6LBR 1 /* Root mode */
#define RPL_MODE_6LR 2  /* Router mode */
#define RPL_MODE_6LN 3  /* Leaf mode */

/* Used with RplMgmtStart() */
#define RPL_MOP_NO_DOWNWARD_ROUTES 0x0
#define RPL_MOP_NON_STORING 0x1
#define RPL_MOP_STORING_NO_MCAST 0x2
#define RPL_MOP_STORING_WITH_MCAST 0x3
#define RPL_MOP_P2P_RPL 0x4

#define RPL_INSTANCE_GLOBAL_MAX 127
#define RPL_PREFIX_LEN 64
#define RPL_PREFIX_LEN_TO_LEN(plen)  ((plen) >> 3)
#define RPL_LEN_TO_PREFIX_LEN(len)  ((len) << 3)
#define RPL_PREFIX_LEN_IN_BYTE 8

#define RPL_6ADDR_4BYTES_CNT 4
#define RPL_6ADDR_2BYTES_CNT 8
#define RPL_6ADDR_1BYTES_CNT 16

#define RPL_4ADDR_2BYTES_CNT 2
#define RPL_4ADDR_1BYTES_CNT 4

#define RPL_TRUE 1
#define RPL_FALSE 0

#define RPL_VERSION_STRING "nStack_RPL_N100 1.0.1.SPC1.B003"

typedef uint8_t rpl_mnode_id_t;
typedef void rpl_netdev_t;

typedef struct {
  uint8_t addr[RPL_MAX_LLADDR_LEN];
  uint8_t len;
} rpl_linkaddr_t;

typedef union {
  uint8_t a8[RPL_6ADDR_1BYTES_CNT];
  uint16_t a16[RPL_6ADDR_2BYTES_CNT];
  uint32_t a32[RPL_6ADDR_4BYTES_CNT];
} rpl_6addr_t;

typedef union {
  uint8_t a8[RPL_4ADDR_1BYTES_CNT];
  uint16_t a16[RPL_4ADDR_2BYTES_CNT];
} rpl_4addr_t;

#ifdef RPL_CONF_IP6
typedef rpl_6addr_t rpl_addr_t;
#define RPL_ADDR_BIT_LEN 128
#define ADDR_LAST2B(addr) RPL_HTONS((addr)->a16[7])
#define RPL_ADDR_ISSET(addr) ((addr)->a32[0] || (addr)->a32[1] || (addr)->a32[2] || (addr)->a32[3])
#define RPL_SET_ADDR(addr, addr0, addr1, addr2, addr3, addr4, addr5, addr6, addr7) do { \
  (addr)->a16[0] = ((addr0) == 0) ? 0 : RPL_HTONS(addr0); \
  (addr)->a16[1] = ((addr1) == 0) ? 0 : RPL_HTONS(addr1); \
  (addr)->a16[2] = ((addr2) == 0) ? 0 : RPL_HTONS(addr2); \
  (addr)->a16[3] = ((addr3) == 0) ? 0 : RPL_HTONS(addr3); \
  (addr)->a16[4] = ((addr4) == 0) ? 0 : RPL_HTONS(addr4); \
  (addr)->a16[5] = ((addr5) == 0) ? 0 : RPL_HTONS(addr5); \
  (addr)->a16[6] = ((addr6) == 0) ? 0 : RPL_HTONS(addr6); \
  (addr)->a16[7] = ((addr7) == 0) ? 0 : RPL_HTONS(addr7); \
} while (0)

#define IS_ADDR_MCAST(addr) ((addr)->a8[0] == 0xff)
#define IS_ADDR_LINKLOCAL(addr) (((addr)->a8[0] == 0xfe) && ((addr)->a8[1] == 0x80))
#define IS_ADDR_UNSET(a) \
  ((((a)->a32[0]) == 0) && (((a)->a32[1]) == 0) && (((a)->a32[2]) == 0) && (((a)->a32[3]) == 0))

#define RPL_PRN_ADDR(log_type, str, ip6addr) do {                                                    \
  log_type("%s **:**:**:**:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",                                   \
           str, (uint32_t)(ip6addr)->a8[8], (uint32_t)(ip6addr)->a8[9], (uint32_t)(ip6addr)->a8[10], \
           (uint32_t)(ip6addr)->a8[11], (uint32_t)(ip6addr)->a8[12], (uint32_t)(ip6addr)->a8[13],    \
           (uint32_t)(ip6addr)->a8[14], (uint32_t)(ip6addr)->a8[15]);                                \
} while (0)

#else /* RPL_CONF_IP6 */
typedef rpl_4addr_t rpl_addr_t;
#define RPL_ADDR_BIT_LEN 32
#define RPL_ADDR_ISSET(addr) (((addr)->a16[0] != 0) || ((addr)->a16[1] != 0))
#define RPL_PRN_ADDR(log_type, str, ip4addr)
#define RPL_SET_ADDR(addr, addr0, addr1, addr2, addr3) do { \
  (addr)->a8[0] = RPL_HTONS(addr0); \
  (addr)->a8[1] = RPL_HTONS(addr1); \
  (addr)->a8[2] = RPL_HTONS(addr2); \
  (addr)->a8[3] = RPL_HTONS(addr3); \
} while (0)
#endif
#if RPL_MAX_LLADDR_LEN == 8
#define PRN_LADDR(log_type, str, lladdr) do {                             \
  log_type("%s *:*:*:*:%02x:%02x:%02x:%02x\n",                            \
           str, (uint32_t)(lladdr)->addr[4], (uint32_t)(lladdr)->addr[5], \
           (uint32_t)(lladdr)->addr[6], (uint32_t)(lladdr)->addr[7]);     \
} while (0)
#elif RPL_MAX_LLADDR_LEN == 6
#define PRN_LADDR(log_type, str, lladdr) do {    \
  log_type("%s *:*:*:%02x:%02x:%02x\n",          \
           str,                                  \
           (uint32_t)(lladdr)->addr[3],          \
           (uint32_t)(lladdr)->addr[4],          \
           (uint32_t)(lladdr)->addr[5]);         \
} while (0)
#endif
typedef struct {
  rpl_addr_t addr;            /* Prefix */
  uint32_t lifetime;          /* Prefix lifetime */
  uint8_t len;                /* Prefix-Length. __Number of valid leading bits__ in the prefix */
  uint8_t cnt;
  uint8_t delegate_daoack_cnt;    /* delegate msta daoack count */
  uint8_t auto_addr_conf : 1; /* autonomous address-configuration flag */
  /*
   * Router Address. Parent node sets this flag to inform that this
   * address is global address to be used in Transit Information Option
   * of DAO in non-storing MOP. RFC 6550 Section 6.7.10
   */
  uint8_t is_router_addr : 1;
  uint8_t is_delegate : 1;
  uint8_t is_info : 1;
  uint8_t is_ra_sent : 1;
  uint8_t is_off : 1;
} rpl_prefix_t;

#ifndef RPL_HTONS
#if RPL_CONF_BIG_ENDIAN
#error "not the right endian?"
#define RPL_HTONS(n) (n)
#define RPL_HTONL(n) (n)
#else /* RPL_BIG_ENDIAN */

#define RPL_HTONS(n) (uint16_t)(((((uint16_t)(n)) & 0xff) << 8) | (((uint16_t)(n)) >> 8))
#define RPL_HTONL(n) (((uint32_t)RPL_HTONS(n) << 16) | RPL_HTONS(((uint32_t)(n)) >> 16))
#endif /* RPL_BIG_ENDIAN */
#endif /* RPL_HTONS */

#if RPL_CONF_INSTID == 1
typedef uint8_t inst_t;
#define RPL_INST_F "hhu"
#elif RPL_CONF_INSTID == 2
typedef uint16_t inst_t;
#define RPL_INST_F "hu"
#elif RPL_CONF_INSTID == 4
typedef uint32_t inst_t;
#define RPL_INST_F "u"
#else
#error "Incorrect RPL_CONF_INSTID"
#endif

typedef struct {
  uint32_t rack_timer_val; /* Time it takes for RACK to come from root to node */
  uint16_t lifetime_unit;  /* Default lifetime unit */
  uint16_t default_rank_threshold; /* default rank threshold */
  uint16_t min_rank_inc;   /* Min rank increase */
  uint16_t max_rank_inc;   /* Max rank increase */
  uint16_t ocp;            /* Objective function Code Point to use */
  uint8_t lifetime;        /* Default lifetime */
  uint8_t dio_imin;        /* DIO Trickle parameter, Interval Minimum */
  uint8_t dio_red;         /* DIO Trickle parameter, Redundancy */
  uint8_t dio_idbl;        /* DIO Trickle parameter, Interval Max */
  uint8_t mop;             /* Mode of Operation to use */
  uint8_t rack_retry;      /* RACK retry count */
} rpl_cfg_t;

typedef struct {
  uint8_t *buf; /* buf pointer */
  uint16_t len; /* buf len */
} rpl_buffer_t;

#define LINK_TX_ACK_OK 0
#define LINK_TX_OK_NOACK_REQUIRED 1
#define LINK_TX_NOACK 2
#define LINK_BROKEN 6

#define RPL_LINK_INFO_NONE 0
#define RPL_LINK_INFO_TX 1
#define RPL_LINK_INFO_RX 2
#define RPL_LINK_DEL_PEER 3

typedef struct {
  uint8_t type; /* Type of event */
  rpl_linkaddr_t peer_mac; /* MAC addres of the peer */
  union {
    struct {
      uint8_t status; /* TX status, ACK_OK, ACK not ok */
      uint8_t retry_cnt; /* retry count, applicable only with ACK_OK */
      uint16_t pkt_size; /* Packet size */
    } tx;
    struct {
      uint8_t lqi; /* Link Quality Indicator */
      int8_t rssi; /* Received Signal Strength Indicator */
    } rx;
  } u;
} rpl_linkinfo_t;

typedef struct {
  rpl_addr_t src; /* src ip address of the packet */
  rpl_addr_t dst; /* dst ip address of the packet */
  void *iface; /* interface on which packet is received */
  rpl_linkinfo_t link; /* Link layer information */
} rpl_pktinfo_t;

typedef struct {
  uint32_t dis_tx;
  uint32_t dis_rx;
  uint32_t dis_drop;
  uint32_t dio_tx;
  uint32_t dio_rx;
  uint32_t dio_drop;
  uint32_t dco_tx;
  uint32_t dco_rx;
  uint32_t dco_drop;
  uint32_t dao_tx;
  uint32_t dao_rx;
  uint32_t dao_drop;
  uint32_t dao_fwd;
  uint32_t daoack_tx;
  uint32_t daoack_rx;
  uint32_t daoack_drop;
  uint32_t dcoack_tx;
  uint32_t dcoack_rx;
  uint32_t dcoack_drop;
  uint32_t dio_trickle_rst;
  uint32_t global_repair;
  uint32_t local_repair;
  uint32_t dtsn_incr;
  uint32_t parent_switch;
  uint32_t rpl_code_err;
} rpl_stat_t;

typedef struct {
  rpl_addr_t tgt;
  rpl_addr_t nhop;
  uint16_t lifetime;
  rpl_mnode_id_t mnid;
  uint8_t path_seq;
} rpl_routeinfo_t;

/* Neighbour Policy reasons for neighbor entry addition */
#define NBPOL_DAO 1 /* nbr added due to DAO - child entry */
#define NBPOL_DIO 2 /* nbr added due to DIO - parent entry */
#define NBPOL_OTHERS 3 /* other reason - possibly security */

#ifndef STATIC_ASSERT
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
/*
 * This can't be used twice on the same line so ensure if using in headers
 * that the headers are not included twice (by wrapping in #ifndef...#endif)
 * Note it doesn't cause an issue when used on same line of separate modules
 * compiled with gcc -combine -fwhole-program.
 */
#define STATIC_ASSERT(e) \
  ;enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1 / (int)(!!(e)) }
#endif /* STATIC_ASSERT */

#ifndef RPL_UNUSED_ARG
#define RPL_UNUSED_ARG(x) ((void)(x))
#endif

#endif /* _RIPPLE_H_ */
