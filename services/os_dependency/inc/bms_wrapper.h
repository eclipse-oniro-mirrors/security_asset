/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef BMS_WRAPPER
#define BMS_WRAPPER

#include <stdint.h>
#include <vector>
#include <string>
#include "asset_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_SIZE(arr) ((sizeof(arr)) / (sizeof((arr)[0])))

enum OwnerType {
    HAP = 0,
    NATIVE = 1,
    HAP_GROUP = 2,
};

typedef struct {
    int32_t appIndex;
    Asset_Blob appId;
    Asset_Blob groupId;
    Asset_Blob developerId;
} HapInfo;

typedef struct {
    uint32_t uid;
} NativeInfo;

typedef struct {
    uint32_t userId;
    OwnerType ownerType;
    // Bundle name for hap or process name for native.
    Asset_Blob processName;
    HapInfo hapInfo;
    NativeInfo nativeInfo;
} ProcessInfo;

int32_t GetCallingProcessInfo(uint32_t userId, uint64_t uid, ProcessInfo *processInfo);

int32_t GetCloneAppIndexes(int32_t userId, int32_t *appIndexes, uint32_t *indexSize, const char *appName);

int32_t isHapInAllowList(int32_t userId, const char *appName, bool *is_in_list);

#ifdef __cplusplus
}
#endif

#endif