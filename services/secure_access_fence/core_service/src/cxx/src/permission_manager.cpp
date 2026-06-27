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
}

using namespace CliTool;

constexpr int32_t MIN_OS_ACCOUNT_ID = 100;

constexpr uint32_t DEFAULT_CALLER_TOKEN_ID = 0;

constexpr uint64_t DEFAULT_TICKET_EXPIRE_TIME_MS = 10000;  // 默认ticket过期时间

constexpr uint64_t MAX_TICKET_EXPIRE_TIME_MS = 24 * 60 * 60 * 1000;   // 最大ticket过期时间：24小时

constexpr int32_t MAX_PERMISSION_NAME_LENGTH = 256;

constexpr int32_t ERROR_CODE_NO_NETWORK = 0x19003;

constexpr int32_t ERROR_CODE_ACCOUNT_NOT_LOGGED_IN = 0x19004;

constexpr char JSON_KEY_CALLER_TOKEN_ID[] = "callerTokenId";
constexpr char JSON_KEY_CLI_CMD_NAME[] = "cliCmdName";
constexpr char JSON_KEY_SUB_CLI_CMD_NAME[] = "subCliCmdName";
constexpr char JSON_KEY_PERMISSION_LIST[] = "permissionList";
constexpr char JSON_KEY_CLI_INFOS[] = "cliInfos";
constexpr char JSON_KEY_API_PERMISSIONS[] = "apiPermissions";
constexpr char JSON_KEY_START_TIME[] = "startTime";
constexpr char JSON_KEY_TICKET_EXPIRE_TIME_MS[] = "ticketExpireTimeMs";
constexpr char JSON_KEY_REMOTE_INFO[] = "remoteInfo";
constexpr char JSON_EMPTY_STRING[] = "";

constexpr char JSON_ESCAPED_QUOTE[] = "\\\"";
constexpr char JSON_ESCAPED_BACKSLASH[] = "\\\\";

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
    std::vector<CommandPermissionInfo> &cmdPermissionInfos) 
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
    if (ret != 0) {
        LOGE("BatchQueryPermissionBySubCommand failed, ret=%{public}d", ret);
        return SAF_ERR_TOOL_ERROR;
    }
    for (const auto &cliPerm : cliCmdPermissions) {
        CommandPermissionInfo permInfo;
        permInfo.cmd.cmdName = cliPerm.cmd.toolName;
        permInfo.cmd.subCmd = cliPerm.cmd.subCommand;
        permInfo.permissions = cliPerm.permissions;
        permInfo.queryRet = cliPerm.queryRet;
        cmdPermissionInfos.push_back(permInfo);
    }
    return SAF_SUCCESS;
}

