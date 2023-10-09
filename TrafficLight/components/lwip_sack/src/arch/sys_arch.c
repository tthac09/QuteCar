/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2015. All rights reserved.
 * Description: implementation for mbox, ticks and memory adaptor
 * Author: none
 * Create: 2013
 */

#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "pthread.h"
#include "lwip/stats.h"

#include "los_config.h"
#include "arch/sys_arch.h"
#if LWIP_SOCKET_POLL
#include "linux/wait.h"
#endif /* LWIP_SOCKET_POLL */

#include "string.h"
#include "stdlib.h"
#include "los_sys.h"
#include "los_sem.h"
#include "los_mux.h"
#include "los_hwi.h"
#if LWIP_LITEOS_COMPAT && LWIP_API_MESH
#include "hi_cipher.h"
#endif

#define UNUSED(a) ((void)(a))

#define MBOX_EXPAND_MULTIPLE_SIZE 2

#if !LWIP_LITEOS_COMPAT
static pthread_mutex_t lwprot_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t lwprot_thread = (pthread_t)0xDEAD;
static int lwprot_count = 0;
#endif

err_t
sys_mbox_new_ext(struct sys_mbox **mb, int size, unsigned char is_auto_expand)
{
  struct sys_mbox *mbox = NULL;
  int ret;

  if ((size <= 0) || (mb == NULL)) {
    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_new: mbox size must bigger than 0 or mb is NULL\n"));
    return ERR_VAL;
  }

  mbox = (struct sys_mbox *)mem_malloc(sizeof(struct sys_mbox));
  if (mbox == NULL) {
    goto err_handler;
  }

  (void)memset_s(mbox, sizeof(struct sys_mbox), 0, sizeof(struct sys_mbox));

  mbox->msgs = (void **)mem_malloc(sizeof(void *) * size);
  if (mbox->msgs == NULL) {
    goto err_handler;
  }

  (void)memset_s(mbox->msgs, (sizeof(void *) * size), 0, (sizeof(void *) * size));

  mbox->mbox_size = size;
  mbox->mbox_origin_size = size;
  mbox->first = 0;
  mbox->last = 0;
  mbox->is_full = 0;
  mbox->is_empty = 1;
  mbox->is_auto_expand = is_auto_expand ? MBOX_AUTO_EXPAND : MBOX_NO_EXPAND;

  ret = pthread_mutex_init(&(mbox->mutex), NULL);
  if (ret != 0) {
    goto err_handler;
  }

  ret = pthread_cond_init(&(mbox->not_empty), NULL);
  if (ret != 0) {
    (void)pthread_mutex_destroy(&(mbox->mutex));
    goto err_handler;
  }

  ret = pthread_cond_init(&(mbox->not_full), NULL);
  if (ret != 0) {
    (void)pthread_mutex_destroy(&(mbox->mutex));
    (void)pthread_cond_destroy(&(mbox->not_empty));
    goto err_handler;
  }

  SYS_STATS_INC_USED(mbox);
  *mb = mbox;
  LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_new: mbox created successfully 0x%p\n", (void *)mbox));
  return ERR_OK;

err_handler:
  if (mbox != NULL) {
    if (mbox->msgs != NULL) {
      mem_free(mbox->msgs);
      mbox->msgs = NULL;
    }
    mem_free(mbox);
  }
  return ERR_MEM;
}

/*
 * Routine:  sys_dual_mbox_new
 *
 * Description:
 *      Creates a new mailbox, and a fixed size of priority mbox
 * Inputs:
 *      sys_dual_mbox_t mbox         -- Handle of mailbox
 *      int queue_sz            -- Size of elements in the mailbox
 * Outputs:
 *      err_t                   -- ERR_OK if message posted, else ERR_MEM or ERR_VAL
 */
