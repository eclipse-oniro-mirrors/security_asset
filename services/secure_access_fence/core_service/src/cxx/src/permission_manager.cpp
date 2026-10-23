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
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include "accesstoken_kit.h"
#include "cli_tool_mgr_client.h"
#include "ipc_skeleton.h"
#include "permission_manager.h"
#include "os_account_wrapper.h"
#include "screen_lock_wrapper.h"
#include "time_wrapper.h"

#include "saf_log.h"
#include "saf_defines.h"
#include "saf_result_code.h"
#include "secure_access_fence_type.h"
#include "wrapper.rs.h"
#include "cJSON.h"

namespace OHOS::Security::SAF {

using namespace CliTool;

namespace {
    bool GetAuthStatus(PermissionStatus permissionStatus, PolicyStatus policyStatus, AuthStatus& authStatus)
    {
        auto it = statusMapping.find(std::make_pair(permissionStatus, policyStatus));
        if (it == statusMapping.end()) {
            return false;
        }
        authStatus = it->second;
        return true;
    }

    bool IsRemoteControlParamsEmpty(const RemoteControlParams &remoteControlParams)
    {
        return remoteControlParams.challenge.empty() && remoteControlParams.remoteControlTicket.empty() &&
            remoteControlParams.controlledDeviceName.empty() && remoteControlParams.controllerDeviceName.empty() &&
            remoteControlParams.signVerifyMsg.empty();
    }

    bool IsRemoteInfoEmpty(const RemoteInfo &remoteInfo)
    {
        return remoteInfo.remoteId.empty() && remoteInfo.domainId.empty()
            && IsRemoteControlParamsEmpty(remoteInfo.remoteControlParams);
    }

    bool IsTaskIdentityEmpty(const TaskIdentity &taskIdentity)
    {
        return taskIdentity.taskAttributes.empty() && taskIdentity.identity.empty()
            && taskIdentity.challenge.empty();
    }

    int32_t VerifyRemoteTicket(const PermissionQuery &permissionQuery)
    {
        if (permissionQuery.domainId.empty() ||
            permissionQuery.remoteInfo.remoteControlParams.remoteControlTicket.empty()) {
            LOGE("permissionQuery domainId or remoteControlTicket is empty");
            return SAF_ERR_ARG_INVALID;
        }
        int32_t osAccountId;
        int32_t callerUid = IPCSkeleton::GetCallingUid();
        bool retFlag = GetOsAccountIdFromUid(callerUid, &osAccountId);
        IF_FALSE_LOGE_RETURN_ERR(retFlag, SAF_ERROR, "VerifyRemoteTicket :: GetOsAccountIdFromUid failed");

        rust::String domainId = permissionQuery.domainId;
        rust::String remoteControlTicket = permissionQuery.remoteInfo.remoteControlParams.remoteControlTicket;
        return verify_remote_ticket(domainId, remoteControlTicket, osAccountId);
    }

    // Presence only: task identity must already have been verified by the caller.
    // Keep token presence separate from ATL validity to preserve error ordering.
    TaskAtlAttributes ParseTaskAtlAttributes(const TaskIdentity &taskIdentity)
    {
        TaskAtlAttributes attributes;
        if (taskIdentity.taskAttributes.empty()) {
            return attributes;
        }
        cJSON *root = cJSON_Parse(taskIdentity.taskAttributes.c_str());
        if (root == nullptr) {
            return attributes;
        }
        const cJSON *token = cJSON_GetObjectItemCaseSensitive(root, "authToken");
        attributes.hasAuthToken = cJSON_IsString(token) && token->valuestring != nullptr &&
            token->valuestring[0] != '\0';
        const cJSON *atl = cJSON_GetObjectItemCaseSensitive(root, "atl");
        if (cJSON_IsNumber(atl)) {
            attributes.atl = atl->valueint;
        }
        const cJSON *allowScreenLockExecution =
            cJSON_GetObjectItemCaseSensitive(root, "allowScreenLockExecution");
        if (cJSON_IsBool(allowScreenLockExecution)) {
            attributes.allowScreenLockExecution = cJSON_IsTrue(allowScreenLockExecution);
        }
        cJSON_Delete(root);
        return attributes;
    }

    // Query device state only when the effective execution policy requires unlock.
    int32_t CheckScreenLock(bool needUnlock)
    {
        if (!needUnlock) {
            return SAF_SUCCESS;
        }
        int32_t osAccountId;
        bool found = GetOsAccountIdFromUid(IPCSkeleton::GetCallingUid(), &osAccountId);
        IF_FALSE_LOGE_RETURN_ERR(found, SAF_ERROR, "CheckScreenLock :: GetOsAccountIdFromUid failed");
        bool isLocked = false;
        int32_t ret = IsScreenLocked(osAccountId, &isLocked);
        IF_ERROR_LOGE_RETURN(ret, "CheckScreenLock :: IsScreenLocked failed, ret=%{public}d", ret);
        IF_TRUE_LOGE_RETURN_ERR(isLocked, SAF_ERR_SCREENLOCK_IS_LOCKED,
            "CheckScreenLock :: requested permissions require unlock");
        return SAF_SUCCESS;
    }

    bool IsDeniedUnderScreenLock(const std::vector<std::string> &permissions,
        const std::vector<std::string> &permissionPolicy)
    {
        if (permissionPolicy.empty()) {
            return false;
        }
        const std::unordered_set<std::string> policyPermissions(
            permissionPolicy.begin(), permissionPolicy.end());
        return std::any_of(permissions.begin(), permissions.end(),
            [&policyPermissions](const std::string &permission) {
                return policyPermissions.find(permission) != policyPermissions.end();
            });
    }

    bool IsAtlLockScreenExecutionAllowed(bool policyFound,
        const std::vector<std::string> &permissions, const std::vector<std::string> &permissionPolicy)
    {
        if (!policyFound) {
            return false;
        }
        return !IsDeniedUnderScreenLock(permissions, permissionPolicy);
    }

    bool AnyCommandRequiresUnlock(const std::vector<TicketCliInfo> &cliInfos)
    {
        return std::any_of(cliInfos.begin(), cliInfos.end(),
            [](const TicketCliInfo &info) { return !info.isLockScreenExecutionAllowed; });
    }

