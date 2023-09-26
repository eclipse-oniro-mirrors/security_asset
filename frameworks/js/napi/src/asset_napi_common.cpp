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

#include "asset_napi_common.h"

#include <vector>

#include "securec.h"

#include "asset_api.h"
#include "asset_log.h"
#include "asset_mem.h"
#include "asset_napi_error_code.h"
#include "asset_type.h"

namespace OHOS {
namespace Security {
namespace Asset {
namespace {
#define MAX_BUFFER_LEN 1024
#define MAX_MESSAGE_LEN 128
#define CALLBACK_ARGS_NUM 2
#define MAX_ARGS_NUM 5

#define NAPI_THROW_BASE(env, condition, ret, code, message)             \
if ((condition)) {                                                      \
    LOGE("[FATAL] " message);                                           \
    napi_throw_error((env), std::to_string((code)).c_str(), message);   \
    return (ret);                                                       \
}

#define NAPI_THROW(env, condition, code, message)                       \
    NAPI_THROW_BASE(env, condition, nullptr, code, message)

#define NAPI_THROW_RETURN_ERR(env, condition, code, message)            \
    NAPI_THROW_BASE(env, condition, napi_generic_failure, code, message)

#define NAPI_CALL_BREAK(env, theCall)   \
if ((theCall) != napi_ok) {             \
    GET_AND_THROW_LAST_ERROR((env));    \
    break;                              \
}

#define NAPI_CALL_RETURN_ERR(env, theCall)  \
if ((theCall) != napi_ok) {                 \
    GET_AND_THROW_LAST_ERROR((env));        \
    return napi_generic_failure;            \
}

#define CHECK_ASSET_TAG(env, condition, tag, message)                                   \
if ((condition)) {                                                                      \
    char msg[MAX_MESSAGE_LEN] = { 0 };                                                  \
    (void)sprintf_s(msg, MAX_MESSAGE_LEN, "AssetTag(0x%08x) " message, tag);            \
    LOGE("[FATAL] %s", msg);                                                            \
    napi_throw_error((env), std::to_string(ASSET_INVALID_ARGUMENT).c_str(), msg);       \
    return napi_invalid_arg;                                                            \
}

void FreeAttrIfNeed(Asset_Attr &attr)
{
    if ((attr.tag & ASSET_TAG_TYPE_MASK) == ASSET_TYPE_BYTES) {
        OH_Asset_FreeAssetBlob(&attr.value.blob);
    }
}

void FreeAssetAttrs(AsyncContext *context)
{
    if (context->attrs != nullptr) {
        for (uint32_t i = 0; i < context->attrCnt; i++) {
            FreeAttrIfNeed(context->attrs[i]);
        }

        AssetFree(context->attrs);
        context->attrs = nullptr;
    }
    if (context->updateAttrs != nullptr) {
        for (uint32_t i = 0; i < context->updateAttrCnt; i++) {
            FreeAttrIfNeed(context->updateAttrs[i]);
        }

        AssetFree(context->updateAttrs);
        context->updateAttrs = nullptr;
    }
    OH_Asset_FreeAssetBlob(&context->challenge);
    // todo: delete reasultSet
}

AsyncContext *CreateAsyncContext()
{
    AsyncContext *context = static_cast<AsyncContext *>(AssetMalloc(sizeof(AsyncContext)));
    if (context != nullptr) {
        (void)memset_s(context, sizeof(AsyncContext), 0, sizeof(AsyncContext));
    }
    return context;
}

void DestroyAsyncContext(napi_env env, AsyncContext *context)
{
    if (context == nullptr) {
        return;
    }
    if (context->work != nullptr) {
        napi_delete_async_work(env, context->work);
        context->work = nullptr;
    }
    if (context->callback != nullptr) {
        napi_delete_reference(env, context->callback);
        context->callback = nullptr;
    }
    FreeAssetAttrs(context);
    AssetFree(context);
}

napi_status ParseByteArray(napi_env env, napi_value value, uint32_t tag, Asset_Blob &blob)
{
    napi_typedarray_type arrayType;
    size_t length = 0;
    void *rawData = nullptr;

    NAPI_CALL_RETURN_ERR(env, napi_get_typedarray_info(env, value, &arrayType, &length, &rawData, nullptr, nullptr));
    CHECK_ASSET_TAG(env, arrayType != napi_uint8_array, tag, "expect type napi_uint8_array");
    CHECK_ASSET_TAG(env, length == 0 || length > MAX_BUFFER_LEN, tag, "invalid array length.");

    blob.data = static_cast<uint8_t *>(AssetMalloc(length));
    NAPI_THROW_RETURN_ERR(env, blob.data == nullptr, ASSET_OUT_OF_MEMRORY, "Unable to allocate memory for Asset_Blob.");
    (void)memcpy_s(blob.data, length, rawData, length);
    blob.size = static_cast<uint32_t>(length);
    return napi_ok;
}

napi_status ParseAssetAttribute(napi_env env, napi_value tag, napi_value value, Asset_Attr &attr)
{
    // parse tag
    napi_valuetype type = napi_undefined;

    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, tag, &type));
    NAPI_THROW_RETURN_ERR(env, type != napi_number, ASSET_INVALID_ARGUMENT, "The tag type of map should be number.");
    NAPI_CALL_RETURN_ERR(env, napi_get_value_uint32(env, tag, &attr.tag));

