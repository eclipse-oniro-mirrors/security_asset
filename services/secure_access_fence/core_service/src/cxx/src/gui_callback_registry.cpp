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

#include "gui_callback_registry.h"

namespace OHOS::Security::SAF {

void GuiCallbackDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    GuiCallbackRegistry::GetInstance().RemoveByDeath(sessionId_);
}

GuiCallbackRegistry &GuiCallbackRegistry::GetInstance()
{
    static GuiCallbackRegistry instance;
    return instance;
}

int32_t GuiCallbackRegistry::Register(uint64_t sessionId, const GuiCallback &callback)
{
    if (callback == nullptr) {
        return SAF_ERR_NULL_PTR;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (registry_.find(sessionId) != registry_.end()) {
        return SAF_ERR_GUI_SESSION_ALREADY_EXIST;
    }

    // Register a DeathRecipient so the entry is cleaned up if the caller process dies.
    auto remoteObj = callback->AsObject();
    if (remoteObj == nullptr) {
        return SAF_ERR_NULL_PTR;
    }
    auto deathRecipient = sptr<GuiCallbackDeathRecipient>::MakeSptr(sessionId);
    if (deathRecipient == nullptr) {
        return SAF_ERR_NULL_PTR;
    }
    if (!remoteObj->AddDeathRecipient(deathRecipient)) {
        return SAF_ERR_NULL_PTR;
    }
    registry_[sessionId] = callback;
    deathRecipients_[sessionId] = deathRecipient;
    return SAF_SUCCESS;
}

int32_t GuiCallbackRegistry::Unregister(uint64_t sessionId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = registry_.find(sessionId);
    if (it == registry_.end()) {
        return SAF_ERR_GUI_SESSION_NOT_FOUND;
    }

    // Remove the DeathRecipient before erasing the callback.
    auto drIt = deathRecipients_.find(sessionId);
    if (drIt != deathRecipients_.end()) {
        auto remoteObj = it->second->AsObject();
        if (remoteObj != nullptr) {
            remoteObj->RemoveDeathRecipient(drIt->second);
        }
        deathRecipients_.erase(drIt);
    }
    registry_.erase(it);
    return SAF_SUCCESS;
}

GuiCallback GuiCallbackRegistry::Find(uint64_t sessionId) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = registry_.find(sessionId);
    if (it == registry_.end()) {
        return nullptr;
    }
    return it->second;
}

bool GuiCallbackRegistry::IsEmpty() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return registry_.empty();
}

void GuiCallbackRegistry::Clear()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    // Remove DeathRecipients before clearing.
    for (auto &pair : registry_) {
        auto drIt = deathRecipients_.find(pair.first);
        if (drIt != deathRecipients_.end()) {
            auto remoteObj = pair.second->AsObject();
            if (remoteObj != nullptr) {
                remoteObj->RemoveDeathRecipient(drIt->second);
            }
        }
    }
    registry_.clear();
    deathRecipients_.clear();
}

void GuiCallbackRegistry::ResetForTesting()
{
    GetInstance().Clear();
}

void GuiCallbackRegistry::DropDeathRecipientForTesting(uint64_t sessionId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    // Test-only seam: production code keeps registry_ and deathRecipients_ in
    // sync, so the fallbacks tolerating a missing death-recipient record in
    // Unregister/Clear cannot be reached through the public API. Erasing only
    // from deathRecipients_ here desyncs the two maps on purpose. Deliberately
    // branch-free so the seam itself adds no uncovered branches.
    deathRecipients_.erase(sessionId);
}

void GuiCallbackRegistry::RemoveByDeath(uint64_t sessionId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = registry_.find(sessionId);
    if (it == registry_.end()) {
        return;
    }
    // The remote object is already dead, no need to call RemoveDeathRecipient.
    registry_.erase(it);
    deathRecipients_.erase(sessionId);
}

} // namespace OHOS::Security::SAF
