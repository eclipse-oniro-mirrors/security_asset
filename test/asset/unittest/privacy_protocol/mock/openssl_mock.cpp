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

#include "openssl_mock.h"
#include "securec.h"
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <cstdint>

using namespace PrivacyProtocolTest;

namespace {
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

template<typename Length, typename Backend, typename... Args>
int32_t MockLength(CryptoCall call, Length *length, Backend run, Args... args)
{
    const auto fault = TakeFault(call);
    if (fault == FaultResult::ERROR) {
        return 0;
    }
    const auto status = run(args...);
    if (fault == FaultResult::WRONG_LENGTH && status == 1) {
        ++*length;
    }
    return status;
}

template<CryptoCall CALL, auto Backend>
int32_t MockFinal(EVP_CIPHER_CTX *ctx, unsigned char *out, int32_t *length)
{
    return MockLength(CALL, length, Backend, ctx, out, length);
}

} // namespace

extern "C" int32_t MockDigest(const void *data, size_t size, unsigned char *out, uint32_t *length,
    const EVP_MD *md, ENGINE *engine)
{
    return MockLength(CryptoCall::DIGEST, length, EVP_Digest, data, size, out, length, md, engine);
}

extern "C" int32_t MockDerive(EVP_PKEY_CTX *ctx, unsigned char *out, size_t *length)
{
    return MockLength(CryptoCall::DERIVE, length, EVP_PKEY_derive, ctx, out, length);
}

extern "C" errno_t MockCopy(void *dest, size_t capacity, const void *src, size_t count)
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

extern "C" EVP_CIPHER_CTX *MockCipherNew()
{
    return MockApi<CryptoCall::NEW_CIPHER_CTX, EVP_CIPHER_CTX_new>::Call();
}

extern "C" int32_t MockCipherCtrl(EVP_CIPHER_CTX *ctx, int32_t type, int32_t arg, void *data)
{
    return MockApi<CryptoCall::CIPHER_CTRL, EVP_CIPHER_CTX_ctrl>::Call(ctx, type, arg, data);
}

extern "C" int32_t MockEncryptInit(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
    ENGINE *engine, const uint8_t *key, const uint8_t *iv)
{
    return MockApi<CryptoCall::ENCRYPT_INIT, EVP_EncryptInit_ex>::Call(ctx, cipher, engine, key, iv);
}

extern "C" int32_t MockEncryptUpdate(EVP_CIPHER_CTX *ctx, uint8_t *out, int32_t *size,
    const uint8_t *in, int32_t count)
{
    return MockApi<CryptoCall::ENCRYPT_UPDATE, EVP_EncryptUpdate>::Call(ctx, out, size, in, count);
}

extern "C" int32_t MockDecryptInit(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
    ENGINE *engine, const uint8_t *key, const uint8_t *iv)
{
    return MockApi<CryptoCall::DECRYPT_INIT, EVP_DecryptInit_ex>::Call(ctx, cipher, engine, key, iv);
}

extern "C" int32_t MockDecryptUpdate(EVP_CIPHER_CTX *ctx, uint8_t *out, int32_t *size,
    const uint8_t *in, int32_t count)
{
    return MockApi<CryptoCall::DECRYPT_UPDATE, EVP_DecryptUpdate>::Call(ctx, out, size, in, count);
}

extern "C" int32_t MockRandom(uint8_t *out, int32_t size)
{
    return MockApi<CryptoCall::RANDOM, RAND_priv_bytes>::Call(out, size);
}

extern "C" EVP_PKEY_CTX *MockPkeyNew(int32_t id, ENGINE *engine)
{
    return MockApi<CryptoCall::NEW_PKEY_CTX, EVP_PKEY_CTX_new_id>::Call(id, engine);
}

extern "C" int32_t MockDeriveInit(EVP_PKEY_CTX *ctx)
{
    return MockApi<CryptoCall::DERIVE_INIT, EVP_PKEY_derive_init>::Call(ctx);
}

extern "C" int32_t MockEncryptFinal(EVP_CIPHER_CTX *ctx, uint8_t *out, int32_t *length)
{
    return MockFinal<CryptoCall::ENCRYPT_FINAL, EVP_EncryptFinal_ex>(ctx, out, length);
}

extern "C" int32_t MockDecryptFinal(EVP_CIPHER_CTX *ctx, uint8_t *out, int32_t *length)
{
    return MockFinal<CryptoCall::DECRYPT_FINAL, EVP_DecryptFinal_ex>(ctx, out, length);
}

#if OPENSSL_VERSION_NUMBER >= 0x30000000L

extern "C" int32_t MockHkdfMode(EVP_PKEY_CTX *ctx, int32_t mode)
{
    return MockApi<CryptoCall::HKDF_PARAMETER, EVP_PKEY_CTX_set_hkdf_mode>::Call(ctx, mode);
}

extern "C" int32_t MockHkdfDigest(EVP_PKEY_CTX *ctx, const EVP_MD *md)
{
    return MockApi<CryptoCall::HKDF_PARAMETER, EVP_PKEY_CTX_set_hkdf_md>::Call(ctx, md);
}

extern "C" int32_t MockHkdfSalt(EVP_PKEY_CTX *ctx, const uint8_t *salt, int32_t size)
{
    return MockApi<CryptoCall::HKDF_PARAMETER, EVP_PKEY_CTX_set1_hkdf_salt>::Call(ctx, salt, size);
}

extern "C" int32_t MockHkdfKey(EVP_PKEY_CTX *ctx, const uint8_t *key, int32_t size)
{
    return MockApi<CryptoCall::HKDF_PARAMETER, EVP_PKEY_CTX_set1_hkdf_key>::Call(ctx, key, size);
}

extern "C" int32_t MockHkdfInfo(EVP_PKEY_CTX *ctx, const uint8_t *info, int32_t size)
{
    return MockApi<CryptoCall::HKDF_PARAMETER, EVP_PKEY_CTX_add1_hkdf_info>::Call(ctx, info, size);
}
#else

extern "C" int32_t MockHkdfCtrl(EVP_PKEY_CTX *ctx, int32_t keyType, int32_t opType,
    int32_t command, int32_t value, void *data)
{
    return MockApi<CryptoCall::HKDF_PARAMETER, EVP_PKEY_CTX_ctrl>::Call(ctx, keyType, opType, command, value, data);
}
#endif
