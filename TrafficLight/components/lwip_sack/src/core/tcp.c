/**
 * @file
 * Transmission Control Protocol for IP
 * See also @ref tcp_raw
 *
 * @defgroup tcp_raw TCP
 * @ingroup callbackstyle_api
 * Transmission Control Protocol for IP\n
 * @see @ref raw_api and @ref netconn
 *
 * Common functions for the TCP implementation, such as functions
 * for manipulating the data structures and the TCP timer functions. TCP functions
 * related to input and output is found in tcp_in.c and tcp_out.c respectively.\n
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

#if LWIP_TCP /* don't build if not configured for use in lwipopts.h */
#include "lwip/init.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/nd6.h"

#include "lwip/priv/api_msg.h"
#include "lwip/tcp_info.h"
#include "lwip/lwip_rpl.h"

#include <string.h>

#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif

#ifndef TCP_LOCAL_PORT_RANGE_START
/* From http://www.iana.org/assignments/port-numbers:
   "The Dynamic and/or Private Ports are those from 49152 through 65535" */
#define TCP_LOCAL_PORT_RANGE_START        0xc000
#define TCP_LOCAL_PORT_RANGE_END          0xffff
#define TCP_ENSURE_LOCAL_PORT_RANGE(port) ((u16_t)(((u32_t)(port) & ~TCP_LOCAL_PORT_RANGE_START) + TCP_LOCAL_PORT_RANGE_START))
#endif

#if LWIP_TCP_KEEPALIVE
#define TCP_KEEP_DUR(pcb)   ((pcb)->keep_cnt * (pcb)->keep_intvl)
#define TCP_KEEP_INTVL(pcb) ((pcb)->keep_intvl)
#else /* LWIP_TCP_KEEPALIVE */
#define TCP_KEEP_DUR(pcb)   TCP_MAXIDLE
#define TCP_KEEP_INTVL(pcb) TCP_KEEPINTVL_DEFAULT
#endif /* LWIP_TCP_KEEPALIVE */

/* As initial send MSS, we use TCP_MSS but limit it to 536. */
#if TCP_MSS > 536
#define INITIAL_MSS 536
#else
#define INITIAL_MSS TCP_MSS
#endif

#define TCP_MIN_RCV_ANN_WND (INITIAL_MSS)

const char *const tcp_state_str[] = {
  "CLOSED",
  "LISTEN",
  "SYN_SENT",
  "SYN_RCVD",
  "ESTABLISHED",
  "FIN_WAIT_1",
  "FIN_WAIT_2",
  "CLOSE_WAIT",
  "CLOSING",
  "LAST_ACK",
  "TIME_WAIT"
};

/* last local TCP port */
static u16_t tcp_port = TCP_LOCAL_PORT_RANGE_START;
/* Incremented every coarse grained timer shot (typically every TCP_SLOW_INTERVAL ms). */
u32_t tcp_ticks;
static const u8_t tcp_backoff[13] =
  { 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7 };
/* Times per slowtmr hits */
static const u8_t tcp_persist_backoff[7] = { 3, 6, 12, 24, 48, 96, 120 };

/* The TCP PCB lists. */

/** List of all TCP PCBs bound but not yet (connected || listening) */
struct tcp_pcb *tcp_bound_pcbs;
/** List of all TCP PCBs in LISTEN state */
union tcp_listen_pcbs_t tcp_listen_pcbs;
/** List of all TCP PCBs that are in a state in which
 * they accept or send data. */
struct tcp_pcb *tcp_active_pcbs;
/** List of all TCP PCBs in TIME-WAIT state */
struct tcp_pcb *tcp_tw_pcbs;

/** An array with all (non-temporary) PCB lists, mainly used for smaller code size */
struct tcp_pcb **const tcp_pcb_lists[] = {&tcp_listen_pcbs.pcbs, &tcp_bound_pcbs,
  &tcp_active_pcbs, &tcp_tw_pcbs
};

u8_t tcp_active_pcbs_changed;

/** Timer counter to handle calling slow-timer from tcp_tmr() */
static u8_t tcp_timer;
static u8_t tcp_timer_ctr;

static u16_t tcp_new_port(void);
static err_t tcp_close_shutdown_fin(struct tcp_pcb *pcb);
#if LWIP_TCP_PCB_NUM_EXT_ARGS
static void tcp_ext_arg_invoke_callbacks_destroyed(struct tcp_pcb_ext_args *ext_args);
#endif

#if LWIP_SACK
void tcp_connect_update_sack(struct tcp_pcb *pcb, u32_t iss);
#endif

/**
 * Initialize this module.
 */
void
tcp_init(void)
{
#ifdef LWIP_RAND
  tcp_port = TCP_ENSURE_LOCAL_PORT_RANGE(LWIP_RAND());
#endif /* LWIP_RAND */
}

/** Free a tcp pcb */
void
tcp_free(struct tcp_pcb *pcb)
{
  LWIP_ASSERT("tcp_free: LISTEN", pcb->state != LISTEN);
#if LWIP_TCP_PCB_NUM_EXT_ARGS
  tcp_ext_arg_invoke_callbacks_destroyed(pcb->ext_args);
#endif
  memp_free(MEMP_TCP_PCB, pcb);
}

/** Free a tcp listen pcb */
static void
tcp_free_listen(struct tcp_pcb *pcb)
{
  LWIP_ASSERT("tcp_free_listen: !LISTEN", pcb->state != LISTEN);
#if LWIP_TCP_PCB_NUM_EXT_ARGS
  tcp_ext_arg_invoke_callbacks_destroyed(pcb->ext_args);
#endif
  memp_free(MEMP_TCP_PCB_LISTEN, pcb);
}

/**
 * Called periodically to dispatch TCP timers.
 */
void
tcp_tmr(void)
{
  /* Call tcp_fasttmr() every TCP_TMR_INTERVAL ms */
  tcp_fasttmr();

  if (++tcp_timer == TCP_SLOW_INTERVAL_PERIOD) {
    /* Call tcp_tmr() every 250 ms, i.e., every other timer
       tcp_tmr() is called. */
    tcp_slowtmr();
    tcp_timer = 0;
  }
}

#if LWIP_LOWPOWER
#include "lwip/lowpower.h"

static u32_t
tcp_set_timer_tick_by_persist(struct tcp_pcb *pcb, u32_t tick)
{
  u32_t val;

  if (pcb->persist_backoff > 0) {
    u8_t backoff_cnt = tcp_persist_backoff[pcb->persist_backoff - 1];
    SET_TMR_TICK(tick, backoff_cnt);
    return tick;
  }

  /* timer not running */
  if (pcb->rtime >= 0) {
    val = pcb->rto - pcb->rtime;
    if (val == 0) {
      val = 1;
    }
    SET_TMR_TICK(tick, val);
  }
  return tick;
}

static u32_t
tcp_set_timer_tick_by_keepalive(struct tcp_pcb *pcb, u32_t tick)
{
  u32_t val;

  if (ip_get_option(pcb, SOF_KEEPALIVE) &&
      ((pcb->state == ESTABLISHED) ||
       (pcb->state == CLOSE_WAIT))) {
    u32_t idle = (pcb->keep_idle) / TCP_SLOW_INTERVAL;
    if (pcb->keep_cnt_sent == 0) {
      val = idle - (tcp_ticks - pcb->tmr);
    } else {
      val = (tcp_ticks - pcb->tmr) - idle;
      idle = (TCP_KEEP_INTVL(pcb) / TCP_SLOW_INTERVAL);
      val  = idle - (val % idle);
    }
    /* need add 1 to trig timer */
    val++;
    SET_TMR_TICK(tick, val);
  }

  return tick;
}

static u32_t tcp_set_timer_tick_by_tcp_state(struct tcp_pcb *pcb, u32_t tick)
{
  u32_t val;

  /* Check if this PCB has stayed too long in FIN-WAIT-2 */
  if (pcb->state == FIN_WAIT_2) {
    /* If this PCB is in FIN_WAIT_2 because of SHUT_WR don't let it time out. */
    if (pcb->flags & TF_RXCLOSED) {
      val = TCP_FIN_WAIT_TIMEOUT / TCP_SLOW_INTERVAL;
      SET_TMR_TICK(tick, val);
    }
  }

  /* Check if this PCB has stayed too long in SYN-RCVD */
  if (pcb->state == SYN_RCVD) {
    val = TCP_SYN_RCVD_TIMEOUT / TCP_SLOW_INTERVAL;
    SET_TMR_TICK(tick, val);
  }

  /* Check if this PCB has stayed too long in LAST-ACK */
  if (pcb->state == LAST_ACK) {
    /*
     * In a TCP connection the end that performs the active close
     * is required to stay in TIME_WAIT state for 2MSL of time
     */
    val = (2 * TCP_MSL) / TCP_SLOW_INTERVAL;
    SET_TMR_TICK(tick, val);
  }

  return tick;
}

#if DRIVER_STATUS_CHECK
static u32_t tcp_set_timer_tick_by_driver_status(struct tcp_pcb *pcb, u32_t tick, bool drv_flag)
{
  u32_t val;

  struct netif *netif = NULL;
  if ((tcp_active_pcbs != NULL) && (drv_flag == lwIP_TRUE)) {
    for (netif = netif_list; netif != NULL; netif = netif->next) {
      /* network mask matches? */
      if ((!(netif->flags & NETIF_FLAG_DRIVER_RDY) != 0)) {
        val = DRIVER_WAKEUP_COUNT - netif->waketime + 1;
        SET_TMR_TICK(tick, val);
      }
    }
  }

  return tick;
}
#endif

u32_t
tcp_slow_tmr_tick(void)
{
  struct tcp_pcb *pcb = NULL;
  u32_t tick = 0;
#if DRIVER_STATUS_CHECK
  bool drv_flag = lwIP_FALSE;
#endif

  pcb = tcp_active_pcbs;
  while (pcb != NULL) {
    if (((pcb->state == SYN_SENT) && (pcb->nrtx >= TCP_SYNMAXRTX)) ||
        (((pcb->state == FIN_WAIT_1) || (pcb->state == CLOSING)) && (pcb->nrtx >= TCP_FW1MAXRTX)) ||
        (pcb->nrtx >= TCP_MAXRTX)) {
      return 1;
    }

    tick = tcp_set_timer_tick_by_persist(pcb, tick);

#if DRIVER_STATUS_CHECK
    if (pcb->drv_status == DRV_NOT_READY) {
      /* iterate through netifs */
      drv_flag = lwIP_TRUE;
    }
#endif /* DRIVER_STATUS_CHECK */

    tick = tcp_set_timer_tick_by_keepalive(pcb, tick);

    /*
     * If this PCB has queued out of sequence data, but has been
     * inactive for too long, will drop the data (it will eventually
     * be retransmitted).
     */
#if TCP_QUEUE_OOSEQ
    if (pcb->ooseq != NULL) {
      SET_TMR_TICK(tick, 1);
    }
#endif /* TCP_QUEUE_OOSEQ */

    tick = tcp_set_timer_tick_by_tcp_state(pcb, tick);

    u8_t ret = poll_tcp_needed(pcb->callback_arg, pcb);
    if ((pcb->poll != NULL) && (ret != 0)) {
      SET_TMR_TICK(tick, 1);
    }
    pcb = pcb->next;
  }

#if DRIVER_STATUS_CHECK
  tick = tcp_set_timer_tick_by_driver_status(pcb, tick, drv_flag);
#endif /* DRIVER_STATUS_CHECK */

  LOWPOWER_DEBUG(("%s:%d tmr tick: %u\n", __func__, __LINE__, tick));
  return tick;
}

u32_t
tcp_fast_tmr_tick(void)
{
  struct tcp_pcb *pcb = NULL;

  pcb = tcp_active_pcbs;
  while (pcb != NULL) {
    /* send delayed ACKs or send pending FIN */
    if ((pcb->flags & TF_ACK_DELAY) ||
        (pcb->flags & TF_CLOSEPEND) ||
        (pcb->refused_data != NULL) ||
        (pcb->tcp_pcb_flag & TCP_PBUF_FLAG_TCP_FIN_RECV_SYSPOST_FAIL)
#if LWIP_TCP_TLP_SUPPORT
        || (pcb->tlp_time_stamp != 0)
#endif /* LWIP_TCP_TLP_SUPPORT */
       ) {
      LOWPOWER_DEBUG(("%s:%d tmr tick: 1\n", __func__, __LINE__));
      return 1;
    }
    pcb = pcb->next;
  }
  LOWPOWER_DEBUG(("%s:%d tmr tick: 0\n", __func__, __LINE__));
  return 0;
}
#endif /* LWIP_LOWPOWER */

#if LWIP_CALLBACK_API || TCP_LISTEN_BACKLOG
/** Called when a listen pcb is closed. Iterates one pcb list and removes the
 * closed listener pcb from pcb->listener if matching.
 */
static void
tcp_remove_listener(struct tcp_pcb *list, struct tcp_pcb_listen *lpcb)
{
  struct tcp_pcb *pcb;
  for (pcb = list; pcb != NULL; pcb = pcb->next) {
    if (pcb->listener == lpcb) {
      pcb->listener = NULL;
    }
  }
}
#endif

/** Called when a listen pcb is closed. Iterates all pcb lists and removes the
 * closed listener pcb from pcb->listener if matching.
 */
static void
tcp_listen_closed(struct tcp_pcb *pcb)
{
#if LWIP_CALLBACK_API || TCP_LISTEN_BACKLOG
  size_t i;
  LWIP_ASSERT("pcb != NULL", pcb != NULL);
  LWIP_ASSERT("pcb->state == LISTEN", pcb->state == LISTEN);
  for (i = 1; i < LWIP_ARRAYSIZE(tcp_pcb_lists); i++) {
    tcp_remove_listener(*tcp_pcb_lists[i], (struct tcp_pcb_listen *)pcb);
  }
#endif
  LWIP_UNUSED_ARG(pcb);
}

#if TCP_LISTEN_BACKLOG
/** @ingroup tcp_raw
 * Delay accepting a connection in respect to the listen backlog:
 * the number of outstanding connections is increased until
 * tcp_backlog_accepted() is called.
 *
 * ATTENTION: the caller is responsible for calling tcp_backlog_accepted()
 * or else the backlog feature will get out of sync!
 *
 * @param pcb the connection pcb which is not fully accepted yet
 */
void
tcp_backlog_delayed(struct tcp_pcb *pcb)
{
  LWIP_ASSERT("pcb != NULL", pcb != NULL);
  LWIP_ASSERT_CORE_LOCKED();
  if ((pcb->flags & TF_BACKLOGPEND) == 0) {
    if (pcb->listener != NULL) {
      pcb->listener->accepts_pending++;
      LWIP_ASSERT("accepts_pending != 0", pcb->listener->accepts_pending != 0);
      pcb->flags |= TF_BACKLOGPEND;
    }
  }
}

/** @ingroup tcp_raw
 * A delayed-accept a connection is accepted (or closed/aborted): decreases
 * the number of outstanding connections after calling tcp_backlog_delayed().
 *
 * ATTENTION: the caller is responsible for calling tcp_backlog_accepted()
 * or else the backlog feature will get out of sync!
 *
 * @param pcb the connection pcb which is now fully accepted (or closed/aborted)
 */
void
tcp_backlog_accepted(struct tcp_pcb *pcb)
{
  LWIP_ASSERT("pcb != NULL", pcb != NULL);
  LWIP_ASSERT_CORE_LOCKED();
  if ((pcb->flags & TF_BACKLOGPEND) != 0) {
    if (pcb->listener != NULL) {
      LWIP_ASSERT("accepts_pending != 0", pcb->listener->accepts_pending != 0);
      pcb->listener->accepts_pending--;
      pcb->flags &= (tcpflags_t)~TF_BACKLOGPEND;
    }
  }
}
#endif /* TCP_LISTEN_BACKLOG */

/**
 * Closes the TX side of a connection held by the PCB.
 * For tcp_close(), a RST is sent if the application didn't receive all data
 * (tcp_recved() not called for all data passed to recv callback).
 *
 * Listening pcbs are freed and may not be referenced any more.
 * Connection pcbs are freed if not yet connected and may not be referenced
 * any more. If a connection is established (at least SYN received or in
 * a closing state), the connection is closed, and put in a closing state.
 * The pcb is then automatically freed in tcp_slowtmr(). It is therefore
 * unsafe to reference it.
 *
 * @param pcb the tcp_pcb to close
 * @return ERR_OK if connection has been closed
 *         another err_t if closing failed and pcb is not freed
 */
