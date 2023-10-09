/**
 * @file
 * Sequential API Main thread module
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "lwip/opt.h"

#if !NO_SYS /* don't build if not configured for use in lwipopts.h */

#include "lwip/priv/tcpip_priv.h"
#include "lwip/sys.h"
#include "lwip/memp.h"
#include "lwip/mem.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/pbuf.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"

#if defined(LWIP_HI3861_THREAD_SLEEP) && LWIP_HI3861_THREAD_SLEEP
#include <hi_cpu.h>
#include <hi_task.h>
#endif

#if LWIP_USE_L2_METRICS
#include "lwip/link_quality.h"
#endif

#define TCPIP_MSG_VAR_REF(name)     API_VAR_REF(name)
#define TCPIP_MSG_VAR_DECLARE(name) API_VAR_DECLARE(struct tcpip_msg, name)
#define TCPIP_MSG_VAR_ALLOC(name)   API_VAR_ALLOC(struct tcpip_msg, MEMP_TCPIP_MSG_API, name, ERR_MEM)
#define TCPIP_MSG_VAR_FREE(name)    API_VAR_FREE(MEMP_TCPIP_MSG_API, name)

/* global variables */
static tcpip_init_done_fn tcpip_init_done;
static void *tcpip_init_done_arg;

#if LWIP_TCPIP_CORE_LOCKING
/* The global semaphore to lock the stack. */
sys_mutex_t lock_tcpip_core;
#endif /* LWIP_TCPIP_CORE_LOCKING */

#ifndef DUAL_MBOX

static sys_mbox_t mbox;

#if LWIP_TIMERS
/* wait for a message, timeouts are processed while waiting */
#define TCPIP_MBOX_FETCH(mbox, msg) sys_timeouts_mbox_fetch(mbox, msg)
#else /* LWIP_TIMERS */
/* wait for a message with timers disabled (e.g. pass a timer-check trigger into tcpip_thread) */
#define TCPIP_MBOX_FETCH(mbox, msg) do { \
  UNLOCK_TCPIP_CORE(); \
  sys_mbox_fetch(mbox, msg); \
  LOCK_TCPIP_CORE(); \
} while (0)

#endif /* LWIP_TIMERS */

#define TCPIP_MBOX_POST(mbox, msg)  sys_mbox_post(mbox, msg)
#define TCPIP_MBOX_TRYPOST(mbox, msg) sys_mbox_trypost(mbox, msg)
#define MBOX mbox
#define MBOXPTR &mbox

#else
static sys_dual_mbox_t dmbox;

#if LWIP_TIMERS
/* wait for a message, timeouts are processed while waiting */
#define TCPIP_MBOX_FETCH(mbox, msg) sys_timeouts_dual_mbox_fetch(mbox, msg)
#else /* LWIP_TIMERS */
/* wait for a message with timers disabled (e.g. pass a timer-check trigger into tcpip_thread) */
#define TCPIP_MBOX_FETCH(mbox, msg) do { \
  UNLOCK_TCPIP_CORE();  \
  sys_dual_mbox_fetch(mbox, msg); \
  LOCK_TCPIP_CORE(); \
} while (0)
#endif /* LWIP_TIMERS */

#define TCPIP_MBOX_POST(mbox, msg)  sys_dual_mbox_post(mbox, msg)
#define TCPIP_MBOX_TRYPOST(mbox, msg) sys_dual_mbox_trypost(mbox, msg)
#define MBOXPTR &dmbox
#define MBOX dmbox

#endif /* LWIP_TCPIP_CORE_LOCKING */

int tcpip_init_finish = 0;

/*
 * The main lwIP thread. This thread has exclusive access to lwIP core functions
 * (unless access to them is not locked). Other threads communicate with this
 * thread using message boxes.
 *
 * It also starts all the timers to make sure they are running in the right
 * thread context.
 *
 * @param arg unused argument
 */
