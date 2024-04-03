/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HUKS_WRAPPER_H
#define HUKS_WRAPPER_H

#include <stdint.h>
#include "hks_api.h"
#include "hks_param.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t TAG_SIZE = 16;
static const uint32_t NONCE_SIZE = 12;

#define ARRAY_SIZE(arr) ((sizeof(arr)) / (sizeof((arr)[0])))
#define ASSET_ROOT_USER_UPPERBOUND 100

enum Accessibility {
    DEVICE_POWERED_ON = 0,
    DEVICE_FIRST_UNLOCKED = 1,
    DEVICE_UNLOCKED = 2,
};

struct KeyId {
    int32_t userId;
    struct HksBlob alias;
    enum Accessibility accessibility;
};

int32_t GenerateKey(const struct KeyId *keyId, bool needAuth, bool requirePasswordSet);
int32_t DeleteKey(const struct KeyId *keyId);
int32_t IsKeyExist(const struct KeyId *keyId);
int32_t EncryptData(const struct KeyId *keyId, const struct HksBlob *aad, const struct HksBlob *inData,
    struct HksBlob *outData);
int32_t DecryptData(const struct KeyId *keyId, const struct HksBlob *aad, const struct HksBlob *inData,
    struct HksBlob *outData);
int32_t InitKey(const struct KeyId *keyId, uint32_t validTime, struct HksBlob *challenge, struct HksBlob *handle);
int32_t ExecCrypt(const struct HksBlob *handle, const struct HksBlob *aad, const struct HksBlob *authToken,
    const struct HksBlob *inData, struct HksBlob *outData);
int32_t Drop(const struct HksBlob *handle);

#ifdef __cplusplus
}
#endif
#endif