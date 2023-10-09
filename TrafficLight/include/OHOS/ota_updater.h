/*
 * Copyright (c) 2020 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup OTA
 * @{
 *
 * @brief Provides system upgrades.
 *
 * @since 1.0
 * @version 1.0
 */

 /**
 * @file ota_updater.h
 *
 * @brief Defines the functions for system upgrades.
 *
 * @since 1.0
 * @version 1.0
 */
#ifndef HARMONY_OS_LITE_OTA_UPDATER_H
#define HARMONY_OS_LITE_OTA_UPDATER_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define HASH_LENGTH               32
#define SIGN_DATA_LEN             256
#define COMP_VERSION_LENGTH       10
#define PKG_VERSION_LENGTH        64
#define PRODUCT_ID_LENGTH         64

#define OTA_MAX_PARTITION_NUM              10
#define COMPONENT_INFO_START               176
#define COMPONENT_INFO_TYPE_SIZE           2
#define COMPONENT_INFO_HEADER_LENGTH       4
#define SINGLE_COMPONENT_INFO_LENGTH       180

#define COMPONENT_ADDR_SIZE             125
#define MAX_BUFFER_SIZE                 1024
#define MAX_TRANSPORT_BUFF_SIZE         (4 * 1024)

#define USE_HARMONY_PKG                  1
#define NOT_USE_HARMONY_PKG              0


// Currently downloading component information
typedef struct {
    unsigned char isInfoComp; /* 1: information component,0: non-information component */
    unsigned int currentSize; /* Currently downloaded size of the component */
    unsigned int index; /* order of this component  */
    unsigned int remainSize; /* remain size of this component */
    unsigned int totalSize; /* total size of this component */
    unsigned int offset; /*  offset of this component */
} CurrentDloadComp;

#pragma pack(1)
typedef struct {
    unsigned short type;
    unsigned short length;
    unsigned int infoCompSize;
    unsigned int upgradePkgVersion;
    char productId[PRODUCT_ID_LENGTH];
    char version[PKG_VERSION_LENGTH];
} PkgBasicInfo;

#pragma pack(1)
typedef struct {
    unsigned char addr[COMPONENT_ADDR_SIZE];    /* destination address */
    unsigned short id;                          /* component ID */
    unsigned char type;                         /* component type */
    unsigned char operType;                     /* component operation type. 0: need upgrade 1: need delete */
    unsigned char isDiff;                       /* Is Diff component */
    unsigned char version[COMP_VERSION_LENGTH]; /* version of component */
    unsigned int length;                      /* size of component. if it is diff component,
                                           this mean diff compoent image size. */
    unsigned int destLength;                  /* size of component. if it is diff component,
                                           this mean total compoent image size affer diff reduction. */
    unsigned char shaData[HASH_LENGTH];
} ComponentInfo;

typedef struct {
    ComponentInfo table[OTA_MAX_PARTITION_NUM];
} ComponentInfos;

typedef enum {
    OTA_NOT_INIT = 0X01,
    OTA_INITED = 0x02,
    OTA_TRANSPORT_INFO_DONE = 0x03,
    OTA_TRANSPORT_ALL_DONE = 0x04,
    OTA_FAILED = 0x05,
    OTA_CANCELED = 0x06,
    OTA_TRANSPORT_VERIFY_FAILED = 0x07,
} OtaStatus;

typedef enum {
    OTA_PARA_ERR = 1,
    OTA_LOW_BATTERY = 2,
    OTA_ERASE_ERR = 3,
    OTA_DATA_TRANS_ERR = 4,
    OTA_DATA_WRITE_ERR = 5,
    OTA_DATA_SIGN_CHECK_ERR = 6,
} OtaErrorCode;

typedef void (*ErrorCallBackFunc)(OtaErrorCode errorCode);
typedef void (*StatusCallBackFunc)(OtaStatus status);

typedef struct {
    void (*ErrorCallBackFunc)(OtaErrorCode errorCode);
    void (*StatusCallBackFunc)(OtaStatus status);
} OtaNotifier;

/**
 * @brief Sets the switch for using the default upgrade package format.
 *
 * You can call this function to choose the default upgrade package format when developing the system upgrade
 * capability. \n
 * If the default upgrade package format is used, the system ensures the security and integrity of the
 * upgrade package. \n
 * If it is not used, you need to ensure the security and integrity of the upgrade package you have chosen.
 * To be specific, you need to call the {@link HotaRead} function to read the data that has been written into
 * flash memory, and then verify the data. \n
 *
 * @param flag Specifies whether to use the default system upgrade package format. The value <b>1</b> (default value)
 * means to use it, and <b>0</b> means not to use it.
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaSetUseHarmonyPkgFlag(unsigned int flag);

/**
 * @brief Obtains the index of the A/B partition to be upgraded.
 *
 * In the A/B upgrade scenario, you can use this function to determine whether partition A or B will be upgraded. \n
 *
 * @param index Indicates the index of a partition. The value <b>1</b> means partition A, and <b>2</b>
 * means partition B.
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaGetUpdateIndex(unsigned int *index);

/**
 * @brief Initializes the OTA module.
 *
 * @param errorCallback Indicates the callback invoked when an error occurs during the upgrade.
 * This parameter can be null.
 * @param statusCallback Indicates the callback invoked when the upgrade status changes. This parameter can be null.
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaInit(ErrorCallBackFunc errorCallback, StatusCallBackFunc statusCallback);

/**
 * @brief Writes specified data into flash memory.
 *
 * @param buffer Indicates the data to be written into flash memory.
 * @param offset Indicates the offset from where to start writing data.
 * @param buffSize Indicates the size of the data to be written.
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaWrite(unsigned char *buffer, unsigned int offset, unsigned int buffSize);

/**
 * @brief Reads the data that has been written into flash memory.
 *
 * This function is required for verifying data correctness when the default upgrade package format is not used. \n
 * It is not required when the default upgrade package format is used. \n
 *
 * @param offset Indicates the offset from where to start reading data.
 * @param bufLen Indicates the length of the data to be read.
 * @param buf Indicates the buffer to store the data that has been read.
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaRead(unsigned int offset, unsigned int bufLen, unsigned char *buf);

/**
 * @brief Cancels an upgrade.
 *
 * If an upgrade fails or is interrupted, you can use this function to cancel it. \n
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaCancel(void);

/**
 * @brief Sets the system state to be upgrading after data has been written into flash memory by {@link HotaWrite}.
 *
 * After this operation is successful, you need to call the {@link HotaRestart} function to complete the upgrade. \n
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaSetBootSettings(void);

/**
 * @brief Restarts the system after an upgrade.
 *
 * You need to call this function after you have called the {@link HotaSetBootSettings} function. \n
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaRestart(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef HARMONY_OS_LITE_OTA_UPDATER_H */
/** @} */