    bool IsRemoteQuery(const PermissionQuery &permissionQuery)
    {
        return !IsRemoteInfoEmpty(permissionQuery.remoteInfo);
    }

} // namespace

constexpr int32_t MIN_OS_ACCOUNT_ID = 100;

constexpr uint32_t DEFAULT_CALLER_TOKEN_ID = 0;

constexpr uint64_t DEFAULT_TICKET_EXPIRE_TIME_MS = 10000;  // 默认ticket过期时间

constexpr uint64_t MAX_TICKET_EXPIRE_TIME_MS = 60 * 1000;   // 最大ticket过期时间：60s

constexpr int32_t MAX_PERMISSION_NAME_LENGTH = 256;

constexpr uint64_t UNSET_TICKET_EXPIRE_TIME_MS = 0;

constexpr int32_t JSON_ESCAPE_RESERVE_PADDING = 16;

constexpr char JSON_KEY_CALLER_TOKEN_ID[] = "callerTokenId";
constexpr char JSON_KEY_CLI_CMD_NAME[] = "cliCmdName";
constexpr char JSON_KEY_SUB_CLI_CMD_NAME[] = "subCliCmdName";
constexpr char JSON_KEY_PERMISSION_LIST[] = "permissionList";
constexpr char JSON_KEY_CLI_INFOS[] = "cliInfos";
constexpr char JSON_KEY_API_PERMISSIONS[] = "apiPermissions";
constexpr char JSON_KEY_START_TIME[] = "startTime";
constexpr char JSON_KEY_TICKET_EXPIRE_TIME_MS[] = "ticketExpireTimeMs";
constexpr char JSON_KEY_REMOTE_INFO[] = "remoteInfo";
constexpr char JSON_KEY_NEED_UNLOCK_SCREEN[] = "needUnlockScreen";
constexpr char JSON_KEY_TASK_IDENTITY[] = "taskIdentity";
constexpr char JSON_KEY_TASK_ATTRIBUTES[] = "taskAttributes";
constexpr char JSON_KEY_TASK_IDENTITY_IDENTITY[] = "identity";
constexpr char JSON_KEY_TASK_CHALLENGE[] = "challenge";
constexpr char JSON_EMPTY_STRING[] = "";
constexpr char JSON_ESCAPED_QUOTE[] = "\\\"";
constexpr char JSON_ESCAPED_BACKSLASH[] = "\\\\";
constexpr char JSON_CHAR_QUOTE = '"';
constexpr char JSON_CHAR_BACKSLASH = '\\';
constexpr char JSON_WRAPPER_MESSAGE_PREFIX[] = "{\"message\":\"";
constexpr char JSON_WRAPPER_CHALLENGE_PREFIX[] = "\",\"challenge\":\"";
constexpr char JSON_WRAPPER_TICKET_PREFIX[] = "\",\"ticket\":\"";
constexpr char JSON_WRAPPER_SUFFIX[] = "\"}";

static uint32_t GetValidCallingTokenId(uint32_t callerTokenId)
{
    if (callerTokenId == DEFAULT_CALLER_TOKEN_ID) {
        LOGI("GetValidCallingTokenId :: Caller token ID is not set, using IPCSkeleton::GetCallingTokenID");
        return IPCSkeleton::GetCallingTokenID();
    }
    return callerTokenId;
}

static bool ExceedsMaxExpireTimeLimit(uint64_t ticketExpireTimeMs)
{
    return ticketExpireTimeMs > MAX_TICKET_EXPIRE_TIME_MS;
}

static bool ExceedsPermissionLengthLimit(const std::string permissionName)
{
    return permissionName.size() > MAX_PERMISSION_NAME_LENGTH;
}

int32_t PermissionManager::VerifyPermissionInfo(const std::vector<PermissionInfo> &permissionInfos)
{
    LOGI("VerifyPermissionInfo, permissionList length = %{public}zu", permissionInfos.size());
    for (const auto &permInfo : permissionInfos) {
        IF_TRUE_LOGE_RETURN_ERR(permInfo.permission.empty() || ExceedsPermissionLengthLimit(permInfo.permission),
            SAF_ERR_ARG_INVALID, "VerifyPermissionInfo failed, permission is invalid");
    }
    return SAF_SUCCESS;
}

int32_t PermissionManager::BatchQueryCommandPermission(const std::vector<CommandInfo> &cmds,
    std::vector<TicketCliInfo> &ticketCliInfos)
{
    std::vector<Command> cliCmds;
    for (const auto &cmd : cmds) {
        Command cliCmd;
        cliCmd.toolName = cmd.cmdName;
        cliCmd.subCommand = cmd.subCmd;
        cliCmds.push_back(cliCmd);
    }
    std::vector<CommandPermission> cliCmdPermissions;
    int32_t ret = CliToolMGRClient::GetInstance().BatchQueryPermissionBySubCommand(cliCmds, cliCmdPermissions);
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_TOOL_ERROR, "BatchQueryPermissionBySubCommand failed, ret=%{public}d", ret);
    for (const auto &cliPerm : cliCmdPermissions) {
        TicketCliInfo cliInfo;
        cliInfo.cmdName = cliPerm.cmd.toolName;
        cliInfo.subCmd = cliPerm.cmd.subCommand;
        cliInfo.permissions = cliPerm.permissions;
        if (cliPerm.queryRet != SAF_SUCCESS) {
            LOGE("BatchQueryPermissionBySubCommand failed, ret=%{public}d", cliPerm.queryRet);
            return SAF_ERR_TOOL_ERROR;
        }
        cliInfo.isLockScreenExecutionAllowed = cliPerm.isLockScreenExecutionAllowed;
        ticketCliInfos.push_back(cliInfo);
    }
    return SAF_SUCCESS;
}

int32_t PermissionManager::ParseOperationInfos(const std::vector<OperationInfo> &operationInfos,
    std::vector<CommandInfo> &cliInfos, std::vector<std::string> &apiPermissions)
{
    for (const auto &opInfo : operationInfos) {
        switch (opInfo.operationType) {
            case OperationType::CLI: {
                IF_TRUE_LOGE_RETURN_ERR(opInfo.cliCmdInfo.cmdName.empty(), SAF_ERR_ARG_INVALID,
                    "ParseOperationInfos failed, cliCmdInfo is invalid");
                cliInfos.push_back(opInfo.cliCmdInfo);
                break;
            }
            case OperationType::API: {
                IF_TRUE_LOGE_RETURN_ERR(opInfo.permission.empty() || ExceedsPermissionLengthLimit(opInfo.permission),
                    SAF_ERR_ARG_INVALID, "ParseOperationInfos failed, api permission is invalid");
                apiPermissions.push_back(opInfo.permission);
                break;
            }
            default: {
                LOGE("ParseOperationInfos failed, Unknown operation type: %{public}d", opInfo.operationType);
                return SAF_ERR_ARG_INVALID;
            }
        }
    }
    return SAF_SUCCESS;
}

void PermissionManager::MergePermissionLists(const std::vector<TicketCliInfo> &ticketCliInfos,
    const std::vector<std::string> &apiPermissions, std::vector<std::string> &permissions)
{
    std::unordered_set<std::string> permissionSet;
    for (const auto &cliInfo : ticketCliInfos) {
        permissionSet.insert(cliInfo.permissions.begin(), cliInfo.permissions.end());
    }
    permissionSet.insert(apiPermissions.begin(), apiPermissions.end());
    permissions = std::vector<std::string>(permissionSet.begin(), permissionSet.end());
}