static err_t
tcp_close_shutdown(struct tcp_pcb *pcb, u8_t rst_on_unacked_data)
{
  if (rst_on_unacked_data && ((pcb->state == ESTABLISHED) || (pcb->state == CLOSE_WAIT))) {
    if ((pcb->refused_data != NULL) || (pcb->rcv_wnd != TCP_WND_MAX(pcb))) {
      /* Not all data received by application, send RST to tell the remote
         side about this. */
      LWIP_ASSERT("pcb->flags & TF_RXCLOSED", pcb->flags & TF_RXCLOSED);

      /**
        RFC 1122 section 4.2.2.13
        If such a host issues a CLOSE call while received data is still pending in TCP, or if new data is
        received after CLOSE is called, its TCP SHOULD send a RST to show that data was lost.
      */
#if DRIVER_STATUS_CHECK
      if (pcb->drv_status == DRV_READY) {
        tcp_rst(pcb->snd_nxt, pcb->rcv_nxt, &pcb->local_ip, &pcb->remote_ip,
                pcb->local_port, pcb->remote_port, pcb);
      }
#else
      tcp_rst(pcb->snd_nxt, pcb->rcv_nxt, &pcb->local_ip, &pcb->remote_ip,
              pcb->local_port, pcb->remote_port, pcb);
#endif

      tcp_pcb_purge(pcb);
      TCP_RMV_ACTIVE(pcb);

      if (tcp_input_pcb == pcb) {
        /* prevent using a deallocated pcb: free it from tcp_input later */
        tcp_trigger_input_pcb_close();
      } else {
        /* Irrespective of state, free the PCB */
        tcp_free(pcb);
      }
      return ERR_OK;
    }
  }

  /* - states which free the pcb are handled here,
     - states which send FIN and change state are handled in tcp_close_shutdown_fin() */
  switch (pcb->state) {
    case CLOSED:
      /* Closing a pcb in the CLOSED state might seem erroneous,
       * however, it is in this state once allocated and as yet unused
       * and the user needs some way to free it should the need arise.
       * Calling tcp_close() with a pcb that has already been closed, (i.e. twice)
       * or for a pcb that has been used and then entered the CLOSED state
       * is erroneous, but this should never happen as the pcb has in those cases
       * been freed, and so any remaining handles are bogus. */
      if (pcb->local_port != 0) {
        TCP_RMV(&tcp_bound_pcbs, pcb);
      }
      tcp_free(pcb);
      break;
    case LISTEN:
      tcp_listen_closed(pcb);
      tcp_listen_pcb_remove(&tcp_listen_pcbs.pcbs, (struct tcp_pcb_listen *)pcb);
      tcp_free_listen(pcb);
      break;
    case SYN_SENT:
      TCP_PCB_REMOVE_ACTIVE(pcb);
      tcp_free(pcb);
      MIB2_STATS_INC(mib2.tcpattemptfails);
      break;
    default:
      return tcp_close_shutdown_fin(pcb);
  }
  return ERR_OK;
}

static err_t
tcp_close_shutdown_fin(struct tcp_pcb *pcb)
{
  err_t err;
  LWIP_ASSERT("pcb != NULL", pcb != NULL);

  switch (pcb->state) {
    case SYN_RCVD:
      err = tcp_send_fin(pcb);
      if (err == ERR_OK) {
        tcp_backlog_accepted(pcb);
        MIB2_STATS_INC(mib2.tcpattemptfails);
        pcb->state = FIN_WAIT_1;
      }
      break;
    case ESTABLISHED:
      err = tcp_send_fin(pcb);
      if (err == ERR_OK) {
        MIB2_STATS_INC(mib2.tcpestabresets);
        pcb->state = FIN_WAIT_1;
      }
      break;
    case CLOSE_WAIT:
      err = tcp_send_fin(pcb);
      if (err == ERR_OK) {
        MIB2_STATS_INC(mib2.tcpestabresets);
        pcb->state = LAST_ACK;
      }
      break;
    default:
      /* Has already been closed, do nothing. */
      return ERR_OK;
  }

  if (err == ERR_OK) {
    /* To ensure all data has been sent when tcp_close returns, we have
       to make sure tcp_output doesn't fail.
       Since we don't really have to ensure all data has been sent when tcp_close
       returns (unsent data is sent from tcp timer functions, also), we don't care
       for the return value of tcp_output for now. */
    (void)tcp_output(pcb);
  } else if (err == ERR_MEM) {
    /* Mark this pcb for closing. Closing is retried from tcp_tmr. */
    pcb->flags |= TF_CLOSEPEND;
    /* We have to return ERR_OK from here to indicate to the callers that this
       pcb should not be used any more as it will be freed soon via tcp_tmr.
       This is OK here since sending FIN does not guarantee a time frime for
       actually freeing the pcb, either (it is left in closure states for
       remote ACK or timeout) */
    return ERR_OK;
  }
  return err;
}

/**
 * @ingroup tcp_raw
 * Closes the connection held by the PCB.
 *
 * Listening pcbs are freed and may not be referenced any more.
 * Connection pcbs are freed if not yet connected and may not be referenced
 * any more. If a connection is established (at least SYN received or in
 * a closing state), the connection is closed, and put in a closing state.
 * The pcb is then automatically freed in tcp_slowtmr(). It is therefore
 * unsafe to reference it (unless an error is returned).
 *
 * @param pcb the tcp_pcb to close
 * @return ERR_OK if connection has been closed
 *         another err_t if closing failed and pcb is not freed
 */
err_t
tcp_close(struct tcp_pcb *pcb)
{
  LWIP_DEBUGF(TCP_DEBUG, ("tcp_close: closing in "));
  LWIP_ASSERT_CORE_LOCKED();
  tcp_debug_print_state(pcb->state);

  if (pcb->state != LISTEN) {
    /* Set a flag not to receive any more data... */
    pcb->flags |= TF_RXCLOSED;
  }
  /* ... and close */
  return tcp_close_shutdown(pcb, 1);
}

/**
 * @ingroup tcp_raw
 * Causes all or part of a full-duplex connection of this PCB to be shut down.
 * This doesn't deallocate the PCB unless shutting down both sides!
 * Shutting down both sides is the same as calling tcp_close, so if it succeds
 * (i.e. returns ER_OK), the PCB must not be referenced any more!
 *
 * @param pcb PCB to shutdown
 * @param shut_rx shut down receive side if this is != 0
 * @param shut_tx shut down send side if this is != 0
 * @return ERR_OK if shutdown succeeded (or the PCB has already been shut down)
 *         another err_t on error.
 */
err_t
tcp_shutdown(struct tcp_pcb *pcb, int shut_rx, int shut_tx)
{
  LWIP_ASSERT_CORE_LOCKED();
  if (pcb->state == LISTEN) {
    return ERR_CONN;
  }
  if (shut_rx) {
    /* shut down the receive side: set a flag not to receive any more data... */
    pcb->flags |= TF_RXCLOSED;
    if (shut_tx) {
      /* shutting down the tx AND rx side is the same as closing for the raw API */
      return tcp_close_shutdown(pcb, 1);
    }
    /* ... and free buffered data */
    if (pcb->refused_data != NULL) {
      (void)pbuf_free(pcb->refused_data);
      pcb->refused_data = NULL;
    }
  }
  if (shut_tx) {
    /* This can't happen twice since if it succeeds, the pcb's state is changed.
       Only close in these states as the others directly deallocate the PCB */
    switch (pcb->state) {
      case SYN_RCVD:
      case ESTABLISHED:
      case CLOSE_WAIT:
        return tcp_close_shutdown(pcb, (u8_t)shut_rx);
      default:
        /* Not (yet?) connected, cannot shutdown the TX side as that would bring us
          into CLOSED state, where the PCB is deallocated. */
        return ERR_CONN;
    }
  }
  return ERR_OK;
}

/**
 * Abandons a connection and optionally sends a RST to the remote
 * host.  Deletes the local protocol control block. This is done when
 * a connection is killed because of shortage of memory.
 *
 * @param pcb the tcp_pcb to abort
 * @param reset boolean to indicate whether a reset should be sent
 */
void
tcp_send_rst_abort(struct tcp_pcb *pcb, int reset)
{
  u32_t seqno, ackno;
  u16_t local_port = 0;
#if LWIP_CALLBACK_API
  tcp_err_fn errf = NULL;
#endif /* LWIP_CALLBACK_API */
  void *errf_arg = NULL;

  LWIP_ASSERT_CORE_LOCKED();
  /* pcb->state LISTEN not allowed here */
  /* Figure out on which TCP PCB list we are, and remove us. If we
   are in an active state, call the receive function associated with
   the PCB with a NULL argument, and send an RST to the remote end. */
  int send_rst = 0;
  enum tcp_state last_state;
  LWIP_ASSERT("don't call tcp_abort/tcp_abandon for listen-pcbs", pcb->state != LISTEN);
  seqno = pcb->snd_nxt;
  ackno = pcb->rcv_nxt;
#if LWIP_CALLBACK_API
  errf = pcb->errf;
#endif /* LWIP_CALLBACK_API */
  errf_arg = pcb->callback_arg;
  if (pcb->state == CLOSED) {
    if (pcb->local_port != 0) {
      /* bound, not yet opened */
      TCP_RMV(&tcp_bound_pcbs, pcb);
    }
  } else {
    send_rst = reset;
    local_port = pcb->local_port;

    /* Ensure RST sent out when state has just moved to TIME_WAIT
       as previous state was not TIME_WAIT
    */
    if (pcb->state == TIME_WAIT) {
      tcp_pcb_remove(&tcp_tw_pcbs, pcb);
    } else {
      TCP_PCB_REMOVE_ACTIVE(pcb);
    }
  }
  if (pcb->unacked != NULL) {
    tcp_segs_free(pcb->unacked);
  }
  if (pcb->unsent != NULL) {
    tcp_segs_free(pcb->unsent);
  }
#if TCP_QUEUE_OOSEQ
  if (pcb->ooseq != NULL) {
    tcp_segs_free(pcb->ooseq);
  }
#endif /* TCP_QUEUE_OOSEQ */
  tcp_backlog_accepted(pcb);
  if (send_rst) {
    LWIP_DEBUGF(TCP_RST_DEBUG, ("tcp_abandon: sending RST\n"));
    tcp_rst(seqno, ackno, &pcb->local_ip, &pcb->remote_ip, local_port, pcb->remote_port, pcb);
  }
  last_state = pcb->state;
  tcp_free(pcb);
  TCP_EVENT_ERR(last_state, errf, errf_arg, ERR_ABRT);
}

/*
 * Abandons a connection and optionally sends a RST to the remote
 * host.  Deletes the local protocol control block. This is done when
 * a connection is killed because of shortage of memory.
 *
 * @param pcb the tcp_pcb to abort
 * @param reset boolean to indicate whether a reset should be sent
 */
void
tcp_abandon(struct tcp_pcb *pcb, int reset)
{
  /* pcb->state LISTEN not allowed here */
  LWIP_ASSERT("don't call tcp_abort/tcp_abandon for listen-pcbs",
              pcb->state != LISTEN);
  /*
   * Figure out on which TCP PCB list we are, and remove us. If we
   * are in an active state, call the receive function associated with
   * the PCB with a NULL argument, and send an RST to the remote end.
   */
  if (pcb->state == TIME_WAIT) {
    tcp_pcb_remove(&tcp_tw_pcbs, pcb);
    tcp_free(pcb);
  } else {
    tcp_send_rst_abort(pcb, reset);
  }
}

/**
 * @ingroup tcp_raw
 * Aborts the connection by sending a RST (reset) segment to the remote
 * host. The pcb is deallocated. This function never fails.
 *
 * ATTENTION: When calling this from one of the TCP callbacks, make
 * sure you always return ERR_ABRT (and never return ERR_ABRT otherwise
 * or you will risk accessing deallocated memory or memory leaks!
 *
 * @param pcb the tcp pcb to abort
 */
void
tcp_abort(struct tcp_pcb *pcb)
{
  tcp_abandon(pcb, 1);
}

/**
 * @ingroup tcp_raw
 * Binds the connection to a local port number and IP address. If the
 * IP address is not given (i.e., ipaddr == NULL), the IP address of
 * the outgoing network interface is used instead.
 *
 * @param pcb the tcp_pcb to bind (no check is done whether this pcb is
 *        already bound!)
 * @param ipaddr the local ip address to bind to (use IP4_ADDR_ANY to bind
 *        to any local address
 * @param port the local port to bind to
 * @return ERR_USE if the port is already in use
 *         ERR_VAL if bind failed because the PCB is not in a valid state
 *         ERR_OK if bound
 */
