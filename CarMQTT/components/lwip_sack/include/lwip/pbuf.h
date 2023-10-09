/**
 * @file
 * pbuf API
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

#ifndef LWIP_HDR_PBUF_H
#define LWIP_HDR_PBUF_H

#include "arch/atomic.h"
#include "lwip/opt.h"
#include "lwip/err.h"

#if defined (__cplusplus) && __cplusplus
extern "C" {
#endif

#define SIZEOF_STRUCT_PBUF        LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf))

/**
 * @defgroup Buffer_Interfaces Buffer Interfaces
 * @ingroup System_interfaces
 */
/**
 * LWIP_SUPPORT_CUSTOM_PBUF==1: Custom pbufs behave similar to its pbuf type,
 * but it is are allocated by external code (initialized by calling
 * pbuf_alloced_custom()), and when pbuf_free gives up the last reference, the custom pbufs
 * are freed by calling pbuf_custom->custom_free_function().
 * Currently, the pbuf_custom code is only needed for one specific configuration
 * of IP_FRAG, unless required by the external driver/application code. */
#ifndef LWIP_SUPPORT_CUSTOM_PBUF
#define LWIP_SUPPORT_CUSTOM_PBUF ((IP_FRAG && !LWIP_NETIF_TX_SINGLE_PBUF) || (LWIP_IPV6 && LWIP_IPV6_FRAG))
#endif

/* @todo: We need a mechanism to prevent wasting memory in every pbuf
   (TCP vs. UDP, IPv4 vs. IPv6: UDP/IPv4 packets may waste up to 28 bytes) */

#define PBUF_TRANSPORT_HLEN 20
#if LWIP_IPV6
#define PBUF_IP_HLEN        40
#else
#define PBUF_IP_HLEN        20
#endif

/* for zero copy */
#define PBUF_ZERO_COPY_RESERVE 36

/**
 * @ingroup pbuf
 * Enumeration of pbuf layers
 */
typedef enum {
  /** Includes spare room for transport layer headers such as UDP header.
   * Use this if you intend to pass the pbuf to functions like udp_send().
   */
  PBUF_TRANSPORT,
  /** Includes spare room for IP header.
   * Use this if you intend to pass the pbuf to functions like raw_send().
   */
  PBUF_IP,
  /** Includes spare room for link layer header (ethernet header).
   * Use this if you intend to pass the pbuf to functions like ethernet_output().
   * @see PBUF_LINK_HLEN
   */
  PBUF_LINK,
  /** Includes spare room for additional encapsulation header before ethernet
   * headers such as 802.11.
   * Use this if you intend to pass the pbuf to functions like netif->linkoutput().
   * @see PBUF_LINK_ENCAPSULATION_HLEN
   */
  PBUF_RAW_TX,
  /** Use this for input packets in a netif driver when calling netif->input().
   * The most common case is ethernet-layer netif driver. */
  PBUF_RAW
} pbuf_layer;

/**
 * @ingroup pbuf
 * Enumeration of pbuf types
 */
typedef enum {
  /** Indicates the pbuf data stored in RAM, used for all outgoing packets such as TX. The struct pbuf and its payload
      are allocated in one piece of contiguous memory, so that the first payload byte
      can be calculated from struct pbuf.
      Currently, pbuf_alloc() allocates PBUF_RAM pbufs as unchained pbufs. */
  PBUF_RAM,
  /** Indicates the pbuf data stored in ROM, that is, struct pbuf and its payload are located in
      totally different memory areas. Since it points to ROM, the payload does not
      have to be copied when queued for transmission. */
  PBUF_ROM,
  /** Indicates that the pbuf comes from the pbuf pool. This is similar to PBUF_ROM, but the payload might change
      so it has to be duplicated when queued before transmitting, depending on
      who has a 'ref' to it. */
  PBUF_REF,
  /** Indicates that the pbuf payload refers to RAM. This type comes from a pool and must be used
      for RX. The payload can be chained (scatter-gather RX) but like PBUF_RAM, struct
      pbuf and its payload are allocated in one piece of contiguous memory, so that
      the first payload byte can be calculated from struct pbuf.
      Do not use this for TX, if the pool becomes empty due to some reason such as TCP queuing,
      you will be unable to receive TCP acks. */
  PBUF_POOL
} pbuf_type;
#define LWIP_MEM_DMA_ALIGN_SIZE(size) (((size) + (MEM_MALLOC_DMA_ALIGN) - 1) & ~(u32_t)((MEM_MALLOC_DMA_ALIGN) - 1))
#if (MEM_MALLOC_DMA_ALIGN != 1)
struct pbuf_dma_info {
  void         *dma;
  /** Pointer to the MAC header in the buffer, used by the driver. */
  void *mac_header;

