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

#include "privacy_computation_napi_loader.h"

#include "privacy_computation_napi_common.h"
#include <atomic>
#include <dlfcn.h>
#include <mutex>
#include <utility>

namespace OHOS::Security::PrivacyComputation {
namespace {
#ifdef ASSET_ENABLE_PRIVACY_COMPUTATION_CONFIG
constexpr char COMP_NAPI_PATH[] = ASSET_ENABLE_PRIVACY_COMPUTATION_CONFIG;
#else
constexpr char COMP_NAPI_PATH[] = "";
#endif
using ComputationNapiFunc = napi_value (*)(napi_env, napi_callback_info);
using ComputationFunc = int32_t (*)(const psi_pir::PrivacySearchResult &, const psi_pir::PrivacyProtocol &,
                                    std::vector<psi_pir::Bytes> &);

std::atomic<void *> g_computationHandle{nullptr};
std::mutex g_computationMutex;

void *GetComputationHandle()
{
    if (!IsComputationLibConfigured()) {
        return nullptr;
    }
    void *handle = g_computationHandle.load(std::memory_order_acquire);
    if (handle != nullptr) {
        return handle;
    }
    std::lock_guard<std::mutex> lock(g_computationMutex);
    handle = g_computationHandle.load(std::memory_order_relaxed);
    if (handle != nullptr) {
        return handle;
    }
    handle = dlopen(COMP_NAPI_PATH, RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        LOGE("dlopen computation so failed: %{public}s", dlerror());
        return nullptr;
    }
    // Closed-source async callbacks may outlive this invocation; keep the library loaded.
    g_computationHandle.store(handle, std::memory_order_release);
    LOGI("ComputationLib NAPI library loaded");
    return handle;
}
} // namespace

bool IsComputationLibConfigured()
{
    return COMP_NAPI_PATH[0] != '\0';
}

napi_value CallComputationNapiFunc(napi_env env, napi_callback_info info, const char *funcName)
{
    void *handle = GetComputationHandle();
    if (handle == nullptr) {
        return CreateRejectedPromise(env, PRIVACY_COMPUTATION_UNAVAILABLE, "Computation NAPI library is unavailable.");
    }
    dlerror();
    auto computationFunc = reinterpret_cast<ComputationNapiFunc>(dlsym(handle, funcName));
    const char *error = dlerror();
    if (error != nullptr || computationFunc == nullptr) {
        LOGE("dlsym %{public}s failed: %{public}s", funcName, error == nullptr ? "null symbol" : error);
        return CreateRejectedPromise(env, PRIVACY_COMPUTATION_UNAVAILABLE, "Computation NAPI symbol is unavailable.");
    }
    LOGI("ComputationLib dispatch %{public}s", funcName);
    return computationFunc(env, info);
}

int32_t DecryptCiphertextsWithComputationLib(const psi_pir::PrivacySearchResult &input,
                                             const psi_pir::PrivacyProtocol &protocol,
                                             std::vector<psi_pir::Bytes> &output)
{
    void *handle = GetComputationHandle();
    if (handle == nullptr) {
        return PRIVACY_COMPUTATION_UNAVAILABLE;
    }
    dlerror();
    auto computationFunc = reinterpret_cast<ComputationFunc>(dlsym(handle, "GetSearchResult"));
    const char *error = dlerror();
    if (error != nullptr || computationFunc == nullptr) {
        LOGE("dlsym GetSearchResult failed: %{public}s", error == nullptr ? "null symbol" : error);
        return PRIVACY_COMPUTATION_UNAVAILABLE;
    }
    std::vector<psi_pir::Bytes> plaintexts;
    const auto result = computationFunc(input, protocol, plaintexts);
    if (result != PRIVACY_COMPUTATION_SUCCESS) {
        LOGE("ComputationLib HE decryption failed, code=%{public}d", result);
        return result;
    }
    if (plaintexts.size() != input.resultCipherText.size()) {
        LOGE("ComputationLib returned inconsistent batch count, expected=%{public}zu, actual=%{public}zu",
             input.resultCipherText.size(), plaintexts.size());
        return PRIVACY_COMPUTATION_DATA_CORRUPTED;
    }
    output = std::move(plaintexts);
    LOGI("ComputationLib HE decryption success, batches=%{public}zu", output.size());
    return PRIVACY_COMPUTATION_SUCCESS;
}
} // namespace OHOS::Security::PrivacyComputation
