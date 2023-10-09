/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL logging functions
 * Author: NA
 * Create: 2019-04-02
 */

#ifndef _RPL_LOG_H_
#define _RPL_LOG_H_

#ifdef __LITEOS__
#include "shell.h"
#else
#include <stdio.h>
#endif

#ifdef RPL_ENABLE_DIAG_SUPPORT
#include "diag_util.h"
#endif

/* Logging Interfaces */
#define RPL_LOG_DBG 1
#define RPL_LOG_INFO 2
#define RPL_LOG_ERR 3
#define RPL_LOG_MUTE 6

#ifndef RPL_LOG_DEFAULT
#define RPL_LOG_DEFAULT RPL_LOG_MUTE
#endif

#define RPL_LOG_DETAILS 0 /* enabled if you want function:line to be printed with all logs */

static inline char *
get_log_type(uint8_t log_type)
{
  if (log_type == RPL_LOG_DBG) {
    return "DBG ";
  }
  if (log_type == RPL_LOG_INFO) {
    return "INFO";
  }
  if (log_type == RPL_LOG_ERR) {
    return "ERR ";
  }
  return "UNK ";
}

#ifdef RPL_LOG_LEVEL
/*
 * NOTE: IF YOU GET COMPILATION ERROR HERE, REMEMBER TO define LOG_LEVEL in corr
 * .c file
 */
#if RPL_LOG_DETAILS
#include <sys/time.h>
#define RPL_PRN_DETAILS(log_type) do {   \
  struct timeval tv;                     \
  (void)gettimeofday(&tv, NULL);         \
  (void)printf("%s %5ld:%-3ld [%s:%d] ", \
               get_log_type(log_type),   \
               tv.tv_sec % 100000,       \
               tv.tv_usec / 1000,        \
               __FUNCTION__,             \
               __LINE__);                \
} while (0)
#else
#define RPL_PRN_DETAILS(log_type)
#endif

#ifdef RPL_ENABLE_DIAG_SUPPORT
#define RPL_INFO_LOG0(fmt)   diag_layer_msg_i0(0, fmt);
#define RPL_INFO_LOG1(fmt, p1)  diag_layer_msg_i1(0, fmt, (hi_u32)(p1));
#define RPL_INFO_LOG2(fmt, p1, p2)  diag_layer_msg_i2(0, fmt, (hi_u32)(p1), (hi_u32)(p2));
#define RPL_INFO_LOG3(fmt, p1, p2, p3) \
  diag_layer_msg_i3(0, fmt, (hi_u32)(p1), (hi_u32)(p2), (hi_u32)(p3));
#define RPL_INFO_LOG4(fmt, p1, p2, p3, p4) \
  diag_layer_msg_i4(0, fmt, (hi_u32)(p1), (hi_u32)(p2), (hi_u32)(p3), (hi_u32)(p4));

#define RPL_ERR_LOG0(fmt)   diag_layer_msg_e0(0, fmt);
#define RPL_ERR_LOG1(fmt, p1)  diag_layer_msg_e1(0, fmt, (hi_u32)(p1));
#define RPL_ERR_LOG2(fmt, p1, p2)  diag_layer_msg_e2(0, fmt, (hi_u32)(p1), (hi_u32)(p2));
#define RPL_ERR_LOG3(fmt, p1, p2, p3) \
  diag_layer_msg_e3(0, fmt, (hi_u32)(p1), (hi_u32)(p2), (hi_u32)(p3));
#define RPL_ERR_LOG4(fmt, p1, p2, p3, p4) \
  diag_layer_msg_e4(0, fmt, (hi_u32)(p1), (hi_u32)(p2), (hi_u32)(p3), (hi_u32)(p4));

#else
#define RPL_PRN(log_type, ...) do { \
  RPL_PRN_DETAILS(log_type); \
  (void)printf(__VA_ARGS__); \
} while (0)

#define RPL_CRIT(...) RPL_PRN(__VA_ARGS__)

#if RPL_LOG_LEVEL <= RPL_LOG_INFO
#define RPL_INFO(...) RPL_PRN(RPL_LOG_INFO, __VA_ARGS__)
#endif

#if RPL_LOG_LEVEL <= RPL_LOG_ERR
#define RPL_ERR(...) RPL_PRN(RPL_LOG_ERR, __VA_ARGS__)
#endif