  atomic_t      dma_ref;
  u16_t         dma_len;
  /** Indicates the link layer data totlen, used by the driver. */
  u16_t link_len;
};
#endif

/** Indicates that the data of the packet must be immediately passed to the application. */
#define PBUF_FLAG_PUSH      0x01U
/** Indicates that this is a custom pbuf. pbuf_free calls pbuf_custom->custom_free_function()
    when the last reference is released (custom PBUF_RAM cannot be trimmed). */
#define PBUF_FLAG_IS_CUSTOM 0x02U
/** Indicates that this pbuf is UDP multicast to be looped back. */
#define PBUF_FLAG_MCASTLOOP 0x04U
/** Indicates that this pbuf was received as link-level broadcast. */
#define PBUF_FLAG_LLBCAST   0x08U
/** Indicates that this pbuf was received as link-level multicast. */
#define PBUF_FLAG_LLMCAST   0x10U
/** Indicates that this pbuf includes a TCP FIN flag. */
#define PBUF_FLAG_TCP_FIN   0x20U
/** Flag to denote presence of for HBH-RPI. */
#define PBUF_FLAG_RPI 0x40U

/** Flag to denote presence of space for HBH. */
#define PBUF_FLAG_HBH_SPACE 0x80U
/* NETIF DRIVER STATUS CHANGE BEGIN */
#if DRIVER_STATUS_CHECK
/** Indicates this pbuf is from DHCP application. */
#define PBUF_FLAG_DHCP_BUF  0x80U
#endif

#define PBUF_FLAG_HOST      0x100U
#define PBUF_FLAG_OUTGOING  0x200U

#if LWIP_SO_DONTROUTE
/** indicates the destination of this pbuf located on directly attached link */
#define PBUF_FLAG_IS_LINK_ONLY   0x400U
#endif /* LWIP_SO_DONTROUTE */

/** indicates this pbuf need to be sent nonsecure manner */
#define PBUF_FLAG_WITH_ENCRYPTION   0x400U

#if LWIP_IPV6 && (LWIP_RPL || LWIP_RIPPLE)
#define PBUF_FLAG_6LO_PKT  0x1000U
#endif /* LWIP_IPV6 && (LWIP_RPL || LWIP_RIPPLE) */

#define PBUF_FLAG_CTRL_PKT  0x2000U

/** Indicates the main packet buffer struct. */
struct pbuf {
  /** Indicates the next pbuf in singly-linked pbuf chain. */
  struct pbuf *next;

  /** Indicates a pointer to the actual data in the buffer. */
  void *payload;

  /**
   * Indicates the total length of the buffer and all next buffers in chain
   * belonging to the same packet.
   *
   * For non-queue packet chains this is the invariant:
   * p->tot_len == p->len + (p->next? p->next->tot_len: 0)
   */
  u16_t tot_len;

  /** Indicates the length of the buffer. */
  u16_t len;

  struct pbuf *list;

  /**
    * Indicates the reference count. This is always equal to the number of pointers
    * that refer to this pbuf. The pointers can be pointers from an application,
    * the stack itself, or pbuf->next pointers from a chain.
    */
  atomic_t ref;
#if (MEM_MALLOC_DMA_ALIGN != 1)
  struct pbuf_dma_info *dma_info;
#endif
#if MEM_PBUF_RAM_SIZE_LIMIT
  u16_t malloc_len;
#endif

