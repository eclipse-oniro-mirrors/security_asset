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

#include "component_id_parser.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>

#include "cJSON.h"

namespace OHOS::Security::SAF {

namespace {

// Extract a string value under key. Returns nullptr when the key is absent or
// the value is not a JSON string.
const char *GetStringValue(const cJSON *root, const char *key)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == nullptr || !cJSON_IsString(item) || item->valuestring == nullptr) {
        return nullptr;
    }
    return item->valuestring;
}

// Extract the application index under key. Accepts both the JSON string form
// ("0") and the JSON number form (0). Returns false when absent or unparsable.
bool GetIndexValue(const cJSON *root, const char *key, int32_t &index)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == nullptr) {
        return false;
    }
    if (cJSON_IsNumber(item)) {
        double value = item->valuedouble;
        if (value < INT32_MIN || value > INT32_MAX) {
            return false;
        }
        index = static_cast<int32_t>(value);
        return true;
    }
    if (cJSON_IsString(item) && item->valuestring != nullptr) {
        errno = 0;
        char *end = nullptr;
        long value = strtol(item->valuestring, &end, 10); // 10: decimal
        if (end == item->valuestring || *end != '\0' || errno == ERANGE ||
            value < INT32_MIN || value > INT32_MAX) {
            return false;
        }
        index = static_cast<int32_t>(value);
        return true;
    }
    return false;
}

} // namespace

bool SplitComponentId(const std::string &componentId, AppInfo &appInfo, std::string &residualComponentId)
{
    appInfo.bundleName.clear();
    appInfo.index = 0;
    residualComponentId.clear();
    if (componentId.empty()) {
        return false;
    }
    cJSON *root = cJSON_Parse(componentId.c_str());
    if (root == nullptr) {
        return false;
    }

    bool splitOk = false;
    const char *bundleName = GetStringValue(root, COMPONENT_ID_BUNDLE_NAME_KEY);
    do {
        if (bundleName == nullptr || *bundleName == '\0') {
            break;
        }
        int32_t index = 0;
        if (!GetIndexValue(root, COMPONENT_ID_INDEX_KEY, index)) {
            break;
        }
        appInfo.bundleName = bundleName;
        appInfo.index = index;
        splitOk = true;
    } while (false);

    if (!splitOk) {
        cJSON_Delete(root);
        return false;
    }

    // The app identity is delivered separately as appInfo; remove it from the
    // residual component descriptor so the two do not duplicate each other.
    cJSON_DeleteItemFromObjectCaseSensitive(root, COMPONENT_ID_BUNDLE_NAME_KEY);
    cJSON_DeleteItemFromObjectCaseSensitive(root, COMPONENT_ID_INDEX_KEY);
    char *residual = cJSON_PrintUnformatted(root);
    if (residual == nullptr) {
        // Serialization failed (allocation); the identity extraction still
        // succeeded, so report success with an empty residual descriptor.
        cJSON_Delete(root);
        return true;
    }
    residualComponentId = residual;
    cJSON_free(residual);
    cJSON_Delete(root);
    return true;
}

} // namespace OHOS::Security::SAF
