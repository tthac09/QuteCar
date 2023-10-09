/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: the log macros of the mcast6.
 * Author: none
 * Create: 2019-07-03
 */

#ifndef __MCAST6_LOG_H__
#define __MCAST6_LOG_H__

#include <stdio.h>

#define MCAST6_LOG_INFO 1
#define MCAST6_LOG_DBG  2
#define MCAST6_LOG_ERR  3

#define MCAST6_PRINT(fmt, args...) do { \
  (void)printf("[%s][%d] " fmt, __FUNCTION__, __LINE__, ##args); \
} while (0)

#ifndef MCAST6_LOG_LEVEL
#define MCAST6_LOG_LEVEL (MCAST6_LOG_ERR)
#endif /* MCAST6_LOG_LEVEL */

#if (MCAST6_LOG_LEVEL <= MCAST6_LOG_INFO)
#define MCAST6_INFO(fmt, args...) MCAST6_PRINT(fmt, ##args)
#endif

#if (MCAST6_LOG_LEVEL <= MCAST6_LOG_DBG)
#define MCAST6_DBG(fmt, args...) MCAST6_PRINT(fmt, ##args)
#endif

#if (MCAST6_LOG_LEVEL <= MCAST6_LOG_ERR)
#define MCAST6_ERR(fmt, args...) MCAST6_PRINT(fmt, ##args)
#endif

#ifndef MCAST6_INFO
#define MCAST6_INFO(fmt, args...)
#endif /* MCAST6_INFO */

#ifndef MCAST6_DBG
#define MCAST6_DBG(fmt, args...)
#endif /* MCAST6_DBG */

#ifndef MCAST6_ERR
#define MCAST6_ERR(fmt, args...)
#endif /* MCAST6_ERR */
#endif /* __MCAST6_LOG_H__ */

