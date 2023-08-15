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
#include "asset_api.h"

#include "securec.h"

namespace AssetNapi {
namespace {
constexpr size_t ASSET_ARGS_NUM = 1;
}

const std::string TEST_STRTING = "test";

napi_value GetInt32(napi_env env, int32_t value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, value, &result));
    return result;
}

napi_value GetNull(napi_env env)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_null(env, &result));
    return result;
}

struct AssetPropertiesAsyncContextT {
    napi_async_work asyncWork = nullptr;
    napi_deferred deferred = nullptr;
    napi_ref callback = nullptr;
    int32_t code = 0;
    int32_t result = 0;
};
using AssetPropertiesAsyncContext = AssetPropertiesAsyncContextT *;

static AssetPropertiesAsyncContext CreateAssetPropertiesAsyncContext()
{
    AssetPropertiesAsyncContext context =
        static_cast<AssetPropertiesAsyncContext>(malloc(sizeof(AssetPropertiesAsyncContextT)));
    if (context != nullptr) {
        (void)memset_s(context, sizeof(AssetPropertiesAsyncContextT), 0, sizeof(AssetPropertiesAsyncContextT));
    }
    return context;
}

void DeleteCommonAsyncContext(napi_env env, napi_async_work &asyncWork, napi_ref &callback)
{
    if (asyncWork != nullptr) {
        napi_delete_async_work(env, asyncWork);
        asyncWork = nullptr;
    }

    if (callback != nullptr) {
        napi_delete_reference(env, callback);
        callback = nullptr;
    }
}

static void DeleteAssetPropertiesAsyncContext(napi_env env, AssetPropertiesAsyncContext &context)
{
    if (context == nullptr) {
        return;
    }
    DeleteCommonAsyncContext(env, context->asyncWork, context->callback);
    free(context);
    context = nullptr;
}

static napi_value ParseInputCode(napi_env env, napi_value *argv, size_t &index, AssetPropertiesAsyncContext context)
{
    if (argv == nullptr) {
        return nullptr;
    }
    napi_value object = argv[index];

    napi_valuetype valueType = napi_valuetype::napi_undefined;
    napi_status status = napi_typeof(env, object, &valueType);
    if (status != napi_ok) {
        return nullptr;
    }

    if (valueType != napi_valuetype::napi_number) {
        napi_throw_error(env, std::to_string(22).c_str(),
            "the type of handle isn't number");
        return nullptr;
    }

    uint32_t codeTmp = 0;
    status = napi_get_value_uint32(env, object, &codeTmp);
    if (status != napi_ok) {
        return nullptr;
    }
    context->code = codeTmp;

    return GetInt32(env, 0);
}

napi_ref GetCallback(napi_env env, napi_value object)
{
    napi_valuetype valueType = napi_undefined;
    napi_status status = napi_typeof(env, object, &valueType);
    if (status != napi_ok) {
        GET_AND_THROW_LAST_ERROR((env));
        return nullptr;
    }

    if (valueType != napi_function) {
        return nullptr;
    }

    napi_ref ref = nullptr;
    status = napi_create_reference(env, object, 1, &ref);
    if (status != napi_ok) {
        GET_AND_THROW_LAST_ERROR((env));
        return nullptr;
    }
    return ref;
}

static napi_value AssetTestParseParams(
    napi_env env, napi_callback_info info, AssetPropertiesAsyncContext context)
{
    size_t argc = 2;
    napi_value argv[1] = {0};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    if (argc < ASSET_ARGS_NUM) {
        napi_throw_error(env, std::to_string(11).c_str(), "no enough params input");
        return nullptr;
    }

    size_t index = 0;
    napi_value result = ParseInputCode(env, argv, index, context);
    if (result == nullptr) {
        return nullptr;
    }

    index++;
    context->callback = GetCallback(env, argv[index]);

    return GetInt32(env, 0);
}


static napi_value GenerateResult(napi_env env, int32_t resultCode)
{
    napi_value result = nullptr;

    if (napi_create_object(env, &result) != napi_ok) {
        return GetNull(env);
    }

    napi_value resultCodeJs = nullptr;
    if (napi_create_int32(env, resultCode, &resultCodeJs) != napi_ok) {
        // to do release result
        return GetNull(env);
    };
    if (napi_set_named_property(env, result, "res", resultCodeJs) != napi_ok) {
        // to do release result + resultCodeJs
        return GetNull(env);
    };
    return result;
}

static void CallbackResultSuccess(napi_env env, napi_ref callback, int32_t resultCode)
{
    napi_value params[2] = { GetNull(env), GenerateResult(env, resultCode) };
    napi_value func = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, callback, &func));

    napi_value recv = nullptr;
    napi_value result = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &recv));
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, recv, func, 2, params, &result));
}

void AssetReturnNapiResult(napi_env env, napi_ref callback, napi_deferred deferred, int32_t resultCode)
{
    // if (errorCode == HKS_SUCCESS) {
        CallbackResultSuccess(env, callback, resultCode);
    // } else {
    //     CallbackResultFailure(env, callback, resultCode);
    // }
}

static napi_value AssetTestAsyncWork(napi_env env, AssetPropertiesAsyncContext context)
{
    napi_value promise = nullptr;
    if (context->callback == nullptr) {
        NAPI_CALL(env, napi_create_promise(env, &context->deferred, &promise));
    }

    napi_value resourceName = nullptr;
    napi_create_string_latin1(env, "AssetTestAsyncWork", NAPI_AUTO_LENGTH, &resourceName);

    napi_create_async_work(
        env,
        nullptr,
        resourceName,
        [](napi_env env, void *data) {
            AssetPropertiesAsyncContext napiContext = static_cast<AssetPropertiesAsyncContext>(data);

            napiContext->result = AssetTest(napiContext->code);
        },
        [](napi_env env, napi_status status, void *data) {
            AssetPropertiesAsyncContext napiContext = static_cast<AssetPropertiesAsyncContext>(data);
            AssetReturnNapiResult(env, napiContext->callback, napiContext->deferred, napiContext->result);
            DeleteAssetPropertiesAsyncContext(env, napiContext);
        },
        static_cast<void *>(context),
        &context->asyncWork);

    napi_status status = napi_queue_async_work(env, context->asyncWork);
    if (status != napi_ok) {
        DeleteAssetPropertiesAsyncContext(env, context);
        return nullptr;
    }

    if (context->callback == nullptr) {
        return promise;
    } else {
        return GetNull(env);
    }
}

napi_value JsAssetTest(napi_env env, napi_callback_info info)
{
    AssetPropertiesAsyncContext context = CreateAssetPropertiesAsyncContext();
    if (context == nullptr) {
        return nullptr;
    }

    napi_value result = AssetTestParseParams(env, info, context);
    if (result == nullptr) {
        DeleteAssetPropertiesAsyncContext(env, context);
        return nullptr;
    }
    result = AssetTestAsyncWork(env, context);
    if (result == nullptr) {
        DeleteAssetPropertiesAsyncContext(env, context);
        return nullptr;
    }
    return result;
}
}
