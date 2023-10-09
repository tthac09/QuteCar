/* coap_io_lwip.h -- Network I/O functions for libcoap on lwIP
 *
 * Copyright (C) 2012,2014 Olaf Bergmann <bergmann@tzi.org>
 *               2014 chrysn <chrysn@fsfe.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use.
 */

#include "coap_internal.h"
#include <lwip/udp.h>
#include <lwip/netifapi.h>

static coap_pdu_t recv_pdu;

#if NO_SYS
pthread_mutex_t lwprot_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t lwprot_thread = (pthread_t)0xDEAD;
int lwprot_count = 0;
#endif

void coap_server_sessions_check_idle(coap_context_t *ctx)
{
  coap_endpoint_t *ep = NULL;
  coap_session_t *s = NULL;
  coap_session_t *tmp = NULL;
  coap_tick_t session_timeout;
  coap_tick_t now;

  if (ctx == NULL) {
    coap_log(LOG_EMERG, "coap_server_sessions_check_idle: nul ctx\n");
    return;
  }
  coap_ticks(&now);
  if (ctx->session_timeout > 0) {
    session_timeout = ctx->session_timeout * COAP_TICKS_PER_SECOND;
  } else {
    session_timeout = COAP_DEFAULT_SESSION_TIMEOUT * COAP_TICKS_PER_SECOND;
  }
  LL_FOREACH(ctx->endpoint, ep) {
    SESSIONS_ITER_SAFE(ep->sessions, s, tmp) {
      if ((s->type == COAP_SESSION_TYPE_SERVER) && (s->ref == 0) && (s->delayqueue == NULL) &&
          ((s->last_rx_tx + session_timeout <= now) || (s->state == COAP_SESSION_STATE_NONE))) {
        coap_log(LOG_INFO, "coap_server_sessions_check_idle: free %p\n", s);
        coap_session_free(s);
      }
    }
  }
}

static err_t coap_server_sessions_check_idle_lwip_fn(struct netif *netif, void *arg)
{
  (void)netif;
  coap_context_t *context = (coap_context_t *)arg;
  coap_server_sessions_check_idle(context);

  return ERR_OK;
}

void coap_server_sessions_check_idle_lwip(coap_context_t *context)
{
  if (context == NULL) {
#ifndef NDEBUG
    coap_log(LOG_EMERG, "coap_server_sessions_check_idle_lwip: invalid param\n");
#endif
    return;
  }
  (void)netifapi_call_argcb(coap_server_sessions_check_idle_lwip_fn, (void *)context);
}

void coap_packet_get_memmapped(coap_packet_t *packet, unsigned char **address, size_t *length)
{
  LWIP_ASSERT("Can only deal with contiguous PBUFs to read the initial details", packet->pbuf->tot_len == packet->pbuf->len);
  *address = packet->pbuf->payload;
  *length = packet->pbuf->tot_len;
}

void coap_free_packet(coap_packet_t *packet)
{
  if (packet->pbuf)
    pbuf_free(packet->pbuf);
  coap_free_type(COAP_PACKET, packet);
}

struct pbuf *coap_packet_extract_pbuf(coap_packet_t *packet)
{
  struct pbuf *ret = packet->pbuf;
  packet->pbuf = NULL;
  return ret;
}

static coap_session_t *
coap_get_session_lwip(coap_endpoint_t *endpoint, const coap_packet_t *packet, coap_tick_t now)
{
  struct coap_context_t *ctx = endpoint->context;
  coap_session_t *session = NULL;
  coap_session_t *tmp = NULL;

  if ((ctx != NULL) && (ctx->sessions != NULL)) {
    SESSIONS_ITER_SAFE(endpoint->sessions, session, tmp) {
      if (session->ifindex == packet->ifindex &&
        coap_address_equals(&session->addr_info.local, &packet->addr_info.local) && // luminais
        coap_address_equals(&session->addr_info.remote, &packet->addr_info.remote)) {
        session->last_rx_tx = now;
        return session;
      }
    }
  }

  return coap_endpoint_get_session(endpoint, packet, now);
}

