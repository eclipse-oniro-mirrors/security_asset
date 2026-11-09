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

#ifndef PRIVACY_PROTOCOL_INTERNAL_PROVIDER_REGISTRY_H
#define PRIVACY_PROTOCOL_INTERNAL_PROVIDER_REGISTRY_H

#include "crypto_provider.h"

#include <memory>

namespace OHOS::Security::PrivacyComputation::internal {

// Internal integration hooks; never exposed through installed public headers.
// Passing nullptr clears an override. Hash, random and pir value crypto then fall
// back to the protocol's OpenSSL implementations; HE uses an installed default factory,
// or remains unavailable if no factory has been installed.
void SetHashProvider(std::shared_ptr<const IHashProvider> provider) noexcept;
void SetRandomSource(std::shared_ptr<IRandomSource> source) noexcept;
void SetHEEngine(std::shared_ptr<const IHEEngine> engine) noexcept;
// Installs a process-lifetime default once, without modifying explicit overrides.
using HEEngineFactory = std::shared_ptr<const IHEEngine> (*)() noexcept;
bool InstallDefaultHEEngineFactory(HEEngineFactory factory) noexcept;
void SetPirValueCrypto(std::shared_ptr<const IPirValueCrypto> crypto) noexcept;

std::shared_ptr<const IHashProvider> GetHashProvider() noexcept;
std::shared_ptr<IRandomSource> GetRandomSource() noexcept;
std::shared_ptr<const IHEEngine> GetHEEngine() noexcept;
std::shared_ptr<const IPirValueCrypto> GetPirValueCrypto() noexcept;

} // namespace OHOS::Security::PrivacyComputation::internal

#endif // PRIVACY_PROTOCOL_INTERNAL_PROVIDER_REGISTRY_H