#ifdef DUAL_MBOX
err_t
sys_dual_mbox_new(struct sys_dual_mbox **dual_mbox, int size)
{
  struct sys_dual_mbox *dmbox = NULL;
  int ret;

  if (size <= 0) {
    LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_new: dmbox size must bigger than 0\n"));
    return ERR_VAL;
  }

  dmbox = (struct sys_dual_mbox *)mem_malloc(sizeof(struct sys_dual_mbox));
  if (dmbox == NULL) {
    goto err_handler;
  }

  (void)memset_s(dmbox, sizeof(struct sys_dual_mbox), 0, sizeof(struct sys_dual_mbox));
  dmbox->msgs = (void **)mem_malloc(sizeof(void *) * size);
  if (dmbox->msgs == NULL) {
    goto err_handler;
  }

  (void)memset_s(dmbox->msgs, (sizeof(void *) * size), 0, (sizeof(void *) * size));

  dmbox->mbox_size = size;

  dmbox->first = 0;
  dmbox->last = 0;
  dmbox->is_full = 0;
  dmbox->is_empty = 1;

  dmbox->msgs_priority = (void **)mem_malloc(sizeof(void *) * TCPIP_PRTY_MBOX_SIZE);
  if (dmbox->msgs_priority == NULL) {
    goto err_handler;
  }

  (void)memset_s(dmbox->msgs_priority, (sizeof(void *) * TCPIP_PRTY_MBOX_SIZE),
                 0, (sizeof(void *) * TCPIP_PRTY_MBOX_SIZE));
  dmbox->mbox_size_priority = TCPIP_PRTY_MBOX_SIZE;

  dmbox->mbox_used = 0;
  dmbox->mbox_used_priority = 0;
  dmbox->mbox_total_used = 0;
  dmbox->first_p = 0;
  dmbox->last_p = 0;

  ret = pthread_mutex_init(&(dmbox->mutex), NULL);
  if (ret != 0) {
    goto err_handler;
  }

  ret = pthread_cond_init(&(dmbox->not_empty), NULL);
  if (ret != 0) {
    (void)pthread_mutex_destroy(&(dmbox->mutex));
    goto err_handler;
  }

  ret = pthread_cond_init(&(dmbox->not_full), NULL);
  if (ret != 0) {
    (void)pthread_mutex_destroy(&(dmbox->mutex));
    (void)pthread_cond_destroy(&(dmbox->not_empty));
    goto err_handler;
  }

  SYS_STATS_INC_USED(mbox);
  *dual_mbox = dmbox;
  LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_new: dmbox created successfully 0x%p\n", (void *)dmbox));
  return ERR_OK;

err_handler:
  if (dmbox != NULL) {
    if (dmbox->msgs != NULL) {
      mem_free(dmbox->msgs);
      dmbox->msgs = NULL;
    }

    if (dmbox->msgs_priority != NULL) {
      mem_free(dmbox->msgs_priority);
      dmbox->msgs_priority = NULL;
    }
    mem_free(dmbox);
  }
  return ERR_MEM;
}
#endif /* DUAL_MBOX */

void
sys_mbox_free(struct sys_mbox **mb)
{
  if ((mb != NULL) && (*mb != SYS_MBOX_NULL)) {
    struct sys_mbox *mbox = *mb;
    int ret;
    SYS_STATS_DEC(mbox.used);

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_free: going to free mbox 0x%p\n", (void *)mbox));

    ret = pthread_mutex_lock(&(mbox->mutex));
    if (ret != 0) {
      return;
    }

    (void)pthread_cond_destroy(&(mbox->not_empty));
    (void)pthread_cond_destroy(&(mbox->not_full));

    (void)pthread_mutex_unlock(&(mbox->mutex));

    (void)pthread_mutex_destroy(&(mbox->mutex));
    if (mbox->msgs != NULL) {
      mem_free(mbox->msgs);
    }
    mbox->msgs = NULL;
    mem_free(mbox);
    *mb = NULL;

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_free: freed mbox\n"));
  }
}

#ifdef DUAL_MBOX
void
sys_dual_mbox_free(struct sys_dual_mbox **dual_mbox)
{
  if ((dual_mbox != NULL) && (*dual_mbox != SYS_MBOX_NULL)) {
    struct sys_dual_mbox *dmbox = *dual_mbox;
    int ret;
    SYS_STATS_DEC(mbox.used);

    LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_free: going to free thread dmbox 0x%p\n", (void *)dmbox));

    ret = pthread_mutex_lock(&(dmbox->mutex));
    if (ret != 0) {
      return;
    }

    (void)pthread_cond_destroy(&(dmbox->not_empty));
    (void)pthread_cond_destroy(&(dmbox->not_full));

    (void)pthread_mutex_unlock(&(dmbox->mutex));

    (void)pthread_mutex_destroy(&(dmbox->mutex));

    mem_free(dmbox->msgs_priority);
    dmbox->msgs_priority = NULL;
    mem_free(dmbox->msgs);
    dmbox->msgs = NULL;
    mem_free(dmbox);
    *dual_mbox = NULL;

    LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_free: freed thread mbox\n"));
  }
}
#endif /* DUAL_MBOX */