  /** Indicates miscellaneous flags. */
  u16_t flags;
  /** Indicates pbuf_type as u8_t instead of enum to save space. */
  u8_t type; /* pbuf_type */

  /** For incoming packets, this contains the input netif's index */
  u8_t if_idx;

#if LWIP_RIPPLE
  u8_t pkt_up;
  u8_t pkt_flags;
#endif
#if LWIP_RIPPLE && defined(LWIP_NA_PROXY) && LWIP_NA_PROXY
  u8_t na_proxy;
#endif
#if LWIP_IPV6
#if LWIP_RPL || LWIP_RIPPLE
#if LWIP_USE_L2_METRICS
  u8_t mac_address[NETIF_MAX_HWADDR_LEN];
  s8_t pkt_rssi;
  u8_t pkt_lqi;
#endif
#endif
#endif

#if LWIP_SO_PRIORITY
  prio_t priority;
#endif /* LWIP_SO_PRIORITY */

#if MEM_PBUF_RAM_SIZE_LIMIT
#if LWIP_SO_PRIORITY
#if LWIP_PLC
  u8_t pad[2];
#else /* LWIP_PLC */
  u8_t pad[1];
#endif /* LWIP_PLC */
#else /* LWIP_SO_PRIORITY */
  u8_t pad[2];
#endif /* LWIP_SO_PRIORITY */
#else /* MEM_PBUF_RAM_SIZE_LIMIT */
#if LWIP_SO_PRIORITY
#if (LWIP_PLC == 0)
  u8_t pad[3];
#endif /* LWIP_PLC == 0 */
#endif /* LWIP_SO_PRIORITY */
#endif /* MEM_PBUF_RAM_SIZE_LIMIT */
};

/** Helper struct for const-correctness only.
 * The purpose of this struct is to provide a const payload pointer
 * for PBUF_ROM type.
 */
struct pbuf_rom {
  /** Indicates the next pbuf in singly linked pbuf chain. */
  struct pbuf *next;

  /** Indicates a pointer to the actual data in the buffer. */
  const void *payload;
};

#if LWIP_SUPPORT_CUSTOM_PBUF
/** Indicates the prototype for a function to free a custom pbuf. */
typedef void (*pbuf_free_custom_fn)(struct pbuf *p);

/** Indicates a custom pbuf. This is similar to a pbuf, but following a function pointer to free it. */
struct pbuf_custom {
  /** Indicates the actual pbuf. */
  struct pbuf pbuf;
  /** This function is called when pbuf_free deallocates this pbuf(_custom) */
  pbuf_free_custom_fn custom_free_function;
};
#endif /* LWIP_SUPPORT_CUSTOM_PBUF */

/** Define this to 0 to prevent freeing ooseq pbufs when the PBUF_POOL is empty */
#ifndef PBUF_POOL_FREE_OOSEQ
#define PBUF_POOL_FREE_OOSEQ 1
#endif /* PBUF_POOL_FREE_OOSEQ */
#if LWIP_TCP && TCP_QUEUE_OOSEQ && NO_SYS && PBUF_POOL_FREE_OOSEQ
extern volatile u8_t pbuf_free_ooseq_pending;
void pbuf_free_ooseq(void);
/** When not using sys_check_timeouts(), call PBUF_CHECK_FREE_OOSEQ()
    at regular intervals from main level to check if ooseq pbufs need to be
    freed! */
#define PBUF_CHECK_FREE_OOSEQ() do { if (pbuf_free_ooseq_pending) { \
  /* pbuf_alloc() reported PBUF_POOL to be empty -> try to free some \
     ooseq queued pbufs now */ \
  pbuf_free_ooseq(); } }while (0)
