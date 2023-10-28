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

#ifdef __cplusplus
extern "C" {
#endif

int32_t RemoveByAlias(const char *alias);
int32_t QueryByAlias(const char *alias, Asset_ResultSet *resultSet);
bool CompareBlob(const Asset_Blob *blob1, const Asset_Blob *blob2);

#ifdef __cplusplus
}
#endif

#endif // ASSET_TEST_COMMON_H
