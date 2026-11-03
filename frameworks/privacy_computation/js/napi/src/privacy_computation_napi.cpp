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

#include "privacy_computation_napi_common.h"
#include "privacy_computation_napi_loader.h"
#include "privacy_client.h"
#include "napi/native_common.h"
#include <new>
#include <utility>

namespace OHOS::Security::PrivacyComputation {
napi_value NapiGenPrivacyTarget(napi_env env, napi_callback_info info)
{
    if (IsComputationLibConfigured()) {
        LOGI("NAPI genPrivacyTarget begin, route=ComputationLib");
        return CallComputationNapiFunc(env, info, "GenPrivacyTarget");
    }
    LOGI("NAPI genPrivacyTarget begin, route=AssetSA");
    auto context = std::unique_ptr<GenPrivacyTargetContext>(new (std::nothrow) GenPrivacyTargetContext());
    NAPI_THROW(env, context == nullptr, PRIVACY_COMPUTATION_OUT_OF_MEMORY,
               GetErrorMessage(PRIVACY_COMPUTATION_OUT_OF_MEMORY));
    context->parse = [](napi_env env, napi_callback_info info,
                        PrivacyComputationBaseContext *baseContext) -> napi_status {
        auto *context = static_cast<GenPrivacyTargetContext *>(baseContext);
        constexpr size_t ARG_COUNT = 2;
        napi_value args[ARG_COUNT] = {nullptr};
        IF_ERR_RETURN(ParseJsArgs(env, info, args, ARG_COUNT));
        IF_ERR_RETURN(NapiGetProperty(env, args[0], context->input));
        IF_ERR_RETURN(NapiGetProperty(env, args[1], context->protocol));
        return napi_ok;
    };
    context->execute = [](napi_env, void *data) {
        if (data == nullptr) {
            LOGE("Async execute context is null");
            return;
        }
        auto *context = static_cast<GenPrivacyTargetContext *>(data);
        context->result = MapProtocolError(GenPrivacyTargetRemote(context->input, context->protocol, context->target));
        if (context->result != PRIVACY_COMPUTATION_SUCCESS) {
            LOGE("gen_privacy_target failed, code=%{public}d", context->result);
        }
    };
    context->resolve = [](napi_env env, PrivacyComputationBaseContext *baseContext, napi_value &output) {
        auto *context = static_cast<GenPrivacyTargetContext *>(baseContext);
        return NapiSetProperty(env, context->target, output);
    };
    return CreateAsyncWork(env, info, std::move(context), __func__);
}

napi_value NapiPrivacySearch(napi_env env, napi_callback_info info)
{
    LOGI("NAPI privacySearch begin, route=Local");
    auto context = std::unique_ptr<PrivacySearchContext>(new (std::nothrow) PrivacySearchContext());
    NAPI_THROW(env, context == nullptr, PRIVACY_COMPUTATION_OUT_OF_MEMORY,
               GetErrorMessage(PRIVACY_COMPUTATION_OUT_OF_MEMORY));
    context->parse = [](napi_env env, napi_callback_info info,
                        PrivacyComputationBaseContext *baseContext) -> napi_status {
        auto *context = static_cast<PrivacySearchContext *>(baseContext);
        constexpr size_t ARG_COUNT = 3;
        napi_value args[ARG_COUNT] = {nullptr};
        IF_ERR_RETURN(ParseJsArgs(env, info, args, ARG_COUNT));
        IF_ERR_RETURN(NapiGetProperty(env, args[0], context->target));
        IF_ERR_RETURN(NapiGetProperty(env, args[1], context->elements));
        IF_ERR_RETURN(NapiGetProperty(env, args[2], context->protocol));
        return napi_ok;
    };
    context->execute = [](napi_env, void *data) {
        if (data == nullptr) {
            LOGE("Async execute context is null");
            return;
        }
        auto *context = static_cast<PrivacySearchContext *>(data);
        context->result = MapProtocolError(
            psi_pir::PrivacySearch(context->target, context->elements, context->protocol, context->searchResult));
        if (context->result != PRIVACY_COMPUTATION_SUCCESS) {
            LOGE("privacy_search failed, code=%{public}d", context->result);
        }
    };
    context->resolve = [](napi_env env, PrivacyComputationBaseContext *baseContext, napi_value &output) {
        auto *context = static_cast<PrivacySearchContext *>(baseContext);
        return NapiSetProperty(env, context->searchResult, output);
    };
    return CreateAsyncWork(env, info, std::move(context), __func__);
}

napi_value NapiGetSearchResult(napi_env env, napi_callback_info info)
{
    if (IsComputationLibConfigured()) {
        LOGI("NAPI genPrivacyTarget begin, route=ComputationLib");
        return CallComputationNapiFunc(env, info, "GetSearchResult");
    }
    auto context = std::unique_ptr<GetSearchResultContext>(new (std::nothrow) GetSearchResultContext());
    NAPI_THROW(env, context == nullptr, PRIVACY_COMPUTATION_OUT_OF_MEMORY,
               GetErrorMessage(PRIVACY_COMPUTATION_OUT_OF_MEMORY));
    context->parse = [](napi_env env, napi_callback_info info,
                        PrivacyComputationBaseContext *baseContext) -> napi_status {
        auto *context = static_cast<GetSearchResultContext *>(baseContext);
        constexpr size_t ARG_COUNT = 2;
        napi_value args[ARG_COUNT] = {nullptr};
        IF_ERR_RETURN(ParseJsArgs(env, info, args, ARG_COUNT));
        IF_ERR_RETURN(NapiGetProperty(env, args[0], context->searchResult));
        IF_ERR_RETURN(NapiGetProperty(env, args[1], context->protocol));
        return napi_ok;
    };
    context->execute = [](napi_env, void *data) {
        if (data == nullptr) {
            LOGE("Async execute context is null");
            return;
        }
        auto *context = static_cast<GetSearchResultContext *>(data);
        LOGI("NAPI getSearchResult begin, route=AssetSA");
        context->result =
            MapProtocolError(GetSearchResultRemote(context->searchResult, context->protocol, context->output));
        if (context->result != PRIVACY_COMPUTATION_SUCCESS) {
            LOGE("get_search_result failed, code=%{public}d", context->result);
        }
    };
    context->resolve = [](napi_env env, PrivacyComputationBaseContext *baseContext, napi_value &output) {
        auto *context = static_cast<GetSearchResultContext *>(baseContext);
        return NapiSetProperty(env, context->output, output);
    };
    return CreateAsyncWork(env, info, std::move(context), __func__);
}

constexpr uint32_t PRIVATE_SET_INTERSECTION = 0;
constexpr uint32_t PRIVATE_INFORMATION_RETRIEVAL = 1;
constexpr uint32_t DATA_SET_SIZE_128 = 0;
constexpr uint32_t DATA_SET_SIZE_256 = 1;
constexpr uint32_t DATA_SET_SIZE_512 = 2;
constexpr const char *PRIVATE_SET_INTERSECTION_PROTOCOL = "PSI_PROTOCOL";
constexpr const char *PRIVATE_INFORMATION_RETRIEVAL_PROTOCOL = "PIR_PROTOCOL";

napi_value DeclareProtocolType(const napi_env env)
{
    napi_value obj;
    NAPI_CALL(env, napi_create_object(env, &obj));
    NAPI_CALL(env, NapiSetProperty(env, obj, PRIVATE_SET_INTERSECTION_PROTOCOL, PRIVATE_SET_INTERSECTION));
    NAPI_CALL(env, NapiSetProperty(env, obj, PRIVATE_INFORMATION_RETRIEVAL_PROTOCOL, PRIVATE_INFORMATION_RETRIEVAL));
    return obj;
}

napi_value DeclareDataSetSize(const napi_env env)
{
    napi_value obj;
    NAPI_CALL(env, napi_create_object(env, &obj));
    NAPI_CALL(env, NapiSetProperty(env, obj, "SIZE_128", DATA_SET_SIZE_128));
    NAPI_CALL(env, NapiSetProperty(env, obj, "SIZE_256", DATA_SET_SIZE_256));
    NAPI_CALL(env, NapiSetProperty(env, obj, "SIZE_512", DATA_SET_SIZE_512));
    return obj;
}

napi_value DeclareHashAlg(const napi_env env)
{
    napi_value obj;
    NAPI_CALL(env, napi_create_object(env, &obj));
    NAPI_CALL(env, NapiSetProperty(env, obj, "NONE", HASH_ALG_NONE));
    NAPI_CALL(env, NapiSetProperty(env, obj, "SHA256", HASH_ALG_SHA256));
    NAPI_CALL(env, NapiSetProperty(env, obj, "SHA384", HASH_ALG_SHA384));
    NAPI_CALL(env, NapiSetProperty(env, obj, "SHA512", HASH_ALG_SHA512));
    return obj;
}

napi_value Register(const napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        // register function
        DECLARE_NAPI_FUNCTION("genPrivacyTarget", NapiGenPrivacyTarget),
        DECLARE_NAPI_FUNCTION("privacySearch", NapiPrivacySearch),
        DECLARE_NAPI_FUNCTION("getSearchResult", NapiGetSearchResult),

        // register enumerate
        DECLARE_NAPI_PROPERTY("ProtocolType", DeclareProtocolType(env)),
        DECLARE_NAPI_PROPERTY("DataSetSize", DeclareDataSetSize(env)),
        DECLARE_NAPI_PROPERTY("HashAlg", DeclareHashAlg(env)),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    LOGI("NAPI security.privacyComputation exports registered");
    return exports;
}

napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Register,
    .nm_modname = "security.privacyComputation",
    .nm_priv = nullptr,
    .reserved = {nullptr},
};

} // namespace OHOS::Security::PrivacyComputation

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&OHOS::Security::PrivacyComputation::g_module);
}
