/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT Platform 
 * Author: HiSpark Product Team.
 * Create: 2020-6-20
 */

#ifndef APP_DEMO_IOT_H_
#define APP_DEMO_IOT_H_
/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT Platform
 * Author: HiSpark Product Team.
 * Create: 2020-5-20
 */

#include "iot_config.h"
#include "iot_log.h"
#include "iot_main.h"
#include "iot_profile.h"
#include <hi_task.h>
#include <string.h>
#include <hi_wifi_api.h>
#include <hi_mux.h>

/*attribute initiative to report */
#define TAKE_THE_INITIATIVE_TO_REPORT
/*oc request id*/
#define CN_COMMADN_INDEX                    "commands/request_id="

int app_demo_iot(void);

#endif