int32_t PermissionManager::BatchVerifyPermissions(const std::vector<std::string> &allPermissions,
    uint32_t callerTokenId, std::vector<PermissionInfo> &permissionInfos, bool skipInvalid)
{
    uint32_t tokenId = GetValidCallingTokenId(callerTokenId);
    std::vector<AccessToken::PermissionStatusDetail> resultList;
    // 调用ATM接口获取permissionStatus及authStatusInfo.flag
    int32_t ret = AccessToken::AccessTokenKit::GetPermissionStatusDetails(tokenId, allPermissions, resultList);
    IF_ERROR_LOGE_RETURN(ret, "GetPermissionStatusDetails failed, ret=%{public}d", ret);

    permissionInfos.reserve(resultList.size());
    for (const auto &permission : resultList) {
        LOGI("ATM grantStatus is %{public}d, ATM resultType is %{public}d, ATM grantFlag is %{public}d",
            permission.grantStatus, static_cast<int32_t>(permission.resultType), permission.grantFlag);
        // 不存在的权限/未声明的权限
        if (permission.resultType == AccessToken::PermissionResultType::PERMISSION_NOT_EXIST ||
            permission.resultType == AccessToken::PermissionResultType::PERMISSION_NOT_DECLARED) {
            LOGE("BatchVerifyPermissions failed, permission resultType is %{public}d", permission.resultType);
            if (skipInvalid) {
                continue;
            }
            return SAF_ERROR;
        }

        PermissionInfo info = {};
        info.permission = permission.permissionName;
        switch (permission.grantStatus) {
            case static_cast<int32_t>(PermissionStatus::DENIED):
                info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case static_cast<int32_t>(PermissionStatus::GRANTED):
                info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case static_cast<int32_t>(PermissionStatus::NOT_DETERMINED):
                info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case static_cast<int32_t>(PermissionStatus::INVALID):
                info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case static_cast<int32_t>(PermissionStatus::RESTRICTED):
                info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            default: {
                LOGE("Unknown grantStatus: %{public}d", permission.grantStatus);
                return SAF_ERROR;
            }
        }
        info.authStatusInfo.flag = permission.grantFlag;
        info.authStatusInfo.authStatus = AuthStatus::FORBIDDEN;  // 设置默认值，后续根据权限状态和策略状态映射表设置
        permissionInfos.push_back(info);
    }
    return SAF_SUCCESS;
}

int32_t PermissionManager::VerifyAggregatedPermissions(uint32_t callerTokenId,
    AggregatedPermissionInfo &aggregatedInfo)
{
    if (aggregatedInfo.permissions.empty()) {
        return SAF_SUCCESS;
    }

    uint32_t tokenId = GetValidCallingTokenId(callerTokenId);
    std::vector<AccessToken::PermissionStatusDetail> resultList;
    // 调用ATM接口获取permissionStatus及authStatusInfo.flag
    int32_t ret = AccessToken::AccessTokenKit::GetPermissionStatusDetails(tokenId, aggregatedInfo.permissions,
        resultList);
    IF_ERROR_LOGE_RETURN(ret, "GetPermissionStatusDetails failed, ret=%{public}d", ret);

    aggregatedInfo.authStatuses.reserve(resultList.size());
    for (const auto &permission : resultList) {
        LOGI("ATM grantStatus is %{public}d, ATM resultType is %{public}d, ATM grantFlag is %{public}d",
            permission.grantStatus, static_cast<int32_t>(permission.resultType), permission.grantFlag);
        // 不存在的权限/未声明的权限
        IF_TRUE_LOGE_RETURN_ERR(permission.resultType == AccessToken::PermissionResultType::PERMISSION_NOT_EXIST ||
            permission.resultType == AccessToken::PermissionResultType::PERMISSION_NOT_DECLARED, SAF_ERROR,
            "BatchVerifyPermissions failed, permission resultType is %{public}d", permission.resultType);

        PermissionInfo info = {};
        info.permission = permission.permissionName;
        switch (permission.grantStatus) {
            case static_cast<int32_t>(PermissionStatus::DENIED):
                info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case static_cast<int32_t>(PermissionStatus::GRANTED):
                info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case static_cast<int32_t>(PermissionStatus::NOT_DETERMINED):
                info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case static_cast<int32_t>(PermissionStatus::INVALID):
                info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case static_cast<int32_t>(PermissionStatus::RESTRICTED):
                info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            default: {
                LOGE("Unknown grantStatus: %{public}d", permission.grantStatus);
                return SAF_ERROR;
            }
        }
        info.authStatusInfo.flag = permission.grantFlag;
        info.authStatusInfo.authStatus = AuthStatus::FORBIDDEN;  // 设置默认值，后续根据权限状态和策略状态映射表设置
        aggregatedInfo.authStatuses.push_back(info);
    }
    return SAF_SUCCESS;
}

int32_t PermissionManager::GetPolicyAuthStatus(const std::vector<std::string> &permissions,
    std::vector<int32_t> &policyStatuses, bool isRemote, bool isPublicCli)
{
    policyStatuses.clear();
    policyStatuses.reserve(permissions.size());
    rust::Vec<rust::String> rustPermissions;
    for (const auto &permission : permissions) {
        rustPermissions.push_back(rust::String(permission));
    }
    rust::Vec<int32_t> rustPolicyStatuses;
    int32_t ret = get_policy_auth_status(rustPermissions, rustPolicyStatuses, isRemote, isPublicCli);
    IF_ERROR_LOGE_RETURN(ret, "get_policy_auth_status failed, ret = %{public}d", ret);

    LOGI("get_policy_auth_status success, rustPolicyStatuses size = %{public}zu",
        rustPolicyStatuses.size());
    for (const auto &rustPolicyStatus : rustPolicyStatuses) {
        LOGI("GetPolicyAuthStatus : rustPolicyStatus = %{public}d", rustPolicyStatus);
        policyStatuses.push_back(rustPolicyStatus);
    }
    return SAF_SUCCESS;
}

int32_t PermissionManager::GetAtlPermissionPolicy(int32_t atl, bool &policyFound,
    std::vector<std::string> &permissionPolicy)
{
    policyFound = false;
    permissionPolicy.clear();
    // Invalid/missing ATL never grants lock-screen execution, even with an authToken.
    if (atl <= 0) {
        return SAF_SUCCESS;
    }
    rust::Vec<rust::String> rustPolicy;
    int32_t ret = get_atl_permission_policy(atl, policyFound, rustPolicy);
    IF_ERROR_LOGE_RETURN(ret, "get_atl_permission_policy failed, ret=%{public}d", ret);
    permissionPolicy.reserve(rustPolicy.size());
    for (const auto &permission : rustPolicy) {
        permissionPolicy.emplace_back(permission.data(), permission.size());
    }
    return SAF_SUCCESS;
}

int32_t PermissionManager::MergePermissionResults(const std::vector<PermissionInfo> &permissionInfos,
    PermissionQueryResult &permissionQueryResult, const std::vector<int32_t> &policyStatuses)
{
    permissionQueryResult.permissionResults.clear();
    bool needDialog = false;
    for (size_t i = 0; i < permissionInfos.size(); ++i) {
        PermissionInfo info = {};
        info.permission = permissionInfos[i].permission;
        info.permissionStatus = permissionInfos[i].permissionStatus;
        info.authStatusInfo.flag = permissionInfos[i].authStatusInfo.flag;
        AuthStatus outStatus = AuthStatus::FORBIDDEN;
        IF_TRUE_LOGE_RETURN_ERR(policyStatuses[i] < static_cast<int32_t>(PolicyStatus::NOT_EXIST) ||
            policyStatuses[i] > static_cast<int32_t>(PolicyStatus::REMOTE_RESTRICTED), SAF_ERROR,
            "Invalid policyStatus[%{public}zu]: %{public}d", i, policyStatuses[i]);

        // 聚合ATM的PermissionStatus和策略文件的PolicyStatus，得到AuthStatus
        bool result = GetAuthStatus(info.permissionStatus, static_cast<PolicyStatus>(policyStatuses[i]), outStatus);
        IF_FALSE_LOGE_RETURN_ERR(result, SAF_ERROR,
            "GetAuthStatus failed, the combination is undefined, AuthStatus is not exist");

        info.authStatusInfo.authStatus = outStatus;
        if (outStatus != AuthStatus::AUTHORIZED) {
            LOGE("MergePermissionResults :: Need Dialog! permission[%{public}zu] authStatus is %{public}u", i,
                outStatus);
            needDialog = true;
        }
        permissionQueryResult.permissionResults.push_back(info);
    }
    permissionQueryResult.needDialog = needDialog;
    return SAF_SUCCESS;
}

