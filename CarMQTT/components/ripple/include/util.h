/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: RPL DAG and Instance management
 * Author: NA
 * Create: 2019-04-04
 */

#ifndef _UTIL_H_
#define _UTIL_H_

/**
 * @Description: Get most significant byte
 *
 * @param v - value for which we need the MSB
 *
 * @return MSB
 */
uint8_t rpl_bit_arithm_msb(unsigned v);

/**
 * @Description: Get random number in a range
 *
 * @param a - beginning val
 * @param b - end val
 *
 * @return random value in range uniformly distributed
 */
uint32_t rpl_random_in_range(uint32_t a, uint32_t b);

#define IS_PRINTF_DEF 0
#if IS_PRINTF_DEF
#define RPL_PRN_BUF(log_type, str, buf, len) do {   \
  int i;                                            \
  log_type("%s len=%d Buffer:\n", str, (int)(len)); \
  for (i = 0; i < (len); i++) {                     \
    printf(" %02x", (uint32_t)(buf)[i]);            \
    if ((i % 8) == 0) {                             \
      printf("\n");                                 \
    } else if ((i % 4) == 0) {                      \
      printf("   ");                                \
    }                                               \
  }                                                 \
  printf("\n");                                     \
} while (0)
#else
#define RPL_PRN_BUF(log_type, str, buf, len) log_type("%s len=%d Buffer:<NOT PRINTED>\n", str, len);
#endif

/**
 * @Description: Decompose Address to prefix and lower 8 bytes address
 *
 * @param addr - Input IPv6 Address
 * @param low8 - Output lower 8 bytes address
 * @param prefix - Output prefix/64
 */
void rpl_addr_to_prefix_low8(const rpl_addr_t *addr, uint8_t *low8, rpl_addr_t *prefix);

/**
 * @Description: Compose Address from prefix and lower 8 bytes address
 *
 * @param low8 - Lower 8 bytes address
 * @param prefix - If NULL, 0xfe80 is used, else prefix is used.
 * @param addr - Output ipv6 address
 */
void rpl_prefix_low8_to_addr(const uint8_t *low8, const rpl_addr_t *prefix, rpl_addr_t *addr);

void rpl_set_pkt_route_status(int up);

void rpl_set_rte_nexthop_nonmesh(int stat);

#endif /* _UTIL_H_ */