static unsigned char
sys_mbox_auto_expand(struct sys_mbox *mbox)
{
  unsigned char is_expanded = 0;
  void **realloc_msgs = NULL;
  if ((mbox->is_auto_expand == MBOX_AUTO_EXPAND) &&
      (MBOX_EXPAND_MULTIPLE_SIZE * (u32_t)mbox->mbox_size) <= MAX_MBOX_SIZE) {
    realloc_msgs = mem_malloc(MBOX_EXPAND_MULTIPLE_SIZE * sizeof(void *) * (u32_t)mbox->mbox_size);
    if (realloc_msgs != NULL) {
      /* memcpy_s */
      if (mbox->first > 0) {
        (void)memcpy_s(realloc_msgs,
                       sizeof(void *) * (mbox->mbox_size - mbox->first),
                       mbox->msgs + mbox->first,
                       sizeof(void *) * (mbox->mbox_size - mbox->first));
        (void)memcpy_s(realloc_msgs + (mbox->mbox_size - mbox->first),
                       sizeof(void *) * (mbox->last),
                       mbox->msgs,
                       sizeof(void *) * (mbox->last));
      } else {
        (void)memcpy_s(realloc_msgs, sizeof(void *) * mbox->mbox_size,
                       mbox->msgs, sizeof(void *) * mbox->mbox_size);
      }
      mem_free(mbox->msgs);
      mbox->msgs = realloc_msgs;
      mbox->first = 0;
      mbox->last = mbox->mbox_size;
      mbox->mbox_size *= MBOX_EXPAND_MULTIPLE_SIZE;
      LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_post : mbox 0x%p is auto expanded\n", (void *)mbox));
      is_expanded = 1;
    }
  }
  return is_expanded;
}

static void
sys_mbox_auto_shrink(struct sys_mbox *mbox)
{
  void **realloc_msgs = NULL;
  if (mbox->is_auto_expand == MBOX_AUTO_EXPAND &&
      mbox->mbox_size / MBOX_EXPAND_MULTIPLE_SIZE >= mbox->mbox_origin_size) {
    realloc_msgs = mem_malloc(sizeof(void *) * (u32_t)mbox->mbox_size / MBOX_EXPAND_MULTIPLE_SIZE);
    if (realloc_msgs != NULL) {
      mem_free(mbox->msgs);
      mbox->msgs = realloc_msgs;
      mbox->first = 0;
      mbox->last = 0;
      mbox->mbox_size /= MBOX_EXPAND_MULTIPLE_SIZE;
      LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_post : mbox 0x%p is auto shrinked\n", (void *)mbox));
    }
  }
}

/*
 * Routine:  sys_mbox_post
 *
 * Description:
 *      Post the "msg" to the mailbox.
 * Inputs:
 *      sys_mbox_t mbox        -- Handle of mailbox
 *      void *msg              -- Pointer to data to post
 */
void
sys_mbox_post(struct sys_mbox **mb, void *msg)
{
  struct sys_mbox *mbox = NULL;
  int ret;
  if ((mb == NULL) || (*mb == NULL)) {
    return;
  }
  mbox = *mb;
  LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_post: mbox 0x%p msg 0x%p\n", (void *)mbox, (void *)msg));

  ret = pthread_mutex_lock(&(mbox->mutex));
  if (ret != 0) {
    return;
  }

  while (mbox->is_full == 1) {
    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_post : mbox 0x%p mbox 0x%p, queue is full\n", (void *)mbox, (void *)msg));
    ret = pthread_cond_wait(&(mbox->not_full), &(mbox->mutex));
    if (ret != 0) {
      (void)pthread_mutex_unlock(&(mbox->mutex));
      return;
    }
  }

  mbox->msgs[mbox->last] = msg;

  mbox->last++;
  if (mbox->last == mbox->mbox_size) {
    mbox->last = 0;
  }

  if (mbox->first == mbox->last) {
    if (sys_mbox_auto_expand(mbox) == 0) {
      mbox->is_full = 1;
    }
  }

  if (mbox->is_empty == 1) {
    mbox->is_empty = 0;
    (void)pthread_cond_signal(&(mbox->not_empty)); /* if signal failed, anyway it will unlock and go out */
    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_post : mbox 0x%p msg 0x%p, signalling not empty\n", (void *)mbox, (void *)msg));
  }
  LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_post: mbox 0x%p msg 0%p posted\n", (void *)mbox, (void *)msg));
  (void)pthread_mutex_unlock(&(mbox->mutex));
}

#ifdef DUAL_MBOX
/*
 * Routine:  sys_dual_mbox_post
 *
 * Description:
 *      Post the "msg" to the mailbox.
 * Inputs:
 *      sys_dual_mbox_t mbox        -- Handle of tcpip mailbox
 *      void *msg              -- Pointer to data to post
 */
