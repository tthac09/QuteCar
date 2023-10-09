/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT Platform
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */

#include "iot_config.h"
#include "iot_log.h"
#include "iot_main.h"
#include "iot_profile.h"
#include <hi_task.h>
#include <string.h>
#include <app_demo_traffic_sample.h>

extern hi_u8 g_current_mode;
extern hi_u8 g_current_type;
extern hi_u8 g_someone_walking_flag;
extern hi_u8 g_with_light_flag;
extern hi_u8 g_menu_select;
extern hi_float g_temperature;
extern hi_float g_humidity;
extern hi_float g_combustible_gas_value;
extern hi_u8 g_ahu20_temperature_buff[6];
extern hi_u8 g_ahu20_humidity_buff[6];
extern hi_u8 g_ahu20_gas_buff[6];

/*report traffic light count*/
extern hi_u32 g_red_led_auto_modu_time_count;
extern hi_u32 g_yellow_led_auto_modu_time_count;
extern hi_u32 g_green_led_auto_modu_time_count;
extern hi_u32 g_red_led_human_modu_time_count;
extern hi_u32 g_yellow_led_human_modu_time_count;
extern hi_u32 g_green_led_human_modu_time_count;
hi_u8 oc_beep_status = BEEP_OFF;
/*attribute initiative to report */
#define TAKE_THE_INITIATIVE_TO_REPORT
/*oc request id*/
#define CN_COMMADN_INDEX "commands/request_id="
/*oc report HiSpark attribute*/
#define TRAFFIC_LIGHT_CMD_PAYLOAD "led_value"
#define TRAFFIC_LIGHT_CMD_CONTROL_MODE "ControlModule"
#define TRAFFIC_LIGHT_CMD_AUTO_MODE "AutoModule"
#define TRAFFIC_LIGHT_CMD_HUMAN_MODE "HumanModule"
#define TRAFFIC_LIGHT_YELLOW_ON_PAYLOAD "YELLOW_LED_ON"
#define TRAFFIC_LIGHT_RED_ON_PAYLOAD "RED_LED_ON"
#define TRAFFIC_LIGHT_GREEN_ON_PAYLOAD "GREEN_LED_ON"
#define TRAFFIC_LIGHT_SERVICE_ID_PAYLOAD "TrafficLight"
#define TRAFFIC_LIGHT_BEEP_CONTROL "BeepControl"
#define TRAFFIC_LIGHT_BEEP_ON "BEEP_ON"
#define TRAFFIC_LIGHT_BEEP_OFF "BEEP_OFF"
#define TRAFFIC_LIGHT_HUMAN_INTERVENTION_ON "HUMAN_MODULE_ON"
#define TRAFFIC_LIGHT_HUMAN_INTERVENTION_OFF "HUMAN_MODULE_OFF"

hi_void oc_traffic_light_app_option(hi_traffic_light_mode app_option_mode, hi_control_mode_type app_option_type)
{
    g_current_mode = app_option_mode;
    switch (g_current_mode)
    {
    case TRAFFIC_CONTROL_MODE:
        // setup_clean_trfl_status(TRAFFIC_CONTROL_MODE);
        oc_traffic_light_status_report(TRAFFIC_CONTROL_MODE, setup_trfl_control_module);
        break;
    case TRAFFIC_AUTO_MODE:
        // setup_clean_trfl_status(TRAFFIC_AUTO_MODE);
        oc_traffic_light_status_report(TRAFFIC_AUTO_MODE, setup_trfl_auto_module);
        break;
    case TRAFFIC_HUMAN_MODE:
        // setup_clean_trfl_status(TRAFFIC_HUMAN_MODE);
        oc_traffic_light_status_report(TRAFFIC_HUMAN_MODE, setup_trfl_human_module);
        break;
    default:
        break;
    }
}

