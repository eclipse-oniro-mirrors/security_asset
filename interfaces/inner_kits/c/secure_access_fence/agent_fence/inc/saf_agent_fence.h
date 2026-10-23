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

#ifndef SAF_AGENT_FENCE_H
#define SAF_AGENT_FENCE_H

#include <vector>
#include <string>

#include "isecure_access_fence.h"
#include "secure_access_fence_type.h"

namespace OHOS::Security::SAF {

class SafAgentFence {
public:
    SafAgentFence() = default;
    ~SafAgentFence() = default;

    /**
     * @brief BatchQueryCommandPermission.
     *
     * @param cmds The command info required
     * @param cmdPermissions Output vector of cmd required permissions
     * @return Returns 0 on success, or error code on failure.
     */
    int32_t BatchQueryCommandPermission(
        const std::vector<CommandInfo> &cmds,
        std::vector<CommandPermissionInfo> &cmdPermissions);

    /**
     * @brief Batch generate tickets for multiple messages.
     *
     * @param osAccountId The OS account ID, >= 100.
     * @param callerId The token id.
     * @param messages The vector of messages for which tickets will be generated.
     * @param ticketInfos Output vector of generated VerifyTicketInfo structures.
     * @return Returns 0 on success, or error code on failure.
     */
    int32_t BatchGenerateTicket(
        int32_t osAccountId,
        const std::string &callerId,
        const std::vector<std::string> &messages,
        std::vector<VerifyTicketInfo> &ticketInfos);

    /**
     * @brief Batch verify tickets for access control.
     *
     * @param osAccountId The OS account ID, >= 100.
     * @param callerId The token id.
     * @param verifyInfos The vector of VerifyTicketInfo structures to verify.
     * @param verifyRes Output vector of verification results.
     * @return Returns 0 on success, or error code on failure.
     */
    int32_t BatchVerifyTicket(
        int32_t osAccountId,
        const std::string &callerId,
        const std::vector<VerifyTicketInfo> &verifyInfos,
        std::vector<int32_t> &verifyRes);

    /**
     * @brief Verify ticket for access control.
     *
     * @param osAccountId The OS account ID, >= 100.
     * @param callerId The token id.
     * @param verifyInfos The VerifyTicketInfo structures to verify.
     * @param verifyRes Output of verification results.
     * @return Returns 0 on success, or error code on failure.
     */
    int32_t VerifyTicket(
        int32_t osAccountId,
        const std::string &callerId,
        const std::string &verifyInfo,
        std::vector<CliInfo> &cliInfos);

