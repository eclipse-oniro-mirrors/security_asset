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

#ifndef SYSTEM_EVENT_WRAPPER
#define SYSTEM_EVENT_WRAPPER

#include <stdint.h>

typedef struct {
    uint32_t size;
    const uint8_t *data;
} ConstSAFBlob;

typedef enum {
    UNKNOWN = 0,
    PACKAGE_REMOVED = 1,
    PACKAGE_ADDED = 2,
    PACKAGE_CHANGED = 3,
    RESTORE_START = 4,
} CommonEventType;

typedef struct {
    CommonEventType eventType;
    ConstSAFBlob uid;
    ConstSAFBlob appIndex;
    ConstSAFBlob bundleName;
    ConstSAFBlob userId;
} CommonEventInfo;

typedef void (*OnCommonEvent)(CommonEventInfo);

typedef struct {
    OnCommonEvent onCommonEvent;
} EventCallBack;

#ifdef __cplusplus
extern "C" {
#endif

bool SubscribeSystemEvent(const EventCallBack eventCallBack);
bool UnSubscribeSystemEvent(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_EVENT_WRAPPER_H
