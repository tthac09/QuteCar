/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT platform
 * Author: HiSpark Product Team.
 * Create: 2020-5-20
 */

/* we use the mqtt to connect to the huawei IoT platform*/
/**
 * STEPS:
 * 1, CONNECT TO THE IOT SERVER
 * 2, SUBSCRIBE  THE DEFAULT TOPIC
 * 3, WAIT FOR ANY MESSAGE COMES OR ANY MESSAGE TO SEND
 * 
*/
#include "iot_config.h"
#include "iot_log.h"
#include "iot_main.h"
#include "iot_hmac.h"
#include <securec.h>
#include <hi_task.h>
#include <hi_msg.h>
#include <hi_mem.h>
#include <MQTTClient.h>
#include <string.h>
#include <hi_mux.h>
#include <hi_wifi_api.h>

// extern hi_u8 wifi_status;
extern hi_u8 wifi_first_connecting;
extern hi_u8 wifi_second_connecting;
extern hi_u8 wifi_second_connected;
hi_bool mqtt_connect_success = HI_FALSE;
///< this is the configuration head
#define CN_IOT_SERVER ".st1.iotda-device.cn-north-4.myhuaweicloud.com:"

#define CONFIG_COMMAND_TIMEOUT 10000L
#define CN_KEEPALIVE_TIME 50
#define CN_CLEANSESSION 1
#define CN_HMAC_PWD_LEN 65 ///< SHA256 IS 32 BYTES AND END APPEND '\0'
#define CN_EVENT_TIME "1970000100"
#define CN_CLIENTID_FMT "%s_0_0_%s" ///< This is the cient ID format, deviceID_0_0_TIME
#define CN_QUEUE_WAITTIMEOUT 5000
#define CN_QUEUE_MSGNUM 16
#define CN_QUEUE_MSGSIZE (sizeof(hi_pvoid))

#define CN_TASK_PRIOR 28
#define CN_TASK_STACKSIZE 0X2000
#define CN_TASK_NAME "IoTMain"

typedef enum
{
    EN_IOT_MSG_PUBLISH = 0,
    EN_IOT_MSG_RECV,
} en_iot_msg_t;

typedef struct
{
    en_iot_msg_t type;
    int qos;
    const char *topic;
    const char *payload;
} IoTMsg_t;

typedef struct
{
    hi_bool stop;
    hi_u32 conLost;
    hi_u32 queueID;
    hi_u32 iotTaskID;
    fnMsgCallBack msgCallBack;
    MQTTClient_deliveryToken tocken;
} IotAppCb_t;
static IotAppCb_t gIoTAppCb;

static const char *gDefaultSubscribeTopic[] = {
    "$oc/devices/" CONFIG_DEVICE_ID "/sys/messages/down",
    "$oc/devices/" CONFIG_DEVICE_ID "/sys/properties/set/#",
    "$oc/devices/" CONFIG_DEVICE_ID "/sys/properties/get/#",
    "$oc/devices/" CONFIG_DEVICE_ID "/sys/shadow/get/response/#",
    "$oc/devices/" CONFIG_DEVICE_ID "/sys/events/down",
    "$oc/devices/" CONFIG_DEVICE_ID "/sys/commands/#"};
#define CN_TOPIC_SUBSCRIBE_NUM (sizeof(gDefaultSubscribeTopic) / sizeof(const char *))

static int MsgRcvCallBack(void *context, char *topic, int topicLen, MQTTClient_message *message)
{
    IoTMsg_t *msg;
    char *buf;
    hi_u32 bufSize;

    if (topicLen == 0)
    {
        topicLen = strlen(topic);
    }
    bufSize = topicLen + 1 + message->payloadlen + 1 + sizeof(IoTMsg_t);
    buf = hi_malloc(0, bufSize);
    if (buf != NULL)
    {
        msg = (IoTMsg_t *)buf;
        buf += sizeof(IoTMsg_t);
        bufSize -= sizeof(IoTMsg_t);
        msg->qos = message->qos;
        msg->type = EN_IOT_MSG_RECV;
        (void)memcpy_s(buf, bufSize, topic, topicLen);
        buf[topicLen] = '\0';
        msg->topic = buf;
        buf += topicLen + 1;
        bufSize -= (topicLen + 1);
        (void)memcpy_s(buf, bufSize, message->payload, message->payloadlen);
        buf[message->payloadlen] = '\0';
        msg->payload = buf;
        // IOT_LOG_DEBUG("RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n",msg->qos,msg->topic,msg->payload);
        if (HI_ERR_SUCCESS != hi_msg_queue_send(gIoTAppCb.queueID, &msg, CN_QUEUE_WAITTIMEOUT, sizeof(hi_pvoid)))
        {
            IOT_LOG_ERROR("========MsgRcvCallBack Wrie queue failed==========\r\n");
            hi_free(0, msg);
        }
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic);

    return 1;
}

///< when the connect lost and this callback will be called
static void ConnLostCallBack(void *context, char *cause)
{
    IOT_LOG_DEBUG("Connection lost:caused by:%s\r\n", cause == NULL ? "Unknown" : cause);
    return;
}

