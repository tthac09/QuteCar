/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL Management APIs. Start/Stop/Configure etc
 * Author: NA
 * Create: 2019-04-03
 */

#ifndef _RPL_MGMT_API_H_
#define _RPL_MGMT_API_H_

#include "ripple.h"

/**
 * @Description: RPL Initialization Interface. Note that init is separate from
 * start interface because an application might choose to init in advance but
 * start only on a particular trigger.
 *
 * @return OK/FAIL
 */
uint8_t rpl_mgmt_init(rpl_netdev_t *dev);

/**
 * @Description: Deinitialize the RPL subsystem. Stops all instances and cleans
 * up all platform resources.
 */
void rpl_mgmt_deinit(void);

/**
 * @Description: Starts the RPL messaging.
 *
 * @return OK/FAIL
 */
uint8_t rpl_mgmt_start(uint8_t mode);

/**
 * @Description: Stop RPL messaging. This stops all RPL instances but does not
 * cleanup platform resources. RPL Start can be called up again without reinit.
 */
void rpl_mgmt_stop(void);

/**
 * @Description: Get RPL Version
 *
 * @return Version string
 */
char *rpl_mgmt_version(void);

/**
 * @Description: Set RPL root parameters. Required only on root node.
 *
 * @param instID - Instance ID to use
 * @param cfg - RPL Cfg to use. If null, default cfg will be used. Check
 * rpl_mgmt_get_default_cfg function.
 * @param dagID - DODAGID or the non-link local ipv6 address of the root
 *
 * @return OK/FAIL
 */
uint8_t rpl_mgmt_set_root(const inst_t inst_id, const rpl_cfg_t *cfg, rpl_addr_t *dag_id);

/**
 * @Description: Set RPL instance prefix.
 *
 * @param instID - RPL Instance
 * @param prefix - Prefix to set
 * Note1: A rpl instance can set and advertize multiple prefixes. Call this API
 * multiple times with different prefixes to set those prefixes. The max limit
 * is governed by RPL_CONF_MAX_PREFIXES.
 * Note2: This api make sense only on root node.
 *
 * @return OK/FAIL
 */
uint8_t rpl_mgmt_set_prefix(const inst_t inst_id, rpl_prefix_t *prefix);

/**
 * @Description: Get default RPL configuration
 *
 * @param cfg - RPL Configuration structure
 */
void rpl_mgmt_get_default_cfg(rpl_cfg_t *cfg);

/**
 * @Description: Set rpl configuration corresponding to the instance
 *
 * @param instID - Instance ID
 * @param cfg - Configuration to set
 *
 * @return OK/FAIL
 */
uint8_t rpl_mgmt_set_cfg(const inst_t inst_id, const rpl_cfg_t *cfg);

/**
 * @Description: Add target to be advertised in the network.
 *
 * @param instID - Instance ID for which the target is to be advertised
 * @param target - Prefix to be advertised
 *
 * @return OK/FAIL
 */
uint8_t rpl_mgmt_add_target(const inst_t inst_id, rpl_prefix_t *target, uint8_t is_delegate);

uint8_t rpl_mgmt_target_off(const inst_t inst_id, const rpl_prefix_t *tgt);

/**
 * @Description: Remove the previously advertised target
 *
 * @param instID - relevant Instance ID
 * @param target - Prefix to be removed
 *
 * @return OK/FAIL
 */
uint8_t rpl_mgmt_remove_target(const inst_t inst_id, const rpl_prefix_t *target);

/**
 * @Description: Reset Ripple Stats
 */
void rpl_mgmt_reset_stats(void);

/**
 * @Description: Get Ripple Stats
 *
 * @param stat - Stats buffer
 */
void rpl_mgmt_get_stats(rpl_stat_t *stat);

/**
 * @Description: Initiate refresh on the given instance. Refresh results in
 * incrementing of DTSN from the root resulting in every other node resetting
 * its DIO trickle and sending the DAO.
 *
 * @param instID - Instance ID on which global repair is to be initiated.
 *
 * @return OK/FAIL
 */
uint8_t rpl_mgmt_refresh_instance(inst_t inst_id);

/**
 * @Description: Global Repair an instance. Global repair may result in path recalculation and parent re-formulation.
 *               On global repair we increment the DODAGVerNum.
 *
 * @param instID - Instance ID on which global repair is to be initiated.
 *
 * @return OK/FAIL
 */
uint8_t rpl_mgmt_global_repair(inst_t inst_id);
uint8_t rpl_mgmt_global_dao_trigger(inst_t inst_id);

uint8_t rpl_get_prefix(const inst_t inst_id, rpl_prefix_t *prefix);
uint8_t rpl_check_inner_ipaddr(const rpl_addr_t *addr);
uint8_t rpl_get_slaac_addr(rpl_addr_t *addr, const rpl_linkaddr_t *lladdr);
uint8_t rpl_get_link_addr(const rpl_addr_t *addr, rpl_linkaddr_t *lladdr);
#if defined(RPL_MNID_IN_PREFIX) && RPL_MNID_IN_PREFIX
uint8_t rpl_get_mnid_addr(const rpl_linkaddr_t *lladdr, rpl_addr_t *addr);
#endif
#endif /* _RPL_MGMT_API_H_ */
