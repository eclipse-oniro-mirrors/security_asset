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

// Test-only stand-in for the IDL-generated ISecureAccessFence interface.
//
// saf_credential.cpp is compiled directly into the saf_credential_ipc_test
// binary. This header is picked up instead of the real IDL-generated
// isecure_access_fence.h because the test target puts mock/ first on the
// include path. It deliberately keeps only the 10 credential methods used by
// the credential inner API and replaces the IPC/refcount machinery with a
// minimal non-owning sptr so that the client-side control flow can be driven
// deterministically from a unit test (no system ability, no binder).
//
// Nothing in this header is shipped: it lives under test/ and is referenced
// only by the saf_credential_ipc_test BUILD.gn target.

#ifndef MOCK_ISECURE_ACCESS_FENCE_H
#define MOCK_ISECURE_ACCESS_FENCE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "saf_credential_type.h"

namespace OHOS {

/**
 * @brief Minimal IRemoteObject placeholder.
 *
 * The real base class is a refcounted binder object; the credential client
 * only needs a common base pointer to hand around between GetProxy() and
 * iface_cast(). A virtual destructor keeps the downcast well-formed.
 */
class IRemoteObject {
public:
    virtual ~IRemoteObject() = default;
};

/**
 * @brief Minimal non-owning smart pointer, API-compatible with OHOS::sptr for
 *        the subset used by saf_credential.cpp.
 *
 * No reference counting is performed: the test fixture owns every mock object
 * for the duration of a test case, so a raw pointer view is sufficient and
 * keeps the mock free of RefBase/atomic dependencies.
 */
template <typename T>
class sptr {
public:
    sptr() = default;
    // Implicit nullptr conversion is required: saf_credential.cpp's
    // IF_TRUE_LOGE_RETURN_NULL macro does `return nullptr;`.
    sptr(std::nullptr_t) {}  // NOLINT
    explicit sptr(T *object) : object_(object) {}

    sptr(const sptr &other) = default;
    sptr &operator=(const sptr &other) = default;

    template <typename U>
    sptr(const sptr<U> &other) : object_(static_cast<T *>(other.GetRefPtr())) {}  // NOLINT

    template <typename U>
    sptr &operator=(const sptr<U> &other)
    {
        object_ = static_cast<T *>(other.GetRefPtr());
        return *this;
    }

    T *GetRefPtr() const
    {
        return object_;
    }

    T *operator->() const
    {
        return object_;
    }

    T &operator*() const
    {
        return *object_;
    }

    explicit operator bool() const
    {
        return object_ != nullptr;
    }

    bool operator==(std::nullptr_t) const
    {
        return object_ == nullptr;
    }

    bool operator!=(std::nullptr_t) const
    {
        return object_ != nullptr;
    }

private:
    T *object_ = nullptr;
};

namespace Security::SAF {

/**
 * @brief Credential subset of the SecureAccessFence IPC interface.
 *
 * Signatures mirror the generated proxy one-for-one (argument order and
 * reference outputs), so saf_credential.cpp compiles unchanged against it.
 */
class ISecureAccessFence : public IRemoteObject {
public:
    ~ISecureAccessFence() override = default;

    virtual int32_t AddModelCredential(const ModelCredentialContent &modelCredentialContent,
                                       const ModelCredentialInfo &modelCredentialInfo,
                                       std::string &credentialHandle, std::string &credentialId,
                                       int32_t &resultCode) = 0;
    virtual int32_t DeleteModelCredential(const std::vector<ModelCredential> &modelCredentials,
                                          CredentialBatchResult &result, int32_t &resultCode) = 0;
    virtual int32_t UpdateModelCredential(const std::vector<ModelCredential> &sourceModelCredentials,
                                          const std::vector<ModelCredential> &destModelCredentials,
                                          CredentialBatchResult &result, int32_t &resultCode) = 0;
    virtual int32_t GetActiveModelCredential(const std::string &credentialHandle, const std::string &model,
                                             ModelCredentialContent &modelCredentialContent,
                                             int32_t &resultCode) = 0;
    virtual int32_t PreGetAllModelCredentials(std::vector<ModelCredential> &modelCredentials,
                                             int32_t &resultCode) = 0;
    virtual int32_t GetModelCredentialValue(const ModelCredentialIndex &modelCredentialIndex,
                                            std::string &credentialValue, int32_t &resultCode) = 0;
    virtual int32_t AddToolCredential(const std::vector<ToolCredential> &toolCredentials,
                                      CredentialBatchResult &result, int32_t &resultCode) = 0;
    virtual int32_t UpdateToolCredential(const std::vector<ToolCredential> &toolCredentials,
                                         CredentialBatchResult &result, int32_t &resultCode) = 0;
    virtual int32_t DeleteToolCredential(const std::vector<ToolCredential> &toolCredentials,
                                         CredentialBatchResult &result, int32_t &resultCode) = 0;
    virtual int32_t GetToolCredential(const ToolInfo &toolInfo,
                                      std::vector<ToolCredentialInfo> &toolCredentialInfos,
                                      int32_t &resultCode) = 0;
};

/**
 * @brief Same blind downcast the real OHOS::iface_cast performs.
 *
 * The mock fixture always stores an ISecureAccessFence-derived object in the
 * IRemoteObject pointer returned by samgr, so the downcast is valid.
 */
template <typename T>
inline sptr<T> iface_cast(const sptr<IRemoteObject> &object)
{
    return sptr<T>(static_cast<T *>(object.GetRefPtr()));
}

}  // namespace Security::SAF
}  // namespace OHOS

#endif  // MOCK_ISECURE_ACCESS_FENCE_H