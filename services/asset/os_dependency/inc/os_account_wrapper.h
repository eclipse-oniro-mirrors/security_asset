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

#ifndef OS_ACCOUNT_WRAPPER
#define OS_ACCOUNT_WRAPPER

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool GetUserIdByUid(uint64_t uid, uint32_t *userId);
bool IsUserIdExist(int32_t userId, bool *exist);
int32_t GetUserIds(int32_t *userIdsPtr, uint32_t *userIdsSize);
int32_t GetUsersSize(uint32_t *userIdsSize);
int32_t GetFirstUnlockUserIds(int32_t *userIdsPtr, uint32_t *userIdsSize);

#ifdef __cplusplus
}
#endif

#endif
