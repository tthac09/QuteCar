/*
 * hostapd / main()
 * Copyright (c) 2002-2011, Jouni Malinen <j@w1.fi>
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

#include "utils/common.h"
#include "ap/ap_drv_ops.h"
#include "ap/ap_config.h"
#include "ctrl_iface.h"
#include "hostapd_if.h"
#include "utils/eloop.h"
#include "eap_register.h"
#include "eap_server/eap.h"
#include "wpa_supplicant/wifi_api.h"
#include "los_task.h"
#include "securec.h"

typedef struct hostapd_conf hostapd_conf;

#define HOSTAPD_MAX_SSID_LEN 32

struct hostapd_data *g_hapd = NULL;

struct hapd_global {
	void **drv_priv;
	size_t drv_count;
};

static struct hapd_global global;
hostapd_conf g_global_conf;
static struct hapd_interfaces *g_interfaces = NULL;

/**
 * hostapd_driver_init - Preparate driver interface
 */
static int hostapd_driver_init(struct hostapd_iface *iface)
{
	struct wpa_init_params params;
	size_t i;
	struct hostapd_data *hapd = iface->bss[0];
	struct hostapd_bss_config *conf = hapd->conf;
	u8 *b = conf->bssid;

	if ((hapd->driver == NULL) || (hapd->driver->hapd_init == NULL)) {
		wpa_error_log0(MSG_ERROR, "No hostapd driver wrapper available");
		return -1;
	}

	/* Initialize the driver interface */
	if (!(b[0] | b[1] | b[2] | b[3] | b[4] | b[5]))
		b = NULL;

	(void)memset_s(&params, sizeof(params), 0, sizeof(params));
	for (i = 0; wpa_drivers[i] != NULL; i++) {
		if (wpa_drivers[i] != hapd->driver)
			continue;

		params.global_priv = global.drv_priv[i];
		break;
	}
	params.bssid = b;
	params.ifname = hapd->conf->iface;
	params.driver_params = hapd->iconf->driver_params;
	params.own_addr = hapd->own_addr;
	wpa_error_buf(MSG_ERROR, "hostapd_driver_init: ifname %s", params.ifname, os_strlen(params.ifname));

	hapd->drv_priv = hapd->driver->hapd_init(hapd, &params);
	if (hapd->drv_priv == NULL) {
		wpa_error_log0(MSG_ERROR, "hapd driver initialization failed.");
		hapd->driver = NULL;
		return -1;
	}

	return 0;
}

/**
 * hostapd_interface_init - Read configuration file and init BSS data
 *
 * This function is used to parse configuration file for a full interface (one
 * or more BSSes sharing the same radio) and allocate memory for the BSS
 * interfaces. No actiual driver operations are started.
 */
static struct hostapd_iface * hostapd_interface_init(struct hapd_interfaces *interfaces,
                                                     const char *config, int debug)
{
	struct hostapd_iface *iface = NULL;

	(void)debug;
	iface = hostapd_init(interfaces, config);
	if ((iface == NULL) || (iface->conf == NULL))
		return NULL;
	iface->interfaces = interfaces;

	if (iface->conf->bss[0]->iface[0] == '\0') {
		wpa_error_log0(MSG_ERROR, "Interface name not specified");
		hostapd_interface_deinit_free(iface);
		return NULL;
	}

	g_hapd = iface->bss[0];
	return iface;
}

static int hostapd_global_init(struct hapd_interfaces *interfaces, const char *entropy_file)
{
	int i;
	(void)interfaces;
	(void)entropy_file;

	(void)memset_s(&global, sizeof(global), 0, sizeof(global));
	if (eap_server_register_methods()) {
		wpa_error_log0(MSG_ERROR, "Failed to register EAP methods");
		return -1;
	}
	if (eloop_init(ELOOP_TASK_HOSTAPD)) {
		wpa_error_log0(MSG_ERROR, "Failed to initialize event loop");
		return -1;
	}

	for (i = 0; wpa_drivers[i]; i++)
		global.drv_count++;

	if (global.drv_count == 0) {
		wpa_error_log0(MSG_ERROR, "No drivers enabled");
		return -1;
	}
	global.drv_priv = os_calloc(global.drv_count, sizeof(void *));
	if (global.drv_priv == NULL)
		return -1;
	return 0;
}

void hostapd_global_deinit(void)
{
	os_free(global.drv_priv);
	global.drv_priv = NULL;
	g_hapd = NULL;
	eloop_destroy(ELOOP_TASK_HOSTAPD);
	eap_server_unregister_methods();
}

void hostapd_global_interfaces_deinit(void)
{
	if (g_interfaces != NULL)
		os_free(g_interfaces->iface);
	os_free(g_interfaces);
	g_interfaces = NULL;
	wpa_error_log0(MSG_ERROR, "hostapd_exit\n");
}

static int hostapd_global_run(struct hapd_interfaces *ifaces, int daemonize, const char *pid_file)
{
	(void)pid_file;
	(void)daemonize;
	(void)ifaces;
	return eloop_run(ELOOP_TASK_HOSTAPD);
}

