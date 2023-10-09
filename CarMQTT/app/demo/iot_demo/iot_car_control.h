#ifndef IOT_CAR_CTRL_H_
#define IOT_CAR_CTRL_H_

#include "iot_config.h"
#include "iot_log.h"
#include "iot_main.h"
#include "iot_profile.h"
#include <hi_task.h>
#include <string.h>
#include <hi_wifi_api.h>
#include <hi_mux.h>
#include <car_control.h>
#include <app_demo_robot_car.h>
#include "strings.h"
#define CAR_CTRL_CMD_SERVICE_ID              "CAR_CTRL"
#define CAR_CTRL_CMD_NAME                    "CAR_CONTROL"
#define CAR_CTRL_GO_FORWARD                  "GO_FORWARD"
#define CAR_CTRL_GO_BACK                     "GO_BACK"
#define CAR_CTRL_STOP                        "STOP"
#define CAR_CTRL_TURN_LEFT                   "TURN_LEFT"
#define CAR_CTRL_TURN_RIGHT                  "TURN_RIGHT"
#define CAR_CTRL_SPPEED_UP                   "SPEED_UP"
#define CAR_CTRL_TRACE_ON                    "TRACE_ON"
#define CAR_CTRL_STEER_ON                    "STEER_ON"
#define CAR_CTRL_DODGE                       "CAR_AWAY"
#define CAR_CTRL_TRACE_FORWARD               "TRACE_FORWARD"
#define CAR_CTRL_LIGHT                       "TRAFFIC_LIGHT" 
#define CAR_CTRL_RED_LIGHT_ON                "RED_LIGHT_ON"    
#define CAR_CTRL_YELLOW_LIGHT_ON             "YELLOW_LIGHT_ON"

#define DURATION_FOREVER                      0

#define CAR_TRACE_CMD_NAME                    "TRACE"
int itoa_key(char *key);
int ValofJson(char* jStr);
int MQTT_car_ctrl(int qos, const char *topic, const char *payload);


#endif