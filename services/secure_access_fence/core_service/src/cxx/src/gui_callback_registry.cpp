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
    registry_[sessionId] = callback;
    return SAF_SUCCESS;
}

int32_t GuiCallbackRegistry::Unregister(uint64_t sessionId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = registry_.find(sessionId);
    if (it == registry_.end()) {
        return SAF_ERR_GUI_SESSION_NOT_FOUND;
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
    registry_.clear();
}

void GuiCallbackRegistry::ResetForTesting()
{
    GetInstance().Clear();
}

} // namespace OHOS::Security::SAF
