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
#include "saf_gui_fence.h"
#include "gui_callback_registry.h"

namespace OHOS {
namespace Security {
namespace SAF {

constexpr const char* CONTROL_DEVICE_PERMISSION = "ohos.permission.CONTROL_DEVICE";

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

/**
 * @brief Get caller app info from IPC context.
 * @param callerAppInfo Output caller app info (bundleName + index).
 * @return Returns 0 on success, or error code on failure.
 */
int32_t GetCallerAppInfo(AppInfo &callerAppInfo);

/**
 * @brief Validate the appInfoList: bundleName non-empty and exists, index >= 0.
 * @param appInfoList List of AppInfo to validate.
 * @return Returns 0 on success, or error code on failure.
 */
int32_t ValidateAppInfoList(const std::vector<AppInfo> &appInfoList);

/**
 * @brief Query control app status via closed-source module plugin.
 * @param callerAppInfo Caller app info obtained from IPC context.
 * @param appInfoList List of target application info to query.
 * @param resultList Output list of control status results.
 * @return Returns 0 on success, or error code on failure.
 */
int32_t QueryControlAppStatusInternal(
    const AppInfo &callerAppInfo,
    const std::vector<AppInfo> &appInfoList,
    std::vector<ControlAppStatusResult> &resultList);

/**
 * @brief Query the control status of simulated click operation (IPC 503).
 * @param appInfoList List of target application info to query.
 * @param resultList Output list of control status results.
 * @param resultCode Output business result code.
 * @return Returns ErrCode (always success, business result in resultCode).
 */
ErrCode QueryControlAppStatus(
    const std::vector<AppInfo> &appInfoList,
    std::vector<ControlAppStatusResult> &resultList,
    int32_t &resultCode);

// Rust callable: returns boot time in milliseconds, or -1 on failure.
int64_t GetBootTimeMs();

// Rust callable: returns the binder calling token id.
uint32_t GetCallingTokenId();

// Rust callable: resolves os_account_id from a token id.
// For SA (TOKEN_NATIVE): returns 0. For HAP: returns osAccountID from HapTokenInfo.
int32_t GetOsAccountIdFromTokenId(uint32_t tokenId);

/**
 * @brief Query the full list of explicitly-denied app-pairs for GUI control (IPC 505).
 *
 * The Service queries the closed-source module for the full list of
 * user-explicitly-denied app-pairs and returns them.
 *
 * @param deniedPairs Output full list of denied app-pairs.
 * @param resultCode Output business result code.
 * @return Returns ErrCode (always success, business result in resultCode).
 */
ErrCode QueryAllDeniedPairs(
    std::vector<std::string> &deniedPairs,
    int32_t &resultCode);

/**
 * @brief Register a component click decision callback by session ID (IPC 506).
 * @param sessionId The session ID identifying the feedback channel.
 * @param callback The IGuiCallback proxy from the A app.
 * @param resultCode Output business result code.
 * @return Returns ErrCode (always success, business result in resultCode).
 */
ErrCode RegisterComponentClickCallback(
    uint64_t sessionId,
    const GuiCallback &callback,
    int32_t &resultCode);

/**
 * @brief Report a component click decision result (IPC 507).
 * @param sessionId The session ID identifying the feedback channel.
 * @param result The component click decision result.
 * @param resultCode Output business result code.
 * @return Returns ErrCode (always success, business result in resultCode).
 */
ErrCode ReportComponentClickDecision(
    uint64_t sessionId,
    const ComponentClickResult &result,
    int32_t &resultCode);

/**
 * @brief Unregister a component click callback by session ID (IPC 508).
 * @param sessionId The session ID to remove.
 * @param resultCode Output business result code.
 * @return Returns ErrCode (always success, business result in resultCode).
 */
ErrCode UnregisterComponentClickCallback(
    uint64_t sessionId,
    int32_t &resultCode);

/**
 * @brief Store an operation record for simulated click control (IPC 509).
 * @param callerApp The caller app (A app) information.
 * @param targetApp The target app (B app) information.
 * @param decision The control decision result.
 * @param resultCode Output business result code.
 * @return Returns ErrCode (always success, business result in resultCode).
 */
ErrCode StoreOperationRecord(
    const AppInfo &callerApp,
    const AppInfo &targetApp,
    ControlDecision decision,
    int32_t &resultCode);

/**
 * @brief Get the control dialog bundle name from the closed-source plugin.
 * @param bundleName Output bundle name.
 * @param resultCode Output business result code.
 * @return Returns ErrCode.
 */
ErrCode GetControlDialogBundleName(
    std::string &bundleName,
    int32_t &resultCode);
    
/**
 * @brief Check whether the callback registry is empty (for SA idle-unload decision).
 * @return true if empty, false otherwise.
 */
bool IsCallbackRegistryEmpty();

} // namespace SAF
} // namespace Security
} // namespace OHOS

#endif