static void
tcpip_thread(void *arg)
{
  struct tcpip_msg *msg = NULL;
  LWIP_UNUSED_ARG(arg);

  LWIP_MARK_TCPIP_THREAD();

  if (tcpip_init_done != NULL) {
    tcpip_init_done(tcpip_init_done_arg);
  }

  LOCK_TCPIP_CORE();
  while (1) {                          /* MAIN Loop */
#if defined(LWIP_HI3861_THREAD_SLEEP) && LWIP_HI3861_THREAD_SLEEP
    UNLOCK_TCPIP_CORE();
    hi_cpup_load_check_proc(hi_task_get_current_id(), LOAD_SLEEP_TIME_DEFAULT);
    LOCK_TCPIP_CORE();
#endif
    LWIP_TCPIP_THREAD_ALIVE();
    /* wait for a message, timeouts are processed while waiting */
    (void)TCPIP_MBOX_FETCH(MBOXPTR, (void **)&msg);
    if (msg == NULL) {
      LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: invalid message: NULL\n"));
      LWIP_ASSERT("tcpip_thread: invalid message", 0);
      continue;
    }
    switch (msg->type) {
#if !LWIP_TCPIP_CORE_LOCKING
      case TCPIP_MSG_API:
        LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: API message %p\n", (void *)msg));
        msg->msg.api_msg.function(msg->msg.api_msg.msg);
        break;
#endif /* !LWIP_TCPIP_CORE_LOCKING */
#if ((!LWIP_TCPIP_CORE_LOCKING) || LWIP_API_MESH)
#if !LWIP_TCPIP_CORE_LOCKING
      case TCPIP_MSG_API_CALL:
#endif /* !LWIP_TCPIP_CORE_LOCKING */
#if LWIP_API_MESH
      case TCPIP_MSG_LL_EVENT_CALL:
#endif /* LWIP_API_MESH */
        LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: API CALL message %p\n", (void *)msg));
        msg->msg.api_call.arg->err = msg->msg.api_call.function(msg->msg.api_call.arg);
        sys_sem_signal(msg->msg.api_call.sem);
        break;
#endif /* ((!LWIP_TCPIP_CORE_LOCKING) || LWIP_API_MESH) */

#if !LWIP_TCPIP_CORE_LOCKING_INPUT
      case TCPIP_MSG_INPKT:
        LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: PACKET %p\n", (void *)msg));
        (void)msg->msg.inp.input_fn(msg->msg.inp.p, msg->msg.inp.netif);
        memp_free(MEMP_TCPIP_MSG_INPKT, msg);
        break;
#if LWIP_PLC || LWIP_IEEE802154
      case TCPIP_MSG_INPKT_LLN:
        LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: PACKET %p\n", (void *)msg));
        msg->msg.inp.input_lln_fn(msg->msg.inp.netif, msg->msg.inp.p,
                                  &(msg->msg.inp.st_sender_mac), &(msg->msg.inp.st_recevr_mac));
        memp_free(MEMP_TCPIP_MSG_INPKT, msg);
        break;
#endif /* LWIP_PLC || LWIP_IEEE802154 */
#endif /* !LWIP_TCPIP_CORE_LOCKING_INPUT */
#if LWIP_TCPIP_TIMEOUT && LWIP_TIMERS
      case TCPIP_MSG_TIMEOUT:
        LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: TIMEOUT %p\n", (void *)msg));
        sys_timeout(msg->msg.tmo.msecs, msg->msg.tmo.h, msg->msg.tmo.arg);
        memp_free(MEMP_TCPIP_MSG_API, msg);
        break;
      case TCPIP_MSG_UNTIMEOUT:
        LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: UNTIMEOUT %p\n", (void *)msg));
        sys_untimeout(msg->msg.tmo.h, msg->msg.tmo.arg);
        memp_free(MEMP_TCPIP_MSG_API, msg);
        break;
#endif /* LWIP_TCPIP_TIMEOUT && LWIP_TIMERS */

      case TCPIP_MSG_CALLBACK:
        LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: CALLBACK %p\n", (void *)msg));
        msg->msg.cb.function(msg->msg.cb.ctx);
        memp_free(MEMP_TCPIP_MSG_API, msg);
        break;

      case TCPIP_MSG_CALLBACK_STATIC:
        LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: CALLBACK_STATIC %p\n", (void *)msg));
        msg->msg.cb.function(msg->msg.cb.ctx);
        break;
#if LWIP_LOWPOWER
      /* just wake up thread do nothing */
      case TCPIP_MSG_NA:
        if (msg->msg.lowpower.type == LOW_BLOCK) {
          LOWPOWER_SIGNAL(msg->msg.lowpower.wait_up);
        } else {
          memp_free(MEMP_TCPIP_MSG_LOWPOWER, msg);
        }
        sys_timeout_set_wake_time(LOW_TMR_DELAY);
        break;
#endif
#if LWIP_L3_EVENT_MSG
      case TCPIP_L3_EVENT_MSG:
        /* here we will invoke callback function directly */
        l3_invoke_msg_callback(msg->msg.l3_event_msg.type, msg->msg.l3_event_msg.msg);
        memp_free(MEMP_L3_EVENT_MSG, msg);
        break;
#endif
      default:
        LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: invalid message: %d\n", msg->type));
        LWIP_ASSERT("tcpip_thread: invalid message", 0);
        break;
    }
  }
}

