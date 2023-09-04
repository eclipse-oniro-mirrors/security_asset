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

namespace OHOS {
namespace Security {
namespace Asset {
namespace {
#define ASSET_MAX_BUFFER_LEN 1024
napi_status ParseByteArray(napi_env env, napi_value value, AssetBlob &blob)
{
    napi_typedarray_type arrayType;
    size_t length = 0;
    void *rawData = nullptr;

    NAPI_CALL_RETURN(env, napi_get_typedarray_info(env, value, &arrayType, &length, &rawData, nullptr, nullptr));
    NAPI_THROW_RETURN(env, arrayType != napi_uint8_array, ASSET_INVALID_ARGUMENT, "Expect type napi_uint8_array.");
    NAPI_THROW_RETURN(env, length == 0 || length > ASSET_MAX_BUFFER_LEN,
        ASSET_INVALID_ARGUMENT, "Invalid array length.");

    blob.data = new(std::nothrow) uint8_t[length];
    NAPI_THROW_RETURN(env, blob.data == nullptr, ASSET_OUT_OF_MEMRORY, "Out of memory.");
    (void)memcpy_s(blob.data, length, rawData, length);
    blob.size = static_cast<uint32_t>(length);
    return napi_ok;
}

napi_status ParseAssetParam(napi_env env, napi_value tag, napi_value value, AssetParam &param)
{
    // parse tag
    napi_valuetype type = napi_undefined;
    NAPI_CALL_RETURN(env, napi_typeof(env, tag, &type));
    NAPI_THROW_RETURN(env, type != napi_number, ASSET_INVALID_ARGUMENT, "The tag type of map should be number.");
    NAPI_CALL_RETURN(env, napi_get_value_uint32(env, tag, &param.tag));

    // parse value
    NAPI_CALL_RETURN(env, napi_typeof(env, value, &type));
    switch (param.tag & ASSET_TAG_TYPE_MASK) {
        case ASSET_TYPE_INT32:
            NAPI_THROW_RETURN(env, type != napi_number, ASSET_INVALID_ARGUMENT, "Expect type napi_number.");
            NAPI_CALL_RETURN(env, napi_get_value_int32(env, value, &param.value.i32));
        case ASSET_TYPE_UINT32:
            NAPI_THROW_RETURN(env, type != napi_number, ASSET_INVALID_ARGUMENT, "Expect type napi_number.");
            NAPI_CALL_RETURN(env, napi_get_value_uint32(env, value, &param.value.u32));
        case ASSET_TYPE_INT64:
            NAPI_THROW_RETURN(env, type != napi_number, ASSET_INVALID_ARGUMENT, "Expect type napi_number.");
            NAPI_CALL_RETURN(env, napi_get_value_int64(env, value, &param.value.i64));
        case ASSET_TYPE_UINT64:
            NAPI_THROW_RETURN(env, type != napi_number, ASSET_INVALID_ARGUMENT, "Expect type napi_number.");
            NAPI_CALL_RETURN(env, napi_get_value_int64(env, value, reinterpret_cast<int64_t *>(&param.value.u64)));
            break;
        case ASSET_TYPE_BOOL:
            NAPI_THROW_RETURN(env, type != napi_boolean, ASSET_INVALID_ARGUMENT, "Expect type napi_boolean.");
            NAPI_CALL_RETURN(env, napi_get_value_bool(env, value, &param.value.boolean));
            break;
        case ASSET_TYPE_BYTES:
            NAPI_THROW_RETURN(env, type != napi_object, ASSET_INVALID_ARGUMENT, "Expect type napi_object.");
            NAPI_CALL_RETURN(env, ParseByteArray(env, value, param.value.blob));
            break;
        default:
            NAPI_THROW_RETURN(env, true, ASSET_INVALID_ARGUMENT, "Invalid tag argument.");
    }
    return napi_ok;
}

void FreeParam(std::vector<AssetParam> &params)
{
    for (auto param : params) {
        if ((param.tag & ASSET_TAG_TYPE_MASK) == ASSET_TYPE_BYTES) {
            delete [] param.value.blob.data;
        }
    }
}

} // anonymous namespace

AsyncContext *CreateAsyncContext()
{
    AsyncContext *context = new(std::nothrow) AsyncContext;
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

    if (context->asyncWork != nullptr) {
        napi_delete_async_work(env, context->asyncWork);
        context->asyncWork = nullptr;
    }

    if (context->callback != nullptr) {
        napi_delete_reference(env, context->callback);
        context->callback = nullptr;
    }

    if (context->params != nullptr) {
        for (int i = 0; i < context->paramCnt; i++) {
            if ((context->params[i].tag & ASSET_TAG_TYPE_MASK) == ASSET_TYPE_BYTES) {
                delete [] context->params[i].value.blob.data;
            }
        }
        delete [] context->params;
    }
    delete context;
}

napi_status ParseMapParam(napi_env env, napi_value arg, AssetParam **params, uint32_t *paramCnt)
{
    // check value type
    bool isMap = false;
    NAPI_CALL_RETURN(env, napi_is_map(env, arg, &isMap));
    NAPI_THROW_RETURN(env, !isMap, ASSET_INVALID_ARGUMENT, "Argument type mismatch.");

    // parse map object
    std::vector<AssetParam> paramVec;
    napi_value iterator;
    NAPI_CALL_RETURN(env, napi_get_named_property(env, arg, "entries", &iterator));
    napi_value result;
    napi_status callRet = napi_ok;
    while ((callRet = napi_call_function(env, arg, iterator, 0, nullptr, &result)) == napi_ok) { // todo: 感觉有问题，是否能成功进行迭代？
        napi_value key;
        napi_value value;
        NAPI_CALL_BREAK(env, napi_get_element(env, result, 0, &key)); // 0: the first element of map
        NAPI_CALL_BREAK(env, napi_get_element(env, result, 1, &value)); // 0: the second element of map

        AssetParam param;
        NAPI_CALL_BREAK(env, ParseAssetParam(env, key, value, param));
        paramVec.push_back(param);
    }

    if (callRet != napi_ok || paramVec.size() == 0) {
        FreeParam(paramVec);
        return napi_generic_failure;
    }

    // transfer vector to array
    *params = new(std::nothrow) AssetParam[paramVec.size()];
    if (*params == nullptr) {
        FreeParam(paramVec);
        NAPI_THROW_RETURN(env, true, ASSET_OUT_OF_MEMRORY, "Out of memory.");
    }
    (void)memcpy_s(*params, paramVec.size() * sizeof(AssetParam), &paramVec[0], paramVec.size() * sizeof(AssetParam));
    return napi_ok;
}

napi_status ParseCallbackParam(napi_env env, napi_value arg, napi_ref *callback)
{
    // check value type
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL_RETURN(env, napi_typeof(env, arg, &valueType));
    NAPI_THROW_RETURN(env, valueType != napi_function, ASSET_INVALID_ARGUMENT, "Expect AsyncCallback type.");

    // create callback reference
    NAPI_CALL_RETURN(env, napi_create_reference(env, arg, 1, callback));
    return napi_ok;
}
} // Asset
} // Security
} // OHOS