static int hostapd_bss_init(struct hostapd_bss_config *bss, const hostapd_conf *conf, const char *ifname)
{
	int errors = 0;
	if (ifname != NULL)
		(void)os_strlcpy(bss->iface, ifname, strlen(ifname) + 1);
	bss->ssid.ssid_len = os_strlen(conf->ssid);
	if ((bss->ssid.ssid_len > SSID_MAX_LEN) || (bss->ssid.ssid_len < 1)) {
		wpa_error_buf(MSG_ERROR, "invalid SSID '%s'", conf->ssid, strlen(conf->ssid));
		errors++;
	} else {
		if (memcpy_s(bss->ssid.ssid, SSID_MAX_LEN, conf->ssid, SSID_MAX_LEN) != EOK) {
			wpa_error_log0(MSG_ERROR, "memcpy failed");
			errors++;
		}
		bss->ssid.ssid_set = 1;
	}
	bss->auth_algs = conf->auth_algs;
	bss->wpa_key_mgmt = conf->wpa_key_mgmt;
	bss->wpa = conf->wpa;
	if (bss->wpa) {
		bss->eapol_version = 1;
		if (conf->wpa_pairwise)
			bss->wpa_pairwise = conf->wpa_pairwise;
		size_t len = os_strlen((char *)conf->key);
		if (len < 8 || len > 64) { /* 8: MIN passphrase len, 64: MAX passphrase len */
			wpa_error_log1(MSG_ERROR, "invalid WPA passphrase length %d (expected 8..63)", len);
			errors++;
		} else if (len == 64) { /* 64: MAX passphrase len */
			os_free(bss->ssid.wpa_psk);
			bss->ssid.wpa_psk = os_zalloc(sizeof(struct hostapd_wpa_psk));
			if (bss->ssid.wpa_psk == NULL)
				errors++;
			else if (hexstr2bin((char *)conf->key, bss->ssid.wpa_psk->psk, PMK_LEN) ||
			         conf->key[PMK_LEN * 2] != '\0') { /* 2: HEX String to BIN */
				wpa_printf(MSG_ERROR, "Invalid PSK '%s'.", conf->key);
				errors++;
			} else {
				bss->ssid.wpa_psk->group = 1;
				os_free(bss->ssid.wpa_passphrase);
				bss->ssid.wpa_passphrase = NULL;
			}
		} else {
			os_free(bss->ssid.wpa_passphrase);
			bss->ssid.wpa_passphrase = os_strdup((char *)conf->key);
			os_free(bss->ssid.wpa_psk);
			bss->ssid.wpa_psk = NULL;
		}
	}
	bss->eap_server = 1;
#ifdef CONFIG_DRIVER_HISILICON
	bss->ignore_broadcast_ssid = conf->ignore_broadcast_ssid; /* set hidden AP */
#endif /*CONFIG_DRIVER_HISILICON*/
	return errors;
}

struct hostapd_config * hostapd_config_read2(const char *ifname)
{
	int errors = 0;
	hostapd_conf *hconf = &g_global_conf;
	if (ifname == NULL)
		return NULL;
	struct hostapd_config *conf = hostapd_config_defaults();
	if (conf == NULL)
		return NULL;

	conf->ht_capab |= HT_CAP_INFO_SHORT_GI20MHZ; /* default setting. */
	conf->hw_mode = HOSTAPD_MODE_IEEE80211G;
#ifdef CONFIG_IEEE80211N
	conf->ieee80211n = 1;
#endif
	if (g_ap_opt_set.short_gi_off == WPA_FLAG_ON)
		conf->ht_capab &= ~HT_CAP_INFO_SHORT_GI20MHZ;

	/* set default driver based on configuration */
	for (size_t i = 0; wpa_drivers[i] != NULL; i++) {
		if (os_strcmp(hconf->driver, wpa_drivers[i]->name) == 0) {
			conf->driver = wpa_drivers[i];
			break;
		}
	}
	if (conf->driver == NULL) {
		wpa_error_log0(MSG_ERROR, "No driver wrappers registered!");
		hostapd_config_free(conf);
		return NULL;
	}

	struct hostapd_bss_config *bss = conf->bss[0];
	conf->last_bss = conf->bss[0];
	conf->channel = (u8)hconf->channel_num;

	errors += hostapd_bss_init(bss, hconf, ifname);
	for (size_t i = 0; i < conf->num_bss; i++)
		hostapd_set_security_params(conf->bss[i], 1);

	if (hostapd_config_check(conf, 1))
		errors++;

#ifndef WPA_IGNORE_CONFIG_ERRORS
	if (errors) {
		wpa_error_log1(MSG_ERROR, "%d errors found in configuration file ", errors);
		wpa_error_buf(MSG_ERROR, "'%s'", ifname, strlen(ifname));
		hostapd_config_free(conf);
		conf = NULL;
	}
#endif /* WPA_IGNORE_CONFIG_ERRORS */

	return conf;
}

