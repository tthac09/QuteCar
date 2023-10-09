/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2015. All rights reserved.
 * Description: define mbox, ticks and memory adaptor
 * Author: none
 * Create: 2013
 */

#ifndef __ARCH_SYS_ARCH_H__
#define __ARCH_SYS_ARCH_H__

#include "lwip/opt.h"

#if NO_SYS == 0
#include "los_sem.h"
#include "pthread.h"
#include "los_typedef.h"
#include "semaphore.h"
#include "los_memory.h"

#define LWIP_OFFSET_OF LOS_OFF_SET_OF
typedef struct posix_sem sys_sem_t;
typedef int sys_prot_t;
#if !LWIP_COMPAT_MUTEX
typedef u32_t sys_mutex_t;
#endif

#define MBOX_NO_EXPAND    0
#define MBOX_AUTO_EXPAND  1

struct sys_mbox {
  int first, last;
  void **msgs;
  int mbox_size;
  int mbox_origin_size;
  unsigned char is_full;
  unsigned char is_empty;
  unsigned char is_auto_expand;
  pthread_cond_t not_empty;
  pthread_cond_t not_full;
  pthread_mutex_t mutex;
};

struct sys_dual_mbox {
  int first, last;
  void **msgs;
  int mbox_size;
  int mbox_total_used;
  int is_full;
  int is_empty;
  int mbox_used;
  pthread_cond_t not_empty;
  pthread_cond_t not_full;
  pthread_mutex_t mutex;

  int first_p, last_p;
  void **msgs_priority;
  int mbox_size_priority;
  int mbox_used_priority;
};
typedef struct sys_dual_mbox *sys_dual_mbox_t;

typedef struct sys_mbox *sys_mbox_t;

struct sys_thread {
  struct sys_thread *next;
  pthread_t pthread;
};

typedef unsigned int sys_thread_t;

#define sys_sem_valid(x)        (((*x).sem == NULL) ? 0 : 1)
#define sys_sem_set_invalid(x)  ((*x).sem = NULL)


#define SYS_MBOX_NULL (NULL)
#define sys_mbox_valid(mbox) (((mbox) != (void *)NULL) && (*(mbox) != (void *)NULL))
#define sys_mbox_set_invalid(mbox) do { \
  if ((mbox) != NULL) { \
    *(mbox) = NULL; \
  } \
} while (0)

/* Note: Please make sure the mbox passed is an valid pointer */
#define sys_dual_mbox_valid(mbox) (*(mbox) != NULL)

#if (MEM_MALLOC_DMA_ALIGN != 1)
static inline void *
sys_align_malloc(u16_t length)
{
  return LOS_MemAllocAlign(OS_SYS_MEM_ADDR, length, MEM_MALLOC_DMA_ALIGN);
}

static inline void
sys_align_free(void *mem)
{
  (void)LOS_MemFree(OS_SYS_MEM_ADDR, mem);
}
#endif

#else
#if defined (__cplusplus) && __cplusplus
extern "C" {
#endif
/* \brief  Get the current systick time in milliSeconds
 *
 *  Returns the current systick time in milliSeconds. This function is only
 *  used in standalone systems.
 *
 *  /returns current systick time in milliSeconds
 */
u32_t sys_now(void);

#if defined (__cplusplus) && __cplusplus
}
#endif

#endif

#endif /* __ARCH_SYS_ARCH_H__ */
