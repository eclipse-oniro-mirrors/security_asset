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
#include "os_account_manager.h"
#include "privacy_limits.h"
#include "privacy_request.h"
#include "protocol_mapper.h"
#include "psi_pir_log.h"
#include "psi_pir_protocol.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <new>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace OHOS::Security::PrivacyComputation {
namespace {
using namespace psi_pir;
using namespace psi_pir::internal;
constexpr char ROOT_DIR[] = "/data/service/el1/public/asset_service/privacy";
constexpr char ROOT_FILE[] = "/data/service/el1/public/asset_service/privacy/root.v1";
constexpr char ROOT_TEMP[] = "/data/service/el1/public/asset_service/privacy/root.pending";
constexpr size_t ROOT_SIZE = 32;
constexpr uint8_t ROOT_VERSION = 1;
constexpr std::array<uint8_t, 8> ROOT_HEADER{'P', 'I', 'R', 'K', ROOT_VERSION, 0, 0, 0};
constexpr size_t ROOT_OFFSET = ROOT_HEADER.size();
constexpr size_t ROOT_DIGEST_OFFSET = ROOT_OFFSET + ROOT_SIZE;
constexpr size_t ROOT_DIGEST_SIZE = 32;
constexpr size_t RECORD_SIZE = ROOT_DIGEST_OFFSET + ROOT_DIGEST_SIZE;
constexpr mode_t ROOT_DIR_MODE = 0700;
constexpr mode_t ROOT_FILE_MODE = 0600;
constexpr mode_t PERMISSION_MASK = 0777;
class FileSeedDeriver final : public IHeSeedDeriver {
public:
    ~FileSeedDeriver() override
    {
        OPENSSL_cleanse(root.data(), root.size());
    }
    std::array<uint8_t, ROOT_SIZE> root{};
    bool ready = false;
    bool Initialize()
    {
        if (mkdir(ROOT_DIR, ROOT_DIR_MODE) != 0 && errno != EEXIST) {
            return false;
        }
        int dir = open(ROOT_DIR, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (dir < 0) {
            return false;
        }
        struct stat st {};
        bool ok = fstat(dir, &st) == 0 && st.st_uid == getuid() && (st.st_mode & PERMISSION_MASK) == ROOT_DIR_MODE;
        if (ok) {
            ok = LoadOrCreate(dir);
        }
        close(dir);
        ready = ok;
        return ok;
    }
    int32_t Derive(const std::array<uint8_t, kIdentityHashSize> &hash, HeSeed &seed) const noexcept override
    {
        if (!ready) {
            return PSI_PIR_HE_ERROR;
        }
        constexpr unsigned char info[] = "PSI-PIR/HE-seed/v2/root-v1/BGV2048_48";
        EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
        std::array<uint8_t, kHeSeedSize> result{};
        size_t size = result.size();
        const bool ok = ctx != nullptr && EVP_PKEY_derive_init(ctx) > 0 &&
                        EVP_PKEY_CTX_set_hkdf_mode(ctx, EVP_KDF_HKDF_MODE_EXTRACT_AND_EXPAND) > 0 &&
                        EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_sha256()) > 0 &&
                        EVP_PKEY_CTX_set1_hkdf_salt(ctx, hash.data(), hash.size()) > 0 &&
                        EVP_PKEY_CTX_set1_hkdf_key(ctx, root.data(), root.size()) > 0 &&
                        EVP_PKEY_CTX_add1_hkdf_info(ctx, info, sizeof(info) - 1) > 0 &&
                        EVP_PKEY_derive(ctx, result.data(), &size) > 0 && size == result.size();
        EVP_PKEY_CTX_free(ctx);
        if (ok) {
            seed.bytes = result;
        }
        OPENSSL_cleanse(result.data(), result.size());
        return ok ? PSI_PIR_SUCCESS : PSI_PIR_CRYPTO_ERROR;
    }

private:
    bool LoadOrCreate(int dir)
    {
        std::array<uint8_t, RECORD_SIZE> record{};
        int fd = open(ROOT_FILE, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
        if (fd >= 0) {
            struct stat st {};
            const bool validFile = fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_uid == getuid() &&
                                   (st.st_mode & PERMISSION_MASK) == ROOT_FILE_MODE && st.st_size == RECORD_SIZE;
            const auto size = validFile ? read(fd, record.data(), record.size()) : -1;
            close(fd);
            const bool ok = size == RECORD_SIZE && Validate(record);
            if (ok) {
                std::copy_n(record.begin() + ROOT_OFFSET, ROOT_SIZE, root.begin());
            }
            OPENSSL_cleanse(record.data(), record.size());
            return ok; // Corrupt or unreadable existing roots are never regenerated.
        }
        if (errno != ENOENT) {
            return false;
        }
        // A persistent creation record remains as a loss marker. Missing root then fails closed.
        fd = open(ROOT_TEMP, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, ROOT_FILE_MODE);
        if (fd < 0) {
            return false;
        }
        std::copy(ROOT_HEADER.begin(), ROOT_HEADER.end(), record.begin());
        unsigned int digestSize = 0;
        bool ok = RAND_priv_bytes(record.data() + ROOT_OFFSET, ROOT_SIZE) == 1 &&
                  EVP_Digest(record.data(), ROOT_DIGEST_OFFSET, record.data() + ROOT_DIGEST_OFFSET, &digestSize,
                             EVP_sha256(), nullptr) == 1 &&
                  digestSize == ROOT_DIGEST_SIZE;
        ok = ok && write(fd, record.data(), record.size()) == RECORD_SIZE && fsync(fd) == 0;
        close(fd);
        ok = ok && link(ROOT_TEMP, ROOT_FILE) == 0 && fsync(dir) == 0;
        if (ok) {
            std::copy_n(record.begin() + ROOT_OFFSET, ROOT_SIZE, root.begin());
        }
        // Keep the marker hard link: deletion of the main record cannot silently create a new root.
        OPENSSL_cleanse(record.data(), record.size());
        return ok;
    }
    bool Validate(const std::array<uint8_t, RECORD_SIZE> &record)
    {
        if (!std::equal(ROOT_HEADER.begin(), ROOT_HEADER.end(), record.begin())) {
            return false;
        }
        std::array<uint8_t, ROOT_DIGEST_SIZE> hash{};
        unsigned int size = 0;
        const bool ok = EVP_Digest(record.data(), ROOT_DIGEST_OFFSET, hash.data(), &size, EVP_sha256(), nullptr) == 1 &&
                        size == hash.size() &&
                        CRYPTO_memcmp(hash.data(), record.data() + ROOT_DIGEST_OFFSET, hash.size()) == 0;
        OPENSSL_cleanse(hash.data(), hash.size());
        return ok;
    }
};
std::mutex g_rootMutex;
std::shared_ptr<const FileSeedDeriver> g_root;
bool EnsureRoot()
{
    std::lock_guard<std::mutex> lock(g_rootMutex);
    if (!g_root) {
        auto root = std::make_shared<FileSeedDeriver>();
        if (!root->Initialize()) {
            PSI_PIR_LOGE("Failed to restore privacy root material");
            return false;
        }
        SetHeSeedDeriver(root);
        g_root = std::move(root);
    }
    return true;
}
int32_t PrepareIdentity(uint32_t uid, CallerIdentity &identity)
{
    int32_t userId = -1;
    if (AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(uid, userId) != 0 || userId < 0) {
        PSI_PIR_LOGE("Failed to resolve privacy caller OS account");
        return PSI_PIR_INTERNAL_ERROR;
    }
    identity = {uid, static_cast<uint32_t>(userId)};
    return PSI_PIR_SUCCESS;
}
bool CopyBlob(const PrivacyBlob &input, Bytes &output, size_t &total)
{
    if (total > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES ||
        input.size > MAX_PRIVACY_BYTES - total - PRIVACY_LENGTH_BYTES || (input.size != 0 && input.data == nullptr)) {
        return false;
    }
    total += PRIVACY_LENGTH_BYTES + input.size;
    if (input.size != 0) {
        output.assign(input.data, input.data + input.size);
    }
    return true;
}
bool CopyBlobs(const PrivacyBlobList &input, std::vector<Bytes> &output, size_t &total)
{
    if (input.count > MAX_PRIVACY_ITEMS || total > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES ||
        (input.count != 0 && input.data == nullptr)) {
        return false;
    }
    total += PRIVACY_LENGTH_BYTES;
    for (uint32_t i = 0; i < input.count; ++i) {
        Bytes value;
        if (!CopyBlob(input.data[i], value, total)) {
            return false;
        }
        output.push_back(std::move(value));
    }
    return true;
}
struct PrivacyResultStorage {
    SearchResult result;
    std::vector<PrivacyBlob> values;
};
int32_t ExportResult(std::unique_ptr<PrivacyResultStorage> resultStorage, PrivacyResult *output)
{
    size_t total = RESULT_FIXED_BYTES + PRIVACY_LENGTH_BYTES;
    if (resultStorage->result.attachedValues) {
        for (const auto &value : *resultStorage->result.attachedValues) {
            if (total > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES ||
                value.size() > MAX_PRIVACY_BYTES - total - PRIVACY_LENGTH_BYTES) {
                return PSI_PIR_CODEC_ERROR;
            }
            total += PRIVACY_LENGTH_BYTES + value.size();
            resultStorage->values.push_back({value.data(), static_cast<uint32_t>(value.size())});
        }
    }
    *output = {resultStorage->result.matchedResult ? 1U : 0U,
               resultStorage->result.attachedValues ? 1U : 0U,
               {resultStorage->values.data(), static_cast<uint32_t>(resultStorage->values.size())},
               resultStorage.get()};
    resultStorage.release();
    return PSI_PIR_SUCCESS;
}
} // namespace
} // namespace OHOS::Security::PrivacyComputation

