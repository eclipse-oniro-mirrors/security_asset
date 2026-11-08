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

#include "gui_fence_inner_helper.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <tuple>

#include "errors.h"
#include "saf_log.h"

namespace OHOS::Security::SAF {

// ERR_REMOTE_DEAD mirrors the value used by the OpenHarmony IPC layer for a
// dead remote object. Kept as a literal here so the helper is self-contained.
namespace {
constexpr int32_t ERR_REMOTE_DEAD = 29189;
} // namespace

// Parse a substring of 's' (delimited by [first, last)) as a base-10 int32_t.
// Returns true and stores the value on success; returns false on any of:
//   - empty field (first == last)
//   - leading/internal/trailing non-digit characters (only an optional leading
//     sign and digits are accepted, matching std::stoi's grammar)
//   - value out of int32_t range (strtol reports ERANGE)
// This is exception-free (no std::stoi) to comply with the OpenHarmony C++
// coding rule that forbids try/catch, and to safely handle untrusted IPC 505
// payloads without crashing the caller process.
static bool ParseInt32(const std::string &s, size_t first, size_t last, int32_t &out)
{
    if (first >= last || last > s.size()) {
        return false;
    }
    // Build a NUL-terminated buffer for strtol. The field length is bounded by
    // the input string length, which is itself bounded by the IPC parcel size.
    std::string field = s.substr(first, last - first);
    if (field.empty()) {
        return false;
    }
    char *endPtr = nullptr;
    errno = 0;
    long value = std::strtol(field.c_str(), &endPtr, 10);
    if (errno == ERANGE || endPtr == field.c_str() || *endPtr != '\0') {
        return false;
    }
    if (value < INT32_MIN || value > INT32_MAX) {
        return false;
    }
    out = static_cast<int32_t>(value);
    return true;
}

// Find the next colon at or after 'from' and parse the substring [from, colon)
// as an int32_t. On success returns the colon position and stores the value.
// Returns std::string::npos when no colon follows 'from' or the field is not a
// valid int32. Exception-free (strtol-based) to comply with the no-try/catch rule.
static size_t ParseFieldUntilColon(const std::string &s, size_t from, int32_t &out)
{
    size_t colon = s.find(':', from);
    if (colon == std::string::npos) {
        return std::string::npos;
    }
    if (!ParseInt32(s, from, colon, out)) {
        return std::string::npos;
    }
    return colon;
}

bool ParseDeniedPair(const std::string &pair, AppPairKey &key)
{
    size_t pos1 = pair.find(':');
    if (pos1 == std::string::npos) {
        return false;
    }
    std::string callerBundle = pair.substr(0, pos1);
    int32_t callerIdx = 0;
    size_t pos2 = ParseFieldUntilColon(pair, pos1 + 1, callerIdx);
    if (pos2 == std::string::npos) {
        return false;
    }
    size_t pos3 = pair.find(':', pos2 + 1);
    if (pos3 == std::string::npos) {
        return false;
    }
    std::string targetBundle = pair.substr(pos2 + 1, pos3 - pos2 - 1);
    int32_t targetIdx = 0;
    if (!ParseInt32(pair, pos3 + 1, pair.size(), targetIdx)) {
        return false;
    }
    key = std::make_tuple(callerBundle, callerIdx, targetBundle, targetIdx);
    return true;
}

int32_t HandleIpcError(sptr<ISecureAccessFence> &proxy, std::function<int32_t()> call)
{
    if (call == nullptr) {
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }
    int32_t ret = call();
    if (ret == ERR_DEAD_OBJECT || ret == ERR_REMOTE_DEAD) {
        LOGW("service unavailable and not retry.");
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }
    return ret;
}

} // namespace OHOS::Security::SAF
