/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Header file for wifi aware api
 * Author: zhouliang
 * Create: 2019-08-04
 */

#ifndef __HI_WIFI_NAN_API_H__
#define __HI_WIFI_NAN_API_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef enum {
    WIFI_SDP_PUBLISH            = 0x01,
    WIFI_SDP_SUBSCRIBE          = 0x02,
    WIFI_SDP_BUTT
}wifi_sdp_type_enum;

typedef int (*hi_wifi_sdp_recv_cb)(unsigned char* mac, unsigned char peer_handle, unsigned char local_handle,
    unsigned char* msg, unsigned char len);
int hi_wifi_sdp_init(const char* ifname);
int hi_wifi_sdp_deinit(void);
int hi_wifi_sdp_start_service(const char* service_name, unsigned char local_handle,
    hi_wifi_sdp_recv_cb recv_cb, unsigned char role);
int hi_wifi_sdp_stop_service(unsigned char local_handle, unsigned char role);
int hi_wifi_sdp_send(unsigned char* mac_addr, unsigned char peer_handle, unsigned char local_handle,
    unsigned char* msg, int len);
int hi_wifi_sdp_adjust_tx_power(const char *ifname, signed char power);
int hi_wifi_sdp_restore_tx_power(const char *ifname);
int hi_wifi_sdp_adjust_rx_param(const char *ifname, signed char rssi);
int hi_wifi_sdp_restore_rx_param(const char *ifname);
int hi_wifi_sdp_beacon_switch(const char *ifname, unsigned char enable);
int hi_wifi_sdp_set_retry_times(hi_u32 retries);
#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

#endif /* end __HI_WIFI_VLWIP_API_H__ */

