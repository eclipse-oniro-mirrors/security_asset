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

// Deterministic arithmetic boundary double. This does not validate real HE wire/cryptographic interoperability.
#include "host_faults.h"
#include "hifhe.h"
#include "crypt_eal_init.h"
#include "crypt_errno.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <vector>

struct HIFHE_Ctx {
    std::array<uint8_t, 32> seed{};
    bool keyed = false;
};
struct HIFHE_HEText {
    std::array<uint8_t, 32> seed{};
    std::vector<uint64_t> slots;
    bool encrypted = false;
    bool negate = false;
};
namespace PrivacyHost {
Faults &State()
{
    static Faults value;
    return value;
}
void Reset()
{
    State() = Faults{};
}
bool Fail(const char *operation)
{
    auto &value = State();
    const auto count = ++value.calls[operation];
    return value.failAt == operation && count == value.failOccurrence;
}
} // namespace PrivacyHost
namespace {
constexpr uint64_t MODULUS = 65537;
constexpr uint32_t HEADER_BYTES = 40;
constexpr uint32_t SLOT_COUNT = 2048;
constexpr uint32_t WIRE_BYTES = HEADER_BYTES + SLOT_COUNT * sizeof(uint64_t);
int32_t Export(HIFHE_HEText **out, const char *name, const HIFHE_HEText &value)
{
    if (out == nullptr || *out != nullptr || PrivacyHost::Fail(name)) {
        return HIFHE_PARAM_INVALID;
    }
    if (PrivacyHost::State().nullResult != name) {
        *out = new HIFHE_HEText(value);
    }
    return HIFHE_SUCCESS;
}
} // namespace
extern "C" int32_t CRYPT_EAL_Init(uint64_t)
{
    const char *mode = std::getenv("PRIVACY_TEST_DRBG");
    if (mode != nullptr && std::strcmp(mode, "failure") == 0) {
        return -1;
    }
    return mode != nullptr && std::strcmp(mode, "success") == 0 ? CRYPT_SUCCESS : CRYPT_EAL_ERR_DRBG_REPEAT_INIT;
}
extern "C" HIFHE_Ctx *HIFHE_New(int32_t)
{
    return PrivacyHost::Fail("New") ? nullptr : new HIFHE_Ctx;
}
extern "C" void HIFHE_Free(HIFHE_Ctx *ctx)
{
    delete ctx;
}
extern "C" void HIFHE_HETextFree(HIFHE_HEText *text)
{
    delete text;
}
extern "C" int32_t HIFHE_SetParaById(HIFHE_Ctx *, int32_t)
{
    return PrivacyHost::Fail("SetPara") ? HIFHE_PARAM_INVALID : HIFHE_SUCCESS;
}
extern "C" int32_t HIFHE_KeyGen(HIFHE_Ctx *ctx)
{
    if (PrivacyHost::Fail("KeyGen")) {
        return HIFHE_PARAM_INVALID;
    }
    ctx->keyed = true;
    return HIFHE_SUCCESS;
}
extern "C" int32_t HIFHE_Ctrl(HIFHE_Ctx *ctx, int32_t cmd, HIFHE_HEText *text, void *value, uint32_t)
{
    if (PrivacyHost::Fail("Ctrl")) {
        return HIFHE_PARAM_INVALID;
    }
    if (cmd == HIFHE_CTRL_BGV_GET_POLY_DEGREE) {
        *static_cast<uint32_t *>(value) = PrivacyHost::State().degree;
    }
    if (cmd == HIFHE_CTRL_BGV_SET_SEED) {
        std::memcpy(ctx->seed.data(), value, ctx->seed.size());
    }
    if (cmd == HIFHE_CTRL_BGV_SET_NEGATE) {
        if (text == nullptr || PrivacyHost::Fail("Negate")) {
            return HIFHE_PARAM_INVALID;
        }
        text->negate = true;
    }
    if (cmd == HIFHE_CTRL_BGV_RERANDOMIZE) {
        if (text == nullptr) {
            return HIFHE_PARAM_INVALID;
        }
        return Export(static_cast<HIFHE_HEText **>(value), "Rerandomize", *text);
    }
    return HIFHE_SUCCESS;
}
extern "C" int32_t HIFHE_Encode(const HIFHE_Ctx *, const uint64_t *values, uint32_t count, HIFHE_HEText **out)
{
    HIFHE_HEText value;
    value.slots.assign(values, values + count);
    return Export(out, "Encode", value);
}
extern "C" int32_t HIFHE_Encrypt(HIFHE_Ctx *ctx, const HIFHE_HEText *plain, HIFHE_HEText **out)
{
    HIFHE_HEText value = *plain;
    value.seed = ctx->seed;
    value.encrypted = true;
    return Export(out, "Encrypt", value);
}
extern "C" int32_t HIFHE_Decrypt(HIFHE_Ctx *ctx, const HIFHE_HEText *cipher, HIFHE_HEText **out)
{
    if (!ctx->keyed || cipher == nullptr || cipher->seed != ctx->seed) {
        return HIFHE_PARAM_INVALID;
    }
    HIFHE_HEText value = *cipher;
    value.encrypted = false;
    return Export(out, "Decrypt", value);
}
extern "C" int32_t HIFHE_Add(const HIFHE_Ctx *, const HIFHE_HEText *cipher, const HIFHE_HEText *plain,
                             HIFHE_HEText **out)
{
    HIFHE_HEText value = *cipher;
    for (size_t i = 0; i < value.slots.size(); ++i) {
        const uint64_t left = cipher->negate ? MODULUS - cipher->slots[i] : cipher->slots[i];
        value.slots[i] = (plain->slots[i] + left) % MODULUS;
    }
    value.negate = false;
    return Export(out, "Add", value);
}
extern "C" int32_t HIFHE_Mul(const HIFHE_Ctx *, const HIFHE_HEText *cipher, const HIFHE_HEText *plain,
                             HIFHE_HEText **out)
{
    HIFHE_HEText value = *cipher;
    for (size_t i = 0; i < value.slots.size(); ++i) {
        value.slots[i] = cipher->slots[i] * plain->slots[i] % MODULUS;
    }
    return Export(out, "Mul", value);
}
extern "C" int32_t HIFHE_Decode(const HIFHE_Ctx *, const HIFHE_HEText *plain, uint64_t *values, uint32_t *count)
{
    if (plain == nullptr || PrivacyHost::Fail("Decode")) {
        return HIFHE_PARAM_INVALID;
    }
    std::memcpy(values, plain->slots.data(), plain->slots.size() * sizeof(uint64_t));
    *count = PrivacyHost::State().decodeCount;
    if (PrivacyHost::State().overrideDecodeFirst) {
        values[0] = PrivacyHost::State().decodeFirst;
    }
    return HIFHE_SUCCESS;
}
extern "C" int32_t HIFHE_Serialize(const HIFHE_Ctx *, const HIFHE_HEText *text, uint8_t *out, uint32_t *size)
{
    if (text == nullptr) {
        return HIFHE_PARAM_INVALID;
    }
    if (out == nullptr) {
        if (PrivacyHost::Fail("SerializeQuery")) {
            return HIFHE_PARAM_INVALID;
        }
        *size = PrivacyHost::State().serializeQueryBytes == 0 ? WIRE_BYTES : PrivacyHost::State().serializeQueryBytes;
        return HIFHE_BUFFER_TOO_SMALL;
    }
    if (PrivacyHost::Fail("Serialize")) {
        return HIFHE_PARAM_INVALID;
    }
    if (*size < WIRE_BYTES) {
        return HIFHE_BUFFER_TOO_SMALL;
    }
    std::memset(out, 0, WIRE_BYTES);
    out[0] = 0xA5;
    out[1] = text->encrypted ? 1 : 0;
    std::memcpy(out + 8, text->seed.data(), text->seed.size());
    std::memcpy(out + HEADER_BYTES, text->slots.data(), SLOT_COUNT * sizeof(uint64_t));
    *size = PrivacyHost::State().serializeResultBytes == 0 ? WIRE_BYTES : PrivacyHost::State().serializeResultBytes;
    return HIFHE_SUCCESS;
}
extern "C" int32_t HIFHE_Deserialize(const HIFHE_Ctx *ctx, const uint8_t *bytes, uint32_t size, HIFHE_HEText **out)
{
    if (size != WIRE_BYTES || bytes[0] != 0xA5) {
        return HIFHE_PARAM_INVALID;
    }
    HIFHE_HEText value;
    value.encrypted = bytes[1] == 1;
    std::memcpy(value.seed.data(), bytes + 8, value.seed.size());
    if (ctx->keyed && value.seed != ctx->seed) {
        return HIFHE_PARAM_INVALID;
    }
    value.slots.resize(SLOT_COUNT);
    std::memcpy(value.slots.data(), bytes + HEADER_BYTES, SLOT_COUNT * sizeof(uint64_t));
    return Export(out, "Deserialize", value);
}
