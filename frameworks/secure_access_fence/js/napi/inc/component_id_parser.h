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

#ifndef COMPONENT_ID_PARSER_H
#define COMPONENT_ID_PARSER_H

#include <string>

#include "secure_access_fence_type.h"

namespace OHOS::Security::SAF {

// JSON keys carried inside the componentId string reported by the click executor.
inline constexpr const char *COMPONENT_ID_BUNDLE_NAME_KEY = "bundleName";
inline constexpr const char *COMPONENT_ID_INDEX_KEY = "index";

/**
 * @brief Split the reported componentId into the app identity and the
 *        residual component descriptor.
 *
 * The click executor reports componentId as a JSON object such as
 * {"bundleName":"com.example.app","index":"0"} that may carry further
 * component-specific fields. The app identity is extracted into appInfo and
 * the two identity fields are then removed from the object, so the
 * re-serialized remainder (unformatted JSON) and appInfo do not duplicate
 * each other. A componentId carrying only the identity fields splits to "{}".
 * The "index" field accepts both the JSON string form and the JSON number
 * form.
 *
 * @param componentId The raw componentId string.
 * @param appInfo Output parsed application info. Reset to empty values when
 *                splitting fails.
 * @param residualComponentId Output componentId with the bundleName and index
 *                            fields removed. Reset to empty when splitting
 *                            fails; the caller decides how to deliver the raw
 *                            string in that case.
 * @return true when componentId carried a non-empty bundleName and a parsable
 *         index; false otherwise.
 */
bool SplitComponentId(const std::string &componentId, AppInfo &appInfo, std::string &residualComponentId);

} // namespace OHOS::Security::SAF

#endif // COMPONENT_ID_PARSER_H
