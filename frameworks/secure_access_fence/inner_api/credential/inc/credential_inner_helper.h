/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CREDENTIAL_INNER_HELPER_H
#define CREDENTIAL_INNER_HELPER_H

#include <cstddef>
#include <cstdint>

namespace OHOS::Security::SAF {

/**
 * @brief Normalize a raw IPC return value into a client-facing error code.
 *
 * Maps dead-object results (ERR_DEAD_OBJECT / ERR_REMOTE_DEAD) to
 * SAF_ERR_SERVICE_UNAVAILABLE; every other value, including
 * SAF_ERR_SERVICE_IS_STOPPING, is passed through unchanged.
 *
 * This helper is deliberately free of samgr/IPC runtime dependencies so it can
 * be unit tested without a running system ability. The retry-on-stopping
 * behaviour lives in SafCredential's HandleIpcError, not here.
 *
 * @param ret Raw return value from the IPC proxy call.
 * @return Normalized error code.
 */
int32_t NormalizeIpcResult(int32_t ret);

/**
 * @brief Check that the source and destination batch sizes match.
 *
 * UpdateModelCredential pairs source[i] with dest[i], so a length mismatch is
 * rejected before any IPC is issued.
 *
 * @param sourceSize Number of source credentials.
 * @param destSize Number of destination credentials.
 * @return true when the sizes are equal, false otherwise.
 */
bool IsBatchSizeMatched(size_t sourceSize, size_t destSize);

} // namespace OHOS::Security::SAF

#endif // CREDENTIAL_INNER_HELPER_H
