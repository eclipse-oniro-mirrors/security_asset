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

#ifndef SAF_AGENT_FENCE_TASK_IDENTITY_NDK_TEST_H
#define SAF_AGENT_FENCE_TASK_IDENTITY_NDK_TEST_H

namespace UnitTest::SafAgentFenceTaskIdentityNdkTest {
int GenerateTaskChallengeTest001(void);
int GenerateTaskChallengeTest002(void);
int GenerateTaskChallengeTest003(void);
int GenerateTaskIdentityTest001(void);
int GenerateTaskIdentityTest002(void);
int GenerateTaskIdentityTest003(void);
int RevokeTaskIdentityTest001(void);
int RevokeTaskIdentityTest002(void);
int RevokeTaskIdentityTest003(void);
int RevokeTaskIdentityTest004(void);
int RetrieveAuthTokenByTaskIdTest001(void);
int RetrieveAuthTokenByTaskIdTest002(void);
int RetrieveAuthTokenByTaskIdTest003(void);
}

#endif // SAF_AGENT_FENCE_TASK_IDENTITY_NDK_TEST_H
