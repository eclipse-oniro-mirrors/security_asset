/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "asset_napi_context.h"

#include "asset_system_api.h"

namespace OHOS {
namespace Security {
namespace Asset {
namespace {
void FreeAssetAttrs(std::vector<AssetAttr> &attrs)
{
    for (auto attr : attrs) {
        if ((attr.tag & SEC_ASSET_TAG_TYPE_MASK) == SEC_ASSET_TYPE_BYTES) {
            AssetFreeBlob(&attr.value.blob);
        }
    }
    attrs.clear();
}
} // anonymous namespace

BaseContext::~BaseContext()
{
    if (work != nullptr && env != nullptr) {
        napi_delete_async_work(env, work);
        work = nullptr;
        env = nullptr;
    }

    FreeAssetAttrs(attrs);
}

PreQueryContext::~PreQueryContext()
{
    AssetFreeBlob(&challenge);
}

QueryContext::~QueryContext()
{
    AssetFreeResultSet(&resultSet);
}

UpdateContext::~UpdateContext()
{
    FreeAssetAttrs(updateAttrs);
}

} // Asset
} // Security
} // OHOS
