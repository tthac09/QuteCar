/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT platform
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */

#ifndef IOT_LOG_H_
#define IOT_LOG_H_

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>


/**
 * @brief:this defines for the log module, and  IOT_LOG_TRACE/IOT_LOG_DEBUG will not participate the compile in the release version
 *
 *
 * */
typedef enum
{
    EN_IOT_LOG_LEVEL_TRACE = 0,    ///< this is used as the trace function,like the function enter and function out
    EN_IOT_LOG_LEVEL_DEBUG,        ///< this is used as the debug, you could add any debug as you wish
    EN_IOT_LOG_LEVEL_INFO,         ///< which means it is import message, and you should known
    EN_IOT_LOG_LEVEL_WARN,      ///< this is used as the executed result,which means the status is not what we expected,but could accept
    EN_IOT_LOG_LEVEL_ERROR,        ///< this is used as the executed result,which means the status is not what we expected,could not accepta
    EN_IOT_LOG_LEVEL_FATAL,        ///< this is used as the parameters input for the api interface, which could not accepted
    EN_IOT_LOG_LEVEL_MAX,
}en_iot_log_level_t;

/**
 * @brief:use this function to get the current output log
 *
 * @return: the current output mask log, defined by en_iot_log_level_t
 * */
en_iot_log_level_t IoTLogLevelGet(void);

/**
 * @brief: use this function to get the debug level name
 *
 * @parameter[in]:level, the level to get
 *
 * @return: the mapped level name
 * */
const char *IoTLogLevelGetName(en_iot_log_level_t level);

/**
 * @brief:use this function to set the current output log
 *
 *@parameter[in] level:defined by en_iot_log_level_t
 *
 * @return: 0 success while -1 failed
 * */
int IoTLogLevelSet(en_iot_log_level_t level);


/*
 * @brief: this is a weak function ,and you could rewrite one
 *
 * @param fmt: same use as the fmt for printf
 *
 * @param unfixed: same use for printf
 *
 * @return: don't care about it
 *
 * @attention: and the components should not call this function directly, you'd better
 *
 *             call IOT_LOG groups
 *
 * */
//__attribute__((weak)) void IOT_PRINT(const char *format, ...);

#ifndef IOT_PRINT
#define IOT_PRINT(fmt, ...) \
    do \
    { \
        printf(fmt, ##__VA_ARGS__); \
    }while(0)
#endif

#ifdef CONFIG_LINKLOG_ENABLE

#define IOT_LOG(level,fmt, ...) \
    do \
    { \
        IOT_PRINT("[%s][%s] " fmt "\r\n", \
        IoTLogLevelGetName((level)),__FUNCTION__, ##__VA_ARGS__); \
    } while (0)

#define IOT_LOG_TRACE(fmt, ...) \
    do \
    { \
        if ((EN_IOT_LOG_LEVEL_TRACE) >= IoTLogLevelGet()) \
        { \
            IOT_LOG(EN_IOT_LOG_LEVEL_TRACE,fmt,##__VA_ARGS__); \
        } \
    } while (0)

#define IOT_LOG_DEBUG(fmt, ...) \
    do \
    { \
        if ((EN_IOT_LOG_LEVEL_DEBUG) >= IoTLogLevelGet()) \
        { \
            IOT_LOG(EN_IOT_LOG_LEVEL_DEBUG,fmt,##__VA_ARGS__); \
        } \
    } while (0)

#else

#define IOT_LOG(level,fmt, ...)
#define IOT_LOG_TRACE(fmt, ...)
#define IOT_LOG_DEBUG(fmt, ...)

#endif

#define IOT_LOG_TRACE(fmt,...)   IOT_LOG(EN_IOT_LOG_LEVEL_TRACE,fmt,##__VA_ARGS__)
#define IOT_LOG_INFO(fmt, ...)   IOT_LOG(EN_IOT_LOG_LEVEL_INFO,fmt,##__VA_ARGS__)
#define IOT_LOG_WARN(fmt, ...)   IOT_LOG(EN_IOT_LOG_LEVEL_WARN,fmt,##__VA_ARGS__)
#define IOT_LOG_ERROR(fmt, ...)  IOT_LOG(EN_IOT_LOG_LEVEL_ERROR,fmt,##__VA_ARGS__)
#define IOT_LOG_FATAL(fmt, ...)  IOT_LOG(EN_IOT_LOG_LEVEL_FATAL,fmt,##__VA_ARGS__)


#endif /* IOT_LOG_H_ */