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

#ifndef PSI_PIR_BGV_DECRYPTOR_H
#define PSI_PIR_BGV_DECRYPTOR_H

#include "he_types.h"

namespace psi_pir::internal {
class BgvDecryptor final {
public:
    int32_t DecryptToSlots(const Bytes &ciphertext, const ProtocolParams &params, const OperationIdentity &identity,
                           Bytes &output) const noexcept;
};

} // namespace psi_pir::internal

#endif // PSI_PIR_BGV_DECRYPTOR_H
