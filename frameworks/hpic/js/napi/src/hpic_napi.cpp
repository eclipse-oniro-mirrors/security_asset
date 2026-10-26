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

#include "hpic_napi_gen_privacy_target.h"
#include "hpic_napi_get_search_result.h"
#include "hpic_napi_privacy_search.h"
#include "hpic_napi_utils.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS::Security::PrivacyComputation {
constexpr uint32_t PRIVATE_SET_INTERSECTION = 0;
constexpr uint32_t PRIVATE_INFORMATION_RETRIEVAL = 1;
constexpr uint32_t DATA_SET_SIZE_128 = 0;
constexpr uint32_t DATA_SET_SIZE_256 = 1;
constexpr uint32_t DATA_SET_SIZE_512 = 2;
constexpr const char *PRIVATE_SET_INTERSECTION_PROTOCOL = "PSI_PROTOCOL";
constexpr const char *PRIVATE_INFORMATION_RETRIEVAL_PROTOCOL = "PIR_PROTOCOL";

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
