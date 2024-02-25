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

int32_t add_asset(const Asset_Attr *attributes, uint32_t attr_cnt);
int32_t remove_asset(const Asset_Attr *query, uint32_t query_cnt);
int32_t update_asset(const Asset_Attr *query, uint32_t query_cnt,
    const Asset_Attr *attributes_to_update, uint32_t update_cnt);
int32_t pre_query_asset(const Asset_Attr *query, uint32_t query_cnt, Asset_Blob *challenge);
int32_t query_asset(const Asset_Attr *query, uint32_t query_cnt, Asset_ResultSet *result_set);
int32_t post_query_asset(const Asset_Attr *handle, uint32_t handle_cnt);

int32_t OH_Asset_Add(const Asset_Attr *attributes, uint32_t attrCnt)
{
    return add_asset(attributes, attrCnt);
}

int32_t OH_Asset_Remove(const Asset_Attr *query, uint32_t queryCnt)
{
    return remove_asset(query, queryCnt);
}

int32_t OH_Asset_Update(const Asset_Attr *query, uint32_t queryCnt,
    const Asset_Attr *attributesToUpdate, uint32_t updateCnt)
{
    return update_asset(query, queryCnt, attributesToUpdate, updateCnt);
}

int32_t OH_Asset_PreQuery(const Asset_Attr *query, uint32_t queryCnt, Asset_Blob *challenge)
{
    return pre_query_asset(query, queryCnt, challenge);
}

int32_t OH_Asset_Query(const Asset_Attr *query, uint32_t queryCnt, Asset_ResultSet *resultSet)
{
    return query_asset(query, queryCnt, resultSet);
}

int32_t OH_Asset_PostQuery(const Asset_Attr *handle, uint32_t handleCnt)
{
    return post_query_asset(handle, handleCnt);
}

Asset_Attr *OH_Asset_ParseAttr(const Asset_Result *result, Asset_Tag tag)
{
    if (result == NULL || result->attrs == NULL || result->count == 0) {
        LOGE("[FATAL][SDK]Argument is NULL.");
        return NULL;
    }
    for (uint32_t i = 0; i < result->count; i++) {
        if (result->attrs[i].tag == tag) {
            return &result->attrs[i];
        }
    }
    LOGE("[FATAL][SDK]Attribute not found.");
    return NULL;
}

void OH_Asset_FreeBlob(Asset_Blob *blob)
{
    if (blob == NULL || blob->data == NULL || blob->size == 0) {
        return;
    }
    (void)memset_s(blob->data, blob->size, 0, blob->size);
    AssetFree(blob->data);
    blob->data = NULL;
    blob->size = 0;
}

void OH_Asset_FreeResultSet(Asset_ResultSet *resultSet)
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
                OH_Asset_FreeBlob(&attrs[j].value.blob);
            }
        }
        AssetFree(resultSet->results[i].attrs);
        resultSet->results[i].attrs = NULL;
        resultSet->results[i].count = 0;
    }
    AssetFree(resultSet->results);
    resultSet->results = NULL;
    resultSet->count = 0;
}