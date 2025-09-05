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

#include "asset_type.h"
#include "asset_log.h"

bool GetUserIdByUid(uint64_t uid, uint32_t *userId)
{
    int userIdTmp;
    int res = OHOS::AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(uid, userIdTmp);
    if (res != 0 || userIdTmp < 0) {
        LOGE("[FATAL]Get user id from uid failed! res is %{public}d, user id is %{public}d.", res, userIdTmp);
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

int32_t GetUserIds(int32_t *userIdsPtr, uint32_t *userIdsSize)
{
    std::vector<OHOS::AccountSA::OsAccountInfo> accountInfos = {};
    int32_t ret = OHOS::AccountSA::OsAccountManager::QueryAllCreatedOsAccounts(accountInfos);
    if (ret != OHOS::ERR_OK) {
        LOGE("[FATAL]Query account id failed! res is %{public}d", ret);
        return ASSET_ACCOUNT_ERROR;
    }
    if (accountInfos.empty()) {
        LOGE("[FATAL]accountInfos is empty");
        return ASSET_ACCOUNT_ERROR;
    }
    std::vector<int32_t> userIdsVec = { 0 };
    std::transform(accountInfos.begin(), accountInfos.end(), std::back_inserter(userIdsVec),
        [](auto &iter) { return iter.GetLocalId(); });
    if (userIdsVec.size() > *userIdsSize) {
        LOGE("[FATAL]Users size increased after getting users size.");
        return ASSET_ACCOUNT_ERROR;
    }
    for (uint32_t i = 0; i < userIdsVec.size(); i++) {
        userIdsPtr[i] = userIdsVec[i];
    }
    *userIdsSize = static_cast<uint32_t>(userIdsVec.size());

    return ASSET_SUCCESS;
}

int32_t GetFirstUnlockUserIds(int32_t *userIdsPtr, uint32_t *userIdsSize)
{
    std::vector<int32_t> userIdsVec = {};
    int32_t ret = OHOS::AccountSA::OsAccountManager::GetUnlockedOsAccountLocalIds(userIdsVec);
    if (ret != OHOS::ERR_OK) {
        LOGE("[FATAL]Query unlocked account id failed! res is %{public}d", ret);
        return ASSET_ACCOUNT_ERROR;
    }
    if (userIdsVec.size() > *userIdsSize) {
        LOGE("[FATAL]Users size increased after getting users size.");
        return ASSET_ACCOUNT_ERROR;
    }
    for (uint32_t i = 0; i < userIdsVec.size(); i++) {
        userIdsPtr[i] = userIdsVec[i];
    }
    *userIdsSize = static_cast<uint32_t>(userIdsVec.size());

    return ASSET_SUCCESS;
}

int32_t GetUsersSize(uint32_t *userIdsSize)
{
    std::vector<OHOS::AccountSA::OsAccountInfo> accountInfos = {};
    int32_t ret = OHOS::AccountSA::OsAccountManager::QueryAllCreatedOsAccounts(accountInfos);
    if (ret != OHOS::ERR_OK) {
        LOGE("[FATAL]Query all account id failed! res is %{public}d", ret);
        return ASSET_ACCOUNT_ERROR;
    }
    if (accountInfos.empty()) {
        LOGE("[FATAL]accountInfos is empty");
        return ASSET_ACCOUNT_ERROR;
    }
    std::vector<int32_t> userIdsVec = { 0 };
    std::transform(accountInfos.begin(), accountInfos.end(), std::back_inserter(userIdsVec),
        [](auto &iter) { return iter.GetLocalId(); });
    *userIdsSize = static_cast<uint32_t>(userIdsVec.size());

    return ASSET_SUCCESS;
}