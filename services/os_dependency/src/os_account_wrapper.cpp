/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "os_account_wrapper.h"

#include "os_account_manager.h"

#include "asset_log.h"

bool GetUserIdByUid(uint64_t uid, uint32_t *userId)
{
    int userIdTmp;
    int res = OHOS::AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(uid, userIdTmp);
    if (res != 0) {
        LOGE("[FATAL]Get user id from uid failed! res is %{public}d", res);
        return false;
    }
    *userId = userIdTmp;
    return true;
}

bool IsUserIdExist(int32_t userId, bool *exist)
{
    bool isUserIdExist;
    int ret = OHOS::AccountSA::OsAccountManager::IsOsAccountExists(userId, isUserIdExist);
    if (ret != 0) {
        LOGE("[FATAL]Check user id failed! res is %{public}d", ret);
        return false;
    }
    *exist = isUserIdExist;
    return true;
}
