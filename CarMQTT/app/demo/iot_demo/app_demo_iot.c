#include "app_demo.iot.h"
#include "iot_car_control.h"


///< this is the callback function, set to the mqtt, and if any messages come, it will be called
///< The payload here is the json string
void DemoMsgRcvCallBack(int qos, const char *topic, const char *payload)
{
    const char *requesID;
    char *tmp;
    IoTCmdResp_t resp;
    IOT_LOG_DEBUG("RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n",qos,topic, payload);
    /*app 下发的操作*/
    // 执行本次创新工程时间的车辆控制逻辑
    MQTT_car_ctrl(qos,topic, payload);



    // Response
    tmp = strstr(topic,CN_COMMADN_INDEX);
    if(tmp != NULL){
        ///< now you could deal your own works here --THE COMMAND FROM THE PLATFORM

            ///< now report the command execute result to the platform
            requesID = tmp + strlen(CN_COMMADN_INDEX);
            resp.requestID = requesID;
            resp.respName = NULL;
            resp.retCode = 0;   ////< which means 0 success and others failed
            resp.paras = NULL;
            // IOT_LOG_DEBUG("respose paras------------------------------ = %s\r\n", resp.paras);
            printf("===========response=============\n");
            (void)IoTProfileCmdResp(CONFIG_DEVICE_PWD,&resp);          
    }   
    

    return;
}

/*Smart Can*/
hi_void iot_publish_car_action(hi_u32 action_code)
{
      IoTProfileService_t service;
      IoTProfileKV_t property;
  
      memset(&property, 0, sizeof(property));
      property.type = EN_IOT_DATATYPE_INT;
      property.key = "CAR_STATUS";
     
      property.i_value = action_code;
    
      memset(&service, 0,sizeof(service));
      service.serviceID = "CAR_CTRL";//
      service.serviceProperty = &property;
  
      IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}



///< this is the demo main task entry,here we will set the wifi/cjson/mqtt ready ,and 
///< wait if any work to do in the while
static hi_void *DemoEntry(hi_void *arg)
{
    // IoTProfileService_t service;
    // IoTProfileKV_t property;
    extern hi_void WifiStaReadyWait(hi_void);
    WifiStaReadyWait();

    extern void cJsonInit(void);
    cJsonInit();
    
    printf("=========before IotMain=========\n");
    IoTMain();
    
    /*云端下发*/
    IoTSetMsgCallback(DemoMsgRcvCallBack);
/*主动上报*/
#ifdef TAKE_THE_INITIATIVE_TO_REPORT
    while (1) {
        // hi_mux_pend(g_mux_id, HI_SYS_WAIT_FOREVER);
        ///< here you could add your own works here--we report the data to the IoTplatform
        ///< now we report the data to the iot platform
       // ///< here you could add your own works here--we report the data to the IoTplatform
       /*用户可以在这调用发布函数进行发布，需要用户自己写调用函数*/
       iot_publish_car_action(1000);//发布例程      这里报错IotSendMsg Wrie queue failed！！！！！
       hi_sleep(1000);
       //printf("========in iot_publish_car_action=========\n");
    }
#endif
    return NULL;
}

///< This is the demo entry, we create a task here, and all the works has been done in the demo_entry
#define CN_IOT_TASK_STACKSIZE  0x1000   
#define CN_IOT_TASK_PRIOR 25
#define CN_IOT_TASK_NAME "IOTDEMO"
int app_demo_iot(void)
{
    hi_u32 ret;
    hi_task_attr attr = {0};
    hi_u32 taskID;

    attr.stack_size = CN_IOT_TASK_STACKSIZE;
    attr.task_prio = CN_IOT_TASK_PRIOR;
    attr.task_name = CN_IOT_TASK_NAME;
    printf("===============DemoEntry==========\n");
    ret = hi_task_create(&taskID, &attr, DemoEntry, NULL);
    if (ret != HI_ERR_SUCCESS) {
       IOT_LOG_ERROR("IOT TASK CTREATE FAILED");
    }
    return 0;
}
