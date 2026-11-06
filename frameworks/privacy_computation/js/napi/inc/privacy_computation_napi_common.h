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

#ifndef PRIVACY_COMPUTATION_NAPI_COMMON_H
#define PRIVACY_COMPUTATION_NAPI_COMMON_H

#include "privacy_computation_log.h"
#include "napi/native_api.h"
#include "privacy_protocol.h"
#include <memory>
#include <string>

namespace OHOS::Security::PrivacyComputation {
constexpr int32_t PRIVACY_COMPUTATION_SUCCESS = 0;
constexpr int32_t PRIVACY_COMPUTATION_INVALID_ARGUMENT = 401;
constexpr int32_t PRIVACY_COMPUTATION_SERVICE_ERROR = 24000001;
constexpr int32_t PRIVACY_COMPUTATION_OUT_OF_MEMORY = 24000006;
constexpr int32_t PRIVACY_COMPUTATION_DATA_CORRUPTED = 24000007;
constexpr int32_t PRIVACY_COMPUTATION_CRYPTO_ERROR = 24000009;
constexpr int32_t PRIVACY_COMPUTATION_ACCOUNT_ERROR = 24000012;
constexpr int32_t PRIVACY_COMPUTATION_UNAVAILABLE = 24000017;
int32_t MapProtocolError(int32_t result) noexcept;
const char *GetErrorMessage(int32_t code) noexcept;

// Common parse/execute/resolve lifecycle follows asset_napi_context.h.
// The completion callback owns the context after successful queueing.
class PrivacyComputationBaseContext {
public:
    PrivacyComputationBaseContext() = default;
    virtual ~PrivacyComputationBaseContext();
    PrivacyComputationBaseContext(const PrivacyComputationBaseContext &) = delete;
    PrivacyComputationBaseContext &operator=(const PrivacyComputationBaseContext &) = delete;

    napi_env env = nullptr;
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    int32_t result = PRIVACY_COMPUTATION_INVALID_ARGUMENT;
    std::string operationName;
    ::PrivacyComputation::PrivacyProtocol protocol;

    napi_status (*parse)(napi_env, napi_callback_info, PrivacyComputationBaseContext *) = nullptr;
    napi_async_execute_callback execute = nullptr;
    napi_status (*resolve)(napi_env, PrivacyComputationBaseContext *, napi_value &) = nullptr;
};

class GenPrivacyTargetContext final : public PrivacyComputationBaseContext {
public:
    ::PrivacyComputation::TargetInput input;
    ::PrivacyComputation::Bytes target;
};

class PrivacySearchContext final : public PrivacyComputationBaseContext {
public:
    ::PrivacyComputation::Bytes target;
    std::vector<::PrivacyComputation::Element> elements;
    ::PrivacyComputation::PrivacySearchResult searchResult;
};

class GetSearchResultContext final : public PrivacyComputationBaseContext {
public:
    ::PrivacyComputation::PrivacySearchResult searchResult;
    ::PrivacyComputation::SearchResult output;
};

napi_status CheckNapiStatus(napi_env env, napi_status status);
napi_status ThrowJsError(napi_env env, int32_t code, const char *message);
napi_value CreateJsError(napi_env env, int32_t code, const char *message);
napi_status RejectPromise(napi_env env, napi_deferred deferred, int32_t code, const char *message);
napi_value CreateRejectedPromise(napi_env env, int32_t code, const char *message);
napi_value CreateAsyncWork(napi_env env, napi_callback_info info,
                           std::unique_ptr<PrivacyComputationBaseContext> context, const char *resourceName);

constexpr uint32_t HASH_ALG_NONE = static_cast<uint32_t>(::PrivacyComputation::HashAlg::NONE);
constexpr uint32_t HASH_ALG_SHA256 = static_cast<uint32_t>(::PrivacyComputation::HashAlg::SHA256);
constexpr uint32_t HASH_ALG_SHA384 = static_cast<uint32_t>(::PrivacyComputation::HashAlg::SHA384);
constexpr uint32_t HASH_ALG_SHA512 = static_cast<uint32_t>(::PrivacyComputation::HashAlg::SHA512);

napi_status ParseJsArgs(napi_env env, napi_callback_info info, napi_value *args, size_t expectedCount);
napi_status NapiGetNamedProperty(napi_env env, napi_value object, const char *name, napi_value &value);
napi_status NapiGetProperty(napi_env env, napi_value value, uint32_t &number);
napi_status NapiGetProperty(napi_env env, napi_value value, ::PrivacyComputation::Bytes &bytes);
napi_status NapiGetProperty(napi_env env, napi_value value, ::PrivacyComputation::PrivacyProtocol &protocol);
napi_status NapiGetProperty(napi_env env, napi_value value, ::PrivacyComputation::TargetInput &target);
napi_status NapiGetProperty(napi_env env, napi_value value, std::vector<::PrivacyComputation::Element> &elements);
napi_status NapiGetProperty(napi_env env, napi_value value, ::PrivacyComputation::PrivacySearchResult &result);
napi_status NapiSetProperty(napi_env env, uint32_t value, napi_value &output);
napi_status NapiSetProperty(napi_env env, bool value, napi_value &output);
napi_status NapiSetProperty(napi_env env, const ::PrivacyComputation::Bytes &bytes, napi_value &output);
napi_status NapiSetProperty(napi_env env, const std::vector<::PrivacyComputation::Bytes> &bytes, napi_value &output);
napi_status NapiSetProperty(napi_env env, const ::PrivacyComputation::PrivacySearchResult &result, napi_value &output);
napi_status NapiSetProperty(napi_env env, const ::PrivacyComputation::SearchResult &result, napi_value &output);

template <typename T> napi_status NapiGetProperty(napi_env env, napi_value object, const char *name, T &value)
{
    napi_value property = nullptr;
    if (const auto callStatus = NapiGetNamedProperty(env, object, name, property); callStatus != napi_ok) {
        return callStatus;
    }
    return NapiGetProperty(env, property, value);
}

template <typename T> napi_status NapiSetProperty(napi_env env, napi_value object, const char *name, const T &value)
{
    napi_value property = nullptr;
    if (const auto callStatus = NapiSetProperty(env, value, property); callStatus != napi_ok) {
        return callStatus;
    }
    return CheckNapiStatus(env, napi_set_named_property(env, object, name, property));
}

} // namespace OHOS::Security::PrivacyComputation
#endif // PRIVACY_COMPUTATION_NAPI_COMMON_H
