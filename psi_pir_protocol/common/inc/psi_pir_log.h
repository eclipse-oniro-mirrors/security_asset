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

#ifndef PSI_PIR_PROTOCOL_LOG_H
#define PSI_PIR_PROTOCOL_LOG_H

#include "hilog/log.h"

// The current Asset build already owns this domain. Keeping the value here
// avoids a dependency on Asset's log wrapper. A future integration can define
// PSI_PIR_LOG_DOMAIN/PSI_PIR_LOG_TAG or replace this header after assigning its domain.
#ifndef PSI_PIR_LOG_DOMAIN
#define PSI_PIR_LOG_DOMAIN 0xD002F20
#endif

#ifndef PSI_PIR_LOG_TAG
#define PSI_PIR_LOG_TAG "PsiPirProtocol"
#endif

#undef LOG_DOMAIN
#define LOG_DOMAIN PSI_PIR_LOG_DOMAIN

#undef LOG_TAG
#define LOG_TAG PSI_PIR_LOG_TAG

#define PSI_PIR_LOGD(fmt, args...) \
    HILOG_DEBUG(LOG_CORE, "%{public}s[%{public}u]: " fmt "\n", \
        __func__, static_cast<unsigned int>(__LINE__), ##args)

#define PSI_PIR_LOGI(fmt, args...) \
    HILOG_INFO(LOG_CORE, "%{public}s[%{public}u]: " fmt "\n", \
        __func__, static_cast<unsigned int>(__LINE__), ##args)

#define PSI_PIR_LOGW(fmt, args...) \
    HILOG_WARN(LOG_CORE, "%{public}s[%{public}u]: " fmt "\n", \
        __func__, static_cast<unsigned int>(__LINE__), ##args)

#define PSI_PIR_LOGE(fmt, args...) \
    HILOG_ERROR(LOG_CORE, "%{public}s[%{public}u]: " fmt "\n", \
        __func__, static_cast<unsigned int>(__LINE__), ##args)

#endif // PSI_PIR_PROTOCOL_LOG_H