#else /* LWIP_TCP && TCP_QUEUE_OOSEQ && NO_SYS && PBUF_POOL_FREE_OOSEQ */
/* Otherwise declare an empty PBUF_CHECK_FREE_OOSEQ */
#define PBUF_CHECK_FREE_OOSEQ()
#endif /* LWIP_TCP && TCP_QUEUE_OOSEQ && NO_SYS && PBUF_POOL_FREE_OOSEQ */

/* Initializes the pbuf module. This call is empty for now, but may not be in future. */
#define pbuf_init()

#define pbuf_set_encypted(pbuffer) \
  pbuffer->flags |= PBUF_FLAG_WITH_ENCRYPTION

#if LWIP_IPV6
#if LWIP_RPL || LWIP_RIPPLE
#if LWIP_USE_L2_METRICS
#define PBUF_SET_RSSI(pbuffer, rssi) \
  pbuffer->pkt_rssi = rssi

#define PBUF_GET_RSSI(pbuffer) \
  pbuffer->pkt_rssi

#define PBUF_SET_LQI(pbuffer, lqi) \
  pbuffer->pkt_lqi = lqi

#define PBUF_GET_LQI(pbuffer) \
  pbuffer->pkt_lqi
#endif /* LWIP_USE_L2_METRICS */
#endif /* LWIP_RPL || LWIP_RIPPLE */
#endif /* LWIP_IPV6 */

u8_t pbuf_ram_in_deflation(void);
struct pbuf *pbuf_alloc(pbuf_layer l, u16_t length, pbuf_type type);
struct pbuf *pbuf_alloc_for_rx(pbuf_layer layer, u16_t length);

#if LWIP_SUPPORT_CUSTOM_PBUF
struct pbuf *pbuf_alloced_custom(pbuf_layer l, u16_t length, pbuf_type type,
                                 struct pbuf_custom *p, void *payload_mem,
                                 u16_t payload_mem_len);
#endif /* LWIP_SUPPORT_CUSTOM_PBUF */
void pbuf_realloc(struct pbuf *p, u16_t size);
u8_t pbuf_header(struct pbuf *p, s16_t header_size);
u8_t pbuf_header_force(struct pbuf *p, s16_t header_size);
void pbuf_ref(struct pbuf *p);
u8_t pbuf_free(struct pbuf *p);
u16_t pbuf_clen(const struct pbuf *p);
void pbuf_cat(struct pbuf *head, struct pbuf *tail);
void pbuf_chain(struct pbuf *head, struct pbuf *tail);
#if LWIP_API_RICH
/**
 * Dechains the first pbuf from its succeeding pbufs in the chain. This function makes p->tot_len field equal to p->len.
 * @param[in] p Indicates the pbuf to dechain.
 * @return Returns the remainder of the pbuf chain, or NULL if it was de-allocated.
 * @note
 * - Do not call this function on a packet queue.
 * - The PBUF_API macro must be enabled by the application in case the pbuf APIs are
 *   directly used by the application code. This macro enables NULL validations and can have a performance impact.
 */
struct pbuf *pbuf_dechain(struct pbuf *p);
#endif /* LWIP_API_RICH */
err_t pbuf_copy(struct pbuf *p_to, const struct pbuf *p_from);
u16_t pbuf_copy_partial(const struct pbuf *p, void *dataptr, u16_t len, u16_t offset);
#if LWIP_IPV6_DHCP6
void *pbuf_get_contiguous(const struct pbuf *p, void *buffer, size_t bufsize, u16_t len, u16_t offset);
#endif /* LWIP_IPV6_DHCP6 */
err_t pbuf_take(struct pbuf *buf, const void *dataptr, u16_t len);
/**
 * @ingroup pbuf
 * @brief This function is the same as pbuf_take(), but puts data at an offset.
 *
 * @param buf Indicates the pbuf to fill with data.
 * @param dataptr Indicates the application supplied data buffer.
 * @param len Indicates the length of the application supplied data buffer.
 * @param offset Indicates the offset in pbuf  to copy dataptr to.
 *
 * @return ERR_OK if successful, ERR_MEM if the pbuf is not big enough.
 * @note The PBUF_API macro must be enabled by the application in case the pbuf APIs are directly used by
 *       the application code. This macro enables NULL validations and can have a performance impact.
 */
