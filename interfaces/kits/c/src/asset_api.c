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

#include "asset_log.h"
#include "asset_mem.h"

#include "securec.h"

extern int32_t AddAssetC2Rust(const Asset_Attr *attributes, uint32_t attrCnt);
extern int32_t RemoveAssetC2Rust(const Asset_Attr *query, uint32_t queryCnt);

int32_t OH_Asset_AddAsset(const Asset_Attr *attributes, uint32_t attrCnt)
{
    return AddAssetC2Rust(attributes, attrCnt);
}

int32_t OH_Asset_RemoveAsset(const Asset_Attr *query, uint32_t queryCnt)
{
    return RemoveAssetC2Rust(query, queryCnt);
}

int32_t OH_Asset_UpdateAsset(const Asset_Attr *query, uint32_t queryCnt,
    const Asset_Attr *attributesToUpdate, uint32_t updateCnt)
{
    return ASSET_SUCCESS;
}

int32_t OH_Asset_PreQueryAsset(const Asset_Attr *query, uint32_t queryCnt, Asset_Blob *challenge)
{
    return ASSET_SUCCESS;
}

int32_t OH_Asset_QueryAsset(const Asset_Attr *query, uint32_t queryCnt, Asset_ResultSet *result)
{
    return ASSET_SUCCESS;
}

int32_t OH_Asset_PostQueryAsset(const Asset_Attr *handle, uint32_t handleCnt)
{
    return ASSET_SUCCESS;
}

Asset_Version OH_Asset_GetVersion(void)
{
    Asset_Version v = { 1, 0, 0 };
    return  v;
}

Asset_Attr *OH_Asset_ParseAttr(const Asset_Result *result, Asset_Tag tag)
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

void OH_Asset_FreeAssetBlob(Asset_Blob *blob)
{
    if (blob == NULL || blob->data == NULL || blob->size == 0) {
        return;
    }
    (void)memset_s(blob->data, blob->size, 0, blob->size);
    AssetFree(blob->data);
    blob->data = NULL;
    blob->size = 0;
}

void OH_Asset_FreeAssetResultSet(Asset_ResultSet *resultSet)
{
    if (resultSet == NULL || resultSet->results == NULL || resultSet->count == 0) {
        return;
    }

    for (uint32_t i = 0; i < resultSet->count; i++) {
        Asset_Attr *attrs = resultSet->results[i].attrs;
        uint32_t attrCnt = resultSet->results[i].count;
        if (attrs == NULL || attrCnt == 0) {
            continue;
        }
        for (uint32_t j = 0; j < attrCnt; j++) {
            if ((attrs[j].tag & ASSET_TAG_TYPE_MASK) == ASSET_TYPE_BYTES) {
                OH_Asset_FreeAssetBlob(&attrs[j].value.blob);
            }
        }
        resultSet->results[i].attrs = NULL;
        resultSet->results[i].count = 0;
    }
    resultSet->results = NULL;
    resultSet->count = 0;
}