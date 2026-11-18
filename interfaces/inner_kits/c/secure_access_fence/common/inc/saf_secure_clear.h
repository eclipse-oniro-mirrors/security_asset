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

#ifndef SAF_SECURE_CLEAR_H
#define SAF_SECURE_CLEAR_H

#include <algorithm>

namespace OHOS {
namespace Security {
namespace SAF {

/// Zero-fills a container's storage before releasing it, so sensitive data
/// (authToken / credentials) does not linger in freed heap memory.
/// Applicable to std::vector and std::string alike.
template<typename T>
void SecureClear(T &container)
{
    if (!container.empty()) {
        std::fill(container.begin(), container.end(), typename T::value_type{});
        container.clear();
        container.shrink_to_fit();
    }
}

} // namespace SAF
} // namespace Security
} // namespace OHOS

#endif // SAF_SECURE_CLEAR_H
