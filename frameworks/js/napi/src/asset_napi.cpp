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

#include "asset_napi_common.h"
#include "asset_system_api.h"
#include "asset_system_type.h"

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
    AddUint32Property(env, tag, "SECRET", ASSET_SYSTEM_TAG_SECRET);
    AddUint32Property(env, tag, "ALIAS", ASSET_SYSTEM_TAG_ALIAS);
    AddUint32Property(env, tag, "ACCESSIBILITY", ASSET_SYSTEM_TAG_ACCESSIBILITY);
    AddUint32Property(env, tag, "REQUIRE_PASSWORD_SET", ASSET_SYSTEM_TAG_REQUIRE_PASSWORD_SET);
    AddUint32Property(env, tag, "AUTH_TYPE", ASSET_SYSTEM_TAG_AUTH_TYPE);
    AddUint32Property(env, tag, "AUTH_VALIDITY_PERIOD", ASSET_SYSTEM_TAG_AUTH_VALIDITY_PERIOD);
    AddUint32Property(env, tag, "AUTH_CHALLENGE", ASSET_SYSTEM_TAG_AUTH_CHALLENGE);
    AddUint32Property(env, tag, "AUTH_TOKEN", ASSET_SYSTEM_TAG_AUTH_TOKEN);
    AddUint32Property(env, tag, "SYNC_TYPE", ASSET_SYSTEM_TAG_SYNC_TYPE);
    AddUint32Property(env, tag, "IS_PERSISTENT", ASSET_SYSTEM_TAG_IS_PERSISTENT);
    AddUint32Property(env, tag, "CONFLICT_RESOLUTION", ASSET_SYSTEM_TAG_CONFLICT_RESOLUTION);
    AddUint32Property(env, tag, "DATA_LABEL_CRITICAL_1", ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_1);
    AddUint32Property(env, tag, "DATA_LABEL_CRITICAL_2", ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_2);
    AddUint32Property(env, tag, "DATA_LABEL_CRITICAL_3", ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_3);
    AddUint32Property(env, tag, "DATA_LABEL_CRITICAL_4", ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_4);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_1", ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_1);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_2", ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_2);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_3", ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_3);
    AddUint32Property(env, tag, "DATA_LABEL_NORMAL_4", ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_4);
    AddUint32Property(env, tag, "RETURN_TYPE", ASSET_SYSTEM_TAG_RETURN_TYPE);
    AddUint32Property(env, tag, "RETURN_LIMIT", ASSET_SYSTEM_TAG_RETURN_LIMIT);
    AddUint32Property(env, tag, "RETURN_OFFSET", ASSET_SYSTEM_TAG_RETURN_OFFSET);
    AddUint32Property(env, tag, "RETURN_ORDERED_BY", ASSET_SYSTEM_TAG_RETURN_ORDERED_BY);
    return tag;
}

napi_value DeclareTagType(napi_env env)
{
    napi_value tagType = nullptr;
    NAPI_CALL(env, napi_create_object(env, &tagType));
    AddUint32Property(env, tagType, "BOOL", ASSET_SYSTEM_TYPE_BOOL);
    AddUint32Property(env, tagType, "NUMBER", ASSET_SYSTEM_TYPE_NUMBER);
    AddUint32Property(env, tagType, "BYTES", ASSET_SYSTEM_TYPE_BYTES);
    return tagType;
}

napi_value DeclareErrorCode(napi_env env)
{
    napi_value errorCode = nullptr;
    NAPI_CALL(env, napi_create_object(env, &errorCode));
    AddUint32Property(env, errorCode, "PERMISSION_DENIED", ASSET_SYSTEM_PERMISSION_DENIED);
    AddUint32Property(env, errorCode, "INVALID_ARGUMENT", ASSET_SYSTEM_INVALID_ARGUMENT);
    AddUint32Property(env, errorCode, "SERVICE_UNAVAILABLE", ASSET_SYSTEM_SERVICE_UNAVAILABLE);
    AddUint32Property(env, errorCode, "NOT_FOUND", ASSET_SYSTEM_NOT_FOUND);
    AddUint32Property(env, errorCode, "DUPLICATED", ASSET_SYSTEM_DUPLICATED);
    AddUint32Property(env, errorCode, "ACCESS_DENIED", ASSET_SYSTEM_ACCESS_DENIED);
    AddUint32Property(env, errorCode, "STATUS_MISMATCH", ASSET_SYSTEM_STATUS_MISMATCH);
    AddUint32Property(env, errorCode, "OUT_OF_MEMORY", ASSET_SYSTEM_OUT_OF_MEMORY);
    AddUint32Property(env, errorCode, "DATA_CORRUPTED", ASSET_SYSTEM_DATA_CORRUPTED);
    AddUint32Property(env, errorCode, "DATABASE_ERROR", ASSET_SYSTEM_DATABASE_ERROR);
    AddUint32Property(env, errorCode, "CRYPTO_ERROR", ASSET_SYSTEM_CRYPTO_ERROR);
    AddUint32Property(env, errorCode, "IPC_ERROR", ASSET_SYSTEM_IPC_ERROR);
    AddUint32Property(env, errorCode, "BMS_ERROR", ASSET_SYSTEM_BMS_ERROR);
    AddUint32Property(env, errorCode, "ACCOUNT_ERROR", ASSET_SYSTEM_ACCOUNT_ERROR);
    AddUint32Property(env, errorCode, "ACCESS_TOKEN_ERROR", ASSET_SYSTEM_ACCESS_TOKEN_ERROR);
    AddUint32Property(env, errorCode, "FILE_OPERATION_ERROR", ASSET_SYSTEM_FILE_OPERATION_ERROR);
    AddUint32Property(env, errorCode, "GET_SYSTEM_TIME_ERROR", ASSET_SYSTEM_GET_SYSTEM_TIME_ERROR);
    AddUint32Property(env, errorCode, "LIMIT_EXCEEDED", ASSET_SYSTEM_LIMIT_EXCEEDED);
    AddUint32Property(env, errorCode, "UNSUPPORTED", ASSET_SYSTEM_UNSUPPORTED);
    return errorCode;
}

