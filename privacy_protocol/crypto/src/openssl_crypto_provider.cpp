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

#include "openssl_crypto_provider.h"
#include "protocol_params.h"
#include "securec.h"

#include "privacy_error_code.h"
#include "privacy_log.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>

namespace PrivacyComputation::internal {
namespace {

constexpr std::size_t AES_KEY_SIZE = 32;
constexpr std::size_t GCM_NONCE_SIZE = 12;
constexpr std::size_t GCM_TAG_SIZE = 16;
constexpr std::size_t FRAME_HEADER_SIZE = 16;
constexpr std::size_t FRAME_FIXED_SIZE = FRAME_HEADER_SIZE + GCM_NONCE_SIZE + GCM_TAG_SIZE;
constexpr std::uint8_t FRAME_VERSION = 0x01;
constexpr std::uint8_t FRAME_ALGORITHM_AES_GCM = 0x01;
constexpr std::uint8_t PAYLOAD_VALUE_ABSENT = 0x00;
constexpr std::uint8_t PAYLOAD_VALUE_PRESENT = 0x01;

// Frame Header field offsets
constexpr std::size_t FRAME_OFFSET_MAGIC = 0;
constexpr std::size_t FRAME_MAGIC_SIZE = 4;
constexpr std::size_t FRAME_OFFSET_VERSION = FRAME_OFFSET_MAGIC + FRAME_MAGIC_SIZE;
constexpr std::size_t FRAME_OFFSET_ALGORITHM = FRAME_OFFSET_VERSION + 1;
constexpr std::size_t FRAME_OFFSET_NONCE_LEN = FRAME_OFFSET_ALGORITHM + 1;
constexpr std::size_t FRAME_OFFSET_TAG_LEN = FRAME_OFFSET_NONCE_LEN + 1;
constexpr std::size_t FRAME_OFFSET_KEY_SIZE = FRAME_OFFSET_TAG_LEN + 1;
constexpr std::size_t FRAME_KEY_SIZE_BYTES = 2;
constexpr std::size_t FRAME_OFFSET_COMPARE_BITS = FRAME_OFFSET_KEY_SIZE + FRAME_KEY_SIZE_BYTES;
constexpr std::size_t FRAME_COMPARE_BITS_BYTES = 2;
constexpr std::size_t FRAME_OFFSET_CIPHER_LEN = FRAME_OFFSET_COMPARE_BITS + FRAME_COMPARE_BITS_BYTES;
constexpr std::size_t UINT32_MOST_SIGNIFICANT_BYTE_SHIFT = 3 * kBitsPerByte;
constexpr std::size_t UINT32_SECOND_BYTE_SHIFT = 2 * kBitsPerByte;

constexpr std::size_t SHA256_DIGEST_SIZE = 32;
constexpr std::size_t SHA384_DIGEST_SIZE = 48;
constexpr std::size_t SHA512_DIGEST_SIZE = 64;
constexpr std::size_t AES_KEY_BITS = AES_KEY_SIZE * kBitsPerByte;
constexpr std::array<std::uint8_t, 4> FRAME_MAGIC{'P', 'I', 'R', '1'};
constexpr std::array<std::uint8_t, 22> HKDF_SALT{
    'O', 'p', 'e', 'n', 'H', 'a', 'r', 'm', 'o', 'n', 'y', '-', 'P', 'I', 'R', '-', 'K', 'D', 'F', '-', 'v', '1',
};
constexpr std::array<std::uint8_t, 34> HKDF_INFO{
    'p', 'i', 'r', '-', 's', 'e', 'r', 'v', 'i', 'c', 'e', '/', 'o', 'r', 'd', 'e', 'r',
    '/', 'A', 'E', 'S', '-', '2', '5', '6', '-', 'G', 'C', 'M', '/', 'v', '1', 0,   0,
};

const EVP_MD *GetDigestAlgorithm(HashAlg algorithm) noexcept
{
    switch (algorithm) {
        case HashAlg::SHA256:
            return EVP_sha256();
        case HashAlg::SHA384:
            return EVP_sha384();
        case HashAlg::SHA512:
            return EVP_sha512();
        default:
            return nullptr;
    }
}

std::size_t GetDigestSize(HashAlg algorithm) noexcept
{
    switch (algorithm) {
        case HashAlg::SHA256:
            return SHA256_DIGEST_SIZE;
        case HashAlg::SHA384:
            return SHA384_DIGEST_SIZE;
        case HashAlg::SHA512:
            return SHA512_DIGEST_SIZE;
        default:
            return 0;
    }
}

bool FitsOpenSslInt(std::size_t size) noexcept
{
    return size <= static_cast<std::size_t>(std::numeric_limits<int32_t>::max());
}

void WriteUint16Be(Bytes &output, std::size_t offset, std::uint16_t value) noexcept
{
    output[offset] = static_cast<std::uint8_t>(value >> kBitsPerByte);
    output[offset + 1] = static_cast<std::uint8_t>(value);
}

void WriteUint32Be(Bytes &output, std::size_t offset, std::uint32_t value) noexcept
{
    output[offset] = static_cast<std::uint8_t>(value >> UINT32_MOST_SIGNIFICANT_BYTE_SHIFT);
    output[offset + 1] = static_cast<std::uint8_t>(value >> UINT32_SECOND_BYTE_SHIFT);
    output[offset + sizeof(uint16_t)] = static_cast<std::uint8_t>(value >> kBitsPerByte);
    output[offset + sizeof(uint32_t) - 1] = static_cast<std::uint8_t>(value);
}

std::uint16_t ReadUint16Be(const Bytes &input, std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(input[offset]) << kBitsPerByte) | input[offset + 1]);
}

