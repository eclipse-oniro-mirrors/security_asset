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

#ifndef HPIC_PRIVACY_RESULT_CODEC_H
#define HPIC_PRIVACY_RESULT_CODEC_H

#include "privacy_limits.h"
#include "privacy_types.h"
#include "psi_pir_types.h"
#include <algorithm>
#include <utility>

namespace OHOS::Security::PrivacyComputation {
namespace result_codec {
constexpr uint32_t BITS_PER_BYTE = 8;
inline void WriteUint32(uint32_t value, uint8_t *&cursor)
{
    for (size_t i = 0; i < sizeof(value); ++i) {
        *cursor++ = static_cast<uint8_t>(value >> (i * BITS_PER_BYTE));
    }
}
inline bool ReadUint32(const uint8_t *&cursor, const uint8_t *end, uint32_t &value)
{
    if (static_cast<size_t>(end - cursor) < sizeof(value)) {
        return false;
    }
    value = 0;
    for (size_t i = 0; i < sizeof(value); ++i) {
        value |= static_cast<uint32_t>(*cursor++) << (i * BITS_PER_BYTE);
    }
    return true;
}
} // namespace result_codec

// Same-process FFI encoding: little-endian u32 matched, hasValues, then (if present)
// u32 count followed by u32 length + bytes for each value. No pointers cross this boundary.
// Validate all sizes before writing; on failure the caller's buffer and size remain unchanged.
inline bool EncodePrivacyResult(const psi_pir::SearchResult &input, PrivacyBuffer &output)
{
    size_t size = RESULT_FIXED_BYTES;
    if (input.attachedValues) {
        size += PRIVACY_LENGTH_BYTES;
        if (input.attachedValues->size() > MAX_PRIVACY_ITEMS) {
            return false;
        }
        for (const auto &value : *input.attachedValues) {
            if (size > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES ||
                value.size() > MAX_PRIVACY_BYTES - size - PRIVACY_LENGTH_BYTES) {
                return false;
            }
            size += PRIVACY_LENGTH_BYTES + value.size();
        }
    }
    if (output.data == nullptr || output.size > MAX_PRIVACY_BYTES || size > output.size) {
        return false;
    }
    auto *cursor = output.data;
    result_codec::WriteUint32(input.matchedResult ? 1U : 0U, cursor);
    result_codec::WriteUint32(input.attachedValues ? 1U : 0U, cursor);
    if (input.attachedValues) {
        result_codec::WriteUint32(static_cast<uint32_t>(input.attachedValues->size()), cursor);
        for (const auto &value : *input.attachedValues) {
            result_codec::WriteUint32(static_cast<uint32_t>(value.size()), cursor);
            cursor = std::copy(value.begin(), value.end(), cursor);
        }
    }
    output.size = static_cast<uint32_t>(size);
    return true;
}

// Commit only a fully validated result; reject truncated data, invalid flags and trailing bytes.
inline bool DecodePrivacyResult(const uint8_t *data, size_t size, psi_pir::SearchResult &output)
{
    if (data == nullptr || size < RESULT_FIXED_BYTES || size > MAX_PRIVACY_BYTES) {
        return false;
    }
    const auto *cursor = data;
    const auto *end = data + size;
    uint32_t matched = 0;
    uint32_t hasValues = 0;
    if (!result_codec::ReadUint32(cursor, end, matched) || !result_codec::ReadUint32(cursor, end, hasValues) ||
        matched > 1 || hasValues > 1) {
        return false;
    }
    psi_pir::SearchResult result;
    result.matchedResult = matched == 1;
    if (hasValues == 1) {
        uint32_t count = 0;
        if (!result_codec::ReadUint32(cursor, end, count) || count > MAX_PRIVACY_ITEMS ||
            count > static_cast<size_t>(end - cursor) / PRIVACY_LENGTH_BYTES) {
            return false;
        }
        result.attachedValues.emplace();
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t length = 0;
            if (!result_codec::ReadUint32(cursor, end, length) || length > static_cast<size_t>(end - cursor)) {
                return false;
            }
            result.attachedValues->emplace_back(cursor, cursor + length);
            cursor += length;
        }
    }
    if (cursor != end) {
        return false;
    }
    output = std::move(result);
    return true;
}
} // namespace OHOS::Security::PrivacyComputation
#endif // HPIC_PRIVACY_RESULT_CODEC_H
