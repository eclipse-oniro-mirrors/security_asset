/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SECURE_ACCESS_FENCE_SERVICE_H
#define SECURE_ACCESS_FENCE_SERVICE_H

#include "cxx.h"
#include "refbase.h"
#include <cstdint>
#include <memory>

#include "message_parcel.h"

namespace OHOS {
namespace Security {
namespace SecureAccessFence {

typedef enum {
    QUERY_PERMISSION_BY_SUB_COMMAND_BATCH = 500,
} SecureAccessFenceCode;

int32_t OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply);

} // namespace SecureAccessFence
} // namespace Security
} // namespace OHOS

#endif