err_t
tcp_bind(struct tcp_pcb *pcb, const ip_addr_t *ipaddr, u16_t port)
{
  int i;
  int max_pcb_list = NUM_TCP_PCB_LISTS;
  struct tcp_pcb *cpcb = NULL;
#if LWIP_IPV6 && LWIP_IPV6_SCOPES
  ip_addr_t zoned_ipaddr;
#endif /* LWIP_IPV6 && LWIP_IPV6_SCOPES */

  LWIP_ASSERT_CORE_LOCKED();

  if (pcb == NULL) {
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_bind: invalid pcb"));
    return ERR_VAL;
  }
  LWIP_ERROR("tcp_bind: can only bind in state CLOSED", pcb->state == CLOSED, return ERR_VAL);

#if LWIP_IPV4
  /* Don't propagate NULL pointer (IPv4 ANY) to subsequent functions */
  if (ipaddr == NULL) {
    ipaddr = IP4_ADDR_ANY;
  }
#else
  LWIP_ERROR("tcp_bind: invalid ipaddr", ipaddr != NULL, return ERR_VAL);
#endif /* LWIP_IPV4 */

  if (netif_ipaddr_isbrdcast(ipaddr) || ip_addr_ismulticast(ipaddr)) {
    return ERR_NOADDR;
  }

#if SO_REUSE
  /* Unless the REUSEADDR flag is set,
     we have to check the pcbs in TIME-WAIT state, also.
     We do not dump TIME_WAIT pcb's; they can still be matched by incoming
     packets using both local and remote IP addresses and ports to distinguish.
   */
  if (ip_get_option(pcb, SOF_REUSEADDR)) {
    max_pcb_list = NUM_TCP_PCB_LISTS_NO_TIME_WAIT;
  }
#endif /* SO_REUSE */

#if LWIP_IPV6 && LWIP_IPV6_SCOPES
  /* If the given IP address should have a zone but doesn't, assign one now.
   * This is legacy support: scope-aware callers should always provide properly
   * zoned source addresses. Do the zone selection before the address-in-use
   * check below; as such we have to make a temporary copy of the address. */
  if (IP_IS_V6(ipaddr) && ip6_addr_lacks_zone(ip_2_ip6(ipaddr), IP6_UNICAST)) {
    ip_addr_copy(zoned_ipaddr, *ipaddr);
    ip6_addr_select_zone(ip_2_ip6(&zoned_ipaddr), ip_2_ip6(&zoned_ipaddr));
    ipaddr = &zoned_ipaddr;
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_SCOPES */

  if (port == 0) {
    port = tcp_new_port();
    if (port == 0) {
      return ERR_USE;
    }
  } else {
    /* Check if the address already is in use (on all lists) */
    for (i = 0; i < max_pcb_list; i++) {
      for (cpcb = *tcp_pcb_lists[i]; cpcb != NULL; cpcb = cpcb->next) {
        /* omit the confilcting check if the pcbs bond to diff netif */
        if ((cpcb->ifindex != 0) && (pcb->ifindex != 0) && (pcb->ifindex != cpcb->ifindex)) {
          continue;
        }
        if (cpcb->local_port == port) {
#if SO_REUSE
          /* Omit checking for the same port if both pcbs have REUSEADDR set.
             For SO_REUSEADDR, the duplicate-check for a 5-tuple is done in
             tcp_connect. */
          if (!ip_get_option(pcb, SOF_REUSEADDR) ||
              !ip_get_option(cpcb, SOF_REUSEADDR))
#endif /* SO_REUSE */
          {
            if (((IP_IS_V6_VAL(*ipaddr) == IP_IS_V6_VAL(cpcb->local_ip)) ||
                 (IP_IS_ANY_TYPE_VAL(cpcb->local_ip)) ||
                 (IP_IS_ANY_TYPE_VAL(*ipaddr))) &&
                (ip_addr_isany(&cpcb->local_ip) ||
                 ip_addr_isany_val(*ipaddr) ||
                 ip_addr_cmp(&cpcb->local_ip, ipaddr))) {
              return ERR_USE;
            }
          }
        }

        /* Address is already bound to an address, return EINVAL */
        if (pcb == cpcb) {
          return ERR_VAL;
        }
      }
    }
  }

  if (!ip_addr_isany_val(*ipaddr)
#if LWIP_IPV4 && LWIP_IPV6
      || (IP_GET_TYPE(ipaddr) != IP_GET_TYPE(&pcb->local_ip))
#endif /* LWIP_IPV4 && LWIP_IPV6 */
     ) {
    ip_addr_set(&pcb->local_ip, ipaddr);
  }
  pcb->local_port = port;
  TCP_REG(&tcp_bound_pcbs, pcb);
  LWIP_DEBUGF(TCP_DEBUG, ("tcp_bind: bind to port %"U16_F"\n", port));
  return ERR_OK;
}
#if LWIP_CALLBACK_API
/**
 * Default accept callback if no accept callback is specified by the user.
 */
static err_t
tcp_accept_null(void *arg, struct tcp_pcb *pcb, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  tcp_abort(pcb);

  return ERR_ABRT;
}
#endif /* LWIP_CALLBACK_API */

#if LWIP_API_RICH
/**
 * @ingroup tcp_raw
 * Set the state of the connection to be LISTEN, which means that it
 * is able to accept incoming connections. The protocol control block
 * is reallocated in order to consume less memory. Setting the
 * connection to LISTEN is an irreversible process.
 *
 * @param pcb the original tcp_pcb
 * @param backlog the incoming connections queue limit
 * @return tcp_pcb used for listening, consumes less memory.
 *
 * @note The original tcp_pcb is freed. This function therefore has to be
 *       called like this:
 *             tpcb = tcp_listen_with_backlog(tpcb, backlog);
 */
struct tcp_pcb *
tcp_listen_with_backlog(struct tcp_pcb *pcb, u8_t backlog)
{
  LWIP_ASSERT_CORE_LOCKED();
  return tcp_listen_with_backlog_and_err(pcb, backlog, NULL);
}
#endif /* LWIP_API_RICH */

/**
 * @ingroup tcp_raw
 * Set the state of the connection to be LISTEN, which means that it
 * is able to accept incoming connections. The protocol control block
 * is reallocated in order to consume less memory. Setting the
 * connection to LISTEN is an irreversible process.
 *
 * @param pcb the original tcp_pcb
 * @param backlog the incoming connections queue limit
 * @param err when NULL is returned, this contains the error reason
 * @return tcp_pcb used for listening, consumes less memory.
 *
 * @note The original tcp_pcb is freed. This function therefore has to be
 *       called like this:
 *             tpcb = tcp_listen_with_backlog_and_err(tpcb, backlog, &err);
 */
struct tcp_pcb *
tcp_listen_with_backlog_and_err(struct tcp_pcb *pcb, u8_t backlog, err_t *err)
{
  struct tcp_pcb_listen *lpcb = NULL;
  err_t res;

  LWIP_UNUSED_ARG(backlog);
  LWIP_ASSERT_CORE_LOCKED();
  LWIP_ERROR("tcp_listen: pcb already connected", pcb->state == CLOSED, res = ERR_CLSD; goto done);

  /* already listening? */
  if (pcb->state == LISTEN) {
    lpcb = (struct tcp_pcb_listen *)pcb;
    res = ERR_ALREADY;
    goto done;
  }
#if SO_REUSE
  if (ip_get_option(pcb, SOF_REUSEADDR)) {
    /* Since SOF_REUSEADDR allows reusing a local address before the pcb's usage
       is declared (listen-/connection-pcb), we have to make sure now that
       this port is only used once for every local IP. */
    for (lpcb = tcp_listen_pcbs.listen_pcbs; lpcb != NULL; lpcb = lpcb->next) {
      if ((lpcb->local_port == pcb->local_port) &&
          ip_addr_cmp(&lpcb->local_ip, &pcb->local_ip)) {
        /* this address/port is already used */
        lpcb = NULL;
        res = ERR_USE;
        goto done;
      }
    }
  }
#endif /* SO_REUSE */
  lpcb = (struct tcp_pcb_listen *)memp_malloc(MEMP_TCP_PCB_LISTEN);
  if (lpcb == NULL) {
    res = ERR_MEM;
    goto done;
  }
  lpcb->callback_arg = pcb->callback_arg;
  lpcb->local_port = pcb->local_port;
  lpcb->ifindex = pcb->ifindex;
  lpcb->state = LISTEN;
  lpcb->prio = pcb->prio;
#if LWIP_SO_SNDBUF
  lpcb->snd_buf_static = pcb->snd_buf_static;
#endif
  lpcb->so_options = pcb->so_options;
  lpcb->ttl = pcb->ttl;
  lpcb->tos = pcb->tos;
#if LWIP_IPV4 && LWIP_IPV6
  IP_SET_TYPE_VAL(lpcb->remote_ip, pcb->local_ip.type);
#endif /* LWIP_IPV4 && LWIP_IPV6 */
  ip_addr_copy(lpcb->local_ip, pcb->local_ip);
  if (pcb->local_port != 0) {
    TCP_RMV(&tcp_bound_pcbs, pcb);
  }
#if LWIP_SO_PRIORITY
  lpcb->priority = pcb->priority;
#endif /* LWIP_SO_PRIORITY */
#if LWIP_TCP_PCB_NUM_EXT_ARGS
  /* copy over ext_args to listening pcb  */
  (void)memcpy_s(&lpcb->ext_args, sizeof(lpcb->ext_args), &pcb->ext_args, sizeof(pcb->ext_args));
#endif
  tcp_free(pcb);
#if LWIP_CALLBACK_API
  lpcb->accept = tcp_accept_null;
#endif /* LWIP_CALLBACK_API */
#if TCP_LISTEN_BACKLOG
  lpcb->accepts_pending = 0;
  tcp_backlog_set(lpcb, backlog);
#endif /* TCP_LISTEN_BACKLOG */
  TCP_REG(&tcp_listen_pcbs.pcbs, (struct tcp_pcb *)lpcb);
  res = ERR_OK;
done:
  if (err != NULL) {
    *err = res;
  }
  return (struct tcp_pcb *)lpcb;
}

/**
 * Update the state that tracks the available window space to advertise.
 *
 * Returns how much extra window would be advertised if we sent an
 * update now.
 */
u32_t
tcp_update_rcv_ann_wnd(struct tcp_pcb *pcb)
{
  u32_t new_right_edge = pcb->rcv_nxt + pcb->rcv_wnd;

  if (TCP_SEQ_GEQ(new_right_edge, pcb->rcv_ann_right_edge + LWIP_MIN((TCP_WND / 2), (u32_t)pcb->mss))) {
    /* we can advertise more window */
    if (pbuf_ram_in_deflation() == lwIP_FALSE) {
      pcb->rcv_ann_wnd = pcb->rcv_wnd;
      return new_right_edge - pcb->rcv_ann_right_edge;
    } else {
      pcb->rcv_ann_wnd = LWIP_MIN(pcb->rcv_wnd, LWIP_MIN((TCP_WND >> 1), (u32_t)pcb->mss));
      return TCP_WND_UPDATE_THRESHOLD; /* force ack now */
    }
  } else {
    if (TCP_SEQ_GT(pcb->rcv_nxt, pcb->rcv_ann_right_edge)) {
      /* Can happen due to other end sending out of advertised window,
       * but within actual available (but not yet advertised) window */
      pcb->rcv_ann_wnd = 0;
    } else {
      /* keep the right edge of window constant */
      u32_t new_rcv_ann_wnd = pcb->rcv_ann_right_edge - pcb->rcv_nxt;
#if !LWIP_WND_SCALE
      LWIP_ASSERT("new_rcv_ann_wnd <= 0xffff", new_rcv_ann_wnd <= 0xffff);
#endif
      pcb->rcv_ann_wnd = (tcpwnd_size_t)new_rcv_ann_wnd;
    }
    return 0;
  }
}

/**
 * @ingroup tcp_raw
 * This function should be called by the application when it has
 * processed the data. The purpose is to advertise a larger window
 * when the data has been processed.
 *
 * @param pcb the tcp_pcb for which data is read
 * @param len the amount of bytes that have been read by the application
 */
void
tcp_recved(struct tcp_pcb *pcb, u16_t len)
{
  u32_t wnd_inflation;
  tcpwnd_size_t rcv_wnd;

  LWIP_ASSERT_CORE_LOCKED();

  /* pcb->state LISTEN not allowed here */
  LWIP_ASSERT("don't call tcp_recved for listen-pcbs",
              pcb->state != LISTEN);

  rcv_wnd = (tcpwnd_size_t)(pcb->rcv_wnd + len);
  if (rcv_wnd > TCP_WND_MAX(pcb) || (rcv_wnd < pcb->rcv_wnd)) {
    pcb->rcv_wnd = TCP_WND_MAX(pcb);
  } else {
    pcb->rcv_wnd = rcv_wnd;
  }

  wnd_inflation = tcp_update_rcv_ann_wnd(pcb);

  /* If the change in the right edge of window is significant (default
   * watermark is TCP_WND/4), then send an explicit update now.
   * Otherwise wait for a packet to be sent in the normal course of
   * events (or more window to be available later) */
  if (wnd_inflation >= TCP_WND_UPDATE_THRESHOLD) {
    tcp_ack_now(pcb);
    (void)tcp_output(pcb);
  }

  LWIP_DEBUGF(TCP_DEBUG, ("tcp_recved: received %"U16_F" bytes, wnd %"TCPWNDSIZE_F" (%"TCPWNDSIZE_F").\n",
                          len, pcb->rcv_wnd, (u16_t)(TCP_WND_MAX(pcb) - pcb->rcv_wnd)));
}

/**
 * Allocate a new local TCP port.
 *
 * @return a new (free) local TCP port number
 */
LWIP_STATIC u16_t
tcp_new_port(void)
{
  u16_t n = 0;
  struct tcp_pcb *pcb = NULL;
  u8_t i;

again:
  if (tcp_port++ == TCP_LOCAL_PORT_RANGE_END) {
    tcp_port = TCP_LOCAL_PORT_RANGE_START;
  }

  /* Check all PCB lists. */
  for (i = 0; i < NUM_TCP_PCB_LISTS; i++) {
    for (pcb = *tcp_pcb_lists[i]; pcb != NULL; pcb = pcb->next) {
      if (pcb->local_port == tcp_port) {
        if (++n > (TCP_LOCAL_PORT_RANGE_END - TCP_LOCAL_PORT_RANGE_START)) {
          return 0;
        }
        goto again;
      }
    }
  }
  return tcp_port;
}

/**
 * @ingroup tcp_raw
 * Connects to another host. The function given as the "connected"
 * argument will be called when the connection has been established.
 *
 * @param pcb the tcp_pcb used to establish the connection
 * @param ipaddr the remote ip address to connect to
 * @param port the remote tcp port to connect to
 * @param connected callback function to call when connected (on error,
                    the err calback will be called)
 * @return ERR_VAL if invalid arguments are given
 *         ERR_OK if connect request has been sent
 *         other err_t values if connect request couldn't be sent
 */
err_t
tcp_connect(struct tcp_pcb *pcb, const ip_addr_t *ipaddr, u16_t port,
            tcp_connected_fn connected)
{
  err_t ret;
  u32_t iss;
  u16_t old_local_port;
  struct netif *netif = NULL;

  LWIP_ASSERT_CORE_LOCKED();

  if ((pcb == NULL) || (ipaddr == NULL)) {
    return ERR_VAL;
  }

  LWIP_ERROR("tcp_connect: can only connect from state CLOSED", pcb->state == CLOSED, return ERR_ISCONN);
  LWIP_ERROR("tcp_connect: can not connect the Multicast IP address",
             !ip_addr_ismulticast_val(ipaddr), return ERR_RTE);

  netif = ip_route_pcb(ipaddr, (struct ip_pcb *)pcb);
  if (netif == NULL) {
    /* Don't even try to send a SYN packet if we have no route
       since that will fail. */
    return ERR_NETUNREACH;
  }

  LWIP_ERROR("tcp_connect: can not connect the Broadcast IP address",
             !ip_addr_isbroadcast_val(ipaddr, netif), return ERR_RTE);

  LWIP_DEBUGF(TCP_DEBUG, ("tcp_connect to port %"U16_F"\n", port));
  ip_addr_set_val(&pcb->remote_ip, ipaddr);
  pcb->remote_port = port;

#if DRIVER_STATUS_CHECK
  if (!(netif->flags & NETIF_FLAG_DRIVER_RDY)) {
    pcb->drv_status = DRV_NOT_READY;
  } else {
    pcb->drv_status = DRV_READY;
  }
#endif /* DRIVER_STATUS_CHECK */

  /* check if we have a route to the remote host */
  if (ip_addr_isany(&pcb->local_ip)) {
    /* no local IP address set, yet. */
    const ip_addr_t *local_ip = ip_netif_get_local_ip(netif, &pcb->remote_ip);
    if ((local_ip == NULL)) {
      /* Don't even try to send a SYN packet if we have no route
         since that will fail. */
      return ERR_NETUNREACH;
    }
    /* Use the address as local address of the pcb. */
    ip_addr_copy(pcb->local_ip, *local_ip);
  } else {
#if LWIP_SO_DONTROUTE
    rt_scope_t scope = RT_SCOPE_UNIVERSAL;

    scope = ip_get_option(pcb, SOF_DONTROUTE) ? RT_SCOPE_LINK : RT_SCOPE_UNIVERSAL;

    /* Don't call ip_route() with IP_ANY_TYPE */
    netif = ip_route(&pcb->local_ip, &pcb->remote_ip, scope);
#else
    netif = ip_route(&pcb->local_ip, &pcb->remote_ip);
#endif
    if (netif == NULL) {
      /* Don't even try to send a SYN packet if we have no route
         since that will fail. */
      return ERR_RTE;
    }
  }
  LWIP_ASSERT("netif != NULL", netif != NULL);

#if LWIP_IPV6 && LWIP_IPV6_SCOPES
  /* If the given IP address should have a zone but doesn't, assign one now.
   * Given that we already have the target netif, this is easy and cheap. */
  if (IP_IS_V6(&pcb->remote_ip) &&
      ip6_addr_lacks_zone(ip_2_ip6(&pcb->remote_ip), IP6_UNICAST)) {
    ip6_addr_assign_zone(ip_2_ip6(&pcb->remote_ip), IP6_UNICAST, netif);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_SCOPES */

  old_local_port = pcb->local_port;
  if (pcb->local_port == 0) {
    pcb->local_port = tcp_new_port();
    if (pcb->local_port == 0) {
      return ERR_NOADDR;
    }
  } else {
#if SO_REUSE
    if (ip_get_option(pcb, SOF_REUSEADDR)) {
      /* Since SOF_REUSEADDR allows reusing a local address, we have to make sure
         now that the 5-tuple is unique. */
      struct tcp_pcb *cpcb;
      int i;
      /* Don't check listen- and bound-PCBs, check active- and TIME-WAIT PCBs. */
      for (i = 2; i < NUM_TCP_PCB_LISTS; i++) {
        for (cpcb = *tcp_pcb_lists[i]; cpcb != NULL; cpcb = cpcb->next) {
          if ((cpcb->local_port == pcb->local_port) &&
              (cpcb->remote_port == port) &&
              ip_addr_cmp(&cpcb->local_ip, &pcb->local_ip) &&
              ip_addr_cmp(&cpcb->remote_ip, ipaddr)) {
            /* linux returns EISCONN here, but ERR_USE should be OK for us */
            return ERR_USE;
          }
        }
      }
    }
#endif /* SO_REUSE */
  }
  iss = tcp_next_iss(pcb);
  pcb->rcv_nxt = 0;
  pcb->snd_nxt = iss;
  pcb->snd_sml = iss;
  pcb->lastack = iss - 1;
  pcb->snd_wl2 = iss - 1;
  pcb->snd_lbb = iss - 1;
  pcb->fast_recovery_point = iss;
  pcb->rto_end = iss;

  /* Start with a window that does not need scaling. When window scaling is
     enabled and used, the window is enlarged when both sides agree on scaling. */
  pcb->rcv_wnd = pcb->rcv_ann_wnd = TCPWND_MIN16(TCP_WND);
  if (pbuf_ram_in_deflation() != lwIP_FALSE) {
    pcb->rcv_ann_wnd = LWIP_MIN(pcb->rcv_ann_wnd, TCP_MIN_RCV_ANN_WND);
  }
  pcb->rcv_ann_right_edge = pcb->rcv_nxt;
  pcb->snd_wnd = TCP_WND;
  /* As initial send MSS, we use TCP_MSS but limit it to 536.
     The send MSS is updated when an MSS option is received. */
  pcb->mss = INITIAL_MSS;
#if TCP_CALCULATE_EFF_SEND_MSS
  pcb->mss = tcp_eff_send_mss(pcb->mss, &pcb->local_ip, &pcb->remote_ip, netif);
#endif /* TCP_CALCULATE_EFF_SEND_MSS */

  LWIP_TCP_CALC_INITIAL_CWND(pcb->mss, pcb->iw);
  pcb->cwnd = pcb->iw;

#if LWIP_CALLBACK_API
  pcb->connected = connected;
#else /* LWIP_CALLBACK_API */
  LWIP_UNUSED_ARG(connected);
#endif /* LWIP_CALLBACK_API */

#if LWIP_SACK
  tcp_connect_update_sack(pcb, iss);
#endif

  /* Send a SYN together with the MSS option. */
  ret = tcp_enqueue_flags(pcb, TCP_SYN);
  if (ret == ERR_OK) {
    /* SYN segment was enqueued, changed the pcbs state now */
    pcb->state = SYN_SENT;
    if (old_local_port != 0) {
      TCP_RMV(&tcp_bound_pcbs, pcb);
    }
    TCP_REG_ACTIVE(pcb);
    MIB2_STATS_INC(mib2.tcpactiveopens);

    (void)tcp_output(pcb);
  }
  return ret;
}

/*
 * Called every TCP_SLOW_INTERVAL ms and implements the retransmission timer and the timer that
 * removes PCBs that have been in TIME-WAIT for enough time. It also increments
 * various timers such as the inactivity timer in each PCB.
 *
 * Automatically called from tcp_tmr().
 */
void
tcp_slowtmr(void)
{
  struct tcp_pcb *pcb = NULL;
  struct tcp_pcb *prev = NULL;
  tcpwnd_size_t eff_wnd;
  u8_t pcb_remove;      /* flag if a PCB should be removed */
  u8_t pcb_reset;       /* flag if a RST should be sent when removing */
  err_t err;
  u8_t connect_timeout;
#if DRIVER_STATUS_CHECK
  struct netif *netif = NULL;
#endif /* DRIVER_STATUS_CHECK */
  err = ERR_OK;

  ++tcp_ticks;
  ++tcp_timer_ctr;

tcp_slowtmr_start:
  /* Steps through all of the active PCBs. */
  prev = NULL;
  pcb = tcp_active_pcbs;
  if (pcb == NULL) {
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: no active pcbs\n"));
  }

#if DRIVER_STATUS_CHECK
  for (netif = netif_list; netif != NULL; netif = netif->next) {
    /* network mask matches? */
    if (!(netif->flags & NETIF_FLAG_DRIVER_RDY)) {
      if (netif->waketime <= DRIVER_WAKEUP_COUNT) {
        netif->waketime++;
      }
    }
  }
#endif /* DRIVER_STATUS_CHECK */

  while (pcb != NULL) {
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: processing active pcb\n"));
    LWIP_ASSERT("tcp_slowtmr: active pcb->state != CLOSED\n", pcb->state != CLOSED);
    LWIP_ASSERT("tcp_slowtmr: active pcb->state != LISTEN\n", pcb->state != LISTEN);
    LWIP_ASSERT("tcp_slowtmr: active pcb->state != TIME-WAIT\n", pcb->state != TIME_WAIT);

    if (pcb->last_timer == tcp_timer_ctr) {
      /* skip this pcb, we have already processed it */
      pcb = pcb->next;
      continue;
    }

    pcb->last_timer = tcp_timer_ctr;
    pcb_remove = 0;
    pcb_reset = 0;

    connect_timeout = 0;
    if ((pcb->state == SYN_SENT) && (pcb->nrtx >= TCP_SYNMAXRTX)) {
      ++pcb_remove;
      ++connect_timeout;
      LWIP_DEBUGF(TCP_DEBUG | TCP_ERR_DEBUG, ("tcp_slowtmr: max SYN retries reached\n"));
    } else if (((pcb->state == FIN_WAIT_1) || (pcb->state == CLOSING)) && (pcb->nrtx >= TCP_FW1MAXRTX)) {
      ++pcb_remove;
      LWIP_DEBUGF(TCP_DEBUG | TCP_ERR_DEBUG,
                  ("tcp_slowtmr: max DATA retries reached in FIN_WAIT_1 or CLOSING state\n"));
    } else if (pcb->nrtx >= TCP_MAXRTX) {
      if (pcb->state == SYN_SENT) {
        ++connect_timeout;
      }
      ++pcb_remove;
      LWIP_DEBUGF(TCP_DEBUG | TCP_ERR_DEBUG, ("tcp_slowtmr: max DATA retries reached, retries : %d\n", pcb->nrtx));
    } else {
      if (pcb->persist_backoff > 0) {
        LWIP_ASSERT("tcp_slowtimr: persist ticking with in-flight data", pcb->unacked == NULL);
        LWIP_ASSERT("tcp_slowtimr: persist ticking with empty send buffer", pcb->unsent != NULL);
        if (pcb->persist_probe >= TCP_MAXRTX) {
          ++pcb_remove; /* max probes reached */
        } else {
          /* If snd_wnd is zero, use persist timer to send 1 byte probes
           * instead of using the standard retransmission mechanism. */
          u8_t backoff_cnt = tcp_persist_backoff[pcb->persist_backoff - 1];
          if (pcb->persist_cnt < backoff_cnt) {
            pcb->persist_cnt++;
          }
          if (pcb->persist_cnt >= backoff_cnt) {
            if (tcp_zero_window_probe(pcb) == ERR_OK) {
              pcb->persist_cnt = 0;
              if (pcb->persist_backoff < sizeof(tcp_persist_backoff)) {
                pcb->persist_backoff++;
              }
            }
          }
        }
      } else {
        /* Increase the retransmission timer if it is running */
        if (pcb->rtime >= 0) {
          ++pcb->rtime;
        }

        if (pcb->unacked != NULL && pcb->rtime >= pcb->rto) {
          /* Time for a retransmission. */
          LWIP_DEBUGF(TCP_RTO_DEBUG, ("tcp_slowtmr: rtime %"S16_F
                                      " pcb->rto %"S16_F"\n",
                                      pcb->rtime, pcb->rto));

          /* Double retransmission time-out unless we are trying to
           * connect to somebody (i.e., we are in SYN_SENT). */
          if (pcb->state != SYN_SENT) {
            u8_t backoff_idx = (u8_t)LWIP_MIN(pcb->nrtx, (sizeof(tcp_backoff) - 1));
            if (pcb->sa != -1) {
              pcb->rto = (int16_t)((((pcb->sa >> 3) + pcb->sv) / TCP_SLOW_INTERVAL) * tcp_backoff[backoff_idx]);
            } else {
              pcb->rto = (int16_t)((TCP_INITIAL_RTO_DURATION / TCP_SLOW_INTERVAL) * tcp_backoff[backoff_idx]);
            }
          } else {
            /* overflow can not happen, max value of RTO will be capped by 64 seconds, and minimum 1second */
            pcb->rto = (s16_t)((u16_t)pcb->rto << 1);
          }

          /*
           * RFC 6298 section 2.5
           * A maximum value MAY be placed on RTO provided it is at least 60 seconds.
           */
          pcb->rto = (s16_t)LWIP_MIN(TCP_MAX_RTO_TICKS, LWIP_MAX(TCP_MIN_RTO_TICKS, pcb->rto));

          /*
           * RFC 6298 section 5.6
           * Start the retransmission timer, such that it expires after RTO
           * seconds (for the value of RTO after the doubling operation
           * outlined in 5.5).
           */
          pcb->rtime = 0;
          pcb->dupacks = 0;

          /* Reduce congestion window and ssthresh. */
          eff_wnd = LWIP_MIN(pcb->cwnd, pcb->snd_wnd);

          /*
           * RFC 5681
           * Does not comply to calucation
           * ssthresh = max (FlightSize/2, 2*SMSS)            (4)
           *
           * Instead min value of SSTHRESH is kept to 8, to limit lower threshold,
           * may not hold good for low bandwidth scenario
           * Threshold reduction is 5/8,instead of 1/2.. slightly more optimistic hence more aggressive
           */
          pcb->ssthresh = (tcpwnd_size_t)(((u64_t)eff_wnd * 5) >> 3); /* divide by 5/8 */

          /* initial threshold may not be true enough to adjust to network condition it must be adjusted */
          /* max value of mss has to be limited */
          pcb->ssthresh = (tcpwnd_size_t)(LWIP_MAX(pcb->ssthresh, (tcpwnd_size_t)(pcb->mss << 3)));

          /**
           * Violating below section of RFC 5681 Section 3.1
           * Furthermore, upon a timeout (as specified in [RFC2988]) cwnd MUST be
           * set to no more than the loss window, LW, which equals 1 full-sized
           * segment (regardless of the value of IW).  Therefore, after
           * retransmitting the dropped segment the TCP sender uses the slow start
           * algorithm to increase the window from 1 full-sized segment to the new
           * value of ssthresh, at which point congestion avoidance again takes
           * over.
           * It should be set of minimum of atleast 2 to avoid issues due to Delayed acks
           */
#ifndef LWIP_INITIAL_CWND_OVERRIDE
          pcb->cwnd = (tcpwnd_size_t)(LWIP_MIN(pcb->iw, (tcpwnd_size_t)pcb->mss << 1));
#else
          pcb->cwnd = (tcpwnd_size_t)(pcb->mss * LWIP_INITIAL_CWND_OVERRIDE);
#endif

          pcb->bytes_acked = 0;

          LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_slowtmr: cwnd %"TCPWNDSIZE_F
                                       " ssthresh %"TCPWNDSIZE_F"\n",
                                       pcb->cwnd, pcb->ssthresh));

          /*
           * 4)  Retransmit timeouts:
           *    After a retransmit timeout, record the highest sequence number
           *    transmitted in the variable recover, and exit the fast recovery
           *    procedure if applicable.
           */
          tcp_clear_flags(pcb, TF_INFR);

          /*
           * RFC 6298 section 5.4
           * Retransmit the earliest segment that has not been acknowledged by the TCP receiver.
           */
          /* Deviation in lwip: try to retransmit all unacked segement, not just the first one */
          tcp_rexmit_rto(pcb);

          /* RFC 6582 Section 4 : Handling Duplicate Acknowledgments after a Timeout.. */
          pcb->fast_recovery_point = pcb->snd_nxt;
        }
      }
    }

#if DRIVER_STATUS_CHECK
    if (pcb->drv_status == DRV_NOT_READY) {
      /* iterate through netifs */
      for (netif = netif_list; netif != NULL; netif = netif->next) {
        /* network mask matches? */
        if (tcp_is_netif_addr_check_success(pcb, netif)) {
          if (netif->waketime > DRIVER_WAKEUP_COUNT) {
            /* waketime already incremented for a previous pcb, So, remove this pcb */
            LWIP_DEBUGF(DRV_STS_DEBUG, ("Driver Wake time max count (%d) reached. Removing PCB\n", netif->waketime));
            pcb_remove++;
          }
          break;
        }
      }
    }

#endif /* DRIVER_STATUS_CHECK */

    /* Check if this PCB has stayed too long in FIN-WAIT-2 */
    if (pcb->state == FIN_WAIT_2) {
      /* If this PCB is in FIN_WAIT_2 because of SHUT_WR don't let it time out. */
      if (pcb->flags & TF_RXCLOSED) {
        /* PCB was fully closed (either through close() or SHUT_RDWR):
           normal FIN-WAIT timeout handling. */
        if ((u32_t)(tcp_ticks - pcb->tmr) >
            TCP_FIN_WAIT_TIMEOUT / TCP_SLOW_INTERVAL) {
          ++pcb_remove;
          LWIP_DEBUGF(TCP_DEBUG | TCP_ERR_DEBUG, ("tcp_slowtmr: removing pcb stuck in FIN-WAIT-2\n"));
        }
      }
    }

    /* Check if KEEPALIVE should be sent */
    if (ip_get_option(pcb, SOF_KEEPALIVE) &&
        ((pcb->state == ESTABLISHED) ||
         (pcb->state == CLOSE_WAIT))) {
      if ((u32_t)(tcp_ticks - pcb->tmr) >
          (pcb->keep_idle + TCP_KEEP_DUR(pcb)) / TCP_SLOW_INTERVAL) {
        LWIP_DEBUGF(TCP_DEBUG | TCP_ERR_DEBUG, ("tcp_slowtmr: KEEPALIVE timeout. Aborting connection to "));
        ip_addr_debug_print_val(TCP_DEBUG, pcb->remote_ip);
        LWIP_DEBUGF(TCP_DEBUG | TCP_ERR_DEBUG, ("\n"));

        ++pcb_remove;
        ++pcb_reset;
      } else if ((u32_t)(tcp_ticks - pcb->tmr) >
                 (pcb->keep_idle + pcb->keep_cnt_sent * TCP_KEEP_INTVL(pcb))
                 / TCP_SLOW_INTERVAL) {
        err = tcp_keepalive(pcb);
        if (err == ERR_OK) {
          pcb->keep_cnt_sent++;
        }
      }
    }

    /* If this PCB has queued out of sequence data, but has been
       inactive for too long, will drop the data (it will eventually
       be retransmitted). */
#if TCP_QUEUE_OOSEQ
    if (pcb->ooseq != NULL &&
        (s32_t)(tcp_ticks - pcb->tmr) >= ((s32_t)pcb->rto * TCP_OOSEQ_TIMEOUT)) {
      tcp_segs_free(pcb->ooseq);
      pcb->ooseq = NULL;
      LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_slowtmr: dropping OOSEQ queued data\n"));
    }
#endif /* TCP_QUEUE_OOSEQ */

    /* Check if this PCB has stayed too long in SYN-RCVD */
    if (pcb->state == SYN_RCVD) {
      if ((u32_t)(tcp_ticks - pcb->tmr) >
          TCP_SYN_RCVD_TIMEOUT / TCP_SLOW_INTERVAL) {
        ++pcb_remove;
        LWIP_DEBUGF(TCP_DEBUG | TCP_ERR_DEBUG, ("tcp_slowtmr: removing pcb stuck in SYN-RCVD\n"));
      }
    }

    /* Check if this PCB has stayed too long in LAST-ACK */
    if (pcb->state == LAST_ACK) {
      if ((u32_t)(tcp_ticks - pcb->tmr) > 2 * TCP_MSL / TCP_SLOW_INTERVAL) {
        ++pcb_remove;
        LWIP_DEBUGF(TCP_DEBUG | TCP_ERR_DEBUG, ("tcp_slowtmr: removing pcb stuck in LAST-ACK\n"));
      }
    }

    /* If the PCB should be removed, do it. */
    if (pcb_remove) {
      struct tcp_pcb *pcb2;
#if LWIP_CALLBACK_API
      tcp_err_fn err_fn = pcb->errf;
#endif /* LWIP_CALLBACK_API */
      void *err_arg = NULL;
      enum tcp_state last_state;
      tcp_pcb_purge(pcb);
      /* Remove PCB from tcp_active_pcbs list. */
      if (prev != NULL) {
        LWIP_ASSERT("tcp_slowtmr: middle tcp != tcp_active_pcbs", pcb != tcp_active_pcbs);
        prev->next = pcb->next;
      } else {
        /* This PCB was the first. */
        LWIP_ASSERT("tcp_slowtmr: first pcb == tcp_active_pcbs", tcp_active_pcbs == pcb);
        tcp_active_pcbs = pcb->next;
      }

      if (pcb_reset) {
        tcp_rst(pcb->snd_nxt, pcb->rcv_nxt, &pcb->local_ip, &pcb->remote_ip,
                pcb->local_port, pcb->remote_port, pcb);
      }

      err_arg = pcb->callback_arg;
      last_state = pcb->state;
      pcb2 = pcb;
      pcb = pcb->next;
      tcp_free(pcb2);

      tcp_active_pcbs_changed = 0;

      if (connect_timeout == 0) {
        TCP_EVENT_ERR(last_state, err_fn, err_arg, ERR_ABRT);
      } else {
        TCP_EVENT_ERR(last_state, err_fn, err_arg, ERR_CONNECTIMEOUT);
      }
      if (tcp_active_pcbs_changed) {
        goto tcp_slowtmr_start;
      }
    } else {
      /* get the 'next' element now and work with 'prev' below (in case of abort) */
      prev = pcb;
      pcb = pcb->next;

      /* We check if we should poll the connection. */
      ++prev->polltmr;
      if (prev->polltmr >= prev->pollinterval) {
        prev->polltmr = 0;
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: polling application\n"));
        tcp_active_pcbs_changed = 0;
        TCP_EVENT_POLL(prev, err);
        if (tcp_active_pcbs_changed) {
          goto tcp_slowtmr_start;
        }
        /* if err == ERR_ABRT, 'prev' is already deallocated */
        if (err == ERR_OK) {
          (void)tcp_output(prev);
        }
      }
    }
  }

  /* Steps through all of the TIME-WAIT PCBs. */
  prev = NULL;
  pcb = tcp_tw_pcbs;
  while (pcb != NULL) {
    LWIP_ASSERT("tcp_slowtmr: TIME-WAIT pcb->state == TIME-WAIT", pcb->state == TIME_WAIT);
    pcb_remove = 0;

    /* Check if this PCB has stayed long enough in TIME-WAIT */
    if ((u32_t)(tcp_ticks - pcb->tmr) > 2 * TCP_MSL / TCP_SLOW_INTERVAL) {
      ++pcb_remove;
    }

    /* If the PCB should be removed, do it. */
    if (pcb_remove) {
      struct tcp_pcb *pcb2;
      tcp_pcb_purge(pcb);
      /* Remove PCB from tcp_tw_pcbs list. */
      if (prev != NULL) {
        LWIP_ASSERT("tcp_slowtmr: middle tcp != tcp_tw_pcbs", pcb != tcp_tw_pcbs);
        prev->next = pcb->next;
      } else {
        /* This PCB was the first. */
        LWIP_ASSERT("tcp_slowtmr: first pcb == tcp_tw_pcbs", tcp_tw_pcbs == pcb);
        tcp_tw_pcbs = pcb->next;
      }
      pcb2 = pcb;
      pcb = pcb->next;
      tcp_free(pcb2);
    } else {
      prev = pcb;
      pcb = pcb->next;
    }
  }
}

