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
#include "time_wrapper.h"
#include "wrapper.rs.h"
#include <chrono>
#include <memory>
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
#include "accesstoken_kit.h"
#include "os_account_wrapper.h"

using namespace OHOS::Security::AccessToken;

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
    if (!CheckIsSystemHap()) {
        LOGE("Caller is not system hap");
        resultCode = SAF_ERR_NOT_SYSTEM_APP;
        return SAF_SUCCESS;
    }
    if (permissionQuery.callerTokenId < 0) {
        LOGE("RequestToolPermissions failed, callerTokenId is invalid = %{public}d", permissionQuery.callerTokenId);
        resultCode = SAF_ERR_ARG_INVALID;
        return SAF_SUCCESS;
    }

    auto manager = PermissionManager::GetInstance();
    resultCode = SAF_ERR_NULL_PTR;
    IF_TRUE_LOGE_RETURN_ERR(manager == nullptr, SAF_SUCCESS, "get permissionManager instance failed");
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
    if (!CheckIsSystemHap()) {
        LOGE("Caller is not system hap");
        resultCode = SAF_ERR_NOT_SYSTEM_APP;
        return SAF_SUCCESS;
    }

    auto manager = PermissionManager::GetInstance();
    resultCode = SAF_ERR_NULL_PTR;
    IF_TRUE_LOGE_RETURN_ERR(manager == nullptr, SAF_SUCCESS, "get permissionManager instance failed");
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

// Rust callable: returns boot time in milliseconds, or -1 on failure.
int64_t GetBootTimeMs()
{
    return TimeWrapper::GetBootTimeMs();
}

int32_t CxxRequestToolTicket(
    const rust::Vec<CxxCommandInfo>& cliCmdInfos,
    uint64_t ticketExpireTimeMs,
    CxxTicketQueryResult& result)
{
    PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    permissionQuery.needTicket = true;
    permissionQuery.ticketExpireTimeMs = ticketExpireTimeMs;

    for (const auto& cmd : cliCmdInfos) {
        OperationInfo opInfo;
        opInfo.operationType = OperationType::CLI;
        opInfo.cliCmdInfo.cmdName = std::string(cmd.cmd_name);
        opInfo.cliCmdInfo.subCmd = std::string(cmd.sub_cmd);
        permissionQuery.operationInfo.push_back(opInfo);
    }

    auto manager = PermissionManager::GetInstance();
    if (manager == nullptr) {
        LOGE("CxxRequestToolTicket: get permissionManager instance failed");
        return SAF_ERR_NULL_PTR;
    }

    PermissionQueryResult queryResult;
    int32_t ret = manager->RequestToolPermissions(permissionQuery, queryResult, true);
    if (ret != SAF_SUCCESS) {
        LOGE("CxxRequestToolTicket failed, ret=%{public}d", ret);
        return ret;
    }

    for (const auto& permInfo : queryResult.permissionResults) {
        CxxPermissionInfo cxxPerm;
        cxxPerm.permission = rust::String(permInfo.permission);
        cxxPerm.permission_status = static_cast<int32_t>(permInfo.permissionStatus);
        cxxPerm.auth_status = static_cast<int32_t>(permInfo.authStatusInfo.authStatus);
        cxxPerm.flag = permInfo.authStatusInfo.flag;
        result.permission_results.push_back(cxxPerm);
    }
    result.has_ticket = queryResult.hasTicket;
    if (queryResult.hasTicket) {
        result.ticket_message = rust::String(queryResult.ticket.message);
        result.ticket_challenge = rust::String(queryResult.ticket.challenge);
        result.ticket = rust::String(queryResult.ticket.ticket);
    }
    return SAF_SUCCESS;
}