#if LWIP_LOWPOWER
/* send a na msg to wake up tcpip_thread */
void
tcpip_send_msg_na(enum lowpower_msg_type type)
{
  struct tcpip_msg *msg = NULL;
  err_t val;

  /* is not used lowpower mode */
  if ((type != LOW_FORCE_NON_BLOCK) && (get_lowpowper_mod() == LOW_TMR_NORMAL_MOD)) {
    return;
  }
  if (sys_timeout_waiting_long() == 0) {
    return;
  }

  msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_LOWPOWER);
  if (msg == NULL) {
    LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_send_msg_na alloc faild\n"));
    return ;
  }
  if (!sys_dual_mbox_valid(MBOXPTR)) {
    LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_send_msg_na mbox invalid\n"));
    memp_free(MEMP_TCPIP_MSG_LOWPOWER, msg);
    return ;
  }

  /* just wake up thread if nonblock */
  msg->type = TCPIP_MSG_NA;
  msg->msg.lowpower.type = type;

  if (type == LOW_BLOCK) {
    LOWPOWER_SEM_NEW(msg->msg.lowpower.wait_up, val);
    if (val != ERR_OK) {
      LWIP_DEBUGF(TCPIP_DEBUG, ("alloc sem faild\n"));
      memp_free(MEMP_TCPIP_MSG_LOWPOWER, msg);
      return;
    }
  }

  if (TCPIP_MBOX_TRYPOST(MBOXPTR, msg) != ERR_OK) {
    if (type == LOW_BLOCK) {
      LOWPOWER_SEM_FREE(msg->msg.lowpower.wait_up);
    }
    memp_free(MEMP_TCPIP_MSG_LOWPOWER, msg);
    LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_send_msg_na post faild\n"));
    return;
  }

  if (type == LOW_BLOCK) {
    LOWPOWER_SEM_WAIT(msg->msg.lowpower.wait_up);
    LOWPOWER_SEM_FREE(msg->msg.lowpower.wait_up);
    memp_free(MEMP_TCPIP_MSG_LOWPOWER, msg);
  }
}
#endif

#if LWIP_L3_EVENT_MSG
void
tcpip_send_msg_l3_event(enum l3_event_msg_type type, void *rinfo)
{
  struct tcpip_msg *msg = NULL;
  msg = (struct tcpip_msg *)memp_malloc(MEMP_L3_EVENT_MSG);
  if (msg == NULL) {
    LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_send_msg_l3_event alloc faild\n"));
    return;
  }

  msg->type = TCPIP_L3_EVENT_MSG;
  msg->msg.l3_event_msg.type = type;
  (void)rinfo;
  /* post the well-structed messge to the mbox */
  if (TCPIP_MBOX_TRYPOST(MBOXPTR, msg) != ERR_OK) {
    memp_free(MEMP_L3_EVENT_MSG, msg);
    LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_send_msg_l3_event post faild\n"));
    return;
  }
}
#endif /* LWIP_L3_EVENT_MSG */

/*
 * Pass a received packet to tcpip_thread for input processing
 *
 * @param p the received packet
 * @param inp the network interface on which the packet was received
 * @param input_fn input function to call
 */