std::uint32_t ReadUint32Be(const Bytes &input, std::size_t offset) noexcept
{
    return (static_cast<std::uint32_t>(input[offset]) << UINT32_MOST_SIGNIFICANT_BYTE_SHIFT) |
           (static_cast<std::uint32_t>(input[offset + 1]) << UINT32_SECOND_BYTE_SHIFT) |
           (static_cast<std::uint32_t>(input[offset + sizeof(uint16_t)]) << kBitsPerByte) |
           static_cast<std::uint32_t>(input[offset + sizeof(uint32_t) - 1]);
}

bool ValidateValueParams(const Bytes &seed, const ProtocolParams &params) noexcept
{
    return params.mode == ProtocolMode::PIR && params.symAlgorithm == SymAlgorithm::AES_GCM &&
           params.symKeySize == AES_KEY_BITS && params.compareBits != 0 && params.compareBits % kBitsPerByte == 0 &&
           params.compareBits <= std::numeric_limits<std::uint16_t>::max() &&
           seed.size() == params.compareBits / kBitsPerByte;
}

bool ConfigureHkdf(EVP_PKEY_CTX *context, const Bytes &seed,
                   const std::array<std::uint8_t, HKDF_INFO.size()> &info) noexcept
{
    if (!FitsOpenSslInt(seed.size()) || !FitsOpenSslInt(info.size())) {
        LOGE("HKDF input length exceeds OpenSSL integer range");
        return false;
    }
    if (EVP_PKEY_derive_init(context) <= 0) {
        LOGE("HKDF derive initialization failed");
        return false;
    }
    if (EVP_PKEY_CTX_set_hkdf_mode(context, EVP_KDF_HKDF_MODE_EXTRACT_AND_EXPAND) <= 0) {
        LOGE("HKDF Extract-and-Expand mode setup failed");
        return false;
    }
    if (EVP_PKEY_CTX_set_hkdf_md(context, EVP_sha256()) <= 0) {
        LOGE("HKDF SHA-256 setup failed");
        return false;
    }
    if (EVP_PKEY_CTX_set1_hkdf_salt(context, HKDF_SALT.data(), HKDF_SALT.size()) <= 0) {
        LOGE("HKDF salt setup failed");
        return false;
    }
    if (EVP_PKEY_CTX_set1_hkdf_key(context, seed.data(), static_cast<int32_t>(seed.size())) <= 0) {
        LOGE("HKDF input key material setup failed");
        return false;
    }
    if (EVP_PKEY_CTX_add1_hkdf_info(context, info.data(), static_cast<int32_t>(info.size())) <= 0) {
        LOGE("HKDF info setup failed");
        return false;
    }
    return true;
}