/*
 * Is called every TCP_FAST_INTERVAL and process data previously
 * "refused" by upper layer (application) and sends delayed ACKs.
 *
 * Automatically called from tcp_tmr().
 */
void
tcp_fasttmr(void)
{
  err_t err;
  struct tcp_pcb *pcb = NULL;

  ++tcp_timer_ctr;

tcp_fasttmr_start:
  pcb = tcp_active_pcbs;

  while (pcb != NULL) {
#if LWIP_TCP_TLP_SUPPORT
    u32_t time_now = sys_now();
    if (pcb->tlp_time_stamp != 0) {
      LWIP_DEBUGF(TCP_TLP_DEBUG, ("tcp_fasttmr: pcb %p, PTO left %"S32_F", sys_now %"U32_F"\n",
                                  pcb, (s32_t)(pcb->tlp_time_stamp - time_now), time_now));
      if ((s32_t)(pcb->tlp_time_stamp - time_now) <= 0) {
        pcb->tlp_pto_cnt++;
        tcp_pto_fire(pcb);
        if (pcb->tlp_pto_cnt >= TCP_TLP_MAX_PROBE_CNT) {
          LWIP_TCP_TLP_CLEAR_VARS(pcb);
          if ((pcb->unacked != NULL) && (pcb->rtime == -1)) {
            pcb->rtime = 0;
          }
        }
      }
    }
#endif /* LWIP_TCP_TLP_SUPPORT */

    if (pcb->last_timer != tcp_timer_ctr) {
      struct tcp_pcb *next;
      pcb->last_timer = tcp_timer_ctr;
      /* send delayed ACKs */
      if (pcb->flags & TF_ACK_DELAY) {
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_fasttmr: delayed ACK\n"));
        tcp_ack_now(pcb);
        (void)tcp_output(pcb);
        pcb->flags = (tcpflags_t)(pcb->flags & (~(TF_ACK_DELAY | TF_ACK_NOW)));
      }
      /* send pending FIN */
      if (pcb->flags & TF_CLOSEPEND) {
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_fasttmr: pending FIN\n"));
        pcb->flags = (tcpflags_t)(pcb->flags & ~(TF_CLOSEPEND));
        (void)tcp_close_shutdown_fin(pcb);
      }

      next = pcb->next;

      /* If there is data which was previously "refused" by upper layer */
      if (pcb->refused_data != NULL) {
        tcp_active_pcbs_changed = 0;
        (void)tcp_process_refused_data(pcb);
        if (tcp_active_pcbs_changed) {
          /* application callback has changed the pcb list: restart the loop */
          goto tcp_fasttmr_start;
        }
      }

      if (pcb->tcp_pcb_flag & TCP_PBUF_FLAG_TCP_FIN_RECV_SYSPOST_FAIL) {
        TCP_EVENT_CLOSED(pcb, err);
        if (err == ERR_OK) {
          pcb->tcp_pcb_flag = (u8_t)(pcb->tcp_pcb_flag & (~TCP_PBUF_FLAG_TCP_FIN_RECV_SYSPOST_FAIL));
        }
      }

      pcb = next;
    } else {
      pcb = pcb->next;
    }
  }
}

