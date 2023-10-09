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

#include "parameter.h"
#include <securec.h>
#include "param_adaptor.h"
#include "hos_errno.h"
#include "hos_types.h"

#define FILE_RO "ro."
#define VERSION_ID_LEN 256

static char g_roBuildOs[] = {"HarmonyOS"};
static char g_roBuildVerShow[] = {"HarmonyOS 2.0"};
static char g_roSdkApiLevel[] = {"3"};
static char g_roFirstApiLevel[] = {"1"};

static boolean IsValidValue(const char* value, unsigned int len)
{
    if ((value == NULL) || !strlen(value) || (strlen(value) + 1 > len)) {
        return FALSE;
    }
    return TRUE;
}

int GetParameter(const char* key, const char* def, char* value, unsigned int len)
{
    if ((key == NULL) || (value == NULL)) {
        return EC_FAILURE;
    }
    if (!CheckPermission()) {
        return EC_FAILURE;
    }
    int ret = GetSysParam(key, value, len);
    if (ret == EC_INVALID) {
        return EC_FAILURE;
    }
    if ((ret < 0) && IsValidValue(def, len)) {
        if (strcpy_s(value, len, def) != 0) {
            return EC_FAILURE;
        }
        ret = EC_SUCCESS;
    }
    return ret;
}

int SetParameter(const char* key, const char* value)
{
    if ((key == NULL) || (value == NULL)) {
        return EC_FAILURE;
    }
    if (!CheckPermission()) {
        return EC_FAILURE;
    }
    if (strncmp(key, FILE_RO, strlen(FILE_RO)) == 0) {
        return EC_FAILURE;
    }
    if (SetSysParam(key, value) != EC_SUCCESS) {
        return EC_FAILURE;
    }
    return EC_SUCCESS;
}

char* GetProductType(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_PRODUCT_TYPE) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_PRODUCT_TYPE) + 1, HOS_PRODUCT_TYPE) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetManufacture(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_MANUFACTURE) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_MANUFACTURE) + 1, HOS_MANUFACTURE) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetBrand(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_BRAND) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_BRAND) + 1, HOS_BRAND) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetMarketName(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_MARKET_NAME) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_MARKET_NAME) + 1, HOS_MARKET_NAME) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetProductSeries(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_PRODUCT_SERIES) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_PRODUCT_SERIES) + 1, HOS_PRODUCT_SERIES) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetProductModel(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_PRODUCT_MODEL) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_PRODUCT_MODEL) + 1, HOS_PRODUCT_MODEL) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetSoftwareModel(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_SOFTWARE_MODEL) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_SOFTWARE_MODEL) + 1, HOS_SOFTWARE_MODEL) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetHardwareModel(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_HARDWARE_MODEL) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_HARDWARE_MODEL) + 1, HOS_HARDWARE_MODEL) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetHardwareProfile(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_HARDWARE_PROFILE) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_HARDWARE_PROFILE) + 1, HOS_HARDWARE_PROFILE) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetSerial(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_SERIAL) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_SERIAL) + 1, HOS_SERIAL) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetOsName(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(g_roBuildOs) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(g_roBuildOs) + 1, g_roBuildOs) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetDisplayVersion(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(g_roBuildVerShow) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(g_roBuildVerShow) + 1, g_roBuildVerShow) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetBootloaderVersion(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_BOOTLOADER_VERSION) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_BOOTLOADER_VERSION) + 1, HOS_BOOTLOADER_VERSION) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetSecurityPatchTag(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_SECURITY_PATCH_TAG) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_SECURITY_PATCH_TAG) + 1, HOS_SECURITY_PATCH_TAG) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetAbiList(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_ABI_LIST) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_ABI_LIST) + 1, HOS_ABI_LIST) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetSdkApiLevel(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(g_roSdkApiLevel) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(g_roSdkApiLevel) + 1, g_roSdkApiLevel) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetFirstApiLevel(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(g_roFirstApiLevel) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(g_roFirstApiLevel) + 1, g_roFirstApiLevel) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetIncrementalVersion(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_INCREMENTAL_VERSION) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_INCREMENTAL_VERSION) + 1, HOS_INCREMENTAL_VERSION) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetVersionId(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(VERSION_ID_LEN);
    if (value == NULL) {
        return NULL;
    }
    if (memset_s(value, VERSION_ID_LEN, 0, VERSION_ID_LEN) != 0) {
        free(value);
        return NULL;
    }
    if (sprintf_s(value, VERSION_ID_LEN - 1, "%s/%s/%s/%s/%s/%s/%s/%s/%s/%s",
        HOS_PRODUCT_TYPE, HOS_MANUFACTURE, HOS_BRAND, HOS_PRODUCT_SERIES,
        g_roBuildOs, HOS_PRODUCT_MODEL, HOS_SOFTWARE_MODEL, g_roSdkApiLevel,
        HOS_INCREMENTAL_VERSION, HOS_BUILD_TYPE) < 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetBuildType(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_BUILD_TYPE) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_BUILD_TYPE) + 1, HOS_BUILD_TYPE) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetBuildUser(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_BUILD_USER) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_BUILD_USER) + 1, HOS_BUILD_USER) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetBuildHost(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_BUILD_HOST) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_BUILD_HOST) + 1, HOS_BUILD_HOST) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetBuildTime(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_BUILD_TIME) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_BUILD_TIME) + 1, HOS_BUILD_TIME) != 0) {
        free(value);
        return NULL;
    }
    return value;
}

char* GetBuildRootHash(void)
{
    if (!CheckPermission()) {
        return NULL;
    }
    char* value = (char*)malloc(strlen(HOS_BUILD_ROOTHASH) + 1);
    if (value == NULL) {
        return NULL;
    }
    if (strcpy_s(value, strlen(HOS_BUILD_ROOTHASH) + 1, HOS_BUILD_ROOTHASH) != 0) {
        free(value);
        return NULL;
    }
    return value;
}