// Keep the HKDF execution path aligned with HUKS: configure OpenSSL's
// EVP_PKEY_HKDF with digest, salt, input key material and protocol info, then
// let OpenSSL perform the complete RFC 5869 Extract-and-Expand operation.
int32_t DeriveAesKey(const Bytes &seed, const ProtocolParams &params,
                     std::array<std::uint8_t, AES_KEY_SIZE> &key) noexcept
{
    auto info = HKDF_INFO;
    info[info.size() - sizeof(uint16_t)] = static_cast<std::uint8_t>(params.compareBits >> kBitsPerByte);
    info[info.size() - 1] = static_cast<std::uint8_t>(params.compareBits);

    EVP_PKEY_CTX *context = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
    if (context == nullptr) {
        LOGE("HKDF context creation failed");
        OPENSSL_cleanse(key.data(), key.size());
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }

    std::size_t keySize = key.size();
    const bool configured = ConfigureHkdf(context, seed, info);
    const bool derived = configured && EVP_PKEY_derive(context, key.data(), &keySize) > 0;
    EVP_PKEY_CTX_free(context);
    if (!derived || keySize != key.size()) {
        LOGE("HKDF-SHA256 derive failed, configured=%{public}d, keyBytes=%{public}zu", configured ? 1 : 0, keySize);
        OPENSSL_cleanse(key.data(), key.size());
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

// Authenticated PIR1 header; byte layout is documented in GET_SEARCH_RESULT_CRYPTO_GUIDE.md.
Bytes BuildFrameHeader(const ProtocolParams &params, std::uint32_t ciphertextSize)
{
    Bytes header(FRAME_HEADER_SIZE, 0);
    if (memcpy_s(header.data(), header.size(), FRAME_MAGIC.data(), FRAME_MAGIC.size()) != EOK) {
        LOGE("Failed to copy PIR frame magic");
        return {};
    }
    header[FRAME_OFFSET_VERSION] = FRAME_VERSION;
    header[FRAME_OFFSET_ALGORITHM] = FRAME_ALGORITHM_AES_GCM;
    header[FRAME_OFFSET_NONCE_LEN] = static_cast<std::uint8_t>(GCM_NONCE_SIZE);
    header[FRAME_OFFSET_TAG_LEN] = static_cast<std::uint8_t>(GCM_TAG_SIZE);
    WriteUint16Be(header, FRAME_OFFSET_KEY_SIZE, static_cast<std::uint16_t>(params.symKeySize));
    WriteUint16Be(header, FRAME_OFFSET_COMPARE_BITS, static_cast<std::uint16_t>(params.compareBits));
    WriteUint32Be(header, FRAME_OFFSET_CIPHER_LEN, ciphertextSize);
    return header;
}

bool ParseFrameHeader(const Bytes &frame, const ProtocolParams &params, std::size_t &ciphertextSize) noexcept
{
    if (frame.size() < FRAME_FIXED_SIZE + 1) {
        LOGE("PIR1 frame too short, frameBytes=%{public}zu, minimum=%{public}zu", frame.size(), FRAME_FIXED_SIZE + 1);
        return false;
    }
    if (!std::equal(FRAME_MAGIC.begin(), FRAME_MAGIC.end(), frame.begin())) {
        LOGE("PIR1 frame magic mismatch");
        return false;
    }
    if (frame[FRAME_OFFSET_VERSION] != FRAME_VERSION || frame[FRAME_OFFSET_ALGORITHM] != FRAME_ALGORITHM_AES_GCM) {
        LOGE("PIR1 frame version or algorithm mismatch, version=%{public}u, algorithm=%{public}u",
             static_cast<uint32_t>(frame[FRAME_OFFSET_VERSION]), static_cast<uint32_t>(frame[FRAME_OFFSET_ALGORITHM]));
        return false;
    }
    if (frame[FRAME_OFFSET_NONCE_LEN] != GCM_NONCE_SIZE || frame[FRAME_OFFSET_TAG_LEN] != GCM_TAG_SIZE) {
        LOGE("PIR1 nonce or tag length mismatch, nonceBytes=%{public}u, tagBytes=%{public}u",
             static_cast<uint32_t>(frame[FRAME_OFFSET_NONCE_LEN]), static_cast<uint32_t>(frame[FRAME_OFFSET_TAG_LEN]));
        return false;
    }
    if (ReadUint16Be(frame, FRAME_OFFSET_KEY_SIZE) != params.symKeySize ||
        ReadUint16Be(frame, FRAME_OFFSET_COMPARE_BITS) != params.compareBits) {
        LOGE("PIR1 protocol fields mismatch, keyBits=%{public}u, compareBits=%{public}u",
             static_cast<uint32_t>(ReadUint16Be(frame, FRAME_OFFSET_KEY_SIZE)),
             static_cast<uint32_t>(ReadUint16Be(frame, FRAME_OFFSET_COMPARE_BITS)));
        return false;
    }
    const auto encodedSize = static_cast<std::size_t>(ReadUint32Be(frame, FRAME_OFFSET_CIPHER_LEN));
    if (encodedSize == 0 || encodedSize > std::numeric_limits<std::size_t>::max() - FRAME_FIXED_SIZE) {
        LOGE("PIR1 ciphertext length invalid, encodedBytes=%{public}zu", encodedSize);
        return false;
    }
    if (frame.size() != FRAME_FIXED_SIZE + encodedSize) {
        LOGE("PIR1 total length mismatch, frameBytes=%{public}zu, encodedBytes=%{public}zu", frame.size(), encodedSize);
        return false;
    }
    ciphertextSize = encodedSize;
    return true;
}

using CipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

CipherContext CreateCipherContext() noexcept
{
    return CipherContext(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
}

bool TryGetPayloadSize(const std::optional<Bytes> &value, std::size_t &payloadSize) noexcept
{
    const auto valueSize = value.has_value() ? value->size() : 0;
    if (valueSize > std::numeric_limits<std::size_t>::max() - 1) {
        return false;
    }
    payloadSize = valueSize + 1;
    return FitsOpenSslInt(payloadSize) && payloadSize <= std::numeric_limits<std::uint32_t>::max() &&
           payloadSize <= std::numeric_limits<std::size_t>::max() - FRAME_FIXED_SIZE;
}

Bytes BuildPayload(const std::optional<Bytes> &value, std::size_t payloadSize)
{
    Bytes payload(payloadSize, 0);
    payload[0] = value.has_value() ? PAYLOAD_VALUE_PRESENT : PAYLOAD_VALUE_ABSENT;
    if (value.has_value() && !value->empty()) {
        if (memcpy_s(payload.data() + 1, payload.size() - 1, value->data(), value->size()) != EOK) {
            LOGE("Failed to copy PIR value payload");
            return {};
        }
    }
    return payload;
}

int32_t PrepareEncryptionFrame(const ProtocolParams &params, std::size_t payloadSize, Bytes &header,
                               Bytes &frame) noexcept
{
    header = BuildFrameHeader(params, static_cast<std::uint32_t>(payloadSize));
    frame.assign(FRAME_FIXED_SIZE + payloadSize, 0);
    if (header.size() != FRAME_HEADER_SIZE ||
        memcpy_s(frame.data(), frame.size(), header.data(), header.size()) != EOK) {
        LOGE("Failed to copy PIR frame header");
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    auto *nonce = frame.data() + FRAME_HEADER_SIZE;
    if (RAND_priv_bytes(nonce, static_cast<int32_t>(GCM_NONCE_SIZE)) != 1) {
        LOGE("GCM nonce generation failed, requestedBytes=%{public}zu", GCM_NONCE_SIZE);
        OPENSSL_cleanse(frame.data(), frame.size());
        OPENSSL_cleanse(header.data(), header.size());
        return PRIVACY_PROTOCOL_RANDOM_ERROR;
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

bool InitializeEncryptionContext(EVP_CIPHER_CTX *context, const std::array<std::uint8_t, AES_KEY_SIZE> &key,
                                 const std::uint8_t *nonce, const Bytes &header) noexcept
{
    int32_t aadLength = 0;
    if (EVP_EncryptInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        LOGE("AES-GCM encrypt init failed");
        return false;
    }
    if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN, static_cast<int32_t>(GCM_NONCE_SIZE), nullptr) != 1) {
        LOGE("AES-GCM encrypt IV length setup failed");
        return false;
    }
    if (EVP_EncryptInit_ex(context, nullptr, nullptr, key.data(), nonce) != 1) {
        LOGE("AES-GCM encrypt key and nonce setup failed");
        return false;
    }
    if (EVP_EncryptUpdate(context, nullptr, &aadLength, header.data(), static_cast<int32_t>(header.size())) != 1) {
        LOGE("AES-GCM encrypt AAD update failed");
        return false;
    }
    return true;
}

bool EncryptPayload(EVP_CIPHER_CTX *context, const Bytes &payload, std::uint8_t *encrypted) noexcept
{
    int32_t encryptedLength = 0;
    int32_t finalLength = 0;
    if (EVP_EncryptUpdate(context, encrypted, &encryptedLength, payload.data(), static_cast<int32_t>(payload.size())) !=
        1) {
        LOGE("AES-GCM payload encryption failed, payloadBytes=%{public}zu", payload.size());
        return false;
    }
    if (EVP_EncryptFinal_ex(context, encrypted + encryptedLength, &finalLength) != 1) {
        LOGE("AES-GCM encrypt final failed");
        return false;
    }
    if (static_cast<std::size_t>(encryptedLength + finalLength) != payload.size()) {
        LOGE("AES-GCM encrypted length mismatch, expected=%{public}zu, actual=%{public}d", payload.size(),
             encryptedLength + finalLength);
        return false;
    }
    auto *tag = encrypted + payload.size();
    if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG, static_cast<int32_t>(GCM_TAG_SIZE), tag) != 1) {
        LOGE("AES-GCM tag extraction failed");
        return false;
    }
    return true;
}

int32_t EncryptValueIntoFrame(const std::optional<Bytes> &value, std::size_t payloadSize,
                              const std::array<std::uint8_t, AES_KEY_SIZE> &key, const Bytes &header,
                              Bytes &frame) noexcept
{
    auto context = CreateCipherContext();
    if (context == nullptr) {
        LOGE("EVP encryption context allocation failed");
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    auto payload = BuildPayload(value, payloadSize);
    if (payload.empty()) {
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    const auto *nonce = frame.data() + FRAME_HEADER_SIZE;
    auto *encrypted = frame.data() + FRAME_HEADER_SIZE + GCM_NONCE_SIZE;
    const bool success = InitializeEncryptionContext(context.get(), key, nonce, header) &&
                         EncryptPayload(context.get(), payload, encrypted);
    OPENSSL_cleanse(payload.data(), payload.size());
    return success ? PRIVACY_PROTOCOL_SUCCESS : PRIVACY_PROTOCOL_CRYPTO_ERROR;
}

bool InitializeDecryptionContext(EVP_CIPHER_CTX *context, const std::array<std::uint8_t, AES_KEY_SIZE> &key,
                                 const std::uint8_t *nonce, const Bytes &frame) noexcept
{
    int32_t aadLength = 0;
    if (EVP_DecryptInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        LOGE("AES-GCM decrypt init failed");
        return false;
    }
    if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN, static_cast<int32_t>(GCM_NONCE_SIZE), nullptr) != 1) {
        LOGE("AES-GCM decrypt IV length setup failed");
        return false;
    }
    if (EVP_DecryptInit_ex(context, nullptr, nullptr, key.data(), nonce) != 1) {
        LOGE("AES-GCM decrypt key and nonce setup failed");
        return false;
    }
    if (EVP_DecryptUpdate(context, nullptr, &aadLength, frame.data(), static_cast<int32_t>(FRAME_HEADER_SIZE)) != 1) {
        LOGE("AES-GCM decrypt AAD update failed");
        return false;
    }
    return true;
}

bool DecryptPayload(EVP_CIPHER_CTX *context, const std::uint8_t *encrypted, const std::uint8_t *tag,
                    Bytes &payload) noexcept
{
    int32_t plaintextLength = 0;
    int32_t finalLength = 0;
    if (EVP_DecryptUpdate(context, payload.data(), &plaintextLength, encrypted, static_cast<int32_t>(payload.size())) !=
        1) {
        LOGE("AES-GCM payload decryption failed, payloadBytes=%{public}zu", payload.size());
        return false;
    }
    std::array<std::uint8_t, GCM_TAG_SIZE> expectedTag{};
    if (memcpy_s(expectedTag.data(), expectedTag.size(), tag, GCM_TAG_SIZE) != EOK) {
        LOGE("AES-GCM expected tag copy failed");
        return false;
    }
    if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_TAG, static_cast<int32_t>(expectedTag.size()),
                            expectedTag.data()) != 1) {
        LOGE("AES-GCM expected tag setup failed");
        return false;
    }
    if (EVP_DecryptFinal_ex(context, payload.data() + plaintextLength, &finalLength) != 1) {
        LOGE("AES-GCM authentication or decrypt final failed");
        return false;
    }
    if (static_cast<std::size_t>(plaintextLength + finalLength) != payload.size()) {
        LOGE("AES-GCM plaintext length mismatch, expected=%{public}zu, actual=%{public}d", payload.size(),
             plaintextLength + finalLength);
        return false;
    }
    return true;
}

