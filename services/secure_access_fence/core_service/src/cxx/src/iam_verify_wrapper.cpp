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

#include "wrapper.rs.h"
#include "iam_verify_wrapper.h"

#include <chrono>
#include <future>
#include <memory>
#include <vector>

#include "saf_log.h"

#include "user_access_ctrl_client.h"
#include "user_access_ctrl_client_callback.h"
#include "attributes.h"
#include "iam_common_defines.h"

using namespace OHOS::UserIam::UserAuth;

namespace OHOS {
namespace Security {
namespace SAF {

constexpr int32_t IAM_VERIFY_RESULT_NOT_SET = -1;
constexpr int32_t IAM_VERIFY_RESULT_TIMEOUT = -2;
constexpr int32_t IAM_VERIFY_TIMEOUT_MS = 10000;

class SyncVerifyCallback final : public VerifyTokenCallback {
public:
    SyncVerifyCallback() : future_(promise_.get_future().share()) {}
    virtual ~SyncVerifyCallback() = default;
    void OnResult(int32_t result, const Attributes &extraInfo) override
    {
        if (!resultSet_) {
            result_ = result;
            extraInfo_ = extraInfo.Serialize();
            promise_.set_value();
            resultSet_ = true;
        }
    }
    std::shared_future<void> GetFuture() { return future_; }
    int32_t result_ = IAM_VERIFY_RESULT_NOT_SET;
    std::vector<uint8_t> extraInfo_;
private:
    std::promise<void> promise_;
    std::shared_future<void> future_;
    bool resultSet_ = false;
};

IamVerifyResult VerifyAuthToken(const rust::Vec<uint8_t> &authToken, uint64_t allowableDurationMs)
{
    IamVerifyResult ret = {};
    if (authToken.empty()) {
        ret.result_code = IAM_VERIFY_RESULT_NOT_SET;
        return ret;
    }
    std::vector<uint8_t> token(authToken.begin(), authToken.end());
    auto callback = std::make_shared<SyncVerifyCallback>();
    UserAccessCtrlClient::GetInstance().VerifyAuthToken(token, allowableDurationMs, callback);
    auto status = callback->GetFuture().wait_for(std::chrono::milliseconds(IAM_VERIFY_TIMEOUT_MS));
    if (status == std::future_status::timeout) {
        LOGE("VerifyAuthToken timeout");
        ret.result_code = IAM_VERIFY_RESULT_TIMEOUT;
        return ret;
    }
    ret.result_code = callback->result_;
    if (callback->result_ == SUCCESS && !callback->extraInfo_.empty()) {
        Attributes attr(callback->extraInfo_);
        std::vector<uint8_t> challenge;
        attr.GetUint8ArrayValue(Attributes::ATTR_CHALLENGE, challenge);
        for (auto b : challenge) {
            ret.challenge.push_back(b);
        }
        attr.GetUint32Value(Attributes::ATTR_AUTH_TRUST_LEVEL, ret.auth_trust_level);
    }
    return ret;
}

} // namespace SAF
} // namespace Security
} // namespace OHOS
