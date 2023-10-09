 /*
  * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
  * Description: RPL DAG and Instance management
  * Author: NA
  * Create: 2019-04-02
  */

#ifndef _RPL_DAG_H_
#define _RPL_DAG_H_

#include "trickle.h" /* needed for rpl_trickle_Timer_t */

struct rpl_instance_s;
struct rpl_ofhandler_s;

#define RANK_INFINITE 0xffff /* Infinite Rank */

#define METRIC_ADDITIVE 0
#define METRIC_MAXIMUM 1
#define METRIC_MINIMUM 2
#define METRIC_MULTIPLICATIVE 3

#define IS_LOCAL_INSTANCE(id) (!!((id) & (1 << ((sizeof(inst_t) * 8) - 1))))
#define RPL_PREFIX_MNID_INDEX_U16 3
#define RPL_PREFIX_MNID_INDEX_U8 9
#define RPL_PREFIX_MNID_MARK  0xff
#define RPL_PREFIX_MNID_LEN 16

#define RPL_RDNSS_SERVER_NUM 2
#define RPL_MS_PER_SECOND 1000

#pragma pack(1)
/* Check RFC 6551 Section 2.1 for the details */
typedef struct {
  uint8_t type; /* Container type */
  uint8_t res : 5;
  uint8_t p : 1; /* all the nodes en-route could record the metric? */
  uint8_t c : 1; /* Constraint or a metric */
  uint8_t o : 1; /* Optional constraint ? Applicable Only if C=1 */
  uint8_t r : 1; /* Applicable Only if C=0 */
  uint8_t a : 3; /* additive/max/min/multiplicative */
  uint8_t prec : 4; /* Precedence */
  uint8_t len;
  union {
    uint16_t etx; /* Estimated Transmissions */
    uint16_t num_hops; /* Number of hops */
  } obj;
} rpl_metric_container_t;

typedef struct {
  uint16_t rsv;
  uint32_t lifetime;
  rpl_addr_t dns_addr[RPL_RDNSS_SERVER_NUM];
} rpl_dns_option_t;

#define RPL_DAG_STATE_INIT        0
#define RPL_DAG_STATE_NO_PARENT   1
#define RPL_DAG_STATE_DAO_LOOP    2
#define RPL_DAG_STATE_FORWARD     3
#define RPL_DAG_STATE_EXPIRE      4
#define RPL_DAG_STATE_BACKUP      5
#define RPL_DAG_STATE_MAX         6

typedef struct {
  /*
   * IPv6 link-local address with network prefix stripped, since network
   * prefix will always be fe80::. Note that this may not necessarily be the
   * IID or the mac address of the parent node
   */
  rpl_addr_t loc_addr;         /* Parents Link-local address */
  rpl_linkaddr_t mac_addr;     /* Parent's MAC address */
  rpl_addr_t global_addr;      /* Needed for non-storing MOP */
  rpl_metric_container_t mc;   /* Metric information for this parent */
  rpl_dns_option_t dns;
  uint16_t rank;               /* rank of this parent node */
  uint16_t link_acc_metric;    /* link acc metric to this parent node */
  uint16_t link_tx_metric;     /* link tx metric to this parent node */
  uint16_t link_metric;        /* link metric to this parent node */
  uint8_t dtsn;                /* DAO trigger Seq num */
  int8_t sm_rssi;              /* Smoothened RSSI */
  uint8_t sm_lqi;              /* Smoothened LQI */
  uint8_t status;              /* the parent status */
  uint8_t is_res_full : 1;     /* parent node has resources? */
  uint8_t inuse : 1;           /* record in use? */
  uint8_t is_preferred : 1;    /* is the parent preferred? */
  uint16_t dis_timer_cnt;      /* dis timer count */
  uint16_t dis_timer_param;    /* dis timer parameter that dis fsm set */
  uint8_t dis_retry_cnt;       /* Number of times DISs were retried */
  uint8_t cur_dis_state;       /* Current state for DIS FSM */
} rpl_parent_t;

typedef struct {
  rpl_parent_t prnt[RPL_CONF_MAX_PARENTS]; /* Parents of this node in the dag */
  struct rpl_instance_s *inst;             /* RPL instance of the DAG */
  rpl_trickle_timer_t dio_trickle;         /* DIO Trickle time */
  rpl_addr_t dodag_id;                     /* RPL DODAG ID to use */
  rpl_timer_t dao_timer;                   /* Scheduling DAO timer */
  rpl_mnode_id_t mnid;
#if RPL_CONF_DCO_EN
  uint8_t dco_seq;                         /* DCO Sequence number */
#endif
  uint8_t dao_seq;                         /* DAO Sequence number */
  uint8_t last_join_status;
  uint8_t cur_dao_state;                   /* Current state for DAO FSM */
  uint8_t dao_retry_cnt;                   /* Number of times DAOs were retried */
  uint16_t rank;                           /* Node's Rank in the DAG */
  uint16_t old_rank;                       /* Node's OLD Rank in the DAG */
  uint16_t dao_cnt;                        /* Node's DAO send count */
  uint16_t rank_threshold;                 /* Node's rank threshold in the DAG */
  uint8_t dodag_ver_num;                   /* DODAG Version Number to use */
  uint8_t dtsn_out;                        /* DIO DTSN out */
  uint8_t preference;                      /* DAG preference */
  uint8_t path_seq;                        /* Path Sequence number in DAO */
  uint8_t state;
  uint8_t check_state_cnt;
  uint8_t daoack_get_ipv4_fail_cnt;        /* Node get ipv4 fail cnt which daoack status abnomal */
  uint8_t grounded : 1;                    /* is the DODAG grounded? */
  uint8_t inuse : 1;                       /* is the DAG in use? */
  uint8_t metric_updated : 1;              /* Some prnts metrics updated .. Attempt prnt resel in periodicTimer */
  uint8_t mnid_sent : 1;
  uint8_t is_prefer : 1;
} rpl_dag_t;

