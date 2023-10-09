/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: Common headers using in lwip ripple adapter
 * Author: NA
 * Create: 2019-05-03
 */
#ifndef _LWIP_RIPPLE_COMMON_H_
#define _LWIP_RIPPLE_COMMON_H_

#define COPY_IP6_ADDR(dst, src) do { \
  (dst)->addr[0] = (src)->a32[0];    \
  (dst)->addr[1] = (src)->a32[1];    \
  (dst)->addr[2] = (src)->a32[2];    \
  (dst)->addr[3] = (src)->a32[3];    \
} while (0)

#define COPY_RPL_ADDR(dst, src) do { \
  (dst)->a32[0] = (src)->addr[0];    \
  (dst)->a32[1] = (src)->addr[1];    \
  (dst)->a32[2] = (src)->addr[2];    \
  (dst)->a32[3] = (src)->addr[3];    \
} while (0)

#define PBUF_FREE(pbuf) do { \
  if ((pbuf) != NULL) {      \
    (void)pbuf_free(pbuf);   \
    (pbuf) = NULL;           \
  }                          \
} while (0)

#endif /* _LWIP_RIPPLE_COMMON_H_ */
