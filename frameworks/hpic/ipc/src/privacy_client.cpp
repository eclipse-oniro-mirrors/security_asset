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

#include "privacy_client_ffi.h"
#include "privacy_wire.h"
#include <utility>

namespace OHOS::Security::PrivacyComputation {
namespace {
using PrivacyRequest = int32_t (*)(const PrivacyClientInput *, PrivacyClientOutput *);
int32_t Request(PrivacyRequest call, PrivacyWire &wire)
{
    if (!wire.valid || wire.bytes.size() > MAX_PRIVACY_MESSAGE) {
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    psi_pir::Bytes result(MAX_PRIVACY_MESSAGE);
    const PrivacyClientInput input{wire.bytes.data(), static_cast<uint32_t>(wire.bytes.size())};
    PrivacyClientOutput output{result.data(), static_cast<uint32_t>(result.size())};
    const auto ret = call(&input, &output);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    if (output.size > result.size()) {
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    result.resize(output.size);
    wire.bytes = std::move(result);
    wire.position = 0;
    return psi_pir::PSI_PIR_SUCCESS;
}
void PutProtocol(PrivacyWire &wire, const psi_pir::PrivacyProtocol &protocol)
{
    wire.Put(static_cast<uint32_t>(protocol.dataSetSize));
    wire.Put(static_cast<uint32_t>(protocol.protocolType));
}
} // namespace
int32_t GenPrivacyTargetRemote(const psi_pir::TargetInput &input, const psi_pir::PrivacyProtocol &protocol,
                               psi_pir::Bytes &output)
{
    PrivacyWire wire;
    PutProtocol(wire, protocol);
    wire.Put(static_cast<uint32_t>(input.hashAlg));
    wire.Put(input.isCt ? 1 : 0);
    wire.PutBytes(input.elemData);
    const auto ret = Request(asset_privacy_gen_target, wire);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    auto result = wire.GetBytes();
    if (!wire.Done() || result.empty()) {
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    output = std::move(result);
    return psi_pir::PSI_PIR_SUCCESS;
}
int32_t GetSearchResultRemote(const psi_pir::PrivacySearchResult &input, const psi_pir::PrivacyProtocol &protocol,
                              psi_pir::SearchResult &output)
{
    PrivacyWire wire;
    PutProtocol(wire, protocol);
    wire.PutList(input.fheCipherText);
    wire.Put(input.symCipherText.has_value() ? 1 : 0);
    if (input.symCipherText) {
        wire.PutList(*input.symCipherText);
    }
    const auto ret = Request(asset_privacy_get_result, wire);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    const auto matched = wire.Get();
    const auto attached = wire.Get();
    psi_pir::SearchResult result;
    result.matchedResult = matched == 1;
    if (attached == 1) {
        result.attachedValues = wire.GetList();
    }
    if (!wire.Done() || matched > 1 || attached > 1) {
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    output = std::move(result);
    return psi_pir::PSI_PIR_SUCCESS;
}
} // namespace OHOS::Security::PrivacyComputation
