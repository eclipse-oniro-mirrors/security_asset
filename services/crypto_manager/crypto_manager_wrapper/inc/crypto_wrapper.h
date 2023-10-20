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
    uint32_t keyLen;
    const uint8_t *keyData;
    uint32_t challengePos;
    uint32_t challengeLen;
    uint8_t *challengeData;
    enum HksKeyPurpose cryptoMode;
    uint32_t handleLen;
    uint8_t *handleData;
    uint32_t aadLen;
    const uint8_t *aad;
    uint32_t dataInLen;
    const uint8_t *dataIn;
    uint32_t dataOutLen;
    uint8_t *dataOut;
};

/* once encrypt&decrypt */
int EncryptWrapper(const struct CryptParam *data);
int DecryptWrapper(const struct CryptParam *data);

/* multi encrypt&decrypt */
int32_t InitCryptoWrapper(const struct CryptParam *data);
int32_t ExecCryptoWrapper(const struct CryptParam *data);
int32_t DropCrypto(const struct CryptParam *data);

#ifdef __cplusplus
}
#endif

#endif