int32_t DecryptFrame(const Bytes &frame, std::size_t ciphertextSize, const std::array<std::uint8_t, AES_KEY_SIZE> &key,
                     Bytes &payload) noexcept
{
    auto context = CreateCipherContext();
    if (context == nullptr) {
        LOGE("EVP decryption context allocation failed");
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    const auto *nonce = frame.data() + FRAME_HEADER_SIZE;
    const auto *encrypted = nonce + GCM_NONCE_SIZE;
    const auto *tag = encrypted + ciphertextSize;
    payload.assign(ciphertextSize, 0);
    const bool success = InitializeDecryptionContext(context.get(), key, nonce, frame) &&
                         DecryptPayload(context.get(), encrypted, tag, payload);
    return success ? PRIVACY_PROTOCOL_SUCCESS : PRIVACY_PROTOCOL_CRYPTO_ERROR;
}

bool IsValidPayload(const Bytes &payload) noexcept
{
    if (payload.empty() || (payload[0] != PAYLOAD_VALUE_ABSENT && payload[0] != PAYLOAD_VALUE_PRESENT)) {
        return false;
    }
    return payload[0] != PAYLOAD_VALUE_ABSENT || payload.size() == 1;
}

void StoreDecryptedPayload(Bytes &payload, std::optional<Bytes> &output)
{
    if (payload[0] == PAYLOAD_VALUE_PRESENT) {
        Bytes value(payload.begin() + 1, payload.end());
        OPENSSL_cleanse(payload.data(), payload.size());
        output.emplace();
        output->swap(value);
        return;
    }
    OPENSSL_cleanse(payload.data(), payload.size());
    output = std::nullopt;
}

class OpenSslHashProvider final : public IHashProvider {
public:
    int32_t Digest(HashAlg algorithm, const Bytes &input, Bytes &output) const noexcept override;
};

int32_t OpenSslHashProvider::Digest(HashAlg algorithm, const Bytes &input, Bytes &output) const noexcept
{
    if (algorithm == HashAlg::NONE) {
        output = input;
        return PRIVACY_PROTOCOL_SUCCESS;
    }
    const auto digestAlgorithm = GetDigestAlgorithm(algorithm);
    const auto expectedSize = GetDigestSize(algorithm);
    if (digestAlgorithm == nullptr || expectedSize == 0) {
        LOGE("unsupported hash algorithm=%{public}u", static_cast<uint32_t>(algorithm));
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    Bytes digest(expectedSize, 0);
    uint32_t actualSize = 0;
    const std::uint8_t emptyInput = 0;
    const auto *inputData = input.empty() ? &emptyInput : input.data();
    if (EVP_Digest(inputData, input.size(), digest.data(), &actualSize, digestAlgorithm, nullptr) != 1 ||
        actualSize != expectedSize) {
        LOGE("EVP_Digest failed, algorithm=%{public}u, inputLen=%{public}zu", static_cast<uint32_t>(algorithm),
             input.size());
        return PRIVACY_PROTOCOL_HASH_ERROR;
    }
    output.swap(digest);
    return PRIVACY_PROTOCOL_SUCCESS;
}

class OpenSslRandomSource final : public IRandomSource {
public:
    int32_t Fill(Bytes &output) noexcept override;
};

int32_t OpenSslRandomSource::Fill(Bytes &output) noexcept
{
    if (output.empty() || !FitsOpenSslInt(output.size())) {
        LOGE("randomBytes fill invalid: size=%{public}zu", output.size());
        return PRIVACY_PROTOCOL_RANDOM_ERROR;
    }
    Bytes randomBytes(output.size(), 0);
    if (RAND_priv_bytes(randomBytes.data(), static_cast<int32_t>(randomBytes.size())) != 1) {
        LOGE("RAND_priv_bytes failed, requestedLen=%{public}zu", randomBytes.size());
        OPENSSL_cleanse(randomBytes.data(), randomBytes.size());
        return PRIVACY_PROTOCOL_RANDOM_ERROR;
    }
    output.swap(randomBytes);
    return PRIVACY_PROTOCOL_SUCCESS;
}

class OpenSslPirValueCrypto final : public IPirValueCrypto {
public:
    int32_t Encrypt(const Bytes &seed, const std::optional<Bytes> &value, const ProtocolParams &params,
                    Bytes &output) const noexcept override;

    int32_t Decrypt(const Bytes &seed, const Bytes &frame, const ProtocolParams &params,
                    std::optional<Bytes> &output) const noexcept override;
};

int32_t OpenSslPirValueCrypto::Encrypt(const Bytes &seed, const std::optional<Bytes> &value,
                                       const ProtocolParams &params, Bytes &output) const noexcept
{
    if (!ValidateValueParams(seed, params)) {
        LOGE("encrypt: invalid order params, seedSize=%{public}zu, "
             "symKeySize=%{public}zu, compareBits=%{public}zu",
             seed.size(), params.symKeySize, params.compareBits);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    std::size_t payloadSize = 0;
    if (!TryGetPayloadSize(value, payloadSize)) {
        LOGE("encrypt: payload size overflow");
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    Bytes header;
    Bytes frame;
    auto ret = PrepareEncryptionFrame(params, payloadSize, header, frame);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("encrypt: PrepareEncryptionFrame failed, ret=%{public}d", ret);
        return ret;
    }
    std::array<std::uint8_t, AES_KEY_SIZE> key{};
    ret = DeriveAesKey(seed, params, key);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("encrypt: DeriveAesKey failed, ret=%{public}d", ret);
        OPENSSL_cleanse(frame.data(), frame.size());
        OPENSSL_cleanse(header.data(), header.size());
        return ret;
    }
    ret = EncryptValueIntoFrame(value, payloadSize, key, header, frame);
    OPENSSL_cleanse(key.data(), key.size());
    if (ret == PRIVACY_PROTOCOL_SUCCESS) {
        output.swap(frame);
    } else {
        LOGE("encrypt: EncryptValueIntoFrame failed, ret=%{public}d", ret);
    }
    return ret;
}

int32_t OpenSslPirValueCrypto::Decrypt(const Bytes &seed, const Bytes &frame, const ProtocolParams &params,
                                       std::optional<Bytes> &output) const noexcept
{
    if (!ValidateValueParams(seed, params)) {
        LOGE("decrypt: invalid order params, seedSize=%{public}zu, "
             "symKeySize=%{public}zu, compareBits=%{public}zu",
             seed.size(), params.symKeySize, params.compareBits);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    std::size_t ciphertextSize = 0;
    if (!ParseFrameHeader(frame, params, ciphertextSize) || !FitsOpenSslInt(ciphertextSize)) {
        LOGE("decrypt: frame header invalid, frameSize=%{public}zu", frame.size());
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    std::array<std::uint8_t, AES_KEY_SIZE> key{};
    auto ret = DeriveAesKey(seed, params, key);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("decrypt: DeriveAesKey failed, ret=%{public}d", ret);
        return ret;
    }
    Bytes payload;
    ret = DecryptFrame(frame, ciphertextSize, key, payload);
    OPENSSL_cleanse(key.data(), key.size());
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("decrypt: AES-GCM frame decryption failed, ret=%{public}d", ret);
        OPENSSL_cleanse(payload.data(), payload.size());
        return ret;
    }
    if (!IsValidPayload(payload)) {
        LOGE("decrypt: authenticated payload layout is invalid");
        OPENSSL_cleanse(payload.data(), payload.size());
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    StoreDecryptedPayload(payload, output);
    return PRIVACY_PROTOCOL_SUCCESS;
}

} // namespace

std::shared_ptr<const IHashProvider> GetOpenSslHashProvider() noexcept
{
    static const auto provider = [] {
        auto instance = std::make_shared<const OpenSslHashProvider>();
        LOGI("OpenSSL hash provider initialized");
        return instance;
    }();
    return provider;
}

std::shared_ptr<IRandomSource> GetOpenSslRandomSource() noexcept
{
    static const auto provider = [] {
        auto instance = std::make_shared<OpenSslRandomSource>();
        LOGI("OpenSSL randomBytes provider initialized");
        return instance;
    }();
    return provider;
}

std::shared_ptr<const IPirValueCrypto> GetOpenSslPirValueCrypto() noexcept
{
    static const auto provider = [] {
        auto instance = std::make_shared<const OpenSslPirValueCrypto>();
        LOGI("OpenSSL PIR value crypto provider initialized");
        return instance;
    }();
    return provider;
}

} // namespace PrivacyComputation::internal
