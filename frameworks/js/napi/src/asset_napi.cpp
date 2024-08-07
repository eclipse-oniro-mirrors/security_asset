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

#include <cstdint>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_system_api.h"
#include "asset_system_type.h"

#include "asset_napi_add.h"
#include "asset_napi_post_query.h"
#include "asset_napi_pre_query.h"
#include "asset_napi_query.h"
#include "asset_napi_remove.h"
#include "asset_napi_update.h"

using namespace OHOS::Security::Asset;

namespace {

void AddUint32Property(const napi_env env, napi_value object, const char *name, uint32_t value)
{
    napi_value property = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_create_uint32(env, value, &property));
    NAPI_CALL_RETURN_VOID(env, napi_set_named_property(env, object, name, property));
}

napi_value DeclareTag(const napi_env env)
{
    napi_value tag = nullptr;
    NAPI_CALL(env, napi_create_object(env, &tag));
    AddUint32Property(env, tag, "SECRET", SEC_ASSET_TAG_SECRET);
    AddUint32Property(env, tag, "ALIAS", SEC_ASSET_TAG_ALIAS);
    AddUint32Property(env, tag, "ACCESSIBILITY", SEC_ASSET_TAG_ACCESSIBILITY);
    AddUint32Property(env, tag, "REQUIRE_PASSWORD_SET", SEC_ASSET_TAG_REQUIRE_PASSWORD_SET);
    AddUint32Property(env, tag, "AUTH_TYPE", SEC_ASSET_TAG_AUTH_TYPE);
    AddUint32Property(env, tag, "AUTH_VALIDITY_PERIOD", SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD);
    AddUint32Property(env, tag, "AUTH_CHALLENGE", SEC_ASSET_TAG_AUTH_CHALLENGE);
    AddUint32Property(env, tag, "AUTH_TOKEN", SEC_ASSET_TAG_AUTH_TOKEN);
    AddUint32Property(env, tag, "SYNC_TYPE", SEC_ASSET_TAG_SYNC_TYPE);
    AddUint32Property(env, tag, "IS_PERSISTENT", SEC_ASSET_TAG_IS_PERSISTENT);
    AddUint32Property(env, tag, "CONFLICT_RESOLUTION", SEC_ASSET_TAG_CONFLICT_RESOLUTION);
    AddUint32Property(env, tag, "DATA_LABEL_CRITICAL_1", SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1);
    AddUint32Property(env, tag, "DATA_LABEL_CRITICAL_2", SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2);
    AddUint32Property(env, tag, "DATA_LABEL_CRITICAL_3", SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3);
    AddUint32Property(env, tag, "DATA_LABEL_CRITICAL_4", SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_1", SEC_ASSET_TAG_DATA_LABEL_NORMAL_1);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_2", SEC_ASSET_TAG_DATA_LABEL_NORMAL_2);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_3", SEC_ASSET_TAG_DATA_LABEL_NORMAL_3);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_4", SEC_ASSET_TAG_DATA_LABEL_NORMAL_4);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_LOCAL_1", SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_LOCAL_2", SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_LOCAL_3", SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_LOCAL_4", SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4);
    AddUint32Property(env, tag, "RETURN_TYPE", SEC_ASSET_TAG_RETURN_TYPE);
    AddUint32Property(env, tag, "RETURN_LIMIT", SEC_ASSET_TAG_RETURN_LIMIT);
    AddUint32Property(env, tag, "RETURN_OFFSET", SEC_ASSET_TAG_RETURN_OFFSET);
    AddUint32Property(env, tag, "RETURN_ORDERED_BY", SEC_ASSET_TAG_RETURN_ORDERED_BY);
    AddUint32Property(env, tag, "UPDATE_TIME", SEC_ASSET_TAG_UPDATE_TIME);
    AddUint32Property(env, tag, "OPERATION_TYPE", SEC_ASSET_TAG_OPERATION_TYPE);
    return tag;
}

napi_value DeclareTagType(const napi_env env)
{
    napi_value tagType = nullptr;
    NAPI_CALL(env, napi_create_object(env, &tagType));
    AddUint32Property(env, tagType, "BOOL", SEC_ASSET_TYPE_BOOL);
    AddUint32Property(env, tagType, "NUMBER", SEC_ASSET_TYPE_NUMBER);
    AddUint32Property(env, tagType, "BYTES", SEC_ASSET_TYPE_BYTES);
    return tagType;
}

