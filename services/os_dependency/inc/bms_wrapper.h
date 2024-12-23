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
#include "asset_type.h"

#ifdef __cplusplus
extern "C" {
#endif

enum OwnerType {
    HAP = 0,
    NATIVE = 1,
    GROUP = 2,
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

#ifdef __cplusplus
}
#endif

#endif