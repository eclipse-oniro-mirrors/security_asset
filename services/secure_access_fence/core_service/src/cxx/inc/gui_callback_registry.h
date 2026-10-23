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

#ifndef GUI_CALLBACK_REGISTRY_H
#define GUI_CALLBACK_REGISTRY_H

#include <cstdint>
#include <map>
#include <mutex>

#include "iremote_broker.h"
#include "refbase.h"

#include "saf_gui_fence.h"
#include "saf_result_code.h"

namespace OHOS::Security::SAF {

/**
 * @brief Registry for component click decision callbacks (IPC 506/507/508).
 *
 * Maintains a mapping from sessionId to IGuiCallback proxy. Used by the Service
 * layer to route ReportComponentClickDecision (IPC 507) back to the A app that
 * registered the callback via RegisterComponentClickCallback (IPC 506).
 *
 * Thread safety: std::recursive_mutex protects all operations.
 *
 * Lifecycle: lives as long as the Service process. The SA idle-unload check
 * queries IsEmpty() - the SA will not be unloaded while the registry is non-empty.
 */
class GuiCallbackRegistry {
public:
    /**
     * @brief Get the singleton instance.
     * @return The singleton instance reference.
     */
    static GuiCallbackRegistry &GetInstance();

    /**
     * @brief Register a callback for a session ID.
     * @param sessionId The session ID identifying the feedback channel.
     * @param callback The IGuiCallback proxy from the A app.
     * @return SAF_SUCCESS on success, SAF_ERR_GUI_SESSION_ALREADY_EXIST if sessionId already registered,
     *         SAF_ERR_NULL_PTR if callback is null.
     */
    int32_t Register(uint64_t sessionId, const GuiCallback &callback);

    /**
     * @brief Unregister a callback by session ID.
     * @param sessionId The session ID to remove.
     * @return SAF_SUCCESS on success, SAF_ERR_GUI_SESSION_NOT_FOUND if sessionId not found.
     */
    int32_t Unregister(uint64_t sessionId);

    /**
     * @brief Find a callback by session ID.
     * @param sessionId The session ID to look up.
     * @return The callback proxy, or nullptr if not found.
     */
    GuiCallback Find(uint64_t sessionId) const;

    /**
     * @brief Check whether the registry is empty (no callbacks registered).
     *
     * Used by the SA idle-unload check: if non-empty, the SA must not be unloaded.
     *
     * @return true if empty, false otherwise.
     */
    bool IsEmpty() const;

    /**
     * @brief Clear all registered callbacks. For testing and SA destruction only.
     */
    void Clear();

    /**
     * @brief Reset the singleton state for unit test isolation.
     * Intended for unit test SetUp/TearDown only; must not be called from production code.
     */
    static void ResetForTesting();

private:
    GuiCallbackRegistry() = default;
    ~GuiCallbackRegistry() = default;
    GuiCallbackRegistry(const GuiCallbackRegistry &) = delete;
    GuiCallbackRegistry &operator=(const GuiCallbackRegistry &) = delete;

    mutable std::recursive_mutex mutex_;
    std::map<uint64_t, GuiCallback> registry_;
};

} // namespace OHOS::Security::SAF

#endif // GUI_CALLBACK_REGISTRY_H
