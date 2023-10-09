/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL Management Interfaces implementation
 * Author: NA
 * Create: 2019-04-02
 */

#ifndef _OF_HELPER_H_
#define _OF_HELPER_H_

#include "dag.h"
#include "parenting.h"

/* DAG Metric Container Object Types, to be confirmed by IANA. */
#define DAG_MC_NONE 0       /* Local identifier for empty MC */
#define DAG_MC_NSA 1        /* Node State and Attributes */
#define DAG_MC_ENERGY 2     /* Node Energy */
#define DAG_MC_HOPCOUNT 3   /* Hop Count */
#define DAG_MC_THROUGHPUT 4 /* Throughput */
#define DAG_MC_LATENCY 5    /* Latency */
#define DAG_MC_LQL 6        /* Link Quality Level */
#define DAG_MC_ETX 7        /* Expected Transmission Count */
#define DAG_MC_LC 8         /* Link Color */

typedef struct rpl_ofhandler_s {
  uint8_t (*init)(void);
  uint16_t (*get_rank)(rpl_parent_t *prnt);
  void (*cleanup)(void);
  void (*get_metric_info)(rpl_parent_t *prnt, rpl_metric_container_t *mc);
  uint16_t (*calc_path_metric)(rpl_parent_t *p);
  rpl_dag_t *(*dag_select)(rpl_dag_t *d1, rpl_dag_t *d2);
  rpl_parent_t *(*best_parent_select)(rpl_parent_t *p1, rpl_parent_t *p2);
  rpl_parent_t *(*bad_parent_select)(rpl_parent_t *p1, rpl_parent_t *p2);
  uint16_t ocp;
} rpl_ofhandler_t;

/**
 * @Description: Get Objective function handler based on OCP
 *
 * @param ocp - Objective Code Point
 *
 * @return Pointer or NULL if fails to find ocp
 */
rpl_ofhandler_t *rpl_of_get(uint16_t ocp);

uint8_t rpl_of_handler_register(rpl_ofhandler_t *handler);

uint8_t rpl_of0_register(void);
uint8_t rpl_mrhof_register(void);

uint8_t rpl_of_init(void);

#endif /* _OF_HELPER_H_ */