#if LWIP_API_RICH
/** Call tcp_output for all active pcbs that have TF_NAGLEMEMERR set */
void
tcp_txnow(void)
{
  struct tcp_pcb *pcb = NULL;

  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
    if (pcb->flags & TF_NAGLEMEMERR) {
      (void)tcp_output(pcb);
    }
  }
}
#endif /* LWIP_API_RICH */

/** Pass pcb->refused_data to the recv callback */
err_t
tcp_process_refused_data(struct tcp_pcb *pcb)
{
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
  struct pbuf *rest = NULL;
  while (pcb->refused_data != NULL)
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
  {
    err_t err;
    u8_t refused_flags = (u8_t)pcb->refused_data->flags;
    /* set pcb->refused_data to NULL in case the callback frees it and then
       closes the pcb */
    struct pbuf *refused_data = pcb->refused_data;
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
    pbuf_split_64k(refused_data, &rest);
    pcb->refused_data = rest;
#else /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
    pcb->refused_data = NULL;
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
    /* Notify again application with data previously received. */
    LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: notify kept packet\n"));
    TCP_EVENT_RECV(pcb, refused_data, ERR_OK, err);
    if (err == ERR_OK) {
      /* did refused_data include a FIN? */
      if ((refused_flags & PBUF_FLAG_TCP_FIN)
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
          && (rest == NULL)
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
         ) {
        /* correct rcv_wnd as the application won't call tcp_recved()
           for the FIN's seqno */
        if (pcb->rcv_wnd != TCP_WND_MAX(pcb)) {
          pcb->rcv_wnd++;
        }
        TCP_EVENT_CLOSED(pcb, err);
        if (err == ERR_ABRT) {
          return ERR_ABRT;
        } else if (err == ERR_OK) {
          pcb->tcp_pcb_flag = (u8_t)(pcb->tcp_pcb_flag & (~TCP_PBUF_FLAG_TCP_FIN_RECV_SYSPOST_FAIL));
        } else {
          pcb->tcp_pcb_flag |= TCP_PBUF_FLAG_TCP_FIN_RECV_SYSPOST_FAIL;
        }
      }
    } else if (err == ERR_ABRT) {
      /* if err == ERR_ABRT, 'pcb' is already deallocated */
      /* Drop incoming packets because pcb is "full" (only if the incoming
         segment contains data). */
      LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: drop incoming packets, because pcb is \"full\"\n"));
      return ERR_ABRT;
    } else {
      /* data is still refused, pbuf is still valid (go on for ACK-only packets) */
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
      if (rest != NULL) {
        pbuf_cat(refused_data, rest);
      }
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
      pcb->refused_data = refused_data;
      return ERR_INPROGRESS;
    }
  }
  return ERR_OK;
}

#if LWIP_SACK_PERF_OPT
/*
 * Deallocates a list of Fast retransmitted TCP segments (tcp_seg structures).
 *
 * @param seg fr_segs list of TCP segments to free
 */
void
tcp_fr_segs_free(struct tcp_sack_fast_rxmited *seg)
{
  while (seg != NULL) {
    struct tcp_sack_fast_rxmited *next = seg->next;
    mem_free(seg);
    seg = next;
  }
}
#endif


/*
 * Deallocates a list of TCP segments (tcp_seg structures).
 *
 * @param seg tcp_seg list of TCP segments to free
 */
void
tcp_segs_free(struct tcp_seg *seg)
{
  while (seg != NULL) {
    struct tcp_seg *next = seg->next;
    tcp_seg_free(seg);
    seg = next;
  }
}

/**
 * Frees a TCP segment (tcp_seg structure).
 *
 * @param seg single tcp_seg to free
 */
void
tcp_seg_free(struct tcp_seg *seg)
{
  if (seg != NULL) {
    if (seg->p != NULL) {
      (void)pbuf_free(seg->p);
#if TCP_DEBUG
      seg->p = NULL;
#endif /* TCP_DEBUG */
    }
    memp_free(MEMP_TCP_SEG, seg);
  }
}

#if LWIP_API_RICH
/**
 * Sets the priority of a connection.
 *
 * @param pcb the tcp_pcb to manipulate
 * @param prio new priority
 */
void
tcp_setprio(struct tcp_pcb *pcb, u8_t prio)
{
  LWIP_ASSERT_CORE_LOCKED();
  pcb->prio = prio;
}
#endif /* LWIP_API_RICH */

#if TCP_QUEUE_OOSEQ
/**
 * Returns a copy of the given TCP segment.
 * The pbuf and data are not copied, only the pointers
 *
 * @param seg the old tcp_seg
 * @return a copy of seg
 */
struct tcp_seg *
tcp_seg_copy(struct tcp_seg *seg)
{
  struct tcp_seg *cseg = NULL;

  cseg = (struct tcp_seg *)memp_malloc(MEMP_TCP_SEG);
  if (cseg == NULL) {
    return NULL;
  }
  (void)memcpy_s((u8_t *)cseg, sizeof(struct tcp_seg), (const u8_t *)seg, sizeof(struct tcp_seg));
  pbuf_ref(cseg->p);
  return cseg;
}
#endif /* TCP_QUEUE_OOSEQ */

#if LWIP_CALLBACK_API
/**
 * Default receive callback that is called if the user didn't register
 * a recv callback for the pcb.
 */
err_t
tcp_recv_null(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  if (p != NULL) {
    tcp_recved(pcb, p->tot_len);
    (void)pbuf_free(p);
  } else if (err == ERR_OK) {
    return tcp_close(pcb);
  }
  return ERR_OK;
}
#endif /* LWIP_CALLBACK_API */

/**
 * Kills the oldest active connection that has the lower priority than
 * 'prio'.
 *
 * @param prio minimum priority
 */
static void
tcp_kill_prio(u8_t prio)
{
  struct tcp_pcb *pcb = NULL;
  struct tcp_pcb *inactive = NULL;
  u32_t inactivity;
  u8_t mprio;

  mprio = (u8_t)LWIP_MIN(TCP_PRIO_MAX, prio);

  /* We kill the oldest active connection that has lower priority than prio. */
  inactivity = 0;
  inactive = NULL;
  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
    if ((pcb->prio < mprio) &&
        (u32_t)(tcp_ticks - pcb->tmr) >= inactivity) {
      inactivity = tcp_ticks - pcb->tmr;
      inactive = pcb;
      mprio = pcb->prio;
    }
  }
  if (inactive != NULL) {
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_kill_prio: killing oldest PCB %p (%"S32_F")\n",
                            (void *)inactive, inactivity));
    tcp_abort(inactive);
  }
}

/**
 * Kills the oldest connection that is in specific state.
 * Called from tcp_alloc() for LAST_ACK and CLOSING if no more connections are available.
 */
static void
tcp_kill_state(enum tcp_state state)
{
  struct tcp_pcb *pcb = NULL;
  struct tcp_pcb *inactive = NULL;
  u32_t inactivity;

  LWIP_ASSERT("invalid state", (state == CLOSING) || (state == LAST_ACK));

  inactivity = 0;
  inactive = NULL;
  /* Go through the list of active pcbs and get the oldest pcb that is in state
     CLOSING/LAST_ACK. */
  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
    if (pcb->state == state) {
      if ((u32_t)(tcp_ticks - pcb->tmr) >= inactivity) {
        inactivity = tcp_ticks - pcb->tmr;
        inactive = pcb;
      }
    }
  }
  if (inactive != NULL) {
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_kill_closing: killing oldest %s PCB %p (%"S32_F")\n",
                            tcp_state_str[state], (void *)inactive, inactivity));
    /* Don't send a RST, since no data is lost. */
    tcp_abandon(inactive, 0);
  }
}

/**
 * Kills the oldest connection that is in TIME_WAIT state.
 * Called from tcp_alloc() if no more connections are available.
 */
static void
tcp_kill_timewait(void)
{
  struct tcp_pcb *pcb = NULL;
  struct tcp_pcb *inactive = NULL;
  u32_t inactivity;

  inactivity = 0;
  inactive = NULL;
  /* Go through the list of TIME_WAIT pcbs and get the oldest pcb. */
  for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
    if ((u32_t)(tcp_ticks - pcb->tmr) >= inactivity) {
      inactivity = tcp_ticks - pcb->tmr;
      inactive = pcb;
    }
  }
  if (inactive != NULL) {
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_kill_timewait: killing oldest TIME-WAIT PCB %p (%"S32_F")\n",
                            (void *)inactive, inactivity));
    tcp_abort(inactive);
  }
}

/* Called when allocating a pcb fails.
 * In this case, we want to handle all pcbs that want to close first: if we can
 * now send the FIN (which failed before), the pcb might be in a state that is
 * OK for us to now free it.
 */
void
tcp_handle_closepend(void)
{
  struct tcp_pcb *pcb = tcp_active_pcbs;

  while (pcb != NULL) {
    struct tcp_pcb *next = pcb->next;
    /* send pending FIN */
    if (pcb->flags & TF_CLOSEPEND) {
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_handle_closepend: pending FIN\n"));
      pcb->flags = (tcpflags_t)(pcb->flags & ~(TF_CLOSEPEND));
      (void)tcp_close_shutdown_fin(pcb);
    }
    pcb = next;
  }

  return;
}

/**
 * Allocate a new tcp_pcb structure.
 *
 * @param prio priority for the new pcb
 * @return a new tcp_pcb that initially is in state CLOSED
 */
