/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "img_dependency_wrapper.h"

#include "app_image_observer_manager.h"
#include "res_type.h"
#include "res_sched_client.h"

#include "asset_log.h"

const std::string LOAD_INTERFACE_KEY = "loadInterface";
const std::string UNLOAD_INTERFACE_KEY = "unloadInterface";
const std::string CLIENT_PID_KEY = "clientPid";

bool IsBeforeImageCreationPoint()
{
    return OHOS::AppExecFwk::AppImageObserverManager::GetInstance().IsBeforeImageCreationPoint();
}

bool IsAbilityCreated()
{
    return OHOS::AppExecFwk::AppImageObserverManager::GetInstance().IsAbilityCreated();
}

void ReportSnapshotFailure(const char *loadInterfaceName, const char *unloadInterfaceName)
{
    std::unordered_map<std::string, std::string> mapPayLoad;
    if (loadInterfaceName != nullptr && unloadInterfaceName != nullptr) {
        mapPayLoad[LOAD_INTERFACE_KEY] = std::string(loadInterfaceName);
        mapPayLoad[UNLOAD_INTERFACE_KEY] = std::string(unloadInterfaceName);
    }
    uint32_t type = OHOS::ResourceSchedule::ResType::RES_TYPE_SNAPSHOT_FAILURE;
    OHOS::ResourceSchedule::ResSchedClient::GetInstance().ReportData(type, 0, mapPayLoad);
}