static int32_t AddPermissionsToArray(const std::vector<std::string> &permissions, cJSON *permArray)
{
    for (const auto &permission : permissions) {
        cJSON *permItem = cJSON_CreateString(permission.c_str());
        if (permItem == nullptr) {
            LOGE("Create permItem string failed");
            return SAF_ERR_NULL_PTR;
        }
        if (!cJSON_AddItemToArray(permArray, permItem)) {
            LOGE("Add permItem to array failed");
            cJSON_Delete(permItem);
            return SAF_ERR_NULL_PTR;
        }
    }
    return SAF_SUCCESS;
}

static int32_t BuildCliInfoObject(const TicketCliInfo &cliInfo, cJSON **cliInfoObjOut)
{
    cJSON *cliInfoObj = cJSON_CreateObject();
    if (cliInfoObj == nullptr) {
        LOGE("Create cliInfoObj failed");
        return SAF_ERR_NULL_PTR;
    }

    if (!cJSON_AddStringToObject(cliInfoObj, JSON_KEY_CLI_CMD_NAME, cliInfo.cmdName.c_str())) {
        LOGE("Add cli cmdName failed");
        cJSON_Delete(cliInfoObj);
        return SAF_ERR_NULL_PTR;
    }

    if (!cJSON_AddStringToObject(cliInfoObj, JSON_KEY_SUB_CLI_CMD_NAME, cliInfo.subCmd.c_str())) {
        LOGE("Add sub cmdName failed");
        cJSON_Delete(cliInfoObj);
        return SAF_ERR_NULL_PTR;
    }

    cJSON *permArray = cJSON_CreateArray();
    if (permArray == nullptr) {
        LOGE("Create permArray failed");
        cJSON_Delete(cliInfoObj);
        return SAF_ERR_NULL_PTR;
    }

    int32_t ret = AddPermissionsToArray(cliInfo.permissions, permArray);
    if (ret != SAF_SUCCESS) {
        cJSON_Delete(permArray);
        cJSON_Delete(cliInfoObj);
        return ret;
    }

    if (!cJSON_AddItemToObject(cliInfoObj, JSON_KEY_PERMISSION_LIST, permArray)) {
        LOGE("Add permission list to cliInfoObj failed");
        cJSON_Delete(permArray);
        cJSON_Delete(cliInfoObj);
        return SAF_ERR_NULL_PTR;
    }

    *cliInfoObjOut = cliInfoObj;
    return SAF_SUCCESS;
}

static int32_t AddCliInfosToArray(const std::vector<TicketCliInfo> &cliInfos, cJSON *cliInfosArray)
{
    cJSON *cliInfoObj = nullptr;
    int32_t ret = SAF_SUCCESS;

    for (const auto &cliInfo : cliInfos) {
        ret = BuildCliInfoObject(cliInfo, &cliInfoObj);
        if (ret != SAF_SUCCESS) {
            break;
        }

        if (!cJSON_AddItemToArray(cliInfosArray, cliInfoObj)) {
            LOGE("Add cliInfoObj to array failed");
            ret = SAF_ERR_NULL_PTR;
            break;
        }
        cliInfoObj = nullptr;
    }

    if (cliInfoObj != nullptr) {
        cJSON_Delete(cliInfoObj);
    }
    return ret;
}

static int32_t BuildCliInfosArray(const TicketMessageInfo &ticketMessageInfo, cJSON *root)
{
    cJSON *cliInfosArray = cJSON_CreateArray();
    if (cliInfosArray == nullptr) {
        LOGE("Create cliInfosArray failed");
        return SAF_ERR_NULL_PTR;
    }

    int32_t ret = AddCliInfosToArray(ticketMessageInfo.cliInfos, cliInfosArray);
    if (ret != SAF_SUCCESS) {
        cJSON_Delete(cliInfosArray);
        return ret;
    }

    if (!cJSON_AddItemToObject(root, JSON_KEY_CLI_INFOS, cliInfosArray)) {
        LOGE("Add cliInfosArray to root failed");
        cJSON_Delete(cliInfosArray);
        return SAF_ERR_NULL_PTR;
    }
    return SAF_SUCCESS;
}

static int32_t BuildApiPermArray(const TicketMessageInfo &ticketMessageInfo, cJSON *root)
{
    cJSON *apiPermArray = cJSON_CreateArray();
    if (apiPermArray == nullptr) {
        LOGE("Create apiPermArray failed");
        return SAF_ERR_NULL_PTR;
    }

    int32_t ret = AddPermissionsToArray(ticketMessageInfo.apiPermissions, apiPermArray);
    if (ret != SAF_SUCCESS) {
        cJSON_Delete(apiPermArray);
        return ret;
    }

    if (!cJSON_AddItemToObject(root, JSON_KEY_API_PERMISSIONS, apiPermArray)) {
        LOGE("Add apiPermArray to root failed");
        cJSON_Delete(apiPermArray);
        return SAF_ERR_NULL_PTR;
    }
    return SAF_SUCCESS;
}

static int32_t AddBasicFieldsToRoot(cJSON *root, const TicketMessageInfo &ticketMessageInfo)
{
    if (!cJSON_AddNumberToObject(root, JSON_KEY_CALLER_TOKEN_ID, ticketMessageInfo.callerTokenId) ||
        !cJSON_AddNumberToObject(root, JSON_KEY_START_TIME, static_cast<double>(ticketMessageInfo.startTime)) ||
        !cJSON_AddNumberToObject(root, JSON_KEY_TICKET_EXPIRE_TIME_MS,
            static_cast<double>(ticketMessageInfo.ticketExpireTimeMs)) ||
        !cJSON_AddStringToObject(root, JSON_KEY_REMOTE_INFO, JSON_EMPTY_STRING)) {
        LOGE("Add basic fields to root failed");
        return SAF_ERR_NULL_PTR;
    }
    return SAF_SUCCESS;
}

