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

namespace OHOS {
namespace Security {
namespace Asset {
    napi_status ParseJsParams(size_t argc, napi_value *argv, AsyncContext *context)
    {
        size_t index = 0;
        napi_valuetype valueType = napi_undefined;
        NAPI_CALL_BASE(env, napi_typeof(env, argv[index], &valueType), napi_invalid_arg);
        NAPI_THROW_BASE(env, valueType != napi_object, napi_object_expected,
            INVALID_ARGUMENT, "The first parameter must be of the Map type.");

        // todo: 如何传递map?

        index++;
        if (index < argc) {
            NAPI_CALL_BASE(env, napi_typeof(env, argv[index], &valueType), napi_invalid_arg);
            NAPI_THROW_BASE(env, valueType != napi_function, napi_function_expected,
                INVALID_ARGUMENT, "The second parameter must be of the AsyncCallback type.");
            napi_ref ref = nullptr;
            NAPI_CALL_BASE(env, napi_create_reference(env, argv[index], 1, &ref), napi_generic_failure);
            context->callback = ref;
        }
        return napi_ok;
    }
} // Asset
} // Security
} // OHOS