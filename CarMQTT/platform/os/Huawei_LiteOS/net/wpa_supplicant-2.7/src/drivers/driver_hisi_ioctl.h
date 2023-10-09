/*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
* Description: driver_hisi_ioctl header
* Author: hisilicon
* Create: 2019-03-04
*/

#ifndef __DRIVER_HISI_IOCTL_H__
#define __DRIVER_HISI_IOCTL_H__

#include "driver_hisi_common.h"
#include "wpa_supplicant/wifi_api.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define EAPOL_PKT_BUF_SIZE 800

#ifndef HISI_CHECK_DHCP_TIME
#define HISI_CHECK_DHCP_TIME 30
#endif

#ifndef NETDEV_UP
#define NETDEV_UP 0x0001
#endif

typedef enum
{
	HISI_CHAN_WIDTH_20_NOHT,
	HISI_CHAN_WIDTH_20,
	HISI_CHAN_WIDTH_40,

	HISI_CHAN_WIDTH_BUTT
} hisi_chan_width_enum;
typedef uint8 hisi_chan_width_enum_uint8;

typedef enum
{
	HISI_WPA_VERSION_1 = 1 << 0,
	HISI_WPA_VERSION_2 = 1 << 1,
} hisi_wpa_versions_enum;
typedef uint8 hisi_wpa_versions_enum_uint8;

typedef enum
{
	HISI_DISCONNECT,
	HISI_CONNECT,
} hisi_connect_status_enum;
typedef uint8 hisi_connect_status_enum_uint8;

int32 hisi_eapol_packet_send(const int8  *ifname, const uint8 *src_addr, const uint8 *dst_addr,
                             uint8 *buf, uint32 length);
int32 hisi_eapol_packet_receive(const int8 *ifname, hisi_rx_eapol_stru *rx_eapol);
int32 hisi_eapol_enable(const int8 *ifname, void (*notify_callback)(void *, void *context), void *context);
int32 hisi_eapol_disable(const int8 *ifname);
int32 hisi_ioctl_set_ap(const int8 *ifname, const void *buf);
int32 hisi_ioctl_change_beacon(const int8 *ifname, const void *buf);
int32 hisi_ioctl_send_mlme(const int8 *ifname, const void *buf);
int32 hisi_ioctl_new_key(const int8 *ifname, const void *buf);
int32 hisi_ioctl_del_key(const int8 *ifname, const void *buf);
int32 hisi_ioctl_set_key(const int8 *ifname, const void *buf);
int32 hisi_ioctl_set_mode(const int8 *ifname, const void *buf);
int32 hisi_ioctl_get_own_mac(const int8 *ifname, const int8 *mac_addr);
int32 hisi_ioctl_get_hw_feature(const int8 *ifname, const void *buf);
int32 hisi_ioctl_scan(const int8 *ifname, const void *buf);
int32 hisi_ioctl_disconnet(const int8 *ifname, const void *buf);
int32 hisi_ioctl_assoc(const int8 *ifname, const void *buf);
#ifdef _PRE_WLAN_FEATURE_REKEY_OFFLOAD
extern int32 hisi_ioctl_set_rekey_info(const int8 *ifname, const void *buf);
#endif
int32 hisi_ioctl_set_max_sta_num(const int8 *ifname, const void *buf_max_sta_num);
int32 hisi_ioctl(const int8 *ifname, hisi_ioctl_command_stru *ioctl_cmd);
int32 hisi_ioctl_set_netdev(const int8 *ifname, const hisi_bool_enum_uint8 *netdev);
int32 hisi_ioctl_ip_notify_driver(const int8 *ifname, const void *buf);
int32 hisi_ioctl_set_pm_on(const int8 *ifname, const void *buf);
int32 hisi_ioctl_sta_remove(const int8 *ifname, const void *buf_addr);
int32 hisi_ioctl_mesh_usr_add(const int8 *ifname, const void *buf_addr);
int32 hisi_ioctl_get_drv_flags(const int8 *ifname, const void *buf_addr);
int32 hisi_ioctl_set_delay_report(const int8 *ifname, const void *buf);
int32 hisi_ioctl_send_action(const int8 *ifname, const void *buf);
int32 hisi_ioctl_mesh_set_mgtk(const int8 *ifname, const void *buf_addr);
int32 hisi_ioctl_mesh_enable_flag(const int8 *ifname, enum hisi_mesh_enable_flag_type flag_type, const void *buf_addr);
#ifdef CONFIG_MESH
int32 hisi_ioctl_set_usr_app_ie(const int8 *ifname, const void *usr_app_ie);
#endif /* CONFIG_MESH */
int32 hisi_ioctl_get_mac_by_iftype(const int8 *ifname, void *buf);
int32 hisi_ioctl_set_p2p_noa(const int8 *ifname, const void *buf_addr);
int32 hisi_ioctl_set_p2p_powersave(const int8 *ifname, const void *buf_addr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /* end of driver_hisi_ioctl.h */
