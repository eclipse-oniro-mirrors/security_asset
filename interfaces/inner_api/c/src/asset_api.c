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

#include "asset_api.h"

#include "securec.h"

#include "asset_log.h"
#include "asset_mem.h"

extern int32_t AddAssetC2Rust(const AssetAttr *attributes, uint32_t attrCnt);

int32_t AddAsset(const AssetAttr *attributes, uint32_t attrCnt)
{
    return AddAssetC2Rust(attributes, attrCnt);
}

int32_t RemoveAsset(const AssetAttr *query, uint32_t queryCnt)
{
    return ASSET_SUCCESS;
}

int32_t UpdateAsset(const AssetAttr *query, uint32_t queryCnt,
    const AssetAttr *attributesToUpdate, uint32_t updateCnt)
{
    return ASSET_SUCCESS;
}

int32_t PreQueryAsset(const AssetAttr *query, uint32_t queryCnt, AssetBlob *challenge)
{
    return ASSET_SUCCESS;
}

int32_t QueryAsset(const AssetAttr *query, uint32_t queryCnt, AssetResultSet *result)
{
    return ASSET_SUCCESS;
}

int32_t PostQueryAsset(const AssetAttr *handle, uint32_t handleCnt)
{
    return ASSET_SUCCESS;
}

Version GetVersion(void)
{
    Version v = { 1, 0, 0 };
    return  v;
}

AssetAttr *ParseAttr(const AssetResult *result, AssetTag tag)
{
    if (result == NULL || result->attrs == NULL || result->count == 0) {
        LOGE("Argument is NULL.");
        return NULL;
    }
    for (uint32_t i = 0; i < result->count; i++) {
        if (result->attrs[i].tag == tag) {
            return &result->attrs[i];
        }
    }
    LOGE("Attribute not found.");
    return NULL;
}

void FreeAssetBlob(AssetBlob *blob)
{
    if (blob == NULL || blob->data == NULL || blob->size == 0) {
        return;
    }
    (void)memset_s(blob->data, blob->size, 0, blob->size);
    AssetFree(blob->data);
    blob->data = NULL;
    blob->size = 0;
}

void FreeAssetResultSet(AssetResultSet *resultSet)
{
    if (resultSet == NULL || resultSet->results == NULL || resultSet->count == 0) {
        return;
    }

    for (uint32_t i = 0; i < resultSet->count; i++) {
        AssetAttr *attrs = resultSet->results[i].attrs;
        uint32_t attrCnt = resultSet->results[i].count;
        if (attrs == NULL || attrCnt == 0) {
            continue;
        }
        for (uint32_t j = 0; j < attrCnt; j++) {
            if ((attrs[j].tag & ASSET_TAG_TYPE_MASK) == ASSET_TYPE_BYTES) {
                FreeAssetBlob(&attrs[j].value.blob);
            }
        }
        resultSet->results[i].attrs = NULL;
        resultSet->results[i].count = 0;
    }
    resultSet->results = NULL;
    resultSet->count = 0;
}