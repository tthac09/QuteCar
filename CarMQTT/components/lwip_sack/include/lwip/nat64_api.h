/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: NAT64 api
 * Author: NA
 * Create: 2019
 */
#ifndef LWIP_HDR_NAT64_API_H
#define LWIP_HDR_NAT64_API_H

#if LWIP_NAT64
int lwip_nat64_init(char *name, uint8_t len);
int lwip_nat64_deinit(void);
#endif
#endif

