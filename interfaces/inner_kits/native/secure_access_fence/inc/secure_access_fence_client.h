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

#ifndef SECURE_ACCESS_FENCE_CLIENT_H
#define SECURE_ACCESS_FENCE_CLIENT_H

#include <vector>
#include <string>
#include <mutex>

#include "isecure_access_fence.h"
#include "secure_access_fence_types.h"
#include "iremote_object.h"
#include "refbase.h"

namespace OHOS {
namespace Security {
namespace SecureAccessFence {

constexpr int32_t SAF_SERVICE_ID = 66533;

constexpr int32_t E_SUCCESS = 0;
constexpr int32_t E_SERVICE_UNAVAILABLE = 1023900001;
constexpr int32_t E_IPC_ERROR = 1023900002;
constexpr int32_t E_PARAM_INVALID = 1023900006;

class SafDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    SafDeathRecipient() = default;
    ~SafDeathRecipient() = default;
    void OnRemoteDied(const wptr<IRemoteObject> &object) override;
};

class SecureAccessFenceClient : public RefBase {
public:
    DISALLOW_COPY_AND_MOVE(SecureAccessFenceClient);
    static sptr<SecureAccessFenceClient> GetInstance();

    int32_t QueryPermissionBySubCommandBatch(
        const std::vector<Command> &cmds,
        std::vector<CommandPermission> &cmdPermissions);

    int32_t GenerateTicketBatch(
        uint32_t osAccountId,
        const std::string &callerId,
        const std::vector<std::string> &messages,
        std::vector<std::string> &tickets,
        std::string &challenge);

    int32_t VerifyTicketBatch(
        uint32_t osAccountId,
        const std::string &callerId,
        const std::vector<TicketVerifyInfo> &verifyInfos,
        const std::string &challenge,
        std::vector<int32_t> &verifyRes);

    void ClearProxy();

private:
    SecureAccessFenceClient();
    ~SecureAccessFenceClient();

    bool ConnectService();
    sptr<ISecureAccessFence> GetProxy();

    static std::mutex instanceLock_;
    static sptr<SecureAccessFenceClient> instance_;

    std::mutex proxyLock_;
    sptr<ISecureAccessFence> proxy_;
    sptr<SafDeathRecipient> deathRecipient_;
};

} // namespace SecureAccessFence
} // namespace Security
} // namespace OHOS

#endif // SECURE_ACCESS_FENCE_CLIENT_H