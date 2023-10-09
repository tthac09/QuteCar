/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL Persistent storage data elements
 * Author: NA
 * Create: 2019-05-13
 */

#ifndef _PSTORE_H_
#define _PSTORE_H_

#define PSTORE_VER 1         /* Persistant storage block version */
#define PS_MAX_PSTORE_SZ 128 /* Max persistent storage block size */

/* Persistent Store variables. Note if you add/rem here, remember to add remove from g_pstoreIdSz[] */
#define PS_VER 0
#define PS_FLAG 1
#define PS_DTSN 2
#define PS_DODAGVERNUM 3
#define PS_PATHSEQ 4
#define PS_DAOSEQ 5
#define PS_DCOSEQ 6
#define PS_MAX 7

#if PS_MAX >= PS_MAX_PSTORE_SZ
#error "PS_MAX exceeded persistent stor ID size"
#endif

uint8_t rpl_pstore_write(uint8_t id, const uint8_t *buf, uint16_t buflen);
uint8_t rpl_pstore_read(uint8_t id, uint8_t *buf, uint16_t buflen);
void rpl_pstore_clear(void);

#endif /* _PSTORE_H_ */
