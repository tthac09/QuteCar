#include <app_demo_robot_car.h>
#include <hi_timer.h>

hi_u32  g_gpio5_tick = 0;
hi_u8   g_car_control_mode = 0;
hi_u8   g_car_speed_control = 0;
hi_u32  g_car_control_demo_task_id = 0;
hi_u8   g_car_status = CAR_STOP_STATUS;
hi_u16  g_car_modular_control_module = 0;
hi_u16  g_car_direction_control_module = 0;
hi_u32  g_car_speed = 1000;

extern hi_void engine_turn_left(hi_void);
extern hi_void engine_turn_right(hi_void);
extern hi_void regress_middle(hi_void);
extern hi_void trace_module(hi_void);
extern hi_void car_speed_up(hi_void);
extern hi_void car_speed_reduction(hi_void);
extern hi_float car_get_distance(hi_void);
extern hi_u8 get_gpio5_voltage(hi_void);
/* gpio key init*/
hi_void gpio5_switch_init(hi_void)
{
    hi_io_set_func(HI_IO_NAME_GPIO_5, HI_IO_FUNC_GPIO_5_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_5, HI_GPIO_DIR_IN);
    hi_io_set_pull(HI_IO_NAME_GPIO_5,HI_IO_PULL_UP);

    hi_io_set_func(HI_IO_NAME_GPIO_11, HI_IO_FUNC_GPIO_5_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_11, HI_GPIO_DIR_IN);
    hi_io_set_pull(HI_IO_NAME_GPIO_11,HI_IO_PULL_UP);

    hi_io_set_func(HI_IO_NAME_GPIO_12, HI_IO_FUNC_GPIO_5_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_12, HI_GPIO_DIR_IN);
    hi_io_set_pull(HI_IO_NAME_GPIO_12,HI_IO_PULL_UP);
}

/*Judge steering gear*/
static hi_u32 engine_go_where(hi_void)
{
    hi_float left_distance = 0;
    hi_float right_distance = 0;
    /*舵机往左转动测量左边障碍物的距离*/

    engine_turn_left();
    hi_sleep(100);
    left_distance = car_get_distance();
    hi_sleep(100);

    /*归中*/
    regress_middle();
    hi_sleep(100);

    /*舵机往右转动测量右边障碍物的距离*/
    engine_turn_right();
    hi_sleep(100);
    right_distance = car_get_distance();
    hi_sleep(100);

    /*归中*/
    regress_middle();
    
    if (left_distance > right_distance) {
        return CAR_TURN_LEFT;
    } else {
        return CAR_TURN_RIGHT;
    }
}

/*根据障碍物的距离来判断小车的行走方向
1、距离大于等于10cm继续前进
2、距离小于10cm，先停止再后退0.1s,再往左转0.2s后再继续进行测距,再进行判断
*/
/*Judge the direction of the car*/
static hi_void car_where_to_go(hi_float distance)
{
    if (g_car_modular_control_module == CAR_CONTROL_ULTRASONIC_TYPE) {
        if (distance < DISTANCE_BETWEEN_CAR_AND_OBSTACLE) {
            car_stop();
            car_go_back();
            hi_sleep(500);
            car_stop();
            car_turn_left();
            hi_sleep(500);
            car_stop();
        } else {
            car_go_forward();
        }
    }  else if (g_car_modular_control_module == CAR_CONTROL_STEER_ENGINE_TYPE) {
        if (distance < DISTANCE_BETWEEN_CAR_AND_OBSTACLE) {
            car_stop();
            hi_sleep(200);
            printf("==========the car ahead still there========\n");
            //car_go_back();
            //hi_sleep(500);
            //car_stop();
            // hi_u32 ret = engine_go_where();
            // if (ret == CAR_TURN_LEFT) {
            //     car_turn_left();
            //     hi_sleep(500);
            // } else if (ret == CAR_TURN_RIGHT) {
            //     car_turn_right();
            //     hi_sleep(500);
            // }
            // car_stop();
        } else {
            //car_go_forward();
            printf("==============  switch to trace  =========\n");
            g_car_control_mode = CAR_MODULE_CONTROL_MODE;
            g_car_modular_control_module = CAR_CONTROL_TRACE_TYPE;                  //寻迹 
            g_car_status = CAR_TRACE_STATUS;
        } 
    }
}

