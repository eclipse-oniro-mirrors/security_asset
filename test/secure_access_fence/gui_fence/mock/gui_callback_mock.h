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

#include <cstdint>
#include <vector>

#include "iremote_object.h"
#include "message_option.h"
#include "message_parcel.h"
#include "refbase.h"
#include "saf_gui_fence.h"

namespace OHOS::Security::SAF::Test {

/**
 * @brief Lightweight IRemoteObject mock for unit testing.
 *
 * Implements only the pure virtuals of IRemoteObject with trivial bodies, so it
 * can be constructed in a host unit test without an IPC runtime. AddDeathRecipient
 * and RemoveDeathRecipient return configurable results and record whether they
 * were called, so the DeathRecipient branches of GuiCallbackRegistry can be
 * exercised. TriggerDeath() fires OnRemoteDied on the stored recipient.
 */
class MockRemoteObject : public IRemoteObject {
public:
    MockRemoteObject() : IRemoteObject(u"ohos.security.saf.MockRemoteObject") {}
    ~MockRemoteObject() override = default;

    int32_t GetObjectRefCount() override { return 1; }

    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        (void)code;
        (void)data;
        (void)reply;
        (void)option;
        return 0;
    }

    int Dump(int fd, const std::vector<std::u16string> &args) override
    {
        (void)fd;
        (void)args;
        return 0;
    }

    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        if (addDeathFail_ || recipient == nullptr) {
            return false;
        }
        deathRecipient_ = recipient;
        return true;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        (void)recipient;
        removeDeathCalled_ = true;
        deathRecipient_ = nullptr;
        return true;
    }

    void SetAddDeathRecipientFail(bool fail) { addDeathFail_ = fail; }
    bool WasRemoveDeathRecipientCalled() const { return removeDeathCalled_; }

    void TriggerDeath()
    {
        if (deathRecipient_ == nullptr) {
            return;
        }
        const wptr<IRemoteObject> weak(this);
        deathRecipient_->OnRemoteDied(weak);
    }

private:
    bool addDeathFail_ = false;
    bool removeDeathCalled_ = false;
    sptr<DeathRecipient> deathRecipient_;
};

/**
 * @brief Mock implementation of IGuiCallback for unit testing.
 *
 * Records whether OnComponentClickDecision was called and stores the last
 * result. AsObject() returns a MockRemoteObject by default so the
 * DeathRecipient registration path in GuiCallbackRegistry can succeed; it can
 * be forced to return nullptr via SetAsObjectNull() to exercise the null
 * branches.
 */
class GuiCallbackMock : public IGuiCallback {
public:
    GuiCallbackMock() : remoteObj_(new MockRemoteObject()) {}
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

    sptr<IRemoteObject> AsObject() override
    {
        if (asObjectNull_) {
            return nullptr;
        }
        return remoteObj_;
    }

    void SetAsObjectNull(bool isNull) { asObjectNull_ = isNull; }
    MockRemoteObject *GetRemoteObject() const { return remoteObj_; }

private:
    bool called_ = false;
    ComponentClickResult lastResult_ {};
    bool asObjectNull_ = false;
    sptr<MockRemoteObject> remoteObj_;
};

} // namespace OHOS::Security::SAF::Test

#endif // GUI_CALLBACK_MOCK_H
