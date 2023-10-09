/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL internal common constants
 * Author: NA
 * Create: 2019-04-02
 */

#ifndef _RPL_COMMON_H_
#define _RPL_COMMON_H_

#include <stdint.h> /* required for uint*_t */
#include <stdio.h>  /* required for NULL */

#include "ripple.h"
#include "rpl_platform_api.h"
#include "securec.h"

/* Keep all global state and config information for RPL */
typedef struct {
  /* Static config: Config which does not change */
  struct {
    rpl_addr_t rpl_mcast_addr; /* 0xff02::1a as defined in RFC 6550 */
    uint8_t mode; /* node's default mode; 6lbr/6lr/6ln */
    rpl_buffer_t send; /* RPL send buffer */
  } sta;

  /* Dynamic config: Config which can change */
  struct {
    rpl_linkaddr_t lladdr;
    rpl_timer_t periodic_timer;
    rpl_stat_t stats; /* RPL Statistics */
    /*
     * Periodic tick for DIS.. once ticks exceed
     * DIS_INTERVAL, send DIS
     */
    uint8_t dis_tick;
    uint8_t rpl_inited : 1; /* RplMgmtInit() called? */
    /*
     * send atleast one DIS without waiting
     * when the node starts up
     */
    uint8_t dis_sent_cnt : 2;
  } dyn;
} rpl_global_t;

#ifndef MAX_UINT16
#define MAX_UINT16 0xffff
#endif

#ifndef MAX_UINT32
#define MAX_UINT32 0xffffffffUL
#endif

rpl_global_t *rpl_get_config(void);

uint8_t rpl_config_get_mode(void);
void rpl_config_set_mode(uint8_t mode);

uint8_t *rpl_config_get_sendbuf(void);
void rpl_config_set_sendbuf(uint8_t *buf);

uint16_t rpl_config_get_sendbuf_len(void);
void rpl_config_set_sendbuf_len(uint16_t len);

rpl_addr_t *rpl_config_get_mcast_addr(void);

uint8_t rpl_config_get_rinited(void);
void rpl_config_set_rinited(uint8_t init);

uint8_t rpl_config_get_dis_cnt(void);
void rpl_config_set_dis_cnt(uint8_t dis_sent_cnt);

uint8_t rpl_config_get_dis_tick(void);
void rpl_config_set_dis_tick(uint8_t dis_tick);

rpl_linkaddr_t *rpl_config_get_lladdr(void);
rpl_timer_t *rpl_config_get_ptimer(void);
rpl_stat_t *rpl_config_get_stats(void);

void rpl_config_clear_stats(void);
void rpl_config_clear_dyn(void);

void rpl_handle_timer_check(void *arg);
void rpl_handle_dio_check(void *arg);
void rpl_tcpip_timer_check(void);
void rpl_tcpip_dio_check(void);

void rpl_beacon_priority_update(uint8_t is_clear);

/* if stat cnt reaches MAX_UINT stop incrementing */
#define RPL_STAT(status) do { \
  rpl_stat_t *stats = rpl_config_get_stats(); \
  (stats->status) += (((stats->status ^ 0xffffffff) != 0) ? 1 : 0); \
} while (0)

#define IS_STORING_MOP(mop) \
  (((mop) == RPL_MOP_STORING_NO_MCAST) || ((mop) == RPL_MOP_STORING_WITH_MCAST))

/* Lollipop Sequence Counters as defined in RFC 6550 */
#define LPOP_SEQ_WIN 64
#define LPOP_INIT ((uint8_t)(-(LPOP_SEQ_WIN)))
#define LPOP_CIRCLE_MAX 0x7f
#define LPOP_CIRCLE_INIT 0
#define LPOP_LINEAR_MAX 0xff
#define LPOP_INCR(lx) ((lx) = (uint8_t)(((int8_t)(lx)) < 0 ? (lx) + 1 : ((lx) + 1) & 0x7f))
#if RPL_CONF_USE_PERSISTENT_STORAGE
#ifndef RPL_CONF_LPOP_EASY_COMPARE
#define LPOP_IS_GREATER(la, lb) rpl_lpop_is_greater((la), (lb))
#else
#define LPOP_IS_GREATER(la, lb) (((la) > (lb)) || (((lb) - (la)) >= LPOP_SEQ_WIN))
#endif
#define LPOP_IS_CHANGED(la, lb) LPOP_IS_GREATER(la, lb)
#else
#define LPOP_IS_GREATER(la, lb) rpl_lpop_is_greater((la), (lb))
#define LPOP_IS_CHANGED(la, lb) (la != lb)
#endif
#define IN_RANGE(val, beg, end) (((val) >= (beg)) && ((val) < (end)))

#define RPL_ALL_NODES_MCAST(addr) RPL_SET_ADDR((addr), 0xff02, 0, 0, 0, 0, 0, 0, 0x001a)

#define INFINITE_RANK 0xffff
#define MC_ETX_DIVISOR 256
#define RPL_MAX_METRIC ((MC_ETX_DIVISOR) * (RPL_CONF_MAX_PATH_COST))
#define RPL_MIN_METRIC MC_ETX_DIVISOR

#define RPL_DAG_MC_ETX_DIVISOR 256
#if RPL_MAX_LLADDR_LEN == 8
#define MACADDR_FMT "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x"
#define MACADDR_VAL(mac)                                          \
  (mac)->addr[0], (mac)->addr[1], (mac)->addr[2], (mac)->addr[3], \
  (mac)->addr[4], (mac)->addr[5], (mac)->addr[6], (mac)->addr[7]
#elif RPL_MAX_LLADDR_LEN == 6
#define MACADDR_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MACADDR_VAL(mac)                                          \
  (mac)->addr[0], (mac)->addr[1], (mac)->addr[2], (mac)->addr[3], \
  (mac)->addr[4], (mac)->addr[5]
#endif

#if RPL_CONF_USE_PERSISTENT_STORAGE
#define PSTORE_LPOP_INCR(ps, val) do { \
  LPOP_INCR(val); \
  (void)rpl_pstore_write((ps), &(val), sizeof(val)); \
} while (0)
#else
#define PSTORE_LPOP_INCR(ps, val) LPOP_INCR(val)
#endif

#endif /* _RPL_COMMON_H_ */
