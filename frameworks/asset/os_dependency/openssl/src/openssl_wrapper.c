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

#include "openssl_wrapper.h"

#include <openssl/rand.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>

#include "securec.h"

#include "asset_log.h"

// The caller should ensure the memory safety, that the points should point at valid memory.
void Sha256(const uint8_t *input, uint32_t intputLen, uint8_t *output)
{
    if (input == NULL || intputLen == 0 || output == NULL) {
        LOGE("invalid input for sha256");
        return;
    }

    (void)SHA256((const unsigned char *)input, intputLen, (unsigned char *)output);
}

int32_t GenerateRandom(uint8_t *random, uint32_t randomLen)
{
    if (RAND_priv_bytes(random, randomLen) < 0) {
        LOGE("Generate random failed!");
        return -1;
    }
    return 0;
}