void
sys_dual_mbox_post(struct sys_dual_mbox **dual_mbox, void *msg)
{
  struct sys_dual_mbox *dmbox = *dual_mbox;

  int ret;

  LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_post: dmbox 0x%p msg 0x%p\n", (void *)dmbox, (void *)msg));

  ret = pthread_mutex_lock(&(dmbox->mutex));
  if (ret != 0) {
    return;
  }

  while ((dmbox->is_full == 1) || (dmbox->mbox_size - dmbox->mbox_total_used == 0)) {
    LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_post : dmbox 0x%p mbox 0x%p, queue is full\n", (void *)dmbox, (void *)msg));
    ret = pthread_cond_wait(&(dmbox->not_full), &(dmbox->mutex));
    if (ret != 0) {
      (void)pthread_mutex_unlock(&dmbox->mutex);
      return;
    }
  }

  dmbox->msgs[dmbox->last] = msg;

  dmbox->last = (dmbox->mbox_size - 1 - dmbox->last) ? (dmbox->last + 1) : 0;
  dmbox->mbox_total_used++;
  dmbox->mbox_used++;

  if (dmbox->first == dmbox->last) {
    dmbox->is_full = 1;
  }

  if (dmbox->is_empty) {
    dmbox->is_empty = 0;
    (void)pthread_cond_signal(&(dmbox->not_empty)); /* if signal failed, anyway it will unlock and go out */
    LWIP_DEBUGF(SYS_DEBUG,
                ("sys_dual_mbox_post : dmbox 0x%p msg 0x%p, signalling not empty\n", (void *)dmbox, (void *)msg));
  }
  LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_post: dmbox 0x%p msg 0%p posted\n", (void *)dmbox, (void *)msg));
  (void)pthread_mutex_unlock(&(dmbox->mutex));
}

/*
 * Routine:  sys_dual_mbox_post_priority
 *
 * Description:
 *      Post the "msg" to the mailbox.
 * Inputs:
 *      sys_dual_mbox_t dmbox        -- Handle of tcpip mailbox
 *      void *msg              -- Pointer to data to post
 */
void
sys_dual_mbox_post_priority(struct sys_dual_mbox **dual_mbox, void *msg)
{
  struct sys_dual_mbox *dmbox = *dual_mbox;
  int ret;
  LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_post_priority: dmbox 0x%p msg 0x%p\n", (void *)dmbox, (void *)msg));

  ret = pthread_mutex_lock(&(dmbox->mutex));
  if (ret != 0) {
    return;
  }
  while ((dmbox->is_full == 1) || (dmbox->mbox_total_used == dmbox->mbox_size) ||
         (dmbox->mbox_used_priority == dmbox->mbox_size_priority)) {
    LWIP_DEBUGF(SYS_DEBUG,
                ("sys_dual_mbox_post_priority : dmbox 0x%p msg 0x%p, queue is full\n", (void *)dmbox, (void *)msg));
    ret = pthread_cond_wait(&(dmbox->not_full), &(dmbox->mutex));
    if (ret != 0) {
      (void)pthread_mutex_unlock(&(dmbox->mutex));
      return;
    }
  }
  dmbox->msgs_priority[dmbox->last_p] = msg;
  dmbox->last_p = (dmbox->mbox_size_priority - 1 - dmbox->last_p) ? (dmbox->last_p + 1) : 0;
  dmbox->mbox_total_used++;
  dmbox->mbox_used_priority++;

  if (dmbox->first_p == dmbox->last_p || dmbox->mbox_total_used == dmbox->mbox_size) {
    dmbox->is_full = 1;
  }

  if (dmbox->is_empty == 1) {
    dmbox->is_empty = 0;
    (void)pthread_cond_signal(&(dmbox->not_empty)); /* if signal failed, anyway it will unlock and go out */
    LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_post_priority : dmbox 0x%p msg 0x%p, signalling not empty\n", \
                            (void *)dmbox, (void *)msg));
  }
  LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_post_priority: dmbox 0x%p msg 0%p posted\n", (void *)dmbox, (void *)msg));
  (void)pthread_mutex_unlock(&(dmbox->mutex));
}
#endif /* DUAL_MBOX */

err_t
sys_mbox_trypost(struct sys_mbox **mb, void *msg)
{
  struct sys_mbox *mbox = NULL;
  int ret;
  if ((mb == NULL) || (*mb == NULL)) {
    return ERR_ARG;
  }
  mbox = *mb;

  LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_trypost: mbox 0x%p msg 0x%p \n", (void *)mbox, (void *)msg));

  ret = pthread_mutex_lock(&(mbox->mutex));
  if (ret != 0) {
    return ERR_MEM;
  }

  if (mbox->is_full == 1) {
    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_trypost : mbox 0x%p msgx 0x%p,queue is full\n", (void *)mbox, (void *)msg));
    (void)pthread_mutex_unlock(&(mbox->mutex));
    return ERR_MEM;
  }

  mbox->msgs[mbox->last] = msg;

  mbox->last = (mbox->mbox_size - 1 - mbox->last) ? (mbox->last + 1) : 0;
  if (mbox->first == mbox->last) {
    if (sys_mbox_auto_expand(mbox) == 0) {
      mbox->is_full = 1;
    }
  }

  if (mbox->is_empty) {
    mbox->is_empty = 0;
    (void)pthread_cond_signal(&(mbox->not_empty));
  }
  LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_trypost: mbox 0x%p msg 0x%p posted\n", (void *)mbox, (void *)msg));
  (void)pthread_mutex_unlock(&(mbox->mutex));
  return ERR_OK;
}

