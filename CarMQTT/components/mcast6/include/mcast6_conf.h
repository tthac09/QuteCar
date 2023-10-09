/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: the configuration of the mcast6.
 * Author: none
 * Create: 2019-07-03
 */

#ifndef __MCAST6_CONF_H__
#define __MCAST6_CONF_H__

#define MCAST6_OK 0              // success
#define MCAST6_FAIL 1            // general failure

#define MCAST6_TRUE 1
#define MCAST6_FALSE 0

#define MCAST6_RESERVED_VALUE 0

#define MCAST6_ADDR_BITS_NUM 128
#define MCAST6_BYTE_BITS_NUM 8

#define MCAST6_ADDR_U32_NUM 4
#define MCAST6_ADDR_U16_NUM 8
#define MCAST6_ADDR_U8_NUM 16

/*
 * if we have lot of downstream targets (if above MCAST6_CONF_ENTRY_MAX_NUM),
 * then MR can say that i am interested in all the MPL traffic by advertizing
 * mcast-prefix::0/64 in the dao target.
 */
#ifndef MCAST6_CONF_ENTRY_MAX_NUM
#define MCAST6_CONF_ENTRY_MAX_NUM 4
#endif

#ifndef MCAST6_CONF_BIG_ENDIAN
#define MCAST6_CONF_BIG_ENDIAN 0
#endif

#endif /* __MCAST6_CONF_H__ */