    // parse value
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, value, &type));
    switch (attr.tag & ASSET_TAG_TYPE_MASK) {
        case ASSET_TYPE_BOOL:
            CHECK_ASSET_TAG(env, type != napi_boolean, attr.tag, "Expect type napi_boolean.");
            NAPI_CALL_RETURN_ERR(env, napi_get_value_bool(env, value, &attr.value.boolean));
            break;
        case ASSET_TYPE_UINT32:
            CHECK_ASSET_TAG(env, type != napi_number, attr.tag, "expect type napi_number");
            NAPI_CALL_RETURN_ERR(env, napi_get_value_uint32(env, value, &attr.value.u32));
            break;
        case ASSET_TYPE_BYTES:
            CHECK_ASSET_TAG(env, type != napi_object, attr.tag, "expect type napi_object");
            NAPI_CALL_RETURN_ERR(env, ParseByteArray(env, value, attr.tag, attr.value.blob));
            break;
        default:
            CHECK_ASSET_TAG(env, true, attr.tag, "Invalid tag argument.");
    }
    return napi_ok;
}

void FreeAttrVec(std::vector<Asset_Attr> &attrs)
{
    for (auto attr : attrs) {
        FreeAttrIfNeed(attr);
    }
}

napi_value GetIteratorNext(napi_env env, napi_value iterator, napi_value func, bool *done)
{
    napi_value next = nullptr;
    NAPI_CALL(env, napi_call_function(env, iterator, func, 0, nullptr, &next));

    napi_value doneValue = nullptr;
    NAPI_CALL(env, napi_get_named_property(env, next, "done", &doneValue));
    NAPI_CALL(env, napi_get_value_bool(env, doneValue, done));
    return next;
}

