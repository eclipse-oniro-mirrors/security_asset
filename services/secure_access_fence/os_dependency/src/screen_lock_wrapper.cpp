/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "screen_lock_wrapper.h"
#ifdef THEME_SCREENLOCK_MGR_ENABLE
#include "screenlock_manager.h"
#include "screenlock_common.h"
#endif
#include "saf_log.h"
#include "saf_result_code.h"
#include <cstdint>

int32_t IsScreenLocked(bool *isLocked)
{
    if (isLocked == nullptr) {
        LOGE("IsScreenLocked: isLocked is nullptr");
        return SAF_ERR_NULL_PTR;
    }

#ifdef THEME_SCREENLOCK_MGR_ENABLE
    auto screenLockMgr = OHOS::ScreenLock::ScreenLockManager::GetInstance();
    if (screenLockMgr == nullptr) {
        LOGE("ScreenLockManager::GetInstance failed");
        *isLocked = false;
        return SAF_ERR_SCREENLOCK_SERVICE_ERROR;
    }

    bool locked = false;
    int32_t ret = screenLockMgr->IsLocked(locked);
    if (ret != OHOS::ScreenLock::E_SCREENLOCK_OK) {
        LOGE("IsLocked failed, error code: %{public}d", ret);
        *isLocked = false;
        return SAF_ERR_SCREENLOCK_SERVICE_ERROR;
    }

    *isLocked = locked;
    LOGI("IsScreenLocked result: %{public}d", locked);
#endif
    return SAF_SUCCESS;
}