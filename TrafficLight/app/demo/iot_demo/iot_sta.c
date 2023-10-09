/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT platform
 * Author: HiSpark Product Team
 * Create: 2019-11-30
 */
///< this demo make the wifi to connect to the specified AP

#include "iot_config.h"
#include "iot_log.h"
#include <unistd.h>
#include <hi_wifi_api.h>
#include <lwip/ip_addr.h>
#include <lwip/netifapi.h>
#include <hi_types_base.h>
#include <hi_task.h>
#include <hi_mem.h>


#define APP_INIT_VAP_NUM    2
#define APP_INIT_USR_NUM    2

static struct netif *gLwipNetif = NULL;
static hi_bool  gScanDone = HI_FALSE;
hi_u8 wifi_status = 0;

hi_u8 wifi_first_connecting = 0;
hi_u8 wifi_second_connecting = 0;
hi_u8 wifi_second_connected = 0;
void hi_wifi_stop_sta(void);
static int hi_wifi_start_sta(void);

#define WIFI_CONNECT_STATUS ((hi_u8)0x02) 

hi_void wifi_reconnected(hi_void)
{
    if (wifi_first_connecting == WIFI_CONNECT_STATUS) {
        wifi_second_connecting = HI_TRUE;
        wifi_first_connecting = HI_FALSE;
        hi_wifi_stop_sta();
        ip4_addr_t ipAddr;
        ip4_addr_t ipAny;
        IP4_ADDR(&ipAny, 0, 0, 0, 0);
        IP4_ADDR(&ipAddr, 0, 0, 0, 0);
        hi_wifi_start_sta();
        netifapi_dhcp_start(gLwipNetif);
        while(0 == memcmp(&ipAddr, &ipAny,sizeof(ip4_addr_t))) {
            IOT_LOG_DEBUG("<Wifi reconnecting>:Wait the DHCP READY");
            netifapi_netif_get_addr(gLwipNetif,&ipAddr,NULL,NULL);
            hi_sleep(1000);
        } 
        wifi_second_connected = HI_FALSE;
        wifi_first_connecting = WIFI_CONNECT_STATUS;
        wifi_status = HI_WIFI_EVT_CONNECTED;
    }
}
/* clear netif's ip, gateway and netmask */
static void StaResetAddr(struct netif *lwipNetif)
{
    ip4_addr_t st_gw;
    ip4_addr_t st_ipaddr;
    ip4_addr_t st_netmask;

    if (lwipNetif == NULL) {
        IOT_LOG_ERROR("hisi_reset_addr::Null param of netdev");
        return;
    }

    IP4_ADDR(&st_gw, 0, 0, 0, 0);
    IP4_ADDR(&st_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&st_netmask, 0, 0, 0, 0);

    netifapi_netif_set_addr(lwipNetif, &st_ipaddr, &st_netmask, &st_gw);
}

static void WpaEventCB(const hi_wifi_event *hisi_event)
{
    int ret;
    char ifname[WIFI_IFNAME_MAX_SIZE + 1] = {0};
    int len = sizeof(ifname);

    if (hisi_event == NULL)
        return;
    IOT_LOG_DEBUG("EVENT_TYPE:%d", hisi_event->event);    
    switch (hisi_event->event) {
        case HI_WIFI_EVT_SCAN_DONE:
            IOT_LOG_DEBUG("WiFi: Scan results available");
            gScanDone = HI_TRUE;
            break;
        case HI_WIFI_EVT_CONNECTED:
            IOT_LOG_DEBUG("WiFi: Connected");
            netifapi_dhcp_start(gLwipNetif);
            wifi_status = HI_WIFI_EVT_CONNECTED;
            if (wifi_second_connected) {
                wifi_second_connected = HI_FALSE;
                wifi_first_connecting = WIFI_CONNECT_STATUS;    
            }
            break;
        case HI_WIFI_EVT_DISCONNECTED:
            IOT_LOG_DEBUG("WiFi: Disconnected");
            netifapi_dhcp_stop(gLwipNetif);
            StaResetAddr(gLwipNetif);
            wifi_status = HI_WIFI_EVT_DISCONNECTED;
            wifi_reconnected();
            break;
        case HI_WIFI_EVT_WPS_TIMEOUT:
            IOT_LOG_DEBUG("WiFi: wps is timeout");
            break;
        default:
            break;
    }
}

