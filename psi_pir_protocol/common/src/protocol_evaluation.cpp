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

#include "protocol_evaluation.h"
#include "psi_pir_error_code.h"
#include "psi_pir_log.h"
#include "soft_he_engine.h"
#include <new>
#include <utility>

namespace psi_pir::internal {
namespace {
// Compatibility for existing injected providers; native HiFHE uses an object-based handle instead.
class ProviderEvaluation final : public HeEvaluation {
public:
    ProviderEvaluation(const IHEEngine &engine, const Bytes &ciphertext) : engine_(engine), current_(ciphertext) {}

    int32_t AddPlain(const Slots &plaintext) noexcept override
    {
        if (status_ != PSI_PIR_SUCCESS) {
            return status_;
        }
        Bytes result;
        const auto ret = engine_.addPlain(current_, plaintext, result);
        return CommitResult(ret, std::move(result));
    }

    int32_t MultiplyPlain(const Slots &plaintext) noexcept override
    {
        if (status_ != PSI_PIR_SUCCESS) {
            return status_;
        }
        Bytes result;
        const auto ret = engine_.multiplyPlain(current_, plaintext, result);
        return CommitResult(ret, std::move(result));
    }

    int32_t Rerandomize() noexcept override
    {
        if (status_ != PSI_PIR_SUCCESS) {
            return status_;
        }
        Bytes result;
        const auto ret = engine_.RandCipherText(current_, result);
        return CommitResult(ret, std::move(result));
    }

    int32_t Serialize(Bytes &output) const noexcept override
    {
        if (status_ == PSI_PIR_SUCCESS) {
            output = current_;
        }
        return status_;
    }

private:
    int32_t CommitResult(int32_t ret, Bytes result) noexcept
    {
        status_ = ret;
        if (ret == PSI_PIR_SUCCESS && result.empty()) {
            status_ = PSI_PIR_HE_ERROR;
        }
        if (status_ != PSI_PIR_SUCCESS) {
            PSI_PIR_LOGE("Provider evaluation failed, ret=%{public}d", status_);
            return status_;
        }
        current_ = std::move(result);
        return PSI_PIR_SUCCESS;
    }

    const IHEEngine &engine_;
    Bytes current_;
    int32_t status_ = PSI_PIR_SUCCESS;
};
} // namespace

int32_t CreateHeEvaluation(const IHEEngine &engine, const Bytes &ciphertext,
                           std::unique_ptr<HeEvaluation> &evaluation) noexcept
{
    if (ciphertext.empty()) {
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (&engine == GetSoftHeEngine().get()) {
        return CreateSoftHeEvaluation(ciphertext, evaluation);
    }
    auto result = std::unique_ptr<HeEvaluation>(new (std::nothrow) ProviderEvaluation(engine, ciphertext));
    if (!result) {
        PSI_PIR_LOGE("Allocate provider evaluation failed");
        return PSI_PIR_INTERNAL_ERROR;
    }
    evaluation = std::move(result);
    return PSI_PIR_SUCCESS;
}
} // namespace psi_pir::internal
