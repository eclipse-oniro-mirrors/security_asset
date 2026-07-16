/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef SAF_PERMISSION_CHANGE_H
#define SAF_PERMISSION_CHANGE_H

#include <vector>
#include <string>
#include "access_token.h"

#ifdef __cplusplus
extern "C" {
#endif

void RestartAccessTokenService();
int GrantSelfPermission();

#ifdef __cplusplus
}

using AccessTokenID = OHOS::Security::AccessToken::AccessTokenID;

AccessTokenID GrantDefaultHapPermission();
AccessTokenID GrantSelfHapPermission(const std::vector<std::string>& permissions);
void RevokeSelfHapPermission(AccessTokenID tokenId);

#endif

#endif // SAF_PERMISSION_CHANGE_H