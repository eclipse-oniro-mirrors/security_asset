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

#include "provider_registry.h"

#include "openssl_crypto_provider.h"
#include "psi_pir_log.h"

#include <atomic>
#include <utility>

namespace psi_pir::internal {
namespace {

std::shared_ptr<const IHashProvider> g_hashProvider;
std::shared_ptr<IRandomSource> g_randomSource;
std::shared_ptr<const IHEEngine> g_heEngine;
std::atomic<HEEngineFactory> g_defaultHEFactory{nullptr};
std::shared_ptr<const IPirValueCrypto> g_pirValueCrypto;

} // namespace

void SetHashProvider(std::shared_ptr<const IHashProvider> provider) noexcept
{
    LOGI("hash provider override changed, custom=%{public}d", provider ? 1 : 0);
    std::atomic_store_explicit(&g_hashProvider, std::move(provider), std::memory_order_release);
}

void SetRandomSource(std::shared_ptr<IRandomSource> source) noexcept
{
    LOGI("random source override changed, custom=%{public}d", source ? 1 : 0);
    std::atomic_store_explicit(&g_randomSource, std::move(source), std::memory_order_release);
}

void SetHEEngine(std::shared_ptr<const IHEEngine> engine) noexcept
{
    LOGI("HE engine changed, registered=%{public}d", engine ? 1 : 0);
    std::atomic_store_explicit(&g_heEngine, std::move(engine), std::memory_order_release);
}

void SetPirValueCrypto(std::shared_ptr<const IPirValueCrypto> crypto) noexcept
{
    LOGI("pir value crypto override changed, custom=%{public}d", crypto ? 1 : 0);
    std::atomic_store_explicit(&g_pirValueCrypto, std::move(crypto), std::memory_order_release);
}

std::shared_ptr<const IHashProvider> GetHashProvider() noexcept
{
    const auto provider = std::atomic_load_explicit(&g_hashProvider, std::memory_order_acquire);
    if (provider) {
        return provider;
    }
    return GetOpenSslHashProvider();
}

std::shared_ptr<IRandomSource> GetRandomSource() noexcept
{
    const auto source = std::atomic_load_explicit(&g_randomSource, std::memory_order_acquire);
    if (source) {
        return source;
    }
    return GetOpenSslRandomSource();
}

std::shared_ptr<const IHEEngine> GetHEEngine() noexcept
{
    const auto engine = std::atomic_load_explicit(&g_heEngine, std::memory_order_acquire);
    if (engine) {
        return engine;
    }
    const auto factory = g_defaultHEFactory.load(std::memory_order_acquire);
    return factory == nullptr ? nullptr : factory();
}

bool InstallDefaultHEEngineFactory(HEEngineFactory factory) noexcept
{
    if (factory == nullptr) {
        return false;
    }
    HEEngineFactory expected = nullptr;
    return g_defaultHEFactory.compare_exchange_strong(expected, factory, std::memory_order_acq_rel,
                                                      std::memory_order_acquire) ||
           expected == factory;
}

std::shared_ptr<const IPirValueCrypto> GetPirValueCrypto() noexcept
{
    const auto crypto = std::atomic_load_explicit(&g_pirValueCrypto, std::memory_order_acquire);
    if (crypto) {
        return crypto;
    }
    return GetOpenSslPirValueCrypto();
}

} // namespace psi_pir::internal
