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
#include <unordered_set>
#include <unordered_map>

#include "accesstoken_kit.h"
#include "cli_tool_mgr_client.h"
#include "ipc_skeleton.h"
#include "permission_manager.h"
#include "os_account_wrapper.h"
#include "screen_lock_wrapper.h"

#include "saf_log.h"
#include "saf_defines.h"
#include "saf_result_code.h"
#include "secure_access_fence_type.h"
#include "wrapper.rs.h"
#include "cJSON.h"

namespace OHOS::Security::SAF {

namespace {
    bool getAuthStatus(PermissionStatus permissionStatus, PolicyStatus policyStatus, AuthStatus& authStatus)
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

    int32_t VerifyRemoteTicket(const PermissionQuery &permissionQuery)
    {
        if (permissionQuery.domainId.empty() ||
            permissionQuery.remoteInfo.remoteControlParams.remoteControlTicket.empty()) {
            LOGE("permissionQuery domainId or remoteControlTicket is empty");
            return SAF_ERR_ARG_INVALID;
        }
        rust::String domainId = permissionQuery.domainId;
        rust::String remoteControlTicket = permissionQuery.remoteInfo.remoteControlParams.remoteControlTicket;
        return verify_remote_ticket(domainId, remoteControlTicket);
    }
}

using namespace CliTool;

constexpr int32_t MIN_OS_ACCOUNT_ID = 100;

constexpr uint32_t DEFAULT_CALLER_TOKEN_ID = 0;

constexpr uint64_t DEFAULT_TICKET_EXPIRE_TIME_MS = 10000;  // 默认ticket过期时间

constexpr uint64_t MAX_TICKET_EXPIRE_TIME_MS = 24 * 60 * 60 * 1000;   // 最大ticket过期时间：24小时

constexpr int32_t MAX_PERMISSION_NAME_LENGTH = 256;

constexpr int32_t ERROR_CODE_NO_NETWORK = 0x19003;

constexpr int32_t ERROR_CODE_ACCOUNT_NOT_LOGGED_IN = 0x19004;

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

int32_t PermissionManager::VerifyPermissionListStatus(const std::vector<PermissionInfo> &permissionInfos)
{
    LOGI("VerifyPermissionListStatus, permissionList length = %{public}zu", permissionInfos.size());
    for (const auto &permInfo : permissionInfos) {
        if (permInfo.permissionStatus != PermissionStatus::GRANTED) {
            LOGE("VerifyPermissionListStatus :: permissionStatus is %{public}d",
                static_cast<int32_t>(permInfo.permissionStatus));
            return SAF_ERR_ARG_INVALID;
        }
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
    if (ret != SAF_SUCCESS) {
        LOGE("BatchQueryPermissionBySubCommand failed, ret=%{public}d", ret);
        return SAF_ERR_TOOL_ERROR;
    }
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

int32_t PermissionManager::CheckNeedUnlockScreen(
    const std::vector<TicketCliInfo> &ticketCliInfos,
    bool &needUnlock, bool &isScreenLocked)
{
    needUnlock = false;
    for (const auto &cliInfo : ticketCliInfos) {
        if (!cliInfo.isLockScreenExecutionAllowed) {
            needUnlock = true;
            break;
        }
    }

    if (!needUnlock) {
        isScreenLocked = false;
        return SAF_SUCCESS;
    }

    return IsScreenLocked(&isScreenLocked);
}

bool PermissionManager::IsProcessLockScreenSuccess(
    const std::vector<TicketCliInfo> &ticketCliInfos,
    bool &needUnlock)
{
    bool isScreenLocked = false;
    int32_t ret = CheckNeedUnlockScreen(ticketCliInfos, needUnlock, isScreenLocked);
    if (ret != SAF_SUCCESS || (needUnlock && isScreenLocked)) {
        return false;
    }
    return true;
}

int32_t PermissionManager::ProcessOperations(const std::vector<OperationInfo> &operationInfos,
    std::vector<CommandInfo> &cliInfos, std::vector<std::string> &apiPermissions)
{
    for (const auto &opInfo : operationInfos) {
        switch (opInfo.operationType) {
            case OperationType::CLI: {
                if (opInfo.cliCmdInfo.cmdName.empty()) {
                    LOGE("ProcessOperations failed, cliCmdInfo is invalid");
                    return SAF_ERR_ARG_INVALID;
                }
                cliInfos.push_back(opInfo.cliCmdInfo);
                break;
            }
            case OperationType::API: {
                if (opInfo.permission.empty() || ExceedsPermissionLengthLimit(opInfo.permission)) {
                    LOGE("ProcessOperations failed, api permission is empty");
                    return SAF_ERR_ARG_INVALID;
                }
                apiPermissions.push_back(opInfo.permission);
                break;
            }
            default: {
                LOGE("ProcessOperations failed, Unknown operation type: %{public}d", opInfo.operationType);
                return SAF_ERR_ARG_INVALID;
            }
        }
    }
    return SAF_SUCCESS;
}

int32_t PermissionManager::MergePermissionLists(const std::vector<TicketCliInfo> &ticketCliInfos,
    const std::vector<std::string> &apiPermissions, std::vector<std::string> &allPermissions)
{
    std::unordered_set<std::string> permissionSet;
    for (const auto &cliInfo : ticketCliInfos) {
        permissionSet.insert(cliInfo.permissions.begin(), cliInfo.permissions.end());
    }
    permissionSet.insert(apiPermissions.begin(), apiPermissions.end());
    allPermissions = std::vector<std::string>(permissionSet.begin(), permissionSet.end());
    return SAF_SUCCESS;
}

int32_t PermissionManager::BatchVerifyPermissions(const std::vector<std::string> &allPermissions,
    uint32_t callerTokenId, std::vector<PermissionInfo> &permissionInfos)
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

int32_t PermissionManager::GetPolicyAuthStatus(const std::vector<std::string> &permissions,
    std::vector<int32_t> &policyStatuses)
{
    policyStatuses.clear();
    policyStatuses.reserve(permissions.size());
    rust::Vec<rust::String> rustPermissions;
    rust::Vec<int32_t> rustPolicyStatuses;
    for (const auto &permission : permissions) {
        rustPermissions.push_back(rust::String(permission));
    }
    int32_t ret = get_policy_auth_status(rustPermissions, rustPolicyStatuses);
    IF_ERROR_LOGE_RETURN(ret, "get_policy_auth_status failed, ret = %{public}d", ret);

    LOGI("get_policy_auth_status success, rustPolicyStatuses size = %{public}zu", rustPolicyStatuses.size());
    for (const auto &rustPolicyStatus : rustPolicyStatuses) {
        LOGI("GetPolicyAuthStatus : rustPolicyStatus = %{public}d", rustPolicyStatus);
        policyStatuses.push_back(rustPolicyStatus);
    }
    return SAF_SUCCESS;
}

int32_t PermissionManager::MergePermissionResults(const std::vector<PermissionInfo> &permissionInfos,
    PermissionQueryResult &permissionQueryResult)
{
    std::vector<int32_t> policyStatuses;
    std::vector<std::string> permissionNames;
    for (auto &permissionInfo : permissionInfos) {
        permissionNames.push_back(permissionInfo.permission);
    }
    int32_t ret = GetPolicyAuthStatus(permissionNames, policyStatuses);
    IF_ERROR_LOGE_RETURN(ret, "GetPolicyAuthStatus failed, ret=%{public}d", ret);

    permissionQueryResult.permissionResults.clear();
    bool needDialog = false;
    for (size_t i = 0; i < permissionInfos.size(); ++i) {
        PermissionInfo info = {};
        info.permission = permissionInfos[i].permission;
        info.permissionStatus = permissionInfos[i].permissionStatus;
        info.authStatusInfo.flag = permissionInfos[i].authStatusInfo.flag;
        AuthStatus outStatus = AuthStatus::FORBIDDEN;
        if (policyStatuses[i] < static_cast<int32_t>(PolicyStatus::NOT_EXIST) ||
            policyStatuses[i] > static_cast<int32_t>(PolicyStatus::REMOTE_RESTRICTED)) {
            LOGE("Invalid policyStatus[%{public}zu]: %{public}d", i, policyStatuses[i]);
            return SAF_ERROR;
        }
        // 聚合ATM的PermissionStatus和策略文件的PolicyStatus，得到AuthStatus
        bool result = getAuthStatus(info.permissionStatus, static_cast<PolicyStatus>(policyStatuses[i]), outStatus);
        if (!result) {
            LOGE("getAuthStatus failed, the combination is undefined, AuthStatus is not exist");
            return SAF_ERROR;
        }
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
    bool retFlag = GetForegroundOsAccountId(&osAccountId);
    IF_FALSE_LOGE_RETURN_ERR(retFlag, SAF_ERROR, "GenerateTicketInfoWithTimeStamp :: GetForegroundOsAccountId failed");
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
        IF_TRUE_LOGE_RETURN_ERR(resultCode == ERROR_CODE_NO_NETWORK, SAF_ERR_NO_NETWORK, "No network");
        IF_TRUE_LOGE_RETURN_ERR(resultCode == ERROR_CODE_ACCOUNT_NOT_LOGGED_IN,
            ERROR_CODE_ACCOUNT_NOT_LOGGED_IN, "Account not logged in");
        return SAF_ERROR;
    }

    // Convert first Rust VerifyTicketInfo to C++ VerifyTicketInfo
    const OHOS::Security::SAF::CxxVerifyTicketInfo &cxxVerifyTicketInfo = rustResults[0];
    ticketInfo.message = std::string(cxxVerifyTicketInfo.message);
    ticketInfo.challenge = std::string(cxxVerifyTicketInfo.challenge);
    ticketInfo.ticket = std::string(cxxVerifyTicketInfo.ticket);
    ticketInfo.ticket = BuildTicketWrapper(ticketInfo);
    return SAF_SUCCESS;
}

int32_t PermissionManager::ProcessTicketInfo(const PermissionQuery &permissionQuery,
    const std::vector<TicketCliInfo> &ticketCliInfos, const std::vector<std::string> &apiPermissions,
    bool ticketMsgNeedLock, PermissionQueryResult &permissionQueryResult)
{
    permissionQueryResult.hasTicket = false;
    if (permissionQueryResult.needDialog || !permissionQuery.needTicket) {
        LOGE("ProcessTicketInfo :: Don't Generate TicketInfo, needDialog = %{public}s, needTicket = %{public}s",
            permissionQueryResult.needDialog ? "true" : "false", permissionQuery.needTicket ? "true" : "false");
        return SAF_SUCCESS;
    }
    IF_TRUE_LOGE_RETURN_ERR(ExceedsMaxExpireTimeLimit(permissionQuery.ticketExpireTimeMs),
        SAF_ERR_ARG_INVALID, "ticketExpireTimeMs is invalid");

    TicketMessageInfo ticketMessageInfo;
    for (const auto &cliInfo : ticketCliInfos) {
        ticketMessageInfo.cliInfos.push_back(cliInfo);
        if (ticketMsgNeedLock && !cliInfo.isLockScreenExecutionAllowed) {
            ticketMessageInfo.needUnlockScreen = true;
        }
    }
    auto now = std::chrono::system_clock::now();
    ticketMessageInfo.startTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    ticketMessageInfo.ticketExpireTimeMs = permissionQuery.ticketExpireTimeMs;
    ticketMessageInfo.apiPermissions = apiPermissions;
    ticketMessageInfo.callerTokenId = GetValidCallingTokenId(permissionQuery.callerTokenId);
    ticketMessageInfo.domainId = permissionQuery.domainId;
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

int32_t PermissionManager::RequestToolPermissions(const PermissionQuery &permissionQuery,
    PermissionQueryResult &permissionQueryResult)
{
    std::vector<CommandInfo> cliInfos;
    std::vector<std::string> apiPermissions;
    // 判断 PermissionQuery中是否有 RemoteInfo 有的话添加remote ticket的判断
    int32_t ret = SAF_SUCCESS;
    bool needSetScreenLock = true;
    if (!IsRemoteInfoEmpty(permissionQuery.remoteInfo)) {
        ret = VerifyRemoteTicket(permissionQuery);
        IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: VerifyRemoteTicket failed, ret=%{public}d", ret);
        needSetScreenLock = false;
    }
    ret = ProcessOperations(permissionQuery.operationInfo, cliInfos, apiPermissions);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: ProcessOperations failed, ret=%{public}d", ret);

    std::vector<TicketCliInfo> ticketCliInfos;
    if (cliInfos.empty()) {
        LOGI("RequestToolPermissions :: CLI Infos is empty");
    } else {
        ret = BatchQueryCommandPermission(cliInfos, ticketCliInfos);
        IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: BatchQueryCommandPermission failed, ret=%{public}d", ret);
    }

    std::vector<std::string> allPermissions;
    ret = MergePermissionLists(ticketCliInfos, apiPermissions, allPermissions);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: MergePermissionLists failed, ret=%{public}d", ret);

    uint32_t tokenId = permissionQuery.callerTokenId;
    std::vector<PermissionInfo> permissionInfos;
    ret = BatchVerifyPermissions(allPermissions, tokenId, permissionInfos);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: BatchVerifyPermissions failed, ret=%{public}d", ret);

    ret = MergePermissionResults(permissionInfos, permissionQueryResult);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: MergePermissionResults failed, ret=%{public}d", ret);

    // remote ticket如果判断过，则后续不需要判断锁屏
    if (needSetScreenLock) {
        bool needUnlock = false;
        bool isScreenLocked = false;
        ret = CheckNeedUnlockScreen(ticketCliInfos, needUnlock, isScreenLocked);
        IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: CheckNeedUnlockScreen failed, ret=%{public}d", ret);
        if (needUnlock && isScreenLocked) {
            LOGE("RequestToolPermissions :: Screen locked and commands require unlock");
            return SAF_ERR_SCREENLOCK_IS_LOCKED;
        }
    }

    ret = ProcessTicketInfo(permissionQuery, ticketCliInfos, apiPermissions, needSetScreenLock, permissionQueryResult);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: ProcessTicketInfo failed, ret=%{public}d", ret);

    return SAF_SUCCESS;
}

int32_t PermissionManager::GrantToolPermissionsByUser(const std::vector<UserAuthResult> &userAuthResults,
    std::vector<VerifyTicketInfo> &ticketInfos)
{
    InitTicketInfos(userAuthResults, ticketInfos);
    int32_t ret = SAF_SUCCESS;
    for (size_t i = 0; i < userAuthResults.size(); ++i) {
        IF_TRUE_LOGW_CONTINUE(userAuthResults[i].permissionQuery.callerTokenId < 0, "callerTokenId is invalid");
        IF_TRUE_LOGW_CONTINUE(userAuthResults[i].permissionInfo.empty(), "permissionInfo is empty");
        ret = VerifyPermissionListStatus(userAuthResults[i].permissionInfo);
        IF_ERROR_LOGW_CONTINUE(ret, "Not all permissions are granted");
        std::vector<CommandInfo> cliInfos;
        std::vector<std::string> apiPermissions;
        ret = ProcessOperations(userAuthResults[i].permissionQuery.operationInfo, cliInfos, apiPermissions);
        IF_ERROR_LOGW_CONTINUE(ret, "GrantToolPermissionsByUser :: ProcessOperations failed, ret=%{public}d", ret);
        std::vector<TicketCliInfo> ticketCliInfos;
        if (cliInfos.empty()) {
            LOGI("GrantToolPermissionsByUser :: CLI Infos is empty");
        } else {
            ret = BatchQueryCommandPermission(cliInfos, ticketCliInfos);
            IF_ERROR_LOGW_CONTINUE(ret,
                "GrantToolPermissionsByUser :: BatchQueryCommandPermission failed, ret=%{public}d", ret);
        }
        bool needUnlock = false;
        if (!IsProcessLockScreenSuccess(ticketCliInfos, needUnlock)) {
            IF_ERROR_LOGW_CONTINUE(SAF_ERROR, "GrantToolPermissionsByUser Screen failed, ret=%{public}d", SAF_ERROR);
        }
        IF_TRUE_LOGW_CONTINUE(ExceedsMaxExpireTimeLimit(userAuthResults[i].permissionQuery.ticketExpireTimeMs),
            "ticketExpireTimeMs is invalid");
        VerifyTicketInfo ticketInfo;
        TicketMessageInfo ticketMessageInfo;
        for (const auto &cliInfo : ticketCliInfos) {
            ticketMessageInfo.cliInfos.push_back(cliInfo);
        }
        ticketMessageInfo.needUnlockScreen = needUnlock;
        auto now = std::chrono::system_clock::now();
        ticketMessageInfo.startTime = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
        ticketMessageInfo.ticketExpireTimeMs = userAuthResults[i].permissionQuery.ticketExpireTimeMs;
        ticketMessageInfo.apiPermissions = apiPermissions;
        ticketMessageInfo.callerTokenId = GetValidCallingTokenId(userAuthResults[i].permissionQuery.callerTokenId);
        ticketMessageInfo.domainId = userAuthResults[i].permissionQuery.domainId;
        ret = GenerateTicketInfoWithTimeStamp(ticketMessageInfo, ticketMessageInfo.callerTokenId, ticketInfo);
        if (ret == SAF_ERR_ACCOUNT_NOT_LOGGED_IN) {
            LOGE("Account is not logged in. Stop generating ticket");
            return ret;
        }
        IF_ERROR_LOGW_CONTINUE(ret,
            "GrantToolPermissionsByUser :: GenerateTicketInfoWithTimeStamp failed, ret=%{public}d", ret);
        ticketInfos[i] = ticketInfo;
    }
    return SAF_SUCCESS;
}
}