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
    ErrCode errCode = BatchQueryCommandPermission(cmds, cmdPermissions, resultCode);
    if (!reply.WriteInt32(errCode)) {
        LOGE("Write Int32 failed!");
        return SAF_ERR_IPC_WRITE_DATA_FAIL;
    }
    if (!SUCCEEDED(errCode)) {
        return SAF_SUCCESS;
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

using IpcHandler = int32_t (*)(MessageParcel& data, MessageParcel& reply);

const std::unordered_map<uint32_t, IpcHandler> IPC_HANDLERS = {
    {SecureAccessFenceCode::BATCH_QUERY_COMMAND_PERMISSION, HandleBatchQueryCommandPermission},
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