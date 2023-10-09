/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: RPL DIS FSM
 * Author: NA
 * Create: 2020-06-05
 */

#ifndef DIS_FSM_H
#define DIS_FSM_H
#include "dag.h"

void rpl_disfsm_on_dis_timer_expiry(rpl_dag_t *dag, rpl_parent_t *prnt);
void rpl_disfsm_on_ucast_dio_recv(rpl_dag_t *dag, rpl_parent_t *prnt, uint8_t status);
void rpl_disfsm_on_mcast_dio_recv(rpl_dag_t *dag, rpl_parent_t *prnt, uint8_t status);
void rpl_disfsm_on_new_parent(rpl_dag_t *dag, rpl_parent_t *prnt);
void rpl_disfsm_on_parent_change(rpl_dag_t *dag, rpl_parent_t *prnt);
#endif /* DIS_FSM_H */
