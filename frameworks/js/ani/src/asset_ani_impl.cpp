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

#include <ani.h>
#include <ani_signature_builder.h>

#include "asset_log.h"

#include "asset_ani_add.h"
#include "asset_ani_post_query.h"
#include "asset_ani_pre_query.h"
#include "asset_ani_query_sync_result.h"
#include "asset_ani_query.h"
#include "asset_ani_remove.h"
#include "asset_ani_update.h"

using namespace OHOS::Security::Asset;

static ani_object AniAdd(ani_env *env, ani_object attributes)
{
    AniAddContext context;
    return context.Process(env, attributes);
}

static ani_object AniAddAsUser(ani_env *env, ani_int userId, ani_object attributes)
{
    AniAddContext context;
    return context.ProcessAsUser(env, attributes, userId);
}

static ani_object AniPostQuery(ani_env *env, ani_object handle)
{
    AniPostQueryContext context;
    return context.Process(env, handle);
}

static ani_object AniPostQueryAsUser(ani_env *env, ani_int userId, ani_object handle)
{
    AniPostQueryContext context;
    return context.ProcessAsUser(env, handle, userId);
}

static ani_object AniPreQuery(ani_env *env, ani_object query)
{
    AniPreQueryContext context;
    return context.Process(env, query);
}

static ani_object AniPreQueryAsUser(ani_env *env, ani_int userId, ani_object query)
{
    AniPreQueryContext context;
    return context.ProcessAsUser(env, query, userId);
}

static ani_object AniQuery(ani_env *env, ani_object query)
{
    AniQueryContext context;
    return context.Process(env, query);
}

static ani_object AniQueryAsUser(ani_env *env, ani_int userId, ani_object query)
{
    AniQueryContext context;
    return context.ProcessAsUser(env, query, userId);
}

static ani_object AniRemove(ani_env *env, ani_object query)
{
    AniRemoveContext context;
    return context.Process(env, query);
}

static ani_object AniRemoveAsUser(ani_env *env, ani_int userId, ani_object query)
{
    AniRemoveContext context;
    return context.ProcessAsUser(env, query, userId);
}

static ani_object AniUpdate(ani_env *env, ani_object query, ani_object attributesToUpdate)
{
    AniUpdateContext context;
    return context.Process(env, query, attributesToUpdate);
}

static ani_object AniUpdateAsUser(ani_env *env, ani_int userId,
    ani_object query, ani_object attributesToUpdate)
{
    AniUpdateContext context;
    return context.ProcessAsUser(env, query, attributesToUpdate, userId);
}

static ani_object AniQuerySyncResult(ani_env *env, ani_object query)
{
    AniQuerySyncResultContext context;
    return context.Process(env, query);
}

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    if (vm == nullptr || result == nullptr) {
        LOGE("vm or result is nullptr!");
        return ANI_OUT_OF_MEMORY;
    }
    ani_env *env;
    ani_status aniResult = vm->GetEnv(ANI_VERSION_1, &env);
    if (aniResult != ANI_OK) {
        LOGE("Unsupported ANI_VERSION_1");
        return aniResult;
    }
    ani_module globalModule{};
    std::string globalNamespace = arkts::ani_signature::Builder::BuildClass({
        "@ohos", "security", "asset"}).Descriptor();
    aniResult = env->FindModule(globalNamespace.c_str(), &globalModule);
    if (aniResult != ANI_OK) {
        LOGE("Not found %{public}s", globalNamespace.c_str());
        return ANI_INVALID_ARGS;
    }

    std::array methods = {
        ani_native_function {"AniAdd", nullptr, reinterpret_cast<void *>(AniAdd)},
        ani_native_function {"AniAddAsUser", nullptr, reinterpret_cast<void *>(AniAddAsUser)},
        ani_native_function {"AniRemove", nullptr, reinterpret_cast<void *>(AniRemove)},
        ani_native_function {"AniRemoveAsUser", nullptr, reinterpret_cast<void *>(AniRemoveAsUser)},
        ani_native_function {"AniUpdate", nullptr, reinterpret_cast<void *>(AniUpdate)},
        ani_native_function {"AniUpdateAsUser", nullptr, reinterpret_cast<void *>(AniUpdateAsUser)},
        ani_native_function {"AniPreQuery", nullptr, reinterpret_cast<void *>(AniPreQuery)},
        ani_native_function {"AniPreQueryAsUser", nullptr, reinterpret_cast<void *>(AniPreQueryAsUser)},
        ani_native_function {"AniQuery", nullptr, reinterpret_cast<void *>(AniQuery)},
        ani_native_function {"AniQueryAsUser", nullptr, reinterpret_cast<void *>(AniQueryAsUser)},
        ani_native_function {"AniPostQuery", nullptr, reinterpret_cast<void *>(AniPostQuery)},
        ani_native_function {"AniPostQueryAsUser", nullptr, reinterpret_cast<void *>(AniPostQueryAsUser)},
        ani_native_function {"AniQuerySyncResult", nullptr, reinterpret_cast<void *>(AniQuerySyncResult)},
    };

    aniResult = env->Module_BindNativeFunctions(globalModule, methods.data(), methods.size());
    if (aniResult != ANI_OK) {
        LOGE("Cannot bind native methods to %{public}s", globalNamespace.c_str());
        return aniResult;
    }

    LOGW("Start bind native methods to %{public}s", globalNamespace.c_str());
    *result = ANI_VERSION_1;
    return ANI_OK;
}