static int32_t AddTaskIdentityToRoot(cJSON *root, const TicketMessageInfo &ticketMessageInfo)
{
    if (!ticketMessageInfo.hasTaskIdentity) {
        return SAF_SUCCESS;
    }
    cJSON *taskIdentityObj = cJSON_CreateObject();
    if (taskIdentityObj == nullptr) {
        LOGE("Create taskIdentityObj failed");
        return SAF_ERR_NULL_PTR;
    }
    if (!cJSON_AddStringToObject(taskIdentityObj, JSON_KEY_TASK_ATTRIBUTES,
        ticketMessageInfo.taskAttributes.c_str()) ||
        !cJSON_AddStringToObject(taskIdentityObj, JSON_KEY_TASK_IDENTITY_IDENTITY,
        ticketMessageInfo.identity.c_str()) ||
        !cJSON_AddStringToObject(taskIdentityObj, JSON_KEY_TASK_CHALLENGE,
        ticketMessageInfo.taskChallenge.c_str())) {
        LOGE("Add task fields to taskIdentityObj failed");
        cJSON_Delete(taskIdentityObj);
        return SAF_ERR_NULL_PTR;
    }
    if (!cJSON_AddItemToObject(root, JSON_KEY_TASK_IDENTITY, taskIdentityObj)) {
        LOGE("Add taskIdentityObj to root failed");
        cJSON_Delete(taskIdentityObj);
        return SAF_ERR_NULL_PTR;
    }
    return SAF_SUCCESS;
}

static int32_t BuildJsonRoot(const TicketMessageInfo &ticketMessageInfo, cJSON **rootOut)
{
    cJSON *root = cJSON_CreateObject();
    if (root == nullptr) {
        LOGE("Create cJSON root object failed");
        return SAF_ERR_NULL_PTR;
    }

    int32_t ret = BuildCliInfosArray(ticketMessageInfo, root);
    if (ret != SAF_SUCCESS) {
        cJSON_Delete(root);
        return ret;
    }

    ret = BuildApiPermArray(ticketMessageInfo, root);
    if (ret != SAF_SUCCESS) {
        cJSON_Delete(root);
        return ret;
    }

    ret = AddBasicFieldsToRoot(root, ticketMessageInfo);
    if (ret != SAF_SUCCESS) {
        cJSON_Delete(root);
        return ret;
    }

    if (ticketMessageInfo.needUnlockScreen) {
        if (!cJSON_AddBoolToObject(root, JSON_KEY_NEED_UNLOCK_SCREEN, true)) {
            LOGE("Add needUnlockScreen to root failed");
            cJSON_Delete(root);
            return SAF_ERR_NULL_PTR;
        }
    }

    ret = AddTaskIdentityToRoot(root, ticketMessageInfo);
    if (ret != SAF_SUCCESS) {
        cJSON_Delete(root);
        return ret;
    }

    *rootOut = root;
    return SAF_SUCCESS;
}

int32_t PermissionManager::SerializeTicketMessageInfo(const TicketMessageInfo &ticketMessageInfo, std::string &message)
{
    cJSON *root = nullptr;
    int32_t ret = BuildJsonRoot(ticketMessageInfo, &root);
    if (ret != SAF_SUCCESS) {
        return ret;
    }

    char *jsonStr = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (jsonStr == nullptr) {
        LOGE("cJSON_PrintUnformatted failed");
        return SAF_ERR_NULL_PTR;
    }

    message = jsonStr;
    cJSON_free(jsonStr);
    return SAF_SUCCESS;
}

static std::string EscapeJsonString(const std::string &input)
{
    std::string result;
    result.reserve(input.size() + JSON_ESCAPE_RESERVE_PADDING);
    for (char c : input) {
        if (c == JSON_CHAR_QUOTE) {
            result += JSON_ESCAPED_QUOTE;
        } else if (c == JSON_CHAR_BACKSLASH) {
            result += JSON_ESCAPED_BACKSLASH;
        } else {
            result += c;
        }
    }
    return result;
}

static std::string BuildTicketWrapper(const VerifyTicketInfo &ticketInfo)
{
    std::string ticketJson = JSON_WRAPPER_MESSAGE_PREFIX;
    ticketJson += EscapeJsonString(ticketInfo.message);
    ticketJson += JSON_WRAPPER_CHALLENGE_PREFIX;
    ticketJson += ticketInfo.challenge;
    ticketJson += JSON_WRAPPER_TICKET_PREFIX;
    ticketJson += ticketInfo.ticket;
    ticketJson += JSON_WRAPPER_SUFFIX;
    return ticketJson;
}

static int32_t StoreChallengeToAntiReplayStore(
    uint64_t ticketExpireTimeMs, const rust::String &callerId, const std::string &challenge)
{
    int64_t bootTimeMs = TimeWrapper::GetBootTimeMs();
    if (bootTimeMs < 0) {
        LOGE("StoreChallengeToAntiReplayStore: GetBootTimeMs failed");
        return SAF_ERROR;
    }
    uint64_t nowBootMs = static_cast<uint64_t>(bootTimeMs);
    if (nowBootMs > UINT64_MAX - ticketExpireTimeMs) {
        LOGE("StoreChallengeToAntiReplayStore: expireTimeMs overflow, "
            "nowBootMs=%{public}llu, duration=%{public}llu",
            static_cast<unsigned long long>(nowBootMs),
            static_cast<unsigned long long>(ticketExpireTimeMs));
        return SAF_ERROR;
    }
    uint64_t expireTimeMs = nowBootMs + ticketExpireTimeMs;
    int32_t storeRet = OHOS::Security::SAF::cxx_store_challenge(callerId, challenge, expireTimeMs);
    IF_TRUE_LOGE_RETURN_ERR(storeRet != SAF_SUCCESS, storeRet,
        "StoreChallengeToAntiReplayStore: cxx_store_challenge failed");
    return SAF_SUCCESS;
}

int32_t PermissionManager::GenerateTicketInfoWithTimeStamp(TicketMessageInfo &ticketMessageInfo,
    uint32_t callerTokenId, VerifyTicketInfo &ticketInfo)
{
    // Serialize ticketMessageInfo to a single message string
    std::string message;
    if (ticketMessageInfo.ticketExpireTimeMs == UNSET_TICKET_EXPIRE_TIME_MS) {
        ticketMessageInfo.ticketExpireTimeMs = DEFAULT_TICKET_EXPIRE_TIME_MS;
    }
    int32_t ret = SerializeTicketMessageInfo(ticketMessageInfo, message);
    IF_ERROR_LOGE_RETURN(ret, "SerializeTicketMessageInfo failed, ret=%{public}d", ret);
    // Prepare Rust vector of strings
    rust::Vec<rust::String> rustMessages;
    rustMessages.push_back(rust::String(message));

    int32_t osAccountId;
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    bool retFlag = GetOsAccountIdFromUid(callerUid, &osAccountId);
    IF_FALSE_LOGE_RETURN_ERR(retFlag, SAF_ERROR, "GenerateTicketInfoWithTimeStamp :: GetOsAccountIdFromUid failed");
    IF_TRUE_LOGE_RETURN_ERR(osAccountId < MIN_OS_ACCOUNT_ID, SAF_ERR_INVALID_OS_ACCOUNT_ID,
        "GenerateTicketInfoWithTimeStamp :: Invalid osAccountId");

    // Call Rust bridge to generate ticket
    rust::String callerId = rust::String(std::to_string(callerTokenId));
    rust::String domainId = rust::String(ticketMessageInfo.domainId);
    rust::Slice<const rust::String> rustMessagesSlice(rustMessages.data(), rustMessages.size());
    int32_t resultCode = SAF_SUCCESS;
    rust::Vec<OHOS::Security::SAF::CxxVerifyTicketInfo> rustResults =
        OHOS::Security::SAF::cxx_batch_generate_ticket(osAccountId, callerId, domainId, rustMessagesSlice, resultCode);

    if (resultCode != SAF_SUCCESS) {
        LOGE("GenerateTicketInfoWithTimeStamp failed, cxx_batch_generate_ticket returned no tickets");
        return resultCode;
    }

    IF_TRUE_LOGE_RETURN_ERR(rustResults.size() == 0, SAF_ERROR,
        "GenerateTicketInfoWithTimeStamp :: cxx_batch_generate_ticket returned empty result");

    // Convert first Rust VerifyTicketInfo to C++ VerifyTicketInfo
    const OHOS::Security::SAF::CxxVerifyTicketInfo &cxxVerifyTicketInfo = rustResults[0];
    ticketInfo.message = std::string(cxxVerifyTicketInfo.message);
    ticketInfo.challenge = std::string(cxxVerifyTicketInfo.challenge);
    ticketInfo.ticket = std::string(cxxVerifyTicketInfo.ticket);
    ticketInfo.ticket = BuildTicketWrapper(ticketInfo);
    ret = StoreChallengeToAntiReplayStore(
        ticketMessageInfo.ticketExpireTimeMs, callerId, ticketInfo.challenge);
    IF_ERROR_LOGE_RETURN(ret, "StoreChallengeToAntiReplayStore failed, ret=%{public}d", ret);
    return SAF_SUCCESS;
}

