/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL messaging common apis
 * Author: NA
 * Create: 2019-04-05
 */

#ifndef _RPL_MESSAGING_H_
#define _RPL_MESSAGING_H_

#include "rpl_common.h"
#include "route_mgmt.h"

#define ICMP6_RPL 155

/* RPL message types */
#define RPL_CODE_DIS 0x00     /* DAG Information Solicitation */
#define RPL_CODE_DIO 0x01     /* DAG Information Option */
#define RPL_CODE_DAO 0x02     /* Destination Advertisement Option */
#define RPL_CODE_DAO_ACK 0x03 /* DAO acknowledgment */
#define RPL_CODE_DCO 0x08     /* Destination Cleanup Option */
#define RPL_CODE_DCO_ACK 0x09 /* Destination Cleanup Option acknowledgment */

/* RPL control message options. */
#define OPT_PAD1 0
#define OPT_PADN 1
#define OPT_DAG_METRIC_CONTAINER 2
#define OPT_ROUTE_INFO 3
#define OPT_DAG_CONF 4
#define OPT_TARGET 5
#define OPT_TRANSIT 6
#define OPT_SOLICITED_INFO 7
#define OPT_PREFIX_INFO 8
#define OPT_TARGET_DESC 9
#define OPT_RESOURCE_INFO 10
#define OPT_PARENT_STATUS_INFO 11

/* DIO should include the DNS info */
#define RPL_OPT_DNS 0xF1

/* DAO-ACK should include mnid option */
#define RPL_OPT_MNID 0xF2

/* Flags in DAO message */
#define DAO_K_FLAG 0x80
#define DAO_D_FLAG 0x40

/* Flags in Prefix Information Option */
#define PIO_R_FLAG 0x20
#define PIO_A_FLAG 0x40

/* Status field in DAOACK */
#define DAO_ACK_STATUS_OK 0
#define DAO_ACK_STATUS_NAT64_IP4 126
#define DAO_ACK_STATUS_TABLE_FULL 128
#define DAO_ACK_STATUS_MG_FULL 129 /* MG node is full in this dag */
#define DAO_ACK_STATUS_MNID_ALLOC_FAIL 250 /* MBR could not allocate MNID */
#define DCO_ACK_STATUS_RTENTRY_NOT_FOUND 128

#define rpl_get8b(BUF, LEN, VAL) do { \
  (VAL) = (BUF)[(LEN)++]; \
} while (0)

#define rpl_get16b(BUF, LEN, VAL) do { \
  (VAL) = (uint16_t)(((BUF)[(LEN)] << 8) | (BUF)[(LEN) + 1]); \
  (LEN) = (uint16_t)((LEN) + 2); \
} while (0)

#define rpl_get32b(BUF, LEN, VAL) do { \
  (VAL) = (uint32_t)(((BUF)[(LEN)] << 24) | ((BUF)[(LEN) + 1] << 16) | ((BUF)[(LEN) + 2] << 8) | (BUF)[(LEN) + 3]); \
  (LEN) = (uint16_t)((LEN) + 4); \
} while (0)

#define rpl_get_addr(BUF, LEN, ADDR) do {               \
  (void)memcpy(&((ADDR)->a8), (BUF) + (LEN), 16);       \
  (LEN) += 16U;                                         \
} while (0)

#define rpl_set8b(BUF, LEN, VAL) do { \
  (BUF)[(LEN)++] = (uint8_t)(VAL); \
} while (0)

#define rpl_set16b(BUF, LEN, VAL) do { \
  (BUF)[(LEN)++] = (uint8_t)((VAL) >> 8); \
  (BUF)[(LEN)++] = (uint8_t)((VAL) & 0xff); \
} while (0)

#define rpl_set32b(BUF, LEN, VAL) do {              \
  (BUF)[(LEN)++] = (uint8_t)((VAL) >> 24);          \
  (BUF)[(LEN)++] = (uint8_t)(((VAL) >> 16) & 0xff); \
  (BUF)[(LEN)++] = (uint8_t)(((VAL) >> 8) & 0xff);  \
  (BUF)[(LEN)++] = (uint8_t)((VAL) & 0xff);         \
} while (0)

#define rpl_set_addr(BUF, LEN, ADDR) do {               \
  (void)memcpy((BUF) + (LEN), &((ADDR)->a8), 16);       \
  (LEN) += 16U;                                         \
} while (0)

#if RPL_CONF_INSTID == 1
#define rpl_set_inst_id(BUF, LEN, VAL) rpl_set8b((BUF), LEN, VAL)
#define rpl_get_inst_id(BUF, LEN, VAL) rpl_get8b((BUF), LEN, VAL)
#elif RPL_CONF_INSTID == 2
#define rpl_set_inst_id(BUF, LEN, VAL) rpl_set16b((BUF), LEN, VAL)
#define rpl_get_inst_id(BUF, LEN, VAL) rpl_get16b((BUF), LEN, VAL)
#elif RPL_CONF_INSTID == 4
#define rpl_set_inst_id(BUF, LEN, VAL) rpl_set32b((BUF), LEN, VAL)
#define rpl_get_inst_id(BUF, LEN, VAL) rpl_get32b((BUF), LEN, VAL)
#else
#error "Incorrect RPL_CONF_INSTID"
#endif

#define DIO_GROUNDED 0x80
#define DIO_MOP_SHIFT 3
#define DIO_MOP_MASK 0x3c
#define DIO_PREFERENCE_MASK 0x07

void rpl_dis_transmit(const rpl_addr_t *addr);
uint8_t rpl_dio_transmit(rpl_dag_t *dag, const rpl_addr_t *dst);
uint8_t rpl_dao_transmit(rpl_dag_t *dag);
uint8_t rpl_npdao_transmit(rpl_dag_t *dag, rpl_parent_t *prnt);
uint16_t rpl_get_option(uint8_t *opt_type, uint8_t *opt_len, const uint8_t *buf, uint16_t buflen);
uint8_t rpl_npdao_proxy_to_parent(rpl_dag_t *dag, const rpl_rte_t *rte, const rpl_parent_t *prnt);
void rpl_dis_target_miss(const rpl_addr_t *tgt);

#endif /* _RPL_MESSAGING_H_ */
