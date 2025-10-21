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

#ifndef ASSET_ANI_COMMON_H
#define ASSET_ANI_COMMON_H

#include <ani.h>

#include "asset_log.h"
#include "asset_system_type.h"

#include "asset_ani_base.h"
#include "asset_ani_pre_query.h"
#include "asset_ani_query.h"
#include "asset_api_check.h"

namespace OHOS {
namespace Security {
namespace Asset {
#define MAX_MESSAGE_LEN 256
#define MAX_ARGS_NUM 5

bool CreateAniUint8Array(ani_env *env, const AssetBlob &blob, ani_object &arrayOut);

bool CreateAniSyncResult(ani_env *env, const AssetSyncResult &syncResult, ani_object &syncResultOut);

int32_t ParseAssetAttributeFromAni(ani_env *env, const ani_object &paramObj, std::vector<AssetAttr> &attrs);

int32_t CreateAniMapArray(ani_env *env, const AssetResultSet &resultSet, ani_object &mapArrayOut);

ani_object CreateAniError(ani_env *env, const int32_t result, const char *errMsg);

ani_object CreateAniResult(ani_env *env, const int32_t result, const char *errMsg, const ani_object &resultObj);

void FreeAssetAttrs(std::vector<AssetAttr> &attrs);
} // Asset
} // Security
} // OHOS

#endif // ASSET_ANI_COMMON_H