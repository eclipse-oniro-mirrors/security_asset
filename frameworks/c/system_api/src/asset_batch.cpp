/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "asset_system_api.h"

#include "asset_log.h"
#include "asset_mem.h"

#include <vector>

extern "C" {
int32_t asset_batch_add(std::vector<std::vector<AssetAttr>> *attributes_array,
    std::vector<std::pair<uint32_t, uint32_t>> *err_info);

int32_t asset_batch_remove(std::vector<std::vector<AssetAttr>> *attributes_array);

int32_t asset_batch_update(std::vector<std::vector<AssetAttr>> *attributes_array,
    std::vector<std::vector<AssetAttr>> *attributes_to_update_array,
    std::vector<std::pair<uint32_t, uint32_t>> *err_info)
};

int32_t AssetBatchAdd(std::vector<std::vector<AssetAttr>> &attrsArray,
    std::vector<std::pair<uint32_t, uint32_t>> &errInfoArray)
{
    return asset_batch_add(&attrsArray, &errInfoArray);
}

int32_t AssetBatchRemove(std::vector<std::vector<AssetAttr>> &attrsArray)
{
    return asset_batch_remove(&attrsArray);
}

int32_t AssetBatchUpdate(std::vector<std::vector<AssetAttr>> &attrsArray,
    std::vector<std::vector<AssetAttr>> &attrsToUpdateArray,
    std::vector<std::pair<uint32_t, uint32_t>> &errInfoArray)
{
    if (attrsArray.empty() || attrsToUpdateArray.empty()) {
        return SEC_ASSET_INVALID_ARGUMENT;
    }
    return asset_batch_update(&attrsArray, &attrsToUpdateArray, &errInfoArray);
}
