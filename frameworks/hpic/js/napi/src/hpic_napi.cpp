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

#include <atomic>
#include <cstdint>
#include <dlfcn.h>
#include <mutex>
#include <string>

#include "hpic_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace {
constexpr char COMP_NAPI_PATH[] = ASSET_ENABLE_PRIVACY_COMPUTATION_CONFIG;
constexpr int32_t COMP_NAPI_UNSUPPORTED = 24000017;

using ComputationNapiFunc = napi_value(*)(napi_env, napi_callback_info);

std::atomic<void*> g_computationHandle{nullptr};
std::mutex g_computationMutex;
void *GetComputationHandle()
{
    if (COMP_NAPI_PATH[0] == '\0') {
        LOGE("computation path is empty, skip dlopen");
        return nullptr;
    }
    void *handle = g_computationHandle.load(std::memory_order_acquire);
    if (handle != nullptr) {
        return handle;
    }
    std::lock_guard<std::mutex> lock(g_computationMutex);
    handle = g_computationHandle.load(std::memory_order_relaxed);
    if (handle != nullptr) {
        return handle;
    }
    handle = dlopen(COMP_NAPI_PATH, RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        LOGE("dlopen computation so failed, %{public}s!", dlerror());
        return nullptr;
    }
    g_computationHandle.store(handle, std::memory_order_release);
    return handle;
}

napi_value RejectPromise(napi_env env, int32_t errCode, const char *msg)
    {
    napi_deferred deferred;
    napi_value promise = nullptr;
    napi_create_promise(env, &deferred, &promise);

    napi_value codeVal = nullptr;
    napi_create_int32(env, errCode, &codeVal);

    napi_value msgVal = nullptr;
    napi_create_string_utf8(env, msg, NAPI_AUTO_LENGTH, &msgVal);

    napi_value errVal = nullptr;
    napi_create_error(env, codeVal, msgVal, &errVal);
    napi_reject_deferred(env, deferred, errVal);
    return promise;
    }

} // anonymous namespace
napi_value CallComputationNapiFunc(napi_env env, napi_callback_info info, const char *funcName)
{
    void *handle = GetComputationHandle();
    if (handle == nullptr) {
        LOGE("Failed to dlopen computation napi so, dlerror: %{public}s", dlerror());
        return RejectPromise(env, COMP_NAPI_UNSUPPORTED, "computation napi so is unavailable");
    }
    auto ComputationFunc = (ComputationNapiFunc)dlsym(handle, funcName);
    if (ComputationFunc == nullptr) {
        LOGE("Failed to dlsym %{public}s, dlerror: %{public}s", funcName, dlerror());
        return RejectPromise(env, COMP_NAPI_UNSUPPORTED, "computation napi symbol not found");
    }
    return ComputationFunc(env, info);
}

namespace OHOS::Security::PrivacyComputation {

constexpr uint32_t PRIVATE_SET_INTERSECTION = 0;
constexpr uint32_t PRIVATE_INFORMATION_RETRIEVAL = 1;
constexpr uint32_t DATA_SET_SIZE_128 = 0;
constexpr uint32_t DATA_SET_SIZE_256 = 1;
constexpr uint32_t DATA_SET_SIZE_512 = 2;
constexpr uint32_t HASH_ALG_NONE = 0;
constexpr uint32_t HASH_ALG_SHA256 = 1;
constexpr uint32_t HASH_ALG_SHA384 = 2;
constexpr uint32_t HASH_ALG_SHA512 = 3;
constexpr const char *PRIVATE_SET_INTERSECTION_PROTOCOL = "PSI_PROTOCOL";
constexpr const char *PRIVATE_INFORMATION_RETRIEVAL_PROTOCOL = "PIR_PROTOCOL";

napi_value NapiGenPrivacyTarget(napi_env env, napi_callback_info info)
{
    return CallComputationNapiFunc(env, info, "GenPrivacyTarget");
}

napi_value NapiPrivacySearch(napi_env env, napi_callback_info info)
{
    return CallComputationNapiFunc(env, info, "PrivacySearch");
}

napi_value NapiGetSearchResult(napi_env env, napi_callback_info info)
{
    return CallComputationNapiFunc(env, info, "GetSearchResult");
}

napi_status NapiSetProperty(napi_env env, napi_value object, const char *propertyName, uint32_t propertyValue)
{
    napi_value property = nullptr;
    napi_status status = napi_create_uint32(env, propertyValue, &property);
    if (status != napi_ok) {
        GET_AND_THROW_LAST_ERROR(env);
        return status;
    }
    status = napi_set_named_property(env, object, propertyName, property);
    if (status != napi_ok) {
        GET_AND_THROW_LAST_ERROR(env);
        return status;
    }
    return napi_ok;
}

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

napi_value Register(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("genPrivacyTarget", NapiGenPrivacyTarget),
        DECLARE_NAPI_FUNCTION("privacySearch", NapiPrivacySearch),
        DECLARE_NAPI_FUNCTION("getSearchResult", NapiGetSearchResult),
        DECLARE_NAPI_PROPERTY("ProtocolType", DeclareProtocolType(env)),
        DECLARE_NAPI_PROPERTY("DataSetSize", DeclareDataSetSize(env)),
        DECLARE_NAPI_PROPERTY("HashAlg", DeclareHashAlg(env)),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Register,
    .nm_modname = "security.privacyComputation",
    .nm_priv = nullptr,
    .reserved = { nullptr },
};

} // namespace OHOS::Security::PrivacyComputation

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&OHOS::Security::PrivacyComputation::g_module);
}