napi_value DeclareAccessibility(napi_env env)
{
    napi_value accessibility = nullptr;
    NAPI_CALL(env, napi_create_object(env, &accessibility));
    AddUint32Property(env, accessibility, "DEVICE_POWERED_ON", ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON);
    AddUint32Property(env, accessibility, "DEVICE_FIRST_UNLOCKED", ASSET_SYSTEM_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED);
    AddUint32Property(env, accessibility, "DEVICE_UNLOCKED", ASSET_SYSTEM_ACCESSIBILITY_DEVICE_UNLOCKED);
    return accessibility;
}

napi_value DeclareAuthType(napi_env env)
{
    napi_value authType = nullptr;
    NAPI_CALL(env, napi_create_object(env, &authType));
    AddUint32Property(env, authType, "NONE", ASSET_SYSTEM_AUTH_TYPE_NONE);
    AddUint32Property(env, authType, "ANY", ASSET_SYSTEM_AUTH_TYPE_ANY);
    return authType;
}

napi_value DeclareSyncType(napi_env env)
{
    napi_value syncType = nullptr;
    NAPI_CALL(env, napi_create_object(env, &syncType));
    AddUint32Property(env, syncType, "NEVER", ASSET_SYSTEM_SYNC_TYPE_NEVER);
    AddUint32Property(env, syncType, "THIS_DEVICE", ASSET_SYSTEM_SYNC_TYPE_THIS_DEVICE);
    AddUint32Property(env, syncType, "TRUSTED_DEVICE", ASSET_SYSTEM_SYNC_TYPE_TRUSTED_DEVICE);
    return syncType;
}

napi_value DeclareConflictResolution(napi_env env)
{
    napi_value conflictResolution = nullptr;
    NAPI_CALL(env, napi_create_object(env, &conflictResolution));
    AddUint32Property(env, conflictResolution, "OVERWRITE", ASSET_SYSTEM_CONFLICT_OVERWRITE);
    AddUint32Property(env, conflictResolution, "THROW_ERROR", ASSET_SYSTEM_CONFLICT_THROW_ERROR);
    return conflictResolution;
}

napi_value DeclareReturnType(napi_env env)
{
    napi_value returnType = nullptr;
    NAPI_CALL(env, napi_create_object(env, &returnType));
    AddUint32Property(env, returnType, "ALL", ASSET_SYSTEM_RETURN_ALL);
    AddUint32Property(env, returnType, "ATTRIBUTES", ASSET_SYSTEM_RETURN_ATTRIBUTES);
    return returnType;
}

napi_value NapiAdd(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetAdd(&context->attrs[0], context->attrs.size());
        };
    return NapiEntry(env, info, __func__, execute);
}

napi_value NapiAddSync(napi_env env, napi_callback_info info)
{
    std::vector<AssetAttr> attrs;
    do {
        if (ParseParam(env, info, attrs) != napi_ok) {
            break;
        }

        int32_t result = AssetAdd(&attrs[0], attrs.size());
        CHECK_RESULT_BREAK(env, result);
    } while (false);
    FreeAssetAttrs(attrs);
    return nullptr;
}

napi_value NapiAddAsUser(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetAdd(&context->attrs[0], context->attrs.size());
        };
    return NapiEntry(env, info, __func__, execute, AS_USER_ARGS_NUM, false, true);
}

napi_value NapiRemove(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetRemove(&context->attrs[0], context->attrs.size());
        };
    return NapiEntry(env, info, __func__, execute);
}

