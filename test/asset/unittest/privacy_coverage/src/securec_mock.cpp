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

#include "securec.h"
#include <cstdlib>
#include <cstring>

extern "C" errno_t memcpy_s(void *dest, size_t capacity, const void *src, size_t count)
{
    if (dest == nullptr || src == nullptr || count > capacity) {
        return EINVAL;
    }
    std::memcpy(dest, src, count);
    return EOK;
}

extern "C" errno_t memset_s(void *dest, size_t capacity, int32_t value, size_t count)
{
    if (dest == nullptr || count > capacity) {
        return EINVAL;
    }
    std::memset(dest, value, count);
    return EOK;
}

extern "C" void *HksMalloc(size_t size)
{
    return std::malloc(size);
}

extern "C" void HksFreeImpl(void *address)
{
    std::free(address);
}

extern "C" int32_t HksMemCmp(const void *first, const void *second, uint32_t size)
{
    return std::memcmp(first, second, size);
}
