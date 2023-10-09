/*
 * Copyright (c) 2020 HiHope Community.
 * Description: implement the IoT platform message package
 * Author: HiSpark Product Team
 * Create: 2020-6-20
 */

#include "iot_profile.h"
#include "iot_main.h"
#include "iot_log.h"
#include <hi_mem.h>
#include <cJSON.h>
#include <string.h>


///< format the report data to json string mode
static cJSON  *FormateProflleValue(IoTProfileKV_t  *kv)
{
    cJSON  *ret = NULL;
    switch (kv->type)
    {
        case EN_IOT_DATATYPE_INT:
            ret = cJSON_CreateNumber(kv->i_value);
            break;
        case EN_IOT_DATATYPE_LONG:
            ret = cJSON_CreateNumber((double)(*(long *)kv->value));
            break;
        case EN_IOT_DATATYPE_FLOAT:
            ret = cJSON_CreateNumber((double)(*(float *)kv->value));
            break;
        case EN_IOT_DATATYPE_DOUBLE:
            ret = cJSON_CreateNumber((*(double *)kv->value));
            break;
        case EN_IOT_DATATYPE_STRING:
            ret = cJSON_CreateString((const char *)kv->value);
            break;
        case EN_IOT_DATATYPE_LAST:
            ret = cJSON_CreateNumber(kv->oc_evvironment_value);
        default:
            break;
    }
    return ret;
}

static cJSON *MakeKvs(IoTProfileKV_t *kvlst)
{

    cJSON *root;
    cJSON *kv;
    IoTProfileKV_t  *kv_info;

    ///< build a root node
    root = cJSON_CreateObject();
    if(NULL == root){
       goto EXIT_MEM;
    }

    ///< add all the property to the properties
    kv_info = kvlst;
    while(NULL != kv_info){
        kv = FormateProflleValue( kv_info);
        if(NULL == kv){
            goto EXIT_MEM;
        }

        cJSON_AddItemToObject(root,kv_info->key,kv);
        kv_info = kv_info->nxt;
    }
    ///< OK, now we return it
    return root;

EXIT_MEM:
    if(NULL != root){
       cJSON_Delete(root);
       root = NULL;
    }
    return root;
}

#define CN_PROFILE_SERVICE_KEY_SERVICEID "service_id"
#define CN_PROFILE_SERVICE_KEY_PROPERTIIES "properties"
#define CN_PROFILE_SERVICE_KEY_EVENTTIME "event_time"
#define CN_PROFILE_KEY_SERVICES   "services"
static cJSON *MakeService(IoTProfileService_t *serviceInfo)
{
    cJSON *root;
    cJSON *serviceID;
    cJSON *properties;
    cJSON *eventTime;

    ///< build a root node
    root = cJSON_CreateObject();
    if(NULL == root){
       goto EXIT_MEM;
    }

    ///< add the serviceID node to the root node
    serviceID = cJSON_CreateString(serviceInfo->serviceID);
    if(NULL == serviceID){
       goto EXIT_MEM;
    }
    cJSON_AddItemToObjectCS(root,CN_PROFILE_SERVICE_KEY_SERVICEID,serviceID);

    ///< add the properties node to the root
    properties = MakeKvs(serviceInfo->serviceProperty);
    if(NULL == properties){
       goto EXIT_MEM;
    }
    cJSON_AddItemToObjectCS(root,CN_PROFILE_SERVICE_KEY_PROPERTIIES,properties);

    ///< add the event time (optional) to the root
    if(NULL != serviceInfo->eventTime){
        eventTime = cJSON_CreateString(serviceInfo->eventTime);
        if(NULL == eventTime){
           goto EXIT_MEM;
        }
        cJSON_AddItemToObjectCS(root,CN_PROFILE_SERVICE_KEY_EVENTTIME,eventTime);
    }

    ///< OK, now we return it
    return root;

EXIT_MEM:
    if(NULL != root){
       cJSON_Delete(root);
       root = NULL;
    }
    return root;
}

static cJSON *MakeServices(IoTProfileService_t *serviceInfo)
{
    cJSON *services = NULL;
    cJSON *service;
    IoTProfileService_t  *serviceTmp;

    ///< create the services array node
    services = cJSON_CreateArray();
    if(NULL == services){
       goto EXIT_MEM;
    }

    serviceTmp = serviceInfo;
    while(NULL != serviceTmp){
        service = MakeService(serviceTmp);
        if(NULL == service){
            goto EXIT_MEM;
        }

        cJSON_AddItemToArray(services,service);
        serviceTmp = serviceTmp->nxt;
    }

    ///< now we return the services
    return services;

EXIT_MEM:
    if(NULL != services){
       cJSON_Delete(services);
       services = NULL;
    }
    return services;
}

