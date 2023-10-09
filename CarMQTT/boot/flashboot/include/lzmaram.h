/*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2020-2020. All rights reserved.
* Description: lzma ram
* Author: Hisilicon
* Create: 2020-2-5
*/

#ifndef __LZMARAM_H__
#define __LZMARAM_H__

#include <hi_types.h>

typedef hi_u32 (*lzma_stream_fct)(hi_u32 offset, hi_u8 *buffer, hi_u32 size);

typedef struct {
    hi_u32 offset;
    lzma_stream_fct func;
} lzma_stream;

typedef struct {
    hi_pbyte (*alloc)(hi_pbyte p, unsigned int size);
    void (*free)(hi_pbyte p, hi_pbyte address); /* address can be 0 */
} i_sz_alloc;

extern unsigned int LzmaDecode2(const unsigned char *prop_data, unsigned int prop_size, hi_u32 *status,
                                i_sz_alloc *alloc, lzma_stream *in_stream, lzma_stream *out_stream,
                                unsigned int uncompress_len, unsigned int compress_len);

unsigned int hi_lzma_decompress(const hi_u8 lzma_head[13], hi_u32 head_size,  /* head 13B */
                                unsigned int compress_len,
                                lzma_stream_fct in_func, lzma_stream_fct out_func);
unsigned int hi_lzma_get_uncompress_len(const hi_u8 lzma_head[13], hi_u32 head_size, /* head 13B */
                                        unsigned int *pul_decompressed_data_len, unsigned int *dic_size);
unsigned int hi_lzma_mem_detect(const hi_u8 lzma_head[13], hi_u32 head_size); /* head 13B */

#endif

