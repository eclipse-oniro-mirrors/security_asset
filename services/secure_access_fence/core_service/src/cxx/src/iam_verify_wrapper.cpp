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
#include <mutex>
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
constexpr int32_t IAM_VERIFY_TIMEOUT_MS = 5000;

struct IamVerifyAsyncResult {
    int32_t resultCode {IAM_VERIFY_RESULT_NOT_SET};
    std::vector<uint8_t> challenge;
    uint32_t authTrustLevel {0};
};

class VerifyTokenCallbackWrapper final : public VerifyTokenCallback {
public:
    explicit VerifyTokenCallbackWrapper(std::weak_ptr<std::promise<IamVerifyAsyncResult>> weakPromise)
        : weakPromise_(weakPromise) {}

    virtual ~VerifyTokenCallbackWrapper() = default;

    void OnResult(int32_t result, const Attributes &extraInfo) override
    {
        std::call_once(flag_, [this, result, &extraInfo]() {
            IamVerifyAsyncResult verifyResult;
            verifyResult.resultCode = result;
            if (result == SUCCESS) {
                extraInfo.GetUint8ArrayValue(Attributes::ATTR_CHALLENGE, verifyResult.challenge);
                extraInfo.GetUint32Value(Attributes::ATTR_AUTH_TRUST_LEVEL, verifyResult.authTrustLevel);
            }
            if (auto sharedPromise = weakPromise_.lock()) {
                sharedPromise->set_value(verifyResult);
            }
        });
    }

private:
    std::once_flag flag_{};
    std::weak_ptr<std::promise<IamVerifyAsyncResult>> weakPromise_;
};

IamVerifyResult VerifyAuthToken(const rust::Vec<uint8_t> &authToken, uint64_t allowableDurationMs)
{
    IamVerifyResult ret = {};
    if (authToken.empty()) {
        ret.result_code = IAM_VERIFY_RESULT_NOT_SET;
        return ret;
    }

    std::vector<uint8_t> token(authToken.begin(), authToken.end());
    auto sharedPromise = std::make_shared<std::promise<IamVerifyAsyncResult>>();
    auto future = sharedPromise->get_future();
    auto callback = std::make_shared<VerifyTokenCallbackWrapper>(sharedPromise);
    UserAccessCtrlClient::GetInstance().VerifyAuthToken(token, allowableDurationMs, callback);

    if (future.wait_for(std::chrono::milliseconds(IAM_VERIFY_TIMEOUT_MS)) == std::future_status::timeout) {
        LOGE("VerifyAuthToken timeout");
        ret.result_code = IAM_VERIFY_RESULT_TIMEOUT;
        return ret;
    }

    IamVerifyAsyncResult verifyResult = future.get();
    ret.result_code = verifyResult.resultCode;
    for (auto b : verifyResult.challenge) {
        ret.challenge.push_back(b);
    }
    ret.auth_trust_level = verifyResult.authTrustLevel;
    return ret;
}

} // namespace SAF
} // namespace Security
} // namespace OHOS