napi_status ParseMapParam(napi_env env, napi_value arg, Asset_Attr **attrs, uint32_t *attrCnt)
{
    // check map type
    bool isMap = false;
    NAPI_CALL_RETURN_ERR(env, napi_is_map(env, arg, &isMap));
    NAPI_THROW_RETURN_ERR(env, !isMap, ASSET_INVALID_ARGUMENT, "Expect Map type.");

    // parse map object
    napi_value entriesFunc = nullptr;
    napi_value iterator = nullptr;
    napi_value nextFunc = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_get_named_property(env, arg, "entries", &entriesFunc));
    NAPI_CALL_RETURN_ERR(env, napi_call_function(env, arg, entriesFunc, 0, nullptr, &iterator));
    NAPI_CALL_RETURN_ERR(env, napi_get_named_property(env, iterator, "next", &nextFunc));

    std::vector<Asset_Attr> attrVec;
    bool done = false;
    napi_value next = nullptr;
    while ((next = GetIteratorNext(env, iterator, nextFunc, &done)) != nullptr && !done) {
        napi_value entry = nullptr;
        napi_value key = nullptr;
        napi_value value = nullptr;
        NAPI_CALL_BREAK(env, napi_get_named_property(env, next, "value", &entry));
        NAPI_CALL_BREAK(env, napi_get_element(env, entry, 0, &key));
        NAPI_CALL_BREAK(env, napi_get_element(env, entry, 1, &value));

        Asset_Attr param;
        NAPI_CALL_BREAK(env, ParseAssetAttribute(env, key, value, param));
        attrVec.push_back(param);
    }

    if (!done || attrVec.size() == 0) {
        LOGE("[FATAL] vector size=%zu", attrVec.size());
        FreeAttrVec(attrVec);
        return napi_generic_failure;
    }

    // transfer vector to array
    *attrs = static_cast<Asset_Attr *>(AssetMalloc(attrVec.size() * sizeof(Asset_Attr)));
    if (*attrs == nullptr) {
        FreeAttrVec(attrVec);
        NAPI_THROW_RETURN_ERR(env, true, ASSET_OUT_OF_MEMRORY, "Unable to allocate memory for Asset_Attr array.");
    }
    (void)memcpy_s(*attrs, attrVec.size() * sizeof(Asset_Attr), &attrVec[0], attrVec.size() * sizeof(Asset_Attr));
    *attrCnt = attrVec.size();
    return napi_ok;
}

napi_status ParseCallbackParam(napi_env env, napi_value arg, napi_ref *callback)
{
    // check value type
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, arg, &valueType));
    NAPI_THROW_RETURN_ERR(env, valueType != napi_function, ASSET_INVALID_ARGUMENT, "Expect AsyncCallback type.");

    // create callback reference
    NAPI_CALL_RETURN_ERR(env, napi_create_reference(env, arg, 1, callback));
    return napi_ok;
}

napi_value GetUndefinedValue(napi_env env)
{
    napi_value value;
    NAPI_CALL(env, napi_get_undefined(env, &value));
    return value;
}

napi_value GetBusinessReturn(napi_env env, AsyncContext *context)
{
    napi_value result = nullptr;
    if (context->challenge.data != nullptr && context->challenge.size != 0) {
        // Create a temp array to store the challenge
        uint8_t *tmp = static_cast<uint8_t *>(AssetMalloc(context->challenge.size));
        NAPI_THROW(env, tmp == nullptr, ASSET_OUT_OF_MEMRORY, "Unable to allocate memory for challenge.");
        (void)memcpy_s(tmp, context->challenge.size, context->challenge.data, context->challenge.size);

        // Create napi array to store the challenge
        napi_value challenge = nullptr;
        napi_status status = napi_create_external_arraybuffer(
            env, context->challenge.data, context->challenge.size,
            [](napi_env env, void *data, void *hint) {
                AssetFree(data);
            },
            nullptr, &challenge);
        if (status != napi_ok) {
            AssetFree(tmp);
            GET_AND_THROW_LAST_ERROR(env);
            return nullptr;
        }
        NAPI_CALL(env, napi_create_typedarray(env, napi_uint8_array, context->challenge.size, challenge, 0, &result));
        return result;
    }

    if (context->resultSet.results != nullptr && context->resultSet.count != 0) {
        // todo: return the result set to JS
        return result;
    }

    return GetUndefinedValue(env);
}