extern "C" int32_t AssetGenPrivacyTarget(uint32_t uid, const PrivacyTargetRequest *input, PrivacyBuffer *output)
{
    using namespace OHOS::Security::PrivacyComputation;
    if (input == nullptr || output == nullptr || output->data == nullptr || output->size > MAX_PRIVACY_BYTES) {
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    psi_pir::TargetInput target;
    target.hashAlg = static_cast<psi_pir::HashAlg>(input->hashAlg);
    size_t total = TARGET_FIXED_BYTES;
    if (!CopyBlob(input->element, target.elemData, total)) {
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    const psi_pir::PrivacyProtocol protocol{static_cast<psi_pir::DataSetSize>(input->protocol.dataSetSize),
                                            static_cast<psi_pir::ProtocolType>(input->protocol.protocolType)};
    psi_pir::internal::ProtocolParams params;
    auto ret = psi_pir::internal::MapPrivacyProtocol(protocol, params);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    psi_pir::CallerIdentity identity;
    ret = PrepareIdentity(uid, identity);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    if (!EnsureRoot()) {
        return psi_pir::PSI_PIR_HE_ERROR;
    }
    psi_pir::Bytes result;
    ret = psi_pir::genPrivacyTarget(target, protocol, identity, result);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    if (result.size() > output->size) {
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    std::copy(result.begin(), result.end(), output->data);
    output->size = static_cast<uint32_t>(result.size());
    return psi_pir::PSI_PIR_SUCCESS;
}

extern "C" int32_t AssetGetPrivacyResult(uint32_t uid, const PrivacySearchRequest *input, PrivacyResult *output)
{
    using namespace OHOS::Security::PrivacyComputation;
    if (input == nullptr || output == nullptr || input->hasSymmetric > 1) {
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    psi_pir::PrivacySearchResult request;
    size_t total = SEARCH_FIXED_BYTES;
    if (!CopyBlobs(input->ciphertexts, request.fheCipherText, total)) {
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    if (input->hasSymmetric == 1) {
        request.symCipherText.emplace();
        if (!CopyBlobs(input->symmetric, *request.symCipherText, total)) {
            return psi_pir::PSI_PIR_INVALID_ARGUMENT;
        }
    }
    const psi_pir::PrivacyProtocol protocol{static_cast<psi_pir::DataSetSize>(input->protocol.dataSetSize),
                                            static_cast<psi_pir::ProtocolType>(input->protocol.protocolType)};
    psi_pir::internal::ProtocolParams params;
    auto ret = psi_pir::internal::MapPrivacyProtocol(protocol, params);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    psi_pir::CallerIdentity identity;
    ret = PrepareIdentity(uid, identity);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    if (!EnsureRoot()) {
        return psi_pir::PSI_PIR_HE_ERROR;
    }
    auto resultStorage = std::unique_ptr<PrivacyResultStorage>(new (std::nothrow) PrivacyResultStorage());
    if (!resultStorage) {
        return psi_pir::PSI_PIR_INTERNAL_ERROR;
    }
    ret = psi_pir::getSearchResult(request, protocol, identity, resultStorage->result);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    return ExportResult(std::move(resultStorage), output);
}

extern "C" void AssetFreePrivacyResult(PrivacyResult *output)
{
    using namespace OHOS::Security::PrivacyComputation;
    if (output == nullptr) {
        return;
    }
    delete static_cast<PrivacyResultStorage *>(output->storage);
    *output = {};
}
