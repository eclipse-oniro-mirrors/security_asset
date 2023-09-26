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

#ifndef ASSET_INNER_API_H
#define ASSET_INNER_API_H

#include <stdint.h>
#include <stdlib.h>

#include "asset_inner_type.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t AddAsset(const AssetAttr *attributes, uint32_t attrCnt);

int32_t RemoveAsset(const AssetAttr *query, uint32_t queryCnt);

int32_t UpdateAsset(const AssetAttr *query, uint32_t queryCnt,
    const AssetAttr *attributesToUpdate, uint32_t updateCnt);

int32_t PreQueryAsset(const AssetAttr *query, uint32_t queryCnt, AssetBlob *challenge);

int32_t QueryAsset(const AssetAttr *query, uint32_t queryCnt, AssetResultSet *result);

int32_t PostQueryAsset(const AssetAttr *handle, uint32_t handleCnt);

Version GetVersion(void);

/**
 * Parse the AssetResult to get the specified attribute.
 * Note: The returned AssetAttr pointer does not need to be released.
 */
AssetAttr *ParseAttr(const AssetResult *result, AssetTag tag);

/**
 * Release the AssetBlob returned by PreQueryAsset function.
 */
void FreeAssetBlob(AssetBlob *blob);

/**
 * Release the AssetResultSet returned by QueryAsset function.
 */
void FreeAssetResultSet(AssetResultSet *resultSet);

#ifdef __cplusplus
}
#endif

#endif // ASSET_INNER_API_H