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

#ifndef HPIC_PRIVACY_WIRE_H
#define HPIC_PRIVACY_WIRE_H
#include "psi_pir_protocol.h"
#include <limits>
namespace OHOS::Security::PrivacyComputation {
constexpr uint32_t GEN_TARGET_TRANSACTION = 11;
constexpr uint32_t GET_RESULT_TRANSACTION = 12;
constexpr size_t MAX_PRIVACY_MESSAGE = 256 * 1024;
// Fixed-width little-endian fields; no native structs or caller identity on the wire.
class PrivacyWire final {
public:
    psi_pir::Bytes bytes;
    size_t position = 0;
    bool valid = true;
    void Put(uint32_t value)
    {
        if (!valid || bytes.size() > MAX_PRIVACY_MESSAGE - 4) { valid = false; return; }
        for (size_t i = 0; i < 4; ++i) { bytes.push_back(static_cast<uint8_t>(value >> (i * 8))); }
    }
    uint32_t Get()
    {
        if (!valid || position > bytes.size() || bytes.size() - position < 4) { valid = false; return 0; }
        uint32_t value = 0;
        for (size_t i = 0; i < 4; ++i) { value |= uint32_t(bytes[position++]) << (i * 8); }
        return value;
    }
    void PutBytes(const psi_pir::Bytes &value)
    {
        if (value.size() > MAX_PRIVACY_MESSAGE) { valid = false; return; }
        Put(static_cast<uint32_t>(value.size()));
        if (!valid || value.size() > MAX_PRIVACY_MESSAGE - bytes.size()) { valid = false; return; }
        bytes.insert(bytes.end(), value.begin(), value.end());
    }
    psi_pir::Bytes GetBytes()
    {
        const size_t length = Get();
        if (!valid || length > MAX_PRIVACY_MESSAGE || length > bytes.size() - position) {
            valid = false; return {};
        }
        psi_pir::Bytes value(bytes.begin() + position, bytes.begin() + position + length);
        position += length;
        return value;
    }
    void PutList(const std::vector<psi_pir::Bytes> &values)
    {
        if (values.size() > MAX_PRIVACY_MESSAGE / 4) { valid = false; return; }
        Put(static_cast<uint32_t>(values.size()));
        for (const auto &value : values) { PutBytes(value); }
    }
    std::vector<psi_pir::Bytes> GetList()
    {
        const auto count = Get();
        if (!valid || count > (bytes.size() - position) / 4) { valid = false; return {}; }
        std::vector<psi_pir::Bytes> values;
        values.reserve(count);
        for (uint32_t i = 0; i < count && valid; ++i) { values.push_back(GetBytes()); }
        return values;
    }
    bool Done() const { return valid && position == bytes.size() && bytes.size() <= MAX_PRIVACY_MESSAGE; }
};
int32_t GenPrivacyTargetRemote(const psi_pir::TargetInput &, const psi_pir::PrivacyProtocol &, psi_pir::Bytes &);
int32_t GetSearchResultRemote(const psi_pir::PrivacySearchResult &, const psi_pir::PrivacyProtocol &,
    psi_pir::SearchResult &);
}
#endif
