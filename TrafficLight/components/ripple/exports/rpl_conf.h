/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL configuration constants
 * Author: NA
 * Create: 2019-04-02
 */

#ifndef _RPL_CONF_H_
#define _RPL_CONF_H_

#ifdef OVERRIDE_RPL_CONF
#include OVERRIDE_RPL_CONF
#endif

/*
 * RPL for ipv6 only networks
 * If this is set to 0 then RPL works for IPv4 network only
 * Note that IPv4 support is built only in framework. Actual handling may not be
 * present.
 */
#ifndef RPL_CONF_BIG_ENDIAN
#define RPL_CONF_BIG_ENDIAN 0
#endif

/*
 * RPL for ipv6 only networks
 * If this is set to 0 then RPL works for IPv4 network only
 * Note that IPv4 support is built only in framework. Actual handling may not be
 * present.
 */
#ifndef RPL_CONF_IP6
#define RPL_CONF_IP6 1
#endif

/*
 * RPL maximum instances.
 * Before changing this value you should understand:
 * 1. difference between Instance and DODAG
 * 2. multiple DODAGs in same instance will have same OF
 * 3. multiple instances may or may not have same OF
 * 4. node set is exclusive in multiple DODAG in same instance
 * 5. there could be overlapping nodes in multiple instances
 * 6. Typical use of multiple instance is to satisfy different app objectives
 * 7. Typical use of multiple DODAGs in same instance is to cluster nodes wrt
 * different roots
 * If you appreciate all of above, then go ahead with the change, else leave it
 * to the experts.
 */
#ifndef RPL_CONF_MAX_INSTANCES
#define RPL_CONF_MAX_INSTANCES 1
#endif

#ifndef RPL_CONF_MULTI_DAGS
#define RPL_CONF_MULTI_DAGS 1
#endif

/*
 * Maximum DODAGs per RPL instance. Expert-guidance needed.
 */
#if RPL_CONF_MULTI_DAGS
#ifndef RPL_CONF_MAX_DAGS_PER_INSTANCE
#define RPL_CONF_MAX_DAGS_PER_INSTANCE 2
#endif
#else
#ifndef RPL_CONF_MAX_DAGS_PER_INSTANCE
#define RPL_CONF_MAX_DAGS_PER_INSTANCE 1
#endif
#endif
/*
 * Maximum prefixes that can advertised by RPL in DIO message
 */
#ifndef RPL_CONF_MAX_PREFIXES
#define RPL_CONF_MAX_PREFIXES 2
#endif

/*
 * Conf OF0 as defined in RFC 6552
 */
#ifndef RPL_CONF_OF0
#define RPL_CONF_OF0 0
#endif

/*
 * Conf MRHOF (Min Rank with Hysteresis Obj func) as defined in RFC 6719
 */
#ifndef RPL_CONF_MRHOF
#define RPL_CONF_MRHOF 1
#endif

/*
 * Enable Storing MOP, enables both storing with and without multicast
 */
#ifndef RPL_CONF_STORING
#define RPL_CONF_STORING 1
#endif

/*
 * Enable Non-Storing MOP
 */
#ifndef RPL_CONF_NONSTORING
#define RPL_CONF_NONSTORING 0
#endif

/*
 * RACK retry timer interval
 */
#ifndef RPL_CONF_RACK_TIMER_VAL
#define RPL_CONF_RACK_TIMER_VAL 10000 /* retry in 10sec */
#endif

/*
 * RACK retry count. Set this value to 0 to disable RACK
 */
#ifndef RPL_CONF_RACK_RETRY
#define RPL_CONF_RACK_RETRY 3
#endif

/*
 * DIO Interval min. As per RFC 6550, the value should be 3 (8ms), which is far
 * too low. I set it to 9, which is 2^9=512ms
 */
#ifndef RPL_CONF_DIO_INTERVAL_MIN
#define RPL_CONF_DIO_INTERVAL_MIN 9
#endif

/*
 * DIO Redundancy. As per RFC 6550, the value should be 10, which is far too
 * high (depends on deployment though)
 */
