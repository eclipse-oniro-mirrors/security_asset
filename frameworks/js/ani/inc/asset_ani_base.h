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

#ifndef ASSET_ANI_BASE_H
#define ASSET_ANI_BASE_H

#include <ani.h>
#include <vector>
#include <functional>

#include "asset_system_type.h"

#define MAX_MESSAGE_LEN 256

namespace OHOS {
namespace Security {
namespace Asset {

class AniBaseContext {
public:
    virtual ~AniBaseContext();

    ani_object Process(ani_env *env, const ani_object &attributes);

    ani_object ProcessAsUser(ani_env *env, const ani_object &attributes, int32_t userId);

protected:
    std::vector<AssetAttr> attrs_;

    char errMsg_[MAX_MESSAGE_LEN];

    std::function<void(char *)> AniGetError();

private:
    int32_t Parse(ani_env *env, const ani_object &attributes);

    virtual int32_t Check(ani_env *env) = 0;

    virtual int32_t Execute(ani_env *env) = 0;

    virtual ani_object GetResult(ani_env *env, int32_t result, const char *errMsg) = 0;
};
} // Asset
} // Security
} // OHOS

#endif // ASSET_ANI_BASE_H