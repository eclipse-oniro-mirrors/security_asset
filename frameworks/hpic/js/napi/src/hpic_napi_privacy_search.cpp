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

#include "hpic_napi_privacy_search.h"
#include "hpic_napi_common.h"
#include "hpic_napi_utils.h"
#include <new>
#include <utility>

namespace OHOS::Security::PrivacyComputation {
namespace {
constexpr size_t ARG_COUNT = 3;

napi_status ParsePrivacySearchArgs(napi_env env, napi_callback_info info, HpicBaseContext *baseContext)
{
    auto *context = static_cast<PrivacySearchContext *>(baseContext);
    napi_value args[ARG_COUNT] = {nullptr};
    IF_ERR_RETURN(ParseJsArgs(env, info, args, ARG_COUNT));
    IF_ERR_RETURN(ParseJsPrivacyTarget(env, args[0], context->target));
    IF_ERR_RETURN(ParseJsElements(env, args[1], context->elements));
    IF_ERR_RETURN(ParseJsProtocol(env, args[2], context->protocol));
    return napi_ok;
}

} // namespace

napi_value NapiPrivacySearch(napi_env env, napi_callback_info info)
{
    auto context = std::unique_ptr<PrivacySearchContext>(new (std::nothrow) PrivacySearchContext());
    NAPI_THROW(env, context == nullptr, HPIC_OUT_OF_MEMORY, GetErrorMessage(HPIC_OUT_OF_MEMORY));
    context->parse = ParsePrivacySearchArgs;
    context->execute = [](napi_env, void *data) {
        if (data == nullptr) {
            LOGE("Async execute context is null");
            return;
        }
        auto *context = static_cast<PrivacySearchContext *>(data);
        context->result = MapProtocolError(
            psi_pir::privacySearch(context->target, context->elements, context->protocol, context->searchResult));
        if (context->result != HPIC_SUCCESS) {
            LOGE("privacy_search failed, code=%{public}d", context->result);
        }
    };
    context->resolve = [](napi_env env, HpicBaseContext *baseContext, napi_value &output) {
        auto *context = static_cast<PrivacySearchContext *>(baseContext);
        return CreateJsPrivacySearchResult(env, context->searchResult, output);
    };
    return CreateAsyncWork(env, info, std::move(context), __func__);
}
} // namespace OHOS::Security::PrivacyComputation
