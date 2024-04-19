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

#ifndef ASSET_TEST_COMMON_H
#define ASSET_TEST_COMMON_H

#include <stdint.h>
#include <stdlib.h>

#include "asset_type.h"
#include "asset_system_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_SIZE(arr) ((sizeof(arr)) / (sizeof((arr)[0])))
#define SPECIFIC_USER_ID 100

int32_t RemoveByAliasNdk(const char *alias);
int32_t RemoveByAliasSdk(const char *alias);
int32_t QueryByAliasNdk(const char *alias, Asset_ResultSet *resultSet);
int32_t QueryByAliasSdk(const char *alias, AssetResultSet *resultSet);
bool CompareBlobNdk(const Asset_Blob *blob1, const Asset_Blob *blob2);
bool CompareBlobSdk(const AssetBlob *blob1, const AssetBlob *blob2);
bool CheckMatchAttrResultNdk(const Asset_Attr *attrs, uint32_t attrCnt, const Asset_Result *result);
bool CheckMatchAttrResultSdk(const AssetAttr *attrs, uint32_t attrCnt, const AssetResult *result);

#ifdef __cplusplus
}
#endif

#endif // ASSET_TEST_COMMON_H
