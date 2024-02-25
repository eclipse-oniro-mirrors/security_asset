/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ASSET_MEM_H
#define ASSET_MEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *AssetMalloc(uint32_t size);

void AssetFree(void *addr);

int32_t AssetMemCmp(const void *ptr1, const void *ptr2, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif // ASSET_MEM_H