#ifndef RPL_CONF_DIO_REDUNDANCY
#define RPL_CONF_DIO_REDUNDANCY 2
#endif

/*
 * DIO Interval maximum.
 *   Imin | Imax | Max Interval
 *   3    | 20   | 2.3 hrs     ... defaults as per RPL RFC 6550
 *   10   | 13   | 2.3 hrs     ... as per Imin configured here
 *   9    | 14   | 2.3 hrs     ... as per Imin configured here
 * ((1<<Imin) << Imax) == 8388608 (2.3 hrs)
 */
#ifndef RPL_CONF_DIO_INTERVAL_DBL
#define RPL_CONF_DIO_INTERVAL_DBL 11
#endif

/*
 * DIS sending interval (unit: seconds)
 */
#ifndef RPL_CONF_DIS_INTERVAL
#define RPL_CONF_DIS_INTERVAL 20
#endif

#ifndef RPL_CONF_DIS_COUNT
#define RPL_CONF_DIS_COUNT 2
#endif

#ifndef RPL_CONF_DIS_RETRY_COUNT
#define RPL_CONF_DIS_RETRY_COUNT 2
#endif

/*
 * Minimum rank increase on every hop
 */
#ifndef RPL_CONF_MIN_RANKINC
#define RPL_CONF_MIN_RANKINC 256
#endif

/*
 * Maximum rank increase
 */
#ifndef RPL_CONF_MAX_RANKINC
#define RPL_CONF_MAX_RANKINC (64 * RPL_CONF_MIN_RANKINC)
#endif

#ifndef RPL_CONF_CHECK_RANKINC
#define RPL_CONF_CHECK_RANKINC (RPL_CONF_MIN_RANKINC / 16)
#endif

#ifndef RPL_CONF_CHECK_RANK_INTERVAL
#define RPL_CONF_CHECK_RANK_INTERVAL 3
#endif

#ifndef RPL_CONF_DIS_QUERY_INTERVAL
#define RPL_CONF_DIS_QUERY_INTERVAL 90 /* dis query period 90 s */
#endif

/*
 * Default Lifetime. Used for routing entry lifetime.
 */
#ifndef RPL_CONF_DEF_LIFETIME
#define RPL_CONF_DEF_LIFETIME 10
#endif

/*
 * Default rank threshold. Used for node rank default threshold.
 */
#ifndef RPLCONF_DEF_RANK_THRESHOLD
#define RPLCONF_DEF_RANK_THRESHOLD 650
#endif

/*
 * Default Lifetime unit. Used for routing entry lifetime.
 */
#ifndef RPL_CONF_DEF_LIFETIME_UNIT
#define RPL_CONF_DEF_LIFETIME_UNIT 60
#endif

/*
 * Maximum parents supported per DAG
 */
#ifndef RPL_CONF_MAX_PARENTS
#define RPL_CONF_MAX_PARENTS 4
#endif

/*
 * DelayDAO configuration as per Section 17 of RFC 6550
 */
#ifndef RPL_CONF_DELAYDAO
#define RPL_CONF_DELAYDAO 1000 /* in millisec */
#endif

/*
 * Configure DAO-ACK timer. If configure 0, then disable DAOACK
 */
#ifndef RPL_CONF_DAOACK_TIMER
#define RPL_CONF_DAOACK_TIMER 0
#endif

/*
 * Enable/Disable DCO-ACK.
 */
#ifndef RPL_CONF_DCOACK_EN
#define RPL_CONF_DCOACK_EN 0
#endif

/*
 * Configure Failure Periodic DAO timer.
 */
#ifndef RPL_CONF_DAOACK_FAILURE_PERIODIC_RETRY
#define RPL_CONF_DAOACK_FAILURE_PERIODIC_RETRY 10000 /* retry after 10 s */
#endif

/*
 * Configure Dag state abnomal DIS timer.
 */
#ifndef RPL_CONF_ABNORMAL_PERIODIC_RETRY
#define RPL_CONF_ABNORMAL_PERIODIC_RETRY 9000 /* retry after 9 s */
#endif