typedef struct rpl_instance_s {
  struct rpl_ofhandler_s *obj_func;                /* Objective Function handler */
  rpl_dag_t dags[RPL_CONF_MAX_DAGS_PER_INSTANCE];  /* DODAGs in the RPL instance */
  rpl_prefix_t target[RPL_CONF_MAX_TARGETS];       /* Targets advertised by node in DAO */
  rpl_prefix_t prefix[RPL_CONF_MAX_PREFIXES];      /* Prefixes advertised in DIO */
  rpl_cfg_t cfg;                                   /* RPL Instance configuration */
  inst_t inst_id;                                  /* RPL Instance ID */
  rpl_mnode_id_t mnid;                             /* unique id in the rpl */
  uint8_t mode;                                    /* Mode of operation */
  uint8_t inuse : 1;                               /* in use? */
  uint8_t isroot : 1;                              /* is the node root for this instance */
} rpl_instance_t;
#pragma pack()

typedef struct {
  rpl_cfg_t cfg;                               /* DAG config */
  rpl_addr_t dodag_id;                         /* DAG ID */
  rpl_prefix_t prefix[RPL_CONF_MAX_PREFIXES];  /* prefixes advertized */
  rpl_metric_container_t mc;                   /* metrics of DIO sender */
  rpl_dns_option_t dns;
  uint8_t status;                              /* node status of DIO sender */
  uint16_t rank;                               /* rank of DIO sender */
  inst_t inst_id;                              /* RPL Instance ID */
  uint8_t preference;                          /* DAG preference */
  uint8_t dodag_ver_num;                       /* DAG version */
  uint8_t dtsn;                                /* DTSN of the sender */
  uint8_t grounded : 1;                        /* Is DAG grounded? */
  uint8_t is_res_full : 1;                     /* resource available? */
} rpl_dio_t;

#define RACK_SUPP(cfg) ((cfg)->rack_retry > 0)

/* Reason parameter in ScheduleDAO */
#define DAO_TIMER_DELAYDAO 1
#define DAO_TIMER_RACK_OR_ACK 2
#define DAO_TIMER_RT_LIFETIME 3
#define DAO_TIMER_FAIL_PERIODIC_TRY 4

/* Reason parameter in ScheduleDIS */
#define DIS_TIMER_WAIT_DIO_TIMEOUT 1
#define DIS_TIMER_RT_LIFETIME 2
#define DIS_TIMER_ABNORMAL_PERIODIC_TRY 3

#define RPL_GET_DAG_FLAG_CREATE 1
#define RPL_GET_DAG_FLAG_ROOT 2
#define RPL_GET_DAG_FLAG_MDAGS 4

#define RPL_SET_INST_FLAG_ROOT 1

rpl_instance_t *rpl_get_instance(inst_t inst_id, uint8_t create_flag);
rpl_dag_t *rpl_get_instance_dag(inst_t inst_id, const rpl_addr_t *dag_id);
rpl_dag_t *rpl_get_dag(rpl_instance_t *inst, const rpl_addr_t *dag_id, uint8_t flags);
void rpl_free_instance(rpl_instance_t *inst, uint8_t forced);
void rpl_free_dag(rpl_dag_t *dag);
void rpl_free_instance_dag(rpl_dag_t *dag);
void rpl_reset_dis_timer(void);
void rpl_reset_dio_trickle(rpl_dag_t *dag);
rpl_dag_t *rpl_get_next_inuse_dag(int *state);
void rpl_free_all_instances(void);
uint8_t rpl_count_all_instances(void);
uint8_t rpl_count_all_active_instances(void);
uint8_t rpl_start_all_instances(void);
uint8_t rpl_process_dio(rpl_dio_t *dio, rpl_pktinfo_t *pkt);
rpl_dag_t *rpl_set_instance(inst_t inst_id, const rpl_cfg_t *cfg, const rpl_addr_t *dag_id, uint8_t flags);
uint8_t rpl_validate_cfg(const rpl_cfg_t *cfg);
void rpl_dao_stop_timer(rpl_dag_t *dag);
void rpl_schedule_dao(rpl_dag_t *dag, uint8_t rack);

void rpl_handle_global_repair(rpl_dag_t *dag);
uint8_t is_node_root_for_any_dag(void);
uint8_t rpl_support_mop(const uint8_t mop);
rpl_dag_t *rpl_get_inst_dag(inst_t inst_id);
void rpl_dags_try_switch(void);
void rpl_dio_check(uint8_t period_sec);
uint8_t rpl_get_dns_info(rpl_dns_option_t *dns);
uint8_t rpl_set_dns_server(const rpl_dns_option_t *dns);
void rpl_dag_set_state(rpl_dag_t *dag, uint8_t state);
void rpl_dag_rem_parent_by_address(rpl_dag_t *dag, const rpl_addr_t *addr);
uint8_t rpl_instance_parent_count(void);
void rpl_target_msta_clear(rpl_dag_t *dag);
void rpl_target_msta_delete_all(rpl_dag_t *dag);
#endif /* _RPL_DAG_H_ */
