#include "iot_car_control.h"

#define BEGIN_SEPRATOR ':'
#define END_SEPRATOR '}'
#define DURATION_PARAM "DURATION"

extern hi_u8 g_car_control_mode;
extern hi_u8 g_car_modular_control_module;
extern hi_u32 g_car_status;
extern hi_u16 g_car_direction_control_module;

hi_u16 global_red_on = HI_FALSE;

int MQTT_car_ctrl(int qos, const char *topic, const char *payload)
{
    char *tmp = NULL;
    IOT_LOG_DEBUG("CAR CTRL RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n", qos, topic, payload);
    /*app 下发的操作*/
    printf("=============   val:%d  \n", strstr(payload, CAR_CTRL_GO_FORWARD));
    printf("=============   val:%d  \n", strstr(payload, CAR_CTRL_TRACE_ON));
    if (strstr(payload, CAR_CTRL_CMD_SERVICE_ID) != NULL)
    { //小车控制
        if ((tmp = strstr(payload, CAR_CTRL_GO_FORWARD)) != NULL)
        { //前进
            printf("============forward============\n");
            g_car_control_mode = CAR_DIRECTION_CONTROL_MODE;
            g_car_direction_control_module = CAR_STOP_TYPE; //先停止，切换模式
            g_car_status = CAR_STOP_STATUS;
            hi_sleep(500);
            int val = ValofJson(payload);
            car_go_forward();
            hi_sleep(val);
            car_stop();
        }
        else if ((tmp = strstr(payload, CAR_CTRL_GO_BACK)) != NULL)
        { //后退
            int val = ValofJson(payload);
            car_go_back();
            hi_sleep(val);
            car_stop();
        }
        else if ((tmp = strstr(payload, CAR_CTRL_TURN_LEFT)) != NULL)
        { //左转
            int val = ValofJson(payload);
            car_turn_left();
            hi_sleep(val);
            car_stop();
        }
        else if ((tmp = strstr(payload, CAR_CTRL_TURN_RIGHT)) != NULL)
        { //右转
            int val = ValofJson(payload);
            car_turn_right();
            hi_sleep(val);
            car_stop();
        }
        else if ((tmp = strstr(payload, CAR_CTRL_STOP)) != NULL)
        { //停止
            // int val = ValofJson(payload);
            // car_stop();
            // hi_sleep(val);
            g_car_control_mode = CAR_DIRECTION_CONTROL_MODE;
            g_car_direction_control_module = CAR_STOP_TYPE; //停止
            g_car_status = CAR_STOP_STATUS;
        }
        else if ((tmp = strstr(payload, CAR_CTRL_SPPEED_UP)) != NULL)
        { //前进
            car_speed_up();
        }
        else if ((tmp = strstr(payload, CAR_CTRL_TRACE_ON)) != NULL)
        {
            printf("=============TRACE_ON=============\n");
            g_car_control_mode = CAR_MODULE_CONTROL_MODE;
            g_car_modular_control_module = CAR_CONTROL_TRACE_TYPE; //开始寻迹
            g_car_status = CAR_TRACE_STATUS;
        }
        else if ((tmp = strstr(payload, CAR_CTRL_STEER_ON)) != NULL)
        {
            printf("=============STEER_ON=============\n");
            g_car_control_mode = CAR_MODULE_CONTROL_MODE;
            g_car_modular_control_module = CAR_CONTROL_STEER_ENGINE_TYPE; //开始超声波
            g_car_status = CAR_RUNNING_STATUS;
        }
        else if ((tmp = strstr(payload, CAR_CTRL_DODGE)) != NULL)
        {
            printf("=============CAR_AWAY=============\n");
            int val = ValofJson(payload);
            car_turn_left();
            hi_sleep(val);
            car_stop();
            car_go_forward();
            hi_sleep(val);
            car_stop();
        }
        else if ((tmp = strstr(payload, CAR_CTRL_TRACE_FORWARD)) != NULL)
        {
            g_car_control_mode = CAR_DIRECTION_CONTROL_MODE;
            g_car_direction_control_module = CAR_STOP_TYPE; //先停止，切换模式
            g_car_status = CAR_STOP_STATUS;
            hi_sleep(100);
            int val = ValofJson(payload);
            car_go_forward();
            hi_sleep(val);
            g_car_control_mode = CAR_MODULE_CONTROL_MODE;
            g_car_modular_control_module = CAR_CONTROL_TRACE_TYPE; //开始寻迹
            g_car_status = CAR_TRACE_STATUS;
        }
        else if ((tmp = strstr(payload, CAR_CTRL_LIGHT)) != NULL)
        { //判断红绿灯
            if ((tmp = strstr(payload, CAR_CTRL_RED_LIGHT_ON)) != NULL || (tmp = strstr(payload, CAR_CTRL_YELLOW_LIGHT_ON)) != NULL)
            {
                global_red_on = HI_TRUE;
            }
            else
            {
                global_red_on = HI_FALSE;
            }
        }
        else if (strstr(payload, CAR_TRACE_CMD_NAME) != NULL)
        { //Auto module
            IOT_LOG_DEBUG("CAR TRACE RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n", qos, topic, payload);
        }
    }
    return 0;
}

int ValofJson(char *jStr) // 查找整型数据
{
    char *tmp = NULL;
    int i = 0;
    if ((tmp = strstr(jStr, DURATION_PARAM)) == NULL)
        return 0;

    while (*tmp++ != BEGIN_SEPRATOR)
        ;
    while (tmp[i++] != END_SEPRATOR)
        ;
    if (i > 0)
        tmp[i - 1] = 0;
    // IOT_LOG_DEBUG("pos is %d,TMP is %s\n",i,tmp);
    int val = itoa_key(tmp);
    return val;
}

int itoa_key(char *key)
{
    int val = 0;
    int i = 0;
    // IOT_LOG_DEBUG("itoa val  is %d,key is %s\n",val,key);
    while (key[i] != 0)
    {
        val = val * 10 + (key[i++] - '0');
    }
    return val;
}
