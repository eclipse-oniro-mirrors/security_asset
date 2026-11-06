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

#ifndef PRIVACY_PROTOCOL_PROTOCOL_ENCODER_H
#define PRIVACY_PROTOCOL_PROTOCOL_ENCODER_H

#include "internal_types.h"
#include "protocol_params.h"
#include "privacy_error_code.h"
#include "privacy_protocol_types.h"

namespace PrivacyComputation::internal {

class ProtocolEncoder final {
public:
    int32_t ValidateHashAlgorithm(HashAlg algorithm) const noexcept;
    int32_t EncodeServerVector(const Element &element, const ProtocolParams &params,
                               Slots &serverVector) const noexcept;
    // Repeat the comparison group across every HE slot, including both PIR groups.
    int32_t EncodeTargetVector(const Element &element, const ProtocolParams &params,
                               Slots &targetVector) const noexcept;
    int32_t EncodeSeed(const Bytes &seed, const ProtocolParams &params, Slots &seedSlots) const noexcept;
    int32_t ExtractSeed(const Slots &plain, std::size_t offset, const ProtocolParams &params,
                        Bytes &seed) const noexcept;
    int32_t GenerateSeed(const ProtocolParams &params, Bytes &seed) const noexcept;
    int32_t GenerateMaskVector(const ProtocolParams &params, Slots &maskVector) const noexcept;
    int32_t IsMatch(const Slots &plain, std::size_t offset, const ProtocolParams &params, bool &matched) const noexcept;

private:
    int32_t Hash(HashAlg algorithm, const Bytes &input, Bytes &digest) const noexcept;
    int32_t BytesToSlots(const Bytes &input, const ProtocolParams &params, Slots &slots) const noexcept;
};

} // namespace PrivacyComputation::internal

#endif // PRIVACY_PROTOCOL_PROTOCOL_ENCODER_H
