/*----------------------------------------------------------------------------
* Copyright (c) 2020 HiHope Community.
* All rights reserved.
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
* 1. Redistributions of source code must retain the above copyright notice, this list of
* conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice, this list
* of conditions and the following disclaimer in the documentation and/or other materials
* provided with the distribution.
* 3. Neither the name of the copyright holder nor the names of its contributors may be used
* to endorse or promote products derived from this software without specific prior written
* permission.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

/* Link Header Files */
#include <link_service.h>
#include <link_platform.h>
#include <histreaming.h>
#include <hi_io.h>
#include <hi_early_debug.h>
#include <hi_gpio.h>
#include <hi_pwm.h>
#include <hi_task.h>
#include <hi_types_base.h>
#include <hi_time.h>
#include <c081_nfc.h>
#include <app_demo_robot_car.h>

#define HISTREAMING_TASK
#ifdef  HISTREAMING_TASK
#define HISTREAMING_DEMO_TASK_STAK_SIZE (1024*4)
#define HISTREAMING_DEMO_TASK_PRIORITY  (26)
hi_u32  g_histreaming_demo_task_id =    0;
#endif

extern hi_u8   g_car_control_mode;
extern hi_u8   g_car_speed_control;
extern hi_u16  g_car_modular_control_module;
extern hi_u16  g_car_direction_control_module;

/*
    car speed control
*/
static hi_void car_speed_control_func(char* value)
{
    if (strcmp(value, "speed_up") == 0) {
        g_car_speed_control = CAR_SPEED_UP;
    } else if(strcmp(value, "speed_reduction") == 0) {
        g_car_speed_control = CAR_SPEED_REDUCTION;
    }
}

/*
modular control func
*/
static hi_void car_modular_control_func(char* value)
{
    printf("car_modular_control_func\n");
    if (strcmp(value, "engine_left") == 0) {                          //舵机向左
        g_car_modular_control_module = CAR_CONTROL_ENGINE_LEFT_TYPE;
    } else if (strcmp(value, "engine_right") == 0) {                  //舵机向右
        g_car_modular_control_module = CAR_CONTROL_ENGINE_RIGHT_TYPE;
    } else if (strcmp(value, "engine_middle") == 0) {                 //舵机居中
        g_car_modular_control_module = CAR_CONTROL_ENGINE_MIDDLE_TYPE;
    } else if (strcmp(value, "trace_module") == 0) {                  //启动循迹模块
        g_car_modular_control_module = CAR_CONTROL_TRACE_TYPE;
    } else if (strcmp(value, "ultrasonic_module") == 0) {             //启动超声波避障模块
        g_car_modular_control_module = CAR_CONTROL_ULTRASONIC_TYPE;
    } else if (strcmp(value, "steer_engine") == 0) {                  //启动舵机超声波避障模块
        g_car_modular_control_module = CAR_CONTROL_STEER_ENGINE_TYPE;
    }
}

/*
car direction control
*/
static hi_void car_direction_control_func(char* value)
{
    printf("car_direction_control_func\n");
    if (strcmp(value, "going") == 0) {
        g_car_direction_control_module = CAR_KEEP_GOING_TYPE;
    } else if(strcmp(value, "backing") == 0) {
        g_car_direction_control_module = CAR_KEEP_GOING_BACK_TYPE;
    } else if(strcmp(value, "lefting") == 0) {
        g_car_direction_control_module = CAR_KEEP_TURN_LEFT_TYPE;
    } else if(strcmp(value, "righting") == 0) {
        g_car_direction_control_module = CAR_KEEP_TURN_RIGHT_TYPE;
    } else if(strcmp(value, "forward") == 0) {
        g_car_direction_control_module = CAR_GO_FORWARD_TYPE;
    } else if (strcmp(value, "left") == 0) {
        g_car_direction_control_module = CAR_TURN_LEFT_TYPE;
    } else if (strcmp(value,"stop") == 0) {
        g_car_direction_control_module = CAR_STOP_TYPE;
    } else if (strcmp(value,"right" ) == 0) {
        g_car_direction_control_module = CAR_TURN_RIGHT_TYPE;
    } else if (strcmp(value, "back") == 0) {
        g_car_direction_control_module = CAT_TURN_BACK_TYPE;
    }
}

