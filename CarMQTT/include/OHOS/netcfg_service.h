/*
 * Copyright (c) 2020 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup netcfgservice
 * @{
 *
 * @brief Provides the network configuration service of HarmonyOS. Two network configuration modes are available:
 * Neighbor Awareness Networking (NAN) and software enabled access point (SoftAP).
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file netcfg_service.h
 *
 * @brief Defines functions of the <b>netcfgservice</b> module.
 *
 * You can use the functions to: \n
 * <ul>
 *   <li>Start the network configuration service.</li>
 *   <li>Stop the network configuration service.</li>
 *   <li>Register callbacks for events of the network configuration service.</li>
 *   <li>Notify the network configuration result.</li>
 * </ul>
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef NETCFG_SERVICE_H
#define NETCFG_SERVICE_H

#include "wifi_error_code.h"
#include "wifi_device_config.h"
#include "wifi_hotspot_config.h"

/**
 * @brief Indicates the length of the personal identification number (PIN) of the device. The length is 16 bytes.
 * If the actual length is less than 16 bytes, <b>'\0'</b> is appended.
 *
 */
#define NETCFG_DEVPARA_PIN_LEN 16
/**
 * @brief Indicates the length of the product ID of the device. The length is 4 bytes.
 * If the actual length is less than 4 bytes, <b>'\0'</b> is appended.
 *
 */
#define NETCFG_DEVPARA_PRODUCTID_LEN 4
/**
 * @brief Indicates the length of the serial number (SN) of the device. The length is 32 bytes.
 * If the actual length is less than 32 bytes, <b>'\0'</b> is appended.
 *
 */
#define NETCFG_DEVPARA_SN_LEN 32
/**
 * @brief Indicates a random number of 16 bytes, which is used for transmitting the service
 * set identifier (SSID) and password from the mobile phone to the device.
 *
 */
#define NETCFG_RANDOM_LEN 16

/**
 * @brief Enumerates common error codes of the <b>netcfgservice</b> module.
 */
enum NetCfgErrorCode {
    /** Success */
    NETCFG_SUCCESS = 0,
    /** Invalid parameter. */
    ERROR_NETCFG_INVALID_ARGS = -1,
    /** AP is disabled in the SoftAP mode. */
    ERROR_NETCFG_HOTSPOT_INACTIVE = -2,
    /** Failed to start the NAN mode. */
    ERROR_NETCFG_NAN_FAIL = -3,
    /** Failed to start the SoftAP mode. */
    ERROR_NETCFG_HOTSPOT_FAIL = -4,
    /** Failed to start the SoftAP and NAN modes. */
    ERROR_NETCFG_ALL_FAIL = -5,
    ERROR_NETCFG_SEND_MSG_BUSY = -6,
    ERROR_NETCFG_OTHERS = -7
};

/**
 * @brief Enumerates network configuration results.
 * The application needs to pass the result to {@link NotifyConfigWifiResult}.
 */
enum NetCfgRstCode {
    /** Network configuration fails. */
    NETCFG_RST_FAIL = -1,
    /** Network configuration is successful. */
    NETCFG_RST_SUCC = 0,
};

/**
 * @brief Enumerates how an application obtains the SSID and password. This enumeration is passed to
 * the <b>mode</b> parameter of the {@link OnTransferConfig} callback in {@link WifiConfigureEvent}.
 */
enum NetCfgMode {
    /** SoftAP */
    NETCFG_MODE_SOFTAP = 0,
    /** NAN */
    NETCFG_MODE_NAN = 1,
};

/**
 * @brief Declares device parameters, which are passed when the application starts the network configuration service.
 */
struct DevPara {
    /** Device PIN. For details about its length, see {@link NETCFG_DEVPARA_PIN_LEN}. */
    char pin[NETCFG_DEVPARA_PIN_LEN];
    /** Device product ID. For details about its length, see {@link NETCFG_DEVPARA_PRODUCTID_LEN}. */
    char productId[NETCFG_DEVPARA_PRODUCTID_LEN];
    /** Device SN. For details about its length, see {@link NETCFG_DEVPARA_SN_LEN}. */
    char sn[NETCFG_DEVPARA_SN_LEN];
};

