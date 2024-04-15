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

#ifndef ASSET_SYSTEM_API_TEST_H
#define ASSET_SYSTEM_API_TEST_H

#define ARRAY_SIZE(arr) ((sizeof(arr)) / (sizeof((arr)[0])))
#define SPECIFIC_USER_ID 100

namespace UnitTest::AssetSystemApiTest {
int AssetSystemApiTest001(void);
int AssetSystemApiTest002(void);
int AssetSystemApiTest003(void);
}

#endif // ASSET_SYSTEM_API_TEST_H
