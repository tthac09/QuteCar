/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: Platform Network and other APIs
 * Author: NA
 * Create: 2019-04-02
 */

#ifndef _RPL_PLATFORM_API_H_
#define _RPL_PLATFORM_API_H_

#include "ripple.h"
#include "platform.h" /* needed for platform-specific data-structures */

typedef struct {
  uint16_t rank;
  uint16_t link_metric;
  uint16_t path_metric;
  int8_t   rssi;
  uint8_t  isresfull;
  uint8_t  ispref;
} rpl_parent_info_t;

/**
 * @Description: Initialize platform.
 *
 * @param dev - Input param. Device on which ripple is used.
 *
 * @return OK/FAIL
 */
uint8_t rpl_platform_init(rpl_netdev_t *dev);

/**
 * @Description: Deinit Platform i.e. free up all resources
 */
void rpl_platform_deinit(void);

/**
 * @Description: Generate uint32_t random number
 *
 * @return uint32_t random number
 */
uint32_t rpl_platform_rand(void);

/**
 * @Description: Start the timer
 *
 * @param timer - timer handler
 * @param timems - timer duration in ms
 * @param timeoutHandler - timeout handler
 * @param arg - opague value given back in handler
 *
 * @return OK/FAIL
 */
uint8_t rpl_platform_start_timer(rpl_timer_t *timer, uint32_t timems,
                                 void (*tocb)(void *), void *arg, uint8_t periodic);

/**
 * @Description: Stop RPL Timer
 *
 * @param timems - timer duration in ms
 */
void rpl_platform_stop_timer(rpl_timer_t *timer);

/**
 * @Description: Check if timer is expired
 *
 * @param timer - Timer handler
 *
 * @return 0/1
 */
uint8_t rpl_platform_timer_is_expired(const rpl_timer_t *timer);

/**
 * @Description: Send icmp message out
 *
 * @param icmpType - icmp type
 * @param icmpCode - icmp code
 * @param buf - buffer to send. Contains only RPL payload (no icmp header)
 * @param len - buffer length
 * @param pkt - Packet info, src/dst address and iface to use
 */
void rpl_platform_send_icmp_msg(uint8_t icmp_type, uint8_t icmp_code,
                                const uint8_t *buf, uint16_t len, const rpl_pktinfo_t *pkt);

/**
 * @Description: Recv icmp message for which handlers were registered
 *
 * @param icmpType - icmp type
 * @param icmpCode - icmp code
 * @param buf - buffer
 * @param len - buffer length
 */
void rpl_core_on_recv_icmp_msg(uint8_t icmp_type, uint8_t icmp_code,
                               uint8_t *buf, uint16_t len, rpl_pktinfo_t *pkt);

/**
 * @Description: Get the address corresponding to prefix in the prefix itself
 *
 * @param prefix - Prefix to search for
 *
 * @return OK/FAIL
 */
uint8_t rpl_platform_address_get(rpl_prefix_t *prefix);

/**
 * @Description: Set address
 *
 * @param prefix - Prefix to use to set the address
 *
 * @return OK/FAIL
 */
uint8_t rpl_platform_address_set(const rpl_prefix_t *prefix);

/**
 * @Description: Check if the neighbor cache is full
 *
 * @param isroot - Is node root?
 *
 * @return true/false
 */
uint8_t rpl_platform_is_nbr_cache_full(uint8_t isroot);

/**
 * @Description: Add neighbor table entry
 *
 * @param iface - Interface
 * @param addr - IPv6 address
 * @param macAddr - MAC address
 * @param reason - Entry addition reason DIO/DAO/SEC/other
 *
 * @return OK/FAIL
 */
uint8_t rpl_platform_add_neighbor(void *iface, const rpl_addr_t *addr,
                                  const rpl_linkaddr_t *macaddr, uint8_t reason);

/**
 * @Description: Remove neighbor table entry
 *
 * @param addr - Address
 *
 * @return OK/FAIL
 */
uint8_t rpl_platform_rem_neighbor(const rpl_addr_t *addr);

/**
 * @Description: Inform RPL layer about the link layer event
 *
 * @param evt - Event info
 *
 * @return OK/FAIL
 */
uint8_t rpl_platform_link_event(const rpl_linkinfo_t *evt);

/**
 * @Description: Iterate over the route table
 *
 * @param state - State maintained internally by API. Initialize this to zero on first iteration.
 * @param tgt - Output target
 * @param nhop - Output nexthop
 * @param lt - output lifetime
 * @param mnid - output Mesh node ID of the target. Applicable only on root node.
 *
 * @return RPL_OK if route entry found, RPL_FAIL if not found.
 */
uint8_t rpl_platform_get_next_rte(int *state, rpl_addr_t *tgt, rpl_addr_t *nhop,
                                  uint16_t *lt, rpl_mnode_id_t *mnid);

/**
 * @Description: Get Mac address of the mesh interface
 *
 * @param iface - Input interface for which MAC address is to be sought
 * @param lladdr - Output link address
 *
 * @return OK/FAIL
 */
uint8_t rpl_platform_get_mac_address(rpl_netdev_t *dev, rpl_linkaddr_t *lladdr);

/**
 * @Description: Get Route table entries size and count
 *
 * @param cntMax - output total count of Route table. can be NULL.
 * @param cntInuse - output count of Route table inuse. can be NULL.
 */
