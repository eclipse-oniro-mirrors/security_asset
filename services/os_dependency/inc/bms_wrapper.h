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

int32_t GetOwnerInfo(int32_t userId, uint64_t uid, OwnerType *ownerType, uint8_t *ownerInfo, uint32_t *infoLen);
int32_t GetCallingName(int32_t userId, uint8_t *name, uint32_t *nameLen, bool *isHap, int32_t *appIndex);

#ifdef __cplusplus
}
#endif

#endif