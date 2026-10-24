/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef HPIC_NAPI_UTILS_H
#define HPIC_NAPI_UTILS_H

#include "napi/native_api.h"
#include "psi_pir_types.h"

namespace OHOS::Security::PrivacyComputation {
napi_status ParseJsArgs(napi_env env, napi_callback_info info, napi_value *args, size_t expectedCount);
napi_status ParseJsPrivacyTarget(napi_env env, napi_value value, psi_pir::Bytes &target);
napi_status ParseJsProtocol(napi_env env, napi_value value, psi_pir::PrivacyProtocol &protocol);
napi_status ParseJsTarget(napi_env env, napi_value value, psi_pir::TargetInput &target);
napi_status ParseJsElements(napi_env env, napi_value value, std::vector<psi_pir::Element> &elements);
napi_status ParseJsSearchResult(napi_env env, napi_value value, psi_pir::PrivacySearchResult &result);
napi_status CreateJsUint8Array(napi_env env, const psi_pir::Bytes &bytes, napi_value &output);
napi_status CreateJsPrivacySearchResult(napi_env env, const psi_pir::PrivacySearchResult &result, napi_value &output);
napi_status CreateJsSearchResult(napi_env env, const psi_pir::SearchResult &result, napi_value &output);

} // namespace OHOS::Security::PrivacyComputation

#endif // HPIC_NAPI_UTILS_H
