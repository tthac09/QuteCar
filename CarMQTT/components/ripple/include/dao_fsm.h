/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL DAO FSM
 * Author: NA
 * Create: 2019-04-24
 */

#ifndef _DAO_FSM_H_
#define _DAO_FSM_H_

void rpl_daofsm_on_new_target(rpl_dag_t *);
void rpl_daofsm_on_dao_ack(rpl_dag_t *dag, uint8_t status);
void rpl_daofsm_on_parent_change(rpl_dag_t *dag);
void rpl_daofsm_on_dao_timer_expiry(rpl_dag_t *dag);
void rpl_daofsm_on_refresh_path(rpl_dag_t *dag);

#endif /* _DAO_FSM_H_ */