#if RPL_LOG_LEVEL <= RPL_LOG_DBG
#define RPL_DBG(...) RPL_PRN(RPL_LOG_DBG, __VA_ARGS__)
#endif

#endif /* RPL_ENABLE_DIAG_SUPPORT */
#endif /* RPL_LOG_LEVEL */

#ifndef RPL_INFO
#define RPL_INFO(...)
#endif

#ifndef RPL_ERR
#define RPL_ERR(...)
#endif

#ifndef RPL_DBG
#define RPL_DBG(...)
#endif

#ifndef RPL_INFO_LOG0
#define RPL_INFO_LOG0  RPL_INFO
#endif

#ifndef RPL_INFO_LOG1
#define RPL_INFO_LOG1  RPL_INFO
#endif

#ifndef RPL_INFO_LOG2
#define RPL_INFO_LOG2  RPL_INFO
#endif

#ifndef RPL_INFO_LOG3
#define RPL_INFO_LOG3  RPL_INFO
#endif

#ifndef RPL_INFO_LOG4
#define RPL_INFO_LOG4  RPL_INFO
#endif

#ifndef RPL_ERR_LOG0
#define RPL_ERR_LOG0  RPL_ERR
#endif

#ifndef RPL_ERR_LOG1
#define RPL_ERR_LOG1  RPL_ERR
#endif

#ifndef RPL_ERR_LOG2
#define RPL_ERR_LOG2  RPL_ERR
#endif

#ifndef RPL_ERR_LOG3
#define RPL_ERR_LOG3  RPL_ERR
#endif
#ifndef RPL_ERR_LOG4
#define RPL_ERR_LOG4  RPL_ERR
#endif

#define RPL_UCOND_RETURN(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR(__VA_ARGS__); \
    return; \
  } \
} while (0)

#define RPL_UCOND0_RETURN(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG0(__VA_ARGS__); \
    return; \
  } \
} while (0)

#define RPL_UCOND_RETURN_NULL(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR(__VA_ARGS__); \
    return NULL; \
  } \
} while (0)

#define RPL_UCOND0_RETURN_NULL(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG0(__VA_ARGS__); \
    return NULL; \
  } \
} while (0)

#define RPL_UCOND2_RETURN_NULL(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG2(__VA_ARGS__); \
    return NULL; \
  } \
} while (0)

#define RPL_UCOND_RETURN_FAIL(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR(__VA_ARGS__); \
    return RPL_FAIL; \
  } \
} while (0)

#define RPL_UCOND0_RETURN_FAIL(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG0(__VA_ARGS__); \
    return RPL_FAIL; \
  } \
} while (0)

#define RPL_UCOND1_RETURN_FAIL(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG1(__VA_ARGS__); \
    return RPL_FAIL; \
  } \
} while (0)

#define RPL_UCOND2_RETURN_FAIL(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG2(__VA_ARGS__); \
    return RPL_FAIL; \
  } \
} while (0)

#define RPL_UCOND3_RETURN_FAIL(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG3(__VA_ARGS__); \
    return RPL_FAIL; \
  } \
} while (0)

#define RPL_UCOND_RETURN_ZERO(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR(__VA_ARGS__); \
    return 0; \
  } \
} while (0)

#define RPL_UCOND0_RETURN_ZERO(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG0(__VA_ARGS__); \
    return 0; \
  } \
} while (0)

#define RPL_UCOND1_RETURN_ZERO(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG1(__VA_ARGS__); \
    return 0; \
  } \
} while (0)

#define RPL_UCOND_GOTO(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR(__VA_ARGS__); \
    goto failure; \
  } \
} while (0)

#define RPL_UCOND0_GOTO(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG0(__VA_ARGS__); \
    goto failure; \
  } \
} while (0)

#define RPL_UCOND1_GOTO(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG1(__VA_ARGS__); \
    goto failure; \
  } \
} while (0)

#define RPL_UCOND2_GOTO(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG2(__VA_ARGS__); \
    goto failure; \
  } \
} while (0)

#define RPL_UCOND3_GOTO(cond, ...) do { \
  if (!(cond)) { \
    RPL_ERR_LOG3(__VA_ARGS__); \
    goto failure; \
  } \
} while (0)

#endif /* _RPL_LOG_H_ */
