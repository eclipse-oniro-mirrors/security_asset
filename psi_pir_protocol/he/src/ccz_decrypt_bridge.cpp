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

#include "ccz_decrypt_bridge.h"
#include "psi_pir_error_code.h"

namespace psi_pir::internal {
int32_t CczDecryptToSlots(const Bytes &, const ProtocolParams &, const OperationIdentity &, Bytes &) noexcept
{
    return psi_pir::PSI_PIR_HE_ERROR;
}
} // namespace psi_pir::internal
