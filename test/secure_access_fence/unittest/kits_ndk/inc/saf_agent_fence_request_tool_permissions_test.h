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

#ifndef SAF_AGENT_FENCE_REQUEST_TOOL_PERMISSIONS_TEST_H
#define SAF_AGENT_FENCE_REQUEST_TOOL_PERMISSIONS_TEST_H

namespace UnitTest::SafAgentFenceRequestToolPermissionsTest {
int RequestToolPermissionsUnknownOperation001(void);
int RequestToolPermissionsEmptyCLICmdNameOperation001(void);
int RequestToolPermissionsCLIOperation001(void);
int RequestToolPermissionsEmptyAPIPermission001(void);
int RequestToolPermissionsAPIOperation001(void);
int RequestToolPermissionsEmptyOperation001(void);
int RequestToolPermissionsMixedOperation001(void);
int RequestToolPermissionsExceedsMaxExpireTimeLimit001(void);
int RequestToolPermissionsWithTokenId001(void);
int RequestToolPermissionsNoTicket001(void);
}

#endif // SAF_AGENT_FENCE_REQUEST_TOOL_PERMISSIONS_TEST_H
