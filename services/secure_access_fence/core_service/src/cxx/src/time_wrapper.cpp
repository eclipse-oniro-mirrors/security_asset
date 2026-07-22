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

#include "time_wrapper.h"
#include "time_service_client.h"
#include "saf_log.h"

namespace OHOS {
namespace Security {
namespace SAF {

int64_t TimeWrapper::GetBootTimeMs()
{
    int64_t bootTimeMs = OHOS::MiscSercices::TimeServiceClient::GetInstance()->GetBootTimeMs();
    if (bootTimeMs < 0) {
        LOGE("TimeWrapper::GetBootTimeMs failed, ret = %{public}lld",
            static_cast<long long>(bootTimeMs));
    }
    return bootTimeMs;
}

} // namespace SAF
} // namespace Security
} // namespace OHOS
