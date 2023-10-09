/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: hisi mesh function
 * Author: shizhencang
 * Create: 2019-09-27
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 * --------------------------------------------------------------------------- */

#ifndef WPA_LOG_H
#define WPA_LOG_H

#include "diag_util.h"

#define MSG_EXCESSIVE_LEVEL 0
#define MSG_MSGDUMP_LEVEL 1
#define MSG_DEBUG_LEVEL 2
#define MSG_INFO_LEVEL 3
#define MSG_WARNING_LEVEL 4
#define MSG_ERROR_LEVEL 5
#define WPA_PRINT_LEVEL MSG_DEBUG_LEVEL

#ifndef CONFIG_NO_WPA_MSG
#define wpa_error_log0 wpa_printf
#define wpa_error_log1 wpa_printf
#define wpa_error_log2 wpa_printf
#define wpa_error_log3 wpa_printf
#define wpa_error_log4 wpa_printf

#define wpa_warning_log0 wpa_printf
#define wpa_warning_log1 wpa_printf
#define wpa_warning_log2 wpa_printf
#define wpa_warning_log3 wpa_printf
#define wpa_warning_log4 wpa_printf

#define wpa_msgdump_log0 wpa_printf
#define wpa_msgdump_log1 wpa_printf
#define wpa_msgdump_log2 wpa_printf
#define wpa_msgdump_log3 wpa_printf
#define wpa_msgdump_log4 wpa_printf

#ifdef CONFIG_DIAG_SUPPORT
#define wpa_error_buf(msg_level, fmt, buffer, size) \
			diag_layer_buf_e(0, fmt, buffer, (hi_u16)(size))
#define wpa_error_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2) \
						diag_layer_two_buf_e(0, fmt, buffer1, (hi_u16)(size1), buffer2, (hi_u16)(size2))

#define wpa_warning_buf(msg_level, fmt, buffer, size) \
			diag_layer_buf_w(0, fmt, buffer, (hi_u16)(size))
#define wpa_warning_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2) \
									diag_layer_two_buf_w(0, fmt, buffer1, (hi_u16)(size1), buffer2, (hi_u16)(size2))
#define wpa_msgdump_buf(msg_level, fmt, buffer, size) \
			diag_layer_buf(0, fmt, buffer, (hi_u16)(size))
#define wpa_msgdump_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2) \
									diag_layer_two_buf(0, fmt, buffer1, (hi_u16)(size1), buffer2, (hi_u16)(size2))
#else
#define wpa_error_buf(msg_level, fmt, buffer, size)
#define wpa_error_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2)
#define wpa_warning_buf(msg_level, fmt, buffer, size)
#define wpa_warning_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2)
#define wpa_msgdump_buf(msg_level, fmt, buffer, size)
#define wpa_msgdump_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2)
#endif

#else
#if WPA_PRINT_LEVEL > MSG_ERROR_LEVEL
#define wpa_error_log0(msg_level, fmt)
#define wpa_error_log1(msg_level, fmt, p1)
#define wpa_error_log2(msg_level, fmt, p1, p2)
#define wpa_error_log3(msg_level, fmt, p1, p2, p3)
#define wpa_error_log4(msg_level, fmt, p1, p2, p3, p4)
#define wpa_error_buf(msg_level, fmt, buffer, size)
#define wpa_error_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2)
#else

#ifdef CONFIG_DIAG_SUPPORT
#define wpa_error_log0(msg_level, fmt) \
			diag_layer_msg_e0(0, fmt)
#define wpa_error_log1(msg_level, fmt, p1) \
			diag_layer_msg_e1(0, fmt, (hi_u32)(p1))
#define wpa_error_log2(msg_level, fmt, p1, p2) \
			diag_layer_msg_e2(0, fmt, (hi_u32)(p1), (hi_u32)(p2))
#define wpa_error_log3(msg_level, fmt, p1, p2, p3) \
			diag_layer_msg_e3(0, fmt, (hi_u32)(p1), (hi_u32)(p2), (hi_u32)(p3))
#define wpa_error_log4(msg_level, fmt, p1, p2, p3, p4) \
			diag_layer_msg_e4(0, fmt, (hi_u32)(p1), (hi_u32)(p2), (hi_u32)(p3), (hi_u32)(p4))
#define wpa_error_buf(msg_level, fmt, buffer, size) \
			diag_layer_buf_e(0, fmt, buffer, (hi_u16)(size))
#define wpa_error_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2) \
						diag_layer_two_buf_e(0, fmt, buffer1, (hi_u16)(size1), buffer2, (hi_u16)(size2))
#else
#define wpa_error_log0(msg_level, fmt)
#define wpa_error_log1(msg_level, fmt, p1)
#define wpa_error_log2(msg_level, fmt, p1, p2)
#define wpa_error_log3(msg_level, fmt, p1, p2, p3)
#define wpa_error_log4(msg_level, fmt, p1, p2, p3, p4)
#define wpa_error_buf(msg_level, fmt, buffer, size)
#define wpa_error_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2)
#endif

#endif

#if WPA_PRINT_LEVEL > MSG_DEBUG_LEVEL
#define wpa_warning_log0(msg_level, fmt)
#define wpa_warning_log1(msg_level, fmt, p1)
#define wpa_warning_log2(msg_level, fmt, p1, p2)
#define wpa_warning_log3(msg_level, fmt, p1, p2, p3)
#define wpa_warning_log4(msg_level, fmt, p1, p2, p3, p4)
#define wpa_warning_buf(msg_level, fmt, buffer, size)
#define wpa_warning_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2)
#else

#ifdef CONFIG_DIAG_SUPPORT
#define wpa_warning_log0(msg_level, fmt) \
			diag_layer_msg_w0(0, fmt)
#define wpa_warning_log1(msg_level, fmt, p1) \
			diag_layer_msg_w1(0, fmt, (hi_u32)(p1))
#define wpa_warning_log2(msg_level, fmt, p1, p2) \
			diag_layer_msg_w2(0, fmt, (hi_u32)(p1), (hi_u32)(p2))
#define wpa_warning_log3(msg_level, fmt, p1, p2, p3) \
			diag_layer_msg_w3(0, fmt, (hi_u32)(p1), (hi_u32)(p2), (hi_u32)(p3))
#define wpa_warning_log4(msg_level, fmt, p1, p2, p3, p4) \
			diag_layer_msg_w4(0, fmt, (hi_u32)(p1), (hi_u32)(p2), (hi_u32)(p3), (hi_u32)(p4))
#define wpa_warning_buf(msg_level, fmt, buffer, size) \
			diag_layer_buf_w(0, fmt, buffer, (hi_u16)(size))
#define wpa_warning_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2) \
									diag_layer_two_buf_w(0, fmt, buffer1, (hi_u16)(size1), buffer2, (hi_u16)(size2))
#else
#define wpa_warning_log0(msg_level, fmt)
#define wpa_warning_log1(msg_level, fmt, p1)
#define wpa_warning_log2(msg_level, fmt, p1, p2)
#define wpa_warning_log3(msg_level, fmt, p1, p2, p3)
#define wpa_warning_log4(msg_level, fmt, p1, p2, p3, p4)
#define wpa_warning_buf(msg_level, fmt, buffer, size)
#define wpa_warning_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2)
#endif

#endif

#if WPA_PRINT_LEVEL > MSG_EXCESSIVE_LEVEL
#define wpa_msgdump_log0(msg_level, fmt)
#define wpa_msgdump_log1(msg_level, fmt, p1)
#define wpa_msgdump_log2(msg_level, fmt, p1, p2)
#define wpa_msgdump_log3(msg_level, fmt, p1, p2, p3)
#define wpa_msgdump_log4(msg_level, fmt, p1, p2, p3, p4)
#define wpa_msgdump_buf(msg_level, fmt, buffer, size)
#define wpa_msgdump_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2)
#else

#ifdef CONFIG_DIAG_SUPPORT
#define wpa_msgdump_log0(msg_level, fmt) \
			diag_layer_msg_i0(0, fmt)
#define wpa_msgdump_log1(msg_level, fmt, p1) \
			diag_layer_msg_i1(0, fmt, (hi_u32)(p1))
#define wpa_msgdump_log2(msg_level, fmt, p1, p2) \
			diag_layer_msg_i2(0, fmt, (hi_u32)(p1), (hi_u32)(p2))
#define wpa_msgdump_log3(msg_level, fmt, p1, p2, p3) \
			diag_layer_msg_i3(0, fmt, (hi_u32)(p1), (hi_u32)(p2), (hi_u32)(p3))
#define wpa_msgdump_log4(msg_level, fmt, p1, p2, p3, p4) \
			diag_layer_msg_i4(0, fmt, (hi_u32)(p1), (hi_u32)(p2), (hi_u32)(p3), (hi_u32)(p4))
#define wpa_msgdump_buf(msg_level, fmt, buffer, size) \
			diag_layer_buf(0, fmt, buffer, (hi_u16)(size))
#define wpa_msgdump_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2) \
									diag_layer_two_buf(0, fmt, buffer1, (hi_u16)(size1), buffer2, (hi_u16)(size2))
#else
#define wpa_msgdump_log0(msg_level, fmt)
#define wpa_msgdump_log1(msg_level, fmt, p1)
#define wpa_msgdump_log2(msg_level, fmt, p1, p2)
#define wpa_msgdump_log3(msg_level, fmt, p1, p2, p3)
#define wpa_msgdump_log4(msg_level, fmt, p1, p2, p3, p4)
#define wpa_msgdump_buf(msg_level, fmt, buffer, size)
#define wpa_msgdump_two_buf(msg_level, fmt, buffer1, size1, buffer2, size2)
#endif

#endif
#endif

#endif
