/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL parent management APIs
 * Author: NA
 * Create: 2019-04-10
 */

#ifndef _PARENTING_H_
#define _PARENTING_H_

rpl_parent_t *rpl_parent_add(rpl_dag_t *dag, rpl_dio_t *dio, rpl_pktinfo_t *pkt, uint8_t flags);
void rpl_parent_remove(rpl_dag_t *dag, rpl_parent_t *prnt);
rpl_parent_t *rpl_get_next_parent(rpl_dag_t *dag, int *state);
uint16_t rpl_parent_get_linkmetric(rpl_parent_t *p);
uint8_t rpl_parent_set_preferred(rpl_dag_t *dag);
rpl_parent_t *rpl_parent_get_preferred(rpl_dag_t *dag);

#endif /* _PARENTING_H_ */
