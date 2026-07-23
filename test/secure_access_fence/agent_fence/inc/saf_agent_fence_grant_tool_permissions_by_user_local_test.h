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

#ifndef SAF_AGENT_FENCE_GRANT_TOOL_PERMISSIONS_BY_USER_LOCAL_TEST_H
#define SAF_AGENT_FENCE_GRANT_TOOL_PERMISSIONS_BY_USER_LOCAL_TEST_H

namespace UnitTest::SafAgentFenceGrantToolPermissionsByUserLocalTest {
int GrantToolPermissionsByUserEmptyPermissionInfo001(void);
int GrantToolPermissionsByUserDeniedPermission001(void);
int GrantToolPermissionsByUserNotDeterminedPermission001(void);
int GrantToolPermissionsByUserRestrictedPermission001(void);
int GrantToolPermissionsByUserUnknownOperation001(void);
int GrantToolPermissionsByUserEmptyList001(void);
int GrantToolPermissionsByUserCLIOperation001(void);
int GrantToolPermissionsByUserAPIOperation001(void);
int GrantToolPermissionsByUserMixedOperation001(void);
int GrantToolPermissionsByUserEmptyOperation001(void);
int GrantToolPermissionsByUserExceedExpireTime001(void);
int VerifyTicketWithEmptyTicketInfo001(void);
int VerifyTicketWithInvalidTicketInfo001(void);
}

#endif // SAF_AGENT_FENCE_GRANT_TOOL_PERMISSIONS_BY_USER_LOCAL_TEST_H