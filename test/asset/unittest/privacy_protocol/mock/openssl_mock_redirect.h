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

#ifndef ASSET_PRIVACY_OPENSSL_MOCK_REDIRECT_H
#define ASSET_PRIVACY_OPENSSL_MOCK_REDIRECT_H

#include "securec.h"
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <cstdint>

// The unit test objects resolve these calls to the mock library.
#pragma redefine_extname EVP_CIPHER_CTX_new MockCipherNew
#pragma redefine_extname EVP_CIPHER_CTX_ctrl MockCipherCtrl
#pragma redefine_extname EVP_EncryptInit_ex MockEncryptInit
#pragma redefine_extname EVP_EncryptUpdate MockEncryptUpdate
#pragma redefine_extname EVP_DecryptInit_ex MockDecryptInit
#pragma redefine_extname EVP_DecryptUpdate MockDecryptUpdate
#pragma redefine_extname RAND_priv_bytes MockRandom
#pragma redefine_extname EVP_PKEY_CTX_new_id MockPkeyNew
#pragma redefine_extname EVP_PKEY_derive_init MockDeriveInit
#pragma redefine_extname EVP_EncryptFinal_ex MockEncryptFinal
#pragma redefine_extname EVP_DecryptFinal_ex MockDecryptFinal
#pragma redefine_extname EVP_Digest MockDigest
#pragma redefine_extname EVP_PKEY_derive MockDerive
#pragma redefine_extname memcpy_s MockCopy
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#pragma redefine_extname EVP_PKEY_CTX_set_hkdf_mode MockHkdfMode
#pragma redefine_extname EVP_PKEY_CTX_set_hkdf_md MockHkdfDigest
#pragma redefine_extname EVP_PKEY_CTX_set1_hkdf_salt MockHkdfSalt
#pragma redefine_extname EVP_PKEY_CTX_set1_hkdf_key MockHkdfKey
#pragma redefine_extname EVP_PKEY_CTX_add1_hkdf_info MockHkdfInfo
#else
#pragma redefine_extname EVP_PKEY_CTX_ctrl MockHkdfCtrl
#endif
#endif
