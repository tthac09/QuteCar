/*
* Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
* Description: driver_hisi_ioctl
* Author: hisilicon
* Create: 2019-03-04
*/

#include "utils/common.h"
#include "driver_hisi_ioctl.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

static int32 hisi_ioctl_command_set(const int8 *ifname, const void *buf, unsigned int cmd)
{
	hisi_ioctl_command_stru ioctl_cmd;
	(void)memset_s(&ioctl_cmd, sizeof(hisi_ioctl_command_stru), 0, sizeof(hisi_ioctl_command_stru));
	ioctl_cmd.cmd = cmd;
	ioctl_cmd.buf = (void *)buf;
	return hisi_ioctl(ifname, &ioctl_cmd);
}

int32 hisi_eapol_packet_send(const int8  *ifname, const uint8 *src_addr, const uint8 *dst_addr,
                             uint8 *buf, uint32 length)
{
	hisi_tx_eapol_stru tx_eapol;
	(void)src_addr;
	(void)dst_addr;
	(void)memset_s(&tx_eapol, sizeof(hisi_tx_eapol_stru), 0, sizeof(hisi_tx_eapol_stru));
	tx_eapol.puc_buf = buf;
	tx_eapol.ul_len  = length;
	return hisi_ioctl_command_set(ifname, &tx_eapol, HISI_IOCTL_SEND_EAPOL);
}

int32 hisi_eapol_packet_receive(const int8 *ifname, hisi_rx_eapol_stru *rx_eapol)
{
	if (rx_eapol == NULL)
		return -HISI_EFAIL;
	rx_eapol->buf = os_zalloc(EAPOL_PKT_BUF_SIZE);
	if (rx_eapol->buf == HISI_PTR_NULL) {
		return -HISI_EFAIL;
	}
	rx_eapol->len  = EAPOL_PKT_BUF_SIZE;
	return hisi_ioctl_command_set(ifname, rx_eapol, HISI_IOCTL_RECEIVE_EAPOL);
}

int32 hisi_eapol_enable(const int8 *ifname, void (*notify_callback)(void *, void *), void *context)
{
	hisi_enable_eapol_stru enable_eapol;
	(void)memset_s(&enable_eapol, sizeof(hisi_enable_eapol_stru), 0, sizeof(hisi_enable_eapol_stru));
	enable_eapol.callback = notify_callback;
	enable_eapol.contex   = context;

	return hisi_ioctl_command_set(ifname, &enable_eapol, HISI_IOCTL_ENALBE_EAPOL);
}

int32 hisi_eapol_disable(const int8 *ifname)
{
	return hisi_ioctl_command_set(ifname, (void *)ifname, HISI_IOCTL_DISABLE_EAPOL);
}

int32 hisi_ioctl_get_own_mac(const int8 *ifname, const int8 *mac_addr)
{
	return hisi_ioctl_command_set(ifname, (void *)mac_addr, HIIS_IOCTL_GET_ADDR);
}

int32 hisi_ioctl_set_ap(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_SET_AP);
}

int32 hisi_ioctl_change_beacon(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_CHANGE_BEACON);
}

int32 hisi_ioctl_get_hw_feature(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, buf, HIIS_IOCTL_GET_HW_FEATURE);
}

int32 hisi_ioctl_send_mlme(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_SEND_MLME);
}

int32 hisi_ioctl_new_key(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_NEW_KEY);
}

int32 hisi_ioctl_del_key(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_DEL_KEY);
}

int32 hisi_ioctl_set_key(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_SET_KEY);
}

int32 hisi_ioctl_set_mode(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_SET_MODE);
}

int32 hisi_ioctl_set_netdev(const int8 *ifname, const hisi_bool_enum_uint8 *netdev)
{
	return hisi_ioctl_command_set(ifname, (void *)netdev, HISI_IOCTL_SET_NETDEV);
}

int32 hisi_ioctl_scan(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_SCAN);
}