int32_t PermissionManager::ProcessOperations(const std::vector<OperationInfo> &operationInfos,
    std::vector<CommandInfo> &cliInfos, std::vector<std::string> &apiPermissions)
{
    for (const auto &opInfo : operationInfos) {
        switch (opInfo.operationType) {
            case OperationType::CLI: {
                if (opInfo.cliCmdInfo.cmdName.empty() || opInfo.cliCmdInfo.subCmd.empty()) {
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

int32_t PermissionManager::MergePermissionLists(const std::vector<CommandPermissionInfo> &cmdPermissionInfos,
    const std::vector<std::string> &apiPermissions, std::vector<std::string> &allPermissions)
{
    std::unordered_set<std::string> permissionSet;
    for (const auto &cmdPerm : cmdPermissionInfos) {
       permissionSet.insert(cmdPerm.permissions.begin(), cmdPerm.permissions.end());
    }
    permissionSet.insert(apiPermissions.begin(), apiPermissions.end());
    allPermissions = std::vector<std::string>(permissionSet.begin(), permissionSet.end());
    return SAF_SUCCESS;
}

int32_t PermissionManager::BatchVerifyPermissions(const std::vector<std::string> &allPermissions, uint32_t callerTokenId, 
    std::vector<PermissionInfo> &permissionInfos)
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
            case -1: info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case 0: info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case 1: info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case 2: info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
            case 3: info.permissionStatus = static_cast<PermissionStatus>(permission.grantStatus); break;
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
    for (size_t i = 0; i < rustPolicyStatuses.size(); ++i) {
        LOGI("GetPolicyAuthStatus : rustPolicyStatuses[%{public}zu] = %{public}d", i, rustPolicyStatuses[i]);
        policyStatuses.push_back(rustPolicyStatuses[i]);
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
        }
        // 聚合ATM的PermissionStatus和策略文件的PolicyStatus，得到AuthStatus
        bool result = getAuthStatus(info.permissionStatus, static_cast<PolicyStatus>(policyStatuses[i]), outStatus);
        if (!result) {
            LOGE("getAuthStatus failed, the combination of PermissionStatus and PolicyStatus is undefined, AuthStatus is not exist");
            return SAF_ERROR;
        }
        info.authStatusInfo.authStatus = outStatus;
        if (outStatus != AuthStatus::AUTHORIZED) {
            LOGE("MergePermissionResults :: Need Dialog! permission[%{public}zu] authStatus is %{public}u", i, outStatus);
            needDialog = true;
        }
        permissionQueryResult.permissionResults.push_back(info);
    }
    permissionQueryResult.needDialog = needDialog;
    return SAF_SUCCESS;
}

int32_t PermissionManager::SerializeTicketMessageInfo(const TicketMessageInfo &ticketMessageInfo, std::string &message)
{
    cJSON *root = cJSON_CreateObject();
    IF_TRUE_LOGE_RETURN_ERR(root == nullptr, SAF_ERR_NULL_PTR, "Create cJSON object failed");

    if (!cJSON_AddNumberToObject(root, JSON_KEY_CALLER_TOKEN_ID, ticketMessageInfo.callerTokenId)) {
        cJSON_Delete(root);
        return SAF_ERR_NULL_PTR;
    }

    cJSON *cliInfosArray = cJSON_CreateArray();
    IF_TRUE_LOGE_RETURN_ERR(cliInfosArray == nullptr, SAF_ERR_NULL_PTR, "Create cJSON array failed");
    for (size_t i = 0; i < ticketMessageInfo.cliInfos.size(); ++i) {
        cJSON *cliInfoObj = cJSON_CreateObject();
        IF_TRUE_LOGE_RETURN_ERR(cliInfoObj == nullptr, SAF_ERR_NULL_PTR, "Create cJSON object failed");

        const CommandPermissionInfo &cliInfo = ticketMessageInfo.cliInfos[i];
        cJSON_AddStringToObject(cliInfoObj, JSON_KEY_CLI_CMD_NAME, cliInfo.cmd.cmdName.c_str());
        cJSON_AddStringToObject(cliInfoObj, JSON_KEY_SUB_CLI_CMD_NAME, cliInfo.cmd.subCmd.c_str());

        cJSON *permArray = cJSON_CreateArray();
        IF_TRUE_LOGE_RETURN_ERR(permArray == nullptr, SAF_ERR_NULL_PTR, "Create cJSON array failed");
        for (size_t j = 0; j < cliInfo.permissions.size(); ++j) {
            cJSON_AddItemToArray(permArray, cJSON_CreateString(cliInfo.permissions[j].c_str()));
        }
        cJSON_AddItemToObject(cliInfoObj, JSON_KEY_PERMISSION_LIST, permArray);
        cJSON_AddItemToArray(cliInfosArray, cliInfoObj);
    }
    cJSON_AddItemToObject(root, JSON_KEY_CLI_INFOS, cliInfosArray);

    cJSON *apiPermArray = cJSON_CreateArray();
    IF_TRUE_LOGE_RETURN_ERR(apiPermArray == nullptr, SAF_ERR_NULL_PTR, "Create cJSON array failed");
    for (size_t i = 0; i < ticketMessageInfo.apiPermissions.size(); ++i) {
        cJSON_AddItemToArray(apiPermArray, cJSON_CreateString(ticketMessageInfo.apiPermissions[i].c_str()));
    }
    cJSON_AddItemToObject(root, JSON_KEY_API_PERMISSIONS, apiPermArray);

    cJSON_AddNumberToObject(root, JSON_KEY_START_TIME, static_cast<double>(ticketMessageInfo.startTime));
    cJSON_AddNumberToObject(root, JSON_KEY_TICKET_EXPIRE_TIME_MS, static_cast<double>(ticketMessageInfo.ticketExpireTimeMs));
    cJSON_AddStringToObject(root, JSON_KEY_REMOTE_INFO, JSON_EMPTY_STRING);

    char *jsonStr = cJSON_PrintUnformatted(root);
    if (jsonStr == nullptr) {
        cJSON_Delete(root);
        return SAF_ERR_NULL_PTR;
    }
    message = jsonStr;
    cJSON_free(jsonStr);
    cJSON_Delete(root);
    return SAF_SUCCESS;
}

static std::string EscapeJsonString(const std::string &input)
{
    std::string result;
    result.reserve(input.size() + 16);
    for (char c : input) {
        if (c == '"') {
            result += JSON_ESCAPED_QUOTE;
        } else if (c == '\\') {
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
    if (ticketMessageInfo.ticketExpireTimeMs == 0) {
        ticketMessageInfo.ticketExpireTimeMs = DEFAULT_TICKET_EXPIRE_TIME_MS;
    }
    SerializeTicketMessageInfo(ticketMessageInfo, message);
    // Prepare Rust vector of strings
    rust::Vec<rust::String> rustMessages;
    rustMessages.push_back(rust::String(message));

    int32_t osAccountId;
    bool ret = GetForegroundOsAccountId(&osAccountId);
    IF_FALSE_LOGE_RETURN_ERR(ret, SAF_ERROR, "GenerateTicketInfoWithTimeStamp :: GetForegroundOsAccountId failed");
    IF_TRUE_LOGE_RETURN_ERR(osAccountId < MIN_OS_ACCOUNT_ID, SAF_ERR_INVALID_OS_ACCOUNT_ID, "GenerateTicketInfoWithTimeStamp :: Invalid osAccountId");

    // Call Rust bridge to generate ticket
    rust::String callerId = rust::String(std::to_string(callerTokenId));
    rust::Slice<const rust::String> rustMessagesSlice(rustMessages.data(), rustMessages.size());
    int32_t resultCode = SAF_SUCCESS;
    rust::Vec<OHOS::Security::SAF::CxxVerifyTicketInfo> rustResults =
        OHOS::Security::SAF::cxx_batch_generate_ticket(osAccountId, callerId, rustMessagesSlice, resultCode);

    if (resultCode != SAF_SUCCESS) {
        LOGE("GenerateTicketInfoWithTimeStamp failed, cxx_batch_generate_ticket returned no tickets");
        IF_TRUE_LOGE_RETURN_ERR(resultCode == ERROR_CODE_NO_NETWORK, SAF_ERR_NO_NETWORK, "No network");
        IF_TRUE_LOGE_RETURN_ERR(resultCode == ERROR_CODE_ACCOUNT_NOT_LOGGED_IN,
            SAF_ERR_ACCOUNT_NOT_LOGEED_IN, "Account not logged in");
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

int32_t PermissionManager::ProcessTicketInfo(const PermissionQuery &permissionQuery, const std::vector<CommandPermissionInfo> &cmdPermissionInfos,
    const std::vector<std::string> &apiPermissions, PermissionQueryResult &permissionQueryResult)
{
    permissionQueryResult.hasTicket = false;
    if (permissionQueryResult.needDialog || !permissionQuery.needTicket) {
        LOGE("ProcessTicketInfo :: Don't Generate TicketInfo, needDialog = %{public}s, needTicket = %{public}s",
        permissionQueryResult.needDialog ? "true" : "false",
        permissionQuery.needTicket ? "true" : "false");
        return SAF_SUCCESS;
    }
    IF_TRUE_LOGE_RETURN_ERR(ExceedsMaxExpireTimeLimit(permissionQuery.ticketExpireTimeMs),
        SAF_ERR_ARG_INVALID, "ticketExpireTimeMs is invalid");

    TicketMessageInfo ticketMessageInfo;
    for (const auto &cmdPermissionInfo : cmdPermissionInfos) {
        ticketMessageInfo.cliInfos.push_back(cmdPermissionInfo);
    }
    auto now = std::chrono::system_clock::now();
    ticketMessageInfo.startTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    ticketMessageInfo.ticketExpireTimeMs = permissionQuery.ticketExpireTimeMs;
    ticketMessageInfo.apiPermissions = apiPermissions;
    ticketMessageInfo.callerTokenId = GetValidCallingTokenId(permissionQuery.callerTokenId);
    int32_t ret = GenerateTicketInfoWithTimeStamp(ticketMessageInfo, ticketMessageInfo.callerTokenId, permissionQueryResult.ticket);
    IF_ERROR_LOGE_RETURN(ret, "ProcessTicketInfo :: GenerateTicketInfoWithTimeStamp failed, ret=%{public}d", ret);
    
    permissionQueryResult.hasTicket = true;
    return SAF_SUCCESS;
}

int32_t PermissionManager::RequestToolPermissions(const PermissionQuery &permissionQuery,
    PermissionQueryResult &permissionQueryResult)
{
    std::vector<CommandInfo> cliInfos;
    std::vector<std::string> apiPermissions;
    int32_t ret = ProcessOperations(permissionQuery.operationInfo, cliInfos, apiPermissions);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: ProcessOperations failed, ret=%{public}d", ret);

    std::vector<CommandPermissionInfo> cmdPermissionInfos;
    if (cliInfos.empty()) {
        LOGI("RequestToolPermissions :: CLI Infos is empty");
    } else {
        ret = BatchQueryCommandPermission(cliInfos, cmdPermissionInfos);
        IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: BatchQueryCommandPermission failed, ret=%{public}d", ret);
    }

    std::vector<std::string> allPermissions;
    ret = MergePermissionLists(cmdPermissionInfos, apiPermissions, allPermissions);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: MergePermissionLists failed, ret=%{public}d", ret);

    uint32_t tokenId = permissionQuery.callerTokenId;
    std::vector<PermissionInfo> permissionInfos;
    ret = BatchVerifyPermissions(allPermissions, tokenId, permissionInfos);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: BatchVerifyPermissions failed, ret=%{public}d", ret);

    ret = MergePermissionResults(permissionInfos, permissionQueryResult);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: MergePermissionResults failed, ret=%{public}d", ret);
    
    ret = ProcessTicketInfo(permissionQuery, cmdPermissionInfos, apiPermissions, permissionQueryResult);
    IF_ERROR_LOGE_RETURN(ret, "RequestToolPermissions :: ProcessTicketInfo failed, ret=%{public}d", ret);

    return SAF_SUCCESS;
}

int32_t PermissionManager::GrantToolPermissionsByUser(const std::vector<UserAuthResult> &userAuthResults,
    std::vector<VerifyTicketInfo> &ticketInfos)
{
    ticketInfos.reserve(userAuthResults.size());
    for (const auto &userAuthResult : userAuthResults) {
        (void)userAuthResult;
        VerifyTicketInfo ticketInfo;
        ticketInfos.push_back(ticketInfo);
    }
    int32_t ret = SAF_SUCCESS;
    bool hasValidTicket = false;   // 生成ticket成功时，置为true
    for (size_t i = 0; i < userAuthResults.size(); ++i) {
        IF_TRUE_LOGE_RETURN_ERR(userAuthResults[i].permissionQuery.callerTokenId < 0,
            SAF_ERR_ARG_INVALID, "callerTokenId is invalid");
        IF_TRUE_LOGE_RETURN_ERR(userAuthResults[i].permissionInfo.empty(), SAF_ERR_ARG_EMPTY, "permissionInfo is empty");
        ret = VerifyPermissionListStatus(userAuthResults[i].permissionInfo);
        IF_ERROR_LOGW_CONTINUE(ret, "Not all permissions are granted");

        std::vector<CommandInfo> cliInfos;
        std::vector<std::string> apiPermissions;
        ret = ProcessOperations(userAuthResults[i].permissionQuery.operationInfo, cliInfos, apiPermissions);
        IF_ERROR_LOGW_CONTINUE(ret, "GrantToolPermissionsByUser :: ProcessOperations failed, ret=%{public}d", ret);
        
        std::vector<CommandPermissionInfo> cmdPermissionInfos;
        if (cliInfos.empty()) {
            LOGI("GrantToolPermissionsByUser :: CLI Infos is empty");
        } else {
            ret = BatchQueryCommandPermission(cliInfos, cmdPermissionInfos);
            IF_ERROR_LOGW_CONTINUE(ret, "GrantToolPermissionsByUser :: BatchQueryCommandPermission failed, ret=%{public}d", ret);
        }
        IF_TRUE_LOGW_CONTINUE(ExceedsMaxExpireTimeLimit(userAuthResults[i].permissionQuery.ticketExpireTimeMs),
            "ticketExpireTimeMs is invalid");
        VerifyTicketInfo ticketInfo;
        TicketMessageInfo ticketMessageInfo;
        for (const auto &cmdPermissionInfo : cmdPermissionInfos) {
            ticketMessageInfo.cliInfos.push_back(cmdPermissionInfo);
        }
        auto now = std::chrono::system_clock::now();
        ticketMessageInfo.startTime = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
        ticketMessageInfo.ticketExpireTimeMs = userAuthResults[i].permissionQuery.ticketExpireTimeMs;
        ticketMessageInfo.apiPermissions = apiPermissions;
        ticketMessageInfo.callerTokenId = GetValidCallingTokenId(userAuthResults[i].permissionQuery.callerTokenId);
        ret = GenerateTicketInfoWithTimeStamp(ticketMessageInfo, ticketMessageInfo.callerTokenId, ticketInfo);
        IF_ERROR_LOGW_CONTINUE(ret, "GrantToolPermissionsByUser :: GenerateTicketInfoWithTimeStamp failed, ret=%{public}d", ret);
        hasValidTicket = true;
        ticketInfos[i] = ticketInfo;
    }
    IF_FALSE_LOGE_RETURN_ERR(hasValidTicket, SAF_ERROR, "don't generate valid ticket");
    return SAF_SUCCESS;
}
}