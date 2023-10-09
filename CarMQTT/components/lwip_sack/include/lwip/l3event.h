/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: implementation for l3_msg_event
 * Author: none
 * Create: 2020
 */

#ifndef LWIP_HDR_L3EVENT_H
#define LWIP_HDR_L3EVENT_H

#include "lwip/opt.h"

/* define the app_callback function point type */
typedef void (*app_callback_fn)(u8_t, void *);

#if !NO_SYS /* don't build if not configured for use in lwipopts.h */
#if LWIP_L3_EVENT_MSG

typedef enum l3_event_msg_type {
#if LWIP_ROUTE_CHANGE_MSG
  L3_EVENT_MSG_ROUTE_CHANGE,
#endif
#if LWIP_RPL_JOIN_SUCC_MSG
  L3_EVENT_MSG_RPL_JOIN_SUCC,
#endif
#if LWIP_RPL_PARENT_CLEAR_MSG
  L3_EVENT_MSG_PARENT_CLEAR,
#endif
#if LWIP_RPL_BEACON_PRI_RESET
  L3_EVENT_MSG_BEACON_PRI_RESET,
#endif
  /* other MSG could be added here */
  L3_EVENT_MSG_MAX
} l3_event_msg_type_e;

/*
 * Func Name: l3_invoke_msg_callback
 */
/**
 * @brief
 *
 *  This is a thread safe API, used to invoke the l3_event_msg
 *
 * @param[IN]  evt_type: the event type
 *             msg: the carry msg for using afterwards, can be NULL.
 *
 * @returns NA \n
 *
 */
void l3_invoke_msg_callback(enum l3_event_msg_type evt_type, void *msg);

/*
 * Func Name: l3_event_msg_callback_reg
 */
/**
 * @brief
 *
 *  This is a thread safe API, used to register the l3_event_msg
 *
 * @param[IN]  evt_type: the event type
 *             app_callback: the register callback function, can be NULL if app unregister or don't want to register.
 *
 * @returns NA \n
 *
 */
void l3_event_msg_callback_reg(enum l3_event_msg_type evt_type, app_callback_fn app_callback);

#endif /* LWIP_L3_EVENT_MSG */
#endif /* !NO_SYS */

#endif /* LWIP_HDR_L3EVENT_H */