#ifdef DUAL_MBOX
/*
 * Routine:  sys_dual_mbox_trypost
 *
 * Description:
 *      Try to post the "msg" to the mailbox.  Returns immediately with
 *      error if cannot.
 * Inputs:
 *      sys_dual_mbox_t dmbox         -- Handle of tcpip mailbox
 *      void *msg               -- Pointer to data to post
 * Outputs:
 *      err_t                   -- ERR_OK if message posted, else ERR_MEM
 *                                  if not.
 */
err_t
sys_dual_mbox_trypost(struct sys_dual_mbox **dual_mbox, void *msg)
{
  if ((dual_mbox == NULL) || (*dual_mbox == NULL) || (msg == NULL)) {
    return ERR_ARG;
  }
  struct sys_dual_mbox *dmbox = *dual_mbox;
  int ret;
  LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_trypost: dmbox 0x%p msg 0x%p \n", (void *)dmbox, (void *)msg));

  ret = pthread_mutex_lock(&(dmbox->mutex));
  if (ret != 0) {
    return ERR_MEM;
  }

  if (dmbox->is_full == 1) {
    LWIP_DEBUGF(SYS_DEBUG,
                ("sys_dual_mbox_trypost : dmbox 0x%p msgx 0x%p,queue is full\n", (void *)dmbox, (void *)msg));
    (void)pthread_mutex_unlock(&(dmbox->mutex));
    return ERR_MEM;
  }

  dmbox->msgs[dmbox->last] = msg;
  dmbox->last = (dmbox->mbox_size - 1 - dmbox->last) ? (dmbox->last + 1) : 0;
  dmbox->mbox_total_used++;
  dmbox->mbox_used++;

  if (dmbox->first == dmbox->last) {
    dmbox->is_full = 1;
  }

  if (dmbox->is_empty == 1) {
    dmbox->is_empty = 0;
    (void)pthread_cond_signal(&(dmbox->not_empty));
  }
  LWIP_DEBUGF(SYS_DEBUG, ("sys_dual_mbox_trypost: dmbox 0x%p msg 0x%p posted\n", (void *)dmbox, (void *)msg));
  (void)pthread_mutex_unlock(&(dmbox->mutex));
  return ERR_OK;
}
#endif /* DUAL_MBOX */

static u32_t
sys_arch_mbox_handler_timeouts(struct sys_mbox *mbox, u32_t timeout, u8_t ignore_timeout)
{
  struct timespec tmsp;
  int ret;
  while ((mbox->is_empty == 1) && (ignore_timeout == 0)) {
    if (timeout != 0) {
#if LWIP_USE_POSIX_COND_WAIT_WITH_ABS_TIMEOUT
      gettimeofday(&tv, NULL);
      tmsp.tv_sec = tv.tv_sec + (timeout / MS_PER_SECOND);
      tmsp.tv_nsec = (tv.tv_usec * NS_PER_USECOND) + ((timeout % US_PER_MSECOND) * US_PER_MSECOND * NS_PER_USECOND);
      tmsp.tv_sec += tmsp.tv_nsec / (NS_PER_USECOND * US_PER_MSECOND * MS_PER_SECOND);
      tmsp.tv_nsec %= (NS_PER_USECOND * US_PER_MSECOND * MS_PER_SECOND);
#else
      tmsp.tv_sec = (timeout / MS_PER_SECOND);
      tmsp.tv_nsec = ((timeout % US_PER_MSECOND) * US_PER_MSECOND * NS_PER_USECOND);
#endif

      ret = pthread_cond_timedwait(&(mbox->not_empty), &(mbox->mutex), &tmsp);
      if (ret != 0) {
        LWIP_DEBUGF(SYS_DEBUG, ("sys_arch_mbox_fetch: mbox 0x%p,timeout in cond wait\n", (void *)mbox));
        (void)pthread_mutex_unlock(&(mbox->mutex));
        return SYS_ARCH_TIMEOUT;
      }
    } else {
      LWIP_DEBUGF(SYS_DEBUG, ("sys_arch_mbox_fetch: cond wait\n"));
      ret = pthread_cond_wait(&(mbox->not_empty), &(mbox->mutex));
      if (ret != 0) {
        (void)pthread_mutex_unlock(&(mbox->mutex));
        return SYS_ARCH_TIMEOUT;
      }
    }
  }
  return ERR_OK;
}

