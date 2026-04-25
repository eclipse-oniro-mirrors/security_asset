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
#include <mutex>

#include "isecure_access_fence.h"
#include "secure_access_fence_type.h"

namespace OHOS {
namespace Security {
namespace SAF {

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
     * @param callerId The app identify ID.
     * @param message The vector of messages for which tickets will be generated.
     * @param ticketInfos Output vector of generated VerifyTicketInfo structures.
     * @return Returns 0 on success, or error code on failure.
     */
    int32_t BatchGenerateTicket(
        uint32_t osAccountId,
        const std::string &callerId,
        const std::vector<std::string> &messages,
        std::vector<VerifyTicketInfo> &ticketInfos);

    /**
     * @brief Batch verify tickets for access control.
     *
     * @param osAccountId The OS account ID, >= 100.
     * @param callerId The app identify ID.
     * @param ticketInfos The vector of VerifyTicketInfo structures to verify.
     * @param verifyRes Output vector of verification results.
     * @return Returns 0 on success, or error code on failure.
     */
    int32_t BatchVerifyTicket(
        uint32_t osAccountId,
        const std::string &callerId,
        const std::vector<VerifyTicketInfo> &verifyInfos,
        std::vector<int32_t> &verifyRes);
};

} // namespace SAF
} // namespace Security
} // namespace OHOS

#endif // SAF_AGENT_FENCE_H