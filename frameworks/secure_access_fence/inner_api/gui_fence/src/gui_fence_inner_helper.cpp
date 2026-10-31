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

#include "gui_fence_inner_helper.h"

#include <tuple>

#include "errors.h"
#include "saf_log.h"

namespace OHOS::Security::SAF {

// ERR_REMOTE_DEAD mirrors the value used by the OpenHarmony IPC layer for a
// dead remote object. Kept as a literal here so the helper is self-contained.
namespace {
constexpr int32_t ERR_REMOTE_DEAD = 29189;
} // namespace

bool ParseDeniedPair(const std::string &pair, AppPairKey &key)
{
    size_t pos1 = pair.find(':');
    if (pos1 == std::string::npos) {
        return false;
    }
    std::string callerBundle = pair.substr(0, pos1);
    size_t pos2 = pair.find(':', pos1 + 1);
    if (pos2 == std::string::npos) {
        return false;
    }
    int32_t callerIdx = std::stoi(pair.substr(pos1 + 1, pos2 - pos1 - 1));
    size_t pos3 = pair.find(':', pos2 + 1);
    if (pos3 == std::string::npos) {
        return false;
    }
    std::string targetBundle = pair.substr(pos2 + 1, pos3 - pos2 - 1);
    int32_t targetIdx = std::stoi(pair.substr(pos3 + 1));
    key = std::make_tuple(callerBundle, callerIdx, targetBundle, targetIdx);
    return true;
}

int32_t HandleIpcError(sptr<ISecureAccessFence> &proxy, std::function<int32_t()> call)
{
    if (call == nullptr) {
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }
    int32_t ret = call();
    if (ret == ERR_DEAD_OBJECT || ret == ERR_REMOTE_DEAD) {
        LOGW("service unavailable and not retry.");
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }
    return ret;
}

} // namespace OHOS::Security::SAF