int32_t GetCallerAppInfo(AppInfo &callerAppInfo)
{
    struct CallerAppInfo rawCallerAppInfo;
    int32_t ret = ::GetCallerAppInfo(&rawCallerAppInfo);
    if (ret != SAF_SUCCESS) {
        LOGE("GetCallerAppInfo failed, ret=%{public}d", ret);
        return ret;
    }
    callerAppInfo.bundleName = rawCallerAppInfo.bundleName;
    callerAppInfo.index = rawCallerAppInfo.index;
    LOGI("GetCallerAppInfo success, bundleName=%{public}s, index=%{public}d",
        callerAppInfo.bundleName.c_str(), callerAppInfo.index);
    return SAF_SUCCESS;
}

int32_t ValidateAppInfoList(const std::vector<AppInfo> &appInfoList)
{
    LOGI("ValidateAppInfoList enter, appInfoList size=%{public}zu", appInfoList.size());
    for (size_t i = 0; i < appInfoList.size(); ++i) {
        const AppInfo &appInfo = appInfoList[i];
        if (appInfo.bundleName.empty()) {
            LOGE("ValidateAppInfoList failed, bundleName is empty at index=%{public}zu", i);
            return SAF_ERR_ARG_INVALID;
        }
        if (appInfo.index < 0) {
            LOGE("ValidateAppInfoList failed, index is negative at index=%{public}zu, index=%{public}d",
                i, appInfo.index);
            return SAF_ERR_ARG_INVALID;
        }
    }
    LOGI("ValidateAppInfoList success");
    return SAF_SUCCESS;
}

int32_t QueryControlAppStatusInternal(
    const AppInfo &callerAppInfo,
    const std::vector<AppInfo> &appInfoList,
    std::vector<ControlAppStatusResult> &resultList)
{
    LOGI("QueryControlAppStatusInternal enter, callerBundleName=%{public}s, appCount=%{public}zu",
        callerAppInfo.bundleName.c_str(), appInfoList.size());

    // Get OS account ID from calling UID
    int32_t osAccountId = INVALID_OS_ACCOUNT_ID;
    int32_t callingUid = IPCSkeleton::GetCallingUid();
    int32_t userId = 0;
    if (GetOsAccountIdFromUid(callingUid, &userId)) {
        osAccountId = userId;
    } else {
        LOGE("QueryControlAppStatusInternal failed to get os account id from uid=%{public}d", callingUid);
        return SAF_ERR_INVALID_OS_ACCOUNT_ID;
    }

    // Serialize appInfoList to "bundleName:index" string array
    std::vector<std::string> appInfoStrList;
    for (const auto &appInfo : appInfoList) {
        appInfoStrList.push_back(appInfo.bundleName + ":" + std::to_string(appInfo.index));
    }

    std::vector<rust::String> appInfoRustList(appInfoStrList.begin(), appInfoStrList.end());
    rust::Vec<int32_t> statusList;
    int32_t ret = query_control_app_status(
        callerAppInfo.bundleName,
        callerAppInfo.index,
        osAccountId,
        rust::Slice<const rust::String>(appInfoRustList.data(), appInfoRustList.size()),
        statusList);
    if (ret != SAF_SUCCESS) {
        LOGE("QueryControlAppStatusInternal query_control_app_status failed, ret=%{public}d", ret);
        return ret;
    }

    if (statusList.size() != appInfoList.size()) {
        LOGE("QueryControlAppStatusInternal size mismatch, appInfoList=%{public}zu, statusList=%{public}zu",
            appInfoList.size(), statusList.size());
        return SAF_ERR_MAP_KEY_NOT_FOUND;
    }

    // Convert statusList to ControlAppStatusResult
    for (size_t i = 0; i < appInfoList.size(); ++i) {
        ControlAppStatusResult result;
        result.appInfo = appInfoList[i];
        result.status = static_cast<ControlAppStatus>(statusList[i]);
        resultList.push_back(result);
    }

    LOGI("QueryControlAppStatusInternal success, resultList size=%{public}zu", resultList.size());
    return SAF_SUCCESS;
}