///< this is the callback function, set to the mqtt, and if any messages come, it will be called
///< The payload here is the json string
static void DemoMsgRcvCallBack(int qos, const char *topic, const char *payload)
{
    const char *requesID;
    char *tmp;
    IoTCmdResp_t resp;
    IOT_LOG_DEBUG("RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n", qos, topic, payload);
    /*app 下发的操作*/
    if (strstr(payload, TRAFFIC_LIGHT_SERVICE_ID_PAYLOAD) != NULL)
    { //traffic light module
        if (strstr(payload, TRAFFIC_LIGHT_CMD_CONTROL_MODE) != NULL)
        {
            g_current_mode = TRAFFIC_CONTROL_MODE;
            if (strstr(payload, TRAFFIC_LIGHT_YELLOW_ON_PAYLOAD) != NULL)
            { //YELLOW LED
                g_current_type = YELLOW_ON;
            }
            else if (strstr(payload, TRAFFIC_LIGHT_RED_ON_PAYLOAD) != NULL)
            { //RED LED
                g_current_type = RED_ON;
            }
            else if (strstr(payload, TRAFFIC_LIGHT_GREEN_ON_PAYLOAD) != NULL)
            { //GREEN LED
                g_current_type = GREEN_ON;
            }
            oc_traffic_light_app_option(g_current_mode, g_current_type);
        }
        else if (strstr(payload, TRAFFIC_LIGHT_CMD_AUTO_MODE) != NULL)
        { //Auto module
            g_current_mode = TRAFFIC_AUTO_MODE;
            g_current_type = NULL;
            oc_traffic_light_app_option(g_current_mode, g_current_type);
        }
        else if (strstr(payload, TRAFFIC_LIGHT_CMD_HUMAN_MODE) != NULL)
        { //Human module
            g_current_mode = TRAFFIC_HUMAN_MODE;
            if (strstr(payload, TRAFFIC_LIGHT_HUMAN_INTERVENTION_ON) != NULL)
            {
                g_current_type = TRAFFIC_HUMAN_TYPE;
            }
            else if (strstr(payload, TRAFFIC_LIGHT_HUMAN_INTERVENTION_OFF))
            {
                g_current_type = TRAFFIC_NORMAL_TYPE;
            }
            oc_traffic_light_app_option(g_current_mode, g_current_type);
        }
        else if (strstr(payload, TRAFFIC_LIGHT_BEEP_CONTROL) != NULL)
        { //BEEP option
            if (strstr(payload, TRAFFIC_LIGHT_BEEP_ON) != NULL)
            { //BEEP ON
                oc_beep_status = BEEP_ON;
            }
            else if (strstr(payload, TRAFFIC_LIGHT_BEEP_OFF) != NULL)
            { //BEEP OFF
                oc_beep_status = BEEP_OFF;
            }
        }
    }

    tmp = strstr(topic, CN_COMMADN_INDEX);
    if (tmp != NULL)
    {
        ///< now you could deal your own works here --THE COMMAND FROM THE PLATFORM

        ///< now er roport the command execute result to the platform
        requesID = tmp + strlen(CN_COMMADN_INDEX);
        resp.requestID = requesID;
        resp.respName = NULL;
        resp.retCode = 0; ////< which means 0 success and others failed
        resp.paras = NULL;
        // IOT_LOG_DEBUG("respose paras------------------------------ = %s\r\n", resp.paras);
        (void)IoTProfileCmdResp(CONFIG_DEVICE_PWD, &resp);
    }
    return;
}

