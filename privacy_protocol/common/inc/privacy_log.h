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

#ifndef PRIVACY_PROTOCOL_LOG_H
#define PRIVACY_PROTOCOL_LOG_H

#include "hilog/log.h"
#include <cstdint>

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002F0F

#undef LOG_TAG
#define LOG_TAG "PrivacyProtocol"

#define LOGD(fmt, args...)                                                                                             \
    HILOG_DEBUG(LOG_CORE, "%{public}s[%{public}u]: " fmt "\n", __func__, static_cast<uint32_t>(__LINE__), ##args)

#define LOGI(fmt, args...)                                                                                             \
    HILOG_INFO(LOG_CORE, "%{public}s[%{public}u]: " fmt "\n", __func__, static_cast<uint32_t>(__LINE__), ##args)

#define LOGW(fmt, args...)                                                                                             \
    HILOG_WARN(LOG_CORE, "%{public}s[%{public}u]: " fmt "\n", __func__, static_cast<uint32_t>(__LINE__), ##args)

#define LOGE(fmt, args...)                                                                                             \
    HILOG_ERROR(LOG_CORE, "%{public}s[%{public}u]: " fmt "\n", __func__, static_cast<uint32_t>(__LINE__), ##args)

#endif // PRIVACY_PROTOCOL_LOG_H
