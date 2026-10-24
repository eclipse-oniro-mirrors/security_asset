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

#ifndef PSI_PIR_PROTOCOL_PROTOCOL_MAPPER_H
#define PSI_PIR_PROTOCOL_PROTOCOL_MAPPER_H

#include "protocol_params.h"
#include "psi_pir_error_code.h"
#include "psi_pir_types.h"

namespace psi_pir::internal {

int32_t MapPrivacyProtocol(const PrivacyProtocol &privacyProtocol, ProtocolParams &params) noexcept;

} // namespace psi_pir::internal

#endif // PSI_PIR_PROTOCOL_PROTOCOL_MAPPER_H