struct tcp_pcb *
tcp_alloc(u8_t prio)
{
  struct tcp_pcb *pcb = NULL;

  LWIP_ASSERT_CORE_LOCKED();

  pcb = (struct tcp_pcb *)memp_malloc(MEMP_TCP_PCB);
  if (pcb == NULL) {
    /* Try to send FIN for all pcbs stuck in TF_CLOSEPEND first */
    tcp_handle_closepend();

    /* Try killing oldest connection in TIME-WAIT. */
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_alloc: killing off oldest TIME-WAIT connection\n"));
    tcp_kill_timewait();
    /* Try to allocate a tcp_pcb again. */
    pcb = (struct tcp_pcb *)memp_malloc(MEMP_TCP_PCB);
    if (pcb == NULL) {
      /* Try killing oldest connection in LAST-ACK (these wouldn't go to TIME-WAIT). */
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_alloc: killing off oldest LAST-ACK connection\n"));
      tcp_kill_state(LAST_ACK);
      /* Try to allocate a tcp_pcb again. */
      pcb = (struct tcp_pcb *)memp_malloc(MEMP_TCP_PCB);
      if (pcb == NULL) {
        /* Try killing oldest connection in CLOSING. */
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_alloc: killing off oldest CLOSING connection\n"));
        tcp_kill_state(CLOSING);
        /* Try to allocate a tcp_pcb again. */
        pcb = (struct tcp_pcb *)memp_malloc(MEMP_TCP_PCB);
        if (pcb == NULL) {
          /* Try killing active connections with lower priority than the new one. */
          LWIP_DEBUGF(TCP_DEBUG, ("tcp_alloc: killing connection with prio lower than %d\n", prio));
          tcp_kill_prio(prio);
          /* Try to allocate a tcp_pcb again. */
          pcb = (struct tcp_pcb *)memp_malloc(MEMP_TCP_PCB);
          if (pcb != NULL) {
            /* adjust err stats: memp_malloc failed multiple times before */
            MEMP_STATS_DEC(err, MEMP_TCP_PCB);
          }
        }
        if (pcb != NULL) {
          /* adjust err stats: memp_malloc failed multiple times before */
          MEMP_STATS_DEC(err, MEMP_TCP_PCB);
        }
      }
      if (pcb != NULL) {
        /* adjust err stats: memp_malloc failed multiple times before */
        MEMP_STATS_DEC(err, MEMP_TCP_PCB);
      }
    }
    if (pcb != NULL) {
      /* adjust err stats: memp_malloc failed above */
      MEMP_STATS_DEC(err, MEMP_TCP_PCB);
    }
  }
  if (pcb != NULL) {
    /* zero out the whole pcb, so there is no need to initialize members to zero */
    (void)memset_s(pcb, sizeof(struct tcp_pcb), 0, sizeof(struct tcp_pcb));
    pcb->prio = prio;
    pcb->snd_buf = TCP_SND_BUF;
#if LWIP_SO_SNDBUF
    pcb->snd_buf_static = TCP_SND_BUF;
#endif

    pcb->snd_queuelen_max = TCP_SND_QUEUELEN;
    pcb->snd_queuelen_lowat = pcb->snd_queuelen_max >> 1;
    pcb->snd_buf_lowat = pcb->snd_buf_static >> 1;

    /* Start with a window that does not need scaling. When window scaling is
       enabled and used, the window is enlarged when both sides agree on scaling. */
    pcb->rcv_wnd = pcb->rcv_ann_wnd = TCPWND_MIN16(TCP_WND);
    if (pbuf_ram_in_deflation() != lwIP_FALSE) {
      pcb->rcv_ann_wnd = LWIP_MIN(pcb->rcv_ann_wnd, TCP_MIN_RCV_ANN_WND);
    }

    pcb->ttl = TCP_TTL;
    /* As initial send MSS, we use TCP_MSS but limit it to 536.
       The send MSS is updated when an MSS option is received. */
    pcb->mss = INITIAL_MSS;
    pcb->rcv_mss = INITIAL_MSS;    // Default  value if MSS is not received.

    /*
     * RFC 6298 section 2.1
     * Until a round-trip time (RTT) measurement has been made for a
     * segment sent between the sender and receiver, the sender SHOULD
     * set RTO <- 1 second, though the "backing off" on repeated
     * retransmission discussed in (5.5) still applies.
     */
    pcb->rto = (TCP_INITIAL_RTO_DURATION / TCP_SLOW_INTERVAL);
    pcb->sv = 0;
    pcb->sa = -1; /* -1 means no valid RTT sample */
    pcb->rtime = -1;
    pcb->tmr = tcp_ticks;
    pcb->last_timer = tcp_timer_ctr;
    pcb->persist_probe = 0;

#if LWIP_TCP_TLP_SUPPORT
    LWIP_TCP_TLP_CLEAR_VARS(pcb);
#endif /* LWIP_TCP_TLP_SUPPORT */

    LWIP_TCP_CALC_INITIAL_CWND(pcb->mss, pcb->iw);
    pcb->cwnd = pcb->iw;
    pcb->bytes_acked = 0;

    /* RFC 5681 recommends setting ssthresh abritrarily high and gives an example
    of using the largest advertised receive window.  We've seen complications with
    receiving TCPs that use window scaling and/or window auto-tuning where the
    initial advertised window is very small and then grows rapidly once the
    connection is established. To avoid these complications, we set ssthresh to the
    largest effective cwnd (amount of in-flight data) that the sender can have. */
    pcb->ssthresh = TCP_SND_BUF;

#if LWIP_CALLBACK_API
    pcb->recv = tcp_recv_null;
#endif /* LWIP_CALLBACK_API */

    /* Init KEEPALIVE timer */
    pcb->keep_idle  = TCP_KEEPIDLE_DEFAULT;

#if LWIP_TCP_KEEPALIVE
    pcb->keep_intvl = TCP_KEEPINTVL_DEFAULT;
    pcb->keep_cnt   = TCP_KEEPCNT_DEFAULT;
#endif /* LWIP_TCP_KEEPALIVE */
    pcb->keep_cnt_sent = 0;
    pcb->tcp_pcb_flag = 0;
#if LWIP_SACK
    pcb->sacked = 0;
#endif
  }
  return pcb;
}

#if LWIP_API_RICH
/**
 * @ingroup tcp_raw
 * Creates a new TCP protocol control block but doesn't place it on
 * any of the TCP PCB lists.
 * The pcb is not put on any list until binding using tcp_bind().
 *
 * @internal: Maybe there should be a idle TCP PCB list where these
 * PCBs are put on. Port reservation using tcp_bind() is implemented but
 * allocated pcbs that are not bound can't be killed automatically if wanting
 * to allocate a pcb with higher prio (@see tcp_kill_prio())
 *
 * @return a new tcp_pcb that initially is in state CLOSED
 */
struct tcp_pcb *
tcp_new(void)
{
  return tcp_alloc(TCP_PRIO_NORMAL);
}
#endif /* LWIP_API_RICH */

/**
 * @ingroup tcp_raw
 * Creates a new TCP protocol control block but doesn't
 * place it on any of the TCP PCB lists.
 * The pcb is not put on any list until binding using tcp_bind().
 *
 * @param type IP address type, see @ref lwip_ip_addr_type definitions.
 * If you want to listen to IPv4 and IPv6 (dual-stack) connections,
 * supply @ref IPADDR_TYPE_ANY as argument and bind to @ref IP_ANY_TYPE.
 * @return a new tcp_pcb that initially is in state CLOSED
 */
struct tcp_pcb *
tcp_new_ip_type(u8_t type)
{
  struct tcp_pcb *pcb = NULL;
  pcb = tcp_alloc(TCP_PRIO_NORMAL);
#if LWIP_IPV4 && LWIP_IPV6
  if (pcb != NULL) {
    IP_SET_TYPE_VAL(pcb->local_ip, type);
    IP_SET_TYPE_VAL(pcb->remote_ip, type);
  }
#else
  LWIP_UNUSED_ARG(type);
#endif /* LWIP_IPV4 && LWIP_IPV6 */
  return pcb;
}

/**
 * @ingroup tcp_raw
 * Used to specify the argument that should be passed callback
 * functions.
 *
 * @param pcb tcp_pcb to set the callback argument
 * @param arg void pointer argument to pass to callback functions
 */
void
tcp_arg(struct tcp_pcb *pcb, void *arg)
{
  LWIP_ASSERT_CORE_LOCKED();
  /* This function is allowed to be called for both listen pcbs and
     connection pcbs. */
  if (pcb != NULL) {
    pcb->callback_arg = arg;
  }
}
#if LWIP_CALLBACK_API

/**
 * @ingroup tcp_raw
 * Used to specify the function that should be called when a TCP
 * connection receives data.
 *
 * @param pcb tcp_pcb to set the recv callback
 * @param recv callback function to call for this pcb when data is received
 */
void
tcp_recv(struct tcp_pcb *pcb, tcp_recv_fn recv_fn)
{
  LWIP_ASSERT_CORE_LOCKED();
  if (pcb != NULL) {
    LWIP_ASSERT("invalid socket state for recv callback", pcb->state != LISTEN);
    pcb->recv = recv_fn;
  }
}

/**
 * @ingroup tcp_raw
 * Used to specify the function that should be called when TCP data
 * has been successfully delivered to the remote host.
 *
 * @param pcb tcp_pcb to set the sent callback
 * @param sent callback function to call for this pcb when data is successfully sent
 */
void
tcp_sent(struct tcp_pcb *pcb, tcp_sent_fn sent)
{
  LWIP_ASSERT_CORE_LOCKED();
  if (pcb != NULL) {
    LWIP_ASSERT("invalid socket state for sent callback", pcb->state != LISTEN);
    pcb->sent = sent;
  }
}

/**
 * @ingroup tcp_raw
 * Used to specify the function that should be called when a fatal error
 * has occurred on the connection.
 *
 * @note The corresponding pcb is already freed when this callback is called!
 *
 * @param pcb tcp_pcb to set the err callback
 * @param err callback function to call for this pcb when a fatal error
 *        has occurred on the connection
 */
void
tcp_err(struct tcp_pcb *pcb, tcp_err_fn err)
{
  LWIP_ASSERT_CORE_LOCKED();
  if (pcb != NULL) {
    LWIP_ASSERT("invalid socket state for err callback", pcb->state != LISTEN);
    pcb->errf = err;
  }
}

/**
 * @ingroup tcp_raw
 * Used for specifying the function that should be called when a
 * LISTENing connection has been connected to another host.
 *
 * @param pcb tcp_pcb to set the accept callback
 * @param accept callback function to call for this pcb when LISTENing
 *        connection has been connected to another host
 */
void
tcp_accept(struct tcp_pcb *pcb, tcp_accept_fn accept_fn)
{
  LWIP_ASSERT_CORE_LOCKED();
  if ((pcb != NULL) && (pcb->state == LISTEN)) {
    struct tcp_pcb_listen *lpcb = (struct tcp_pcb_listen *)pcb;
    lpcb->accept = accept_fn;
  }
}
#endif /* LWIP_CALLBACK_API */


/**
 * @ingroup tcp_raw
 * Used to specify the function that should be called periodically
 * from TCP. The interval is specified in terms of the TCP coarse
 * timer interval, which is called twice a second.
 *
 */
void
tcp_poll(struct tcp_pcb *pcb, tcp_poll_fn poll, u8_t interval)
{
  LWIP_ASSERT_CORE_LOCKED();
  LWIP_ASSERT("invalid socket state for poll", pcb->state != LISTEN);
#if LWIP_CALLBACK_API
  pcb->poll = poll;
#else /* LWIP_CALLBACK_API */
  LWIP_UNUSED_ARG(poll);
#endif /* LWIP_CALLBACK_API */
  pcb->pollinterval = interval;
}

/**
 * Purges a TCP PCB. Removes any buffered data and frees the buffer memory
 * (pcb->ooseq, pcb->unsent and pcb->unacked are freed).
 *
 * @param pcb tcp_pcb to purge. The pcb itself is not deallocated!
 */
void
tcp_pcb_purge(struct tcp_pcb *pcb)
{
#if LWIP_SACK
  struct _sack_seq *ptr = NULL;
#endif
  if (pcb->state != CLOSED &&
      pcb->state != TIME_WAIT &&
      pcb->state != LISTEN) {

    LWIP_DEBUGF(TCP_DEBUG, ("tcp_pcb_purge\n"));

    tcp_backlog_accepted(pcb);

    if (pcb->refused_data != NULL) {
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_pcb_purge: data left on ->refused_data\n"));
      (void)pbuf_free(pcb->refused_data);
      pcb->refused_data = NULL;
    }
    if (pcb->unsent != NULL) {
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_pcb_purge: not all data sent\n"));
    }
    if (pcb->unacked != NULL) {
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_pcb_purge: data left on ->unacked\n"));
    }
#if TCP_QUEUE_OOSEQ
    if (pcb->ooseq != NULL) {
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_pcb_purge: data left on ->ooseq\n"));
    }
    tcp_segs_free(pcb->ooseq);
    pcb->ooseq = NULL;
#endif /* TCP_QUEUE_OOSEQ */

    /* Stop the retransmission timer as it will expect data on unacked
       queue if it fires */
    pcb->rtime = -1;

    tcp_segs_free(pcb->unsent);
    tcp_segs_free(pcb->unacked);
    pcb->unacked = pcb->unsent = NULL;
#if LWIP_SACK_PERF_OPT
    tcp_fr_segs_free(pcb->fr_segs);
    pcb->fr_segs = NULL;
    pcb->last_frseg = NULL;
#endif

#if LWIP_SACK
    if (pcb->sack_seq != NULL) {
      do {
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_pcb_purge: Freeing sack options data\n"));
        ptr = pcb->sack_seq->next;
        mem_free(pcb->sack_seq);
        pcb->sack_seq = ptr;
      } while (pcb->sack_seq != NULL);
      pcb->sack_seq = NULL;
    }
#endif

#if TCP_OVERSIZE
    pcb->unsent_oversize = 0;
#endif /* TCP_OVERSIZE */
  }
}

/*
 * Removes listen pcb  from a PCB list.
 *
 * @param pcblist PCB list from which listen pcb to be remvoed
 * @param pcb tcp_pcb_listen to be removed. The pcb itself is NOT deallocated!
 * Note: 'pcb' from 'pcblist' is from 'tcp_listen_pcbs'
 */
void
tcp_listen_pcb_remove(struct tcp_pcb **pcblist, struct tcp_pcb_listen *lpcb)
{
  TCP_RMV(pcblist, (struct tcp_pcb *)lpcb);

  lpcb->state = CLOSED;
  /* reset the local port to prevent the pcb from being 'bound' */
  lpcb->local_port = 0;

  LWIP_ASSERT("tcp_listen_pcb_remove: tcp_pcbs_sane()", tcp_pcbs_sane());
}

#if LWIP_SMALL_SIZE
void tcp_remove(struct tcp_pcb **pcbs, struct tcp_pcb *npcb)
{
  if (*(pcbs) == (npcb)) {
    (*(pcbs)) = (*pcbs)->next;
  }
  else {
    struct tcp_pcb *tcp_tmp_pcb;
    for (tcp_tmp_pcb = *pcbs;
        tcp_tmp_pcb != NULL;
        tcp_tmp_pcb = tcp_tmp_pcb->next) {
      if (tcp_tmp_pcb->next == (npcb)) {
        tcp_tmp_pcb->next = (npcb)->next;
        break;
      }
    }
  }
  (npcb)->next = NULL;
}
#endif

void tcp_sndbuf_init(struct tcp_pcb *pcb)
{
  u32_t sndqueuemax;
  tcpwnd_size_t snd_buf = pcb->snd_buf_static;
  tcpwnd_size_t mss = pcb->mss;

  if ((snd_buf >> 1) > ((mss << 1) + 1)) {
    if ((snd_buf >> 1) < (snd_buf - 1)) {
      pcb->snd_buf_lowat = snd_buf >> 1;
    } else {
      pcb->snd_buf_lowat = snd_buf - 1;
    }
  } else {
    if (((mss << 1) + 1) < (snd_buf - 1)) {
      pcb->snd_buf_lowat = (mss << 1) + 1;
    } else {
      pcb->snd_buf_lowat = snd_buf - 1;
    }
  }

  sndqueuemax = ((snd_buf / mss) << 3);
  if (sndqueuemax > SNDQUEUE_MAX) {
    sndqueuemax = SNDQUEUE_MAX;
  }
  pcb->snd_queuelen_max = (u16_t)sndqueuemax;
  pcb->snd_queuelen_lowat = pcb->snd_queuelen_max >> 1;
  if (pcb->snd_queuelen_lowat < SNDQUEUE_MIN) {
    pcb->snd_queuelen_lowat = LWIP_MIN(SNDQUEUE_MIN, pcb->snd_queuelen_max);
  }
}

/**
 * Purges the PCB and removes it from a PCB list. Any delayed ACKs are sent first.
 *
 * @param pcblist PCB list to purge.
 * @param pcb tcp_pcb to purge. The pcb itself is NOT deallocated!
 * Note: 'pcb' from 'pcblist' can be either 'tcp_tw_pcbs' or 'tcp_active_pcbs'
 */
