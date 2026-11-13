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

#include "component_click_callback.h"

#include <unistd.h>

#include <cinttypes>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <utility>

#include "errors.h"
#include "iremote_stub.h"
#include "message_option.h"
#include "message_parcel.h"
#include "saf_gui_fence.h"
#include "saf_log.h"
#include "saf_result_code.h"
#include "secure_access_fence_type.h"

#include "agent_fence_error_codes.h"
#include "component_id_parser.h"
#include "napi_common.h"

namespace OHOS::Security::SAF {

using namespace OHOS::Security::SAF_ASSET_COMMON;

namespace {

// Upper bound of results waiting for JS-thread delivery. Click results are
// user-paced; the bound only guards against a stalled JS thread.
constexpr size_t MAX_PENDING_RESULTS = 1024;

// code value delivered to JS when the componentId payload could not be parsed.
constexpr int32_t COMPONENT_ID_PARSE_ERROR = INVALID_PARAMETER;

// reason text delivered to JS together with COMPONENT_ID_PARSE_ERROR.
constexpr const char *COMPONENT_ID_PARSE_FAIL_REASON = "Failed to parse component Id";

// Result payload handed from the binder thread (IPC stub) to the JS thread.
struct ClickResultPayload {
    std::string componentId;
    bool isAllowed;
    std::string reason;
};

// Module-global state: exactly one callback per process. The public JS API
// takes no session parameter, so the registration is process-wide.
std::mutex g_callbackMutex;
napi_threadsafe_function g_resultTsfn = nullptr;
sptr<IGuiCallback> g_callbackStub = nullptr; // keeps the IPC stub alive while registered
bool g_registered = false;

// Results queued by the binder thread; drained by the JS thread in CallJs.
std::deque<ClickResultPayload> g_pendingResults;

// The session id of the component click feedback channel: the uid of the
// controller application process. The click executor reports results with the
// same key so the SAF service can route them back to this process.
uint64_t GetComponentClickSessionId()
{
    return static_cast<uint64_t>(getuid());
}

// Pop one queued result; false when the queue is empty. Takes g_callbackMutex.
bool PopPendingResult(ClickResultPayload &payload)
{
    std::lock_guard<std::mutex> lock(g_callbackMutex);
    if (g_pendingResults.empty()) {
        return false;
    }
    payload = std::move(g_pendingResults.front());
    g_pendingResults.pop_front();
    return true;
}

// Build the ControlComponentClickResult JS object from one payload. Returns
// nullptr when the object cannot be created or a property fails to be set.
napi_value BuildJsClickResult(napi_env env, const ClickResultPayload &payload)
{
    AppInfo appInfo;
    std::string componentId;
    std::string reason = payload.reason;
    bool splitOk = SplitComponentId(payload.componentId, appInfo, componentId);
    if (!splitOk) {
        LOGE("BuildJsClickResult: SplitComponentId failed, componentId=%{public}s",
            payload.componentId.c_str());
        // Undecodable componentId: deliver the raw string so the upper layer
        // can still inspect it (code 24010000, empty appInfo).
        componentId = payload.componentId;
        // The code/reason pair describes the delivery error, not the decision.
        reason = COMPONENT_ID_PARSE_FAIL_REASON;
    }

    napi_value jsResult = nullptr;
    if (napi_create_object(env, &jsResult) != napi_ok) {
        LOGE("BuildJsClickResult: napi_create_object failed");
        return nullptr;
    }
    // code: 0 when the result was fully delivered; 24010000 when the
    // componentId could not be decoded into appInfo (partial delivery).
    if (NapiSetProperty(env, jsResult, "code", splitOk ? 0 : COMPONENT_ID_PARSE_ERROR) != napi_ok) {
        return nullptr;
    }
    if (NapiSetProperty(env, jsResult, "appInfo", appInfo) != napi_ok) {
        return nullptr;
    }
    // On success the app identity fields were already stripped from
    // componentId to avoid duplicating appInfo.
    if (NapiSetProperty(env, jsResult, "componentId", componentId) != napi_ok) {
        return nullptr;
    }
    if (NapiSetProperty(env, jsResult, "isAllowed", payload.isAllowed) != napi_ok) {
        return nullptr;
    }
    if (NapiSetProperty(env, jsResult, "reason", reason) != napi_ok) {
        return nullptr;
    }
    return jsResult;
}

// Invoke the JS callback with one click result. Runs on the JS thread through
// the threadsafe function machinery.
void CallJs(napi_env env, napi_value jsCallback, void *context, void *data)
{
    (void)context;
    (void)data;
    if (env == nullptr || jsCallback == nullptr) {
        LOGE("CallJs: invalid env or jsCallback");
        return;
    }
    ClickResultPayload payload;
    if (!PopPendingResult(payload)) {
        return;
    }
    napi_value jsResult = BuildJsClickResult(env, payload);
    if (jsResult == nullptr) {
        return;
    }

    napi_value undefined = nullptr;
    if (napi_get_undefined(env, &undefined) != napi_ok) {
        LOGE("CallJs: napi_get_undefined failed");
        return;
    }
    // A JS exception thrown inside the callback propagates as a pending
    // exception; it belongs to the application, not to this delivery.
    napi_value callResult = nullptr;
    if (napi_call_function(env, undefined, jsCallback, 1, &jsResult, &callResult) != napi_ok) {
        LOGE("CallJs: napi_call_function failed");
    }
}

// Called on a binder thread when the SAF service delivers a click result
// through the IGuiCallback channel. Enqueue the payload and wake the JS thread.
void EnqueueClickResult(const ComponentClickResult &result)
{
    std::lock_guard<std::mutex> lock(g_callbackMutex);
    if (g_resultTsfn == nullptr) {
        LOGW("EnqueueClickResult: no callback registered, drop result, compId=%{public}s",
            result.compId.c_str());
        return;
    }
    if (g_pendingResults.size() >= MAX_PENDING_RESULTS) {
        LOGE("EnqueueClickResult: pending results overflow, drop compId=%{public}s",
            result.compId.c_str());
        return;
    }
    g_pendingResults.push_back(ClickResultPayload { result.compId, result.allowed, result.reason });
    if (napi_call_threadsafe_function(g_resultTsfn, nullptr, napi_tsfn_nonblocking) != napi_ok) {
        // The threadsafe function is closing (unregister in progress); the
        // queued item can no longer be delivered.
        g_pendingResults.pop_back();
    }
}

// IPC stub running inside the controller application process. The service-side
// GuiCallbackProxy calls into it; it deserializes the click result with the
// same field order written by GuiCallbackProxy::WriteResult.
class ComponentClickCallbackStub final : public IRemoteStub<IGuiCallback> {
public:
    ComponentClickCallbackStub() = default;
    ~ComponentClickCallbackStub() override = default;

