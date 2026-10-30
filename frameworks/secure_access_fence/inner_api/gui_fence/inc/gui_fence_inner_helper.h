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

#ifndef GUI_FENCE_INNER_HELPER_H
#define GUI_FENCE_INNER_HELPER_H

#include <functional>
#include <string>

#include "iremote_broker.h"

#include "gui_cache_manager.h" // AppPairKey
#include "isecure_access_fence.h" // ISecureAccessFence
#include "saf_result_code.h"

namespace OHOS::Security::SAF {

/**
 * @brief Parse a deniedPair string "callerBundle:callerIdx:targetBundle:targetIdx" into AppPairKey.
 *
 * The string format matches what IPC 505 (QueryAllDeniedPairs) returns. Each
 * denied pair is colon-separated into four fields. Returns false if the format
 * is invalid (fewer than three colons, or index fields cannot be parsed as
 * integers via std::stoi).
 *
 * Note: this function does NOT validate semantic emptiness of the bundle names
 * (an empty callerBundle such as ":0:com.b:0" still parses successfully); it
 * only validates the colon-delimited structure.
 *
 * @param pair The serialized denied-pair string.
 * @param key Output AppPairKey on success.
 * @return true on success, false if the format is invalid.
 */
bool ParseDeniedPair(const std::string &pair, AppPairKey &key);

/**
 * @brief Normalize the return value of an IPC call into a client-facing error code.
 *
 * Maps dead-object IPC results (ERR_DEAD_OBJECT / ERR_REMOTE_DEAD) to
 * SAF_ERR_SERVICE_UNAVAILABLE; all other return values are passed through
 * unchanged. A null callable yields SAF_ERR_SERVICE_UNAVAILABLE.
 *
 * The proxy reference is accepted so callers can reassign it on retry in
 * variants that implement retry-on-stopping; this gui_fence variant does not
 * retry, so the proxy is only forwarded.
 *
 * @param proxy The IPC proxy used by the call (forwarded, not dereferenced).
 * @param call The IPC invocation to run.
 * @return Normalized error code.
 */
int32_t HandleIpcError(sptr<ISecureAccessFence> &proxy, std::function<int32_t()> call);

} // namespace OHOS::Security::SAF

#endif // GUI_FENCE_INNER_HELPER_H
