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

#ifndef PSI_PIR_PROTOCOL_ERROR_CODE_H
#define PSI_PIR_PROTOCOL_ERROR_CODE_H

#include <stdint.h>

namespace psi_pir {

constexpr int32_t PSI_PIR_SUCCESS = 0;
constexpr int32_t PSI_PIR_INVALID_ARGUMENT = 1;
constexpr int32_t PSI_PIR_UNSUPPORTED_PROTOCOL = 2;
constexpr int32_t PSI_PIR_INCOMPATIBLE_HE_PARAMETERS = 3;
constexpr int32_t PSI_PIR_CODEC_ERROR = 4;
constexpr int32_t PSI_PIR_HASH_ERROR = 5;
constexpr int32_t PSI_PIR_RANDOM_ERROR = 6;
constexpr int32_t PSI_PIR_HE_ERROR = 7;
constexpr int32_t PSI_PIR_CRYPTO_ERROR = 8;
constexpr int32_t PSI_PIR_INTERNAL_ERROR = 9;

} // namespace psi_pir

#endif // PSI_PIR_PROTOCOL_ERROR_CODE_H