void PermissionManager::BuildTicketMessageInfo(const PermissionQuery &permissionQuery,
    const std::vector<TicketCliInfo> &ticketCliInfos, const std::vector<std::string> &apiPermissions,
    bool needUnlock, TicketMessageInfo &ticketMessageInfo)
{
    ticketMessageInfo.cliInfos = ticketCliInfos;
    ticketMessageInfo.needUnlockScreen = needUnlock;
    auto now = std::chrono::system_clock::now();
    ticketMessageInfo.startTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    ticketMessageInfo.ticketExpireTimeMs = permissionQuery.ticketExpireTimeMs;
    ticketMessageInfo.apiPermissions = apiPermissions;
    ticketMessageInfo.callerTokenId = GetValidCallingTokenId(permissionQuery.callerTokenId);
    ticketMessageInfo.domainId = permissionQuery.domainId;
    if (!IsTaskIdentityEmpty(permissionQuery.taskIdentity)) {
        ticketMessageInfo.hasTaskIdentity = true;
        ticketMessageInfo.taskAttributes = permissionQuery.taskIdentity.taskAttributes;
        ticketMessageInfo.identity = permissionQuery.taskIdentity.identity;
        ticketMessageInfo.taskChallenge = permissionQuery.taskIdentity.challenge;
    }
}

int32_t PermissionManager::ProcessTicketInfo(const PermissionQuery &permissionQuery,
    const std::vector<TicketCliInfo> &ticketCliInfos, const std::vector<std::string> &apiPermissions,
    bool needUnlock, PermissionQueryResult &permissionQueryResult)
{
    permissionQueryResult.hasTicket = false;
    if (permissionQueryResult.needDialog || !permissionQuery.needTicket) {
        LOGE("ProcessTicketInfo :: Don't Generate TicketInfo, needDialog = %{public}s, needTicket = %{public}s",
            permissionQueryResult.needDialog ? "true" : "false", permissionQuery.needTicket ? "true" : "false");
        return SAF_SUCCESS;
    }
    IF_TRUE_LOGE_RETURN_ERR(ExceedsMaxExpireTimeLimit(permissionQuery.ticketExpireTimeMs),
        SAF_ERR_ARG_INVALID, "ticketExpireTimeMs is invalid");

    TicketMessageInfo ticketMessageInfo{};
    BuildTicketMessageInfo(permissionQuery, ticketCliInfos, apiPermissions, needUnlock, ticketMessageInfo);
    int32_t ret = GenerateTicketInfoWithTimeStamp(ticketMessageInfo, ticketMessageInfo.callerTokenId,
        permissionQueryResult.ticket);
    IF_ERROR_LOGE_RETURN(ret, "ProcessTicketInfo :: GenerateTicketInfoWithTimeStamp failed, ret=%{public}d", ret);

    permissionQueryResult.hasTicket = true;
    return SAF_SUCCESS;
}

void PermissionManager::InitTicketInfos(const std::vector<UserAuthResult> &userAuthResults,
    std::vector<VerifyTicketInfo> &ticketInfos)
{
    ticketInfos.reserve(userAuthResults.size());
    for (const auto &userAuthResult : userAuthResults) {
        (void)userAuthResult;
        VerifyTicketInfo ticketInfo;
        ticketInfos.push_back(ticketInfo);
    }
}

void PermissionManager::GetValidPermissions(std::vector<std::string> &permissions,
    const std::vector<PermissionInfo> &permissionInfos)
{
    std::unordered_set<std::string> validPermissions;
    std::unordered_set<std::string> allPermissions;
    for (const auto &permInfo : permissionInfos) {
        allPermissions.insert(permInfo.permission);
        if (permInfo.permissionStatus == PermissionStatus::GRANTED) {
            validPermissions.insert(permInfo.permission);
        }
    }
    std::vector<std::string> filteredPermissions;
    for (const auto &perm : permissions) {
        if (validPermissions.find(perm) != validPermissions.end()) {
            filteredPermissions.push_back(perm);
        } else if (allPermissions.find(perm) == allPermissions.end()) {
            LOGW("GetValidPermissions :: Permission is not in the permissionInfos list");
        } else {
            LOGW("GetValidPermissions :: Permission is not granted");
        }
    }
    permissions = filteredPermissions;
}

int32_t PermissionManager::VerifyRemoteTicketIfNeed(const PermissionQuery &permissionQuery,
    bool &isRemoteTicketVerified)
{
    isRemoteTicketVerified = false;
    if (!IsRemoteQuery(permissionQuery)) {
        return SAF_SUCCESS;
    }
    int32_t ret = VerifyRemoteTicket(permissionQuery);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: VerifyRemoteTicket failed, ret=%{public}d", ret);
    isRemoteTicketVerified = true;
    return SAF_SUCCESS;
}

int32_t PermissionManager::QueryCommandPermissions(const std::vector<CommandInfo> &cmds,
    std::vector<TicketCliInfo> &ticketCliInfos)
{
    if (cmds.empty()) {
        LOGI("RequestToolPermissions :: CLI Infos is empty");
        return SAF_SUCCESS;
    }
    int32_t ret = BatchQueryCommandPermission(cmds, ticketCliInfos);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: BatchQueryCommandPermission failed, ret=%{public}d", ret);
    return SAF_SUCCESS;
}

int32_t PermissionManager::ConvertOperationsToPermissions(const PermissionQuery &permissionQuery,
    std::vector<std::string> &apiPermissions, std::vector<TicketCliInfo> &ticketCliInfos)
{
    std::vector<CommandInfo> cliInfos;
    int32_t ret = ParseOperationInfos(permissionQuery.operationInfo, cliInfos, apiPermissions);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: ParseOperationInfos failed, ret=%{public}d", ret);
    ret = QueryCommandPermissions(cliInfos, ticketCliInfos);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: QueryCommandPermissions failed, ret=%{public}d", ret);
    return SAF_SUCCESS;
}