///<use this function to deal all the comming message
static int ProcessQueueMsg(MQTTClient client)
{
    hi_u32 ret;
    hi_u32 msgSize;
    IoTMsg_t *msg;
    hi_u32 timeout;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;

    timeout = CN_QUEUE_WAITTIMEOUT;
    do
    {
        msg = NULL;
        msgSize = sizeof(hi_pvoid);
        ret = hi_msg_queue_wait(gIoTAppCb.queueID, &msg, timeout, &msgSize);
        if (msg != NULL)
        {
            // IOT_LOG_DEBUG("QUEUEMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n",msg->qos,msg->topic,msg->payload);
            switch (msg->type)
            {
            case EN_IOT_MSG_PUBLISH:
                pubmsg.payload = (void *)msg->payload;
                pubmsg.payloadlen = (int)strlen(msg->payload);
                pubmsg.qos = msg->qos;
                pubmsg.retained = 0;
                ret = MQTTClient_publishMessage(client, msg->topic, &pubmsg, &gIoTAppCb.tocken);
                if (ret != MQTTCLIENT_SUCCESS)
                {
                    IOT_LOG_ERROR("MSGSEND:failed\r\n");
                }
                IOT_LOG_DEBUG("MSGSEND:SUCCESS\r\n");
                gIoTAppCb.tocken++;
                break;
            case EN_IOT_MSG_RECV:
                if (gIoTAppCb.msgCallBack != NULL)
                {
                    printf("=========deal the comming message=======\n");
                    gIoTAppCb.msgCallBack(msg->qos, msg->topic, msg->payload);
                }
                break;
            default:
                break;
            }
            hi_free(0, msg);
        }
        timeout = 0; ///< continous to deal the message without wait here
    } while (ret == HI_ERR_SUCCESS);

    return 0;
}

static hi_void MainEntryProcess(hi_void)
{
    int subQos[CN_TOPIC_SUBSCRIBE_NUM] = {1};
    int rc;
    char *clientID = NULL;
    char *userID = NULL;
    char *userPwd = NULL;

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
#ifdef CONFIG_MQTT_SSL
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    cert_string trustStore = {(const unsigned char *)gIotCA, sizeof(gIotCA)};
#ifdef CONFIG_MQTT_SSL_X509
    cert_string keyStore = {(const unsigned char *)gDeviceCA, sizeof(gDeviceCA)};
    key_string privateKey = {(const unsigned char *)gDevicePK, sizeof(gDevicePK)};
    ssl_opts.los_keyStore = &keyStore;
    ssl_opts.los_privateKey = &privateKey;
    ssl_opts.privateKeyPassword = gDevicePKPwd;
#endif
    ssl_opts.los_trustStore = &trustStore;
    conn_opts.ssl = &ssl_opts;
#endif

    IOT_LOG_DEBUG("IoT machine start here\r\n");
    ///< make the clientID userID userPwd
    clientID = hi_malloc(0, strlen(CN_CLIENTID_FMT) + strlen(CONFIG_DEVICE_ID) + strlen(CN_EVENT_TIME) + 1);
    if (NULL == clientID)
    {
        IOT_LOG_ERROR("No memory for the clientID\r\n");
        goto EXIT_CLIENTID;
    }
    (void)snprintf(clientID, strlen(CN_CLIENTID_FMT) + strlen(CONFIG_DEVICE_ID) + strlen(CN_EVENT_TIME) + 1,
                   CN_CLIENTID_FMT, CONFIG_DEVICE_ID, CN_EVENT_TIME);
    userID = CONFIG_DEVICE_ID;
    if (NULL != CONFIG_DEVICE_PWD)
    {
        userPwd = hi_malloc(0, CN_HMAC_PWD_LEN);
        if (NULL == userPwd)
        {
            IOT_LOG_ERROR("No memory for the user passwd\r\n");
            goto EXIT_USERPWD;
        }
        (void)HmacGeneratePwd((const unsigned char *)CONFIG_DEVICE_PWD, strlen(CONFIG_DEVICE_PWD),
                              (const unsigned char *)CN_EVENT_TIME, strlen(CN_EVENT_TIME),
                              (unsigned char *)userPwd, CN_HMAC_PWD_LEN);
    }

    conn_opts.keepAliveInterval = CN_KEEPALIVE_TIME;
    conn_opts.cleansession = CN_CLEANSESSION;
    conn_opts.username = userID;
    conn_opts.password = userPwd;
    conn_opts.MQTTVersion = MQTTVERSION_3_1_1;

    ///< wait for the wifi connect ok
    IOT_LOG_DEBUG("IOTSERVER:%s\r\n", CN_IOT_SERVER);
    IOT_LOG_DEBUG("CLIENTID:%s USERID:%s USERPWD:%s\r\n", clientID, userID, userPwd == NULL ? "NULL" : userPwd);
    /*check wifi */

    if (wifi_second_connected)
    {
        IOT_LOG_ERROR("Wifi disconnect, Please Check...%d\r\n", rc);
        goto EXIT_CREATE;
    }
    rc = MQTTClient_create(&client, CN_IOT_SERVER, clientID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        IOT_LOG_ERROR("Create Client failed,Please check the parameters--%d\r\n", rc);
        goto EXIT_CREATE;
    }

    rc = MQTTClient_setCallbacks(client, NULL, ConnLostCallBack, MsgRcvCallBack, NULL);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        IOT_LOG_ERROR("Set the callback failed,Please check the callback paras\r\n");
        goto EXIT_SETCALLBACK;
    }

    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        IOT_LOG_ERROR("Connect IoT server failed,please check the network and parameters:%d\r\n", rc);
        goto EXIT_CONNECT;
    }
    IOT_LOG_DEBUG("Connect success\r\n");

    rc = MQTTClient_subscribeMany(client, CN_TOPIC_SUBSCRIBE_NUM, (char *const *)gDefaultSubscribeTopic, (int *)&subQos[0]);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        IOT_LOG_ERROR("Subscribe the default topic failed,Please check the parameters\r\n");
        goto EXIT_SUBSCRIBE;
    }
    IOT_LOG_DEBUG("Subscribe success\r\n");
    mqtt_connect_success = HI_TRUE;
    while (MQTTClient_isConnected(client))
    {
        printf("=========ProcessQueueMsg=========\n");
        ProcessQueueMsg(client); ///< do the job here
        MQTTClient_yield();      ///< make the keepalive done
    }
    IOT_LOG_ERROR("disconnect and begin to quit\r\n");
    MQTTClient_disconnect(client, CONFIG_COMMAND_TIMEOUT);

