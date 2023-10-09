/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: WAL layer external API interface implementation.
 * Author: shichongfu
 * Create: 2018-08-04
 */

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include <hi_at.h>
#include "at_hipriv.h"
#include "at.h"
#include "hi_wifi_mfg_test_if.h"
#include "hi_wifi_api.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/*****************************************************************************
  3 函数实现
*****************************************************************************/
hi_u32 at_hi_wifi_al_tx(hi_s32 argc, const hi_char *argv[])
{
    if (at_param_null_check(argc, argv) == HI_ERR_FAILURE) {
        return HI_ERR_FAILURE;
    }
    hi_u32 ret = hi_wifi_at_start(argc, argv, HISI_AT_AL_TX);
    return ret;
}

hi_u32 at_hi_wifi_al_rx(hi_s32 argc, const hi_char *argv[])
{
    if (at_param_null_check(argc, argv) == HI_ERR_FAILURE) {
            return HI_ERR_FAILURE;
    }
    hi_u32 ret = hi_wifi_at_start(argc, argv, HISI_AT_AL_RX);
    return ret;
}

hi_u32 at_hi_wifi_rx_info(hi_s32 argc, const hi_char *argv[])
{
    hi_u32 ret = hi_wifi_at_start(argc, argv, HISI_AT_RX_INFO);
    return ret;
}

hi_u32 at_hi_wifi_set_country(hi_s32 argc, const hi_char *argv[])
{
    if (at_param_null_check(argc, argv) == HI_ERR_FAILURE) {
        return HI_ERR_FAILURE;
    }
    hi_u32 ret = hi_wifi_at_start(argc, argv, HISI_AT_SET_COUNTRY);
    return ret;
}

hi_u32 at_hi_wifi_get_country(hi_s32 argc, const hi_char *argv[])
{
    hi_u32 ret = hi_wifi_at_start(argc, argv, HISI_AT_GET_COUNTRY);
    return ret;
}

hi_u32 at_hi_wifi_set_tpc(hi_s32 argc, const hi_char *argv[])
{
    if (at_param_null_check(argc, argv) == HI_ERR_FAILURE) {
        return HI_ERR_FAILURE;
    }
    hi_u32 ret = hi_wifi_at_start(argc, argv, HISI_AT_SET_TPC);
    return ret;
}

