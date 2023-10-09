/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: head file of cipher
 * Author: jiangleipeng
 * Create: 2019-07-20
 */

#ifndef __CIPHER_H__
#define __CIPHER_H__
#include <hi_types.h>
#include <hi3861_platform_base.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif  /* __cplusplus */

hi_u32 cipher_init(hi_bool is_liteos);
hi_u32 cipher_deinit(hi_void);
hi_void cipher_clk_switch(hi_bool enable);
hi_bool cipher_get_clk_ctrl(hi_void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif  /* __cplusplus */

#endif  /* __CIPHER_H__ */

