/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "credential_inner_helper.h"

#include "errors.h"

#include "saf_result_code.h"

namespace OHOS::Security::SAF {

namespace {
// ERR_REMOTE_DEAD mirrors the value used by the OpenHarmony IPC layer for a
// dead remote object. Kept as a literal so the helper stays self-contained.
constexpr int32_t ERR_REMOTE_DEAD = 29189;
} // namespace

int32_t NormalizeIpcResult(int32_t ret)
{
    if (ret == ERR_DEAD_OBJECT || ret == ERR_REMOTE_DEAD) {
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }
    return ret;
}

bool IsBatchSizeMatched(size_t sourceSize, size_t destSize)
{
    return sourceSize == destSize;
}

} // namespace OHOS::Security::SAF
