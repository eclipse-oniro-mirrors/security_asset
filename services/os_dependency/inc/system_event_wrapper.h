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

typedef void (*OnPackageRemoved)(int32_t, const uint8_t *, uint32_t, const uint8_t *, int32_t);
typedef void (*OnUserRemoved)(int32_t);
typedef void (*OnScreenOff)(void);
typedef void (*OnCharging)(void);
typedef void (*OnAppRestore)(int32_t, const uint8_t *, int32_t);
typedef void (*OnUserUnlocked)(int32_t);

typedef struct {
    OnPackageRemoved onPackageRemoved;
    OnUserRemoved onUserRemoved;
    OnScreenOff onScreenOff;
    OnCharging onCharging;
    OnAppRestore onAppRestore;
    OnUserUnlocked onUserUnlocked;
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
