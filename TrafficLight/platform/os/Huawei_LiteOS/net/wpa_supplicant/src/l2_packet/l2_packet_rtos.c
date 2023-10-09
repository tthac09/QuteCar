/*
 * WPA Supplicant - Layer2 packet handling with Linux packet sockets
 * Copyright (c) 2003-2005, Jouni Malinen <j@w1.fi>
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2019. All rights reserved.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
/****************************************************************************
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations,
 * which might include those applicable to Huawei LiteOS of U.S. and the country in
 * which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in
 * compliance with such applicable export control laws and regulations.
 ****************************************************************************/

#include "common.h"
#include "src/utils/eloop.h"
#ifdef CONFIG_DRIVER_HISILICON
#include "drivers/driver_hisi_ioctl.h"
#endif /* CONFIG_DRIVER_HISILICON */
#include "securec.h"

struct l2_packet_data {
	char ifname[IFNAMSIZ + 1];
	u8 own_addr[ETH_ALEN];
	void (*rx_callback)(void *ctx, const u8 *src_addr,
				const u8 *buf, size_t len);
	void *rx_callback_ctx;
	int l2_hdr; /* whether to include layer 2 (Ethernet) header data
			 * buffers */
	void *eloop_event;
};

int l2_packet_get_own_addr(const struct l2_packet_data *l2, u8 *addr , size_t len)
{
	if ((l2 == NULL) || (addr == NULL))
		return -1;
	if (memcpy_s(addr, len, l2->own_addr, sizeof(l2->own_addr)) != EOK)
		return -1;

	return 0;
}

int l2_packet_send(const struct l2_packet_data *l2, const u8 *dst_addr, u16 proto,
		   const u8 *buf, size_t len)
{
	int ret =0;

	(void)proto;
	if (l2 == NULL)
		return -1;
#ifdef CONFIG_DRIVER_HISILICON
	ret = hisi_eapol_packet_send(l2->ifname, l2->own_addr, dst_addr, (unsigned char *)buf, len);
#endif /* CONFIG_DRIVER_HISILICON */

	return ret;
}

static void l2_packet_receive(void *eloop_ctx, void *sock_ctx)
{
	struct l2_packet_data *l2 = eloop_ctx;

	(void)sock_ctx;

#ifdef CONFIG_DRIVER_HISILICON
	hisi_rx_eapol_stru  st_rx_eapol;
	unsigned char *puc_src = NULL;

	eloop_read_event(l2->eloop_event, 0);
	/* Callback is called only once per multiple packets, drain all of them */
	while (HISI_SUCC == hisi_eapol_packet_receive(l2->ifname, &st_rx_eapol)) {
		puc_src = (unsigned char *)(st_rx_eapol.buf + 6);
		if (l2->rx_callback != NULL)
			l2->rx_callback(l2->rx_callback_ctx, puc_src, st_rx_eapol.buf, st_rx_eapol.len);

		os_free(st_rx_eapol.buf);
		st_rx_eapol.buf = NULL;
	}

	if (st_rx_eapol.buf != NULL) {
		os_free(st_rx_eapol.buf);
		st_rx_eapol.buf = NULL;
	}
#endif /* CONFIG_DRIVER_HISILICON */
}

static void l2_packet_eapol_callback(void *ctx, void *context)
{
	struct l2_packet_data *l2 = (struct l2_packet_data *)context;

	(void)ctx;
	eloop_post_event(l2->eloop_event, NULL, 1);
}
struct l2_packet_data * l2_packet_init(
	const char *ifname, const u8 *own_addr, unsigned short protocol,
	void (*rx_callback)(void *ctx, const u8 *src_addr,
				const u8 *buf, size_t len),
	void *rx_callback_ctx, int l2_hdr)
{
	struct l2_packet_data *l2 = NULL;

	(void)own_addr;
	(void)protocol;
	if (ifname == NULL)
		return NULL;
	l2 = os_zalloc(sizeof(struct l2_packet_data));
	if (l2 == NULL) {
		return NULL;
	}
	if (strcpy_s(l2->ifname, sizeof(l2->ifname), ifname) != EOK) {
		os_free(l2);
		wpa_error_log0(MSG_ERROR, "l2_packet_init strcpy_s failed.");
		return NULL;
	}

	l2->rx_callback = rx_callback;
	l2->rx_callback_ctx = rx_callback_ctx;
	l2->l2_hdr = l2_hdr;

#ifdef CONFIG_DRIVER_HISILICON
	(void)hisi_eapol_enable(l2->ifname, l2_packet_eapol_callback, l2);
#endif /* CONFIG_DRIVER_HISILICON */
	(void)eloop_register_event(&l2->eloop_event, sizeof(void *), l2_packet_receive, l2, NULL);
	if (l2->eloop_event == NULL) {
#ifdef CONFIG_DRIVER_HISILICON
		(void)hisi_eapol_disable(l2->ifname);
#endif /* CONFIG_DRIVER_HISILICON */
		os_free(l2);
		return NULL;
	}

#ifdef CONFIG_DRIVER_HISILICON
	(void)hisi_ioctl_get_own_mac(l2->ifname, (char *)l2->own_addr);
#endif /* CONFIG_DRIVER_HISILICON */
	return l2;
}

struct l2_packet_data * l2_packet_init_bridge(
	const char *br_ifname, const char *ifname, const u8 *own_addr,
	unsigned short protocol,
	void (*rx_callback)(void *ctx, const u8 *src_addr,
				const u8 *buf, size_t len),
	void *rx_callback_ctx, int l2_hdr)
{
	(void)ifname;
	return l2_packet_init(br_ifname, own_addr, protocol, rx_callback,
				  rx_callback_ctx, l2_hdr);
}

void l2_packet_deinit(struct l2_packet_data *l2)
{
	if (l2 == NULL)
		return;

	if (l2->eloop_event != NULL) {
		(void)eloop_unregister_event(l2->eloop_event, sizeof(void *));

#ifdef CONFIG_DRIVER_HISILICON
		(void)hisi_eapol_disable(l2->ifname);
#endif /* CONFIG_DRIVER_HISILICON */
	}

	os_free(l2);
}

int l2_packet_get_ip_addr(struct l2_packet_data *l2, char *buf, size_t len)
{
	(void)l2;
	(void)buf;
	(void)len;
	return 0;
}

void l2_packet_notify_auth_start(struct l2_packet_data *l2)
{
	(void)l2;
}
