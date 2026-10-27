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

#include "saf_credential_test.h"

#include "nativetoken_kit.h"
#include "token_setproc.h"

namespace UnitTest::SafCredentialTest {

// Grants the test process system_basic APL + ACCESS_SECURE_FENCE so the
// service's authorization check (CheckSignaturePermission) passes. Mirrors
// the pattern in saf_permission_change.cpp but with the credential permission
// set.
int GrantCredentialPermission(void)
{
    const char *permissions[] = {
        "ohos.permission.ACCESS_SECURE_FENCE",
    };
    NativeTokenInfoParams info = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = permissions,
        .acls = nullptr,
        .processName = "saf_credential_test",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&info);
    return SetSelfTokenID(tokenId);
}

}  // namespace UnitTest::SafCredentialTest
