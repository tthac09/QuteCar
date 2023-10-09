/*
 * Copyright (c) 2020 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DIAG_UTIL_H__
#define __DIAG_UTIL_H__

#include <hi_diag.h>
#include <hi_config.h>

#define ERROR_ID    501
#define WARNING_ID  502
#define INFO_ID     503

#ifdef MAKE_PRIM_XML_PROCESS_IN

/* 编程规范行宽限制例外:如下宏定义非C代码，为python脚本从代码里按行提取字符串使用，限制行宽会使python提取字符串异常 */
#define diag_layer_msg_e0(id, sz)                  {_PRIM_LAYER_ST,_PRIM_PRI_=0,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_e1(id, sz, d0)              {_PRIM_LAYER_ST,_PRIM_PRI_=0,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_e2(id, sz, d0, d1)          {_PRIM_LAYER_ST,_PRIM_PRI_=0,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_e3(id, sz, d0, d1, d2)      {_PRIM_LAYER_ST,_PRIM_PRI_=0,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_e4(id, sz, d0, d1, d2, d3)  {_PRIM_LAYER_ST,_PRIM_PRI_=0,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_buf_e(id, sz, buffer, size)     {_PRIM_LAYER_ST,_PRIM_PRI_=0,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_two_buf_e(id, sz, buf1, size1, buf2, size2)     {_PRIM_LAYER_ST,_PRIM_PRI_=0,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}

#define diag_layer_msg_w0(id, sz)                  {_PRIM_LAYER_ST,_PRIM_PRI_=1,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_w1(id, sz, d0)              {_PRIM_LAYER_ST,_PRIM_PRI_=1,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_w2(id, sz, d0, d1)          {_PRIM_LAYER_ST,_PRIM_PRI_=1,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_w3(id, sz, d0, d1, d2)      {_PRIM_LAYER_ST,_PRIM_PRI_=1,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_w4(id, sz, d0, d1, d2, d3)  {_PRIM_LAYER_ST,_PRIM_PRI_=1,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_buf_w(id, sz, buffer, size)     {_PRIM_LAYER_ST,_PRIM_PRI_=1,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_two_buf_w(id, sz, buf1, size1, buf2, size2)     {_PRIM_LAYER_ST,_PRIM_PRI_=1,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}

#define diag_layer_msg_i0(id, sz)                  {_PRIM_LAYER_ST,_PRIM_PRI_=2,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_i1(id, sz, d0)              {_PRIM_LAYER_ST,_PRIM_PRI_=2,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_i2(id, sz, d0, d1)          {_PRIM_LAYER_ST,_PRIM_PRI_=2,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_i3(id, sz, d0, d1, d2)      {_PRIM_LAYER_ST,_PRIM_PRI_=2,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_msg_i4(id, sz, d0, d1, d2, d3)  {_PRIM_LAYER_ST,_PRIM_PRI_=2,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_buf(id, sz, buffer, size)       {_PRIM_LAYER_ST,_PRIM_PRI_=2,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}
#define diag_layer_two_buf(id, sz, buf1, size1, buf2, size2)       {_PRIM_LAYER_ST,_PRIM_PRI_=2,_PRIM_ID_=id,_PRIM_SZ_=sz,_PRIM_LINE_=__LINE__,_PRIM_FILE_=__FILE_NAME__,_PRIM_FILE_ID_=__FILE_IDX__,_PRIM_MOD_ID_=2,_PRIM_END_}

#else /* MAKE_PRIM_XML_PROCESS_IN */

