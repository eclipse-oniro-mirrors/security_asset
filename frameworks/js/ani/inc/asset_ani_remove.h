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

#ifndef ASSET_ANI_REMOVE_H
#define ASSET_ANI_REMOVE_H

#include <ani.h>

#include "asset_ani_base.h"

namespace OHOS {
namespace Security {
namespace Asset {

class AniRemoveContext : public AniBaseContext {
public:
    virtual ~AniRemoveContext();

private:
    int32_t Check(ani_env *env);

    int32_t Execute(ani_env *env);

    ani_object GetResult(ani_env *env, int32_t result, const char *errMsg);
};

} // Asset
} // Security
} // OHOS

#endif // ASSET_ANI_REMOVE_H