/*
 * Configure No-Path DAO
 */
#ifndef RPL_CONF_NPDAO_EN
#define RPL_CONF_NPDAO_EN 1 /* 0/1 .. enable/disable */
#endif

/*
 * Configure Destination Cleanup Object (DCO).
 * https://tools.ietf.org/html/draft-ietf-roll-efficient-npdao-10
 */
#ifndef RPL_CONF_DCO_EN
#define RPL_CONF_DCO_EN 1 /* 0/1 .. enable/disable */
#endif

/*
 * Configure instance-id size. RPL standard defines instance-id size as 1B, but
 * we have PDT (Transport-PDT using ROADM/OLA devices) which requires more than
 * 200 instances in the same network. In such case instance-id can be
 * configured to be 2 (for uint16_t) or 4 (uint32_t)
 */
#ifndef RPL_CONF_INSTID
#define RPL_CONF_INSTID 1 /* possible values 1 or 2 or 4 */
#endif

/*
 * Self Targets advertised by the node in the DAO message. This is what we call
 * DAO-Delegation.
 */
#ifndef RPL_CONF_MAX_TARGETS
#define RPL_CONF_MAX_TARGETS 9
#endif

/*
 * Initial link metric to use. When DIO is received the only info regarding
 * link i can use is RSSI. If RSSI is not received/not set, then we need to use
 * some initial metric configuration, as defined here.
 */
#ifndef RPL_CONF_INIT_LINK_METRIC
#define RPL_CONF_INIT_LINK_METRIC 2
#endif

/*
 * RSSI configuration
 */
#ifndef RPL_CONF_RSSI_WORST
#define RPL_CONF_RSSI_WORST (-90)
#endif

#ifndef RPL_CONF_RSSI_BEST
#define RPL_CONF_RSSI_BEST (-60)
#endif

/*
 * RSSI threshold to consider for DIO. Applies only for DIO which create new
 * instance or parent.
 */
#ifndef RPL_CONF_DIO_RSSI_THRESHOLD
#define RPL_CONF_DIO_RSSI_THRESHOLD (-90)
#endif

#ifndef RPL_CONF_MAX_PATH_COST
#define RPL_CONF_MAX_PATH_COST 32
#endif

/*
 * Mesh Node uniq ID (MNID) configuration
 * Maximum number of mnid that can be allocated. Please note that the start and
 * end value of mnid can be anything. For e.g. mnidStart could be 1 in which
 * case mnidEnd would be 128. Similarly, you could use mnidStart=64, in which
 * case mnidEnd would be 191. Note that mnidStart and mnidEnd values are
 * inclusive in allocating mnid.
 */
#ifndef RPL_CONF_MAX_MNID_COUNT
#define RPL_CONF_MAX_MNID_COUNT 128
#endif

/*
 * Mesh Node uniq ID (MNID) configuration: Start of mnid range allocation
 * Note the start/end values are inclusive.
 */
#ifndef RPL_CONF_MNID_BEG
#define RPL_CONF_MNID_BEG 1
#endif

/*
 * Mesh Node uniq ID (MNID) configuration: End of mnid range allocation
 * Note the start/end values are inclusive.
 */
#ifndef RPL_CONF_MNID_END
#define RPL_CONF_MNID_END 128
#endif

/*
 * RPL's storing MOP has problems with sub-dodag update. There is no elegant of
 * solving that issue. Thus if subdodag has to be updated, the switching parent
 * increments its DTSN and the downstream nodes sees the increment and reset
 * their own trickle timer after incrementing their own dtsn.
 */
#ifndef RPL_CONF_AGGRESSIVE_SUBDODAG_UPDATE
#define RPL_CONF_AGGRESSIVE_SUBDODAG_UPDATE 1
#endif

/*
 * Maximum packet size of an RPL control message
 */
#ifndef RPL_CONF_MAX_PKT_SZ
#define RPL_CONF_MAX_PKT_SZ 512
#endif

/*
 * Max size of routing table
 */
#ifndef RPL_CONF_MAX_ROUTES
#define RPL_CONF_MAX_ROUTES 128
#endif
/*
 * Max MG numbers
 */
