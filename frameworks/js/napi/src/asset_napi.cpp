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

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_napi_add.h"
#include "asset_napi_get_version.h"
#include "asset_napi_post_query.h"
#include "asset_napi_pre_query.h"
#include "asset_napi_query.h"
#include "asset_napi_remove.h"
#include "asset_napi_update.h"
#include "asset_type.h"

using namespace OHOS::Security::Asset;

namespace {
    void AddUint32Property(napi_env env, napi_value object, const char *name, uint32_t value)
    {
        napi_value property = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_create_uint32(env, value, &property));
        NAPI_CALL_RETURN_VOID(env, napi_set_named_property(env, object, name, property));
    }

    napi_value DeclareTag(napi_env env)
    {
        napi_value tag = nullptr;
        NAPI_CALL(env, napi_create_object(env, &tag));
        AddUint32Property(env, tag, "SECRET", SECRET);
        AddUint32Property(env, tag, "ALIAS", ALIAS);
        AddUint32Property(env, tag, "ACCESSIBILITY", ACCESSIBILITY);
        AddUint32Property(env, tag, "AUTH_TYPE", AUTH_TYPE);
        AddUint32Property(env, tag, "AUTH_VALIDITY_PERIOD", AUTH_VALIDITY_PERIOD);
        AddUint32Property(env, tag, "AUTH_CHALLENGE", AUTH_CHALLENGE);
        AddUint32Property(env, tag, "AUTH_TOKEN", AUTH_TOKEN);
        AddUint32Property(env, tag, "SYNC_TYPE", SYNC_TYPE);
        AddUint32Property(env, tag, "CONFLICT_POLICY", CONFLICT_POLICY);
        AddUint32Property(env, tag, "DATA_LABLE_CRITICAL_1", DATA_LABLE_CRITICAL_1);
        AddUint32Property(env, tag, "DATA_LABLE_CRITICAL_2", DATA_LABLE_CRITICAL_2);
        AddUint32Property(env, tag, "DATA_LABLE_CRITICAL_3", DATA_LABLE_CRITICAL_3);
        AddUint32Property(env, tag, "DATA_LABLE_CRITICAL_4", DATA_LABLE_CRITICAL_4);
        AddUint32Property(env, tag, "DATA_LABLE_NORMAL_1", DATA_LABLE_NORMAL_1);
        AddUint32Property(env, tag, "DATA_LABLE_NORMAL_2", DATA_LABLE_NORMAL_2);
        AddUint32Property(env, tag, "DATA_LABLE_NORMAL_3", DATA_LABLE_NORMAL_3);
        AddUint32Property(env, tag, "DATA_LABLE_NORMAL_4", DATA_LABLE_NORMAL_4);
        AddUint32Property(env, tag, "RETURN_TYPE", RETURN_TYPE);
        AddUint32Property(env, tag, "RETURN_LIMIT", RETURN_LIMIT);
        AddUint32Property(env, tag, "RETURN_OFFSET", RETURN_OFFSET);
        AddUint32Property(env, tag, "RETURN_ORDER_BY", RETURN_ORDER_BY);
        return tag;
    }

    napi_value DeclareErrorCode(napi_env env)
    {
        napi_value errorCode = nullptr;
        NAPI_CALL(env, napi_create_object(env, &errorCode));
        AddUint32Property(env, errorCode, "PERMISSION_DENIED", PERMISSION_DENIED);
        AddUint32Property(env, errorCode, "INVALID_ARGUMENT", INVALID_ARGUMENT);
        AddUint32Property(env, errorCode, "NOT_SUPPORTED", NOT_SUPPORTED);
        AddUint32Property(env, errorCode, "SERVICE_UNAVAILABLE", SERVICE_UNAVAILABLE);
        AddUint32Property(env, errorCode, "NOT_FOUND", NOT_FOUND);
        AddUint32Property(env, errorCode, "DUPLICATED", DUPLICATED);
        AddUint32Property(env, errorCode, "ACCESS_DENIED", ACCESS_DENIED);
        AddUint32Property(env, errorCode, "AUTH_TOKEN_EXPIRED", AUTH_TOKEN_EXPIRED);
        AddUint32Property(env, errorCode, "OUT_OF_MEMRORY", OUT_OF_MEMRORY);
        AddUint32Property(env, errorCode, "DATA_CORRUPTED", DATA_CORRUPTED);
        return errorCode;
    }

    napi_value DeclareAccessibility(napi_env env)
    {
        napi_value accessibility = nullptr;
        NAPI_CALL(env, napi_create_object(env, &accessibility));
        AddUint32Property(env, accessibility, "DEVICE_POWER_ON", DEVICE_POWER_ON);
        AddUint32Property(env, accessibility, "DEVICE_FIRST_UNLOCK", DEVICE_FIRST_UNLOCK);
        AddUint32Property(env, accessibility, "DEVICE_UNLOCK", DEVICE_UNLOCK);
        AddUint32Property(env, accessibility, "DEVICE_SECURE", DEVICE_SECURE);
        return accessibility;
    }

    napi_value DeclareAuthType(napi_env env)
    {
        napi_value authType = nullptr;
        NAPI_CALL(env, napi_create_object(env, &authType));
        AddUint32Property(env, authType, "NONE", NONE);
        AddUint32Property(env, authType, "ANY", ANY);
        return authType;
    }

    napi_value DeclareSyncType(napi_env env)
    {
        napi_value syncType = nullptr;
        NAPI_CALL(env, napi_create_object(env, &syncType));
        AddUint32Property(env, syncType, "NEVER", NEVER);
        AddUint32Property(env, syncType, "THIS_DEVICE", THIS_DEVICE);
        AddUint32Property(env, syncType, "TRUSTED_ACCOUNT", TRUSTED_ACCOUNT);
        AddUint32Property(env, syncType, "TRUSTED_DEVICE", TRUSTED_DEVICE);
        return syncType;
    }

    napi_value DeclareConflictPolicy(napi_env env)
    {
        napi_value conflictPolicy = nullptr;
        NAPI_CALL(env, napi_create_object(env, &conflictPolicy));
        AddUint32Property(env, conflictPolicy, "OVERRIDE", OVERRIDE);
        AddUint32Property(env, conflictPolicy, "REPORT", REPORT);
        return conflictPolicy;
    }

    napi_value DeclareReturnType(napi_env env)
    {
        napi_value returnType = nullptr;
        NAPI_CALL(env, napi_create_object(env, &returnType));
        AddUint32Property(env, returnType, "ALL", ALL);
        AddUint32Property(env, returnType, "ATTRIBUTES", ATTRIBUTES);
        return returnType;
    }

    napi_value Register(napi_env env, napi_value exports)
    {
        napi_property_descriptor desc[] = {
            // function register
            DECLARE_NAPI_FUNCTION("add", NapiAdd),
            DECLARE_NAPI_FUNCTION("remove", NapiRemove),
            DECLARE_NAPI_FUNCTION("update", NapiUpdate),
            DECLARE_NAPI_FUNCTION("preQuery", NapiPreQuery),
            DECLARE_NAPI_FUNCTION("query", NapiQuery),
            DECLARE_NAPI_FUNCTION("postQuery", NapiPostQuery),
            DECLARE_NAPI_FUNCTION("getVersion", NapiGetVersion),

            // enum register
            DECLARE_NAPI_PROPERTY("Tag", DeclareTag(env)),
            DECLARE_NAPI_PROPERTY("ErrorCode", DeclareErrorCode(env)),
            DECLARE_NAPI_PROPERTY("Accessibility", DeclareAccessibility(env)),
            DECLARE_NAPI_PROPERTY("AuthType", DeclareAuthType(env)),
            DECLARE_NAPI_PROPERTY("SyncType", DeclareSyncType(env)),
            DECLARE_NAPI_PROPERTY("ConflictPolicy", DeclareConflictPolicy(env)),
            DECLARE_NAPI_PROPERTY("ReturnType", DeclareReturnType(env)),
        };

        NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
        return exports;
    }

    napi_module g_module = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = Register,
        .nm_modname = "security.asset",
        .nm_priv = static_cast<void *>(0),
        .reserved = { 0 },
    };
}

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_module);
}