ErrCode QueryControlAppStatus(
    const std::vector<AppInfo> &appInfoList,
    std::vector<ControlAppStatusResult> &resultList,
    int32_t &resultCode)
{
    auto startTime = std::chrono::steady_clock::now();
    if (!CheckPermission(CONTROL_DEVICE_PERMISSION)) {
        LOGE("Permission denied! Need %{public}s", CONTROL_DEVICE_PERMISSION);
        resultCode = SAF_ERR_PERMISSION_DENIED;
        return SAF_SUCCESS;
    }
    if (appInfoList.empty()) {
        LOGE("QueryControlAppStatus failed, appInfoList is empty");
        resultCode = SAF_ERR_ARG_EMPTY;
        return SAF_SUCCESS;
    }

    // Validate AppInfo: bundleName non-empty and exists, index >= 0
    int32_t validateResult = ValidateAppInfoList(appInfoList);
    if (validateResult != SAF_SUCCESS) {
        LOGE("QueryControlAppStatus ValidateAppInfoList failed, ret=%{public}d", validateResult);
        resultCode = validateResult;
        return SAF_SUCCESS;
    }

    // Get caller app info from IPC context
    AppInfo callerAppInfo;
    int32_t callerResult = GetCallerAppInfo(callerAppInfo);
    if (callerResult != SAF_SUCCESS) {
        LOGE("QueryControlAppStatus GetCallerAppInfo failed, ret=%{public}d", callerResult);
        resultCode = callerResult;
        return SAF_SUCCESS;
    }

    // Query control status via closed-source plugin
    int32_t queryResult = QueryControlAppStatusInternal(callerAppInfo, appInfoList, resultList);
    if (queryResult != SAF_SUCCESS) {
        LOGE("QueryControlAppStatus QueryControlAppStatusInternal failed, ret=%{public}d", queryResult);
        resultCode = queryResult;
        return SAF_SUCCESS;
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    int32_t elapsedTime = static_cast<int32_t>(duration.count());
    notify_performance_metrics(static_cast<int32_t>(appInfoList.size()), elapsedTime,
        INVALID_OS_ACCOUNT_ID, rust::String("QueryControlAppStatus"));

    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

int32_t CxxQueryStaticCliCmdPermissions(
    const rust::Vec<CxxCommandInfo>& cmds,
    uint32_t callerTokenId,
    rust::Vec<CxxCommandPermissionInfo>& result)
{
    std::vector<CommandInfo> cliCmds;
    for (const auto& cmd : cmds) {
        CommandInfo cliCmd;
        cliCmd.cmdName = std::string(cmd.cmd_name);
        cliCmd.subCmd = std::string(cmd.sub_cmd);
        cliCmds.push_back(cliCmd);
    }

    auto manager = PermissionManager::GetInstance();
    if (manager == nullptr) {
        LOGE("CxxQueryStaticCliCmdPermissions: get permissionManager instance failed");
        return SAF_ERR_NULL_PTR;
    }

    std::vector<CommandPermissionInfo> cmdPermissions;
    int32_t ret = manager->QueryStaticCliCmdPermissions(cliCmds, callerTokenId, cmdPermissions);
    if (ret != SAF_SUCCESS) {
        LOGE("CxxQueryStaticCliCmdPermissions failed, ret=%{public}d", ret);
        return ret;
    }

    for (const auto& permInfo : cmdPermissions) {
        CxxCommandPermissionInfo cxxPerm;
        cxxPerm.cmd_name = rust::String(permInfo.cmd.cmdName);
        cxxPerm.sub_cmd = rust::String(permInfo.cmd.subCmd);
        cxxPerm.permissions.reserve(permInfo.permissions.size());
        for (const auto& perm : permInfo.permissions) {
            cxxPerm.permissions.push_back(rust::String(perm));
        }
        cxxPerm.query_ret = permInfo.queryRet;
        result.push_back(cxxPerm);
    }

    return SAF_SUCCESS;
}

// Rust callable: returns the binder calling token id.
uint32_t GetCallingTokenId()
{
    return IPCSkeleton::GetCallingTokenID();
}

// Rust callable: resolves os_account_id from a token id.
// TOKEN_HAP → GetHapTokenInfo → osAccountID; TOKEN_NATIVE/other → 0.
int32_t GetOsAccountIdFromTokenId(uint32_t tokenId)
{
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (tokenType != ATokenTypeEnum::TOKEN_HAP) {
        LOGI("non-HAP token (type=%{public}d), os_account_id=0", static_cast<int32_t>(tokenType));
        return 0;
    }
    HapTokenInfo hapInfo;
    int32_t ret = AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo);
    if (ret != RET_SUCCESS) {
        LOGE("GetHapTokenInfo failed, ret=%{public}d, os_account_id=0", ret);
        return 0;
    }
    LOGI("HAP token, os_account_id=%{public}d", hapInfo.userID);
    return hapInfo.userID;
}

// ---- IPC 505-509 Service functions (TDD stubs - to be implemented in T06-T08) ----

ErrCode QueryAllDeniedPairs(
    std::vector<std::string> &deniedPairs,
    int32_t &resultCode)
{
    auto startTime = std::chrono::steady_clock::now();
    
    // Get OS account ID from foreground user (calling UID may belong to system SA with osAccountId=0)
    int32_t osAccountId = INVALID_OS_ACCOUNT_ID;
    int32_t foregroundOsAccountId = 0;
    if (GetForegroundOsAccountId(&foregroundOsAccountId)) {
        osAccountId = foregroundOsAccountId;
    } else {
        LOGE("QueryAllDeniedPairs failed to get foreground os account id");
        resultCode = SAF_ERR_INVALID_OS_ACCOUNT_ID;
        return SAF_SUCCESS;
    }

    rust::Vec<rust::String> deniedPairsRust;
    int32_t queryRet = query_all_denied_pairs(osAccountId, deniedPairsRust);
    if (queryRet != SAF_SUCCESS) {
        LOGE("QueryAllDeniedPairs query_all_denied_pairs failed, ret=%{public}d", queryRet);
        resultCode = queryRet;
        return SAF_SUCCESS;
    }

    // Convert rust::Vec<rust::String> to std::vector<std::string>
    for (const auto &pair : deniedPairsRust) {
        deniedPairs.push_back(std::string(pair));
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    int32_t elapsedTime = static_cast<int32_t>(duration.count());
    notify_performance_metrics(static_cast<int32_t>(deniedPairs.size()), elapsedTime,
        osAccountId, rust::String("QueryAllDeniedPairs"));

    LOGI("QueryAllDeniedPairs success, deniedPairs size=%{public}zu", deniedPairs.size());
    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

ErrCode RegisterComponentClickCallback(
    uint64_t sessionId,
    const GuiCallback &callback,
    int32_t &resultCode)
{
    if (callback == nullptr) {
        LOGE("RegisterComponentClickCallback failed, callback is null");
        resultCode = SAF_ERR_NULL_PTR;
        return SAF_SUCCESS;
    }

    int32_t ret = GuiCallbackRegistry::GetInstance().Register(sessionId, callback);
    if (ret != SAF_SUCCESS) {
        LOGE("RegisterComponentClickCallback GuiCallbackRegistry::Register failed, ret=%{public}d", ret);
        resultCode = ret;
        return SAF_SUCCESS;
    }

    LOGI("RegisterComponentClickCallback success, sessionId=%{public}llu",
        static_cast<unsigned long long>(sessionId));
    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

ErrCode ReportComponentClickDecision(
    uint64_t sessionId,
    const ComponentClickResult &result,
    int32_t &resultCode)
{
    GuiCallback callback = GuiCallbackRegistry::GetInstance().Find(sessionId);
    if (callback == nullptr) {
        LOGE("ReportComponentClickDecision failed, sessionId not found, sessionId=%{public}llu",
            static_cast<unsigned long long>(sessionId));
        resultCode = SAF_ERR_GUI_SESSION_NOT_FOUND;
        return SAF_SUCCESS;
    }

    if (result.compId.empty()) {
        LOGE("ReportComponentClickDecision failed, compId is empty");
        resultCode = SAF_ERR_ARG_EMPTY;
        return SAF_SUCCESS;
    }

    callback->OnComponentClickDecision(result);
    LOGI("ReportComponentClickDecision success, sessionId=%{public}llu, compId=%{public}s",
        static_cast<unsigned long long>(sessionId), result.compId.c_str());
    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

ErrCode UnregisterComponentClickCallback(
    uint64_t sessionId,
    int32_t &resultCode)
{
    int32_t ret = GuiCallbackRegistry::GetInstance().Unregister(sessionId);
    if (ret != SAF_SUCCESS) {
        LOGE("UnregisterComponentClickCallback failed, ret=%{public}d", ret);
        resultCode = ret;
        return SAF_SUCCESS;
    }

    LOGI("UnregisterComponentClickCallback success, sessionId=%{public}llu",
        static_cast<unsigned long long>(sessionId));
    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

ErrCode StoreOperationRecord(
    const AppInfo &callerApp,
    const AppInfo &targetApp,
    ControlDecision decision,
    int32_t &resultCode)
{
    auto startTime = std::chrono::steady_clock::now();

    if (callerApp.bundleName.empty()) {
        LOGE("StoreOperationRecord failed, callerApp.bundleName is empty");
        resultCode = SAF_ERR_ARG_EMPTY;
        return SAF_SUCCESS;
    }
    if (targetApp.bundleName.empty()) {
        LOGE("StoreOperationRecord failed, targetApp.bundleName is empty");
        resultCode = SAF_ERR_ARG_EMPTY;
        return SAF_SUCCESS;
    }

    int32_t osAccountId = INVALID_OS_ACCOUNT_ID;
    int32_t callingUid = IPCSkeleton::GetCallingUid();
    int32_t userId = 0;
    if (GetOsAccountIdFromUid(callingUid, &userId)) {
        osAccountId = userId;
    } else {
        LOGE("StoreOperationRecord failed to get os account id from uid=%{public}d", callingUid);
    }

    int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    int32_t storeRet = store_operation_record(
        callerApp.bundleName,
        callerApp.index,
        targetApp.bundleName,
        targetApp.index,
        static_cast<int32_t>(decision),
        timestamp,
        osAccountId);
    if (storeRet != SAF_SUCCESS) {
        LOGE("StoreOperationRecord store_operation_record failed, ret=%{public}d (logging only)", storeRet);
        // Do not return error - logging failure does not affect the caller
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    int32_t elapsedTime = static_cast<int32_t>(duration.count());
    notify_performance_metrics(1, elapsedTime, osAccountId, rust::String("StoreOperationRecord"));

    LOGI("StoreOperationRecord success, callerApp=%{public}s, targetApp=%{public}s, decision=%{public}d",
        callerApp.bundleName.c_str(), targetApp.bundleName.c_str(), static_cast<int32_t>(decision));
    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

ErrCode GetControlDialogBundleName(std::string &bundleName, int32_t &resultCode)
{
    LOGI("GetControlDialogBundleName enter");
    rust::String rustBundleName;
    int32_t ret = get_control_dialog_bundle_name(rustBundleName);
    if (ret != SAF_SUCCESS) {
        LOGE("GetControlDialogBundleName failed, ret=%{public}d", ret);
        resultCode = ret;
        return SAF_SUCCESS;
    }
    bundleName = std::string(rustBundleName);
    LOGI("GetControlDialogBundleName success, bundleName=%{public}s", bundleName.c_str());
    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

bool IsCallbackRegistryEmpty()
{
    return GuiCallbackRegistry::GetInstance().IsEmpty();
}

} // namespace SAF
} // namespace Security
} // namespace OHOS