err_t
tcpip_inpkt(struct pbuf *p, struct netif *inp, netif_input_fn input_fn)
{
#if LWIP_TCPIP_CORE_LOCKING_INPUT
  err_t ret;
#else
  struct tcpip_msg *msg = NULL;
#endif

#if LWIP_TCPIP_CORE_LOCKING_INPUT
  LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_inpkt: PACKET %p/%p\n", (void *)p, (void *)inp));
#if LWIP_LOWPOWER
  tcpip_send_msg_na(LOW_BLOCK);
#endif
  LOCK_TCPIP_CORE();
  ret = input_fn(p, inp);
  UNLOCK_TCPIP_CORE();
  return ret;
#else /* LWIP_TCPIP_CORE_LOCKING_INPUT */

  if (!sys_dual_mbox_valid(MBOXPTR)) {
    return ERR_VAL;
  }

  msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_INPKT);
  if (msg == NULL) {
    return ERR_MEM;
  }

  msg->type = TCPIP_MSG_INPKT;
  msg->msg.inp.p = p;
  msg->msg.inp.netif = inp;
  msg->msg.inp.input_fn = input_fn;
  if (TCPIP_MBOX_TRYPOST(MBOXPTR, msg) != ERR_OK) {
    memp_free(MEMP_TCPIP_MSG_INPKT, msg);
    return ERR_MEM;
  }
  return ERR_OK;
#endif /* LWIP_TCPIP_CORE_LOCKING_INPUT */
}

/**
 * Pass a received packet to tcpip_thread for input processing
 *
 * @param p the received packet
 * @param inp the network interface on which the packet was received
 * @param input_fn input function to call
 */
#if LWIP_PLC || LWIP_IEEE802154
err_t
tcpip_lln_inpkt(struct netif *iface, struct pbuf *p,
                struct  linklayer_addr *sender_mac, struct  linklayer_addr *recver_mac,
                netif_lln_input_fn input_lln_fn)
{
#if LWIP_TCPIP_CORE_LOCKING_INPUT
  err_t ret;
#else
  struct tcpip_msg *msg = NULL;
#endif

#if LWIP_TCPIP_CORE_LOCKING_INPUT
  LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_lln_inpkt: PACKET %p/%p\n", (void *)p, (void *)iface));
#if LWIP_LOWPOWER
  tcpip_send_msg_na(LOW_BLOCK);
#endif
  LOCK_TCPIP_CORE();
  ret = input_lln_fn(iface, p, sender_mac, recver_mac);
  UNLOCK_TCPIP_CORE();
  return ret;
#else /* LWIP_TCPIP_CORE_LOCKING_INPUT */

  if (!sys_dual_mbox_valid(MBOXPTR)) {
    return ERR_VAL;
  }

  msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_INPKT);
  if (msg == NULL) {
    return ERR_MEM;
  }

  msg->type = TCPIP_MSG_INPKT_LLN;
  msg->msg.inp.p = p;
  msg->msg.inp.netif = iface;
  msg->msg.inp.input_lln_fn = input_lln_fn;
  (void)memcpy_s(&(msg->msg.inp.st_sender_mac), sizeof(struct  linklayer_addr), sender_mac,
                 sizeof(struct  linklayer_addr));
  (void)memcpy_s(&(msg->msg.inp.st_recevr_mac), sizeof(struct  linklayer_addr), recver_mac,
                 sizeof(struct  linklayer_addr));
  if (TCPIP_MBOX_TRYPOST(MBOXPTR, msg) != ERR_OK) {
    memp_free(MEMP_TCPIP_MSG_INPKT, msg);
    return ERR_MEM;
  }
  return ERR_OK;
#endif /* LWIP_TCPIP_CORE_LOCKING_INPUT */
}

#endif

