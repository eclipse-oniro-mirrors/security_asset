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

#ifndef CRYPTO_WRAPPER
#define CRYPTO_WRAPPER

#include "hks_api.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CryptParam {
    enum HksKeyPurpose cryptoMode;
    uint32_t challengePos;
    uint32_t expTime;
    const struct HksBlob *aadData;
    const struct HksBlob *authToken;
};

/* once encrypt&decrypt */
int32_t EncryptWrapper(const struct HksBlob *keyData, const struct HksBlob *aadData,
        const struct HksBlob *inData, struct HksBlob *outData);
int DecryptWrapper(const struct HksBlob *keyData, const struct HksBlob *aadData,
        const struct HksBlob *inData, struct HksBlob *outData);

/* multi encrypt&decrypt */
int32_t InitCryptoWrapper(const struct CryptParam *param, const struct HksBlob *key_data,
    struct HksBlob *challenge_data, struct HksBlob *handle_data);
int32_t ExecCryptoWrapper(const CryptParam *param, const struct HksBlob *handleData,
    const struct HksBlob *inData, struct HksBlob *outData);
int32_t DropCrypto(const CryptParam *param, struct HksBlob *handle_data);

#ifdef __cplusplus
}
#endif

#endif