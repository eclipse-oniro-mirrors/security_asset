/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SAF_CREDENTIAL_TYPE_H
#define SAF_CREDENTIAL_TYPE_H

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS::Security::SAF {

// When the vendor IDL-generated secure_access_fence_type.h has already defined
// these credential types (same namespace, identical field layout), skip the
// duplicate definitions below to avoid redefinition errors. This happens in
// compilation units that include both the asset-side inner API headers and the
// vendor-side IDL-generated headers (e.g. the NAPI layer).
#ifdef OHOS_SECURITY_SAF_SECUREACCESSFENCETYPE_H

// Types ErrInfo, CredentialBatchResult, ModelCredentialContent,
// ModelCredentialInfo, ModelCredentialIndex, ModelCredential, ToolInfo,
// ToolCredentialContent, ToolCredentialInfo and ToolCredential are already
// defined by secure_access_fence_type.h — nothing to do here.

#else

/**
 * @brief Error information for a single failed item in a batch operation.
 *
 * Corresponds to `ErrInfo` in @hms.security.secureAccessFence.d.ts.
 */
struct ErrInfo {
    /** Index of the failed item in the input array. */
    int32_t index = 0;
    /**
     * Error code indicating the failure reason. Always non-zero: an entry only
     * appears here when it failed, so callers can trust `errCode != 0` to mean
     * "this item failed". The value is always one of the base SAF result codes
     * declared in saf_result_defs.h. A missing target entry is reported as
     * SAF_ERR_ARG_INVALID, since for update/delete that is a failure rather
     * than a no-op.
     */
    int32_t errCode = 0;
    /** Error message describing the failure reason. */
    std::string message;
};

/**
 * @brief Result of a batch credential operation.
 *
 * Corresponds to `Result` in @hms.security.secureAccessFence.d.ts.
 */
struct CredentialBatchResult {
    /** Number of items that failed in the batch operation. */
    int32_t failedCount = 0;
    /** Detailed error information for each failed item in the batch. */
    std::vector<ErrInfo> failedErrorInfos;
};

/**
 * @brief Content of the model credential (the secret part).
 *
 * Corresponds to `ModelCredentialContent` in @hms.security.secureAccessFence.d.ts.
 */
struct ModelCredentialContent {
    /** Base URL of the model service endpoint. */
    std::string baseUrl;
    /** Credential value, such as the API KEY used to access the model service. */
    std::string credentialValue;
};

/**
 * @brief Configuration information of the model credential (the metadata part).
 *
 * Corresponds to `ModelCredentialInfo` in @hms.security.secureAccessFence.d.ts.
 */
struct ModelCredentialInfo {
    /** Provider of the model service. */
    std::string provider;
    /** List of model names that this credential applies to. */
    std::vector<std::string> modelList;
    /** Alias of the credential handle, input by user. */
    std::string handleAlias;
    /** Alias of the credential value, input by user. */
    std::string credentialAlias;
    /** Whether the credential is in active state. true: active; false: inactive. */
    bool state = false;
};

/**
 * @brief Index of the model credential, used to locate one credential entry.
 *
 * Corresponds to `ModelCredentialIndex` in @hms.security.secureAccessFence.d.ts.
 */
struct ModelCredentialIndex {
    /** Handle of the credential. */
    std::string credentialHandle;
    /** Unique identifier of the credential entry within the handle. */
    std::string credentialId;
};

/**
 * @brief Model credential entry: content + info + index.
 *
 * Corresponds to `ModelCredential` in @hms.security.secureAccessFence.d.ts.
 * `modelCredentialContent.credentialValue` is empty in
 * PreGetAllModelCredentials results.
 */
struct ModelCredential {
    /** Content of the model credential. */
    ModelCredentialContent modelCredentialContent;
    /** Configuration information of the model credential. */
    ModelCredentialInfo modelCredentialInfo;
    /** Index of the model credential. */
    ModelCredentialIndex modelCredentialIndex;
};

/**
 * @brief Identity of a tool, used as the lookup key by GetToolCredential.
 *
 * The storage identity is agentTokenId + subjectId only.
 */
struct ToolInfo {
    /** Access token ID of the agent. */
    uint64_t agentTokenId = 0;
    /** Unique identifier of the tool, primary key for tool credential lookup. */
    std::string subjectId;
};

/**
 * @brief Tool credential content (single environment variable entry).
 *
 * Corresponds to `ToolCredentialContent` in @hms.security.secureAccessFence.d.ts.
 */
struct ToolCredentialContent {
    /** Environment variable name of the tool. */
    std::string envName;
    /** Credential value of the tool. */
    std::string credentialValue;
};

/**
 * @brief Tool credential information returned by GetToolCredential.
 *
 * Same layout as ToolCredentialContent; the separate name is kept because
 * external modules already depend on it.
 */
struct ToolCredentialInfo {
    /** Environment variable name of the tool. */
    std::string envName;
    /** Credential value of the tool. */
    std::string credentialValue;
};

/**
 * @brief Tool credential entry, binds the tool identity to its credential contents.
 *
 * Corresponds to `ToolCredential` in @hms.security.secureAccessFence.d.ts.
 */
struct ToolCredential {
    /** Unique identifier of the tool, primary key for tool credential lookup. */
    std::string subjectId;
    /** List of tool credential contents (env var entries). */
    std::vector<ToolCredentialContent> toolCredentialContents;
};

#endif  // OHOS_SECURITY_SAF_SECUREACCESSFENCETYPE_H (else: asset-side standalone definitions)

}  // namespace OHOS::Security::SAF
#endif  // SAF_CREDENTIAL_TYPE_H