napi_value DeclareErrorCode(const napi_env env)
{
    napi_value errorCode = nullptr;
    NAPI_CALL(env, napi_create_object(env, &errorCode));
    AddUint32Property(env, errorCode, "PERMISSION_DENIED", SEC_ASSET_PERMISSION_DENIED);
    AddUint32Property(env, errorCode, "NOT_SYSTEM_APPLICATION", SEC_ASSET_NOT_SYSTEM_APPLICATION);
    AddUint32Property(env, errorCode, "INVALID_ARGUMENT", SEC_ASSET_INVALID_ARGUMENT);
    AddUint32Property(env, errorCode, "SERVICE_UNAVAILABLE", SEC_ASSET_SERVICE_UNAVAILABLE);
    AddUint32Property(env, errorCode, "NOT_FOUND", SEC_ASSET_NOT_FOUND);
    AddUint32Property(env, errorCode, "DUPLICATED", SEC_ASSET_DUPLICATED);
    AddUint32Property(env, errorCode, "ACCESS_DENIED", SEC_ASSET_ACCESS_DENIED);
    AddUint32Property(env, errorCode, "STATUS_MISMATCH", SEC_ASSET_STATUS_MISMATCH);
    AddUint32Property(env, errorCode, "OUT_OF_MEMORY", SEC_ASSET_OUT_OF_MEMORY);
    AddUint32Property(env, errorCode, "DATA_CORRUPTED", SEC_ASSET_DATA_CORRUPTED);
    AddUint32Property(env, errorCode, "DATABASE_ERROR", SEC_ASSET_DATABASE_ERROR);
    AddUint32Property(env, errorCode, "CRYPTO_ERROR", SEC_ASSET_CRYPTO_ERROR);
    AddUint32Property(env, errorCode, "IPC_ERROR", SEC_ASSET_IPC_ERROR);
    AddUint32Property(env, errorCode, "BMS_ERROR", SEC_ASSET_BMS_ERROR);
    AddUint32Property(env, errorCode, "ACCOUNT_ERROR", SEC_ASSET_ACCOUNT_ERROR);
    AddUint32Property(env, errorCode, "ACCESS_TOKEN_ERROR", SEC_ASSET_ACCESS_TOKEN_ERROR);
    AddUint32Property(env, errorCode, "FILE_OPERATION_ERROR", SEC_ASSET_FILE_OPERATION_ERROR);
    AddUint32Property(env, errorCode, "GET_SYSTEM_TIME_ERROR", SEC_ASSET_GET_SYSTEM_TIME_ERROR);
    AddUint32Property(env, errorCode, "LIMIT_EXCEEDED", SEC_ASSET_LIMIT_EXCEEDED);
    AddUint32Property(env, errorCode, "UNSUPPORTED", SEC_ASSET_UNSUPPORTED);
    return errorCode;
}

napi_value DeclareAccessibility(const napi_env env)
{
    napi_value accessibility = nullptr;
    NAPI_CALL(env, napi_create_object(env, &accessibility));
    AddUint32Property(env, accessibility, "DEVICE_POWERED_ON", SEC_ASSET_ACCESSIBILITY_DEVICE_POWERED_ON);
    AddUint32Property(env, accessibility, "DEVICE_FIRST_UNLOCKED", SEC_ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED);
    AddUint32Property(env, accessibility, "DEVICE_UNLOCKED", SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED);
    return accessibility;
}

napi_value DeclareAuthType(const napi_env env)
{
    napi_value authType = nullptr;
    NAPI_CALL(env, napi_create_object(env, &authType));
    AddUint32Property(env, authType, "NONE", SEC_ASSET_AUTH_TYPE_NONE);
    AddUint32Property(env, authType, "ANY", SEC_ASSET_AUTH_TYPE_ANY);
    return authType;
}

napi_value DeclareSyncType(const napi_env env)
{
    napi_value syncType = nullptr;
    NAPI_CALL(env, napi_create_object(env, &syncType));
    AddUint32Property(env, syncType, "NEVER", SEC_ASSET_SYNC_TYPE_NEVER);
    AddUint32Property(env, syncType, "THIS_DEVICE", SEC_ASSET_SYNC_TYPE_THIS_DEVICE);
    AddUint32Property(env, syncType, "TRUSTED_DEVICE", SEC_ASSET_SYNC_TYPE_TRUSTED_DEVICE);
    AddUint32Property(env, syncType, "TRUSTED_ACCOUNT", SEC_ASSET_SYNC_TYPE_TRUSTED_ACCOUNT);
    return syncType;
}

