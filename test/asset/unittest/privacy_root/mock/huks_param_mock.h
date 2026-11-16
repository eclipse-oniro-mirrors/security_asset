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

#ifndef ASSET_PRIVACY_HUKS_PARAM_MOCK_H
#define ASSET_PRIVACY_HUKS_PARAM_MOCK_H

#include <cstdint>

namespace PrivacyRootTest {
enum class ParamFailure { NONE, INIT, ADD, BUILD };

struct ParamFault {
    ParamFailure failure = ParamFailure::NONE;
    uint32_t paramSet = 1;
    uint32_t initCount = 0;
};

inline thread_local ParamFault g_paramFault;
} // namespace PrivacyRootTest
#endif
