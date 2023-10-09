/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT platform
 * Author: HiSpark Product Team.
 * Create: 2020-5-20
 */
#ifndef IOT_MAIN_H_
#define IOT_MAIN_H_
#include <stdint.h>
#include <stddef.h>

typedef void  (*fnMsgCallBack)(int qos, const char *topic, const char *payload);

/**
 * This is the iot main function. Please call this function first
 * 
*/
int IoTMain(void);


/**
 * Use this function to set the message call back function, when some messages comes, 
 * the callback will be called, if you don't care about the message, set it to NULL
*/
int IoTSetMsgCallback(fnMsgCallBack msgCallback);

/**
 * When you want to send some messages to the iot server(including the response message),
 * please call this api
 * @param qos: the mqtt qos,:0,1,2
 * @param topic: the iot mqtt topic
 * @param payload: the mqtt payload
 * 
 * @return 0 success while others failed
 * 
 * @instruction: if success means we write the message to the queue susccess, not means communicate with the server success
*/
int IotSendMsg(int qos, const char *topic, const char *payload);

#endif /* IOT_MAIN_H_ */