#if LWIP_API_MESH
err_t
tcpip_linklayer_event_call(tcpip_api_call_fn fn, struct tcpip_api_call_data *call)
{
  err_t err;

  TCPIP_MSG_VAR_DECLARE(msg);

  if (!sys_dual_mbox_valid(MBOXPTR)) {
    return ERR_VAL;
  }
  if (call == NULL) {
    return ERR_VAL;
  }
#if !LWIP_NETCONN_SEM_PER_THREAD
  err = sys_sem_new(&call->sem, 0);
  if (err != ERR_OK) {
    return err;
  }
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

  TCPIP_MSG_VAR_ALLOC(msg);
  TCPIP_MSG_VAR_REF(msg).type = TCPIP_MSG_LL_EVENT_CALL;
  TCPIP_MSG_VAR_REF(msg).msg.api_call.arg = call;
  TCPIP_MSG_VAR_REF(msg).msg.api_call.function = fn;
#if LWIP_NETCONN_SEM_PER_THREAD
  TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = LWIP_NETCONN_THREAD_SEM_GET();
#else /* LWIP_NETCONN_SEM_PER_THREAD */
  TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = &call->sem;
#endif /* LWIP_NETCONN_SEM_PER_THREAD */
  TCPIP_MBOX_POST(MBOXPTR, &TCPIP_MSG_VAR_REF(msg));
  (void)sys_arch_sem_wait(TCPIP_MSG_VAR_REF(msg).msg.api_call.sem, 0);
  TCPIP_MSG_VAR_FREE(msg);

#if !LWIP_NETCONN_SEM_PER_THREAD
  sys_sem_free(&call->sem);
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

  return call->err;
}
#endif /* LWIP_API_MESH */

/**
 * @ingroup lwip_os
 * Pass a received packet to tcpip_thread for input processing with
 * ethernet_input or ip_input. Don't call directly, pass to netif_add()
 * and call netif->input().
 *
 * @param p the received packet, p->payload pointing to the Ethernet header or
 *          to an IP header (if inp doesn't have NETIF_FLAG_ETHARP or
 *          NETIF_FLAG_ETHERNET flags)
 * @param inp the network interface on which the packet was received
 */
