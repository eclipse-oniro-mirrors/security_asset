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

#ifndef ASSET_API_H
#define ASSET_API_H

#include <stdint.h>
#include <stdlib.h>

#include "asset_type.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t AddAsset(const AssetParam *attributes, uint32_t attrCnt);

int32_t RemoveAsset(const AssetParam *query, uint32_t queryCnt);

int32_t UpdateAsset(const AssetParam *query, uint32_t queryCnt,
    const AssetParam *attributesToUpdate, uint32_t updateCnt);

int32_t PreQueryAsset(const AssetParam *query, uint32_t queryCnt, AssetBlob *challenge);

int32_t QueryAsset(const AssetParam *query, uint32_t queryCnt, ResultSet *result);

int32_t PostQueryAsset(const AssetParam *handle, uint32_t handleCnt);

Version GetVersion(void);

#ifdef __cplusplus
}
#endif

#endif // ASSET_API_H