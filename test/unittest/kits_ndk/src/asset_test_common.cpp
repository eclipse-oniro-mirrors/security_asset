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

#include "asset_test_common.h"

#include <string>
#include <gtest/gtest.h>

#include "asset_system_api.h"

int32_t RemoveByAlias(const char *alias)
{
    AssetAttr attr[] = {
        {
            .tag = ASSET_SYSTEM_TAG_ALIAS,
            .value.blob = {
                .size = strlen(alias),
                .data = reinterpret_cast<uint8_t*>(const_cast<char*>(alias))
            }
        }
    };
    return AssetRemove(attr, ARRAY_SIZE(attr));
}

int32_t QueryByAlias(const char *alias, AssetResultSet *resultSet)
{
    AssetAttr attr[] = {
        {
            .tag = ASSET_SYSTEM_TAG_ALIAS,
            .value.blob = {
                .size = strlen(alias),
                .data = reinterpret_cast<uint8_t*>(const_cast<char*>(alias))
            }
        }, {
            .tag = ASSET_SYSTEM_TAG_RETURN_TYPE,
            .value.u32 = ASSET_SYSTEM_RETURN_ALL
        }
    };
    return AssetQuery(attr, ARRAY_SIZE(attr), resultSet);
}

bool CompareBlob(const AssetBlob *blob1, const AssetBlob *blob2)
{
    if (blob1->size != blob2->size) {
        return false;
    }
    return memcmp(blob1->data, blob2->data, blob1->size) == 0;
}