EXIT_SUBSCRIBE:
EXIT_CONNECT:
EXIT_SETCALLBACK:
    MQTTClient_destroy(&client);
EXIT_CREATE:
    if (NULL != userPwd)
    {
        hi_free(0, userPwd);
    }
EXIT_USERPWD:
    hi_free(0, clientID);
EXIT_CLIENTID:
    return;
}

static hi_void *MainEntry(hi_void *arg)
{
    while (gIoTAppCb.stop == HI_FALSE)
    {
        printf("=========before MainEntryProcess========\n");
        MainEntryProcess();
        IOT_LOG_DEBUG("The connection lost and we will try another connect\r\n");
        hi_sleep(1000 * 5);
    }
    return NULL;
}

int IoTMain(void)
{
    hi_u32 ret;
    hi_task_attr attr = {0};

    ret = hi_msg_queue_create(&gIoTAppCb.queueID, CN_QUEUE_MSGNUM, CN_QUEUE_MSGSIZE);
    if (ret != HI_ERR_SUCCESS)
    {
        IOT_LOG_ERROR("Create the msg queue Failed\r\n");
    }

    attr.stack_size = CN_TASK_STACKSIZE;
    attr.task_prio = CN_TASK_PRIOR;
    attr.task_name = CN_TASK_NAME;
    printf("=======before MainEntry===========\n");
    ret = hi_task_create(&gIoTAppCb.iotTaskID, &attr, MainEntry, NULL);
    if (ret != HI_ERR_SUCCESS)
    {
        IOT_LOG_ERROR("Create the Main Entry Failed\r\n");
    }

    return 0;
}

int IoTSetMsgCallback(fnMsgCallBack msgCallback)
{
    gIoTAppCb.msgCallBack = msgCallback;
    return 0;
}

int IotSendMsg(int qos, const char *topic, const char *payload)
{
    int rc = -1;
    IoTMsg_t *msg;
    char *buf;
    hi_u32 bufSize;

    bufSize = strlen(topic) + 1 + strlen(payload) + 1 + sizeof(IoTMsg_t);
    buf = hi_malloc(0, bufSize);
    if (buf != NULL)
    {
        msg = (IoTMsg_t *)buf;
        buf += sizeof(IoTMsg_t);
        bufSize -= sizeof(IoTMsg_t);
        msg->qos = qos;
        msg->type = EN_IOT_MSG_PUBLISH;
        (void)memcpy_s(buf, bufSize, topic, strlen(topic));
        buf[strlen(topic)] = '\0';
        msg->topic = buf;
        buf += strlen(topic) + 1;
        bufSize -= (strlen(topic) + 1);
        (void)memcpy_s(buf, bufSize, payload, strlen(payload));
        buf[strlen(payload)] = '\0';
        msg->payload = buf;
        // IOT_LOG_DEBUG("SNDMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n",msg->qos,msg->topic,msg->payload);

        if (HI_ERR_SUCCESS != hi_msg_queue_send(gIoTAppCb.queueID, &msg, CN_QUEUE_WAITTIMEOUT, sizeof(hi_pvoid)))
        {
            IOT_LOG_ERROR("=============IotSendMsg Wrie queue failed==============\r\n");
            hi_free(0, msg);
        }
        else
        {
            rc = 0;
        }
    }
    return rc;
}