napi_value DeclareConflictResolution(const napi_env env)
{
    napi_value conflictResolution = nullptr;
    NAPI_CALL(env, napi_create_object(env, &conflictResolution));
    AddUint32Property(env, conflictResolution, "OVERWRITE", SEC_ASSET_CONFLICT_OVERWRITE);
    AddUint32Property(env, conflictResolution, "THROW_ERROR", SEC_ASSET_CONFLICT_THROW_ERROR);
    return conflictResolution;
}

napi_value DeclareReturnType(const napi_env env)
{
    napi_value returnType = nullptr;
    NAPI_CALL(env, napi_create_object(env, &returnType));
    AddUint32Property(env, returnType, "ALL", SEC_ASSET_RETURN_ALL);
    AddUint32Property(env, returnType, "ATTRIBUTES", SEC_ASSET_RETURN_ATTRIBUTES);
    return returnType;
}

napi_value DeclareOperationType(const napi_env env)
{
    napi_value operationType = nullptr;
    NAPI_CALL(env, napi_create_object(env, &operationType));
    AddUint32Property(env, operationType, "NEED_SYNC", SEC_ASSET_NEED_SYNC);
    AddUint32Property(env, operationType, "NEED_LOGOUT", SEC_ASSET_NEED_LOGOUT);
    AddUint32Property(env, operationType, "NEED_SWITCH_OFF", SEC_ASSET_NEED_SWITCH_OFF);
    return operationType;
}

napi_value Register(const napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        // register function
        DECLARE_NAPI_FUNCTION("add", NapiAdd),
        DECLARE_NAPI_FUNCTION("addSync", NapiAddSync),
        DECLARE_NAPI_FUNCTION("addAsUser", NapiAddAsUser),
        DECLARE_NAPI_FUNCTION("remove", NapiRemove),
        DECLARE_NAPI_FUNCTION("removeSync", NapiRemoveSync),
        DECLARE_NAPI_FUNCTION("removeAsUser", NapiRemoveAsUser),
        DECLARE_NAPI_FUNCTION("update", NapiUpdate),
        DECLARE_NAPI_FUNCTION("updateSync", NapiUpdateSync),
        DECLARE_NAPI_FUNCTION("updateAsUser", NapiUpdateAsUser),
        DECLARE_NAPI_FUNCTION("preQuery", NapiPreQuery),
        DECLARE_NAPI_FUNCTION("preQuerySync", NapiPreQuerySync),
        DECLARE_NAPI_FUNCTION("preQueryAsUser", NapiPreQueryAsUser),
        DECLARE_NAPI_FUNCTION("query", NapiQuery),
        DECLARE_NAPI_FUNCTION("querySync", NapiQuerySync),
        DECLARE_NAPI_FUNCTION("queryAsUser", NapiQueryAsUser),
        DECLARE_NAPI_FUNCTION("postQuery", NapiPostQuery),
        DECLARE_NAPI_FUNCTION("postQuerySync", NapiPostQuerySync),
        DECLARE_NAPI_FUNCTION("postQueryAsUser", NapiPostQueryAsUser),

        // register enumerate
        DECLARE_NAPI_PROPERTY("Tag", DeclareTag(env)),
        DECLARE_NAPI_PROPERTY("TagType", DeclareTagType(env)),
        DECLARE_NAPI_PROPERTY("ErrorCode", DeclareErrorCode(env)),
        DECLARE_NAPI_PROPERTY("Accessibility", DeclareAccessibility(env)),
        DECLARE_NAPI_PROPERTY("AuthType", DeclareAuthType(env)),
        DECLARE_NAPI_PROPERTY("SyncType", DeclareSyncType(env)),
        DECLARE_NAPI_PROPERTY("ConflictResolution", DeclareConflictResolution(env)),
        DECLARE_NAPI_PROPERTY("ReturnType", DeclareReturnType(env)),
        DECLARE_NAPI_PROPERTY("OperationType", DeclareOperationType(env)),
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

} // anonymous namespace

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_module);
}
