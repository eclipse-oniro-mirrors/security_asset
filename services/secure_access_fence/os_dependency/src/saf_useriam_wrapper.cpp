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

#include "saf_useriam_wrapper.h"
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <future>
#include "saf_log.h"
#include "companion_device_auth_client.h"
#include "icontinuous_auth_status_callback.h"

using namespace OHOS::UserIam::CompanionDeviceAuth;

#define SUCCESS 0
#define TIMEOUT 1
#define GENERAL_ERROR 2
#define DEFAULT_TIMEOUT_MS 5000

struct CheckResult {
    bool matched;
    bool isAuthPassed;
    int32_t actualTrustLevel;
};

class CheckCallback : public IContinuousAuthStatusCallback {
public:
    CheckCallback(std::shared_ptr<std::promise<CheckResult>> promise, int32_t expectedAuthTrustLevel)
            : promise_(promise),
              expectedAuthTrustLevel_(expectedAuthTrustLevel),
              triggered_(false) {}

    void OnContinuousAuthStatusChange(const bool isAuthPassed,
        const std::optional<int32_t> authTrustLevel) override {

        bool expected = false;
        if (!triggered_.compare_exchange_strong(expected, true)) {
            LOGI("Callback already triggered, ignore");
            return;
        }

        LOGI("=== Callback triggered ===");
        LOGI("isAuthPassed: %{public}d", isAuthPassed);

        CheckResult result;
        result.isAuthPassed = isAuthPassed;
        result.actualTrustLevel = authTrustLevel.has_value() ? authTrustLevel.value() : -1;

        if (isAuthPassed && authTrustLevel.has_value()) {
            int32_t actualTrustLevel = authTrustLevel.value();
            if (actualTrustLevel == expectedAuthTrustLevel_) {
                result.matched = true;
                LOGI("Matched! isAuthPassed: true, authTrustLevel: %{public}d", actualTrustLevel);
            } else {
                result.matched = false;
                LOGI("Not matched, authTrustLevel: %{public}d != expected: %{public}d",
                    actualTrustLevel, expectedAuthTrustLevel_);
            }
        } else {
            result.matched = false;
            if (!isAuthPassed) {
                LOGI("Not matched, isAuthPassed: false");
            } else {
                LOGI("Not matched, authTrustLevel not available");
            }
        }

        promise_->set_value(result);
    }

private:
    std::shared_ptr<std::promise<CheckResult>> promise_;
    int32_t expectedAuthTrustLevel_;
    std::atomic<bool> triggered_;
};

int32_t IsDeviceValid(int32_t userId, const char* deviceId, int32_t authTrustLevel, bool* isValid)
{
    if (deviceId == nullptr || isValid == nullptr) {
        LOGE("Invalid input parameters");
        return -1;
    }

    std::string deviceIdStr(deviceId);

    auto &client = CompanionDeviceAuthClient::GetInstance();
    std::vector<ClientTemplateStatus> templateStatusList;
    int32_t ret = client.GetTemplateStatus(userId, templateStatusList);
    if (ret != SUCCESS) {
        LOGE("GetTemplateStatus failed, ret: %{public}d", ret);
        return ret;
    }

    uint64_t matchedTemplateId = 0;
    bool found = false;
    for (const auto& templateStatus : templateStatusList) {
        std::string currentDeviceId = templateStatus.deviceStatus.deviceKey.deviceId;
        bool isOnline = templateStatus.deviceStatus.isOnline;

        if (currentDeviceId == deviceIdStr && isOnline) {
            matchedTemplateId = templateStatus.templateId;
            found = true;
            LOGI("DeviceId found, templateId: %{public}s", std::to_string(matchedTemplateId).c_str());
            break;
        }
    }

    if (!found) {
        *isValid = false;
        LOGI("DeviceId not found");
        return SUCCESS;
    }

    auto promise = std::make_shared<std::promise<CheckResult>>();
    auto future = promise->get_future();

    auto callback = std::make_shared<CheckCallback>(promise, authTrustLevel);
    ret = client.SubscribeContinuousAuthStatusChange(userId, matchedTemplateId, callback);

    if (ret != SUCCESS) {
        LOGE("SubscribeContinuousAuthStatusChange failed, ret: %{public}d", ret);
        return ret;
    }

    std::future_status status = future.wait_for(std::chrono::milliseconds(DEFAULT_TIMEOUT_MS));

    client.UnsubscribeContinuousAuthStatusChange(callback);
    LOGI("Unsubscribed");

    if (status == std::future_status::ready) {
        CheckResult result = future.get();
        *isValid = result.matched;

        if (result.matched) {
            LOGI("========== Matched ==========");
            LOGI("isAuthPassed: true, authTrustLevel: %{public}d", result.actualTrustLevel);
        } else {
            LOGI("========== Not matched ==========");
            LOGI("isAuthPassed: %{public}d, authTrustLevel: %{public}d, expected: %{public}d",
                result.isAuthPassed, result.actualTrustLevel, authTrustLevel);
        }

        return SUCCESS;
    } else if (status == std::future_status::timeout) {
        LOGE("========== Timeout ==========");
        LOGE("Did not get result within %{public}u ms", DEFAULT_TIMEOUT_MS);
        *isValid = false;
        return TIMEOUT;
    } else {
        LOGE("========== Future failed = ==========");
        *isValid = false;
        return GENERAL_ERROR;
    }
}