u32_t
sys_arch_mbox_fetch_ext(struct sys_mbox **mb, void **msg, u32_t timeout, u8_t ignore_timeout)
{
  struct sys_mbox *mbox = NULL;
#if LWIP_USE_POSIX_COND_WAIT_WITH_ABS_TIMEOUT
  struct timeval tv;
#endif /* LWIP_USE_POSIX_COND_WAIT_WITH_ABS_TIMEOUT */
  int ret;
  mbox = *mb;

  LWIP_DEBUGF(SYS_DEBUG, ("sys_arch_mbox_fetch: mbox 0x%p msg 0x%p\n", (void *)mbox, (void *)msg));

  /* The mutex lock is quick so we don't bother with the timeout stuff here. */
  ret = pthread_mutex_lock(&(mbox->mutex));
  if (ret != 0) {
    return SYS_ARCH_TIMEOUT;
  }
  u32_t timeouts = sys_arch_mbox_handler_timeouts(mbox, timeout, ignore_timeout);
  if (timeouts != ERR_OK) {
    return SYS_ARCH_TIMEOUT;
  }

  if (msg != NULL) {
    *msg = mbox->msgs[mbox->first];
    LWIP_DEBUGF(SYS_DEBUG, ("sys_arch_mbox_fetch: mbox 0x%p msg 0x%p\n", (void *)mbox, (void *)*msg));
  } else {
    LWIP_DEBUGF(SYS_DEBUG, ("sys_arch_mbox_fetch: mbox 0x%p, null msg\n", (void *)mbox));
  }

  mbox->first = (mbox->mbox_size - 1 - mbox->first) ? (mbox->first + 1) : 0;

  if (mbox->first == mbox->last) {
    mbox->is_empty = 1;
    sys_mbox_auto_shrink(mbox);
  }

  if (mbox->is_full == 1) {
    mbox->is_full = 0;
    (void)pthread_cond_signal(&(mbox->not_full));
  }
  LWIP_DEBUGF(SYS_DEBUG, ("sys_arch_mbox_fetch: mbox 0x%p msg 0x%p fetched\n", (void *)mbox, (void *)msg));
  (void)pthread_mutex_unlock(&(mbox->mutex));

  return 0;
}

#ifdef DUAL_MBOX
u32_t
sys_arch_dual_mbox_fetch_ext(struct sys_dual_mbox **dual_mbox, void **msg, u32_t timeout, u8_t ignore_timeout)
{
  struct sys_dual_mbox *dmbox = *dual_mbox;
#if LWIP_USE_POSIX_COND_WAIT_WITH_ABS_TIMEOUT
  struct timeval tv;
#endif /* LWIP_USE_POSIX_COND_WAIT_WITH_ABS_TIMEOUT */
  int ret;
  LWIP_DEBUGF(SYS_DEBUG, ("sys_arch_dual_mbox_fetch: dmbox %p msg %p\n", (void *)dmbox, (void *)msg));
  /* The mutex lock is quick so we don't bother with the timeout stuff here. */
  ret = pthread_mutex_lock(&(dmbox->mutex));
  if (ret != 0) {
    return SYS_ARCH_TIMEOUT;
  }
  u32_t timeouts = sys_arch_mbox_handler_timeouts(dmbox, timeout, ignore_timeout);
  if (timeouts != ERR_OK) {
    return SYS_ARCH_TIMEOUT;
  }

  if (msg != NULL) {
    /* first check priority mbox */
    if (dmbox->mbox_used_priority > 0) {
      *msg = dmbox->msgs_priority[dmbox->first_p];
      dmbox->mbox_total_used--;
      dmbox->mbox_used_priority--;

      dmbox->first_p++;
      if (dmbox->mbox_size_priority == dmbox->first_p) {
        dmbox->first_p = 0;
      }
    } else {
      *msg = dmbox->msgs[dmbox->first];
      dmbox->mbox_total_used--;
      dmbox->mbox_used--;
      dmbox->first = (dmbox->mbox_size - 1 - dmbox->first) ? (dmbox->first + 1) : 0;
    }
    LWIP_DEBUGF(SYS_DEBUG, ("sys_arch_dual_mbox_fetch: dmbox %p msg %p\n", (void *)dmbox, (void *)*msg));
  } else {
    LWIP_DEBUGF(SYS_DEBUG, ("sys_arch_dual_mbox_fetch: dmbox %p, null msg\n", (void *)dmbox));
    if (dmbox->mbox_used_priority) {
      dmbox->first_p++;
      if (dmbox->mbox_size_priority == dmbox->first_p) {
        dmbox->first_p = 0;
      }
    } else if (dmbox->mbox_used) {
      dmbox->first = (dmbox->mbox_size - 1 - dmbox->first) ? (dmbox->first + 1) : 0;
    }
  }

  if (dmbox->mbox_total_used == 0) {
    dmbox->is_empty = 1;
  }

  if (dmbox->is_full == 1) {
    dmbox->is_full = 0;
    (void)pthread_cond_signal(&(dmbox->not_full));
  }
  LWIP_DEBUGF(SYS_DEBUG, ("sys_arch_dual_mbox_fetch: dmbox %p msg %p fetched\n", (void *)dmbox, (void *)msg));
  (void)pthread_mutex_unlock(&(dmbox->mutex));

  return 0;
}
#endif /* DUAL_MBOX */

