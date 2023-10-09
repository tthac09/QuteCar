/*
 * WPA Supplicant - Basic mesh mode routines
 * Copyright (c) 2013-2014, cozybit, Inc.  All rights reserved.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef MESH_H
#define MESH_H

int wpa_supplicant_join_mesh(struct wpa_supplicant *wpa_s,
			     struct wpa_ssid *ssid);
int wpa_supplicant_leave_mesh(struct wpa_supplicant *wpa_s);
void wpa_supplicant_mesh_iface_deinit(struct wpa_supplicant *wpa_s,
				      struct hostapd_iface *ifmsh);
int wpas_mesh_scan_result_text(const u8 *ies, size_t ies_len, char *buf,
			       char *end);
int wpas_mesh_add_interface(struct wpa_supplicant *wpa_s, char *ifname,
			    size_t len);
int wpas_mesh_peer_remove(struct wpa_supplicant *wpa_s, const u8 *addr);
int wpas_mesh_peer_add(struct wpa_supplicant *wpa_s, const u8 *addr,
		       int duration);

#ifdef CONFIG_MESH
#ifdef LOS_WPA_PATCH
void ibss_mesh_setup_freq(struct wpa_supplicant *wpa_s,
			  const struct wpa_ssid *ssid,
			  struct hostapd_freq_params *freq);
#endif /* LOS_WPA_PATCH */

void wpa_mesh_notify_peer(struct wpa_supplicant *wpa_s, const u8 *addr,
			  const u8 *ies, size_t ie_len);
void wpa_supplicant_mesh_add_scan_ie(struct wpa_supplicant *wpa_s,
				     struct wpabuf **extra_ie);
struct hostapd_iface * hi_get_ifmsh_from_wpa_s(const struct wpa_supplicant *wpa_s);
struct mesh_rsn * hi_get_mrsn_from_wpa_s(const struct wpa_supplicant *wpa_s);

#else /* CONFIG_MESH */

static inline void wpa_mesh_notify_peer(struct wpa_supplicant *wpa_s,
					const u8 *addr,
					const u8 *ies, size_t ie_len)
{
	(void)wpa_s;
	(void)addr;
	(void)ies;
	(void)ie_len;
}

static inline void wpa_supplicant_mesh_add_scan_ie(struct wpa_supplicant *wpa_s,
						   struct wpabuf **extra_ie)
{
	(void)wpa_s;
	(void)extra_ie;
}

#endif /* CONFIG_MESH */

#endif /* MESH_H */