void
tcp_pcb_remove(struct tcp_pcb **pcblist, struct tcp_pcb *pcb)
{
  TCP_RMV(pcblist, pcb);

  tcp_pcb_purge(pcb);

  /* if there is an outstanding delayed ACKs, send it */
  if ((pcb->state != TIME_WAIT) && (pcb->flags & TF_ACK_DELAY)) {
    pcb->flags |= TF_ACK_NOW;
    (void)tcp_output(pcb);
  }

  LWIP_ASSERT("unsent segments leaking", pcb->unsent == NULL);
  LWIP_ASSERT("unacked segments leaking", pcb->unacked == NULL);
#if TCP_QUEUE_OOSEQ
  LWIP_ASSERT("ooseq segments leaking", pcb->ooseq == NULL);
#endif /* TCP_QUEUE_OOSEQ */
#if LWIP_SACK
  pcb->sacked = 0;
#if LWIP_TCP_TLP_SUPPORT
  LWIP_TCP_TLP_CLEAR_VARS(pcb);
#endif /* LWIP_TCP_TLP_SUPPORT */
#endif /* LWIP_SACK */

  pcb->state = CLOSED;
  /* reset the local port to prevent the pcb from being 'bound' */
  pcb->local_port = 0;

  pcb->tcp_pcb_flag = 0;
  LWIP_ASSERT("tcp_pcb_remove: tcp_pcbs_sane()", tcp_pcbs_sane());
}

/**
 * Calculates a new initial sequence number for new connections.
 *
 * @return u32_t pseudo random sequence number
 */
u32_t
tcp_next_iss(struct tcp_pcb *pcb)
{
#ifdef LWIP_HOOK_TCP_ISN
  return LWIP_HOOK_TCP_ISN(&pcb->local_ip, pcb->local_port, &pcb->remote_ip, pcb->remote_port);
#else /* LWIP_HOOK_TCP_ISN */
  static u32_t iss = 6510;

  LWIP_UNUSED_ARG(pcb);
  iss = iss + (u32_t)(LWIP_RAND());

  return iss;
#endif /* LWIP_HOOK_TCP_ISN */
}

#if TCP_CALCULATE_EFF_SEND_MSS
/**
 * Calculates the effective send mss that can be used for a specific IP address
 * by using ip_route to determine the netif used to send to the address and
 * calculating the minimum of TCP_MSS and that netif's mtu (if set).
 */
u16_t
tcp_eff_send_mss_impl(u16_t sendmss, const ip_addr_t *dest
#if LWIP_IPV6 || LWIP_IPV4_SRC_ROUTING
                      , const ip_addr_t *src
#endif /* LWIP_IPV6 || LWIP_IPV4_SRC_ROUTING */
                      , struct netif *outif)
{
  u16_t mss_s;
  u16_t offset;

  u16_t mtu;

#if LWIP_IPV6 || LWIP_IPV4_SRC_ROUTING
  (void)(src);
#endif /* LWIP_IPV6 || LWIP_IPV4_SRC_ROUTING */

#if LWIP_IPV6
#if LWIP_IPV4
  if (IP_IS_V6(dest))
#endif /* LWIP_IPV4 */
  {
    /* First look in destination cache, to see if there is a Path MTU. */
    mtu = nd6_get_destination_mtu(ip_2_ip6(dest), outif);
  }
#if LWIP_IPV4
  else
#endif /* LWIP_IPV4 */
#endif /* LWIP_IPV6 */
#if LWIP_IPV4
  {
    if (outif == NULL) {
      /* limit max value which can be supported on the stack */
      mss_s = (u16_t)LWIP_MIN(sendmss, (u16_t)(IP_FRAG_MAX_MTU - IP_HLEN - TCP_HLEN));
      return mss_s;
    }
    mtu = outif->mtu;
  }
#endif /* LWIP_IPV4 */

  if (mtu != 0) {
#if LWIP_IPV6
#if LWIP_IPV4
    if (IP_IS_V6(dest))
#endif /* LWIP_IPV4 */
    {
      offset = (u16_t)(IP6_HLEN + TCP_HLEN);
    }
#if LWIP_IPV4
    else
#endif /* LWIP_IPV4 */
#endif /* LWIP_IPV6 */
#if LWIP_IPV4
    {
      offset = (u16_t)(IP_HLEN + TCP_HLEN);
    }
#endif /* LWIP_IPV4 */

#if LWIP_RIPPLE && LWIP_IPV6
    if (IP_IS_V6(dest) && !ip6_addr_islinklocal(ip_2_ip6(dest)) &&
        (lwip_rpl_is_router() == lwIP_TRUE) && (lwip_rpl_is_rpl_netif(outif) != lwIP_FALSE)) {
      offset += lwip_hbh_len(NULL);
    }
#endif
    /* MMS_S is the maximum size for a transport-layer message that TCP may send, do not keep 0 mss */
    mss_s = (u16_t)((mtu > offset) ? (u16_t)(mtu - offset) : (u16_t)TCP_MSS);

    /* RFC 1122, chap 4.2.2.6:
     * Eff.snd.MSS = min(SendMSS+20, MMS_S) - TCPhdrsize - IPoptionsize
     * We correct for TCP options in tcp_write(), and don't support IP options.
     */
    sendmss = LWIP_MIN(sendmss, mss_s);
  }
  return sendmss;
}
#endif /* TCP_CALCULATE_EFF_SEND_MSS */

/** Helper function for tcp_netif_ip_addr_changed() that iterates a pcb list */
static void
tcp_netif_ip_addr_changed_pcblist(const ip_addr_t *old_addr, struct tcp_pcb *pcb_list)
{
  struct tcp_pcb *pcb = NULL;
  pcb = pcb_list;
  while (pcb != NULL) {
    /* PCB bound to current local interface address? */
    if (ip_addr_cmp(&pcb->local_ip, old_addr)
#if LWIP_AUTOIP
        /* connections to link-local addresses must persist (RFC3927 ch. 1.9) */
        && (!IP_IS_V4_VAL(pcb->local_ip) || !ip4_addr_islinklocal(ip_2_ip4(&pcb->local_ip)))
#endif /* LWIP_AUTOIP */
       ) {
      /* this connection must be aborted */
      struct tcp_pcb *next = pcb->next;
      LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_STATE, ("netif_set_ipaddr: aborting TCP pcb %p\n", (void *)pcb));
      tcp_abort(pcb);
      pcb = next;
    } else {
      pcb = pcb->next;
    }
  }
}

/** This function is called from netif.c when address is changed or netif is removed
 *
 * @param old_addr IP address of the netif before change
 * @param new_addr IP address of the netif after change or NULL if netif has been removed
 */
void
tcp_netif_ip_addr_changed(const ip_addr_t *old_addr, const ip_addr_t *new_addr)
{
  struct tcp_pcb_listen *lpcb = NULL;
  struct tcp_pcb_listen *next = NULL;

  if ((old_addr != NULL) && !ip_addr_isany_val(*old_addr)) {
    tcp_netif_ip_addr_changed_pcblist(old_addr, tcp_active_pcbs);
    tcp_netif_ip_addr_changed_pcblist(old_addr, tcp_bound_pcbs);

    if (!ip_addr_isany(new_addr)) {
      /* PCB bound to current local interface address? */
      for (lpcb = tcp_listen_pcbs.listen_pcbs; lpcb != NULL; lpcb = next) {
        next = lpcb->next;
        /* PCB bound to current local interface address? */
        if (ip_addr_cmp(&lpcb->local_ip, old_addr)) {
          /* The PCB is listening to the old ipaddr and
            * is set to listen to the new one instead */
          ip_addr_copy(lpcb->local_ip, *new_addr);
        }
      }
    }
  }
}

#if API_MSG_DEBUG || (defined LWIP_DEBUG)
const char *
tcp_debug_state_str(enum tcp_state s)
{
  return tcp_state_str[s];
}
#endif /* API_MSG_DEBUG */

#if TCP_DEBUG || TCP_INPUT_DEBUG || TCP_OUTPUT_DEBUG
/**
 * Print a tcp header for debugging purposes.
 *
 * @param tcphdr pointer to a struct tcp_hdr
 */
void
tcp_debug_print(struct tcp_hdr *tcphdr)
{
  LWIP_DEBUGF(TCP_DEBUG, ("TCP header:\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("|    %5"U16_F"      |    %5"U16_F"      | (src port, dest port)\n",
                          lwip_ntohs(tcphdr->src), lwip_ntohs(tcphdr->dest)));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("|           %010"U32_F"          | (seq no)\n",
                          lwip_ntohl(tcphdr->seqno)));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("|           %010"U32_F"          | (ack no)\n",
                          lwip_ntohl(tcphdr->ackno)));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(TCP_DEBUG,
              ("| %2"U16_F" |   |%"U16_F"%"U16_F"%"U16_F"%"U16_F"%"U16_F"%"U16_F""
               "|     %5"U16_F"     | (hdrlen, flags (",
               TCPH_HDRLEN(tcphdr),
               (u16_t)(TCPH_FLAGS(tcphdr) >> 5 & 1),
               (u16_t)(TCPH_FLAGS(tcphdr) >> 4 & 1),
               (u16_t)(TCPH_FLAGS(tcphdr) >> 3 & 1),
               (u16_t)(TCPH_FLAGS(tcphdr) >> 2 & 1),
               (u16_t)(TCPH_FLAGS(tcphdr) >> 1 & 1),
               (u16_t)(TCPH_FLAGS(tcphdr)      & 1),
               lwip_ntohs(tcphdr->wnd)));
  tcp_debug_print_flags(TCPH_FLAGS(tcphdr));
  LWIP_DEBUGF(TCP_DEBUG, ("), win)\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("|    0x%04"X16_F"     |     %5"U16_F"     | (chksum, urgp)\n",
                          lwip_ntohs(tcphdr->chksum), lwip_ntohs(tcphdr->urgp)));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
}

/**
 * Print a tcp state for debugging purposes.
 *
 * @param s enum tcp_state to print
 */
void
tcp_debug_print_state(enum tcp_state s)
{
  LWIP_DEBUGF(TCP_DEBUG, ("State: %s\n", tcp_state_str[s]));
}

/**
 * Print tcp flags for debugging purposes.
 *
 * @param flags tcp flags, all active flags are printed
 */
void
tcp_debug_print_flags(u8_t flags)
{
  if (flags & TCP_FIN) {
    LWIP_DEBUGF(TCP_DEBUG, ("FIN "));
  }
  if (flags & TCP_SYN) {
    LWIP_DEBUGF(TCP_DEBUG, ("SYN "));
  }
  if (flags & TCP_RST) {
    LWIP_DEBUGF(TCP_DEBUG, ("RST "));
  }
  if (flags & TCP_PSH) {
    LWIP_DEBUGF(TCP_DEBUG, ("PSH "));
  }
  if (flags & TCP_ACK) {
    LWIP_DEBUGF(TCP_DEBUG, ("ACK "));
  }
  if (flags & TCP_URG) {
    LWIP_DEBUGF(TCP_DEBUG, ("URG "));
  }
  if (flags & TCP_ECE) {
    LWIP_DEBUGF(TCP_DEBUG, ("ECE "));
  }
  if (flags & TCP_CWR) {
    LWIP_DEBUGF(TCP_DEBUG, ("CWR "));
  }
  LWIP_DEBUGF(TCP_DEBUG, ("\n"));
}

/**
 * Print all tcp_pcbs in every list for debugging purposes.
 */
void
tcp_debug_print_pcbs(void)
{
  struct tcp_pcb *pcb = NULL;
  struct tcp_pcb_listen *pcbl = NULL;

  LWIP_DEBUGF(TCP_DEBUG, ("Active PCB states:\n"));
  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
    LWIP_DEBUGF(TCP_DEBUG, ("Local port %"U16_F", foreign port %"U16_F" snd_nxt %"U32_F" rcv_nxt %"U32_F" ",
                            pcb->local_port, pcb->remote_port,
                            pcb->snd_nxt, pcb->rcv_nxt));
    tcp_debug_print_state(pcb->state);
  }

  LWIP_DEBUGF(TCP_DEBUG, ("Listen PCB states:\n"));
  for (pcbl = tcp_listen_pcbs.listen_pcbs; pcbl != NULL; pcbl = pcbl->next) {
    LWIP_DEBUGF(TCP_DEBUG, ("Local port %"U16_F" ", pcbl->local_port));
    tcp_debug_print_state(pcbl->state);
  }

  LWIP_DEBUGF(TCP_DEBUG, ("TIME-WAIT PCB states:\n"));
  for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
    LWIP_DEBUGF(TCP_DEBUG, ("Local port %"U16_F", foreign port %"U16_F" snd_nxt %"U32_F" rcv_nxt %"U32_F" ",
                            pcb->local_port, pcb->remote_port,
                            pcb->snd_nxt, pcb->rcv_nxt));
    tcp_debug_print_state(pcb->state);
  }
}

/**
 * Check state consistency of the tcp_pcb lists.
 */
s16_t
tcp_pcbs_sane(void)
{
  struct tcp_pcb *pcb = NULL;
  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
    LWIP_ASSERT("tcp_pcbs_sane: active pcb->state != CLOSED", pcb->state != CLOSED);
    LWIP_ASSERT("tcp_pcbs_sane: active pcb->state != LISTEN", pcb->state != LISTEN);
    LWIP_ASSERT("tcp_pcbs_sane: active pcb->state != TIME-WAIT", pcb->state != TIME_WAIT);
  }
  for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
    LWIP_ASSERT("tcp_pcbs_sane: tw pcb->state == TIME-WAIT", pcb->state == TIME_WAIT);
  }
  return 1;
}
#endif /* TCP_DEBUG */

#if LWIP_TCP_INFO
/*
 * mapping lwIP-tcp-state to Linux-tcp-state.
 * remove this function by making lwIP-tcp-state consistent with Linux-tcp-state in the future.
 */
LWIP_STATIC u8_t
lwip_tcp_state_mapping(enum tcp_state state)
{
  u8_t ret;
  switch (state) {
    case CLOSED:
      ret = TCP_CLOSE;
      break;
    case LISTEN:
      ret = TCP_LISTEN;
      break;
    case SYN_SENT:
      ret = TCP_SYN_SENT;
      break;
    case SYN_RCVD:
      ret = TCP_SYN_RECV;
      break;
    case ESTABLISHED:
      ret = TCP_ESTABLISHED;
      break;
    case FIN_WAIT_1:
      ret = TCP_FIN_WAIT1;
      break;
    case FIN_WAIT_2:
      ret = TCP_FIN_WAIT2;
      break;
    case CLOSE_WAIT:
      ret = TCP_CLOSE_WAIT;
      break;
    case CLOSING:
      ret = TCP_CLOSING;
      break;
    case LAST_ACK:
      ret = TCP_LAST_ACK;
      break;
    case TIME_WAIT:
      ret = TCP_TIME_WAIT;
      break;
    default:
      ret = TCP_CLOSE;
  }

  return ret;
}

/*
 * function to get tcp information from lwip stack
 */
void
tcp_get_info(const struct tcp_pcb *pcb, struct tcp_info *tcpinfo)
{
  struct tcp_seg *useg = NULL;
  (void)memset_s(tcpinfo, sizeof(struct tcp_info), 0, sizeof(struct tcp_info));

  tcpinfo->tcpi_state = lwip_tcp_state_mapping(pcb->state);

  /* No data for listening socket */
  if (pcb->state == LISTEN) {
    return;
  }

  /* RTO retransmissions backoff, tcpi_retransmits equals tcpi_backoff for non-Thin stream */
  tcpinfo->tcpi_retransmits = pcb->nrtx;
  tcpinfo->tcpi_backoff = pcb->nrtx;

  /* Number of keep alive probes or zero window probes */
  tcpinfo->tcpi_probes = (u8_t)(pcb->keep_cnt_sent ? pcb->keep_cnt_sent : pcb->persist_backoff);

#if LWIP_TCP_TIMESTAMPS
  if (pcb->flags & TF_TIMESTAMP) {
    tcpinfo->tcpi_options = tcpinfo->tcpi_options | TCPI_OPT_TIMESTAMPS;
  }
#endif

#if LWIP_SACK
  if (pcb->flags & TF_SACK) {
    tcpinfo->tcpi_options = tcpinfo->tcpi_options | TCPI_OPT_SACK;
  }
#endif

#if LWIP_WND_SCALE
  if (pcb->flags & TF_WND_SCALE) {
    tcpinfo->tcpi_options = tcpinfo->tcpi_options | TCPI_OPT_WSCALE;
  }
#endif

  /* RTO duration in usec */
  tcpinfo->tcpi_rto = ((u32_t)(s32_t)pcb->rto * TCP_SLOW_INTERVAL * US_PER_MSECOND);

  /* receive option else same value as send mss */
  tcpinfo->tcpi_snd_mss = pcb->mss;
  tcpinfo->tcpi_rcv_mss = pcb->rcv_mss;

  useg = pcb->unacked;

  /* only unacked segments, sacked or fast retransmitted packets are not counted */
  u32_t unacked = 0;
  for (; useg != NULL; useg = useg->next) {
    unacked++;
  }
  tcpinfo->tcpi_unacked = unacked;

  unacked = 0;
  for (useg = pcb->unsent; useg != NULL; useg = useg->next) {
    if ((ntohl(useg->tcphdr->seqno) + (u32_t)(TCP_TCPLEN(useg)) - 1) < pcb->snd_nxt) {
      unacked++;
    } else {
      break;
    }
  }
  tcpinfo->tcpi_unacked += unacked;

  /* time in us */
  if (pcb->sa == -1) {
    tcpinfo->tcpi_rtt = 0;
    tcpinfo->tcpi_rttvar = 0;
  } else {
    tcpinfo->tcpi_rtt = (((u32_t)pcb->sa) >> 3) * TCP_SLOW_INTERVAL * US_PER_MSECOND;
    tcpinfo->tcpi_rttvar = (((u32_t)pcb->sv) >> 2) * TCP_SLOW_INTERVAL * US_PER_MSECOND;
  }
  /* congestion wnd and slow start threshold */
  tcpinfo->tcpi_snd_ssthresh = pcb->ssthresh;
  tcpinfo->tcpi_snd_cwnd = pcb->cwnd;

  /* constant reordering */
  tcpinfo->tcpi_reordering = DUPACK_THRESH;
}
#endif

