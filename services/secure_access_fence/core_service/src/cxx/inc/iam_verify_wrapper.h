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

#ifndef IAM_VERIFY_WRAPPER_H
#define IAM_VERIFY_WRAPPER_H

// This header is included by the CXX bridge (wrapper.rs.h) after rust/cxx.h.
// It declares the IAM auth token wrappers so the CXX bridge can find them.
// IamVerifyResult, IamRedeemResult and rust::Vec are defined by the CXX bridge runtime.

#include <cstdint>

namespace OHOS {
namespace Security {
namespace SAF {

struct IamVerifyResult;
struct IamRedeemResult;

IamVerifyResult VerifyAuthToken(const rust::Vec<uint8_t> &authToken, uint64_t allowableDurationMs);
IamRedeemResult RedeemScheduledAuthToken(
    const rust::Vec<uint8_t> &challenge, const rust::Vec<uint8_t> &scheduleToken);

} // namespace SAF
} // namespace Security
} // namespace OHOS

#endif // IAM_VERIFY_WRAPPER_H
