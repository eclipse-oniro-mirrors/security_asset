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

#ifndef PSI_PIR_PROTOCOL_IMPL_H
#define PSI_PIR_PROTOCOL_IMPL_H

#include "psi_pir_protocol.h"
#include "protocol_codec.h"
#include "protocol_encoder.h"
#include "protocol_params.h"

namespace psi_pir::internal {

class PsiPirProtocolImpl final {
public:
    int32_t privacySearch(const Bytes &privacyTarget, const std::vector<Element> &elements,
        const ProtocolParams &params, PrivacySearchResult &result) const noexcept;
    int32_t getSearchResult(const PrivacySearchResult &privacySearchResult,
        const ProtocolParams &params, SearchResult &result) const noexcept;

private:
    ProtocolCodec codec_;
    ProtocolEncoder encoder_;
};

} // namespace psi_pir::internal

#endif // PSI_PIR_PROTOCOL_IMPL_H