/** Callback from lwIP when a package was received.
 *
 * The current implementation deals this to coap_dispatch immediately, but
 * other mechanisms (as storing the package in a queue and later fetching it
 * when coap_read is called) can be envisioned.
 *
 * It handles everything coap_read does on other implementations.
 */
static void coap_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  coap_endpoint_t *ep = (coap_endpoint_t*)arg;
  coap_pdu_t *pdu = NULL;
  coap_session_t *session;
  coap_tick_t now;
  coap_packet_t *packet = coap_malloc_type(COAP_PACKET, sizeof(coap_packet_t));

  /* this is fatal because due to the short life of the packet, never should there be more than one coap_packet_t required */
  LWIP_ASSERT("Insufficient coap_packet_t resources.", packet != NULL);
  (void)memset_s(packet, sizeof(coap_packet_t), 0, sizeof(coap_packet_t));
  packet->pbuf = p;
  packet->addr_info.remote.port = port;
  packet->addr_info.remote.addr = *addr;
  packet->addr_info.local.port = upcb->local_port;
  packet->addr_info.local.addr = *ip_current_dest_addr();
#ifdef WITH_LWIP_SPECIFIC_IFINDEX
  packet->ifindex = ip_current_netif()->ifindex;
#endif

  pdu = coap_pdu_from_pbuf(p, &recv_pdu);
  if (!pdu)
    goto error;

  if (!coap_pdu_parse(ep->proto, p->payload, p->len, pdu)) {
    goto error;
  }

  coap_ticks(&now);
  session = coap_get_session_lwip(ep, packet, now);
  if (!session)
    goto error;
  LWIP_ASSERT("Proto not supported for LWIP", COAP_PROTO_NOT_RELIABLE(session->proto));
  coap_dispatch(ep->context, session, pdu);

  (void)pbuf_free(p);
  packet->pbuf = NULL;
  coap_free_packet(packet);

  return;

error:
  /* FIXME: send back RST? */
  (void)pbuf_free(p);
  if (packet) {
    packet->pbuf = NULL;
    coap_free_packet(packet);
  }
  return;
}

coap_endpoint_t *
coap_new_endpoint(coap_context_t *context, const coap_address_t *addr, coap_proto_t proto) {
  coap_endpoint_t *result;
  err_t err;

  LWIP_ASSERT("Proto not supported for LWIP endpoints", proto == COAP_PROTO_UDP);

  result = coap_malloc_type(COAP_ENDPOINT, sizeof(coap_endpoint_t));
  if (result == NULL) {
    coap_log(LOG_EMERG, "coap_new_endpoint: malloc ep fail\n");
    return NULL;
  }

  (void)memset_s(result, sizeof(coap_endpoint_t), 0, sizeof(coap_endpoint_t));

  result->sock.pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  if (result->sock.pcb == NULL) {
    coap_log(LOG_EMERG, "coap_new_endpoint: udp new fail\n");
    goto error;
  }

  udp_recv(result->sock.pcb, coap_recv, (void*)result);
  err = udp_bind(result->sock.pcb, &addr->addr, addr->port);
  if (err) {
    udp_remove(result->sock.pcb);
    goto error;
  }

  result->default_mtu = COAP_DEFAULT_MTU;
  result->context = context;
  result->proto = proto;

  context->endpoint = result;

  return result;

error:
  coap_free_type(COAP_ENDPOINT, result);
  return NULL;
}

void coap_free_endpoint(coap_endpoint_t *ep)
{
  coap_session_t *session = NULL;
  coap_session_t *tmp = NULL;

  SESSIONS_ITER_SAFE(ep->sessions, session, tmp) {
    if ((session->ref == 0) && (session->type == COAP_SESSION_TYPE_SERVER)) {
      coap_session_free(session);
    }
  }
  udp_remove(ep->sock.pcb);
  coap_free_type(COAP_ENDPOINT, ep);
}

