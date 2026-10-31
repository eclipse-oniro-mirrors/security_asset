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

#ifndef PSI_PIR_PROTOCOL_INTERNAL_CRYPTO_PROVIDER_H
#define PSI_PIR_PROTOCOL_INTERNAL_CRYPTO_PROVIDER_H

#include "internal_types.h"
#include "protocol_params.h"
#include "psi_pir_types.h"

#include <stdint.h>

namespace psi_pir::internal {

// Internal crypto extension points. They are deliberately kept outside the
// installed northbound headers: the protocol implementation selects an
// implementation, while business callers only use the three protocol APIs.
class IHashProvider {
public:
    virtual ~IHashProvider() = default;
    virtual int32_t Digest(HashAlg algorithm, const Bytes &input, Bytes &output) const noexcept = 0;
};

class IRandomSource {
public:
    virtual ~IRandomSource() = default;
    virtual int32_t Fill(Bytes &output) noexcept = 0;
};

class IHEEngine {
public:
    virtual ~IHEEngine() = default;
    virtual bool IsCompatible(const ProtocolParams &params) const noexcept = 0;
    // TA-compatible operation: plaintext - Dec(ciphertext), modulo the plaintext modulus.
    virtual int32_t AddPlain(const Bytes &ciphertext, const Slots &plaintext, Bytes &output) const noexcept = 0;
    virtual int32_t MultiplyPlain(const Bytes &ciphertext, const Slots &plaintext, Bytes &output) const noexcept = 0;
    virtual int32_t RandCipherText(const Bytes &ciphertext, Bytes &output) const noexcept = 0;
};

class IPirValueCrypto {
public:
    virtual ~IPirValueCrypto() = default;
    virtual int32_t encrypt(const Bytes &seed, const std::optional<Bytes> &value, const ProtocolParams &params,
                            Bytes &output) const noexcept = 0;
    virtual int32_t decrypt(const Bytes &seed, const Bytes &ciphertext, const ProtocolParams &params,
                            std::optional<Bytes> &output) const noexcept = 0;
};

} // namespace psi_pir::internal

#endif // PSI_PIR_PROTOCOL_INTERNAL_CRYPTO_PROVIDER_H