int32 hisi_ioctl_disconnet(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_DISCONNET);
}

int32 hisi_ioctl_assoc(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_ASSOC);
}

#ifdef _PRE_WLAN_FEATURE_REKEY_OFFLOAD
int32 hisi_ioctl_set_rekey_info(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_SET_REKEY_INFO);
}
#endif

int32 hisi_ioctl_send_action(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_SEND_ACTION);
}

int32 hisi_ioctl_mesh_usr_add(const int8 *ifname, const void *buf_addr)
{
	return hisi_ioctl_command_set(ifname, (void *)buf_addr, HISI_IOCTL_SET_MESH_USER);
}

int32 hisi_ioctl_mesh_set_mgtk(const int8 *ifname, const void *buf_addr)
{
	return hisi_ioctl_command_set(ifname, (void *)buf_addr, HISI_IOCTL_SET_MESH_GTK);
}

static int32 hisi_get_cmd_from_mesh_flag(enum hisi_mesh_enable_flag_type flag_type)
{
	int32 cmd_list[] = {HISI_IOCTL_EN_ACCEPT_PEER, HISI_IOCTL_EN_ACCEPT_STA};
	return cmd_list[flag_type];
}

int32 hisi_ioctl_mesh_enable_flag(const int8 *ifname, enum hisi_mesh_enable_flag_type flag_type,
                                  const void *buf_addr)
{
	if (flag_type >= HISI_MESH_ENABLE_FLAG_BUTT) {
		return -HISI_EFAIL;
	}

	return hisi_ioctl_command_set(ifname, (void *)buf_addr, hisi_get_cmd_from_mesh_flag(flag_type));
}

#ifdef CONFIG_MESH
 int32 hisi_ioctl_set_usr_app_ie(const int8 *ifname, const void *usr_app_ie)
{
	return hisi_ioctl_command_set(ifname, (void *)usr_app_ie, HISI_IOCTL_SET_USR_APP_IE);
}
#endif /* CONFIG_MESH */

int32 hisi_ioctl_sta_remove(const int8 *ifname, const void *buf_addr)
{
	return hisi_ioctl_command_set(ifname, (void *)buf_addr, HISI_IOCTL_STA_REMOVE);
}

int32 hisi_ioctl_get_drv_flags(const int8 *ifname, const void *buf_addr)
{
	return hisi_ioctl_command_set(ifname, (void *)buf_addr, HISI_IOCTL_GET_DRIVER_FLAGS);
}

int32 hisi_ioctl_set_delay_report(const int8 *ifname, const void *buf)
{
	return hisi_ioctl_command_set(ifname, (void *)buf, HISI_IOCTL_DELAY_REPORT);
}

int32 hisi_ioctl(const int8 *ifname, hisi_ioctl_command_stru *ioctl_cmd)
{
	int32 ret;
	if (ioctl_cmd == HI_NULL)
		return -HISI_EFAIL;
	ret = hisi_hwal_wpa_ioctl((int8 *)ifname, ioctl_cmd);
	if (ret != HISI_SUCC) {
		if ((ret == (-HISI_EINVAL)) && (ioctl_cmd->cmd == HISI_IOCTL_RECEIVE_EAPOL)) {
			/* When receiving eapol message, the last empty message will be received, which is a normal phenomenon */
			wpa_warning_log2(MSG_DEBUG, "hwal_wpa_ioctl ioctl_cmd->cmd=%u, lret=%d.", ioctl_cmd->cmd, ret);
		}
		else {
			/* When issuing the del key command, if the driver has deleted the user resource,
			   it will return an error, which is a normal phenomenon */
			if (ioctl_cmd->cmd != HISI_IOCTL_DEL_KEY) {
				wpa_error_log2(MSG_ERROR, "hwal_wpa_ioctl ioctl_cmd->cmd=%u, lret=%d.", ioctl_cmd->cmd, ret);
			}
		}
		return -HISI_EFAIL;
	}
	return HISI_SUCC;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