int32_t PermissionManager::QueryPolicyAndMergeResults(const PermissionQuery &permissionQuery,
    AggregatedPermissionInfo &aggregatedInfo, PermissionQueryResult &permissionQueryResult, bool isPublicCli)
{
    if (aggregatedInfo.permissions.empty()) {
        return SAF_SUCCESS;
    }
    std::vector<int32_t> policyStatuses;
    int32_t ret = GetPolicyAuthStatus(aggregatedInfo.permissions, policyStatuses, IsRemoteQuery(permissionQuery),
        isPublicCli);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: GetPolicyAuthStatus failed, ret=%{public}d", ret);
    ret = MergePermissionResults(aggregatedInfo.authStatuses, permissionQueryResult, policyStatuses);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: MergePermissionResults failed, ret=%{public}d", ret);
    return SAF_SUCCESS;
}

int32_t PermissionManager::VerifyAndParseTaskIdentity(const PermissionQuery &permissionQuery,
    TaskAtlAttributes &atlAttributes)
{
    if (IsTaskIdentityEmpty(permissionQuery.taskIdentity)) {
        return SAF_SUCCESS;
    }
    int32_t ret = VerifyTaskIdentity(permissionQuery);
    IF_ERROR_LOGE_RETURN(ret, "VerifyAndParseTaskIdentity :: VerifyTaskIdentity failed, ret=%{public}d", ret);
    atlAttributes = ParseTaskAtlAttributes(permissionQuery.taskIdentity);
    return SAF_SUCCESS;
}

int32_t PermissionManager::RequestToolPermissions(const PermissionQuery &permissionQuery,
    PermissionQueryResult &permissionQueryResult, bool isPublicCli)
{
    // Step 1: PreCheck permissionQuery validity
    bool isRemoteTicketVerified = false;
    int32_t ret = VerifyRemoteTicketIfNeed(permissionQuery, isRemoteTicketVerified);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: VerifyRemoteTicketIfNeed failed, ret=%{public}d", ret);

    // Step 2: Convert and merge API & CLI permissions
    std::vector<std::string> apiPermissions;
    std::vector<TicketCliInfo> ticketCliInfos;
    ret = ConvertOperationsToPermissions(permissionQuery, apiPermissions, ticketCliInfos);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: ConvertOperationsToPermissions failed, ret=%{public}d", ret);
    AggregatedPermissionInfo aggregatedInfo;
    MergePermissionLists(ticketCliInfos, apiPermissions, aggregatedInfo.permissions);
    bool needUnlock = !isRemoteTicketVerified && AnyCommandRequiresUnlock(ticketCliInfos);

    // Step 3: Screen lock determination. If the task carries an authToken and the CLI
    // metadata already requires unlock (needUnlock), the exact ATL lock-screen policy
    // decides whether execution is allowed. Otherwise fall back to CLI metadata.
    // Screen-lock execution is only exempted when the task attributes explicitly allow it.
    TaskAtlAttributes taskAtl;
    ret = VerifyAndParseTaskIdentity(permissionQuery, taskAtl);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: VerifyAndParseTaskIdentity failed, ret=%{public}d", ret);
    bool needsAtlExemption = needUnlock && taskAtl.hasAuthToken && taskAtl.allowScreenLockExecution;
    if (needsAtlExemption) {
        bool policyFound = false;
        std::vector<std::string> permissionPolicy;
        ret = GetAtlPermissionPolicy(taskAtl.atl, policyFound, permissionPolicy);
        IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: GetAtlPermissionPolicy failed, ret=%{public}d", ret);
        needUnlock = !IsAtlLockScreenExecutionAllowed(policyFound, aggregatedInfo.permissions, permissionPolicy);
        // Keep effective CLI flags consistent with the task-wide ticket requirement.
        for (auto &cliInfo : ticketCliInfos) {
            cliInfo.isLockScreenExecutionAllowed = !needUnlock;
        }
    }
    ret = CheckScreenLock(needUnlock);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: CheckScreenLock failed, ret=%{public}d", ret);

    // Step 4: Verify permissions existence and auth results. Generate ticket info.
    ret = VerifyAggregatedPermissions(permissionQuery.callerTokenId, aggregatedInfo);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: VerifyAggregatedPermissions failed, ret=%{public}d", ret);
    ret = QueryPolicyAndMergeResults(permissionQuery, aggregatedInfo, permissionQueryResult, isPublicCli);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: QueryPolicyAndMergeResults failed, ret=%{public}d", ret);
    return ProcessTicketInfo(permissionQuery, ticketCliInfos, apiPermissions, needUnlock, permissionQueryResult);
}

int32_t PermissionManager::GrantToolPermissionsByUser(const std::vector<UserAuthResult> &userAuthResults,
    std::vector<VerifyTicketInfo> &ticketInfos)
{
    InitTicketInfos(userAuthResults, ticketInfos);
    int32_t ret = SAF_SUCCESS;
    for (size_t i = 0; i < userAuthResults.size(); ++i) {
        IF_TRUE_LOGW_CONTINUE(userAuthResults[i].permissionQuery.callerTokenId < 0, "callerTokenId is invalid");
        IF_TRUE_LOGW_CONTINUE(userAuthResults[i].permissionInfo.empty(), "permissionInfo is empty");
        IF_TRUE_LOGW_CONTINUE(ExceedsMaxExpireTimeLimit(userAuthResults[i].permissionQuery.ticketExpireTimeMs),
            "ticketExpireTimeMs is invalid");
        VerifyTicketInfo ticketInfo;
        ret = GetVerifyTicketInfo(userAuthResults[i], ticketInfo);
        if (ret == SAF_ERR_TRUSTED_RING_NOT_LOGGED_IN) {
            LOGE("Account is not logged in. GrantToolPermissions failed");
            return ret;
        }
        IF_ERROR_LOGW_CONTINUE(ret, "GrantToolPermissionsByUser :: GetVerifyTicketInfo failed, ret=%{public}d", ret);
        ticketInfos[i] = ticketInfo;
    }
    return SAF_SUCCESS;
}