#ifndef RPL_CONF_MAX_MG_NUMBERS
#define RPL_CONF_MAX_MG_NUMBERS 64
#endif

#ifndef RPL_CONF_LINK_DAO
#define RPL_CONF_LINK_DAO 1
#endif

#ifndef RPL_CONF_DAO_INFO
#define RPL_CONF_DAO_INFO 1
#endif

#ifndef RPL_CONF_DAO_STORE_INTERVAL
#define RPL_CONF_DAO_STORE_INTERVAL 7
#endif

#ifndef RPL_ROUTE_AGGREGATE_PREFIX
#define RPL_ROUTE_AGGREGATE_PREFIX 1
#endif

#ifndef RPL_ROUTE_MATCH_SAME_LEN
#define RPL_ROUTE_MATCH_SAME_LEN 0
#endif

#ifndef RPL_ROUTE_TARGET_IID
#define RPL_ROUTE_TARGET_IID 0
#endif

#ifndef RPL_DAO_ACK_RESV_MNID
#define RPL_DAO_ACK_RESV_MNID 0
#endif

#ifndef RPL_MNID_IN_PREFIX
#define RPL_MNID_IN_PREFIX 1
#endif

#ifndef RPL_RPI_INSERT_MEMMOVE
#define RPL_RPI_INSERT_MEMMOVE 1
#endif
/*
 * Use persistent storage. Note: Reboot scenarios have issues if this is not supported!
 */
#ifndef RPL_CONF_USE_PERSISTENT_STORAGE
#define RPL_CONF_USE_PERSISTENT_STORAGE 0
#endif

/*
 * dio dtsn lpop init is temporary, and will be increased when time elapse.
 */
#ifndef RPL_CONF_DIO_DTSN_INIT_INTV
#define RPL_CONF_DIO_DTSN_INIT_INTV 5
#endif

/*
 * special handle when path seq is equal to LPOP_INIT
 */
#ifndef RPL_CONF_PATHSEQ_LPOP_INIT
#define RPL_CONF_PATHSEQ_LPOP_INIT 1
#endif

/*
 * Certain platforms such as LWIP allow use of their default routing table. If
 * this is set to 0, then the platform specific default route table will be
 * used. Note that this is applicable only for default routes.
 */
#ifndef RPL_CONF_USE_RIPPLE_DEF_ROUTE_TABLE
#define RPL_CONF_USE_RIPPLE_DEF_ROUTE_TABLE 1
#endif

#ifndef RPL_DAG_SWITCH_THRESHOLD
#define RPL_DAG_SWITCH_THRESHOLD  (2 * RPL_CONF_MAX_RANKINC)
#endif

#ifndef RPL_MAX_LLADDR_LEN
#define RPL_MAX_LLADDR_LEN 6
#endif

#ifndef RPL_CONF_MCAST
#define RPL_CONF_MCAST 0
#endif

#if RPL_CONF_MCAST
#include "mcast6_rpl.h"

#ifndef DAO_SET_MCAST_TARGETS
#define DAO_SET_MCAST_TARGETS mcast6_targets_fill
#endif /* DAO_SET_MCAST_TARGETS */

#ifndef DAO_MCAST_HANDLE
#define DAO_MCAST_HANDLE mcast6_dao_handle
#endif /* DAO_MCAST_HANDLE */

#ifndef TIMER_MCAST_HANDLE
#define TIMER_MCAST_HANDLE mcast6_tmr
#endif /* TIMER_MCAST_HANDLE */

#ifndef MCAST_START_HANDLE
#define MCAST_START_HANDLE mcast6_init
#endif /* MCAST_START_HANDLE */

#ifndef MCAST_STOP_HANDLE
#define MCAST_STOP_HANDLE mcast6_deinit
#endif /* MCAST_STOP_HANDLE */
#endif /* RPL_CONF_MCAST */

#ifndef RPL_CONF_NOTIFY
#define RPL_CONF_NOTIFY 1
#endif

#endif /* _RPL_CONF_H_ */