    void OnComponentClickDecision(const ComponentClickResult &result) override
    {
        EnqueueClickResult(result);
    }

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
        MessageOption &option) override
    {
        if (code == static_cast<uint32_t>(IGuiCallback::ON_COMPONENT_CLICK_DECISION)) {
            return HandleOnComponentClickDecision(data);
        }
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }

private:
    int HandleOnComponentClickDecision(MessageParcel &data)
    {
        if (data.ReadInterfaceToken() != IGuiCallback::GetDescriptor()) {
            LOGE("HandleOnComponentClickDecision: interface token mismatch");
            return ERR_INVALID_DATA;
        }
        ComponentClickResult result;
        result.compId = data.ReadString();
        result.allowed = data.ReadBool();
        result.reason = data.ReadString();
        OnComponentClickDecision(result);
        return ERR_NONE;
    }
};

// Caller must hold g_callbackMutex. Abort the threadsafe function (dropping
// undelivered items), clear pending results and drop the stub reference.
void ReleaseCallbackLocked()
{
    if (g_resultTsfn != nullptr) {
        napi_release_threadsafe_function(g_resultTsfn, napi_tsfn_abort);
        g_resultTsfn = nullptr;
    }
    g_pendingResults.clear();
    g_callbackStub = nullptr;
    g_registered = false;
}

napi_value GetUndefined(const napi_env env)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    return undefined;
}

// Validate that argv[0] is a JS function and output it. Throws a
// BusinessError (24010000) and returns false when the argument is invalid.
bool GetJsCallbackArgument(const napi_env env, napi_callback_info info, napi_value &jsCallback)
{
    constexpr size_t argcExpected = 1;
    size_t argc = argcExpected;
    napi_value argv[argcExpected] = { nullptr };
    if (napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) != napi_ok) {
        napi_throw(env, NapiCreateError(env, INVALID_PARAMETER, "Failed to get callback info"));
        return false;
    }
    napi_valuetype valueType = napi_undefined;
    if (argc < argcExpected || napi_typeof(env, argv[0], &valueType) != napi_ok ||
        valueType != napi_function) {
        napi_throw(env, NapiCreateError(env, INVALID_PARAMETER, "callback is null or undefined"));
        return false;
    }
    jsCallback = argv[0];
    return true;
}

