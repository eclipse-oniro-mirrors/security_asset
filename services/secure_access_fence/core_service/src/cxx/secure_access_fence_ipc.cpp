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

#include "secure_access_fence_ipc.h"
#include "secure_access_fence_service.h"
#include "wrapper.rs.h"
#include <unordered_map>
#include <chrono>
#include <vector>
#include <string>

#include "secure_access_fence_type.h"

#include "saf_log.h"
#include "saf_result_code.h"

namespace OHOS {
namespace Security {
namespace SAF {

const int32_t VECTOR_MAX_SIZE = 99;

namespace {
int32_t HandleBatchQueryCommandPermission(MessageParcel& data, MessageParcel& reply)
{
    std::vector<CommandInfo> cmds;
    int32_t cmdSize = data.ReadInt32();
    if (cmdSize > static_cast<int32_t>(VECTOR_MAX_SIZE) || cmdSize < 0) {
        LOGE("The vector size exceeds the limit!");
        return SAF_ERR_INVALID_ARRAY_LEN;
    }
    for (int32_t i1 = 0; i1 < cmdSize; ++i1) {
        CommandInfo value1;
        if (CommandInfoBlockUnmarshalling(data, value1) != ERR_NONE) {
            LOGE("Read value1 failed!");
            return SAF_ERR_IPC_READ_DATA_FAIL;
        }
        cmds.push_back(value1);
    }
    std::vector<CommandPermissionInfo> cmdPermissions;
    int32_t resultCode;
    // errCode恒为success.业务结果用resultCode
    ErrCode errCode = BatchQueryCommandPermission(cmds, cmdPermissions, resultCode);
    if (!reply.WriteInt32(errCode)) {
        LOGE("Write Int32 failed!");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!SUCCEEDED(errCode)) {
        return SAF_SUCCESS; // 接口返回成功；调用方需要从reply中读取int32，判断IPC是否成功。
    }
    if (cmdPermissions.size() > static_cast<size_t>(VECTOR_MAX_SIZE)) {
        LOGE("The list size exceeds the limit!");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!reply.WriteInt32(cmdPermissions.size())) {
        LOGE("Write cmdPermissions length failed!");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    for (auto it2 = cmdPermissions.begin(); it2 != cmdPermissions.end(); ++it2) {
        if (CommandPermissionInfoBlockMarshalling(reply, (*it2)) != ERR_NONE) {
            LOGE("Write *it2 failed!");
            return SAF_ERR_IPC_WRITE_DATA_FAIL;
        }
    }
    if (!reply.WriteInt32(resultCode)) {
        LOGE("Write resultCode failed!");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    return SAF_SUCCESS;
}

int32_t HandleRequestToolPermission(MessageParcel& data, MessageParcel& reply)
{
    PermissionQuery permissionQuery;
    if (PermissionQueryBlockUnmarshalling(data, permissionQuery) != ERR_NONE) {
        LOGE("Read permissionQuery failed!");
        return SAF_ERR_IPC_READ_DATA_FAIL;
    }
    PermissionQueryResult permissionQueryResult;
    int32_t resultCode;
    ErrCode errCode = RequestToolPermissions(permissionQuery, permissionQueryResult, resultCode);
    if (!reply.WriteInt32(errCode)) {
        LOGE("Write Int32 failed!");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!SUCCEEDED(errCode)) {
        return SAF_SUCCESS; // 接口返回成功；调用方需要从reply中读取int32，判断IPC是否成功。
    }
    if (PermissionQueryResultBlockMarshalling(reply, permissionQueryResult) != ERR_NONE) {
        LOGE("Write permissionQueryResult failed!");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!reply.WriteInt32(resultCode)) {
        LOGE("Write resultCode failed!");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    return SAF_SUCCESS;
}

int32_t HandleGrantToolPermission(MessageParcel& data, MessageParcel& reply)
{
    std::vector<UserAuthResult> userAuthResult;
    int32_t userAuthResultSize = data.ReadInt32();
    if (userAuthResultSize > static_cast<int32_t>(VECTOR_MAX_SIZE) || userAuthResultSize < 0) {
        LOGE("The vecotr/array size exceeds the security limit!");
        return SAF_ERR_IPC_READ_DATA_FAIL;
    }
    for (int32_t i7 = 0; i7 < userAuthResultSize; ++i7) {
        UserAuthResult value7;
        if (UserAuthResultBlockUnmarshalling(data, value7) != ERR_NONE) {
            LOGE("Read [value7] failed!");
            return SAF_ERR_IPC_READ_DATA_FAIL;
        }
        userAuthResult.push_back(value7);
    }
    std::vector<VerifyTicketInfo> ticketInfo;
    int32_t result;
    ErrCode errCode = GrantToolPermissionsByUser(userAuthResult, ticketInfo, result);
    if (!reply.WriteInt32(errCode)) {
        LOGE("Write Int32 failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!SUCCEEDED(errCode)) {
        return SAF_SUCCESS;  // 接口返回成功；调用方需要从reply中读取int32，判断IPC是否成功。
    }
    if (ticketInfo.size() > static_cast<size_t>(VECTOR_MAX_SIZE)) {
        LOGE("The list size exceeds the security limit!");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!reply.WriteInt32(ticketInfo.size())) {
        LOGE("Write [ticketInfo.size()] failed!");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    for (auto it8 = ticketInfo.begin(); it8 != ticketInfo.end(); ++it8) {
        if (VerifyTicketInfoBlockMarshalling(reply, (*it8)) != ERR_NONE) {
            LOGE("Write [(*it8)] failed!");
            return SAF_ERR_IPC_WRITE_DATA_FAIL;
        }
    }
    if (!reply.WriteInt32(result)) {
        LOGE("Write [result] failed!");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    return SAF_SUCCESS;
}

using IpcHandler = int32_t (*)(MessageParcel& data, MessageParcel& reply);

// Read AppInfo list from MessageParcel. Returns SAF_SUCCESS on success.
static int32_t ReadAppInfoList(MessageParcel& data, std::vector<AppInfo>& appInfoList)
{
    int32_t appInfoSize = data.ReadInt32();
    if (appInfoSize > static_cast<int32_t>(VECTOR_MAX_SIZE) || appInfoSize < 0) {
        LOGE("ReadAppInfoList invalid appInfoSize=%{public}d", appInfoSize);
        return SAF_ERR_INVALID_ARRAY_LEN;
    }
    for (int32_t i = 0; i < appInfoSize; ++i) {
        AppInfo appInfo;
        if (AppInfoBlockUnmarshalling(data, appInfo) != ERR_NONE) {
            LOGE("ReadAppInfoList failed at index=%{public}d", i);
            return SAF_ERR_IPC_READ_DATA_FAIL;
        }
        appInfoList.push_back(appInfo);
    }
    return SAF_SUCCESS;
}

// Write ControlAppStatusResult list + resultCode to reply. Returns SAF_SUCCESS on success.
static int32_t WriteControlAppResultList(MessageParcel& reply,
    const std::vector<ControlAppStatusResult>& resultList, int32_t resultCode)
{
    if (resultList.size() > static_cast<size_t>(VECTOR_MAX_SIZE)) {
        LOGE("WriteControlAppResultList size exceeds limit");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!reply.WriteInt32(resultList.size())) {
        LOGE("WriteControlAppResultList write size failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    for (const auto &result : resultList) {
        if (ControlAppStatusResultBlockMarshalling(reply, result) != ERR_NONE) {
            LOGE("WriteControlAppResultList write result failed");
            return SAF_ERR_IPC_WRITE_DATA_FAIL;
        }
    }
    if (!reply.WriteInt32(resultCode)) {
        LOGE("WriteControlAppResultList write resultCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    return SAF_SUCCESS;
}

int32_t HandleQueryControlAppStatus(MessageParcel& data, MessageParcel& reply)
{
    std::vector<AppInfo> appInfoList;
    int32_t ret = ReadAppInfoList(data, appInfoList);
    if (ret != SAF_SUCCESS) {
        return ret;
    }

    std::vector<ControlAppStatusResult> resultList;
    int32_t resultCode = 0;
    ErrCode errCode = QueryControlAppStatus(appInfoList, resultList, resultCode);
    if (!reply.WriteInt32(errCode)) {
        LOGE("HandleQueryControlAppStatus write errCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!SUCCEEDED(errCode)) {
        return SAF_SUCCESS;
    }
    return WriteControlAppResultList(reply, resultList, resultCode);
}

int32_t HandleCheckControlPermission(MessageParcel& data, MessageParcel& reply)
{
    AppInfo callerApp;
    if (AppInfoBlockUnmarshalling(data, callerApp) != ERR_NONE) {
        LOGE("HandleCheckControlPermission read callerApp failed");
        return SAF_ERR_IPC_READ_DATA_FAIL;
    }
    AppInfo targetApp;
    if (AppInfoBlockUnmarshalling(data, targetApp) != ERR_NONE) {
        LOGE("HandleCheckControlPermission read targetApp failed");
        return SAF_ERR_IPC_READ_DATA_FAIL;
    }

    std::vector<std::string> deniedPairs;
    int32_t resultCode = 0;
    ErrCode errCode = CheckControlPermission(callerApp, targetApp, deniedPairs, resultCode);
    if (!reply.WriteInt32(errCode)) {
        LOGE("HandleCheckControlPermission write errCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!SUCCEEDED(errCode)) {
        return SAF_SUCCESS;
    }
    if (deniedPairs.size() > static_cast<size_t>(VECTOR_MAX_SIZE)) {
        LOGE("HandleCheckControlPermission deniedPairs size exceeds limit");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!reply.WriteInt32(static_cast<int32_t>(deniedPairs.size()))) {
        LOGE("HandleCheckControlPermission write deniedPairs size failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    for (const auto &pair : deniedPairs) {
        if (!reply.WriteString(pair)) {
            LOGE("HandleCheckControlPermission write deniedPair failed");
            return SAF_ERR_IPC_WRITE_DATA_FAIL;
        }
    }
    if (!reply.WriteInt32(resultCode)) {
        LOGE("HandleCheckControlPermission write resultCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    return SAF_SUCCESS;
}

int32_t HandleRegisterComponentClickCallback(MessageParcel& data, MessageParcel& reply)
{
    uint64_t sessionId = data.ReadUint64();
    auto remoteObj = data.ReadRemoteObject();
    GuiCallback callback = iface_cast<IGuiCallback>(remoteObj);

    int32_t resultCode = 0;
    ErrCode errCode = RegisterComponentClickCallback(sessionId, callback, resultCode);
    if (!reply.WriteInt32(errCode)) {
        LOGE("HandleRegisterComponentClickCallback write errCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!SUCCEEDED(errCode)) {
        return SAF_SUCCESS;
    }
    if (!reply.WriteInt32(resultCode)) {
        LOGE("HandleRegisterComponentClickCallback write resultCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    return SAF_SUCCESS;
}

int32_t HandleReportComponentClickDecision(MessageParcel& data, MessageParcel& reply)
{
    uint64_t sessionId = data.ReadUint64();
    ComponentClickResult result;
    if (ComponentClickResultBlockUnmarshalling(data, result) != ERR_NONE) {
        LOGE("HandleReportComponentClickDecision read result failed");
        return SAF_ERR_IPC_READ_DATA_FAIL;
    }

    int32_t resultCode = 0;
    ErrCode errCode = ReportComponentClickDecision(sessionId, result, resultCode);
    if (!reply.WriteInt32(errCode)) {
        LOGE("HandleReportComponentClickDecision write errCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!SUCCEEDED(errCode)) {
        return SAF_SUCCESS;
    }
    if (!reply.WriteInt32(resultCode)) {
        LOGE("HandleReportComponentClickDecision write resultCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    return SAF_SUCCESS;
}

int32_t HandleUnregisterComponentClickCallback(MessageParcel& data, MessageParcel& reply)
{
    uint64_t sessionId = data.ReadUint64();

    int32_t resultCode = 0;
    ErrCode errCode = UnregisterComponentClickCallback(sessionId, resultCode);
    if (!reply.WriteInt32(errCode)) {
        LOGE("HandleUnregisterComponentClickCallback write errCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!SUCCEEDED(errCode)) {
        return SAF_SUCCESS;
    }
    if (!reply.WriteInt32(resultCode)) {
        LOGE("HandleUnregisterComponentClickCallback write resultCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    return SAF_SUCCESS;
}

int32_t HandleStoreOperationRecord(MessageParcel& data, MessageParcel& reply)
{
    AppInfo callerApp;
    if (AppInfoBlockUnmarshalling(data, callerApp) != ERR_NONE) {
        LOGE("HandleStoreOperationRecord read callerApp failed");
        return SAF_ERR_IPC_READ_DATA_FAIL;
    }
    AppInfo targetApp;
    if (AppInfoBlockUnmarshalling(data, targetApp) != ERR_NONE) {
        LOGE("HandleStoreOperationRecord read targetApp failed");
        return SAF_ERR_IPC_READ_DATA_FAIL;
    }
    int32_t decisionValue = data.ReadInt32();
    ControlDecision decision = static_cast<ControlDecision>(decisionValue);

    int32_t resultCode = 0;
    ErrCode errCode = StoreOperationRecord(callerApp, targetApp, decision, resultCode);
    if (!reply.WriteInt32(errCode)) {
        LOGE("HandleStoreOperationRecord write errCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!SUCCEEDED(errCode)) {
        return SAF_SUCCESS;
    }
    if (!reply.WriteInt32(resultCode)) {
        LOGE("HandleStoreOperationRecord write resultCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    return SAF_SUCCESS;
}

int32_t HandleGetControlDialogBundleName(MessageParcel& data, MessageParcel& reply)
{
    std::string bundleName;
    int32_t resultCode = 0;
    ErrCode errCode = GetControlDialogBundleName(bundleName, resultCode);
    if (!reply.WriteInt32(errCode)) {
        LOGE("HandleGetControlDialogBundleName write errCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!SUCCEEDED(errCode)) {
        return SAF_SUCCESS;
    }
    if (!reply.WriteString16(Str8ToStr16(bundleName))) {
        LOGE("HandleGetControlDialogBundleName write bundleName failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!reply.WriteInt32(resultCode)) {
        LOGE("HandleGetControlDialogBundleName write resultCode failed");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    return SAF_SUCCESS;
}

const std::unordered_map<uint32_t, IpcHandler> IPC_HANDLERS = {
    {SecureAccessFenceCode::BATCH_QUERY_COMMAND_PERMISSION, HandleBatchQueryCommandPermission},
    {SecureAccessFenceCode::REQUEST_TOOL_PERMISSION, HandleRequestToolPermission},
    {SecureAccessFenceCode::GRANT_TOOL_PERMISSION, HandleGrantToolPermission},
    {SecureAccessFenceCode::QUERY_CONTROL_APP_STATUS, HandleQueryControlAppStatus},
    {SecureAccessFenceCode::CHECK_CONTROL_PERMISSION, HandleCheckControlPermission},
    {SecureAccessFenceCode::REGISTER_COMPONENT_CLICK_CALLBACK, HandleRegisterComponentClickCallback},
    {SecureAccessFenceCode::REPORT_COMPONENT_CLICK_DECISION, HandleReportComponentClickDecision},
    {SecureAccessFenceCode::UNREGISTER_COMPONENT_CLICK_CALLBACK, HandleUnregisterComponentClickCallback},
    {SecureAccessFenceCode::STORE_OPERATION_RECORD, HandleStoreOperationRecord},
    {SecureAccessFenceCode::GET_CONTROL_DIALOG_BUNDLE_NAME, HandleGetControlDialogBundleName}
};

}

int32_t OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply)
{
    auto it = IPC_HANDLERS.find(code);
    if (it == IPC_HANDLERS.end()) {
        LOGE("Invalid IPC code: %{public}u", code);
        return SAF_ERR_IPC_INVALID_IPC_CODE;
    }
    return it->second(data, reply);
}

}
}
}