/**
 * @brief Declares network configuration parameters, which are passed when the application starts
 * the network configuration service.
 */
struct NetCfgPara {
    /** Short-range power level. The value ranges from -70 dB to -42 dB. The default value is -61 dB. */
    signed char db;
    /** Timeout duration for establishing a NAN channel. The value ranges from 2s to 15s. The default value is 3s. */
    unsigned char nanAckTimout;
     /** Heartbeat timeout duration of the network configuration service.
       The value ranges from 10s to 120s. The default value is 60s. */
    unsigned char hbTimout;
};

/**
 * @brief Declares information about a hotspot to connect.
 */
struct NetCfgConfig {
    /** Basic hotspot information, including the SSID, BSSID, pre-shared key, and security type */
    WifiDeviceConfig config;
    /** Communication channel information of the hotspot */
    unsigned char channel;
    /** Random value information */
    char random[NETCFG_RANDOM_LEN];
    /** Method for obtaining the hotspot information. For details, see {@link NetCfgMode}. */
    char mode;
};

/**
 * @brief Declares callbacks of the network configuration service.
 */
struct WifiConfigureEvent {
    void (*OnPreTransferConfig)(void);
    /** Called when the network configuration service receives information about
      available access points (APs) from the mobile phone. */
    void (*OnTransferConfig)(struct NetCfgConfig *config);
    /** Called when the network configuration service receives a control command from the mobile phone. */
    void (*OnMessageReceived)(char *msg, int msgLen);
    /** Called when the network configuration service receives a command from the mobile phone
      to disconnect from a device. (This command will not stop the network configuration service.) */
    void (*OnDisconnected)(void);
    void (*OnTimeout)(void);
};

/**
 * @brief Starts a network configuration in SoftAP mode.
 *
 * Packets with the multicast address 238.238.238.238 and port number 5683 are listened by default.
 *
 * @param config Indicates the pointer to the SoftAP configuration. For details, see {@link HotspotConfig}.
 * If the value is <b>NULL</b>, the application starts the AP and the SoftAP network configuration service uses the AP.
 * If the value is not <b>NULL</b>, the SoftAP network configuration service starts the AP
 * based on {@link HotspotConfig}.
 * @param devPara Indicates the pointer to device parameters. For details, see {@link DevPara}.
 * @param netCfgPara Indicates the pointer to network configuration parameters. For details, see {@link NetcfgPara}.
 * @return Returns the operation result. For details, see {@link NetCfgErrorCode}.
 * @since 1.0
 * @version 1.0
 */
int StartConfigWifi(const HotspotConfig *config, const struct DevPara *devPara, const struct NetCfgPara *netCfgPara);

/**
 * @brief Stops a network configuration.
 *
 * @since 1.0
 * @version 1.0
 */
void StopConfigWifi(void);

/**
 * @brief Registers a callback for an event of the network configuration service.
 *
 * @param event Indicates the pointer to the event of the network configuration service. For details,
 * see {@link WifiConfigureEvent}}.
 * @return Returns <b>NETCFG_SUCCESS</b> if the callback is registered successfully;
 * returns <b>ERROR_NETCFG_INVALID_ARGS</b> if the <b>event</b> parameter is invalid (for example,
 * the <b>event</b> parameter is <b>NULL</b>). For details, see {@link NetCfgErrorCode}.
 * @since 1.0
 * @version 1.0
 */
int RegConfigWifiCallback(const struct WifiConfigureEvent *event);

/**
 * @brief Notifies the network configuration result.
 *
 * The application can call this function only after it has obtained the SSID and password. \n
 * If the network configuration fails, pass <b>NETCFG_RST_FAIL (-1)</b>;
 * if the network configuration is successful, pass <b>NETCFG_RST_SUCC (0)</b>. \n
 * The application can also use this function to transmit other result information to the Feature Ability (FA).
 * The network configuration service transparently transmits all the result information.
 * It also resets its state to the initial state when the result is <b>NETCFG_RST_FAIL (-1)</b>.
 *
 * @since 1.0
 * @version 1.0
 */
void NotifyConfigWifiResult(signed char result);

int SendMessage(char *msg, int msgLen);

#endif
/** @} */
