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

#ifndef HPIC_NAPI_CONTEXT_H
#define HPIC_NAPI_CONTEXT_H

#include "hpic_napi_error_code.h"
#include "napi/native_api.h"
#include "psi_pir_protocol.h"

namespace OHOS::Security::PrivacyComputation {
// Common parse/execute/resolve lifecycle follows asset_napi_context.h.
// The completion callback owns the context after successful queueing.
class HpicBaseContext {
public:
    HpicBaseContext() = default;
    virtual ~HpicBaseContext();
    HpicBaseContext(const HpicBaseContext &) = delete;
    HpicBaseContext &operator=(const HpicBaseContext &) = delete;

    napi_env env = nullptr;
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    int32_t result = HPIC_INVALID_ARGUMENT;
    psi_pir::PrivacyProtocol protocol;

    napi_status (*parse)(napi_env, napi_callback_info, HpicBaseContext *) = nullptr;
    napi_async_execute_callback execute = nullptr;
    napi_status (*resolve)(napi_env, HpicBaseContext *, napi_value &) = nullptr;
};

class GenPrivacyTargetContext final : public HpicBaseContext {
public:
    psi_pir::TargetInput input;
    psi_pir::Bytes target;
};

class PrivacySearchContext final : public HpicBaseContext {
public:
    psi_pir::Bytes target;
    std::vector<psi_pir::Element> elements;
    psi_pir::PrivacySearchResult searchResult;
};

class GetSearchResultContext final : public HpicBaseContext {
public:
    psi_pir::PrivacySearchResult searchResult;
    psi_pir::SearchResult output;
};

} // namespace OHOS::Security::PrivacyComputation

#endif // HPIC_NAPI_CONTEXT_H