///< use this function to make a topic to publish
///< if request_id  is needed depends on the fmt
static char *MakeTopic(const char *fmt, const char *device_id, const char *requestID)
{
    int len;
    char *ret = NULL;

    len = strlen(fmt) + strlen(device_id);
    if(NULL != requestID){
        len += strlen(requestID);
    }

    ret = hi_malloc(0,len);
    if(NULL != ret){
        if(NULL !=  requestID){
            (void) snprintf(ret,len,fmt,device_id,requestID);
        }
        else {
            (void) snprintf(ret,len,fmt,device_id);
        }   
    }
    return ret;
}

#define CN_PROFILE_CMDRESP_KEY_RETCODE        "result_code"
#define CN_PROFILE_CMDRESP_KEY_RESPNAME       "response_name"
#define CN_PROFILE_CMDRESP_KEY_PARAS          "paras"
static char *MakeProfileCmdResp(IoTCmdResp_t *payload)
{
    char *ret = NULL;
    cJSON *root;
    cJSON *retCode;
    cJSON *respName;
    cJSON *paras;

    ///< create the root node
    root = cJSON_CreateObject();
    if(NULL == root){
       goto EXIT_MEM;
    }

    ///< create retcode and retdesc and add it to the root
    retCode = cJSON_CreateNumber(payload->retCode);
    if(NULL == retCode){
        goto EXIT_MEM;
    }
    cJSON_AddItemToObjectCS(root,CN_PROFILE_CMDRESP_KEY_RETCODE,retCode);

    if(NULL != payload->respName){
        respName = cJSON_CreateString(payload->respName);
        if(NULL == respName){
            goto EXIT_MEM;
        }
        cJSON_AddItemToObjectCS(root,CN_PROFILE_CMDRESP_KEY_RESPNAME,respName);
    }

    if(NULL != payload->paras)
    {
        paras = MakeKvs(payload->paras);
        if(NULL == paras){
            goto EXIT_MEM;
        }
        cJSON_AddItemToObjectCS(root,CN_PROFILE_CMDRESP_KEY_PARAS,paras);
    }

    ///< OK, now we make it to a buffer
    ret = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return ret;

EXIT_MEM:
    if(NULL != root){
       cJSON_Delete(root);
    }

    return ret;
}
#define CN_PROFILE_TOPICFMT_CMDRESP   "$oc/devices/%s/sys/commands/response/request_id=%s"
int IoTProfileCmdResp(const char *deviceID,IoTCmdResp_t *payload)
{
    int ret = -1;
    const char *topic;
    const char *msg;

    if((NULL == deviceID)||(NULL == payload) || (NULL == payload->requestID)){
        return ret;
    }

    topic = MakeTopic(CN_PROFILE_TOPICFMT_CMDRESP, deviceID,payload->requestID);
    msg = MakeProfileCmdResp(payload);
    if((NULL != topic) && (NULL != msg)){
        ret = IotSendMsg(0,topic,msg);
    }

    hi_free(0,topic);
    cJSON_free(msg);

    return ret;
}

static char *MakeProfilePropertyReport(IoTProfileService_t *payload)
{
    char *ret = NULL;
    cJSON *root;
    cJSON *services;

    ///< create the root node
    root = cJSON_CreateObject();
    if(NULL == root){
       goto EXIT_MEM;
    }

    ///< create the services array node to the root
    services = MakeServices(payload);
    if(NULL == services){
        goto EXIT_MEM;
    }
    cJSON_AddItemToObjectCS(root,CN_PROFILE_KEY_SERVICES,services);

    ///< OK, now we make it to a buffer
    ret = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return ret;

EXIT_MEM:
    if(NULL != root){
       cJSON_Delete(root);
    }
    return ret;
}
#define CN_PROFILE_TOPICFMT_PROPERTYREPORT   "$oc/devices/%s/sys/properties/report"
int IoTProfilePropertyReport(char *deviceID,IoTProfileService_t *payload)
{
    int ret = -1;
    char *topic;
    char *msg;

    if((NULL == deviceID) || (NULL== payload) || (NULL== payload->serviceID) || (NULL == payload->serviceProperty)){
        return ret;
    }
    
    topic = MakeTopic(CN_PROFILE_TOPICFMT_PROPERTYREPORT, deviceID,NULL);
    msg = MakeProfilePropertyReport(payload);
    
    if((NULL != topic) && (NULL != msg)){
        ret = IotSendMsg(0,topic,msg);
    }

    hi_free(0,topic);
    cJSON_free(msg);

    return ret;
}






