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

#ifndef PSI_PIR_PROTOCOL_INTERNAL_TYPES_H
#define PSI_PIR_PROTOCOL_INTERNAL_TYPES_H

#include <cstdint>
#include <vector>

namespace psi_pir::internal {

using Slots = std::vector<std::uint64_t>;

constexpr std::size_t kBitsPerByte = 8;
constexpr std::size_t kPirGroupsPerItem = 2;
constexpr std::size_t kPsiGroupsPerItem = 1;
constexpr std::size_t kMaxBitsPerSlot = sizeof(std::uint64_t) * kBitsPerByte;


} // namespace psi_pir::internal

#endif // PSI_PIR_PROTOCOL_INTERNAL_TYPES_H