/*car mode control func*/
static hi_void car_mode_control_func(hi_void)
{
    hi_u16 current_car_modular_control_module = g_car_modular_control_module;
    hi_u16 current_car_control_mode = g_car_control_mode;
    hi_float m_distance = 0.0;
    //regress_middle();

    switch (g_car_modular_control_module) {
        case CAR_CONTROL_ULTRASONIC_TYPE://无舵机超声波
            while (1) {
                if ((current_car_modular_control_module != g_car_modular_control_module) 
                    || (current_car_control_mode != g_car_control_mode) ) {
                    printf("car_mode_control_func 0 module changed\n");
                    regress_middle();
                    break;
                }

                /*获取前方物体的距离*/
                m_distance = car_get_distance();
                //printf("m_distance = %lf\n",m_distance);
                car_where_to_go(m_distance);
                hi_watchdog_feed();
                hi_sleep(20);
            }
            break;
        case CAR_CONTROL_STEER_ENGINE_TYPE: //有舵机      <==================do this
            while (1) {

                /*获取前方物体的距离*/
                m_distance = car_get_distance();
                printf("++++++++++++++++++++m_distance = %lf\n",m_distance);
                car_where_to_go(m_distance);
                hi_watchdog_feed();
                hi_sleep(20);

                if ((current_car_modular_control_module != g_car_modular_control_module) 
                    || (current_car_control_mode != g_car_control_mode)) {
                    printf("car_mode_control_func 1 module changed\n");
                    //regress_middle();
                    break;
                }
            }
            break;
        default:
            break;
    }
}

static hi_void car_speed_control_func(hi_void)
{
    switch (g_car_speed_control) {
        case CAR_SPEED_UP:
            car_speed_up();
            break;
        case CAR_SPEED_REDUCTION:
            car_speed_reduction();
            break;
        default:
            break;
    }
    g_car_speed_control = 0;
}

static hi_void car_module_control_func(hi_void)
{
    switch (g_car_modular_control_module) {
        case CAR_CONTROL_ENGINE_LEFT_TYPE:
            engine_turn_left();
            break;
        case CAR_CONTROL_ENGINE_RIGHT_TYPE:
            engine_turn_right();
            break;
        case CAR_CONTROL_ENGINE_MIDDLE_TYPE:
            regress_middle();
            break;
        case CAR_CONTROL_TRACE_TYPE:
            //printf("________zyh(*** %s ***)____________\n", __func__);
            printf("======before trace_module========\n");
            trace_module();
            break;
        case CAR_CONTROL_ULTRASONIC_TYPE:
            car_mode_control_func();
            break;
        case CAR_CONTROL_STEER_ENGINE_TYPE:
            printf("======before STEER_module========\n");
            car_mode_control_func();
            break;
        default:
            break;
    }
    //g_car_modular_control_module = 0;
}

static hi_void car_direction_control_func(hi_void)
{
    switch (g_car_direction_control_module) {
        case CAR_KEEP_GOING_TYPE:      //一直往前
            car_go_forward();
            break;
        case CAR_KEEP_GOING_BACK_TYPE: //一直往后
            car_go_back();
            break;
        case CAR_KEEP_TURN_LEFT_TYPE:  //一直往左
            car_turn_left();
            break;
        case CAR_KEEP_TURN_RIGHT_TYPE: //一直往右
            car_turn_right();
            break;
        case CAR_GO_FORWARD_TYPE:      //车子向前0.5s
            car_go_forward();
            hi_sleep(200);
            car_stop();
            break;
        case CAR_TURN_LEFT_TYPE:       //车子向左0.5s
            car_turn_left();
            hi_sleep(200);
            car_stop();
            break;
        case CAR_STOP_TYPE:            //车子停止
            car_stop();
            break;
        case CAR_TURN_RIGHT_TYPE:      //车子向右0.5s
            car_turn_right();
            hi_sleep(200);
            car_stop();
            break;
        case CAT_TURN_BACK_TYPE:       //车子向后0.5s
            car_go_back();
            hi_sleep(200);
            car_stop();
            break;
        default:
            break;
    }
    g_car_direction_control_module = 0;
}

hi_void *robot_car_demo(hi_void* param)
{
    while (1) {
        switch (g_car_control_mode) {
            case CAR_DIRECTION_CONTROL_MODE:
                car_direction_control_func();
                break;
            case CAR_MODULE_CONTROL_MODE:
                printf("======before car_module_control_func=======\n");
                car_module_control_func();
                break;
            case CAR_SPEED_CONTROL_MODE:
                car_speed_control_func();
            default:
                break;
        }
        hi_watchdog_feed();
        hi_sleep(20);
    }
}