static void hostapd_quit(struct hapd_interfaces *interfaces, struct hisi_wifi_dev *wifi_dev)
{
	hostapd_global_ctrl_iface_deinit(interfaces);

	/* Deinitialize all interfaces */
	for (size_t i = 0; i < interfaces->count; i++) {
		if (interfaces->iface[i] == NULL)
			continue;
		hostapd_interface_deinit_free(interfaces->iface[i]);
	}

	hostapd_global_deinit();

	os_free(interfaces->iface);
	os_free(interfaces);
	g_interfaces = NULL;

	LOS_TaskLock();
	if (wifi_dev != NULL)
		wifi_dev->priv = NULL;
	LOS_TaskUnlock();

	/* send sta start failed event, softAP and STA share one task */
	(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_ERROR);
}

static struct hapd_interfaces *hostapd_interfaces_init(char *ifname)
{
	struct hapd_interfaces *interfaces = NULL;
	(void)ifname;
	interfaces = os_zalloc(sizeof(struct hapd_interfaces));
	if (interfaces == NULL) {
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_ERROR);
		return NULL;
	}
	interfaces->reload_config = hostapd_reload_config;
	interfaces->config_read_cb = hostapd_config_read2;
	interfaces->for_each_interface = hostapd_for_each_interface;
	interfaces->ctrl_iface_init = hostapd_ctrl_iface_init;
	interfaces->ctrl_iface_deinit = hostapd_ctrl_iface_deinit;
	interfaces->driver_init = hostapd_driver_init;
	interfaces->count = 1;

	interfaces->iface = os_calloc(interfaces->count, sizeof(struct hostapd_iface *));
	if (interfaces->iface == NULL) {
		wpa_error_log0(MSG_ERROR, "malloc failed");
		os_free(interfaces);
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_ERROR);
		return NULL;
	}

	if (hostapd_global_init(interfaces, NULL)) {
		wpa_error_log0(MSG_ERROR, "Failed to initilize global context");
		hostapd_global_deinit();
		os_free(interfaces->iface);
		os_free(interfaces);
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_ERROR);
		return NULL;
	}

	return interfaces;
}

static struct hisi_wifi_dev * hostapd_get_wifi_dev(const char* ifname)
{
	if (ifname == NULL) {
		wpa_error_log0(MSG_ERROR, "hostapd_main: ifname is null \n");
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_ERROR);
		return NULL;
	}

	wpa_debug_level = MSG_DEBUG;

	wpa_printf(MSG_DEBUG, "hostapd_main: ifname = %s", ifname);

	struct hisi_wifi_dev *wifi_dev = hi_get_wifi_dev_by_name(ifname);
	if (wifi_dev == NULL) {
		wpa_error_log0(MSG_ERROR, "hostapd_main: get_wifi_dev_by_name failed \n");
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_ERROR);
		return NULL;
	}

	return wifi_dev;
}

 void hostapd_main(char* ifname)
{
	wpa_error_log0(MSG_ERROR, "---> hostapd_main enter.");

	struct hisi_wifi_dev *wifi_dev = hostapd_get_wifi_dev(ifname);
	if (wifi_dev == NULL)
		return;

	struct hapd_interfaces *interfaces = hostapd_interfaces_init(ifname);
	if (interfaces == NULL)
		return;

	/* Allocate and parse configuration for full interface files */
	for (size_t i = 0; i < interfaces->count; i++) {
		interfaces->iface[i] = hostapd_interface_init(interfaces, ifname, 0);
		if (interfaces->iface[i] == NULL) {
			wpa_error_log0(MSG_ERROR, "Failed to initialize interface");
			goto OUT;
		}
	}

	LOS_TaskLock();
	wifi_dev->priv = g_hapd;
	LOS_TaskUnlock();

	g_interfaces = interfaces;

	/*
	 * Enable configured interfaces. Depending on channel configuration,
	 * this may complete full initialization before returning or use a
	 * callback mechanism to complete setup in case of operations like HT
	 * co-ex scans, ACS, or DFS are needed to determine channel parameters.
	 * In such case, the interface will be enabled from eloop context within
	 * hostapd_global_run().
	 */
	interfaces->terminate_on_error = interfaces->count;
	for (size_t i = 0; i < interfaces->count; i++) {
		if (hostapd_driver_init(interfaces->iface[i]) || hostapd_setup_interface(interfaces->iface[i]))
			goto OUT;
	}

	(void)hostapd_global_ctrl_iface_init(interfaces);

	wpa_error_buf(MSG_ERROR, "hostapd_main: wifi_dev: ifname = %s\n", wifi_dev->ifname, strlen(wifi_dev->ifname));

	if (hostapd_global_run(interfaces, 0, NULL) != HISI_FAIL) {
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_OK);
		return;
	}

	wpa_error_log0(MSG_ERROR, "Failed to start eloop");
OUT:
	hostapd_quit(interfaces, wifi_dev);
}


struct hapd_interfaces * hostapd_get_interfaces(void)
{
	return g_interfaces;
}

