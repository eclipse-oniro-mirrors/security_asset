/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SAF_MEMORY_MANAGER_WRAPPER_H
#define SAF_MEMORY_MANAGER_WRAPPER_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

bool CheckMemoryMgr();
int32_t NotifyStatus(int32_t status);
int32_t SetCritical(bool critical);

#ifdef __cplusplus
}
#endif

#endif // SAF_MEMORY_MANAGER_WRAPPER_H
