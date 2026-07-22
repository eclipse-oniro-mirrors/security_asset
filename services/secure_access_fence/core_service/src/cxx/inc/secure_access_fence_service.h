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

#include <vector>
#include <string>
#include "secure_access_fence_type.h"
#include "permission_manager.h"
#include "saf_result_code.h"

namespace OHOS {
namespace Security {
namespace SAF {

ErrCode BatchQueryCommandPermission(
    const std::vector<CommandInfo> &cmds,
    std::vector<CommandPermissionInfo> &cmdPermissions,
    int32_t &resultCode);

ErrCode RequestToolPermissions(
    const PermissionQuery &permissionQuery,
    PermissionQueryResult &permissionQueryResult,
    int32_t &resultCode);

ErrCode GrantToolPermissionsByUser(
    const std::vector<UserAuthResult> &userAuthResults,
    std::vector<VerifyTicketInfo> &ticketInfos,
    int32_t &resultCode);

// Rust callable: returns boot time in milliseconds, or -1 on failure.
int64_t GetBootTimeMs();

} // namespace SAF
} // namespace Security
} // namespace OHOS

#endif