void rpl_core_route_table_entry_cnt(uint16_t *const cnt_max, uint16_t *const cnt_inuse);

/**
 * @Description: Get next hop entry for the dst
 *
 * @param instID - RPLInstanceID
 * @param dst - Input destination IP address
 * @param nhop - Next hop for the destination
 *
 * @return OK if routing entry found
 */
uint8_t rpl_core_rtable_get_nexthop(inst_t inst_id, const rpl_addr_t *dst, rpl_addr_t *nhop);

/**
 * @Description: Remove all parents associated with given next hop address.
 * This is triggered when nbr is evicted from the nbr cache.
 *
 * @param addr - Next hop address
 */
void rpl_core_rem_parent_by_address(const rpl_addr_t *addr);

/**
 * @Description: Remove routing entries based on parent address
 *
 * @param nhop - Next hop address
 */
void rpl_core_rem_routes_by_nexthop(const rpl_addr_t *nhop);

/**
 * @Description: Iterate through the set of parents. Note that this api works
 * only if the instances and dags in use is just one. If more dags are in use
 * then the api fails.
 *
 * @param state - Internal state needed for API. Initialize to zero during first iteration.
 * @param addr - Output Parent address
 * @param info - Output Parent info
 *
 * @return OK/FAIL
 */
uint8_t rpl_core_get_next_parent_entry(int *state, rpl_addr_t *addr, rpl_parent_info_t *info);

uint8_t rpl_behind_mbr_node(const rpl_addr_t *target);
uint8_t rpl_nonmesh_node(const rpl_addr_t *target);
uint8_t rpl_behind_mbr_solicited_node(const rpl_addr_t *target);
uint8_t rpl_nonmesh_solicited_node(const rpl_addr_t *target);

/**
 * @Description: Write to persistent block
 *
 * @param buf - buffer to write
 * @param buflen - length of buffer
 * @param off - offset of the buffer
 *
 * @return OK/FAIL
 */
uint8_t rpl_platform_buf_write(const uint8_t *buf, uint16_t buflen, uint16_t off);

/**
 * @Description: Read from persistent block
 *
 * @param buf - Buffer to read from
 * @param buflen - length of buffer
 * @param off - offset of the buffer
 *
 * @return OK/FAIL
 */
uint8_t rpl_platform_buf_read(uint8_t *buf, uint16_t buflen, uint16_t off);

/**
 * @Description: Clear the persistent block
 */
void rpl_platform_buf_clear(void);

/**
 * @Description: Get RPL persistent storage buffer size
 *
 * @return Buffer size
 */
uint16_t rpl_core_persist_store_blk_size(void);

/**
 * @Description: Get DAG related details
 *
 * @param instID - if *instID != RPL_INSTID_NA, then it will check for the existence of the *instID,
 *                 if it is NA then it will return the first inuse instance.
 * @param rank - Rank of the node in that instance
 * @param dagID - DODAGID of the DAG
 *
 * @return OK/FAIL
 */
uint8_t rpl_core_get_dag_details(inst_t *inst_id, uint16_t *rank, rpl_addr_t *dag_id, uint8_t pkt_up);

/**
 * @Description: Set Default route
 *
 * @param instID - RPLInstanceID
 * @param addr - default address to route to
 *
 * @return OK/FAIL
 */
uint8_t rpl_platform_update_def_route(inst_t inst_id, const rpl_addr_t *addr);

/**
 * @Description: Get Default route set by Ripple. Essentially this is the
 * link-local address of the preferred ripple parent.
 *
 * @param instID - Instance ID. Use RPL_INSTID_NA for using first inuse dag.
 * @param addr - IPv6 address of the default route
 *
 * @return OK/FAIL
 */
uint8_t rpl_core_get_def_route(inst_t inst_id, rpl_addr_t *addr);

rpl_netdev_t *rpl_platform_get_dev(void);
void rpl_platform_set_dev(rpl_netdev_t *dev);
uint8_t rpl_platform_remove_peer(rpl_linkaddr_t *lladdr);

void *rpl_platform_malloc(uint16_t size);
void rpl_platform_free(void *p);

uint8_t rpl_lpop_is_greater(uint8_t a, uint8_t b);
uint8_t rpl_delete_ipv4addr_by_mnid(uint8_t mnid);
uint8_t rpl_delete_ipv4addr(const rpl_linkaddr_t *lladdr, uint8_t mnid);
uint32_t *rpl_request_ipv4addr(const rpl_addr_t *ip6addr, const rpl_linkaddr_t *lladdr,
                               uint8_t dao_sn, uint32_t lifetime, uint32_t *ipv4);

void rpl_nd6_send_ra(const rpl_addr_t *ipv6addr, uint8_t flags);
void rpl_enable_ra_set(uint8_t enable);

uint8_t rpl_process_dao_delegate_target(const rpl_prefix_t *tgt,
                                        const rpl_addr_t *addr, const rpl_linkaddr_t *lladdr);

uint8_t rpl_target_route_delete(const rpl_prefix_t *tgt);

uint8_t rpl_peer_lookup(const uint8_t *mac, uint8_t mac_len, uint8_t is_mesh);

uint8_t is_rpl_running(void);

#endif /* _RPL_PLATFORM_API_H_ */
