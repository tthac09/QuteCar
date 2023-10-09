/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL Route mgmt headers
 * Author: NA
 * Create: 2019-04-25
 */

#ifndef _ROUTE_MGMT_H_
#define _ROUTE_MGMT_H_

#include "ripple.h"
#include "rpl_common.h"
#include "dag.h"

#define RPL_MAX_NEXT_HOP_LEN 8
#define RPL_MAX_NEXT_HOP_INDEX 8
#define RPL_ROUTE_TARGET_IID_LEN 8
#define RPL_ROUTE_TARGET_IID_INDEX 8
#define RPL_ROUTE_IID_HALF_INDEX 4

#pragma pack(1)
typedef struct {
#if RPL_ROUTE_TARGET_IID
  uint8_t iid[RPL_ROUTE_TARGET_IID_LEN];
#else
  rpl_addr_t ta; /* Target address */
#endif
  uint8_t nhop[RPL_MAX_NEXT_HOP_LEN]; /* Lower 8 bytes of next hop ... IPv6 nhop = 0xfe80 + low8 */
#if RPL_CONF_NONSTORING
  rpl_addr_t parent; /* Parent Address */
#endif
  uint16_t lifetime; /* Target lifetime */
#if RPL_CONF_MAX_INSTANCES > 1
  inst_t inst_id; /* RPLInstanceID */
#endif
  rpl_mnode_id_t mnid; /* Mesh Node ID */
  uint8_t path_seq; /* Path Sequence */
  uint8_t len; /* Target prefix length */
  uint8_t ipv4sta : 1;
  uint8_t non_mesh : 1;
} rpl_rte_t;
#pragma pack()

typedef uint8_t (*rpl_route_cb)(rpl_rte_t *, void *);

uint16_t rpl_get_mg_cnt(void);
uint8_t rpl_get_msta_cnt(void);

void rpl_purge_routes(uint8_t period_sec);

rpl_rte_t *rpl_get_route_entry(inst_t inst_id, const rpl_addr_t *addr, uint8_t len);
rpl_rte_t *rpl_get_next_route_entry(int *state);
/**
 * @Description: Update routing table information
 *
 * @param instID - RPLInstanceID
 * @param rti - Routing information
 *
 * @return OK/FAIL .. FAILs if no space in routing table
 */
uint8_t rpl_route_update(inst_t inst_id, rpl_rte_t *rte, uint8_t route_event_flag);

/**
 * @Description: Get Route entry
 *
 * @param instID - RPLInstanceID
 * @param tgt - Input Target to search for
 * @param rti - Output route information
 *
 * @return RPL_OK if entry is found, RPL_FAIL if entry not found
 */
uint8_t rpl_copy_route_entry(inst_t inst_id, const rpl_prefix_t *tgt, rpl_rte_t *rte);

void rpl_copy_rte_remove(const rpl_rte_t *crte);

uint16_t rpl_route_copy(rpl_rte_t *dstrte, uint16_t cnt);

uint8_t rpl_route_entry_traverse(inst_t inst_id, rpl_route_cb cb, void *arg);

uint8_t rpl_kickout_sta(rpl_rte_t *rte, void *data);
void rpl_router_mg_clear(inst_t inst_id);
void rpl_router_msta_clear(inst_t inst_id);
void rpl_route_clear_nexthop(rpl_addr_t *ip6addr);
void rpl_router_all_clear(inst_t inst_id);
void rpl_route_delete_address(inst_t inst_id, rpl_addr_t *addr);
void rpl_route_npdao(inst_t inst_id, rpl_parent_t *prnt);
void rpl_route_delete_nhop_npdao(rpl_addr_t *nhop, rpl_parent_t *prnt);
void rpl_route_delete_by_address(const rpl_addr_t *addr, uint8_t len);

void rpl_nd6_refresh_dst_cache(rpl_rte_t *rte);
void rpl_nd6_refresh_nhop_cache(rpl_addr_t *nhop);
void rpl_nd6_remove_neighbor_by_mac(const uint8_t *mac, uint8_t maclen);
void rpl_netif_set_rank(uint16_t rank);
#endif /* _ROUTE_MGMT_H_ */