u32_t
sys_arch_mbox_fetch(struct sys_mbox **mb, void **msg, u32_t timeout)
{
  return sys_arch_mbox_fetch_ext(mb, msg, timeout, 0);
}

#ifdef DUAL_MBOX
u32_t
sys_arch_dual_mbox_fetch(struct sys_dual_mbox **dual_mbox, void **msg, u32_t timeout)
{
  return sys_arch_dual_mbox_fetch_ext(dual_mbox, msg, timeout, 0);
}
#endif /* DUAL_MBOX */

/*
 * Routine:  sys_init
 *
 * Description:
 *      Initialize sys arch
 */
void
sys_init(void)
{
  /* set rand seed to make random sequence diff on every startup */
  extern VOID LOS_GetCpuCycle(UINT32 *puwCntHi, UINT32 *puwCntLo); /* NOPAD */
  u32_t seedhsb, seedlsb;
  LOS_GetCpuCycle(&seedhsb, &seedlsb);
  srand(seedlsb);
}


sys_prot_t
sys_arch_protect(void)
{
#if LWIP_LITEOS_COMPAT
  LOS_TaskLock();
  return 0;
#else
  int ret;

  /*
   * Note that for the UNIX port, we are using a lightweight mutex, and our
   * own counter (which is locked by the mutex). The return code is not actually
   * used.
   */
  if (lwprot_thread != pthread_self()) {
    /* We are locking the mutex where it has not been locked before or is being locked by another thread */
    ret = pthread_mutex_lock(&lwprot_mutex);
    LWIP_ASSERT("pthread_mutex_lock failed", (ret == 0));
    lwprot_thread = pthread_self();
    lwprot_count = 1;
  } else {
    /* It is already locked by THIS thread */
    lwprot_count++;
  }
  return 0;
#endif
}


/*
 * Routine:  sys_arch_unprotect
 *
 * Description:
 *      This optional function does a "fast" set of critical region
 *      protection to the value specified by pval. See the documentation for
 *      sys_arch_protect() for more information. This function is only
 *      required if your port is supporting an OS.
 * Inputs:
 *      sys_prot_t              -- Previous protection level (not used here)
 */
void
sys_arch_unprotect(sys_prot_t pval)
{
#if LWIP_LITEOS_COMPAT
  LWIP_UNUSED_ARG(pval);
  LOS_TaskUnlock();
#else
  int ret;

  LWIP_UNUSED_ARG(pval);
  if (lwprot_thread == pthread_self()) {
    if (--lwprot_count == 0) {
      lwprot_thread = (pthread_t) 0xDEAD;
      ret = pthread_mutex_unlock(&lwprot_mutex);
      LWIP_ASSERT("pthread_mutex_unlock failed", (ret == 0));
      LWIP_UNUSED_ARG(ret);
    }
  }
#endif
}

u32_t
sys_now(void)
{
  /* Lwip docs mentioned like wraparound is not a problem in this funtion */
  return (u32_t)((LOS_TickCountGet() * MS_PER_SECOND) / LOSCFG_BASE_CORE_TICK_PER_SECOND);
}

sys_thread_t
sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize, int prio)
{
  TSK_INIT_PARAM_S task;
  u32_t taskid, ret;
  (void)memset_s(&task, sizeof(task), 0, sizeof(task));

  (void)stacksize;
  /* Create host Task */
  task.pfnTaskEntry = (TSK_ENTRY_FUNC)function;
  task.uwStackSize  = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
  task.pcName = (char *)name;
  task.usTaskPrio = prio;
  task.auwArgs[0] = (UINTPTR)arg;
  task.uwResved   = LOS_TASK_STATUS_DETACHED;
  ret = LOS_TaskCreate(&taskid, &task);
  if (ret != LOS_OK) {
    LWIP_DEBUGF(SYS_DEBUG, ("sys_thread_new: LOS_TaskCreate error %u\n", (unsigned long)ret));
    return SYS_ARCH_ERROR;
  }

  return taskid;
}

#ifdef LWIP_DEBUG
/*
 * brief  Displays an error message on assertion
 *
 * This function will display an error message on an assertion
 * to the dbg output.
 *
 * param[in]    msg   Error message to display
 * param[in]    line  Line number in file with error
 * param[in]    file  Filename with error
 */
void
assert_printf(char *msg, int line, const char *file)
{
  if (msg != NULL) {
    LWIP_DEBUGF(LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
                ("%s:%d in file %s", msg, line, file));
    return;
  } else {
    LWIP_DEBUGF(LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
                ("LWIP ASSERT"));
    return;
  }
}
#endif /* LWIP_DEBUG */


