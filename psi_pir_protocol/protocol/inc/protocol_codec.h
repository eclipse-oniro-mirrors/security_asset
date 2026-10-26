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

#ifndef PSI_PIR_PROTOCOL_PROTOCOL_CODEC_H
#define PSI_PIR_PROTOCOL_PROTOCOL_CODEC_H

#include "internal_types.h"
#include "protocol_params.h"
#include "psi_pir_error_code.h"
#include "psi_pir_types.h"

namespace psi_pir::internal {

class ProtocolCodec final {
public:
    int32_t ValidateProtocol(const ProtocolParams &params) const noexcept;
    int32_t ValidatePrivacyTarget(const Bytes &privacyTarget) const noexcept;
    int32_t ValidatePlainSearchResult(const PrivacySearchResult &result, const ProtocolParams &params) const noexcept;
    // Requires validated protocol parameters; checks decoded slots and writes uint64 little-endian bytes.
    int32_t SerializePlaintext(const Slots &slots, const ProtocolParams &params, Bytes &output) const noexcept;
    int32_t ParsePlaintexts(const PrivacySearchResult &result, const ProtocolParams &params,
                            std::vector<Slots> &plaintexts) const noexcept;
};

} // namespace psi_pir::internal

#endif // PSI_PIR_PROTOCOL_PROTOCOL_CODEC_H
