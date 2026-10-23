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

#ifndef SAF_GUI_FENCE_H
#define SAF_GUI_FENCE_H

#include <cstdint>
#include <string>

#include "iremote_broker.h"

#include "secure_access_fence_type.h"

namespace OHOS::Security::SAF {

/**
 * @brief IPC callback interface for component click decision.
 *
 * A应用实现此接口的 Stub 端,注册时传入其 proxy。
 * 当 ReportComponentClickDecision 被调用时,SAF 通过 proxy 跨进程回调 A应用。
 *
 * Note: AppInfo, ControlDecision, and ComponentClickResult are defined in
 * secure_access_fence_type.h (IDL-generated) to avoid ODR violations.
 */
class IGuiCallback : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.security.saf.IGuiCallback");

    /**
     * @brief Called when a component click decision is reported.
     * @param result The component click decision result.
     */
    virtual void OnComponentClickDecision(const ComponentClickResult &result) = 0;
};

using GuiCallback = sptr<IGuiCallback>;

class SafGuiFence {
public:
    SafGuiFence() = default;
    ~SafGuiFence() = default;

    /**
     * @brief Register a callback for component click decision by session ID.
     *
     * @param sessionId The session ID.
     * @param callback The remote callback to be invoked when ReportComponentClickDecision is called.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t RegisterComponentClickCallback(
        uint64_t sessionId,
        const GuiCallback &callback);

    /**
     * @brief Report component click decision.
     *
     * @param sessionId The session ID.
     * @param result The component click decision result.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t ReportComponentClickDecision(
        uint64_t sessionId,
        const ComponentClickResult &result);

    /**
     * @brief Unregister the callback and destroy the session by session ID.
     *
     * @param sessionId The session ID.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t UnregisterComponentClickCallback(uint64_t sessionId);

    /**
     * @brief Check whether the caller app is allowed to control the target app.
     *
     * @param callerApp The caller app (A app) information.
     * @param targetApp The target app (B app) information.
     * @param decision Output control decision.
     * @return Returns 0 on success, or error code on failure.
     */
    static int32_t CheckControlPermission(
        const AppInfo &callerApp,
        const AppInfo &targetApp,
        ControlDecision &decision);
};

} // namespace OHOS::Security::SAF
#endif // SAF_GUI_FENCE_H
