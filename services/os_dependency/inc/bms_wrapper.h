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

#ifdef __cplusplus
extern "C" {
#endif

enum OwnerType {
    HAP = 0,
    NATIVE = 1,
};

typedef struct {
    uint8_t *appId;
    uint32_t appIdLen;
    int32_t appIndex;
    uint8_t *groupId;
    uint32_t groupIdLen;
    uint8_t *developerId;
    uint8_t developerIdLen;
} HapInfo;

typedef struct {
    uint32_t uid;
} NativeInfo;

typedef struct {
    uint32_t userId;
    OwnerType ownerType;

    // Bundle name for hap or process name for native.
    uint8_t *processName;
    uint32_t processNameLen;

    HapInfo hapInfo;
    NativeInfo nativeInfo;
} ProcessInfo;

int32_t GetCallingProcessInfo(uint32_t userId, uint64_t uid, ProcessInfo *processInfo);

#ifdef __cplusplus
}
#endif

#endif