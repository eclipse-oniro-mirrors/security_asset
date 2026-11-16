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

#ifndef ASSET_PRIVACY_HIFHE_API_MOCK_H
#define ASSET_PRIVACY_HIFHE_API_MOCK_H

#include "hifhe.h"
#include <cstdint>

namespace PrivacyHeTest {
enum class HeOperation { NONE, ENCODE, ENCRYPT, DECRYPT, DESERIALIZE, ADD, MULTIPLY, RERANDOMIZE };

struct HeFault {
    HeOperation operation = HeOperation::NONE;
    int32_t status = HIFHE_SUCCESS;
};

inline thread_local HeFault g_heFault;

inline bool TakeFault(HeOperation operation)
{
    if (g_heFault.operation != operation) {
        return false;
    }
    g_heFault.operation = HeOperation::NONE;
    return true;
}

// The adapter initializes output handles to null before invoking HiFHE.
template<HeOperation OPERATION, auto Backend, typename... Args>
int32_t MockHeCall(Args... args)
{
    if (TakeFault(OPERATION)) {
        return g_heFault.status;
    }
    return Backend(args...);
}

inline int32_t MockCtrl(HIFHE_Ctx *ctx, int32_t command, HIFHE_HEText *text, void *value, uint32_t size)
{
    if (command == HIFHE_CTRL_BGV_RERANDOMIZE && TakeFault(HeOperation::RERANDOMIZE)) {
        return g_heFault.status;
    }
    return HIFHE_Ctrl(ctx, command, text, value, size);
}
} // namespace PrivacyHeTest

// Only the unit test target redirects these calls.
#define HIFHE_Encode ::PrivacyHeTest::MockHeCall<::PrivacyHeTest::HeOperation::ENCODE, &::HIFHE_Encode>
#define HIFHE_Encrypt ::PrivacyHeTest::MockHeCall<::PrivacyHeTest::HeOperation::ENCRYPT, &::HIFHE_Encrypt>
#define HIFHE_Decrypt ::PrivacyHeTest::MockHeCall<::PrivacyHeTest::HeOperation::DECRYPT, &::HIFHE_Decrypt>
#define HIFHE_Deserialize ::PrivacyHeTest::MockHeCall<::PrivacyHeTest::HeOperation::DESERIALIZE, &::HIFHE_Deserialize>
#define HIFHE_Add ::PrivacyHeTest::MockHeCall<::PrivacyHeTest::HeOperation::ADD, &::HIFHE_Add>
#define HIFHE_Mul ::PrivacyHeTest::MockHeCall<::PrivacyHeTest::HeOperation::MULTIPLY, &::HIFHE_Mul>
#define HIFHE_Ctrl ::PrivacyHeTest::MockCtrl
#endif