static int StaStartConnect(void)
{
    int ret;
    errno_t rc;
    hi_wifi_assoc_request assoc_req = {0};

    /* copy SSID to assoc_req */
    rc = memcpy_s(assoc_req.ssid, HI_WIFI_MAX_SSID_LEN + 1, CONFIG_AP_SSID, strlen(CONFIG_AP_SSID)); /* 9:ssid length */
    if (rc != EOK) {
        return -1;
    }

    /*
     * OPEN mode
     * for WPA2-PSK mode:
     * set assoc_req.auth as HI_WIFI_SECURITY_WPA2PSK,
     * then memcpy(assoc_req.key, "12345678", 8).
     */
    assoc_req.auth = HI_WIFI_SECURITY_WPA2PSK;
    rc = memcpy_s(assoc_req.key,HI_WIFI_MAX_KEY_LEN+1, CONFIG_AP_PWD, strlen(CONFIG_AP_PWD));
    if ( rc != EOK){
        return -1;
    }
    
    ret = hi_wifi_sta_connect(&assoc_req);
    if (ret != HISI_OK) {
        return -1;
    }

    return 0;
}

static int hi_wifi_start_sta(void)
{
    int ret;
    char ifname[WIFI_IFNAME_MAX_SIZE + 1] = {0};
    int len = sizeof(ifname);
    const unsigned char wifi_vap_res_num = APP_INIT_VAP_NUM;
    const unsigned char wifi_user_res_num = APP_INIT_USR_NUM;
    unsigned int  num = WIFI_SCAN_AP_LIMIT;

    ret = hi_wifi_init(wifi_vap_res_num, wifi_user_res_num);
    if (ret != HISI_OK) {
        return -1;
    }

    ret = hi_wifi_sta_start(ifname, &len);
    if (ret != HISI_OK) {
        return -1;
    }

    /* register call back function to receive wifi event, etc scan results event,
     * connected event, disconnected event.
     */
    ret = hi_wifi_register_event_callback(WpaEventCB);
    if (ret != HISI_OK) {
        IOT_LOG_DEBUG("register wifi event callback failed");
    }

    /* acquire netif for IP operation */
    gLwipNetif = netifapi_netif_find(ifname);
    if (gLwipNetif == NULL) {
        IOT_LOG_DEBUG("%s: get netif failed", __FUNCTION__);
        return -1;
    }
    /* start scan, scan results event will be received soon */
    gScanDone = HI_FALSE;
    ret = hi_wifi_sta_scan();
    if (ret != HISI_OK) {
        return -1;
    }
    /*wifi reconnecting*/
    if (wifi_second_connecting) {
        wifi_second_connecting = HI_FALSE;
        gScanDone = HI_TRUE;
        wifi_second_connected = HI_TRUE;
    }
    while (gScanDone == HI_FALSE){
        hi_sleep(1000);
        IOT_LOG_DEBUG("Wait for the wifi sta scan done");
    }
    IOT_LOG_DEBUG("wifi sta scan done");

    hi_wifi_ap_info *pst_results = hi_malloc(0, sizeof(hi_wifi_ap_info) * WIFI_SCAN_AP_LIMIT);
    if (pst_results == NULL) {
        return -1;
    }

    ret = hi_wifi_sta_scan_results(pst_results, &num);
    if (ret != HISI_OK) {
        free(pst_results);
        return -1;
    }

    for (unsigned int loop = 0; (loop < num) && (loop < WIFI_SCAN_AP_LIMIT); loop++) {
        IOT_LOG_DEBUG("SSID: %s", pst_results[loop].ssid);
    }
    hi_free(0,pst_results);

    /* we connect use the config ssid and passwd */
    ret = StaStartConnect();
    if (ret != 0) {
        return -1;
    }
    return 0;
}

void hi_wifi_stop_sta(void)
{
    int ret;

    ret = hi_wifi_sta_stop();
    if (ret != HISI_OK) {
        IOT_LOG_DEBUG("failed to stop sta");
    }

    ret = hi_wifi_deinit();
    if (ret != HISI_OK) {
        IOT_LOG_DEBUG("failed to deinit wifi");
    }

    gLwipNetif = NULL;
}

hi_void WifiStaReadyWait(hi_void)
{
    ip4_addr_t ipAddr;
    ip4_addr_t ipAny;
    IP4_ADDR(&ipAny, 0, 0, 0, 0);
    IP4_ADDR(&ipAddr, 0, 0, 0, 0);
    hi_wifi_start_sta();   

    while(0 == memcmp(&ipAddr, &ipAny,sizeof(ip4_addr_t))) {
        IOT_LOG_DEBUG("Wait the DHCP READY");
        hi_sleep(1000);
        netifapi_netif_get_addr(gLwipNetif, &ipAddr, NULL, NULL);
    }
    wifi_first_connecting = WIFI_CONNECT_STATUS;
    printf("wifi first connecting status = %d\r\n", wifi_first_connecting);
    IOT_LOG_DEBUG("wifi sta dhcp done");

    return;
}