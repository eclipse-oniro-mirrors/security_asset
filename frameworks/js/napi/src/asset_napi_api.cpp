/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "asset_napi_test.h"

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace AssetNapi {
inline void AddInt32Property(napi_env env, napi_value object, const char *name, int32_t value)
{
    napi_value property = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_create_int32(env, value, &property));
    NAPI_CALL_RETURN_VOID(env, napi_set_named_property(env, object, name, property));
}

static void AddAssetTagPart(napi_env env, napi_value tag)
{
    /* Invalid TAG */
    AddInt32Property(env, tag, "TEST_TAG", 0);
}

static napi_value CreateAssetTag(napi_env env)
{
    napi_value tag = nullptr;
    NAPI_CALL(env, napi_create_object(env, &tag));

    AddAssetTagPart(env, tag);

    return tag;
}

static napi_value CreateAssetEnum(napi_env env)
{
    napi_value keyStorageType = nullptr;
    NAPI_CALL(env, napi_create_object(env, &keyStorageType));

    AddInt32Property(env, keyStorageType, "TEST_ENUM", 0);

    return keyStorageType;
}


static void AddAssetErrorCodePart(napi_env env, napi_value errorCode)
{
    AddInt32Property(env, errorCode, "TEST_ERROR_CODE", 0);
}

static napi_value CreateAssetErrorCode(napi_env env)
{
    napi_value errorCode = nullptr;
    NAPI_CALL(env, napi_create_object(env, &errorCode));

    AddAssetErrorCodePart(env, errorCode);

    return errorCode;
}
}  // namespace AssetNapi

using namespace AssetNapi;

extern "C" {
static napi_value AssetNapiRegister(napi_env env, napi_value exports)
{
    napi_property_descriptor assetDesc[] = {
        DECLARE_NAPI_PROPERTY("AssetErrorCode", CreateAssetErrorCode(env)),
        DECLARE_NAPI_PROPERTY("AssetTag", CreateAssetTag(env)),
        DECLARE_NAPI_PROPERTY("AssetEnum", CreateAssetEnum(env)),

        DECLARE_NAPI_FUNCTION("AssetTest", JsAssetTest),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(assetDesc) / sizeof(assetDesc[0]), assetDesc));
    return exports;
}

static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = AssetNapiRegister,
    .nm_modname = "security.asset",
    .nm_priv = reinterpret_cast<void *>(0),
    .reserved = { 0 },
};

__attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_module);
}
}