ssize_t
coap_socket_send_pdu(coap_socket_t *sock, coap_session_t *session,
  coap_pdu_t *pdu) {
  /* FIXME: we can't check this here with the existing infrastructure, but we
  * should actually check that the pdu is not held by anyone but us. the
  * respective pbuf is already exclusively owned by the pdu. */
  uint16_t tot_len;
  if (pdu->pbuf == NULL) {
    return COAP_INVALID_TID;
  }
  pbuf_realloc(pdu->pbuf, pdu->used_size + coap_pdu_parse_header_size(session->proto, pdu->pbuf->payload));
  tot_len = pdu->pbuf->tot_len;
  (void)udp_sendto(sock->pcb, pdu->pbuf, &session->addr_info.remote.addr,
                   session->addr_info.remote.port);
  if (pdu->pbuf->tot_len != tot_len) {
    /* move to udp payload again for next retransmission, no need to check return value. */
    (void)pbuf_header(pdu->pbuf, -((int16_t)(pdu->pbuf->tot_len - tot_len)));
  }
  return pdu->used_size;
}

ssize_t
coap_socket_send(coap_socket_t *sock, coap_session_t *session,
  const uint8_t *data, size_t data_len ) {
  /* Not implemented, use coap_socket_send_pdu instead */
  (void)sock;
  (void)session;
  (void)data;
  (void)data_len;
  return -1;
}

int
coap_socket_bind_udp(coap_socket_t *sock,
  const coap_address_t *listen_addr,
  coap_address_t *bound_addr) {
  (void)sock;
  (void)listen_addr;
  (void)bound_addr;
  return 0;
}

int
coap_socket_connect_udp(coap_socket_t *sock,
  const coap_address_t *local_if,
  const coap_address_t *server,
  int default_port,
  coap_address_t *local_addr,
  coap_address_t *remote_addr) {
  (void)sock;
  (void)local_if;
  (void)server;
  (void)default_port;
  (void)local_addr;
  (void)remote_addr;
  return 1;
}

#ifndef COAP_NO_TCP
int
coap_socket_connect_tcp1(coap_socket_t *sock,
  const coap_address_t *local_if,
  const coap_address_t *server,
  int default_port,
  coap_address_t *local_addr,
  coap_address_t *remote_addr) {
  (void)sock;
  (void)local_if;
  (void)server;
  (void)default_port;
  (void)local_addr;
  (void)remote_addr;
  return 0;
}

int
coap_socket_connect_tcp2(coap_socket_t *sock,
  coap_address_t *local_addr,
  coap_address_t *remote_addr) {
  (void)sock;
  (void)local_addr;
  (void)remote_addr;
  return 0;
}

int
coap_socket_bind_tcp(coap_socket_t *sock,
  const coap_address_t *listen_addr,
  coap_address_t *bound_addr) {
  (void)sock;
  (void)listen_addr;
  (void)bound_addr;
  return 0;
}

int
coap_socket_accept_tcp(coap_socket_t *server,
  coap_socket_t *new_client,
  coap_address_t *local_addr,
  coap_address_t *remote_addr) {
  (void)server;
  (void)new_client;
  (void)local_addr;
  (void)remote_addr;
  return 0;
}
#endif

ssize_t
coap_socket_write(coap_socket_t *sock, const uint8_t *data, size_t data_len) {
  (void)sock;
  (void)data;
  (void)data_len;
  return -1;
}

ssize_t
coap_socket_read(coap_socket_t *sock, uint8_t *data, size_t data_len) {
  (void)sock;
  (void)data;
  (void)data_len;
  return -1;
}

void coap_socket_close(coap_socket_t *sock) {
  (void)sock;
  return;
}