static int GetStatusValue(struct LinkService* ar, const char* property, char* value, int len)
{
    (void)(ar);

    printf("Receive property: %s(value=%s[%d])\n", property, value, len);
    if (strcmp(property, "Status") == 0) {
        strcpy(value, "wgm test");
    }

    /*
     * if Ok return 0,
     * Otherwise, any error, return StatusFailure
     */
    return 0;
}
/* recv from app cmd */
static int ModifyStatus(struct LinkService* ar, const char* property, char* value, int len)
{
    (void)(ar);

    if (property == NULL || value == NULL) {
        return -1;
    }

    /* modify status property*/
    printf("Receive property: %s(value=%s[%d])\n", property, value,len);
    if (strcmp(property,"CarControl") == 0) {
        g_car_control_mode = CAR_DIRECTION_CONTROL_MODE;
        car_direction_control_func(value);
    } else if (strcmp(property, "ModularControl") == 0) {
        g_car_control_mode = CAR_MODULE_CONTROL_MODE;
        car_modular_control_func(value);
    } else if (strcmp(property, "SpeedControl") == 0) {
        g_car_control_mode = CAR_SPEED_CONTROL_MODE;
        car_speed_control_func(value);
    } 

    /*
     * if Ok return 0,
     * Otherwise, any error, return StatusFailure
     */
    return 0;
}

/*
 * It is a Wifi IoT device
 */
static const char* g_wifista_type = "Light";
static const char* GetDeviceType(struct LinkService* ar)
{
    (void)(ar);

    return g_wifista_type;
}

static void *g_link_platform = NULL;

void* histreaming_open(void )
{
    hi_u32 ret = hi_gpio_init();
    
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ===== gpio -> hi_gpio_init ret:%d\r\n", ret);
        return NULL;
    } else {
        /* code */
        printf("----- gpio init success-----\r\n");
    }
    
    LinkService* wifiIot = 0;
    LinkPlatform* link = 0;

    wifiIot = (LinkService*)malloc(sizeof(LinkService));
    if (!wifiIot) {
        printf("malloc wifiIot failure\n");
        return NULL;
    }

    wifiIot->get    = GetStatusValue;
    wifiIot->modify = ModifyStatus;
    wifiIot->type = GetDeviceType;
    
    link = LinkPlatformGet();
    if (!link) {
        printf("get link failure\n");
        return NULL;
    }

    if (link->addLinkService(link, wifiIot, 1) != 0) {
        histreaming_close(link);
        return NULL;
    }

    if (link->open(link) != 0) {
        histreaming_close(link);
        return NULL;
    }

    /* cache link ptr*/
    g_link_platform = (void*)(link);
#ifdef HISTREAMING_TASK    
    hi_task_delete(g_histreaming_demo_task_id);
#endif
    return (void*)link;
}

void histreaming_close(void *link)
{
    LinkPlatform *linkPlatform = (LinkPlatform*)(link);
    if (!linkPlatform) {
        return;
    }

    linkPlatform->close(linkPlatform);

    if (linkPlatform != NULL) {
        LinkPlatformFree(linkPlatform);
    }
}

#ifdef HISTREAMING_TASK 

hi_void histreaming_demo(hi_void)
{
    hi_u32 ret = 0;
    hi_task_attr histreaming ={0};
    histreaming.stack_size = HISTREAMING_DEMO_TASK_STAK_SIZE;
    histreaming.task_prio = HISTREAMING_DEMO_TASK_PRIORITY;
    histreaming.task_name = "histreaming_demo";
    ret = hi_task_create(&g_histreaming_demo_task_id, &histreaming, histreaming_open, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("Falied to create histreaming demo task!\n");
    }
}

#endif
