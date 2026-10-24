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
#include "privacy_request.h"
#include "privacy_wire.h"
#include "protocol_mapper.h"
#include "psi_pir_log.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <sys/stat.h>
#include <unistd.h>

namespace OHOS::Security::PrivacyComputation {
namespace {
using namespace psi_pir;
using namespace psi_pir::internal;
constexpr char ROOT_DIR[] = "/data/service/el1/public/asset_service/privacy";
constexpr char ROOT_FILE[] = "/data/service/el1/public/asset_service/privacy/root.v1";
constexpr char ROOT_TEMP[] = "/data/service/el1/public/asset_service/privacy/root.pending";
constexpr size_t ROOT_SIZE = 32;
constexpr size_t RECORD_SIZE = 72;
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
        if (mkdir(ROOT_DIR, 0700) != 0 && errno != EEXIST) {
            return false;
        }
        int dir = open(ROOT_DIR, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (dir < 0) {
            return false;
        }
        struct stat st {};
        bool ok = fstat(dir, &st) == 0 && st.st_uid == getuid() && (st.st_mode & 0777) == 0700;
        if (ok) {
            ok = LoadOrCreate(dir);
        }
        close(dir);
        ready = ok;
        return ok;
    }
    int32_t Derive(const std::array<uint8_t, 32> &hash, HeSeed &seed) const noexcept override
    {
        if (!ready) {
            return PSI_PIR_HE_ERROR;
        }
        constexpr unsigned char info[] = "PSI-PIR/HE-seed/v2/root-v1/BGV2048_48";
        EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
        std::array<uint8_t, 32> result{};
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
                                   (st.st_mode & 0777) == 0600 && st.st_size == RECORD_SIZE;
            const auto size = validFile ? read(fd, record.data(), record.size()) : -1;
            close(fd);
            const bool ok = size == RECORD_SIZE && Validate(record);
            if (ok) {
                std::copy_n(record.begin() + 8, ROOT_SIZE, root.begin());
            }
            OPENSSL_cleanse(record.data(), record.size());
            return ok; // Corrupt or unreadable existing roots are never regenerated.
        }
        if (errno != ENOENT) {
            return false;
        }
        // A persistent creation record remains as a loss marker. Missing root then fails closed.
        fd = open(ROOT_TEMP, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0) {
            return false;
        }
        record[0] = 'P';
        record[1] = 'I';
        record[2] = 'R';
        record[3] = 'K';
        record[4] = 1;
        unsigned int digestSize = 0;
        bool ok = RAND_priv_bytes(record.data() + 8, ROOT_SIZE) == 1 &&
                  EVP_Digest(record.data(), 40, record.data() + 40, &digestSize, EVP_sha256(), nullptr) == 1 &&
                  digestSize == 32;
        ok = ok && write(fd, record.data(), record.size()) == RECORD_SIZE && fsync(fd) == 0;
        close(fd);
        ok = ok && link(ROOT_TEMP, ROOT_FILE) == 0 && fsync(dir) == 0;
        if (ok) {
            std::copy_n(record.begin() + 8, ROOT_SIZE, root.begin());
        }
        // Keep the marker hard link: deletion of the main record cannot silently create a new root.
        OPENSSL_cleanse(record.data(), record.size());
        return ok;
    }
    bool Validate(const std::array<uint8_t, RECORD_SIZE> &record)
    {
        if (record[0] != 'P' || record[1] != 'I' || record[2] != 'R' || record[3] != 'K' || record[4] != 1 ||
            record[5] != 0 || record[6] != 0 || record[7] != 0) {
            return false;
        }
        std::array<uint8_t, 32> hash{};
        unsigned int size = 0;
        const bool ok = EVP_Digest(record.data(), 40, hash.data(), &size, EVP_sha256(), nullptr) == 1 &&
                        size == hash.size() && CRYPTO_memcmp(hash.data(), record.data() + 40, hash.size()) == 0;
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
int32_t Process(uint32_t code, uint32_t uid, PrivacyWire &wire, PrivacyWire &response)
{
    PrivacyProtocol protocol;
    protocol.dataSetSize = static_cast<DataSetSize>(wire.Get());
    protocol.protocolType = static_cast<ProtocolType>(wire.Get());
    ProtocolParams params;
    const auto mapping = MapPrivacyProtocol(protocol, params);
    if (mapping != PSI_PIR_SUCCESS) {
        return mapping;
    }
    int32_t userId = -1;
    if (AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(uid, userId) != 0 || userId < 0) {
        return PSI_PIR_INTERNAL_ERROR;
    }
    const OperationIdentity identity{uid, static_cast<uint32_t>(userId)};
#ifdef ASSET_ENABLE_PRIVACY_COMPUTATION_CONFIG
    if (ASSET_ENABLE_PRIVACY_COMPUTATION_CONFIG[0] != '\0') {
        // A NAPI export cannot be invoked from a service without a JS runtime/identity delegation.
        return PSI_PIR_HE_ERROR;
    }
#endif
    if (code == GEN_TARGET_TRANSACTION) {
        TargetInput input;
        input.hashAlg = static_cast<HashAlg>(wire.Get());
        const auto isCt = wire.Get();
        input.isCt = isCt == 1;
        input.elemData = wire.GetBytes();
        if (!wire.Done() || isCt > 1) {
            return PSI_PIR_INVALID_ARGUMENT;
        }
        if (!EnsureRoot()) {
            return PSI_PIR_HE_ERROR;
        }
        Bytes target;
        const auto ret = genPrivacyTarget(input, protocol, identity, target);
        if (ret == PSI_PIR_SUCCESS) {
            response.PutBytes(target);
        }
        return ret;
    }
    if (code != GET_RESULT_TRANSACTION) {
        return PSI_PIR_INVALID_ARGUMENT;
    }
    PrivacySearchResult input;
    input.fheCipherText = wire.GetList();
    const auto symmetric = wire.Get();
    if (symmetric == 1) {
        input.symCipherText = wire.GetList();
    }
    if (!wire.Done() || symmetric > 1) {
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (!EnsureRoot()) {
        return PSI_PIR_HE_ERROR;
    }
    SearchResult result;
    const auto ret = getSearchResult(input, protocol, identity, DecryptionBackend::SOFT, result);
    if (ret == PSI_PIR_SUCCESS) {
        response.Put(result.matchedResult ? 1 : 0);
        response.Put(result.attachedValues.has_value() ? 1 : 0);
        if (result.attachedValues) {
            response.PutList(*result.attachedValues);
        }
    }
    return ret;
}
} // namespace
} // namespace OHOS::Security::PrivacyComputation

// Called synchronously on the Binder entry thread. UID is captured in Rust before dispatch.
extern "C" int32_t AssetPrivacyRequest(const AssetPrivacyInput *input, AssetPrivacyOutput *output)
{
    using namespace OHOS::Security::PrivacyComputation;
    if (input == nullptr || output == nullptr || input->data == nullptr || output->data == nullptr ||
        input->size > MAX_PRIVACY_MESSAGE) {
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    PrivacyWire wire;
    wire.bytes.assign(input->data, input->data + input->size);
    PrivacyWire response;
    const auto ret = Process(input->code, input->uid, wire, response);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    if (!response.valid || response.bytes.size() > output->size) {
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    std::copy(response.bytes.begin(), response.bytes.end(), output->data);
    output->size = static_cast<uint32_t>(response.bytes.size());
    return psi_pir::PSI_PIR_SUCCESS;
}
