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
#include "dlfcn.h"
#include <algorithm>
#include <cstring>
#include "securec.h"

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
    if (memcpy_s(output->data, output->size, salt->data, std::min(output->size, salt->size)) != EOK) {
        return HKS_ERROR_INVALID_ARGUMENT;
    }
    output->size = PrivacyHost::State().seedBytes;
    return PrivacyHost::State().huksFinishStatus;
}
extern "C" int32_t HksAbort(const HksBlob *, const HksParamSet *)
{
    return HKS_SUCCESS;
}
namespace {
constexpr uint32_t CALLER_UID = 20010001;
}

extern "C" int32_t asset_privacy_gen_target(const PrivacyTargetRequest *input, PrivacyBuffer *output)
{
    return AssetGenPrivacyTarget(CALLER_UID, input, output);
}
extern "C" int32_t asset_privacy_decrypt_ciphertext(const PrivacyDecryptRequest *input, PrivacyBuffer *output)
{
    return AssetDecryptPrivacyCiphertext(CALLER_UID, input, output);
}
namespace {
napi_value ClosedResult(napi_env env, const char *operation)
{
    ++PrivacyHost::State().calls[operation];
    const auto status = PrivacyHost::State().closedStatus;
    if (status != 0) {
        return OHOS::Security::PrivacyComputation::CreateRejectedPromise(env, status, "closed boundary mock");
    }
    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    napi_value result = nullptr;
    if (napi_create_promise(env, &deferred, &promise) != napi_ok ||
        napi_create_string_utf8(env, operation, NAPI_AUTO_LENGTH, &result) != napi_ok ||
        napi_resolve_deferred(env, deferred, result) != napi_ok) {
        return nullptr;
    }
    return promise;
}
napi_value ClosedGen(napi_env env, napi_callback_info)
{
    return ClosedResult(env, "ClosedGenPrivacyTarget");
}
napi_value ClosedGet(napi_env env, napi_callback_info)
{
    return ClosedResult(env, "ClosedGetSearchResult");
}

} // namespace
void *PrivacyHost::OpenLibrary(const char *, int32_t)
{
    static uint8_t handle;
    return PrivacyHost::State().dlopenFailure ? nullptr : &handle;
}
void *PrivacyHost::FindLibrarySymbol(void *, const char *name)
{
    if (PrivacyHost::State().symbolFailure) {
        return nullptr;
    }
    if (std::strcmp(name, "GetSearchResult") == 0) {
        return reinterpret_cast<void *>(ClosedGet);
    }
    return std::strcmp(name, "GenPrivacyTarget") == 0 ? reinterpret_cast<void *>(ClosedGen) : nullptr;
}
char *PrivacyHost::GetLibraryError()
{
    return nullptr;
}
