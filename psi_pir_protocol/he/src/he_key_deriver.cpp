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

#include "he_key_deriver.h"
#include "psi_pir_error_code.h"
#include "psi_pir_log.h"
#include <array>
#include <mutex>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <utility>

namespace psi_pir::internal {
namespace {
constexpr size_t IDENTITY_FIELD_BYTES = sizeof(uint32_t);
constexpr size_t USER_ID_OFFSET = IDENTITY_FIELD_BYTES;
constexpr size_t FIXED_SALT_BYTES = 32;
constexpr size_t FIXED_SALT_OFFSET = USER_ID_OFFSET + IDENTITY_FIELD_BYTES;
constexpr size_t IDENTITY_MATERIAL_BYTES = FIXED_SALT_OFFSET + FIXED_SALT_BYTES;
std::mutex g_seedMutex;
std::shared_ptr<const IHeSeedDeriver> g_seedDeriver;
} // namespace

void SetHeSeedDeriver(std::shared_ptr<const IHeSeedDeriver> deriver) noexcept
{
    std::lock_guard<std::mutex> lock(g_seedMutex);
    g_seedDeriver = std::move(deriver);
}

HeSeed::~HeSeed()
{
    OPENSSL_cleanse(bytes.data(), bytes.size());
}

int32_t HeKeyDeriver::DeriveKey(const CallerIdentity &identity, HeSeed &seed) const noexcept
{
    PSI_PIR_LOGI("HPIC seed identity derivation begin");
    std::array<uint8_t, IDENTITY_MATERIAL_BYTES> material{};
    for (size_t i = 0; i < IDENTITY_FIELD_BYTES; ++i) {
        material[i] = static_cast<uint8_t>(identity.uid >> ((IDENTITY_FIELD_BYTES - 1 - i) * kBitsPerByte));
        material[USER_ID_OFFSET + i] =
            static_cast<uint8_t>(identity.userId >> ((IDENTITY_FIELD_BYTES - 1 - i) * kBitsPerByte));
    }
    for (size_t i = 0; i < FIXED_SALT_BYTES; ++i) {
        material[FIXED_SALT_OFFSET + i] = static_cast<uint8_t>(i);
    }
    std::array<uint8_t, kIdentityHashSize> digest{};
    unsigned int digestSize = 0;
    const auto digestRet =
        EVP_Digest(material.data(), material.size(), digest.data(), &digestSize, EVP_sha256(), nullptr);
    OPENSSL_cleanse(material.data(), material.size());
    if (digestRet != 1 || digestSize != digest.size()) {
        OPENSSL_cleanse(digest.data(), digest.size());
        PSI_PIR_LOGE("HPIC seed identity digest failed");
        return psi_pir::PSI_PIR_CRYPTO_ERROR;
    }
    std::shared_ptr<const IHeSeedDeriver> deriver;
    {
        std::lock_guard<std::mutex> lock(g_seedMutex);
        deriver = g_seedDeriver;
    }
    const auto ret = deriver ? deriver->Derive(identity, digest, seed) : psi_pir::PSI_PIR_HE_ERROR;
    OPENSSL_cleanse(digest.data(), digest.size());
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("HPIC seed derivation failed, providerPresent=%{public}d, ret=%{public}d", deriver ? 1 : 0, ret);
    } else {
        PSI_PIR_LOGI("HPIC seed derivation success, seedBytes=%{public}zu", seed.bytes.size());
    }
    return ret;
}
} // namespace psi_pir::internal
