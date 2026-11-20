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

#ifndef PRIVACY_COVERAGE_BUNDLE_MGR_INTERFACE_H
#define PRIVACY_COVERAGE_BUNDLE_MGR_INTERFACE_H

#include "host_faults.h"
#include <memory>
#include <new>

namespace OHOS::AppExecFwk {
struct AssetGroupInfo {
    std::string appId;
};

class IBundleMgr {
public:
    int32_t GetAssetGroupsInfo(uint64_t uid, AssetGroupInfo &info)
    {
        auto &state = PrivacyHost::State();
        ++state.bundleQueries;
        state.lastBundleUid = uid;
        info.appId = state.appId;
        return state.bundleStatus;
    }
};
} // namespace OHOS::AppExecFwk

namespace OHOS {
struct PrivacyRemoteObject {};

template<typename T>
std::shared_ptr<T> iface_cast(const std::shared_ptr<PrivacyRemoteObject> &)
{
    if (PrivacyHost::State().bundleInterfaceUnavailable) {
        return nullptr;
    }
    return std::shared_ptr<T>(new (std::nothrow) T());
}
} // namespace OHOS
#endif
