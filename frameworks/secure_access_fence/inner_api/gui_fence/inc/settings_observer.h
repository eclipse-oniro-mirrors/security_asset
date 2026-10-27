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

#ifndef SETTINGS_OBSERVER_H
#define SETTINGS_OBSERVER_H

#include <functional>
#include <string>

#include "data_ability_observer_stub.h"

namespace OHOS::Security::SAF {
/**
 * @brief Observer for settings data changes.
 *
 * When the observed settings key changes, OnChange invokes the registered callback.
 *
 */

    
class SettingsDataObserver : public AAFwk::DataAbilityObserverStub {
public:
    using OnChangeCallback = std::function<void()>;

    explicit SettingsDataObserver(OnChangeCallback callback) : callback_(std::move(callback)) {}
    ~SettingsDataObserver() override = default;
    void OnChange() override;
    
private:
    OnChangeCallback callback_;
};

/**
 * @brief Register a Datashare observer for a settings key on the user's secure settings.
 *
 * @param key The settings key to observe.
 * @param userId The OS account ID for per-user settings.
 * @param observer The observer to register.
 * @return true on success, false on failure.
 */
bool RegisterSettingsObserver(const std::string &key, int32_t userId,
    const sptr<SettingsDataObserver> &observer);
} // namespace OHOS::Security::SAF

#endif