napi_value NapiRemoveSync(napi_env env, napi_callback_info info)
{
    std::vector<AssetAttr> attrs;
    do {
        if (ParseParam(env, info, attrs) != napi_ok) {
            break;
        }

        int32_t result = AssetRemove(&attrs[0], attrs.size());
        CHECK_RESULT_BREAK(env, result);
    } while (false);
    FreeAssetAttrs(attrs);
    return nullptr;
}

napi_value NapiRemoveAsUser(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetRemove(&context->attrs[0], context->attrs.size());
        };
    return NapiEntry(env, info, __func__, execute, AS_USER_ARGS_NUM, false, true);
}

napi_value NapiUpdate(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetUpdate(&context->attrs[0], context->attrs.size(),
                &context->updateAttrs[0], context->updateAttrs.size());
        };
    return NapiEntry(env, info, __func__, execute, UPDATE_ARGS_NUM, true);
}

napi_value NapiUpdateSync(napi_env env, napi_callback_info info)
{
    std::vector<AssetAttr> attrs;
    std::vector<AssetAttr> updateAttrs;
    do {
        if (ParseParam(env, info, UPDATE_ARGS_NUM, attrs, updateAttrs, true, false) != napi_ok) {
            break;
        }
        int32_t result = AssetUpdate(&attrs[0], attrs.size(), &updateAttrs[0], updateAttrs.size());
        CHECK_RESULT_BREAK(env, result);
    } while (false);
    FreeAssetAttrs(attrs);
    FreeAssetAttrs(updateAttrs);
    return nullptr;
}

napi_value NapiUpdateAsUser(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetUpdate(&context->attrs[0], context->attrs.size(),
                &context->updateAttrs[0], context->updateAttrs.size());
        };
    return NapiEntry(env, info, __func__, execute, UPDATE_ARGS_NUM + 1, true, true);
}

napi_value NapiPreQuery(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetPreQuery(&context->attrs[0], context->attrs.size(), &context->challenge);
        };
    return NapiEntry(env, info, __func__, execute);
}

napi_value NapiPreQuerySync(napi_env env, napi_callback_info info)
{
    std::vector<AssetAttr> attrs;
    AssetBlob challenge = { 0 };
    napi_value result = nullptr;
    do {
        if (ParseParam(env, info, attrs) != napi_ok) {
            break;
        }

        int32_t res = AssetPreQuery(&attrs[0], attrs.size(), &challenge);
        CHECK_RESULT_BREAK(env, res);
        result = CreateJsUint8Array(env, challenge);
    } while (false);
    AssetFreeBlob(&challenge);
    FreeAssetAttrs(attrs);
    return result;
}

napi_value NapiPreQueryAsUser(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetPreQuery(&context->attrs[0], context->attrs.size(), &context->challenge);
        };
    return NapiEntry(env, info, __func__, execute, AS_USER_ARGS_NUM, false, true);
}

napi_value NapiQuery(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetQuery(&context->attrs[0], context->attrs.size(), &context->resultSet);
        };
    return NapiEntry(env, info, __func__, execute);
}

napi_value NapiQuerySync(napi_env env, napi_callback_info info)
{
    std::vector<AssetAttr> attrs;
    AssetResultSet resultSet = { 0 };
    napi_value result = nullptr;
    do {
        if (ParseParam(env, info, attrs) != napi_ok) {
            break;
        }

        int32_t res = AssetQuery(&attrs[0], attrs.size(), &resultSet);
        CHECK_RESULT_BREAK(env, res);
        result = CreateJsMapArray(env, resultSet);
    } while (false);
    AssetFreeResultSet(&resultSet);
    FreeAssetAttrs(attrs);
    return result;
}

napi_value NapiQueryAsUser(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetQuery(&context->attrs[0], context->attrs.size(), &context->resultSet);
        };
    return NapiEntry(env, info, __func__, execute, AS_USER_ARGS_NUM, false, true);
}

napi_value NapiPostQuery(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetPostQuery(&context->attrs[0], context->attrs.size());
        };
    return NapiEntry(env, info, __func__, execute);
}

napi_value NapiPostQuerySync(napi_env env, napi_callback_info info)
{
    std::vector<AssetAttr> attrs;
    do {
        if (ParseParam(env, info, attrs) != napi_ok) {
            break;
        }

        int32_t result = AssetPostQuery(&attrs[0], attrs.size());
        CHECK_RESULT_BREAK(env, result);
    } while (false);
    FreeAssetAttrs(attrs);
    return nullptr;
}

napi_value NapiPostQueryAsUser(napi_env env, napi_callback_info info)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetPostQuery(&context->attrs[0], context->attrs.size());
        };
    return NapiEntry(env, info, __func__, execute, AS_USER_ARGS_NUM, false, true);
}

napi_value Register(napi_env env, napi_value exports)
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
