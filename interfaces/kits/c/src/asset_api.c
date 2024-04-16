/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "sec_asset_api.h"
#include "sec_asset_type.h"

int32_t OH_Asset_Add(const Asset_Attr *attributes, uint32_t attrCnt)
{
    return AssetAdd((const AssetAttr *)attributes, attrCnt);
}

int32_t OH_Asset_Remove(const Asset_Attr *query, uint32_t queryCnt)
{
    return AssetRemove((const AssetAttr *)query, queryCnt);
}

int32_t OH_Asset_Update(const Asset_Attr *query, uint32_t queryCnt,
    const Asset_Attr *attributesToUpdate, uint32_t updateCnt)
{
    return AssetUpdate((const AssetAttr *)query, queryCnt, (const AssetAttr *)attributesToUpdate, updateCnt);
}

int32_t OH_Asset_PreQuery(const Asset_Attr *query, uint32_t queryCnt, Asset_Blob *challenge)
{
    return AssetPreQuery((const AssetAttr *)query, queryCnt, (AssetBlob *)challenge);
}

int32_t OH_Asset_Query(const Asset_Attr *query, uint32_t queryCnt, Asset_ResultSet *resultSet)
{
    return AssetQuery((const AssetAttr *)query, queryCnt, (AssetResultSet *)resultSet);
}

int32_t OH_Asset_PostQuery(const Asset_Attr *handle, uint32_t handleCnt)
{
    return AssetPostQuery((const AssetAttr *)handle, handleCnt);
}

Asset_Attr *OH_Asset_ParseAttr(const Asset_Result *result, Asset_Tag tag)
{
    return (Asset_Attr *)AssetParseAttr((const AssetResult *)result, (AssetTag)tag);
}

void OH_Asset_FreeBlob(Asset_Blob *blob)
{
    AssetFreeBlob((AssetBlob *)blob);
}

void OH_Asset_FreeResultSet(Asset_ResultSet *resultSet)
{
    AssetFreeResultSet((AssetResultSet *)resultSet);
}