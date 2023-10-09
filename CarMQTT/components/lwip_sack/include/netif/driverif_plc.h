/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2016. All rights reserved.
 * Description: declaration of driverif PLC APIs
 * Author: none
 * Create: 2013-12-22
 */
#ifndef __DRIVERIF_PLC__H__
#define __DRIVERIF_PLC__H__
/**
 *  @file driverif_plc.h
 */
#include "lwip/opt.h"
#include "netif/etharp.h"
#include "lwip/netif.h"
#if defined (__cplusplus) && __cplusplus
extern "C" {
#endif

err_t plcdriverif_init(struct netif *netif);

#if defined (__cplusplus) && __cplusplus
}
#endif

#endif /* __DRIVERIF_PLC__H__ */