napi_value GetBusinessError(napi_env env, int32_t errCode)
{
    napi_value result = nullptr;
    napi_value code = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_create_int32(env, errCode, &code));
    NAPI_CALL(env, napi_set_named_property(env, result, "code", code));

    napi_value message = nullptr;
    const char *errorMsg = GetErrorMessage(errCode);
    NAPI_CALL(env, napi_create_string_utf8(env, errorMsg, strlen(errorMsg), &message));
    NAPI_CALL(env, napi_set_named_property(env, result, "message", message));
    return result;
}

void ResolvePromise(napi_env env, AsyncContext *context)
{
    napi_value result = nullptr;
    if (context->result == ASSET_SUCCESS) {
        result = GetBusinessReturn(env, context);
        NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, context->deferred, result));
    } else {
        result = GetBusinessError(env, context->result);
        NAPI_CALL_RETURN_VOID(env, napi_reject_deferred(env, context->deferred, result));
    }
}

void ResolveCallback(napi_env env, AsyncContext *context)
{
    napi_value cbArgs[CALLBACK_ARGS_NUM] = { 0 };
    size_t index = 0;
    if (context->result == ASSET_SUCCESS) {
        cbArgs[index++] = GetUndefinedValue(env);
        cbArgs[index++] = GetBusinessReturn(env, context);
    } else {
        cbArgs[index++] = GetBusinessError(env, context->result);
    }

    napi_value callback = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, context->callback, &callback));
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, nullptr, callback, index, cbArgs, nullptr));
}

napi_value CreateAsyncWork(napi_env env, AsyncContext *context, const char *funcName,
    napi_async_execute_callback execute)
{
    napi_value result;
    if (context->callback == nullptr) {
        NAPI_CALL(env, napi_create_promise(env, &context->deferred, &result));
    } else {
        NAPI_CALL(env, napi_get_undefined(env, &result));
    }

    napi_value resource = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, funcName, NAPI_AUTO_LENGTH, &resource));
    NAPI_CALL(env, napi_create_async_work(
        env, nullptr, resource, execute,
        [](napi_env env, napi_status status, void *data) {
            AsyncContext *asyncContext = static_cast<AsyncContext *>(data);
            if (asyncContext->callback != nullptr) {
                ResolveCallback(env, asyncContext);
            } else {
                ResolvePromise(env, asyncContext);
            }
            DestroyAsyncContext(env, asyncContext);
        },
        static_cast<void *>(context), &context->work));
    NAPI_CALL(env, napi_queue_async_work(env, context->work));
    return result;
}

} // anonymous namespace

napi_value NapiEntry(napi_env env, napi_callback_info info, const char *funcName, napi_async_execute_callback execute,
    size_t expectArgNum)
{
    size_t argc = expectArgNum;
    napi_value argv[MAX_ARGS_NUM] = { 0 };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_THROW(env, argc < (expectArgNum - 1), ASSET_INVALID_ARGUMENT, "The number of arguments is insufficient.");

    AsyncContext *context = CreateAsyncContext();
    NAPI_THROW(env, context == nullptr, ASSET_OUT_OF_MEMRORY, "Unable to allocate memory for AsyncContext.");

    do {
        size_t index = 0;
        if (ParseMapParam(env, argv[index++], &context->attrs, &context->attrCnt) != napi_ok) {
            LOGE("Parse first map parameter failed.");
            break;
        }

        if (index < (argc - 1) &&
            ParseMapParam(env, argv[index++], &context->updateAttrs, &context->updateAttrCnt) != napi_ok) {
            LOGE("Parse second map parameter failed.");
            break;
        }

        if (index < argc && ParseCallbackParam(env, argv[index++], &context->callback) != napi_ok) {
            LOGE("Parse async callback parameter failed.");
            break;
        }

        napi_value promise = CreateAsyncWork(env, context, funcName, execute);
        if (promise == nullptr) {
            LOGE("Create async work failed.");
            break;
        }
        return promise;
    } while (0);
    DestroyAsyncContext(env, context);
    return nullptr;
}

} // Asset
} // Security
} // OHOS