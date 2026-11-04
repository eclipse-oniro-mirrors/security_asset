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

#ifndef SAF_CREDENTIAL_H
#define SAF_CREDENTIAL_H

#include <cstdint>
#include <string>
#include <vector>

#include "saf_credential_type.h"

namespace OHOS::Security::SAF {

/**
 * @brief Inner API for the Credential APIs.
 *
 * Architecture:
 *   Each call goes through the access_fence SA (66532) via direct IPC, using
 *   the asset-side generated ISecureAccessFence proxy (same pattern as
 *   SafAgentFence). The SA service layer forwards to the credential
 *   management implementation (DiaAdapter -> DIA).
 *
 * The class is stateless; instances are cheap to construct.
 */
class SafCredential {
public:
    SafCredential() = default;
    ~SafCredential() = default;

    // ===== Model Credential API =====

    /**
     * @brief Add a model credential.
     *
     * d.ts: addModelCredential(content, info): Promise<ModelCredentialIndex>
     *
     * @param modelCredentialContent Content of the credential to add.
     *        `baseUrl` and `credentialValue` are required.
     * @param modelCredentialInfo Configuration of the credential to add.
     *        `provider` is required; `handleAlias` is required when creating a
     *        new handle.
     * @param credentialHandle [out] Handle of the added credential.
     * @param credentialId [out] Unique identifier of the added entry within the
     *        handle.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201): missing ACCESS_SECURE_FENCE.
     *         - SAF_ERR_NOT_SYSTEM_APP (202): caller is not a system app.
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t AddModelCredential(const ModelCredentialContent &modelCredentialContent,
                               const ModelCredentialInfo &modelCredentialInfo,
                               std::string &credentialHandle, std::string &credentialId);

    /**
     * @brief Batch delete model credentials.
     *
     * d.ts: deleteModelCredential(modelCredentials): Promise<Result>
     *
     * @param modelCredentials The model credentials to delete, identified by
     *        `modelCredentialIndex.credentialHandle` and `credentialId`.
     * @param result [out] Batch result (failed count + per-item error info).
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t DeleteModelCredential(const std::vector<ModelCredential> &modelCredentials,
                                  CredentialBatchResult &result);

    /**
     * @brief Batch update model credentials.
     *
     * d.ts: updateModelCredential(source, dest): Promise<Result>
     *
     * Each source credential (identified by index) is replaced by the
     * corresponding destination credential. The two arrays must be the same
     * length.
     *
     * @param sourceModelCredentials Source credentials to update.
     * @param destModelCredentials Destination credentials with new values.
     * @param result [out] Batch result.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_ARG_INVALID: src/dst length mismatch or empty.
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t UpdateModelCredential(const std::vector<ModelCredential> &sourceModelCredentials,
                                  const std::vector<ModelCredential> &destModelCredentials,
                                  CredentialBatchResult &result);

    /**
     * @brief Query the active model credential content by handle and model.
     *
     * d.ts: getActiveModelCredential(handle, model): Promise<ModelCredentialContent>
     *
     * @param credentialHandle Handle of the credential to query.
     * @param model Model name to filter; when non-empty the active credential
     *        must list it, otherwise no content is returned.
     * @param modelCredentialContent [out] The credential content
     *        (baseUrl + credentialValue).
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t GetActiveModelCredential(const std::string &credentialHandle, const std::string &model,
                                     ModelCredentialContent &modelCredentialContent);

    /**
     * @brief Query all model credentials without their credential values.
     *
     * d.ts: preGetAllModelCredentials(): Promise<ModelCredential[]>
     *
     * @param modelCredentials [out] All stored model credentials, with
     *        `modelCredentialContent.credentialValue` empty.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t PreGetAllModelCredentials(std::vector<ModelCredential> &modelCredentials);

    /**
     * @brief Query one model credential value by index.
     *
     * d.ts: getModelCredentialValue(index): Promise<string>
     *
     * @param modelCredentialIndex Index of the credential to query.
     * @param credentialValue [out] The credential value (plaintext).
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t GetModelCredentialValue(const ModelCredentialIndex &modelCredentialIndex,
                                    std::string &credentialValue);

    // ===== Tool Credential API =====

    /**
     * @brief Batch add tool credentials.
     *
     * d.ts: addToolCredential(toolCredentials): Promise<Result>
     *
     * @param toolCredentials Tool credentials to add. For each entry,
     *        `subjectId` and `toolCredentialContents` are required.
     * @param result [out] Batch result.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: system app (202) only; the agent identity is taken from
     * the caller's access token ID by the service.
     */
    int32_t AddToolCredential(const std::vector<ToolCredential> &toolCredentials,
                              CredentialBatchResult &result);

    /**
     * @brief Batch update tool credentials.
     *
     * d.ts: updateToolCredential(toolCredentials): Promise<Result>
     *
     * Each entry updates the secret values of its existing env names under the
     * caller's agent identity.
     *
     * @param toolCredentials Tool credentials to update.
     * @param result [out] Batch result.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t UpdateToolCredential(const std::vector<ToolCredential> &toolCredentials,
                                 CredentialBatchResult &result);

    /**
     * @brief Batch delete tool credentials.
     *
     * d.ts: deleteToolCredential(toolCredentials): Promise<Result>
     *
     * Each entry deletes the listed env names under the caller's agent
     * identity.
     *
     * @param toolCredentials Tool credentials to delete.
     * @param result [out] Batch result.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t DeleteToolCredential(const std::vector<ToolCredential> &toolCredentials,
                                 CredentialBatchResult &result);

    /**
     * @brief Query tool credentials by tool identity.
     *
     * Inner API only. The d.ts exposes no query API for tool credentials.
     *
     * @param toolInfo Identity of the tool to query. `agentTokenId` is supplied
     *        by the caller because this path is invoked by another system
     *        service rather than by the owning app.
     * @param toolCredentialInfos [out] The tool credential info list
     *        (environment variable entries) for the given tool.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t GetToolCredential(const ToolInfo &toolInfo,
                              std::vector<ToolCredentialInfo> &toolCredentialInfos);
};

}  // namespace OHOS::Security::SAF
#endif  // SAF_CREDENTIAL_H
