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

#ifndef PRIVACY_PROTOCOL_EVALUATION_H
#define PRIVACY_PROTOCOL_EVALUATION_H

#include "internal_types.h"
#include "privacy_protocol_types.h"

namespace OHOS::Security::PrivacyComputation::internal {
// Opaque per-batch handle. Owns the current ciphertext and its evaluation context.
// A failed operation invalidates the handle; Serialize then returns that error without modifying output.
class HeEvaluation {
public:
    virtual ~HeEvaluation() = default;
    HeEvaluation(const HeEvaluation &) = delete;
    HeEvaluation &operator=(const HeEvaluation &) = delete;
    // TA-compatible subtraction: plaintext - Dec(current), modulo the plaintext modulus.
    virtual int32_t AddPlain(const Slots &plaintext) noexcept = 0;
    virtual int32_t MultiplyPlain(const Slots &plaintext) noexcept = 0;
    virtual int32_t Rerandomize() noexcept = 0;
    virtual int32_t Serialize(Bytes &output) const noexcept = 0;

protected:
    HeEvaluation() = default;
};

} // namespace OHOS::Security::PrivacyComputation::internal

#endif // PRIVACY_PROTOCOL_EVALUATION_H
