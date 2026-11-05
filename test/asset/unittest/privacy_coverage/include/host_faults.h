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

#ifndef PRIVACY_COVERAGE_HOST_FAULTS_H
#define PRIVACY_COVERAGE_HOST_FAULTS_H
#include <cstdint>
#include <string>
#include <unordered_map>
namespace PrivacyHost {
struct Faults {
    std::string failAt;
    uint32_t failOccurrence = 1;
    std::unordered_map<std::string, uint32_t> calls;
    std::string nullResult;
    uint32_t degree = 2048;
    uint32_t serializeQueryBytes = 0;
    uint32_t serializeResultBytes = 0;
    uint32_t decodeCount = 2048;
    uint64_t decodeFirst = 0;
    bool overrideDecodeFirst = false;
    int32_t accountStatus = 0;
    int32_t userId = 100;
    int32_t keyExistsStatus = 0;
    int32_t generateStatus = 0;
    int32_t huksInitStatus = 0;
    int32_t huksUpdateStatus = 0;
    int32_t huksFinishStatus = 0;
    uint32_t seedBytes = 32;
    bool dlopenFailure = false;
    bool symbolFailure = false;
    int32_t closedStatus = 0;
};
Faults &State();
void Reset();
bool Fail(const char *operation);
} // namespace PrivacyHost
#endif
