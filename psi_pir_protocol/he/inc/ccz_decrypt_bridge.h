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

#ifndef PSI_PIR_CCZ_DECRYPT_BRIDGE_H
#define PSI_PIR_CCZ_DECRYPT_BRIDGE_H

#include "he_types.h"

namespace psi_pir::internal {
// Closed source performs HE Decrypt/Decode only. On success, output contains one batch of
// 2048 uint64 slots serialized in little-endian order (16384 bytes), matching BgvDecryptor.
// Match detection, index recovery and PIR attached-value processing belong to the open-source
// protocol layer. Never substitute the closed-source complete GetSearchResult entry point.
// The raw HE-only export and ABI are pending alignment with the closed-source owner.
int32_t CczDecryptToSlots(const Bytes &ciphertext, const ProtocolParams &params, const OperationIdentity &identity,
                          Bytes &output) noexcept;

} // namespace psi_pir::internal

#endif // PSI_PIR_CCZ_DECRYPT_BRIDGE_H
