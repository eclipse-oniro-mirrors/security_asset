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

#include "hifhe_api_mock.h"

using namespace PrivacyHeTest;

namespace {
bool TakeFault(HeOperation operation)
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
} // namespace

extern "C" int32_t MockCtrl(HIFHE_Ctx *ctx, int32_t command, HIFHE_HEText *text, void *value, uint32_t size)
{
    if (command == HIFHE_CTRL_BGV_RERANDOMIZE && TakeFault(HeOperation::RERANDOMIZE)) {
        return g_heFault.status;
    }
    return HIFHE_Ctrl(ctx, command, text, value, size);
}

extern "C" int32_t MockEncode(const HIFHE_Ctx *ctx, const uint64_t *values, uint32_t count, HIFHE_HEText **out)
{
    return MockHeCall<HeOperation::ENCODE, HIFHE_Encode>(ctx, values, count, out);
}

extern "C" int32_t MockEncrypt(HIFHE_Ctx *ctx, const HIFHE_HEText *plaintext, HIFHE_HEText **out)
{
    return MockHeCall<HeOperation::ENCRYPT, HIFHE_Encrypt>(ctx, plaintext, out);
}

extern "C" int32_t MockDecrypt(HIFHE_Ctx *ctx, const HIFHE_HEText *ciphertext, HIFHE_HEText **out)
{
    return MockHeCall<HeOperation::DECRYPT, HIFHE_Decrypt>(ctx, ciphertext, out);
}

extern "C" int32_t MockDeserialize(const HIFHE_Ctx *ctx, const uint8_t *data, uint32_t count, HIFHE_HEText **out)
{
    return MockHeCall<HeOperation::DESERIALIZE, HIFHE_Deserialize>(ctx, data, count, out);
}

extern "C" int32_t MockAdd(const HIFHE_Ctx *ctx, const HIFHE_HEText *left,
    const HIFHE_HEText *right, HIFHE_HEText **out)
{
    return MockHeCall<HeOperation::ADD, HIFHE_Add>(ctx, left, right, out);
}

extern "C" int32_t MockMul(const HIFHE_Ctx *ctx, const HIFHE_HEText *left,
    const HIFHE_HEText *right, HIFHE_HEText **out)
{
    return MockHeCall<HeOperation::MULTIPLY, HIFHE_Mul>(ctx, left, right, out);
}
