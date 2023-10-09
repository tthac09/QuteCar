/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: the APIs of the mcast6 ESMRF.
 * Author: none
 * Create: 2019-07-03
 */

#ifndef __MCAST6_ESMRF_H__
#define __MCAST6_ESMRF_H__

#include <stdint.h>
#include <mcast6_conf.h>

typedef struct mcast6_esmrf_msg {
  uint8_t *m6addr;
  uint16_t port;
  uint8_t *data;
  uint16_t datalen;
  uint8_t *sendaddr;
} mcast6_esmrf_msg_t;

#pragma pack(1)
typedef struct mcast6_esmrf_icmp6_hdr {
  uint32_t addr[MCAST6_ADDR_U32_NUM];
  uint16_t port;
  uint8_t payload[0];
} mcast6_esmrf_icmp6_hdr_t;
#pragma pack()

void mcast6_esmrf_deinit(void);
uint8_t mcast6_esmrf_init(void);

void mcast6_downward(const mcast6_esmrf_msg_t *esmrf_msg);

#endif /* __MCAST6_ESMRF_H__ */