//按键中断响应函数
hi_void gpio5_isr_func_mode(hi_void)
{
    printf("gpio5_isr_func_mode start\n");
    hi_u32 tick_interval = 0;
    hi_u32 current_gpio5_tick = 0; 

    current_gpio5_tick = hi_get_tick();
    tick_interval = current_gpio5_tick - g_gpio5_tick;
    
    if (tick_interval < KEY_INTERRUPT_PROTECT_TIME) {  
        return HI_NULL;
    }
    g_gpio5_tick = current_gpio5_tick;

    /*if (g_car_status == CAR_STOP_STATUS) {
        g_car_control_mode = CAR_MODULE_CONTROL_MODE;
        g_car_modular_control_module = CAR_CONTROL_STEER_ENGINE_TYPE;
        g_car_status = CAR_RUNNING_STATUS;
        printf("_______1______\n");
    } else if (g_car_status == CAR_RUNNING_STATUS) {
        g_car_control_mode = CAR_DIRECTION_CONTROL_MODE;
        g_car_direction_control_module = CAR_STOP_TYPE;
        g_car_status = CAR_STOP_STATUS;
        printf("_______12_____\n");
    }*/

    // 按顺序状态切换
    if (g_car_status == CAR_STOP_STATUS) {
        g_car_control_mode = CAR_MODULE_CONTROL_MODE;
        g_car_modular_control_module = CAR_CONTROL_TRACE_TYPE;                  //寻迹 
        g_car_status = CAR_TRACE_STATUS;
        printf("_______1______\n");
    } else if (g_car_status == CAR_TRACE_STATUS) {
        g_car_control_mode = CAR_MODULE_CONTROL_MODE;
        g_car_modular_control_module = CAR_CONTROL_STEER_ENGINE_TYPE;           //超声波
        g_car_status = CAR_RUNNING_STATUS;
        printf("_______2______\n");
    } else if (g_car_status == CAR_RUNNING_STATUS) {    
        g_car_control_mode = CAR_DIRECTION_CONTROL_MODE;                        
        g_car_direction_control_module = CAR_STOP_TYPE;                          //停止
        g_car_status = CAR_STOP_STATUS;
        printf("_______3______\n");
    }
}

//robot car task 创建函数
hi_void app_demo_robot_car_task(hi_void)
{
    hi_u32 ret = 0;
    hi_task_attr attr = {0};
    
    hi_task_lock();
    attr.stack_size = CAR_CONTROL_DEMO_TASK_STAK_SIZE;
    attr.task_prio = CAR_CONTROL_DEMO_TASK_PRIORITY;
    attr.task_name = (hi_char*)"car_i2c_nfc_demo";
    
    ret = hi_task_create(&g_car_control_demo_task_id, &attr, robot_car_demo, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("Falied to create car control demo task!\n");
    }
    hi_task_unlock();
}
extern  hi_u32 g_car_speed_left;
extern  hi_u32 g_car_speed_right;
extern  hi_u8 count;
static hi_u8 speed_left_flag = 0;
static hi_u8 speed_right_flag = 0;
hi_void gpio11_irq_callback(hi_void *param)
{
    g_car_speed_left = 60000;
    count = 0;
    //hi_pwm_start(HI_PWM_PORT_PWM4, g_car_speed_left, 60000);
    printf("_______func :%s\r\n",__func__);
}
hi_void gpio12_irq_callback(hi_void *param)
{
    g_car_speed_right = 60000;
    count = 0;
    
    //hi_pwm_start(HI_PWM_PORT_PWM1, g_car_speed_right, 60000);
    printf("_______func :%s\r\n",__func__);
}
//按键中断
hi_void gpio5_interrupt_monitor(hi_void)
{
    hi_u32 ret = 0;
    /*gpio5 switch2 mode*/
    (hi_void)hi_gpio_init();
    g_gpio5_tick = hi_get_tick();
    ret = hi_gpio_register_isr_function(HI_GPIO_IDX_5, HI_INT_TYPE_EDGE,HI_GPIO_EDGE_FALL_LEVEL_LOW, get_gpio5_voltage, HI_NULL);
    ret = hi_gpio_register_isr_function(HI_GPIO_IDX_11, HI_INT_TYPE_EDGE,HI_GPIO_EDGE_FALL_LEVEL_LOW, gpio11_irq_callback, HI_NULL);
    ret = hi_gpio_register_isr_function(HI_GPIO_IDX_12, HI_INT_TYPE_EDGE,HI_GPIO_EDGE_FALL_LEVEL_LOW, gpio12_irq_callback, HI_NULL);
    if (ret == HI_ERR_SUCCESS) {
        printf(" register gpio5\r\n");
    }
}