#if DRIVER_STATUS_CHECK


unsigned char
tcp_is_netif_addr_check_success(struct tcp_pcb *pcb, struct netif *netif)
{
  int i = 0;

  if (IP_IS_V6_VAL(pcb->remote_ip)) {
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
      if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))
          && ip6_addr_netcmp(ip_2_ip6(&(pcb->remote_ip)), netif_ip6_addr(netif, i))) {
        return 1;
      }
    }
  } else {
    if (!ip4_addr_isany_val(*netif_ip4_addr(netif))
        && ip4_addr_netcmp(ip_2_ip4(&pcb->remote_ip), ip_2_ip4(&netif->ip_addr), ip_2_ip4(&netif->netmask))) {
      return 1;
    }
  }
  return 0;
}


void
tcp_update_drv_status_to_pcbs(struct tcp_pcb *pcb_list, struct netif *netif, u8_t status)
{
  struct tcp_pcb *pcb = NULL;
  for (pcb = pcb_list; pcb != NULL; pcb = pcb->next) {
    /* network mask matches? */
    if (netif_is_up(netif)) {
      if (tcp_is_netif_addr_check_success(pcb, netif)) {
        pcb->drv_status = status;
      }
    }
  }
}


void
tcpip_upd_status_to_tcp_pcbs(struct netif *netif, u8_t status)
{
  LWIP_ERROR("netif_set_driver_ready: invalid arguments", (netif != NULL), return);

  tcp_update_drv_status_to_pcbs(tcp_active_pcbs, netif, status);
  tcp_update_drv_status_to_pcbs(tcp_tw_pcbs, netif, status);

  return;
}


void
tcp_ip_flush_pcblist_on_wake_queue(struct netif *netif, struct tcp_pcb *pcb_list, u8_t status)
{
  struct tcp_pcb *pcb = NULL;
  /* iterate all PCB for that netif, and if any thing is down, change status and mark status UP */
  for (pcb = pcb_list; pcb != NULL; pcb = pcb->next) {
    if (tcp_is_netif_addr_check_success(pcb, netif)) {
      tcp_flush_pcb_on_wake_queue(pcb, status);
      /* Reset the RTO timer if its running */
      if (pcb->rtime > 0) {
        pcb->rtime = 0;
      }
    }
  }
}

void
tcp_ip_event_sendplus_on_wake_queue (struct netif *netif)
{
  struct tcp_pcb *pcb = NULL;
  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
    if (tcp_is_netif_addr_check_success(pcb, netif)) {
      if ((pcb->sndplus != NULL) && (pcb->callback_arg != NULL)) {
        pcb->sndplus(pcb->callback_arg, pcb);
      }
    }
  }
}

void
tcpip_flush_on_wake_queue(struct netif *netif, u8_t status)
{
  tcp_ip_flush_pcblist_on_wake_queue(netif, tcp_active_pcbs, status);
  tcp_ip_flush_pcblist_on_wake_queue(netif, tcp_tw_pcbs, status);
}
#endif


#if LWIP_API_RICH
const char *
get_lwip_version(void)
{
  return NSTACK_VERSION_STR;
}
#endif /* LWIP_API_RICH */

/*
 * This function is for making sure that accept() should not block indefinetely
 * when removing IPv6 address used for accept() by using API[netifapi_netif_rmv_ip6_address].
 */
void
tcp_unlock_accept(ip6_addr_t *ipaddr)
{
  struct tcp_pcb_listen *pcb = NULL;
  for (pcb = tcp_listen_pcbs.listen_pcbs; pcb != NULL; pcb = pcb->next) {
    if (ip6_addr_cmp(ipaddr, ip_2_ip6(&(pcb->local_ip)))) {
      struct netconn *conn = (struct netconn *)pcb->callback_arg;
      if (conn == NULL) {
        continue;
      }
      /* use trypost to prevent deadlock */
      if (sys_mbox_valid(&(conn->acceptmbox)) && (sys_mbox_trypost(&conn->acceptmbox, &netconn_aborted) == ERR_OK)) {
        /* Register event with callback */
        API_EVENT(conn, NETCONN_EVT_RCVPLUS, 0);
      }
    }
  }
}

#if LWIP_TCP_TLP_SUPPORT

/*
 * draft-dukkipati-tcpm-tcp-loss-probe-01
 * Conditions for scheduling PTO:
 * (a) Connection is in Open state.
 * Open state:  the sender has so far received in-sequence ACKs with no SACK blocks,
 * and no other indications (such as retransmission timeout) that a loss may have occurred.
 * (b) Connection is either cwnd limited or application limited.
 * (c) Number of consecutive PTOs <= 2.
 * (d) Connection is SACK enabled.
 *
 * When the above conditions are met, then we need to decide at what duration PTO needs to be
 * fired, below is explained:
 * a) FlightSize > 1: schedule PTO in max(2*SRTT, 10ms).
 * b) FlightSize == 1: schedule PTO in max(2*SRTT, 1.5*SRTT+WCDelAckT).
 * c) If RTO is earlier, schedule PTO in its place: PTO = min(RTO, PTO).
 *
 * Deviation in lwip:
 * 1, PTO is fired in tcp_fasttmr, so The real PTO trigger timer is not very precise. This issue could be
 * fixed if we install one PTO timer for every TCP PCB.
 * 2, lwip don't comply with condition (a). PTO would be scheduled in both Open and Disorder state,
 * not just Open state. Let's consider this scenario: 4 segments was sent out, but only segment 2 was
 * sacked. In this scenario, TCP Early Retransmit cannot be trigged as the total unacked count was 4
 * and greater than DupACK threshold. But if we stop PTO as it is in Disorder state, TLP is not workable
 * to trigger FR. so The only recovery method is RTO retransmit. Recent-ACK can be used to do fast
 * recovery in this scenario, but lwip don't support RACK now.
 */
void
tcp_tlp_schedule_probe(struct tcp_pcb *pcb, u32_t wnd)
{
  u32_t pto_duration;
  u32_t time_now;
  u32_t srtt;

  if ((pcb->unacked != NULL) && (pcb->flags & TF_SACK) && (pcb->state == ESTABLISHED)) { /* (d) */
    if ((!(pcb->flags & TF_IN_SACK_RTO)) && (!(pcb->flags & TF_IN_SACK_FRLR))) { /* (a) */
      if ((pcb->unsent == NULL) ||
          (((ntohl(pcb->unsent->tcphdr->seqno) - pcb->lastack) + pcb->unsent->len) > wnd)) { /* (b) */
        if (pcb->tlp_pto_cnt < TCP_TLP_MAX_PROBE_CNT) { /* (c) */
          if (pcb->sa != -1) {
            srtt = (((u32_t)pcb->sa) >> 3); /* time duration in ms */
          } else {
            srtt = 100; /* use 100ms as the default SRTT if no valid RTT sample */
          }

          time_now = sys_now();
          /* FlightSize > 1: schedule PTO in max(2*SRTT, 10ms) */
          if (pcb->unacked->next != NULL) {
            pto_duration = LWIP_MAX(srtt << 1, 20); /* it should be 20ms here */
            LWIP_DEBUGF(TCP_TLP_DEBUG,
                        ("tcp_tlp_schedule_probe: FlightSize > 1: pto duration %"S32_F", sys_now %"U32_F"\n",
                         pto_duration, time_now));
          } else {
            /* FlightSize == 1: schedule PTO in max(2*SRTT, 1.5*SRTT+WCDelAckT) */
            pto_duration = LWIP_MAX((srtt << 1), ((((srtt << 1) + srtt) >> 1) + LWIP_TCP_TLP_WCDELACKT));
            LWIP_DEBUGF(TCP_TLP_DEBUG,
                        ("tcp_tlp_schedule_probe: FlightSize == 1: pto duration %"S32_F", sys_now %"U32_F"\n",
                         pto_duration, time_now));
          }

          pto_duration = LWIP_MIN(pto_duration, (u32_t)(pcb->rto * TCP_SLOW_INTERVAL));
          pcb->tlp_time_stamp = time_now + pto_duration;
          if (pcb->tlp_time_stamp == 0) { /* tlp_time_stamp 0 indicates PTO not scheduled */
            pcb->tlp_time_stamp = 1;
          }
          pcb->rtime = -1; /* stop RTO as PTO would be triggerd before RTO */
        }
      }
    }
  }

  return;
}
#endif /* LWIP_TCP_TLP_SUPPORT */

#if LWIP_TCP_PCB_NUM_EXT_ARGS
/**
 * @defgroup tcp_raw_extargs ext arguments
 * @ingroup tcp_raw
 * Additional data storage per tcp pcb\n
 * @see @ref tcp_raw
 *
 * When LWIP_TCP_PCB_NUM_EXT_ARGS is > 0, every tcp pcb (including listen pcb)
 * includes a number of additional argument entries in an array.
 *
 * To support memory management, in addition to a 'void *', callbacks can be
 * provided to manage transition from listening pcbs to connections and to
 * deallocate memory when a pcb is deallocated (see struct @ref tcp_ext_arg_callbacks).
 *
 * After allocating this index, use @ref tcp_ext_arg_set and @ref tcp_ext_arg_get
 * to store and load arguments from this index for a given pcb.
 */

static u8_t tcp_ext_arg_id;

/**
 * @ingroup tcp_raw_extargs
 * Allocate an index to store data in ext_args member of struct tcp_pcb.
 * Returned value is an index in mentioned array.
 * The index is *global* over all pcbs!
 *
 * When @ref LWIP_TCP_PCB_NUM_EXT_ARGS is > 0, every tcp pcb (including listen pcb)
 * includes a number of additional argument entries in an array.
 *
 * To support memory management, in addition to a 'void *', callbacks can be
 * provided to manage transition from listening pcbs to connections and to
 * deallocate memory when a pcb is deallocated (see struct @ref tcp_ext_arg_callbacks).
 *
 * After allocating this index, use @ref tcp_ext_arg_set and @ref tcp_ext_arg_get
 * to store and load arguments from this index for a given pcb.
 *
 * @return a unique index into struct tcp_pcb.ext_args
 */
u8_t
tcp_ext_arg_alloc_id(void)
{
  u8_t result = tcp_ext_arg_id;
  tcp_ext_arg_id++;

  LWIP_ASSERT_CORE_LOCKED();

#if LWIP_TCP_PCB_NUM_EXT_ARGS >= 255
#error LWIP_TCP_PCB_NUM_EXT_ARGS
#endif
  LWIP_ASSERT("Increase LWIP_TCP_PCB_NUM_EXT_ARGS in lwipopts.h", result < LWIP_TCP_PCB_NUM_EXT_ARGS);
  return result;
}

/**
 * @ingroup tcp_raw_extargs
 * Set callbacks for a given index of ext_args on the specified pcb.
 *
 * @param pcb tcp_pcb for which to set the callback
 * @param id ext_args index to set (allocated via @ref tcp_ext_arg_alloc_id)
 * @param callbacks callback table (const since it is referenced, not copied!)
 */
void
tcp_ext_arg_set_callbacks(struct tcp_pcb *pcb, uint8_t id, const struct tcp_ext_arg_callbacks * const callbacks)
{
  LWIP_ASSERT("pcb != NULL", pcb != NULL);
  LWIP_ASSERT("id < LWIP_TCP_PCB_NUM_EXT_ARGS", id < LWIP_TCP_PCB_NUM_EXT_ARGS);
  LWIP_ASSERT("callbacks != NULL", callbacks != NULL);

  LWIP_ASSERT_CORE_LOCKED();

  pcb->ext_args[id].callbacks = callbacks;
}

/**
 * @ingroup tcp_raw_extargs
 * Set data for a given index of ext_args on the specified pcb.
 *
 * @param pcb tcp_pcb for which to set the data
 * @param id ext_args index to set (allocated via @ref tcp_ext_arg_alloc_id)
 * @param arg data pointer to set
 */
void tcp_ext_arg_set(struct tcp_pcb *pcb, uint8_t id, void *arg)
{
  LWIP_ASSERT("pcb != NULL", pcb != NULL);
  LWIP_ASSERT("id < LWIP_TCP_PCB_NUM_EXT_ARGS", id < LWIP_TCP_PCB_NUM_EXT_ARGS);

  LWIP_ASSERT_CORE_LOCKED();

  pcb->ext_args[id].data = arg;
}

/**
 * @ingroup tcp_raw_extargs
 * Set data for a given index of ext_args on the specified pcb.
 *
 * @param pcb tcp_pcb for which to set the data
 * @param id ext_args index to set (allocated via @ref tcp_ext_arg_alloc_id)
 * @return data pointer at the given index
 */
void *tcp_ext_arg_get(const struct tcp_pcb *pcb, uint8_t id)
{
  LWIP_ASSERT("pcb != NULL", pcb != NULL);
  LWIP_ASSERT("id < LWIP_TCP_PCB_NUM_EXT_ARGS", id < LWIP_TCP_PCB_NUM_EXT_ARGS);

  LWIP_ASSERT_CORE_LOCKED();

  return pcb->ext_args[id].data;
}

/** This function calls the "destroy" callback for all ext_args once a pcb is
 * freed.
 */
static void
tcp_ext_arg_invoke_callbacks_destroyed(struct tcp_pcb_ext_args *ext_args)
{
  int i;
  LWIP_ASSERT("ext_args != NULL", ext_args != NULL);

  for (i = 0; i < LWIP_TCP_PCB_NUM_EXT_ARGS; i++) {
    if (ext_args[i].callbacks != NULL) {
      if (ext_args[i].callbacks->destroy != NULL) {
        ext_args[i].callbacks->destroy((u8_t)i, ext_args[i].data);
      }
    }
  }
}

/** This function calls the "passive_open" callback for all ext_args if a connection
 * is in the process of being accepted. This is called just after the SYN is
 * received and before a SYN/ACK is sent, to allow to modify the very first
 * segment sent even on passive open. Naturally, the "accepted" callback of the
 * pcb has not been called yet!
 */
err_t
tcp_ext_arg_invoke_callbacks_passive_open(struct tcp_pcb_listen *lpcb, struct tcp_pcb *cpcb)
{
  int i;
  LWIP_ASSERT("lpcb != NULL", lpcb != NULL);
  LWIP_ASSERT("cpcb != NULL", cpcb != NULL);

  for (i = 0; i < LWIP_TCP_PCB_NUM_EXT_ARGS; i++) {
    if (lpcb->ext_args[i].callbacks != NULL) {
      if (lpcb->ext_args[i].callbacks->passive_open != NULL) {
        err_t err = lpcb->ext_args[i].callbacks->passive_open((u8_t)i, lpcb, cpcb);
        if (err != ERR_OK) {
          return err;
        }
      }
    }
  }
  return ERR_OK;
}
#endif /* LWIP_TCP_PCB_NUM_EXT_ARGS */

#endif /* LWIP_TCP */