#define diag_layer_msg_e0(id, sz) hi_diag_layer_msg0(hi_diag_log_msg_mk_id_e(id), ERROR_ID);
#define diag_layer_msg_e1(id, sz, d0) hi_diag_layer_msg1(hi_diag_log_msg_mk_id_e(id), ERROR_ID, d0);
#define diag_layer_msg_e2(id, sz, d0, d1) hi_diag_layer_msg2(hi_diag_log_msg_mk_id_e(id), ERROR_ID, d0, d1);
#define diag_layer_msg_e3(id, sz, d0, d1, d2) hi_diag_layer_msg3(hi_diag_log_msg_mk_id_e(id), ERROR_ID, d0, d1, d2);
#define diag_layer_msg_e4(id, sz, d0, d1, d2, d3) \
do { \
    diag_log_msg log_msg = {d0, d1, d2, d3}; \
    hi_diag_layer_msg4(hi_diag_log_msg_mk_id_e(id), ERROR_ID, log_msg); \
} while (0)
#define diag_layer_buf_e(id, sz, buffer, size) hi_diag_layer_msg_buffer(hi_diag_log_msg_mk_id_e(id), ERROR_ID, \
                                                                        buffer, size);
#define diag_layer_two_buf_e(id, sz, buf1, size1, buf2, size2) \
do { \
    diag_buffer_size buf_size = {size1, size2}; \
    hi_diag_layer_two_buffer(hi_diag_log_msg_mk_id_e(id), ERROR_ID, buf1, buf2, buf_size); \
} while (0)

#define diag_layer_msg_w0(id, sz) hi_diag_layer_msg0(hi_diag_log_msg_mk_id_w(id), WARNING_ID);
#define diag_layer_msg_w1(id, sz, d0) hi_diag_layer_msg1(hi_diag_log_msg_mk_id_w(id), WARNING_ID, d0);
#define diag_layer_msg_w2(id, sz, d0, d1) hi_diag_layer_msg2(hi_diag_log_msg_mk_id_w(id), WARNING_ID, d0, d1);
#define diag_layer_msg_w3(id, sz, d0, d1, d2) hi_diag_layer_msg3(hi_diag_log_msg_mk_id_w(id), WARNING_ID, d0, d1, d2);
#define diag_layer_msg_w4(id, sz, d0, d1, d2, d3) \
do { \
    diag_log_msg log_msg = {d0, d1, d2, d3}; \
    hi_diag_layer_msg4(hi_diag_log_msg_mk_id_w(id), WARNING_ID, log_msg); \
} while (0)
#define diag_layer_buf_w(id, sz, buffer, size) hi_diag_layer_msg_buffer(hi_diag_log_msg_mk_id_w(id), WARNING_ID, \
                                                                        buffer, size);
#define diag_layer_two_buf_w(id, sz, buf1, size1, buf2, size2) \
do { \
    diag_buffer_size buf_size = {size1, size2}; \
    hi_diag_layer_two_buffer(hi_diag_log_msg_mk_id_w(id), WARNING_ID, buf1, buf2, buf_size); \
} while (0)

#define diag_layer_msg_i0(id, sz) hi_diag_layer_msg0(hi_diag_log_msg_mk_id_i(id), INFO_ID);
#define diag_layer_msg_i1(id, sz, d0) hi_diag_layer_msg1(hi_diag_log_msg_mk_id_i(id), INFO_ID, d0);
#define diag_layer_msg_i2(id, sz, d0, d1) hi_diag_layer_msg2(hi_diag_log_msg_mk_id_i(id), INFO_ID, d0, d1);
#define diag_layer_msg_i3(id, sz, d0, d1, d2) hi_diag_layer_msg3(hi_diag_log_msg_mk_id_i(id), INFO_ID, d0, d1, d2);
#define diag_layer_msg_i4(id, sz, d0, d1, d2, d3) \
do { \
    diag_log_msg log_msg = {d0, d1, d2, d3}; \
    hi_diag_layer_msg4(hi_diag_log_msg_mk_id_i(id), INFO_ID, log_msg); \
} while (0)
#define diag_layer_buf(id, sz, buffer, size) hi_diag_layer_msg_buffer(hi_diag_log_msg_mk_id_i(id), INFO_ID, \
                                                                      buffer, size);
#define diag_layer_two_buf(id, sz, buf1, size1, buf2, size2) \
do { \
    diag_buffer_size buf_size = {size1, size2}; \
    hi_diag_layer_two_buffer(hi_diag_log_msg_mk_id_i(id), INFO_ID, buf1, buf2, buf_size); \
} while (0)

#endif  /* end MAKE_PRIM_XML_PROCESS_IN */

#endif
