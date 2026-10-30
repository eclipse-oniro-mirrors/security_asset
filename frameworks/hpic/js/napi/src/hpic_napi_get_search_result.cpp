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

#include "hpic_napi_get_search_result.h"
#include "hpic_napi_common.h"
#include "hpic_napi_loader.h"
#include "hpic_napi_utils.h"
#include "privacy_client.h"
#include <new>
#include <utility>

namespace OHOS::Security::PrivacyComputation {
namespace {
constexpr size_t ARG_COUNT = 2;

napi_status ParseGetSearchResultArgs(napi_env env, napi_callback_info info, HpicBaseContext *baseContext)
{
    auto *context = static_cast<GetSearchResultContext *>(baseContext);
    napi_value args[ARG_COUNT] = {nullptr};
    IF_ERR_RETURN(ParseJsArgs(env, info, args, ARG_COUNT));
    IF_ERR_RETURN(ParseJsSearchResult(env, args[0], context->searchResult));
    IF_ERR_RETURN(ParseJsProtocol(env, args[1], context->protocol));
    return napi_ok;
}

} // namespace

napi_value NapiGetSearchResult(napi_env env, napi_callback_info info)
{
    if (IsCczConfigured()) {
        LOGI("HPIC NAPI getSearchResult begin, route=CCZ");
        return CallComputationNapiFunc(env, info, "GetSearchResult");
    }
    LOGI("HPIC NAPI getSearchResult begin, route=AssetSA");
    auto context = std::unique_ptr<GetSearchResultContext>(new (std::nothrow) GetSearchResultContext());
    NAPI_THROW(env, context == nullptr, HPIC_OUT_OF_MEMORY, GetErrorMessage(HPIC_OUT_OF_MEMORY));
    context->parse = ParseGetSearchResultArgs;
    context->execute = [](napi_env, void *data) {
        if (data == nullptr) {
            LOGE("Async execute context is null");
            return;
        }
        auto *context = static_cast<GetSearchResultContext *>(data);
        context->result =
            MapProtocolError(GetSearchResultRemote(context->searchResult, context->protocol, context->output));
        if (context->result != HPIC_SUCCESS) {
            LOGE("get_search_result failed, code=%{public}d", context->result);
        }
    };
    context->resolve = [](napi_env env, HpicBaseContext *baseContext, napi_value &output) {
        auto *context = static_cast<GetSearchResultContext *>(baseContext);
        return CreateJsSearchResult(env, context->output, output);
    };
    return CreateAsyncWork(env, info, std::move(context), __func__);
}
} // namespace OHOS::Security::PrivacyComputation