/*traffic light:clean stautus and report status to the huawei ocean cloud*/
hi_void setup_clean_trfl_status(hi_traffic_light_mode early_mode)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;
    if (early_mode == TRAFFIC_CONTROL_MODE)
    {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "HumanModule";
        property.value = "OFF";
        memset(&service, 0, sizeof(service));
        service.serviceID = "TrafficLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    }
    else if (early_mode == TRAFFIC_AUTO_MODE)
    {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "ControlModule";
        property.value = "OFF";
        memset(&service, 0, sizeof(service));
        service.serviceID = "TrafficLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    }
    else if (early_mode == TRAFFIC_HUMAN_MODE)
    {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "AutoModule";
        property.value = "OFF";
        memset(&service, 0, sizeof(service));
        service.serviceID = "TrafficLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    }
}
/*traffic light:1.control module*/
hi_void setup_trfl_control_module(hi_traffic_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("traffic light:control module\r\n");
    if (current_mode != TRAFFIC_CONTROL_MODE && current_type != RED_ON)
    {
        printf("select current module is not the TRAFFIC_CONTROL_MODE\r\n");
        return HI_NULL;
    }
    g_current_mode = TRAFFIC_CONTROL_MODE;

    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "ControlModule";
    if (g_current_type == RED_ON)
    {
        property.value = "RED_LED_ON";
    }
    else if (g_current_type == YELLOW_ON)
    {
        property.value = "YELLOW_LED_ON";
    }
    else if (g_current_type == GREEN_ON)
    {
        property.value = "GREEN_LED_ON";
    }
    memset(&service, 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /*report beep status*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "AutoModule";
    if (oc_beep_status == BEEP_ON)
    {
        property.value = "BEEP_ON";
    }
    else
    {
        property.value = "BEEP_OFF";
    }
    memset(&service, 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
}
/*report light time count*/
hi_void report_led_light_time_count(hi_void)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;
    /*report red led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "AutoModuleRLedTC";
    property.i_value = g_red_led_auto_modu_time_count;
    memset(&service, 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /*report yellow led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "AutoModuleYLedTC";
    property.i_value = g_yellow_led_auto_modu_time_count;
    memset(&service, 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /*report green led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "AutoModuleGLedTC";
    property.i_value = g_green_led_auto_modu_time_count;
    memset(&service, 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
}
/*traffic light:2.auto module*/
hi_void setup_trfl_auto_module(hi_traffic_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("traffic light:auto module\r\n");
    if (current_mode != TRAFFIC_AUTO_MODE)
    {
        printf("select current module is not the CONTROL_MODE\r\n");
        return HI_NULL;
    }
    /*report beep status*/
    g_current_mode = TRAFFIC_AUTO_MODE;
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "AutoModule";
    if (oc_beep_status == BEEP_ON)
    {
        property.value = "BEEP_ON";
    }
    else
    {
        property.value = "BEEP_OFF";
    }
    memset(&service, 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /*report light time count*/
    report_led_light_time_count();
}

/*traffic light:3.human module*/
hi_void setup_trfl_human_module(hi_traffic_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("traffic light:human module\r\n");
    if (current_mode != TRAFFIC_HUMAN_MODE)
    {
        printf("select current module is not the CONTROL_MODE\r\n");
        return HI_NULL;
    }
    g_current_mode = TRAFFIC_HUMAN_MODE;
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "HumanModule";
    if (oc_beep_status == BEEP_ON)
    {
        property.value = "BEEP_ON";
    }
    else
    {
        property.value = "BEEP_OFF";
    }
    memset(&service, 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);

    /*red led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "HumanModuleRledTC";
    property.i_value = g_red_led_human_modu_time_count;
    memset(&service, 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /*yellow led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "HumanModuleYledTC";
    property.i_value = g_yellow_led_human_modu_time_count;
    memset(&service, 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    /*green led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "HumanModuleGledTC";
    property.i_value = g_green_led_human_modu_time_count;
    memset(&service, 0, sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
}

/*call back func and report status to huawei ocean cloud*/
hi_void oc_traffic_light_status_report(hi_traffic_light_mode current_mode, oc_trfl_call_back_func msg_report)
{
    // printf("tarffic light: reporting status...\r\n");
    switch (current_mode)
    {
    case TRAFFIC_CONTROL_MODE:
        msg_report(TRAFFIC_CONTROL_MODE, RED_ON);
        break;
    case TRAFFIC_AUTO_MODE:
        msg_report(TRAFFIC_AUTO_MODE, NULL);
        break;
    case TRAFFIC_HUMAN_MODE:
        msg_report(TRAFFIC_HUMAN_MODE, NULL);
        break;
    default:
        break;
    }
    return HI_NULL;
}

///< this is the demo main task entry,here we will set the wifi/cjson/mqtt ready ,and
///< wait if any work to do in the while
static hi_void *DemoEntry(hi_void *arg)
{
    // IoTProfileService_t service;
    // IoTProfileKV_t property;
    hi_watchdog_disable();

    extern hi_void WifiStaReadyWait(hi_void);
    WifiStaReadyWait();

    extern void cJsonInit(void);
    cJsonInit();

    IoTMain();
    IoTSetMsgCallback(DemoMsgRcvCallBack);
/*主动上报*/
#ifdef TAKE_THE_INITIATIVE_TO_REPORT
    while (1) // ========================= main while==========================
    {
        ///< here you could add your own works here--we report the data to the IoTplatform
        hi_sleep(1000);
        ///< now we report the data to the iot platform
        if (g_menu_select == TRAFFIC_LIGHT_MENU && g_menu_mode == SUB_MODE_SELECT_MODE)
        {
            switch (g_current_mode)
            {
            case TRAFFIC_CONTROL_MODE:
                // setup_clean_trfl_status(TRAFFIC_CONTROL_MODE);
                oc_traffic_light_status_report(TRAFFIC_CONTROL_MODE, setup_trfl_control_module);
                break;
            case TRAFFIC_AUTO_MODE:
                // setup_clean_trfl_status(TRAFFIC_AUTO_MODE);
                oc_traffic_light_status_report(TRAFFIC_AUTO_MODE, setup_trfl_auto_module);
                break;
            case TRAFFIC_HUMAN_MODE:
                // setup_clean_trfl_status(TRAFFIC_HUMAN_MODE);
                oc_traffic_light_status_report(TRAFFIC_HUMAN_MODE, setup_trfl_human_module);
                break;
            default:
                break;
            }
        }
    }
#endif
    return NULL;
}

///< This is the demo entry, we create a task here, and all the works has been done in the demo_entry
#define CN_IOT_TASK_STACKSIZE 0x1000
#define CN_IOT_TASK_PRIOR 28
#define CN_IOT_TASK_NAME "IOTDEMO"
int app_demo_iot(void)
{
    hi_u32 ret;
    hi_task_attr attr = {0};
    hi_u32 taskID;

    attr.stack_size = CN_IOT_TASK_STACKSIZE;
    attr.task_prio = CN_IOT_TASK_PRIOR;
    attr.task_name = CN_IOT_TASK_NAME;
    ret = hi_task_create(&taskID, &attr, DemoEntry, NULL);
    if (ret != HI_ERR_SUCCESS)
    {
        IOT_LOG_ERROR("IOT TASK CTREATE FAILED");
    }

    return 0;
}
