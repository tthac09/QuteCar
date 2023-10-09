/*
 * hostapd - Authenticator for IEEE 802.11i RSN pre-authentication
 * Copyright (c) 2004-2005, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef PREAUTH_H
#define PREAUTH_H

#ifdef CONFIG_RSN_PREAUTH

int rsn_preauth_iface_init(struct hostapd_data *hapd);
void rsn_preauth_iface_deinit(struct hostapd_data *hapd);
void rsn_preauth_finished(struct hostapd_data *hapd, struct sta_info *sta,
			  int success);
void rsn_preauth_send(struct hostapd_data *hapd, struct sta_info *sta,
		      u8 *buf, size_t len);
void rsn_preauth_free_station(struct hostapd_data *hapd, struct sta_info *sta);

#else /* CONFIG_RSN_PREAUTH */

static inline int rsn_preauth_iface_init(struct hostapd_data *hapd)
{
	(void)hapd;
	return 0;
}

static inline void rsn_preauth_iface_deinit(struct hostapd_data *hapd)
{
	(void)hapd;
}

static inline void rsn_preauth_finished(struct hostapd_data *hapd,
					struct sta_info *sta,
					int success)
{
	(void)hapd;
	(void)sta;
	(void)success;
}

static inline void rsn_preauth_send(struct hostapd_data *hapd,
				    struct sta_info *sta,
				    u8 *buf, size_t len)
{
	(void)hapd;
	(void)sta;
	(void)buf;
	(void)len;
}

static inline void rsn_preauth_free_station(struct hostapd_data *hapd,
					    struct sta_info *sta)
{
	(void)hapd;
	(void)sta;
}

#endif /* CONFIG_RSN_PREAUTH */

#endif /* PREAUTH_H */
