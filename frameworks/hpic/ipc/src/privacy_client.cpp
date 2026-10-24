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

#include "privacy_wire.h"
#include "hpic_log.h"
#include "iservice_registry.h"
#include "system_ability_load_callback_stub.h"
#include "message_parcel.h"
#include "message_option.h"
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <new>
#include <utility>

namespace OHOS::Security::PrivacyComputation {
namespace {
constexpr int32_t ASSET_ID = 8100;
class LoadCallback final : public SystemAbilityLoadCallbackStub {
public:
    std::mutex mutex;
    std::condition_variable ready;
    bool done = false;
    sptr<IRemoteObject> remote;
    void OnLoadSystemAbilitySuccess(int32_t, const sptr<IRemoteObject> &object) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        remote = object; done = true; ready.notify_all();
    }
    void OnLoadSystemAbilityFail(int32_t) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        done = true; ready.notify_all();
    }
};
// Explicit bounded word framing matches Rust u32 parcel reads; no implicit Vec ABI.
bool WriteMessage(MessageParcel &parcel, const psi_pir::Bytes &bytes)
{
    if (bytes.size() > MAX_PRIVACY_MESSAGE || !parcel.WriteUint32(static_cast<uint32_t>(bytes.size()))) { return false; }
    for (size_t offset = 0; offset < bytes.size(); offset += 4) {
        uint32_t word = 0;
        for (size_t i = 0; i < 4 && offset + i < bytes.size(); ++i) { word |= uint32_t(bytes[offset + i]) << (i * 8); }
        if (!parcel.WriteUint32(word)) { return false; }
    }
    return true;
}
bool ReadMessage(MessageParcel &parcel, psi_pir::Bytes &bytes)
{
    uint32_t length = 0;
    if (!parcel.ReadUint32(length) || length > MAX_PRIVACY_MESSAGE) { return false; }
    psi_pir::Bytes output(length);
    for (size_t offset = 0; offset < output.size(); offset += 4) {
        uint32_t word = 0;
        if (!parcel.ReadUint32(word)) { return false; }
        for (size_t i = 0; i < 4 && offset + i < output.size(); ++i) { output[offset + i] = uint8_t(word >> (i * 8)); }
    }
    bytes = std::move(output);
    return true;
}
int32_t Request(uint32_t code, PrivacyWire &wire)
{
    if (!wire.valid || wire.bytes.size() > MAX_PRIVACY_MESSAGE) { return psi_pir::PSI_PIR_INVALID_ARGUMENT; }
    auto manager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (manager == nullptr) { return psi_pir::PSI_PIR_INTERNAL_ERROR; }
    auto remote = manager->CheckSystemAbility(ASSET_ID);
    if (remote == nullptr) {
        sptr<LoadCallback> callback = new (std::nothrow) LoadCallback();
        if (callback == nullptr || manager->LoadSystemAbility(ASSET_ID, callback) != 0) {
            LOGE("Failed to load asset SA"); return psi_pir::PSI_PIR_INTERNAL_ERROR;
        }
        std::unique_lock<std::mutex> lock(callback->mutex);
        if (!callback->ready.wait_for(lock, std::chrono::seconds(5), [&callback] { return callback->done; })) {
            LOGE("Timed out loading asset SA"); return psi_pir::PSI_PIR_INTERNAL_ERROR;
        }
        remote = callback->remote;
    }
    if (remote == nullptr) { return psi_pir::PSI_PIR_INTERNAL_ERROR; }
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    option.SetWaitTime(5);
    if (!data.WriteInterfaceToken(u"security_asset_service") || !WriteMessage(data, wire.bytes)) {
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    const auto transport = remote->SendRequest(code, data, reply, option);
    int32_t result = psi_pir::PSI_PIR_INTERNAL_ERROR;
    if (transport != 0 || !reply.ReadInt32(result)) {
        LOGE("Privacy IPC failed, status=%{public}d", transport); return psi_pir::PSI_PIR_INTERNAL_ERROR;
    }
    if (result != psi_pir::PSI_PIR_SUCCESS) { return result; }
    psi_pir::Bytes response;
    if (!ReadMessage(reply, response) || response.size() > MAX_PRIVACY_MESSAGE) {
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    wire.bytes = std::move(response); wire.position = 0;
    return psi_pir::PSI_PIR_SUCCESS;
}
void PutProtocol(PrivacyWire &wire, const psi_pir::PrivacyProtocol &protocol)
{
    wire.Put(static_cast<uint32_t>(protocol.dataSetSize));
    wire.Put(static_cast<uint32_t>(protocol.protocolType));
}
}
int32_t GenPrivacyTargetRemote(const psi_pir::TargetInput &input, const psi_pir::PrivacyProtocol &protocol,
    psi_pir::Bytes &output)
{
    PrivacyWire wire;
    PutProtocol(wire, protocol);
    wire.Put(static_cast<uint32_t>(input.hashAlg)); wire.Put(input.isCt ? 1 : 0); wire.PutBytes(input.elemData);
    const auto ret = Request(GEN_TARGET_TRANSACTION, wire);
    if (ret != psi_pir::PSI_PIR_SUCCESS) { return ret; }
    auto result = wire.GetBytes();
    if (!wire.Done() || result.empty()) { return psi_pir::PSI_PIR_CODEC_ERROR; }
    output = std::move(result);
    return psi_pir::PSI_PIR_SUCCESS;
}
int32_t GetSearchResultRemote(const psi_pir::PrivacySearchResult &input, const psi_pir::PrivacyProtocol &protocol,
    psi_pir::SearchResult &output)
{
    PrivacyWire wire;
    PutProtocol(wire, protocol);
    wire.PutList(input.fheCipherText); wire.Put(input.symCipherText.has_value() ? 1 : 0);
    if (input.symCipherText) { wire.PutList(*input.symCipherText); }
    const auto ret = Request(GET_RESULT_TRANSACTION, wire);
    if (ret != psi_pir::PSI_PIR_SUCCESS) { return ret; }
    const auto matched = wire.Get(); const auto attached = wire.Get();
    psi_pir::SearchResult result;
    result.matchedResult = matched == 1;
    if (attached == 1) { result.attachedValues = wire.GetList(); }
    if (!wire.Done() || matched > 1 || attached > 1) { return psi_pir::PSI_PIR_CODEC_ERROR; }
    output = std::move(result);
    return psi_pir::PSI_PIR_SUCCESS;
}
}
