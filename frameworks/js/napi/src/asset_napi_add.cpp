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

#include "asset_napi_add.h"

#include <new>

#include "asset_napi_common.h"
#include "asset_type.h"

namespace OHOS {
namespace Security {
namespace Asset {
    napi_value NapiAdd(napi_env env, napi_callback_info info)
    {
        size_t argc = DEFAULT_MAX_ARGS_NUM;
        napi_value argv[DEFAULT_MAX_ARGS_NUM] = { 0 };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW(env, argc >= DEFAULT_MIN_ARGS_NUM, INVALID_ARGUMENT, "The number of parameters must be 1 or 2.");

        AsyncContext *context = new(std::nothrow) AsyncContext;
        NAPI_THROW(env, context != nullptr, OUT_OF_MEMRORY, "Out of memory");

        if (ParseJsParams(env, argc, argv, context) != napi_ok) {
            delete context;
            return nullptr;
        }

        delete context;
        return nullptr;
    }
} // Asset
} // Security
} // OHOS