int32_t PermissionManager::GetVerifyTicketInfo(const UserAuthResult &userAuthResult, VerifyTicketInfo &ticketInfo)
{
    int32_t ret = VerifyPermissionInfo(userAuthResult.permissionInfo);
    IF_ERROR_LOGE_RETURN(ret, "Invalid userAuthResult: permission is invalid");
    std::vector<CommandInfo> cliInfos;
    std::vector<std::string> apiPermissions;
    ret = ParseOperationInfos(userAuthResult.permissionQuery.operationInfo, cliInfos, apiPermissions);
    IF_ERROR_LOGE_RETURN(ret, "GetVerifyTicketInfo :: ParseOperationInfos failed, ret=%{public}d", ret);
    GetValidPermissions(apiPermissions, userAuthResult.permissionInfo);
    std::vector<TicketCliInfo> ticketCliInfos;
    if (cliInfos.empty()) {
        LOGI("GetVerifyTicketInfo :: CLI Infos is empty");
    } else {
        ret = BatchQueryCommandPermission(cliInfos, ticketCliInfos);
        IF_ERROR_LOGE_RETURN(ret,
            "GetVerifyTicketInfo :: BatchQueryCommandPermission failed, ret=%{public}d", ret);
    }
    const bool needUnlock = AnyCommandRequiresUnlock(ticketCliInfos);
    ret = CheckScreenLock(needUnlock);
    IF_ERROR_LOGE_RETURN(ret, "GetVerifyTicketInfo :: CheckScreenLock failed, ret=%{public}d", ret);

    TicketMessageInfo ticketMessageInfo;
    for (auto &cliInfo : ticketCliInfos) {
        GetValidPermissions(cliInfo.permissions, userAuthResult.permissionInfo);
        ticketMessageInfo.cliInfos.push_back(cliInfo);
    }
    ticketMessageInfo.needUnlockScreen = needUnlock;
    auto now = std::chrono::system_clock::now();
    ticketMessageInfo.startTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    ticketMessageInfo.ticketExpireTimeMs = userAuthResult.permissionQuery.ticketExpireTimeMs;
    ticketMessageInfo.apiPermissions = apiPermissions;
    ticketMessageInfo.callerTokenId = GetValidCallingTokenId(userAuthResult.permissionQuery.callerTokenId);
    ticketMessageInfo.domainId = userAuthResult.permissionQuery.domainId;
    ret = GenerateTicketInfoWithTimeStamp(ticketMessageInfo, ticketMessageInfo.callerTokenId, ticketInfo);
    if (ret == SAF_ERR_TRUSTED_RING_NOT_LOGGED_IN) {
        LOGE("Account is not logged in. Stop generating ticket");
        return ret;
    }
    IF_ERROR_LOGE_RETURN(ret,
        "GetVerifyTicketInfo :: GenerateTicketInfoWithTimeStamp failed, ret=%{public}d", ret);
    
    StoreGrantRecordIfValid(userAuthResult, ticketMessageInfo);
    
    return SAF_SUCCESS;
}

int32_t PermissionManager::VerifyTaskIdentity(const PermissionQuery &permissionQuery)
{
    int32_t osAccountId;
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    bool retFlag = GetOsAccountIdFromUid(callerUid, &osAccountId);
    IF_FALSE_LOGE_RETURN_ERR(retFlag, SAF_ERROR, "VerifyTaskIdentity :: GetOsAccountIdFromUid failed");

    std::string callerTokenId = std::to_string(IPCSkeleton::GetCallingTokenID());
    int32_t ret = OHOS::Security::SAF::cxx_verify_task_identity(
        osAccountId,
        rust::String(callerTokenId),
        rust::String(permissionQuery.taskIdentity.taskAttributes),
        rust::String(permissionQuery.taskIdentity.identity),
        rust::String(permissionQuery.taskIdentity.challenge));
    IF_ERROR_LOGE_RETURN(ret, "VerifyTaskIdentity :: cxx_verify_task_identity failed, ret=%{public}d", ret);
    return SAF_SUCCESS;
}

void PermissionManager::StoreGrantRecordIfValid(const UserAuthResult &userAuthResult,
    const TicketMessageInfo &ticketMessageInfo)
{
    if (IsRemoteInfoEmpty(userAuthResult.permissionQuery.remoteInfo)) {
        return;
    }
    
    std::vector<std::string> grantedPermissions;
    for (const auto &permInfo : userAuthResult.permissionInfo) {
        if (permInfo.permissionStatus == PermissionStatus::GRANTED) {
            grantedPermissions.push_back(permInfo.permission);
        }
    }
    
    if (grantedPermissions.empty()) {
        return;
    }
    
    std::string controlledDeviceName =
        userAuthResult.permissionQuery.remoteInfo.remoteControlParams.controlledDeviceName;
    std::string controllerDeviceName =
        userAuthResult.permissionQuery.remoteInfo.remoteControlParams.controllerDeviceName;
    
    int32_t osAccountId;
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    bool retFlag = GetOsAccountIdFromUid(callerUid, &osAccountId);
    if (!retFlag || osAccountId < MIN_OS_ACCOUNT_ID) {
        LOGE("invalid uid or osAccountId");
        return;
    }
    
    rust::Vec<rust::String> rustPermissions;
    for (const auto &perm : grantedPermissions) {
        rustPermissions.push_back(rust::String(perm));
    }
    int32_t storeRet = OHOS::Security::SAF::cxx_store_controlled_grant_record(
        osAccountId,
        rust::String(controlledDeviceName),
        rust::String(controllerDeviceName),
        rustPermissions,
        static_cast<int32_t>(ticketMessageInfo.callerTokenId)
    );
    if (storeRet != SAF_SUCCESS) {
        LOGE("StoreGrantRecordIfValid :: cxx_store_controlled_grant_record failed, ret=%{public}d", storeRet);
    }
}

int32_t PermissionManager::QueryStaticCliCmdPermissions(
    const std::vector<CommandInfo> &cliCmds,
    uint32_t callerTokenId, std::vector<CommandPermissionInfo> &cmdPermissions)
{
    // Step 1: BatchQueryCommandPermission
    std::vector<TicketCliInfo> ticketCliInfos;
    int32_t ret = BatchQueryCommandPermission(cliCmds, ticketCliInfos);
    IF_ERROR_LOGE_RETURN_ERR(ret != SAF_SUCCESS, ret,
        "QueryStaticCliCmdPermissions: BatchQueryCommandPermission failed");

    // Step 2: merge command permissions
    std::unordered_set<std::string> permissionSet;
    for (const auto &cliInfo : ticketCliInfos) {
        permissionSet.insert(cliInfo.permissions.begin(), cliInfo.permissions.end());
    }
    std::vector<std::string> allPermissions(permissionSet.begin(), permissionSet.end());

    // Step 3: BatchVerifyPermissions (with callerTokenId)
    uint32_t tokenId = GetValidCallingTokenId(callerTokenId);
    std::vector<PermissionInfo> permissionInfos;
    if (!allPermissions.empty()) {
        ret = BatchVerifyPermissions(allPermissions, tokenId, permissionInfos, true);
        IF_ERROR_LOGE_RETURN_ERR(ret != SAF_SUCCESS, ret,
            "QueryStaticCliCmdPermissions: BatchVerifyPermissions failed");
    }

    // Step 4: collect granted permissions
    std::unordered_set<std::string> grantedPermissions;
    for (const auto &info : permissionInfos) {
        if (info.permissionStatus == PermissionStatus::GRANTED) {
            grantedPermissions.insert(info.permission);
        }
    }

    // Step 5: convert TicketCliInfo -> CommandPermissionInfo (only granted permissions)
    for (const auto &cliInfo : ticketCliInfos) {
        CommandPermissionInfo permInfo;
        permInfo.cmd.cmdName = cliInfo.cmdName;
        permInfo.cmd.subCmd = cliInfo.subCmd;
        permInfo.queryRet = SAF_SUCCESS;
        for (const auto &perm : cliInfo.permissions) {
            if (grantedPermissions.count(perm) > 0) {
                permInfo.permissions.push_back(perm);
            }
        }
        cmdPermissions.push_back(permInfo);
    }
    return SAF_SUCCESS;
}
}