err_t
tcpip_input(struct pbuf *p, struct netif *inp)
{
#if LWIP_ETHERNET
  if (inp->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {
    return tcpip_inpkt(p, inp, ethernet_input_list);
  } else
#endif /* LWIP_ETHERNET */
    return tcpip_inpkt(p, inp, ip_input);
}

#if LWIP_PLC || LWIP_IEEE802154
err_t
tcpip_lln_input(struct netif *iface, struct pbuf *p,
                struct linklayer_addr *sendermac,
                struct linklayer_addr *recvermac)
{
#if LWIP_USE_L2_METRICS
  lwip_update_nbr_signal_quality(iface, sendermac, PBUF_GET_RSSI(p),
                                 PBUF_GET_LQI(p));
#endif
#if LWIP_RPL || LWIP_RIPPLE
  if (memcpy_s(p->mac_address, sizeof(p->mac_address), sendermac->addr,
               sendermac->addrlen > sizeof(p->mac_address) ?
               sizeof(p->mac_address) : sendermac->addrlen) != EOK) {
    return ERR_MEM;
  }
#endif
  return tcpip_lln_inpkt(iface, p, sendermac, recvermac, ip_input_lln);
}
#endif

/**
 * Call a specific function in the thread context of
 * tcpip_thread for easy access synchronization.
 * A function called in that way may access lwIP core code
 * without fearing concurrent access.
 *
 * @param function the function to call
 * @param ctx parameter passed to f
 * @param block 1 to block until the request is posted, 0 to non-blocking mode
 * @return ERR_OK if the function was called, another err_t if not
 */
err_t
tcpip_callback_with_block(tcpip_callback_fn function, void *ctx, u8_t block)
{
  struct tcpip_msg *msg = NULL;
  if (tcpip_init_finish == 0) {
    return ERR_IF;
  }

  if (!sys_dual_mbox_valid(MBOXPTR)) {
    return ERR_VAL;
  }

  msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
  if (msg == NULL) {
    return ERR_MEM;
  }

  msg->type = TCPIP_MSG_CALLBACK;
  msg->msg.cb.function = function;
  msg->msg.cb.ctx = ctx;
  if (block) {
    TCPIP_MBOX_POST(MBOXPTR, msg);
  } else {
    if (TCPIP_MBOX_TRYPOST(MBOXPTR, msg) != ERR_OK) {
      memp_free(MEMP_TCPIP_MSG_API, msg);
      return ERR_MEM;
    }
  }
  return ERR_OK;
}

#if LWIP_TCPIP_TIMEOUT && LWIP_TIMERS
/*
 * call sys_timeout in tcpip_thread
 *
 * @param msecs time in milliseconds for timeout
 * @param h function to be called on timeout
 * @param arg argument to pass to timeout function h
 * @return ERR_MEM on memory error, ERR_OK otherwise
 */
err_t
tcpip_timeout(u32_t msecs, sys_timeout_handler h, void *arg)
{
  struct tcpip_msg *msg = NULL;

  LWIP_ASSERT("Invalid mbox", sys_mbox_valid_val(MBOX));

  msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
  if (msg == NULL) {
    return ERR_MEM;
  }

  msg->type = TCPIP_MSG_TIMEOUT;
  msg->msg.tmo.msecs = msecs;
  msg->msg.tmo.h = h;
  msg->msg.tmo.arg = arg;
  TCPIP_MBOX_POST(MBOXPTR, msg);
  return ERR_OK;
}

/*
 * call sys_untimeout in tcpip_thread
 *
 * @param h function to be called on timeout
 * @param arg argument to pass to timeout function h
 * @return ERR_MEM on memory error, ERR_OK otherwise
 */
err_t
tcpip_untimeout(sys_timeout_handler h, void *arg)
{
  struct tcpip_msg *msg = NULL;

  LWIP_ASSERT("Invalid mbox", sys_mbox_valid_val(MBOX));

  msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
  if (msg == NULL) {
    return ERR_MEM;
  }

  msg->type = TCPIP_MSG_UNTIMEOUT;
  msg->msg.tmo.h = h;
  msg->msg.tmo.arg = arg;
  TCPIP_MBOX_POST(MBOXPTR, msg);
  return ERR_OK;
}
#endif /* LWIP_TCPIP_TIMEOUT && LWIP_TIMERS */

/*
 * Sends a message to TCPIP thread to call a function. Caller thread blocks on
 * on a provided semaphore, which ist NOT automatically signalled by TCPIP thread,
 * this has to be done by the user.
 * It is recommended to use LWIP_TCPIP_CORE_LOCKING since this is the way
 * with least runtime overhead.
 *
 * @param fn function to be called from TCPIP thread
 * @param apimsg argument to API function
 * @param sem semaphore to wait on
 * @return ERR_OK if the function was called, another err_t if not
 */
err_t
tcpip_send_msg_wait_sem(tcpip_callback_fn fn, void *apimsg, sys_sem_t *sem)
{
#if LWIP_TCPIP_CORE_LOCKING
  LWIP_UNUSED_ARG(sem);
#if LWIP_LOWPOWER
  tcpip_send_msg_na(LOW_BLOCK);
#endif
  LOCK_TCPIP_CORE();
  fn(apimsg);
  UNLOCK_TCPIP_CORE();
  return ERR_OK;
#else /* LWIP_TCPIP_CORE_LOCKING */
  TCPIP_MSG_VAR_DECLARE(msg);

  LWIP_ASSERT("semaphore not initialized", sys_sem_valid(sem));

  if (!sys_dual_mbox_valid(MBOXPTR)) {
    return ERR_VAL;
  }
  TCPIP_MSG_VAR_ALLOC(msg);
  TCPIP_MSG_VAR_REF(msg).type = TCPIP_MSG_API;
  TCPIP_MSG_VAR_REF(msg).msg.api_msg.function = fn;
  TCPIP_MSG_VAR_REF(msg).msg.api_msg.msg = apimsg;
  TCPIP_MBOX_POST(MBOXPTR, &TCPIP_MSG_VAR_REF(msg));
  (void)sys_arch_sem_wait(sem, 0);
  TCPIP_MSG_VAR_FREE(msg);
  return ERR_OK;
#endif /* LWIP_TCPIP_CORE_LOCKING */
}

/*
 * Synchronously calls function in TCPIP thread and waits for its completion.
 * It is recommended to use LWIP_TCPIP_CORE_LOCKING (preferred) or
 * LWIP_NETCONN_SEM_PER_THREAD.
 * If not, a semaphore is created and destroyed on every call which is usually
 * an expensive/slow operation.
 * @param fn Function to call
 * @param call Call parameters
 * @return Return value from tcpip_api_call_fn
 */
err_t
tcpip_api_call(tcpip_api_call_fn fn, struct tcpip_api_call_data *call)
{
  err_t err;
#if LWIP_TCPIP_CORE_LOCKING
#if LWIP_LOWPOWER
  tcpip_send_msg_na(LOW_BLOCK);
#endif
  LOCK_TCPIP_CORE();
  err = fn(call);
  UNLOCK_TCPIP_CORE();
  return err;
#else /* LWIP_TCPIP_CORE_LOCKING */
  TCPIP_MSG_VAR_DECLARE(msg);

  if (!sys_dual_mbox_valid(MBOXPTR)) {
    return ERR_VAL;
  }

#if !LWIP_NETCONN_SEM_PER_THREAD
  err = sys_sem_new(&call->sem, 0);
  if (err != ERR_OK) {
    return err;
  }
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

  TCPIP_MSG_VAR_ALLOC(msg);
  TCPIP_MSG_VAR_REF(msg).type = TCPIP_MSG_API_CALL;
  TCPIP_MSG_VAR_REF(msg).msg.api_call.arg = call;
  TCPIP_MSG_VAR_REF(msg).msg.api_call.function = fn;
#if LWIP_NETCONN_SEM_PER_THREAD
  TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = LWIP_NETCONN_THREAD_SEM_GET();
#else /* LWIP_NETCONN_SEM_PER_THREAD */
  TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = &call->sem;
#endif /* LWIP_NETCONN_SEM_PER_THREAD */
  TCPIP_MBOX_POST(MBOXPTR, &TCPIP_MSG_VAR_REF(msg));
  (void)sys_arch_sem_wait(TCPIP_MSG_VAR_REF(msg).msg.api_call.sem, 0);
  TCPIP_MSG_VAR_FREE(msg);

#if !LWIP_NETCONN_SEM_PER_THREAD
  sys_sem_free(&call->sem);
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

  return call->err;
#endif /* LWIP_TCPIP_CORE_LOCKING */
}

#ifdef DUAL_MBOX
err_t
tcpip_netifapi_priority(tcpip_api_call_fn fn, struct tcpip_api_call_data *call)
{
#if LWIP_TCPIP_CORE_LOCKING
  err_t err;
#if LWIP_LOWPOWER
  tcpip_send_msg_na(LOW_BLOCK);
#endif
  LOCK_TCPIP_CORE();
  err = fn(call);
  UNLOCK_TCPIP_CORE();
  return err;
#else /* LWIP_TCPIP_CORE_LOCKING */
  TCPIP_MSG_VAR_DECLARE(msg);

  if (!sys_dual_mbox_valid(MBOXPTR)) {
    return ERR_VAL;
  }

#if !LWIP_NETCONN_SEM_PER_THREAD
  err_t err = sys_sem_new(&call->sem, 0);
  if (err != ERR_OK) {
    return err;
  }
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

  TCPIP_MSG_VAR_ALLOC(msg);
  TCPIP_MSG_VAR_REF(msg).type = TCPIP_MSG_API_CALL;
  TCPIP_MSG_VAR_REF(msg).msg.api_call.arg = call;
  TCPIP_MSG_VAR_REF(msg).msg.api_call.function = fn;
#if LWIP_NETCONN_SEM_PER_THREAD
  TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = LWIP_NETCONN_THREAD_SEM_GET();
#else /* LWIP_NETCONN_SEM_PER_THREAD */
  TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = &call->sem;
#endif /* LWIP_NETCONN_SEM_PER_THREAD */
  sys_dual_mbox_post_priority(MBOXPTR, &TCPIP_MSG_VAR_REF(msg));
  (void)sys_arch_sem_wait(TCPIP_MSG_VAR_REF(msg).msg.api_call.sem, 0);
  TCPIP_MSG_VAR_FREE(msg);

#if !LWIP_NETCONN_SEM_PER_THREAD
  sys_sem_free(&call->sem);
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

  return call->err;
#endif /* LWIP_TCPIP_CORE_LOCKING */
}

#endif

#if LWIP_API_RICH
/*
 * Allocate a structure for a static callback message and initialize it.
 * This is intended to be used to send "static" messages from interrupt context.
 *
 * @param function the function to call
 * @param ctx parameter passed to function
 * @return a struct pointer to pass to tcpip_trycallback().
 */
struct tcpip_callback_msg *
tcpip_callbackmsg_new(tcpip_callback_fn function, void *ctx)
{
  struct tcpip_msg *msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
  if (msg == NULL) {
    return NULL;
  }
  msg->type = TCPIP_MSG_CALLBACK_STATIC;
  msg->msg.cb.function = function;
  msg->msg.cb.ctx = ctx;
  return (struct tcpip_callback_msg *)msg;
}

/*
 * Free a callback message allocated by tcpip_callbackmsg_new().
 *
 * @param msg the message to free
 */
void
tcpip_callbackmsg_delete(struct tcpip_callback_msg *msg)
{
  memp_free(MEMP_TCPIP_MSG_API, msg);
}

/*
 * Try to post a callback-message to the tcpip_thread mbox
 * This is intended to be used to send "static" messages from interrupt context.
 *
 * @param msg pointer to the message to post
 * @return sys_mbox_trypost() return code
 */
err_t
tcpip_trycallback(struct tcpip_callback_msg *msg)
{
  if (sys_dual_mbox_valid(MBOXPTR)) {
    return TCPIP_MBOX_TRYPOST(MBOXPTR, msg);
  } else {
    return ERR_VAL;
  }
}
#endif /* LWIP_API_RICH */

/*
 * @ingroup lwip_os
 * Initialize this module:
 * - initialize all sub modules
 * - start the tcpip_thread
 *
 * @param initfunc a function to call when tcpip_thread is running and finished initializing
 * @param arg argument to pass to initfunc
 */
void
tcpip_init(tcpip_init_done_fn initfunc, void *arg)
{
  LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_init: Starting\n"));
  if (tcpip_init_finish) {
    return;
  }

  lwip_init();

  tcpip_init_done = initfunc;
  tcpip_init_done_arg = arg;

#if LWIP_TCPIP_CORE_LOCKING
  if (sys_mutex_new(&lock_tcpip_core) != ERR_OK) {
    LWIP_ASSERT("failed to create lock_tcpip_core", 0);
  }
#endif

#ifndef DUAL_MBOX
  if (sys_mbox_new(MBOXPTR, TCPIP_MBOX_SIZE) != ERR_OK) {
    LWIP_ASSERT("failed to create tcpip_thread mbox", 0);
  }

#else
  if (sys_dual_mbox_new(&dmbox, TCPIP_MBOX_SIZE) != ERR_OK) {
    LWIP_ASSERT("failed to create tcpip_thread mbox", 0);
  }
#endif /* LWIP_TCPIP_CORE_LOCKING */

  (void)sys_thread_new(TCPIP_THREAD_NAME, tcpip_thread, NULL, TCPIP_THREAD_STACKSIZE, TCPIP_THREAD_PRIO);

#ifdef LWIP_DEBUG_INFO
  LWIP_PLATFORM_PRINT("%s\n", NSTACK_VERSION_STR);
#endif /* LWIP_DEBUG_INFO */

  tcpip_init_finish = 1;
}
#if LWIP_API_RICH
/*
 * Simple callback function used with tcpip_callback to free a pbuf
 * (pbuf_free has a wrong signature for tcpip_callback)
 *
 * @param p The pbuf (chain) to be dereferenced.
 */
static void
pbuf_free_int(void *p)
{
  struct pbuf *q = (struct pbuf *)p;
  (void)pbuf_free(q);
}

/*
 * A simple wrapper function that allows you to free a pbuf from interrupt context.
 *
 * @param p The pbuf (chain) to be dereferenced.
 * @return ERR_OK if callback could be enqueued, an err_t if not
 */
err_t
pbuf_free_callback(struct pbuf *p)
{
  return tcpip_callback_with_block(pbuf_free_int, p, 0);
}

/*
 * A simple wrapper function that allows you to free heap memory from
 * interrupt context.
 *
 * @param m the heap memory to free
 * @return ERR_OK if callback could be enqueued, an err_t if not
 */
err_t
mem_free_callback(void *m)
{
  return tcpip_callback_with_block(mem_free, m, 0);
}
#endif
#endif /* !NO_SYS */
