/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ASSET_LOG_H
#define ASSET_LOG_H

#undef HILOG_RAWFORMAT
#include "hilog/log.h"

#undef LOG_PUBLIC
#define LOG_PUBLIC "{public}"

#undef LOG_TAG
#define LOG_TAG "Asset"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002F08

#define LOGD(fmt, arg...) \
HILOG_DEBUG(LOG_CORE, "%" LOG_PUBLIC "s[%" LOG_PUBLIC "u]: " fmt "\n", __func__, __LINE__, ##arg)

#define LOGI(fmt, arg...) \
HILOG_INFO(LOG_CORE, "%" LOG_PUBLIC "s[%" LOG_PUBLIC "u]: " fmt "\n", __func__, __LINE__, ##arg)

#define LOGW(fmt, arg...) \
HILOG_WARN(LOG_CORE, "%" LOG_PUBLIC "s[%" LOG_PUBLIC "u]: " fmt "\n", __func__, __LINE__, ##arg)

#define LOGE(fmt, arg...) \
HILOG_ERROR(LOG_CORE, "%" LOG_PUBLIC "s[%" LOG_PUBLIC "u]: " fmt "\n", __func__, __LINE__, ##arg)

#endif /* ASSET_LOG_H */