hi_u32 at_hi_wifi_set_rate_power_sub(hi_s32 argc, const hi_char *argv[], hi_bool tuning)
{
    hi_u8  protol, rate;
    hi_s32 val;
    hi_u8  ofs = tuning ? 1 : 0;
    hi_u8  protol_ofs = tuning ? 10 : 0; /* 10:本命令的偏移,以便与产测的at_hi_wifi_set_rate_power做区分 */
    hi_s32 low_limit = tuning ? -100 : -8; /* -100:调试命令下限,-8:产测命令下限 */
    hi_s32 up_limit = tuning ? 40 : 7; /* 40:调试命令上限,7:产测命令上限 */

    if ((at_param_null_check(argc, argv) == HI_ERR_FAILURE) || (argc != 3)) { /* 本命令固定3个参数 */
        return HI_ERR_FAILURE;
    }

    /* get protol */
    if ((integer_check(argv[0]) != HI_ERR_SUCCESS) ||
        (atoi(argv[0]) < HI_WIFI_PHY_MODE_11BGN) || (atoi(argv[0]) > HI_WIFI_PHY_MODE_11B)) { /* 范围0~2 */
        return HI_ERR_FAILURE;
    }
    protol = (hi_u8)atoi(argv[0]);

    /* get rate */
    if (integer_check(argv[1]) != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    if (((protol == HI_WIFI_PHY_MODE_11BGN) && ((atoi(argv[1]) < 0) || (atoi(argv[1]) > 7 + ofs))) || /* 11n范围0~7 */
        ((protol == HI_WIFI_PHY_MODE_11BG) && ((atoi(argv[1]) < 0) || (atoi(argv[1]) > 7 + ofs))) ||  /* 11g范围0~7 */
        ((protol == HI_WIFI_PHY_MODE_11B) && ((atoi(argv[1]) < 0) || (atoi(argv[1]) > 3 + ofs)))) { /* 11b范围0~3 */
        return HI_ERR_FAILURE;
    }
    rate = (hi_u8)atoi(argv[1]);

    /* get val */
    if (argv[2][0] == '-') { /* 参数2 */
        if (((argv[2][1] != '\0') && (integer_check(&argv[2][1]) != HI_ERR_SUCCESS)) || /* 参数2 */
            (argv[2][1] == '\0')) { /* 参数2 */
            return HI_ERR_FAILURE;
        }
    } else {
        if (integer_check(argv[2]) != HI_ERR_SUCCESS) { /* 参数2 */
            return HI_ERR_FAILURE;
        }
    }
    if ((atoi(argv[2]) < low_limit) || (atoi(argv[2]) > up_limit)) { /* 2:下标 */
        return HI_ERR_FAILURE;
    }
    val = atoi(argv[2]); /* 参数2 */
    protol += protol_ofs; /* 协议增加偏移区分是研发调试还是产测命令 */

    hi_u32 ret = wal_set_cal_rate_power(protol, rate, val);
    if (ret == HI_ERR_SUCCESS) {
        hi_at_printf("OK\r\n");
    }

    return ret;
}

/*****************************************************************************
 功能描述  : 对不同协议场景、不用速率分别做功率补偿，供客户研发调试用,不写efuse
 输入参数  : [1]argc 命令参数个数
             [2]argv 参数指针
 输出参数  : 无
 返 回 值  : 对不同协议场景、不用速率分别做功率补偿，at命令进行补偿参数校验 是否成功的结果
*****************************************************************************/
hi_u32 at_hi_wifi_set_rate_power(hi_s32 argc, const hi_char *argv[])
{
    return at_hi_wifi_set_rate_power_sub(argc, argv, HI_TRUE);
}

/*****************************************************************************
 功能描述  : 进行常温频偏功率补偿，at命令进行补偿参数校验
 输入参数  : [1]argc 命令参数个数
             [2]argv 参数指针
 输出参数  : 无
 返 回 值  : 进行常温频偏功率补偿，at命令进行补偿参数校验 是否成功的结果
*****************************************************************************/
hi_u32 at_hi_wifi_set_cal_freq(hi_s32 argc, const hi_char *argv[])
{
    hi_s32 freq_offset;

    if ((at_param_null_check(argc, argv) == HI_ERR_FAILURE) || (argc != 1)) { /* 本命令固定1个参数 */
        return HI_ERR_FAILURE;
    }

    /* get freq offset */
    if (argv[0][0] == '-') {
        if (((argv[0][1] != '\0') && (integer_check(&argv[0][1]) != HI_ERR_SUCCESS)) ||
            (argv[0][1] == '\0')) {
            return HI_ERR_FAILURE;
        }
    } else {
        if (integer_check(argv[0]) != HI_ERR_SUCCESS) {
            return HI_ERR_FAILURE;
        }
    }
    if ((atoi(argv[0]) < -128) || (atoi(argv[0]) > 127)) { /* 范围-128~127 */
        return HI_ERR_FAILURE;
    }
    freq_offset = atoi(argv[0]);

    hi_u32 ret = wal_set_cal_freq(freq_offset);
    if (ret == HI_ERR_SUCCESS) {
        hi_at_printf("OK\r\n");
    }

    return ret;
}

#ifdef CONFIG_FACTORY_TEST_MODE
/*****************************************************************************
 功能描述  : 对各band做平均功率补偿，at命令进行补偿参数校验
 输入参数  : [1]argc 命令参数个数
             [2]argv 参数指针
 输出参数  : 无
 返 回 值  : 对各band做平均功率补偿，at命令进行补偿参数校验 是否成功的结果
*****************************************************************************/
hi_u32 at_hi_wifi_set_cal_band_power(hi_s32 argc, const hi_char *argv[])
{
    hi_u8  band_num;
    hi_s32 offset;

    if ((at_param_null_check(argc, argv) == HI_ERR_FAILURE) || (argc != 2)) { /* 本命令固定2个参数 */
        return HI_ERR_FAILURE;
    }

    /* get band num */
    if ((integer_check(argv[0]) != HI_ERR_SUCCESS) || (atoi(argv[0]) < 0) || (atoi(argv[0]) > 2)) { /* 范围0~2 */
        return HI_ERR_FAILURE;
    }
    band_num = (hi_u8)atoi(argv[0]);

    /* get power offset */
    if (argv[1][0] == '-') {
        if (((argv[1][1] != '\0') && (integer_check(&argv[1][1]) != HI_ERR_SUCCESS)) ||
            (argv[1][1] == '\0')) {
            return HI_ERR_FAILURE;
        }
    } else {
        if (integer_check(argv[1]) != HI_ERR_SUCCESS) {
            return HI_ERR_FAILURE;
        }
    }
    if ((atoi(argv[1]) < -60) || (atoi(argv[1]) > 60)) { /* 调整范围-60~60 */
        return HI_ERR_FAILURE;
    }
    offset = atoi(argv[1]);

    hi_u32 ret = wal_set_cal_band_power(band_num, offset);
    if (ret == HI_ERR_SUCCESS) {
        hi_at_printf("OK\r\n");
    }

    return ret;
}

/*****************************************************************************
 功能描述  : 对不同协议场景、不用速率分别做功率补偿，at命令进行补偿参数校验.供产测用
 输入参数  : [1]argc 命令参数个数
             [2]argv 参数指针
 输出参数  : 无
 返 回 值  : 对不同协议场景、不用速率分别做功率补偿，at命令进行补偿参数校验 是否成功的结果
*****************************************************************************/
hi_u32 at_hi_wifi_set_cal_rate_power(hi_s32 argc, const hi_char *argv[])
{
    return at_hi_wifi_set_rate_power_sub(argc, argv, HI_FALSE);
}

hi_u32 at_hi_wifi_get_customer_mac(hi_s32 argc, const hi_char *argv[])
{
    hi_unref_param(argc);
    hi_unref_param(argv);

    hi_u32 ret = wal_get_customer_mac();
    if (ret != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }

    return HI_ERR_SUCCESS;
}

hi_u32 at_hi_wifi_set_customer_mac(hi_s32 argc, const hi_char *argv[])
{
    hi_uchar mac_addr[6]; /* 6:下标 */
    hi_u8    type = 0; /* 默认为0,即保存到efuse */
    if ((argc < 1) || (argc > 2) || (at_param_null_check(argc, argv) == HI_ERR_FAILURE)) { /* 2:参数个数 */
        return HI_ERR_FAILURE;
    }

    if (argv[0][17] != '\0') { /* 17:MAC_ADDR_LEN */
        return HI_ERR_FAILURE;
    }

    hi_u32 ret = cmd_strtoaddr(argv[0], mac_addr, 6);  /* 6:lenth */
    if (ret != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    if (argc == 2) { /* 2:命令配置了type参数 */
        /* get type */
        if (integer_check(argv[1]) != HI_ERR_SUCCESS) {
            return HI_ERR_FAILURE;
        }
        type = (hi_u8)atoi(argv[1]);
        if ((type != 0) && (type != 1)) { /* 范围0,1 */
            return HI_ERR_FAILURE;
        }
    }
    if (wal_set_customer_mac((hi_char*)mac_addr, type) != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }

    return HI_ERR_SUCCESS;
}

hi_u32 at_hi_wifi_set_dataefuse(hi_s32 argc, const hi_char *argv[])
{
    hi_u32 type = 0; /* 默认为0,即保存到efuse */

    if ((argc == 1) && (at_param_null_check(argc, argv) == HI_ERR_FAILURE)) {
        return HI_ERR_FAILURE;
    }

    if (argc == 1) { /* 命令配置了type参数 */
        /* get type */
        if (integer_check(argv[0]) != HI_ERR_SUCCESS) {
            return HI_ERR_FAILURE;
        }
        type = (hi_u32)atoi(argv[0]);
        if ((type != 0) && (type != 1)) { /* 范围0,1 */
            return HI_ERR_FAILURE;
        }
    }

    hi_u32 ret = wal_set_dataefuse(type);
    if (ret != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }

    return HI_ERR_SUCCESS;
}
#endif

hi_u32 at_hi_wifi_get_cal_data(hi_s32 argc, const hi_char *argv[])
{
    hi_unref_param(argc);
    hi_unref_param(argv);

    hi_u32 ret = wal_get_cal_data();
    if (ret != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }

    return HI_ERR_SUCCESS;
}

hi_u32 at_hi_wifi_set_trc(hi_s32 argc, const hi_char *argv[])
{
    if (at_param_null_check(argc, argv) == HI_ERR_FAILURE) {
        return HI_ERR_FAILURE;
    }
    hi_u32 ret = hi_wifi_at_start(argc, argv, HISI_AT_SET_TRC);
    return ret;
}

hi_u32 at_hi_wifi_set_rate(hi_s32 argc, const hi_char *argv[])
{
    if (at_param_null_check(argc, argv) == HI_ERR_FAILURE) {
        return HI_ERR_FAILURE;
    }
    hi_u32 ret = hi_wifi_at_start(argc, argv, HISI_AT_SET_RATE);
    return ret;
}

hi_u32 at_hi_wifi_set_arlog(hi_s32 argc, const hi_char *argv[])
{
    if (at_param_null_check(argc, argv) == HI_ERR_FAILURE) {
        return HI_ERR_FAILURE;
    }
    hi_u32 ret = hi_wifi_at_start(argc, argv, HISI_AT_SET_ARLOG);
    return ret;
}

hi_u32 at_hi_wifi_get_vap_info(hi_s32 argc, const hi_char *argv[])
{
    if (at_param_null_check(argc, argv) == HI_ERR_FAILURE) {
        return HI_ERR_FAILURE;
    }
    hi_u32 ret = hi_wifi_at_start(argc, argv, HISI_AT_GET_VAP_INFO);
    return ret;
}

hi_u32 at_hi_wifi_get_usr_info(hi_s32 argc, const hi_char *argv[])
{
    if (at_param_null_check(argc, argv) == HI_ERR_FAILURE) {
        return HI_ERR_FAILURE;
    }
    hi_u32 ret = hi_wifi_at_start(argc, argv, HISI_AT_GET_USR_INFO);
    return ret;
}


const at_cmd_func g_at_hipriv_func_tbl[] = {
    {"+RXINFO", 7, HI_NULL, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_rx_info},
    {"+CC", 3, HI_NULL, (at_call_back_func)at_hi_wifi_get_country, (at_call_back_func)at_hi_wifi_set_country, HI_NULL},
#ifndef CONFIG_FACTORY_TEST_MODE
    {"+TPC", 4, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_set_tpc, HI_NULL},
    {"+TRC", 4, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_set_trc, HI_NULL},
    {"+SETRATE", 8, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_set_rate, HI_NULL},
    {"+ARLOG", 6, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_set_arlog, HI_NULL},
    {"+VAPINFO", 8, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_get_vap_info, HI_NULL},
    {"+USRINFO", 8, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_get_usr_info, HI_NULL},
#endif
};

#define AT_HIPRIV_FUNC_NUM (sizeof(g_at_hipriv_func_tbl) / sizeof(g_at_hipriv_func_tbl[0]))

void hi_at_hipriv_cmd_register(void)
{
    hi_at_register_cmd(g_at_hipriv_func_tbl, AT_HIPRIV_FUNC_NUM);
}

const at_cmd_func g_at_hipriv_factory_test_func_tbl[] = {
    {"+ALTX", 5, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_al_tx, HI_NULL},
    {"+ALRX", 5, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_al_rx, HI_NULL},
#ifdef CONFIG_FACTORY_TEST_MODE
    {"+CALBPWR", 8, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_set_cal_band_power, HI_NULL},
    {"+CALRPWR", 8, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_set_cal_rate_power, HI_NULL},
    {"+EFUSEMAC", 9, HI_NULL, (at_call_back_func)at_hi_wifi_get_customer_mac,
        (at_call_back_func)at_hi_wifi_set_customer_mac, HI_NULL},
    {"+WCALDATA", 9, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_set_dataefuse,
        (at_call_back_func)at_hi_wifi_set_dataefuse},
#endif
    {"+CALFREQ", 8, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_set_cal_freq, HI_NULL},
    {"+SETRPWR", 8, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_set_rate_power, HI_NULL},
    {"+RCALDATA", 9, HI_NULL, HI_NULL, HI_NULL, (at_call_back_func)at_hi_wifi_get_cal_data},
};
#define AT_HIPRIV_FACTORY_TEST_FUNC_NUM (sizeof(g_at_hipriv_factory_test_func_tbl) / sizeof(g_at_hipriv_factory_test_func_tbl[0]))

void hi_at_hipriv_factory_test_cmd_register(void)
{
    hi_at_register_cmd(g_at_hipriv_factory_test_func_tbl, AT_HIPRIV_FACTORY_TEST_FUNC_NUM);
}

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

