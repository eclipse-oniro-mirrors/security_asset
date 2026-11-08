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
 * @brief Register a Datashare observer for the agent_app_control_status key
 * on the given user's secure settings.
 *
 * The observed key is fixed internally (agent_app_control_status); it is not
 * exposed as a parameter because there is exactly one such key, and exposing
 * it would invite future misuse with URI-reserved characters.
 *
 * @param userId The OS account ID for per-user settings.
 * @param observer The observer to register.
 * @return true on success, false on failure.
 */
bool RegisterSettingsObserver(int32_t userId, const sptr<SettingsDataObserver> &observer);

/**
 * @brief Unregister a previously registered Datashare observer.
 *
 * @param userId The OS account ID that was used during registration.
 * @param observer The observer to unregister.
 * @return true on success, false on failure.
 */
bool UnregisterSettingsObserver(int32_t userId, const sptr<SettingsDataObserver> &observer);
} // namespace OHOS::Security::SAF

#endif