// Overwrite semantics: unregister the previously registered callback first.
// Caller must hold g_callbackMutex. Returns false (with a pending
// BusinessError thrown) when the service rejected the unregister; the previous
// callback then stays registered and the new registration must not proceed.
bool UnregisterPreviousLocked(const napi_env env, uint64_t sessionId)
{
    if (!g_registered) {
        return true;
    }
    int32_t ret = SafGuiFence::UnregisterComponentClickCallback(sessionId);
    if (ret == SAF_ERR_GUI_SESSION_NOT_FOUND) {
        // Service-side session already gone (e.g. service restart); the
        // local state is stale, continue with the new registration.
        LOGW("UnregisterPreviousLocked: previous session not found at service");
    } else if (ret != SAF_SUCCESS) {
        // Keep the previous callback registered and surface the error.
        LOGE("UnregisterPreviousLocked: unregister previous failed, ret=%{public}d", ret);
        napi_throw(env, MapErrCodeToBusinessError(env, ret));
        return false;
    }
    ReleaseCallbackLocked();
    return true;
}

// Create the threadsafe function bridging the binder thread to the JS thread.
// Returns nullptr (with a pending BusinessError thrown) on failure.
napi_threadsafe_function CreateResultTsfn(const napi_env env, napi_value jsCallback)
{
    napi_value resourceName = nullptr;
    if (napi_create_string_utf8(env, "SAFComponentClickResult", NAPI_AUTO_LENGTH, &resourceName) !=
        napi_ok) {
        napi_throw(env, NapiCreateError(env, COMMON_INTERNAL_ERROR, "Failed to create resource name"));
        return nullptr;
    }
    napi_threadsafe_function tsfn = nullptr;
    // Unlimited tsfn queue: the pending-results deque carries the
    // back-pressure bound instead.
    napi_status status = napi_create_threadsafe_function(env, jsCallback, nullptr, resourceName,
        0, 1, nullptr, nullptr, nullptr, CallJs, &tsfn);
    if (status != napi_ok || tsfn == nullptr) {
        LOGE("CreateResultTsfn: create threadsafe function failed");
        napi_throw(env, NapiCreateError(env, COMMON_INTERNAL_ERROR, "Failed to create threadsafe function"));
        return nullptr;
    }
    return tsfn;
}

} // namespace

napi_value NapiRegisterControlComponentResultCallback(const napi_env env, napi_callback_info info)
{
    napi_value jsCallback = nullptr;
    if (!GetJsCallbackArgument(env, info, jsCallback)) {
        return nullptr;
    }

    uint64_t sessionId = GetComponentClickSessionId();
    std::lock_guard<std::mutex> lock(g_callbackMutex);
    if (!UnregisterPreviousLocked(env, sessionId)) {
        return nullptr;
    }

    napi_threadsafe_function tsfn = CreateResultTsfn(env, jsCallback);
    if (tsfn == nullptr) {
        return nullptr;
    }
    GuiCallback stub = sptr<ComponentClickCallbackStub>::MakeSptr();
    if (stub == nullptr) {
        napi_release_threadsafe_function(tsfn, napi_tsfn_abort);
        napi_throw(env, NapiCreateError(env, COMMON_INTERNAL_ERROR, "Failed to create callback stub"));
        return nullptr;
    }

    int32_t ret = SafGuiFence::RegisterComponentClickCallback(sessionId, stub);
    if (ret != SAF_SUCCESS) {
        LOGE("NapiRegisterControlComponentResultCallback: register failed, ret=%{public}d", ret);
        napi_release_threadsafe_function(tsfn, napi_tsfn_abort);
        napi_throw(env, MapErrCodeToBusinessError(env, ret));
        return nullptr;
    }

    g_resultTsfn = tsfn;
    g_callbackStub = stub;
    g_registered = true;
    LOGI("NapiRegisterControlComponentResultCallback: success, sessionId=%{public}" PRIu64, sessionId);
    return GetUndefined(env);
}

napi_value NapiUnregisterControlComponentResultCallback(const napi_env env, napi_callback_info info)
{
    (void)info; // no arguments
    std::lock_guard<std::mutex> lock(g_callbackMutex);
    if (!g_registered) {
        // Idempotent unregister: nothing registered in this process.
        LOGW("NapiUnregisterControlComponentResultCallback: no callback registered");
        return GetUndefined(env);
    }

    uint64_t sessionId = GetComponentClickSessionId();
    int32_t ret = SafGuiFence::UnregisterComponentClickCallback(sessionId);
    if (ret != SAF_SUCCESS && ret != SAF_ERR_GUI_SESSION_NOT_FOUND) {
        // Keep the local state so the caller may retry.
        LOGE("NapiUnregisterControlComponentResultCallback: unregister failed, ret=%{public}d", ret);
        napi_throw(env, MapErrCodeToBusinessError(env, ret));
        return nullptr;
    }
    if (ret == SAF_ERR_GUI_SESSION_NOT_FOUND) {
        // Session already gone at the service; treat as success.
        LOGW("NapiUnregisterControlComponentResultCallback: session not found at service");
    }
    ReleaseCallbackLocked();
    LOGI("NapiUnregisterControlComponentResultCallback: success, sessionId=%{public}" PRIu64, sessionId);
    return GetUndefined(env);
}

} // namespace OHOS::Security::SAF
