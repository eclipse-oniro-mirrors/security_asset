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

#ifndef PRIVACY_PROTOCOL_OPENSSL_MOCK_H
#define PRIVACY_PROTOCOL_OPENSSL_MOCK_H

#include "securec.h"
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <cstdint>

namespace PrivacyProtocolTest {
enum class CryptoCall {
    NONE,
    COPY,
    RANDOM,
    DIGEST,
    NEW_CIPHER_CTX,
    CIPHER_CTRL,
    ENCRYPT_INIT,
    ENCRYPT_UPDATE,
    ENCRYPT_FINAL,
    DECRYPT_INIT,
    DECRYPT_UPDATE,
    DECRYPT_FINAL,
    NEW_PKEY_CTX,
    DERIVE_INIT,
    DERIVE,
    HKDF_PARAMETER,
};

enum class FaultResult { NONE, ERROR, WRONG_LENGTH };

struct CryptoFault {
    CryptoCall call = CryptoCall::NONE;
    uint32_t remaining = 0;
    FaultResult result = FaultResult::ERROR;
    size_t copyCapacity = 0;
    size_t copySize = 0;
};

inline thread_local CryptoFault g_cryptoFault;

inline FaultResult TakeFault(CryptoCall call)
{
    if (g_cryptoFault.call != call || g_cryptoFault.remaining == 0) {
        return FaultResult::NONE;
    }
    --g_cryptoFault.remaining;
    if (g_cryptoFault.remaining != 0) {
        return FaultResult::NONE;
    }
    return g_cryptoFault.result;
}

template<CryptoCall CALL, auto Backend>
struct MockApi;

template<CryptoCall CALL, typename Result, typename... Args, Result (*Backend)(Args...)>
struct MockApi<CALL, Backend> {
    static Result Call(Args... args)
    {
        if (TakeFault(CALL) == FaultResult::ERROR) {
            return {};
        }
        return Backend(args...);
    }
};

template<typename Length, typename Backend>
int32_t MockLength(CryptoCall call, Length *length, Backend run)
{
    const auto fault = TakeFault(call);
    if (fault == FaultResult::ERROR) {
        return 0;
    }
    const auto status = run();
    if (fault == FaultResult::WRONG_LENGTH && status == 1) {
        ++*length;
    }
    return status;
}

inline int32_t MockDigest(const void *data, size_t size, unsigned char *out, uint32_t *length,
    const EVP_MD *md, ENGINE *engine)
{
    return MockLength(CryptoCall::DIGEST, length, [&] { return EVP_Digest(data, size, out, length, md, engine); });
}

inline int32_t MockDerive(EVP_PKEY_CTX *ctx, unsigned char *out, size_t *length)
{
    return MockLength(CryptoCall::DERIVE, length, [&] { return EVP_PKEY_derive(ctx, out, length); });
}

template<CryptoCall CALL, auto Backend>
int32_t MockFinal(EVP_CIPHER_CTX *ctx, unsigned char *out, int32_t *length)
{
    return MockLength(CALL, length, [&] { return Backend(ctx, out, length); });
}

inline errno_t MockCopy(void *dest, size_t capacity, const void *src, size_t count)
{
    if (g_cryptoFault.copyCapacity != 0 &&
        (g_cryptoFault.copyCapacity != capacity || g_cryptoFault.copySize != count)) {
        return memcpy_s(dest, capacity, src, count);
    }
    if (TakeFault(CryptoCall::COPY) == FaultResult::ERROR) {
        return EINVAL;
    }
    return memcpy_s(dest, capacity, src, count);
}
} // namespace PrivacyProtocolTest

// Redirect calls in the test executable. Other targets compile against the real APIs.
#define EVP_CIPHER_CTX_new ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::NEW_CIPHER_CTX, \
    &::EVP_CIPHER_CTX_new>::Call
#define EVP_CIPHER_CTX_ctrl ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::CIPHER_CTRL, \
    &::EVP_CIPHER_CTX_ctrl>::Call
#define EVP_EncryptInit_ex ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::ENCRYPT_INIT, \
    &::EVP_EncryptInit_ex>::Call
#define EVP_EncryptUpdate ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::ENCRYPT_UPDATE, \
    &::EVP_EncryptUpdate>::Call
#define EVP_EncryptFinal_ex ::PrivacyProtocolTest::MockFinal<::PrivacyProtocolTest::CryptoCall::ENCRYPT_FINAL, \
    &::EVP_EncryptFinal_ex>
#define EVP_DecryptInit_ex ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::DECRYPT_INIT, \
    &::EVP_DecryptInit_ex>::Call
#define EVP_DecryptUpdate ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::DECRYPT_UPDATE, \
    &::EVP_DecryptUpdate>::Call
#define EVP_DecryptFinal_ex ::PrivacyProtocolTest::MockFinal<::PrivacyProtocolTest::CryptoCall::DECRYPT_FINAL, \
    &::EVP_DecryptFinal_ex>
#define EVP_Digest ::PrivacyProtocolTest::MockDigest
#define RAND_priv_bytes ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::RANDOM, \
    &::RAND_priv_bytes>::Call
#define EVP_PKEY_CTX_new_id ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::NEW_PKEY_CTX, \
    &::EVP_PKEY_CTX_new_id>::Call
#define EVP_PKEY_derive_init ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::DERIVE_INIT, \
    &::EVP_PKEY_derive_init>::Call
#define EVP_PKEY_derive ::PrivacyProtocolTest::MockDerive
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#define EVP_PKEY_CTX_set_hkdf_mode ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::HKDF_PARAMETER, \
    &::EVP_PKEY_CTX_set_hkdf_mode>::Call
#define EVP_PKEY_CTX_set_hkdf_md ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::HKDF_PARAMETER, \
    &::EVP_PKEY_CTX_set_hkdf_md>::Call
#define EVP_PKEY_CTX_set1_hkdf_salt ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::HKDF_PARAMETER, \
    &::EVP_PKEY_CTX_set1_hkdf_salt>::Call
#define EVP_PKEY_CTX_set1_hkdf_key ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::HKDF_PARAMETER, \
    &::EVP_PKEY_CTX_set1_hkdf_key>::Call
#define EVP_PKEY_CTX_add1_hkdf_info ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::HKDF_PARAMETER, \
    &::EVP_PKEY_CTX_add1_hkdf_info>::Call
#else
#define EVP_PKEY_CTX_ctrl ::PrivacyProtocolTest::MockApi<::PrivacyProtocolTest::CryptoCall::HKDF_PARAMETER, \
    &::EVP_PKEY_CTX_ctrl>::Call
#endif
#define memcpy_s ::PrivacyProtocolTest::MockCopy
#endif