err_t pbuf_take_at(struct pbuf *buf, const void *dataptr, u16_t len, u16_t offset);
struct pbuf *pbuf_skip(struct pbuf *in, u16_t in_offset, u16_t *out_offset);
#if LWIP_API_RICH
/**
 * @ingroup pbuf
 * @brief
 * Creates a single pbuf from a queue of pbufs. This function either frees or returns the  pbuf 'p'.
 * Therefore, the caller must check the result.
 *
 * @param p the Indicates the source pbuf.
 * @param layer Indicates the pbuf_layer of the new pbuf.
 *
 * @return Returns a new, single pbuf (p->next is NULL) or returns the old pbuf if allocation fails.
 * @note The PBUF_API macro must be enabled by the application in case the pbuf APIs are directly used
 *       by the application code. This macro enables NULL validations and can have a performance impact.
 */
struct pbuf *pbuf_coalesce(struct pbuf *p, pbuf_layer layer);
#endif /* LWIP_API_RICH */
struct pbuf *pbuf_clone(pbuf_layer l, pbuf_type type, struct pbuf *p);
#if (MEM_MALLOC_DMA_ALIGN != 1)
#if LWIP_API_RICH
struct pbuf *pbuf_dma_alloc(u16_t len);
err_t pbuf_dma_ref(struct pbuf_dma_info *dma_info);
#endif /* LWIP_API_RICH */
void  pbuf_dma_free(struct pbuf_dma_info *dma_info);
#endif

#if LWIP_CHECKSUM_ON_COPY
#if LWIP_API_RICH
err_t pbuf_fill_chksum(struct pbuf *p, u16_t start_offset, const void *dataptr,
                       u16_t len, u16_t *chksum);
#endif /* LWIP_API_RICH */
#endif /* LWIP_CHECKSUM_ON_COPY */
#if LWIP_TCP && TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
/**
 * @ingroup pbuf
 * @brief
 * This method modifies a pbuf chain, so that its total length is
 * smaller than 64K. The remainder of the original pbuf chain is stored
 * in *rest.
 * This function does not create new pbufs. Instead, it splits an existing chain
 * in two parts. The total length of the modified packet queue will be
 * smaller than 64K. This function does not support packet queues.
 *
 * @param[in] p Indicates the pbuf queue to be split.
 * @param[in] rest Indicates the pointer to store the remainder (after the first 64K) of the original pbuf chain.
 * @note
 * The PBUF_API macro must be enabled by the application in case the pbuf APIs
 * are directly used by the application code. This macro enables NULL validations and can have a performance impact.
 */
void pbuf_split_64k(struct pbuf *p, struct pbuf **rest);
#endif /* LWIP_TCP && TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
/**
 * @ingroup pbuf
 * @brief
 * Gets one byte from the specified position in a pbuf.
 *
 *
 * @param p Indicates the pbuf to parse.
 * @param offset Indicates the offset into p of the byte to return.
 * The offset must be set to a value less than p->tot_len.
 * @return Returns a byte at an offset into p, or zero if offset >= p->tot_len.
 */
u8_t pbuf_get_at(const struct pbuf *p, u16_t offset);
/**
 * @ingroup pbuf
 * @brief Gets one byte from the specified position in a pbuf.
 *
 * @param p Indicates the pbuf to parse.
 * @param offset Indicates the offset into p of the byte to return.
 * @return Returns a byte at an offset into p [0..0xFF] or negative if offset >= p->tot_len.
 */