    /**
     * @brief Request permissions for certain CLI or API tools.
     *
     * @param PermissionQuery The query for requesting permissions.
     * @param PermissionQueryResult The result for query.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t RequestToolPermissions(
        const PermissionQuery& permissionQuery,
        PermissionQueryResult& permissionQueryResult);

    /**
     * @brief Request ticket for CLI tools.
     *
     * @param cliCmdInfos The CLI command infos to query.
     * @param ticketOptions The ticket options (expire time, etc).
     * @param ticketQueryResult The result containing permission info and ticket.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t RequestToolTicket(
        const std::vector<CommandInfo>& cliCmdInfos,
        const TicketOptions& ticketOptions,
        TicketQueryResult& ticketQueryResult);

    /**
     * @brief Query static CLI command permissions with ATM auth status.
     *
     * @param cmds The CLI commands to query.
     * @param callerTokenId Caller token ID (0 = use IPC caller's token).
     * @param cmdPermissions The permission results for each command.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t QueryStaticCliCmdPermissions(
        const std::vector<CommandInfo>& cmds,
        int32_t callerTokenId,
        std::vector<CommandPermissionInfo>& cmdPermissions);

    /**
     * @brief Grant permissions for certain CLI or API tools.
     *
     * @param userAuthResult The vector of the results of user authorization.
     * @param ticketInfo The vector of the tickets corresponding to each userAuthResult.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t GrantToolPermissionsByUser(
        const std::vector<UserAuthResult>& userAuthResult,
        std::vector<VerifyTicketInfo>& ticketInfo);

    /**
     * @brief Generate controlled device packages for remote control scenario.
     *
     * @param permissionQuery The vector of permission queries.
     * @param remoteAuthPackage Output vector of generated remote auth packages.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t GenerateControlledDevicePackage(
        const std::vector<PermissionQuery> &permissionQuery,
        std::vector<RemoteAuthPackage> &remoteAuthPackage);

    /**
     * @brief Verify controlled device packages for remote control scenario.
     *
     * @param ticketInfo The vector of remote auth packages to verify.
     * @param verifyRes Output vector of verification results (boolean).
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t VerifyControlledDevicePackage(
        const std::vector<RemoteAuthPackage> &ticketInfo,
        std::vector<bool> &verifyRes);

    /**
     * @brief Generate controller device packages for remote control scenario.
     *
     * @param remoteUserAuthResults The vector of remote user authentication results.
     * @param remoteAuthPackage Output vector of generated remote auth packages.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t GenerateControllerDevicePackage(
        const std::vector<RemoteUserAuthResults> &remoteUserAuthResults,
        std::vector<RemoteAuthPackage> &remoteAuthPackage);

    /**
     * @brief Verify controller device packages for remote control scenario.
     *
     * @param ticketInfo The vector of remote auth packages to verify.
     * @param remoteInfo Remote information for verification.
     * @param verifyRes Output vector of verification results (boolean).
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t VerifyControllerDevicePackage(
        const std::vector<RemoteAuthPackage> &ticketInfo,
        const RemoteInfo &remoteInfo,
        std::vector<bool> &verifyRes);

    /**
     * @brief Get remote grant status.
     *
     * @param remoteGrantStatus Output remote grant status.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t GetRemoteGrantStatus(int32_t& remoteGrantStatus);

    /**
     * @brief Update remote grant status.
     *
     * @param remoteGrantStatus Remote grant status to set.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t UpdateRemoteGrantStatus(int32_t remoteGrantStatus);

    /**
     * @brief Generate a random challenge for task-level authorization.
     *
     * @param taskIdentityInfo Task identity info.
     * @param challenge Output challenge bytes (as i32 list over IPC).
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t GenerateTaskChallenge(
        const TaskIdentityInfo &taskIdentityInfo,
        std::vector<int32_t> &challenge);

    /**
     * @brief Generate a task identity for task-level authorization.
     *
     * @param taskIdentityInfo Task identity info (may carry an auth token).
     * @param taskIdentity Output generated task identity.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t GenerateTaskIdentity(
        const TaskIdentityInfo &taskIdentityInfo,
        TaskIdentity &taskIdentity);

    /**
     * @brief Revoke task identities.
     *
     * @param taskIds List of task IDs to revoke.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t RevokeTaskIdentity(const std::vector<std::string> &taskIds);

    /**
     * @brief Retrieve auth token by task id.
     *
     * @param callerToken Caller token id.
     * @param taskId Task id to retrieve auth token for.
     * @param authToken Output auth token bytes (empty for low-risk tasks).
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t RetrieveAuthTokenByTaskId(
        int32_t callerToken,
        const std::string &taskId,
        std::vector<int32_t> &authToken);

    /**
     * @brief Query the control status of simulated click operation.
     *
     * @param appInfoList List of target application info to query.
     * @param resultList Output list of control status results.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t QueryControlAppStatus(
        const std::vector<AppInfo> &appInfoList,
        std::vector<ControlAppStatusResult> &resultList);

    /**
     * @brief Get the control dialog bundle name from the closed-source side.
     *
     * @param bundleName Output bundle name of the control dialog UIExtensionAbility.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t GetControlDialogBundleName(std::string& bundleName);
};

}
#endif // SAF_AGENT_FENCE_H