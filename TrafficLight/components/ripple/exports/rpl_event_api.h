/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL Management APIs. Start/Stop/Configure etc
 * Author: NA
 * Create: 2019-04-03
 */

#ifndef _RPL_EVENT_API_H_
#define _RPL_EVENT_API_H_

#include "ripple.h"

typedef struct {
  uint8_t status; /* check DAO_ACK_STATUS_* status values */
  uint16_t mnid; /* Unique mesh node ID, valid only if status=DAO_ACK_STATUS_OK */
  uint8_t flags;
} rpl_node_join_stat_t;

typedef enum {
  RPL_EVT_UNUSED,

  /*
   * When: On receiving a new instance (typically through first DIO).
   * OnReturn: If the indication returns RPL_FAIL, then the node does not join this instance.
   * Parameter: rpl_cfg_t, Configuation of the instance
   */
  RPL_EVT_JOIN_INSTANCE,

  /*
   * When: Node join status. If node join is success, then the E2E path is established.
   * OnReturn: NA
   * Parameter: rpl_node_join_stat_t
   */
  RPL_EVT_NODE_JOIN_STATUS,

  /*
   * When: New Node routing entry is added or removed.
   * OnReturn: NA
   * Parameter: rpl_routeinfo_t
   */
  RPL_EVT_NODE_ROUTE_UPDATE,

  /*
   * When: New Node routing updated ind on root.
   * OnReturn: NA
   * Parameter: rpl_routeinfo_t
   */
  RPL_EVT_NODE_ROUTE_UPDATE_IND,
  /*
   * When: RPL Join success event.
   * OnReturn: NA
   * Parameter: rpl_routeinfo_t
   */
  RPL_EVT_JOIN_SUCC,
  /*
   * When: Node parent clear.
   * OnReturn: NA
   * Parameter: rpl_routeinfo_t
   */
  RPL_EVT_NODE_PARENT_CLEAR,
  /*
   * When: Node beacon priority reset(except parent clear).
   * OnReturn: NA
   * Parameter: rpl_routeinfo_t
   */
  RPL_EVT_NODE_BEACON_PRI_RESET,
  /*
   * When: route dhcp resource full
   * OnReturn: NA
   * Parameter: rpl_routeinfo_t
   */
  RPL_EVT_NODE_GET_IPV4_ADDR_FAIL,
  /*
   * When: route dhcp resource full for msta connected to this MG
   * OnReturn: NA
   * Parameter: rpl_routeinfo_t
   */
  RPL_EVT_NODE_MSTA_GET_IPV4_ADDR_FAIL,
  /*
   * When: route table full
   * OnReturn: NA
   * Parameter: rpl_routeinfo_t
   */
  RPL_EVT_RT_FULL,
  /*
   * When: mg node numbers full
   * OnReturn: NA
   * Parameter: rpl_routeinfo_t
   */
  RPL_EVT_MG_FULL,
  /*
   * When: RPL Deinit event.
   * OnReturn: NA
   * Parameter: rpl_routeinfo_t
   */
  RPL_EVT_DEINIT
} rpl_event_type_e;

/**
 * @Description: Indicate RPL event
 *
 * @param evt - Event Identified
 * @param instID - Instance on which event is received
 * @param arg - Additional information of the event.
 *
 * @return OK/FAIL
 */
uint8_t rpl_event_indicate(uint16_t evt, inst_t inst_id, void *arg);

void rpl_node_kickout_all(void);
void rpl_node_assoc_ban(void);
void rpl_node_kickout_msta(void);
void rpl_node_reassoc_evt(void);

void rpl_clear_old_buff(void);

#endif /* _RPL_EVENT_API_H_ */
