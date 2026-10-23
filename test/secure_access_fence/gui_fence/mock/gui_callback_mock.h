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

#ifndef GUI_CALLBACK_MOCK_H
#define GUI_CALLBACK_MOCK_H

#include "saf_gui_fence.h"

namespace OHOS::Security::SAF::Test {

/**
 * @brief Mock implementation of IGuiCallback for unit testing.
 *
 * Records whether OnComponentClickDecision was called and stores the last result.
 * Does not involve real IPC - AsObject() returns nullptr.
 */
class GuiCallbackMock : public IGuiCallback {
public:
    GuiCallbackMock() = default;
    ~GuiCallbackMock() override = default;

    void OnComponentClickDecision(const ComponentClickResult &result) override
    {
        lastResult_ = result;
        called_ = true;
    }

    bool WasCalled() const { return called_; }
    const ComponentClickResult &GetLastResult() const { return lastResult_; }
    void Reset()
    {
        called_ = false;
        lastResult_ = {};
    }

    sptr<IRemoteObject> AsObject() override { return nullptr; }

private:
    bool called_ = false;
    ComponentClickResult lastResult_ {};
};

} // namespace OHOS::Security::SAF::Test

#endif // GUI_CALLBACK_MOCK_H
