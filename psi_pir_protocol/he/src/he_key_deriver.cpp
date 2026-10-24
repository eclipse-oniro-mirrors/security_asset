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
#include <array>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <mutex>
#include <utility>

namespace psi_pir::internal {
namespace {
std::mutex g_seedMutex;
std::shared_ptr<const IHeSeedDeriver> g_seedDeriver;
}

void SetHeSeedDeriver(std::shared_ptr<const IHeSeedDeriver> deriver) noexcept
{
    std::lock_guard<std::mutex> lock(g_seedMutex);
    g_seedDeriver = std::move(deriver);
}

HeSeed::~HeSeed()
{
    OPENSSL_cleanse(bytes.data(), bytes.size());
}

int32_t HeKeyDeriver::DeriveKey(const OperationIdentity &identity, HeSeed &seed) const noexcept
{
    std::array<uint8_t, 40> material{};
    for (size_t i = 0; i < 4; ++i) {
        material[i] = static_cast<uint8_t>(identity.uid >> ((3 - i) * 8));
        material[4 + i] = static_cast<uint8_t>(identity.userId >> ((3 - i) * 8));
    }
    for (size_t i = 0; i < 32; ++i) {
        material[8 + i] = static_cast<uint8_t>(i);
    }
    std::array<uint8_t, 32> digest{};
    unsigned int digestSize = 0;
    const auto digestRet =
        EVP_Digest(material.data(), material.size(), digest.data(), &digestSize, EVP_sha256(), nullptr);
    OPENSSL_cleanse(material.data(), material.size());
    if (digestRet != 1 || digestSize != digest.size()) {
        OPENSSL_cleanse(digest.data(), digest.size());
        return psi_pir::PSI_PIR_CRYPTO_ERROR;
    }
    std::shared_ptr<const IHeSeedDeriver> deriver;
    {
        std::lock_guard<std::mutex> lock(g_seedMutex);
        deriver = g_seedDeriver;
    }
    const auto ret = deriver ? deriver->Derive(digest, seed) : psi_pir::PSI_PIR_HE_ERROR;
    OPENSSL_cleanse(digest.data(), digest.size());
    return ret;
}
} // namespace psi_pir::internal
