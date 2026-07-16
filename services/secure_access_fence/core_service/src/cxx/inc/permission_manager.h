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

#ifndef PERMISSION_MANAGER_H
#define PERMISSION_MANAGER_H

#pragma once

#include <vector>
#include <string>
#include <map>
#include "singleton.h"
#include "secure_access_fence_type.h"

namespace OHOS::Security::SAF {

struct TicketCliInfo {
    std::string cmdName;
    std::string subCmd;
    std::vector<std::string> permissions;
    bool isLockScreenExecutionAllowed{true};
};
// Message structure used to generate tickets from C++ side.
struct TicketMessageInfo {
    uint32_t callerTokenId;
    std::vector<TicketCliInfo> cliInfos;
    std::vector<std::string> apiPermissions;
    uint64_t startTime{0};
    uint64_t ticketExpireTimeMs{0};
    bool needUnlockScreen{false};
    std::string domainId;
};

const std::map<std::pair<PermissionStatus, PolicyStatus>, AuthStatus> statusMapping = {
    // (-1, -1): FORBIDDEN
    {std::make_pair(PermissionStatus::DENIED, PolicyStatus::NOT_EXIST), AuthStatus::FORBIDDEN},
    // (-1, 0): REQUIRE_AUTH
    {std::make_pair(PermissionStatus::DENIED, PolicyStatus::REQUIRE_AUTH), AuthStatus::REQUIRE_AUTH},

    // (0, -1): AUTHORIZED
    {std::make_pair(PermissionStatus::GRANTED, PolicyStatus::NOT_EXIST), AuthStatus::AUTHORIZED},
    // (0, 0): REQUIRE_AUTH
    {std::make_pair(PermissionStatus::GRANTED, PolicyStatus::REQUIRE_AUTH), AuthStatus::REQUIRE_AUTH},
};

class PermissionManager : public OHOS::DelayedSingleton<PermissionManager> {
public:
    PermissionManager() = default;
    ~PermissionManager() = default;

    int32_t RequestToolPermissions(const PermissionQuery &permissionQuery,
        PermissionQueryResult &permissionQueryResult);

    int32_t GrantToolPermissionsByUser(const std::vector<UserAuthResult> &userAuthResults,
        std::vector<VerifyTicketInfo> &ticketInfos);

private:
    int32_t BatchQueryCommandPermission(const std::vector<CommandInfo> &cmds,
        std::vector<TicketCliInfo> &ticketCliInfos);

    int32_t CheckNeedUnlockScreen(const std::vector<TicketCliInfo> &ticketCliInfos,
        bool &needUnlock, bool &isScreenLocked);

    bool IsProcessLockScreenSuccess(const std::vector<TicketCliInfo> &ticketCliInfos,
        bool &needUnlock);

    int32_t ProcessOperations(const std::vector<OperationInfo> &operationInfos,
        std::vector<CommandInfo> &cliInfos, std::vector<std::string> &apiPermissions);

    int32_t MergePermissionLists(const std::vector<TicketCliInfo> &ticketCliInfos,
        const std::vector<std::string> &apiPermissions, std::vector<std::string> &allPermissions);

    int32_t BatchVerifyPermissions(const std::vector<std::string> &allPermissions, uint32_t callerTokenId,
        std::vector<PermissionInfo> &authResults);

    int32_t MergePermissionResults(const std::vector<PermissionInfo> &authResults,
        PermissionQueryResult &permissionQueryResult);

    int32_t VerifyPermissionInfo(const std::vector<PermissionInfo> &permissionInfos);

    int32_t SerializeTicketMessageInfo(const TicketMessageInfo &ticketMessageInfo, std::string &message);

    int32_t GenerateTicketInfoWithTimeStamp(TicketMessageInfo &ticketMessageInfo, uint32_t callerTokenId,
        VerifyTicketInfo &ticketInfo);

    int32_t GetPolicyAuthStatus(const std::vector<std::string> &permissions, std::vector<int32_t> &policyStatuses);

    int32_t ProcessTicketInfo(const PermissionQuery &permissionQuery,
        const std::vector<TicketCliInfo> &ticketCliInfos, const std::vector<std::string> &apiPermissions,
        bool ticketMsgNeedLock, PermissionQueryResult &permissionQueryResult);
    
    void InitTicketInfos(const std::vector<UserAuthResult> &userAuthResults,
        std::vector<VerifyTicketInfo> &ticketInfos);

    void GetValidPermissions(std::vector<std::string> &permissions, const std::vector<PermissionInfo> &permissionInfos);

    int32_t GetVerifyTicketInfo(const UserAuthResult &userAuthResult, VerifyTicketInfo &ticketInfo);
};

} // namespace OHOS::Security::SAF

#endif