int pbuf_try_get_at(const struct pbuf *p, u16_t offset);
void pbuf_put_at(struct pbuf *p, u16_t offset, u8_t data);
#if LWIP_API_RICH
/**
 * @ingroup pbuf
 * @brief
 * Compares pbuf contents at a specified offset with memory s2, both of length n.
 *
 * @param p Indicates the pbuf to compare.
 * @param offset Indicates the offset into p at which to start comparing.
 * @param s2 Indicates the buffer to compare.
 * @param n Indicates the length of buffer to compare.
 * @return Returns zero if equal, nonzero otherwise (0xffff if p is too short, diff offset+1 otherwise)
 * @note The PBUF_API macro must be enabled by the application in case the pbuf APIs are directly
 *       used by the application code. This macro enables NULL validations and can have a performance impact.
 */
u16_t pbuf_memcmp(const struct pbuf *p, u16_t offset, const void *s2, u16_t n);
/**
 * @ingroup pbuf
 * Finds the occurrence of mem (with length mem_len) in pbuf p, starting at offset
 * start_offset.
 *
 * @param p Indicates the pbuf to search. The maximum length is 0xFFFE since 0xFFFF is used as
 *        return value 'not found'.
 * @param mem Indicates the buffer contents to search for.
 * @param mem_len Indicates the length of mem.
 * @param start_offset Indicates the offset into p at which to start searching.
 * @return Returns the index where mem was found in p, or returns 0xFFFF if mem was not found in p.
 * @note The PBUF_API macro must be enabled by the application in case the pbuf APIs are directly
 *       used by the application code. This macro enables NULL validations and can have a performance impact.
 */
u16_t pbuf_memfind(const struct pbuf *p, const void *mem, u16_t mem_len, u16_t start_offset);

/**
 * Finds the occurrence of substr with length substr_len in pbuf p, starting at offset
 * start_offset.
 *
 * @param p Indicates the pbuf to search. The maximum length is 0xFFFE since 0xFFFF is used as
 *        return value 'not found'.
 * @param substr Indicates the string to search for in p, maximum length is 0xFFFE
 * @return Returns 0xFFFF if substr was not found in p, or returns the index where it was found.
 * @note The PBUF_API macro must be enabled by the application in case the pbuf APIs are directly
 *       used by the application code. This macro enables NULL validations and can have a performance impact.
 */
u16_t pbuf_strstr(const struct pbuf *p, const char *substr);
#endif /* LWIP_API_RICH */

#if MEM_PBUF_RAM_SIZE_LIMIT
#if LWIP_API_RICH
/*
 * Func Name: pbuf_ram_size_set
 */
/**
 * @ingroup Buffer_Interfaces
 * @pbrief
 *  This API is used to set the maximum buffer size of PBUF_RAM allocation.
 *
 * @param[in]    ram_max_size   Indicates the maximum RAM buffer size allowed.
 * [The ram_max_size must be greater than PBUF_RAM_SIZE_MIN and less than 0x7FFFFFFF.]
 *
 * @returns
 *  Zero : On failure \n
 *  Non Zero value: Previous maximum RAM buffer size.
 * @note
 *  The PBUF_API macro must be enabled by the application in case the pbuf APIs are
 *  directly used by the application code. This macro enables NULL validations and can have a performance impact.
 */
u32_t pbuf_ram_size_set(u32_t ram_max_size);
#endif /* LWIP_API_RICH */
#ifdef LWIP_DEBUG
/*
 * Func Name: pbuf_ram_display
 */
/**
 * @ingroup Buffer_Interfaces
 * @brief
 *  This API is used to display the pbuf RAM details.
 *
 * @par Parameter
 * \n
 * void
 * @return
 *  void
 * @note
 * This API is enabled only if the LWIP_DEBUG macro is enabled.  \n
 * The PBUF_API macro must be enabled by the application in case the pbuf APIs are directly
  used by the application code. This macro enables NULL validations and can have a performance impact.
 */
void pbuf_ram_display(void);
#endif
#endif
#if defined (__cplusplus) && __cplusplus
}
#endif
#endif /* LWIP_HDR_PBUF_H */
