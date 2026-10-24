#include "psi_pir_protocol.h"

#include "psi_pir_log.h"
#include "protocol_mapper.h"
#include "psi_pir_protocol_impl.h"
#include "search_result_processor.h"
#include "bgv_encryptor.h"
#include "bgv_decryptor.h"
#include "ccz_decrypt_bridge.h"
#include "soft_he_engine.h"
#include <utility>

namespace psi_pir {

int32_t privacySearch(const Bytes &privacyTarget,
    const std::vector<Element> &elements, const PrivacyProtocol &privacyProtocol,
    PrivacySearchResult &privacySearchResult) noexcept
{
    PSI_PIR_LOGI("privacySearch enter, targetBytes=%{public}zu, elements=%{public}zu, "
        "dataSetSize=%{public}u, protocolType=%{public}u", privacyTarget.size(), elements.size(),
        static_cast<uint32_t>(privacyProtocol.dataSetSize), static_cast<uint32_t>(privacyProtocol.protocolType));
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(privacyProtocol, params);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("privacySearch failed at protocol mapping, ret=%{public}d", ret);
        return ret;
    }
    const auto installRet = internal::EnsureDefaultHeEngineInstalled();
    if (installRet != PSI_PIR_SUCCESS) {
        return installRet;
    }
    const auto searchRet = internal::PsiPirProtocolImpl().privacySearch(privacyTarget, elements,
        params, privacySearchResult);
    if (searchRet != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("privacySearch failed in protocol core, ret=%{public}d", searchRet);
        return searchRet;
    }
    const auto symmetricCount = privacySearchResult.symCipherText.has_value() ?
        privacySearchResult.symCipherText->size() : 0;
    PSI_PIR_LOGI("privacySearch success, ret=%{public}d, batches=%{public}zu, symmetricCount=%{public}zu",
        PSI_PIR_SUCCESS, privacySearchResult.fheCipherText.size(), symmetricCount);
    return PSI_PIR_SUCCESS;
}

int32_t internal::ProcessDecryptedSearchResult(const PrivacySearchResult &privacySearchResult,
    const PrivacyProtocol &privacyProtocol, SearchResult &searchResult) noexcept
{
    const auto symmetricCount = privacySearchResult.symCipherText.has_value() ?
        privacySearchResult.symCipherText->size() : 0;
    PSI_PIR_LOGI("getSearchResult enter, batches=%{public}zu, symmetricCount=%{public}zu, "
        "dataSetSize=%{public}u, protocolType=%{public}u", privacySearchResult.fheCipherText.size(), symmetricCount,
        static_cast<uint32_t>(privacyProtocol.dataSetSize), static_cast<uint32_t>(privacyProtocol.protocolType));
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(privacyProtocol, params);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("getSearchResult failed at protocol mapping, ret=%{public}d", ret);
        return ret;
    }
    const auto resultRet = internal::PsiPirProtocolImpl().getSearchResult(privacySearchResult, params, searchResult);
    if (resultRet != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("getSearchResult failed in protocol core, ret=%{public}d", resultRet);
        return resultRet;
    }
    PSI_PIR_LOGI("getSearchResult success, ret=%{public}d, batches=%{public}zu",
        PSI_PIR_SUCCESS, privacySearchResult.fheCipherText.size());
    return PSI_PIR_SUCCESS;
}

int32_t genPrivacyTarget(const TargetInput &input, const PrivacyProtocol &protocol,
                             const OperationIdentity &identity, Bytes &output) noexcept
{
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    return ret == PSI_PIR_SUCCESS ? internal::BgvEncryptor().GenerateTarget(input, params, identity, output) : ret;
}


int32_t getSearchResult(const PrivacySearchResult &input, const PrivacyProtocol &protocol,
                            const OperationIdentity &identity, DecryptionBackend backend,
                            SearchResult &output) noexcept
{
    internal::ProtocolParams params;
    auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    if (backend != DecryptionBackend::SOFT && backend != DecryptionBackend::CCZ) {
        return PSI_PIR_INVALID_ARGUMENT;
    }
    auto decoded = input;
    // Only HE decryption varies by backend; preserve the attached-value frames and batch order.
    for (size_t i = 0; i < input.fheCipherText.size(); ++i) {
        ret = backend == DecryptionBackend::CCZ
                  ? internal::CczDecryptToSlots(input.fheCipherText[i], params, identity, decoded.fheCipherText[i])
                  : internal::BgvDecryptor().DecryptToSlots(input.fheCipherText[i], params, identity, decoded.fheCipherText[i]);
        if (ret != PSI_PIR_SUCCESS) {
            return ret;
        }
    }
    SearchResult result;
    // Shared open-source post-processing for both SOFT and CCZ: never delegate it to CCZ.
    ret = internal::ProcessDecryptedSearchResult(decoded, protocol, result);
    if (ret == PSI_PIR_SUCCESS) {
        output = std::move(result);
    }
    return ret;
}
} // namespace psi_pir
