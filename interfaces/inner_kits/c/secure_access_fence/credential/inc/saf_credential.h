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

#include <string>
#include <vector>

#include "saf_credential_type.h"

namespace OHOS::Security::SAF {

/**
 * @brief Inner API for the 26.2.0 Credential APIs.
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
     * d.ts: addModelCredential(modelCredential: ModelCredential): Promise<string>
     *
     * @param modelCredential The model credential to add. `provider`, `baseUrl`
     *        and `credentialValue` are required.
     * @param credentialHandle [out] The handle of the added credential, used as
     *        the unique identifier for subsequent operations on this credential.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_NOT_SYSTEM_APP (202): caller is not a system app.
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: only system app (202). No @permission required.
     */
    int32_t AddModelCredential(const ModelCredential &modelCredential,
                               std::string &credentialHandle);

    /**
     * @brief Batch delete model credentials.
     *
     * d.ts: deleteModelCredential(modelCredentials: ModelCredential[]): Promise<Result>
     *
     * @param modelCredentials The model credentials to delete, identified by
     *        `credentialHandle`.
     * @param result [out] Batch result (failed count + per-item error info).
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201): missing ACCESS_SECURE_FENCE.
     *         - SAF_ERR_NOT_SYSTEM_APP (202): caller is not a system app.
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
     * Each source credential (identified by `credentialHandle`) is replaced by
     * the corresponding destination credential. The two arrays must be the same
     * length.
     *
     * @param sourceModelCredentials Source credentials to update, identified by
     *        `credentialHandle`.
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
     * @brief Query the active model credential info by handle.
     *
     * d.ts: getActiveModelCredential(credentialHandle: string): Promise<ModelCredentialInfo>
     *
     * @param credentialHandle Handle of the credential to query.
     * @param modelCredentialInfo [out] The credential info (baseUrl + credentialValue).
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t GetActiveModelCredential(const std::string &credentialHandle,
                                     ModelCredentialInfo &modelCredentialInfo);

    /**
     * @brief Query all model credentials.
     *
     * d.ts: getAllModelCredentials(): Promise<ModelCredential[]>
     *
     * @param modelCredentials [out] All stored model credentials.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t GetAllModelCredentials(std::vector<ModelCredential> &modelCredentials);

    // ===== Tool Credential API =====

    /**
     * @brief Batch add tool credentials.
     *
     * d.ts: addToolCredential(toolCredentials: ToolCredential[]): Promise<Result>
     *
     * @param toolCredentials Tool credentials to add. For each entry,
     *        `toolInfo.subjectId` and `toolCredentialInfos` are required.
     * @param result [out] Batch result.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: only system app (202). No @permission required.
     */
    int32_t AddToolCredential(const std::vector<ToolCredential> &toolCredentials,
                              CredentialBatchResult &result);

    /**
     * @brief Batch update tool credentials.
     *
     * d.ts: updateToolCredential(source, dest): Promise<Result>
     *
     * Each source tool credential (identified by `ToolInfo`) is replaced by the
     * corresponding destination tool credential. The two arrays must be the
     * same length.
     *
     * @param sourceToolCredentials Source tool credentials to update.
     * @param destToolCredentials Destination tool credentials with new values.
     * @param result [out] Batch result.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_ARG_INVALID: src/dst length mismatch or empty.
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t UpdateToolCredential(const std::vector<ToolCredential> &sourceToolCredentials,
                                 const std::vector<ToolCredential> &destToolCredentials,
                                 CredentialBatchResult &result);

    /**
     * @brief Batch delete tool credentials.
     *
     * d.ts: deleteToolCredential(toolInfos: ToolInfo[]): Promise<Result>
     *
     * @param toolInfos Identity info of the tool credentials to delete. Each
     *        entry is matched by the combination of
     *        {agentTokenId, subjectId, accountId, subAgentId}.
     * @param result [out] Batch result.
     * @return SAF_SUCCESS on success; otherwise an error code:
     *         - SAF_ERR_PERMISSION_DENIED (201).
     *         - SAF_ERR_NOT_SYSTEM_APP (202).
     *         - SAF_ERR_SERVICE_UNAVAILABLE: SA service unavailable.
     *
     * Authorization: ACCESS_SECURE_FENCE (201) + system app (202).
     */
    int32_t DeleteToolCredential(const std::vector<ToolInfo> &toolInfos,
                                 CredentialBatchResult &result);

    /**
     * @brief Query tool credential info by agent identity.
     *
     * d.ts: getToolCredential(toolInfo: ToolInfo): Promise<ToolCredentialInfo[]>
     *
     * @param toolInfo Identity info of the tool to query. `subjectId` is required.
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
