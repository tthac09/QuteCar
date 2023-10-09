

include $(MAIN_TOPDIR)/build/config/usr_config.mk

ap_srcs = src/ap/hostapd.c \
src/ap/bss_load.c \
src/ap/wpa_auth_glue.c \
src/ap/utils.c \
src/ap/ap_config.c \
src/ap/sta_info.c \
src/ap/tkip_countermeasures.c \
src/ap/ap_mlme.c \
src/ap/ieee802_11_shared.c \
src/ap/drv_callbacks.c \
src/ap/ap_drv_ops.c \
src/ap/ieee802_11_ht.c \
src/ap/ieee802_11.c \
src/ap/hw_features.c \
src/ap/wpa_auth.c \
src/ap/wpa_auth_ie.c \
src/ap/pmksa_cache_auth.c \
src/ap/ieee802_1x.c \
src/ap/eap_user_db.c \
src/ap/beacon.c

mesh_srcs =
ifeq ($(CONFIG_MESH_SUPPORT), y)
mesh_srcs += src/crypto/crypto_internal.c \
    src/crypto/tls_none.c \
    src/crypto/aes-siv.c \
    src/crypto/aes-ctr.c \
    src/crypto/des-internal.c \
    wpa_supplicant/mesh.c \
    wpa_supplicant/mesh_mpm.c \
    wpa_supplicant/mesh_rsn.c \
    wpa_supplicant/wifi_mesh_api.c
endif

wps_srcs =
ifeq ($(CONFIG_WPS_SUPPORT), y)
wps_srcs += src/wps/wps.c \
    src/wps/wps_common.c \
    src/wps/wps_attr_parse.c \
    src/wps/wps_attr_build.c \
    src/wps/wps_attr_process.c \
    src/wps/wps_dev_attr.c \
    src/wps/wps_enrollee.c \
    wpa_supplicant/notify.c \
    src/eap_common/eap_wsc_common.c \
    src/eap_peer/eap_wsc.c \
    wpa_supplicant/wps_supplicant.c
endif

wpa3_srcs =
ifeq ($(CONFIG_WPA3_SUPPORT), y)
wpa3_srcs += wpa_supplicant/sme.c \
    src/rsn_supp/pmksa_cache.c
endif

mesh_wpa3_srcs =
ifeq ($(CONFIG_MESH_SUPPORT), y)
    mesh_wpa3_srcs += src/common/sae.c \
    src/common/dragonfly.c
else ifeq ($(CONFIG_WPA3_SUPPORT), y)
    mesh_wpa3_srcs += src/common/sae.c \
    src/common/dragonfly.c
endif

utils_srcs = src/utils/base64.c \
src/utils/common.c \
src/utils/ip_addr.c \
src/utils/uuid.c \
src/utils/wpa_debug.c \
src/utils/wpabuf.c \
src/utils/os_rtos.c

other_srcs = src/l2_packet/l2_packet_rtos.c \
src/rsn_supp/wpa.c \
src/rsn_supp/wpa_ie.c \
src/tls/bignum.c

eap_srcs = src/eapol_supp/eapol_supp_sm.c \
src/eap_peer/eap.c \
src/eap_common/eap_common.c \
src/eap_peer/eap_methods.c \
src/eapol_auth/eapol_auth_sm.c \
src/eap_server/eap_server.c \
src/eap_server/eap_server_identity.c \
src/eap_server/eap_server_methods.c

drv_srcs = src/drivers/drivers.c \
src/drivers/driver_common.c \
src/drivers/driver_hisi.c \
src/drivers/driver_hisi_ioctl.c


crypto_srcs = src/crypto/aes-omac1.c \
src/crypto/aes-unwrap.c \
src/crypto/aes-wrap.c \
src/crypto/random.c \
src/crypto/rc4.c \
src/crypto/crypto_mbedtls.c \
src/crypto/dh_groups.c \
src/crypto/sha1-prf.c \
src/crypto/sha256-prf.c \
src/crypto/tls_internal.c

common_srcs = src/common/ieee802_11_common.c \
src/common/wpa_common.c \
src/common/hw_features_common.c

sup_srcs = wpa_supplicant/bss.c \
wpa_supplicant/config.c \
wpa_supplicant/config_none.c \
wpa_supplicant/ctrl_iface.c \
wpa_supplicant/ctrl_iface_rtos.c \
wpa_supplicant/wpa_cli_rtos.c \
wpa_supplicant/eap_register.c \
wpa_supplicant/events.c \
wpa_supplicant/main_rtos.c \
wpa_supplicant/scan.c \
wpa_supplicant/wpa_supplicant.c \
wpa_supplicant/wpas_glue.c \
wpa_supplicant/ap.c \
wpa_supplicant/wifi_api.c \
wpa_supplicant/wifi_softap_api.c

hostapd_srcs = hostapd/main_rtos.c \
hostapd/eap_register.c \
hostapd/ctrl_iface_rtos.c

SRC_FILES = $(ap_srcs) $(mesh_srcs) $(wps_srcs) $(wpa3_srcs) $(mesh_wpa3_srcs) $(utils_srcs) $(other_srcs) $(eap_srcs) $(drv_srcs) $(crypto_srcs) $(common_srcs) $(sup_srcs) $(hostapd_srcs)
