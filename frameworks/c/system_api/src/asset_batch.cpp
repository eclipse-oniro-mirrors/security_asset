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

struct CArray {
    const AssetAttr *data;
    size_t len;
};

struct C2DArray {
    const CArray *items;
    size_t len;
};

struct MutPairVec {
    std::pair<uint32_t, uint32_t> *data;
    size_t len;
};

extern "C" {
int32_t asset_batch_add(C2DArray &arr, MutPairVec &err_info);

int32_t asset_batch_remove(C2DArray &arr);

int32_t asset_batch_update(C2DArray &arr, C2DArray &arr_to_update, MutPairVec &err_info);
};

int32_t AssetBatchAdd(std::vector<std::vector<AssetAttr>> &attrsArray,
    std::vector<std::pair<uint32_t, uint32_t>> &errInfoArray)
{
    std::vector<CArray> cArrays;
    for (const auto &inner : attrsArray) {
        cArrays.push_back({
            inner.data(),
            inner.size()
        });
    }
    C2DArray arr {
        cArrays.data(),
        cArrays.size()
    };
    MutPairVec outVec;
    int32_t ret = asset_batch_add(arr, outVec);
    errInfoArray = std::vector<std::pair<uint32_t, uint32_t>>(outVec.data, outVec.data + outVec.len);
    return ret;
}

int32_t AssetBatchRemove(std::vector<std::vector<AssetAttr>> &attrsArray)
{
    std::vector<CArray> cArrays;
    for (const auto &inner : attrsArray) {
        cArrays.push_back({
            inner.data(),
            inner.size()
        });
    }
    C2DArray arr {
        cArrays.data(),
        cArrays.size()
    };
    return asset_batch_remove(arr);
}

int32_t AssetBatchUpdate(std::vector<std::vector<AssetAttr>> &attrsArray,
    std::vector<std::vector<AssetAttr>> &attrsToUpdateArray,
    std::vector<std::pair<uint32_t, uint32_t>> &errInfoArray)
{
    if (attrsArray.empty() || attrsToUpdateArray.empty()) {
        return SEC_ASSET_INVALID_ARGUMENT;
    }
    std::vector<CArray> cArrays;
    for (const auto &inner : attrsArray) {
        cArrays.push_back({
            inner.data(),
            inner.size()
        });
    }
    C2DArray arr {
        cArrays.data(),
        cArrays.size()
    };

    std::vector<CArray> cArraysUpdate;
    for (const auto &inner : attrsArray) {
        cArraysUpdate.push_back({
            inner.data(),
            inner.size()
        });
    }
    C2DArray arrToUpdate {
        cArraysUpdate.data(),
        cArraysUpdate.size()
    };
    MutPairVec outVec;
    int32_t ret = asset_batch_update(arr, arrToUpdate, outVec);
    errInfoArray = std::vector<std::pair<uint32_t, uint32_t>>(outVec.data, outVec.data + outVec.len);
    return ret;
}
