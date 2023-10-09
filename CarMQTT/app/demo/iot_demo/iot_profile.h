/*
 * Copyright (c) 2020 HiHope Community.
 * Description:IoT platform
 * Author: HiSpark Product Team.
 * Create: 2020-5-20
 */
#ifndef IOT_PROFILE_H_
#define IOT_PROFILE_H_

#include <hi_types_base.h>

#define OC_BEEP_STATUS_ON       ((hi_u8) 0x01)
#define OC_BEEP_STATUS_OFF      ((hi_u8) 0x00)

////< enum all the data type for the oc profile
typedef enum
{
    EN_IOT_DATATYPE_INT = 0,
    EN_IOT_DATATYPE_LONG,
    EN_IOT_DATATYPE_FLOAT,
    EN_IOT_DATATYPE_DOUBLE,
    EN_IOT_DATATYPE_STRING,           ///< must be ended with '\0'
    EN_IOT_DATATYPE_LAST,
}IoTDataType_t;

typedef enum {
    OC_LED_ON =1,
    OC_LED_OFF
}oc_led_value;

typedef struct
{
    void                            *nxt;   ///< ponit to the next key
    const char                      *key;
    const char                      *value;
    hi_u32                          i_value;
    // hi_float                       oc_evvironment_value;
    IoTDataType_t                   type;
}IoTProfileKV_t;

typedef struct
{
   void *nxt;
   char *serviceID;                  ///< the service id in the profile, which could not be NULL
   char *eventTime;                  ///< eventtime, which could be NULL means use the platform time
   IoTProfileKV_t *serviceProperty; ///< the property in the profile, which could not be NULL
}IoTProfileService_t;

typedef struct
{
    int  retCode;           ///< response code, 0 success while others failed
    const char   *respName; ///< response name
    const char   *requestID;///< specified by the message command
    IoTProfileKV_t  *paras;  ///< the command paras
}IoTCmdResp_t;
/**
 * Use this function to make the command response here
 * and you must supplied the device id, and the payload defines as IoTCmdResp_t
 * 
*/
int IoTProfileCmdResp(const char *deviceid,IoTCmdResp_t *payload);

/**
 * use this function to report the property to the iot platform
 * 
*/
int IoTProfilePropertyReport(char *deviceID,IoTProfileService_t *payload);
/**/

#endif