err_t
sys_sem_new(sys_sem_t *sem,  u8_t count)
{
  u32_t puw_sem_handle;
  u32_t uw_ret;

  if (sem == NULL) {
    return -1;
  }

  LWIP_ASSERT("in sys_sem_new count exceeds the limit", (count < 0xFF));

  uw_ret = LOS_SemCreate(count, &puw_sem_handle);
  if (uw_ret != ERR_OK) {
    return -1;
  }

  sem->sem = GET_SEM(puw_sem_handle);

  return ERR_OK;
}

u32_t
sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
  u32_t retval;
  u64_t u64_start_tick;
  u64_t u64_end_tick;

  if ((sem == NULL) || (sem->sem == NULL)) {
    return SYS_ARCH_ERROR;
  }

  u64_start_tick = LOS_TickCountGet();

  if (timeout == 0) {
    timeout = LOS_WAIT_FOREVER;
  } else {
    timeout = LOS_MS2Tick(timeout);
    if (!timeout) {
      timeout = 1;
    }
  }
  retval = LOS_SemPend(sem->sem->semID, timeout);
  if (retval == LOS_ERRNO_SEM_TIMEOUT) {
    return SYS_ARCH_TIMEOUT;
  }
  if (retval != ERR_OK) {
    return SYS_ARCH_ERROR;
  }

  u64_end_tick = LOS_TickCountGet();
  /* Here milli second will not come more than 32 bit because timeout received as 32 bit millisecond only */
  return (u32_t)(((u64_end_tick - u64_start_tick) * MS_PER_SECOND) / LOSCFG_BASE_CORE_TICK_PER_SECOND);
}


/*
 * Routine:  sys_sem_signal
 *
 * Description:
 *      Signals (releases) a semaphore
 * Inputs:
 *      sys_sem_t sem           -- Semaphore to signal
 */
void
sys_sem_signal(sys_sem_t *sem)
{
  u32_t uw_ret;

  if ((sem == NULL) || (sem->sem == NULL)) {
    return;
  }

  uw_ret = LOS_SemPost(sem->sem->semID);
  LWIP_ASSERT("LOS_SemPost failed", (uw_ret == 0));
  UNUSED(uw_ret);

  return;
}


/*
 * Routine:  sys_sem_free
 *
 * Description:
 *      Deallocates a semaphore
 * Inputs:
 *      sys_sem_t sem           -- Semaphore to free
 */
void
sys_sem_free(sys_sem_t *sem)
{
  u32_t uw_ret;

  if ((sem == NULL) || (sem->sem == NULL)) {
    return;
  }

  uw_ret = LOS_SemDelete(sem->sem->semID);
  sem->sem = NULL;
  LWIP_ASSERT("LOS_SemDelete failed", (uw_ret == 0));
  UNUSED(uw_ret);
  return;
}

#if !LWIP_COMPAT_MUTEX
/*
 * Create a new mutex
 * @param mutex pointer to the mutex to create
 * @return a new mutex
 */
err_t
sys_mutex_new(sys_mutex_t *mutex)
{
  u32_t ret;
  ret = LOS_MuxCreate(mutex);
  LWIP_ASSERT("sys_mutex_new failed", (ret == LOS_OK));
  return ret;
}

/*
 * Lock a mutex
 * @param mutex the mutex to lock
 */
void
sys_mutex_lock(sys_mutex_t *mutex)
{
  u32_t ret;
  ret = LOS_MuxPend(*mutex, LOS_WAIT_FOREVER);
  LWIP_ASSERT("sys_mutex_lock failed", (ret == LOS_OK));
  UNUSED(ret);
  return;
}

/*
 * Unlock a mutex
 * @param mutex the mutex to unlock
 */
void
sys_mutex_unlock(sys_mutex_t *mutex)
{
  u32_t ret;
  ret = LOS_MuxPost(*mutex);
  LWIP_ASSERT("sys_mutex_unlock failed", (ret == LOS_OK));
  UNUSED(ret);
  return;
}

/*
 * Delete a semaphore
 * @param mutex the mutex to delete
 */
void
sys_mutex_free(sys_mutex_t *mutex)
{
  u32_t ret;
  ret = LOS_MuxDelete(*mutex);
  LWIP_ASSERT("sys_mutex_free failed", (ret == LOS_OK));
  UNUSED(ret);
  return;
}
#endif

#if LWIP_LITEOS_COMPAT && LWIP_API_MESH
unsigned int sys_arch_rand(void)
{
  unsigned int val;

  (void)hi_cipher_trng_get_random_bytes((u8_t *)(&val), sizeof(val));

  return val;
}
#endif
