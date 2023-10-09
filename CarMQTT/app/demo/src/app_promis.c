/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: app_promis.c
 * Author: yangjiahai
 * Create: 2020-03-17
 */

/****************************************************************************
      1 头文件包含
****************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include <hi_wifi_api.h>
#include <hi_errno.h>
#include "app_promis.h"
#include <hi_at.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
 功能描述  : 混杂模式下收包上报
*****************************************************************************/
int hi_promis_recv(void* recv_buf, int frame_len, signed char rssi)
{
    hi_at_printf("resv buf: %u , len: %d , rssi: %c\r\n", *(unsigned int*)recv_buf, frame_len, rssi);

    return HI_ERR_SUCCESS;
}

/*****************************************************************************
 功能描述  : 开启混杂模式
 输入参数  : ifname：vap模式，如：wlan0
 输出参数  : 无
 返 回 值  : 成功返回HI_ERR_SUCCESS

 修改历史      :
  1.日    期   : 2020年3月17日
    作    者   : y00521973
    修改内容   : 新生成函数
*****************************************************************************/
unsigned int hi_promis_start(const char *ifname)
{
    int ret;
    hi_wifi_ptype_filter filter = {0};

    filter.mdata_en = 1;
    filter.udata_en = 1;
    filter.mmngt_en = 1;
    filter.umngt_en = 1;

    hi_wifi_promis_set_rx_callback(hi_promis_recv);

    ret = hi_wifi_promis_enable(ifname, 1, &filter);
    if (ret != HI_ERR_SUCCESS) {
        hi_at_printf("hi_wifi_promis_enable:: set error!\r\n");
        return ret;
    }

    hi_at_printf("start promis SUCCESS!\r\n");

    return HI_ERR_SUCCESS;
}

/*****************************************************************************
 功能描述  : 关闭混杂模式
 输入参数  : ifname：vap模式，如：wlan0
 输出参数  : 无
 返 回 值  : 成功返回HI_ERR_SUCCESS

 修改历史      :
  1.日    期   : 2020年3月17日
    作    者   : y00521973
    修改内容   : 新生成函数
*****************************************************************************/
unsigned int hi_promis_stop(const char *ifname)
{
    int ret;
    hi_wifi_ptype_filter filter = {0};

    ret = hi_wifi_promis_enable(ifname, 0, &filter);
    if (ret != HI_ERR_SUCCESS) {
        hi_at_printf("hi_wifi_promis_enable:: set error!\r\n");
        return ret;
    }

    hi_at_printf("stop promis SUCCESS!\r\n");

    return HI_ERR_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif
