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

#include "asset_api.h"

#include <gtest/gtest.h>

int32_t RemoveByAlias(const char* alias)
{
    Asset_Attr attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = {
                .size = strlen(alias),
                .data = reinterpret_cast<uint8_t*>(const_cast<char*>(alias))
            }
        }
    };
    return OH_Asset_Remove(attr, sizeof(attr) / sizeof(attr[0]));
}