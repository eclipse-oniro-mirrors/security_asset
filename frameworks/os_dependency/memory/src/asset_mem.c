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

#include "asset_mem.h"

#include <stdlib.h>
#include <string.h>

#include "securec.h"

#include "asset_log.h"

void *AssetMalloc(uint32_t size)
{
    if (size == 0) {
        return NULL;
    }
    void *addr = malloc(size);
    if (addr != NULL) {
        (void)memset_s(addr, size, 0, size);
        LOGE("[YZT] malloc %p size = %u", addr, size); // todo: test memory leak
    }
    return addr;
}

void AssetFree(void *addr)
{
    if (addr == NULL) {
        return;
    }
    LOGE("[YZT] free %p", addr); // todo: test memory leak
    free(addr);
}

int32_t AssetMemCmp(const void *ptr1, const void *ptr2, uint32_t size)
{
    return memcmp(ptr1, ptr2, size);
}

