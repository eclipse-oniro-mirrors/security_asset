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

#include "secure_access_fence_service.h"
#include "permission_manager.h"
#include "wrapper.rs.h"
#include <chrono>
#include <vector>
#include <string>
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "secure_access_fence_type.h"
#include "cli_tool_mgr_client.h"
#include "saf_log.h"
#include "saf_result_code.h"
#include "saf_defines.h"
#include "access_token_wrapper.h"

namespace OHOS {
namespace Security {
namespace SAF {
using namespace CliTool;

constexpr const char* CLI_TOOL_PERMISSION = "ohos.permission.QUERY_CLI_TOOL";
constexpr const char* VERIFY_TOOL_PERMISSION = "ohos.permission.QUERY_TOOL_PERMISSIONS";
constexpr const char* MANAGE_TOOL_PERMISSION = "ohos.permission.MANAGE_TOOL_RUNTIME_PERMISSIONS";
const int32_t INVALID_OS_ACCOUNT_ID = 99999;

ErrCode BatchQueryCommandPermission(
    const std::vector<CommandInfo> &cmds,
    std::vector<CommandPermissionInfo> &cmdPermissions,
    int32_t &resultCode)
{
    auto startTime = std::chrono::steady_clock::now();
    if (!CheckPermission(CLI_TOOL_PERMISSION)) {
        LOGE("Permission denied! Need %{public}s", CLI_TOOL_PERMISSION);
        notify_error(
            rust::String("Permission denied"), SAF_ERR_PERMISSION_DENIED,
            INVALID_OS_ACCOUNT_ID, rust::String("BatchQueryCommandPermission")
        );
        return SAF_ERR_PERMISSION_DENIED;
    }
    std::vector<Command> cliCmds;
    for (const auto &cmd : cmds) {
        Command cliCmd;
        cliCmd.toolName = cmd.cmdName;
        cliCmd.subCommand = cmd.subCmd;
        cliCmds.push_back(cliCmd);
    }
    std::vector<CommandPermission> cliCmdPermissions;
    int32_t ret = CliToolMGRClient::GetInstance().BatchQueryPermissionBySubCommand(cliCmds, cliCmdPermissions);
    if (ret != 0) {
        LOGE("BatchQueryPermissionBySubCommand failed, ret=%{public}d", ret);
        resultCode = ret;
        notify_error(
            rust::String("BatchQueryPermissionBySubCommand failed"), SAF_ERR_TOOL_ERROR,
            INVALID_OS_ACCOUNT_ID, rust::String("BatchQueryCommandPermission")
        );
        return SAF_ERR_TOOL_ERROR;
    }
    for (const auto &cliPerm : cliCmdPermissions) {
        CommandPermissionInfo permInfo;
        permInfo.cmd.cmdName = cliPerm.cmd.toolName;
        permInfo.cmd.subCmd = cliPerm.cmd.subCommand;
        permInfo.permissions = cliPerm.permissions;
        permInfo.queryRet = cliPerm.queryRet;
        cmdPermissions.push_back(permInfo);
    }
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    int32_t elapsedTime = static_cast<int32_t>(duration.count());
    notify_performance_metrics(static_cast<int32_t>(cmds.size()), elapsedTime,
        INVALID_OS_ACCOUNT_ID, rust::String("BatchQueryCommandPermission")
    );
    resultCode = 0;
    return SAF_SUCCESS;
}

ErrCode RequestToolPermissions(
    const PermissionQuery &permissionQuery,
    PermissionQueryResult &permissionQueryResult,
    int32_t &resultCode)
{
    auto startTime = std::chrono::steady_clock::now();
    if (!CheckPermission(VERIFY_TOOL_PERMISSION)) {
        LOGE("Permission denied! Need %{public}s", VERIFY_TOOL_PERMISSION);
        resultCode = SAF_ERR_PERMISSION_DENIED;
        return SAF_SUCCESS;
    }
    
    if (permissionQuery.callerTokenId < 0) {
        LOGE("RequestToolPermissions failed, callerTokenId is invalid = %{public}d", permissionQuery.callerTokenId);
        resultCode = SAF_ERR_ARG_INVALID;
        return SAF_SUCCESS;
    }

    auto manager = PermissionManager::GetInstance();
    IF_TRUE_LOGE_RETURN_ERR(manager == nullptr, SAF_ERR_NULL_PTR, "get permissionManager instance failed");
    int32_t ret = manager->RequestToolPermissions(permissionQuery, permissionQueryResult);
    if (ret != SAF_SUCCESS) {
        LOGE("RequestToolPermissions failed, ret=%{public}d", ret);
        resultCode = ret;
        return SAF_SUCCESS;
    }

    resultCode = SAF_SUCCESS;
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    int32_t elapsedTime = static_cast<int32_t>(duration.count());
    notify_performance_metrics(static_cast<int32_t>(permissionQuery.operationInfo.size()), elapsedTime,
        INVALID_OS_ACCOUNT_ID, rust::String("RequestToolPermissions")
    );
    return SAF_SUCCESS;
}

ErrCode GrantToolPermissionsByUser(
    const std::vector<UserAuthResult> &userAuthResults, 
    std::vector<VerifyTicketInfo> &ticketInfos,
    int32_t &resultCode)
{
    auto startTime = std::chrono::steady_clock::now();
    if (!CheckPermission(MANAGE_TOOL_PERMISSION)) {
        LOGE("Permission denied! Need %{public}s", MANAGE_TOOL_PERMISSION);
        resultCode = SAF_ERR_PERMISSION_DENIED;
        return SAF_SUCCESS;
    }

    auto manager = PermissionManager::GetInstance();
    IF_TRUE_LOGE_RETURN_ERR(manager == nullptr, SAF_ERR_NULL_PTR, "get permissionManager instance failed");
    int32_t ret = manager->GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    if (ret != SAF_SUCCESS) {
        LOGE("GrantToolPermissionsByUser failed, ret=%{public}d", ret);
        resultCode = ret;
        return SAF_SUCCESS;
    }

    resultCode = SAF_SUCCESS;
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    int32_t elapsedTime = static_cast<int32_t>(duration.count());
    notify_performance_metrics(static_cast<int32_t>(userAuthResults.size()), elapsedTime,
        INVALID_OS_ACCOUNT_ID, rust::String("GrantToolPermissionsByUser")
    );
    return SAF_SUCCESS;
}

}
}
}