/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ASSET_NAPI_UPDATE_H
#define ASSET_NAPI_UPDATE_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_napi_common.h"

namespace OHOS {
namespace Security {
namespace Asset {

#define AS_USER_UPDATE_ARGS_NUM 3
#define UPDATE_ARGS_NUM 2

napi_value NapiUpdate(const napi_env env, napi_callback_info info);

napi_value NapiUpdateSync(const napi_env env, napi_callback_info info);

napi_value NapiUpdateAsUser(const napi_env env, napi_callback_info info);

} // Asset
} // Security
} // OHOS

#endif // ASSET_NAPI_UPDATE_H