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

// Types ErrInfo, CredentialBatchResult, ModelCredentialInfo, ModelCredential,
// ToolInfo, ToolCredentialInfo, ToolCredential are already defined by
// secure_access_fence_type.h — nothing to do here.

#else

/**
 * @brief Error information for a single failed item in a batch operation.
 *
 * Corresponds to `ErrInfo` in @hms.security.secureAccessFence.d.ts (since 26.2.0).
 */
struct ErrInfo {
    /** Index of the failed item in the input array. */
    int32_t index = 0;
    /** Error code indicating the failure reason. */
    int32_t errCode = 0;
    /** Error message describing the failure reason. */
    std::string message;
};

/**
 * @brief Result of a batch credential operation.
 *
 * Corresponds to `Result` in @hms.security.secureAccessFence.d.ts (since 26.2.0).
 */
struct CredentialBatchResult {
    /** Number of items that failed in the batch operation. */
    int32_t failedCount = 0;
    /** Detailed error information for each failed item in the batch. */
    std::vector<ErrInfo> failedErrorInfos;
};

/**
 * @brief Credential information for Model access (query result).
 *
 * Corresponds to `ModelCredentialInfo` in @hms.security.secureAccessFence.d.ts (since 26.2.0).
 */
struct ModelCredentialInfo {
    /** Base URL of the model service endpoint. */
    std::string baseUrl;
    /** Credential value, such as the API KEY used to access the model service. */
    std::string credentialValue;
};

/**
 * @brief Credential configuration information of the agent (full model credential).
 *
 * Corresponds to `ModelCredential` (extends ModelCredentialInfo) in
 * @hms.security.secureAccessFence.d.ts (since 26.2.0).
 *
 * NOTE: IDL does not support `struct extends`, so this struct flattens
 * ModelCredentialInfo's fields plus its own. Optional fields (credentialHandle,
 * credentialAlias) are represented by empty strings when not set.
 */
struct ModelCredential {
    // === Fields inherited from ModelCredentialInfo ===
    std::string baseUrl;
    std::string credentialValue;
    // === ModelCredential's own fields ===
    /** Provider of the model service. */
    std::string provider;
    /** List of model names that this credential applies to. */
    std::vector<std::string> modelList;
    /** Whether the credential is in active state. true: active; false: inactive. */
    bool state = false;
    /**
     * Handle of the credential returned by AddModelCredential, used as a unique
     * identifier for subsequent operations. Empty when not yet assigned.
     */
    std::string credentialHandle;
    /**
     * Alias of the credential returned by AddModelCredential. Empty when not set.
     */
    std::string credentialAlias;
};

/**
 * @brief Identity info of the agent used to manage tool credentials.
 *
 * Corresponds to `ToolInfo` in @hms.security.secureAccessFence.d.ts (since 26.2.0).
 * The d.ts `bigint` type for agentTokenId is mapped to uint64_t.
 */
struct ToolInfo {
    /** Access token ID of the agent (d.ts bigint -> uint64_t). */
    uint64_t agentTokenId = 0;
    /** Unique identifier of the tool, primary key for tool credential lookup. */
    std::string subjectId;
    /** Account ID of the agent owner. Empty when not set. */
    std::string accountId;
    /** Sub-agent ID, distinguishes sub-agents under the same agent. Empty when not set. */
    std::string subAgentId;
};

/**
 * @brief Tool credential information (single environment variable entry).
 *
 * Corresponds to `ToolCredentialInfo` in @hms.security.secureAccessFence.d.ts (since 26.2.0).
 */
struct ToolCredentialInfo {
    /** Environment variable name of the tool. */
    std::string envName;
    /** Credential value of the tool. */
    std::string credentialValue;
};

/**
 * @brief Tool credential entry, binds agent identity info to its credential values.
 *
 * Corresponds to `ToolCredential` in @hms.security.secureAccessFence.d.ts (since 26.2.0).
 */
struct ToolCredential {
    /** Identity info of the tool. */
    ToolInfo toolInfo;
    /** List of tool credential infos (env var entries). */
    std::vector<ToolCredentialInfo> toolCredentialInfos;
};

#endif  // OHOS_SECURITY_SAF_SECUREACCESSFENCETYPE_H (else: asset-side standalone definitions)

}  // namespace OHOS::Security::SAF
#endif  // SAF_CREDENTIAL_TYPE_H
