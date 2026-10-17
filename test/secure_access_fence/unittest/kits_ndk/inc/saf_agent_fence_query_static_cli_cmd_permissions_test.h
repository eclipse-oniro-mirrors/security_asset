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

#ifndef SAF_AGENT_FENCE_QUERY_STATIC_CLI_CMD_PERMISSIONS_TEST_H
#define SAF_AGENT_FENCE_QUERY_STATIC_CLI_CMD_PERMISSIONS_TEST_H

namespace UnitTest::SafAgentFenceQueryStaticCliCmdPermissionsTest {
int QueryStaticCliCmdPermissionsEmptyCmds001(void);
int QueryStaticCliCmdPermissionsEmptyCmdName001(void);
int QueryStaticCliCmdPermissionsExceedsMaxCount001(void);
int QueryStaticCliCmdPermissionsBatteryManagerTotalEnergy001(void);
int QueryStaticCliCmdPermissionsBmUninstall001(void);
int QueryStaticCliCmdPermissionsLocationEnable001(void);
int QueryStaticCliCmdPermissionsInvalidCallerTokenId001(void);
int QueryStaticCliCmdPermissionsNoQueryToolPermission001(void);
int QueryStaticCliCmdPermissionsNoLocationSwitchPermission001(void);
}

#endif // SAF_AGENT_FENCE_QUERY_STATIC_CLI_CMD_PERMISSIONS_TEST_H
