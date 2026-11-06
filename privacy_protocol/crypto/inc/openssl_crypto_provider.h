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

#ifndef PRIVACY_PROTOCOL_OPENSSL_CRYPTO_PROVIDER_H
#define PRIVACY_PROTOCOL_OPENSSL_CRYPTO_PROVIDER_H

#include "crypto_provider.h"

#include <memory>

namespace PrivacyComputation::internal {

// Built-in implementations used when no integration override is registered.
// These factories and provider interfaces are internal; business callers only
// see the two public privacy_protocol operations.
std::shared_ptr<const IHashProvider> GetOpenSslHashProvider() noexcept;
std::shared_ptr<IRandomSource> GetOpenSslRandomSource() noexcept;
std::shared_ptr<const IPirValueCrypto> GetOpenSslPirValueCrypto() noexcept;

} // namespace PrivacyComputation::internal

#endif // PRIVACY_PROTOCOL_OPENSSL_CRYPTO_PROVIDER_H
