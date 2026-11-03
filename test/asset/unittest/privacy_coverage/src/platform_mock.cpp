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

#include "host_faults.h"
#include "hks_api.h"
#include "hks_param.h"
#include "privacy_request.h"
#include "privacy_client_ffi.h"
#include "privacy_computation_napi_common.h"
#include <algorithm>
#include <cstring>

extern "C" int32_t HksKeyExist(const HksBlob *, const HksParamSet *)
{
    return PrivacyHost::State().keyExistsStatus;
}
extern "C" int32_t HksGenerateKey(const HksBlob *, const HksParamSet *, HksParamSet *)
{
    return PrivacyHost::State().generateStatus;
}
extern "C" int32_t HksInit(const HksBlob *, const HksParamSet *, HksBlob *, HksBlob *)
{
    return PrivacyHost::State().huksInitStatus;
}
extern "C" int32_t HksUpdate(const HksBlob *, const HksParamSet *, const HksBlob *, HksBlob *)
{
    return PrivacyHost::State().huksUpdateStatus;
}
extern "C" int32_t HksFinish(const HksBlob *, const HksParamSet *, const HksBlob *salt, HksBlob *output)
{
    std::memcpy(output->data, salt->data, std::min(output->size, salt->size));
    output->size = PrivacyHost::State().seedBytes;
    return PrivacyHost::State().huksFinishStatus;
}
extern "C" int32_t HksAbort(const HksBlob *, const HksParamSet *)
{
    return HKS_SUCCESS;
}
extern "C" int32_t asset_privacy_gen_target(const PrivacyTargetRequest *input, PrivacyBuffer *output)
{
    return AssetGenPrivacyTarget(20010001, input, output);
}
extern "C" int32_t asset_privacy_decrypt_ciphertext(const PrivacyDecryptRequest *input, PrivacyBuffer *output)
{
    return AssetDecryptPrivacyCiphertext(20010001, input, output);
}
namespace {
int32_t ClosedDecrypt(const psi_pir::PrivacySearchResult &input, const psi_pir::PrivacyProtocol &protocol,
                      std::vector<psi_pir::Bytes> &output)
{
    for (const auto &cipher : input.resultCipherText) {
        psi_pir::Bytes plain;
        const auto ret = psi_pir::DecryptCiphertext(cipher, protocol, {20010001, 100}, plain);
        if (ret != psi_pir::PSI_PIR_SUCCESS) {
            return ret;
        }
        output.push_back(std::move(plain));
    }
    if (PrivacyHost::State().closedBatchMismatch) {
        output.emplace_back();
    }
    return PrivacyHost::State().closedStatus;
}
napi_value ClosedGen(napi_env env, napi_callback_info)
{
    return OHOS::Security::PrivacyComputation::CreateRejectedPromise(env, 24000017, "closed boundary mock");
}
} // namespace
extern "C" void *dlopen(const char *, int32_t)
{
    static uint8_t handle;
    return PrivacyHost::State().dlopenFailure ? nullptr : &handle;
}
extern "C" void *dlsym(void *, const char *name)
{
    if (PrivacyHost::State().symbolFailure) {
        return nullptr;
    }
    return std::strcmp(name, "GetSearchResult") == 0 ? reinterpret_cast<void *>(ClosedDecrypt)
                                                     : reinterpret_cast<void *>(ClosedGen);
}
extern "C" char *dlerror()
{
    return nullptr;
}
