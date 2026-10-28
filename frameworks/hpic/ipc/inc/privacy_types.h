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

#ifndef HPIC_PRIVACY_TYPES_H
#define HPIC_PRIVACY_TYPES_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// These views cross the same-process C/Rust FFI only. Pointers are never written into Parcel.
// Input storage remains owned by the caller until the synchronous call returns.
struct PrivacyBlob {
    const uint8_t *data;
    uint32_t size;
};
struct PrivacyBlobList {
    const struct PrivacyBlob *data;
    uint32_t count;
};
struct PrivacyProtocol {
    uint32_t dataSetSize;
    uint32_t protocolType;
};
struct PrivacyTargetRequest {
    struct PrivacyProtocol protocol;
    uint32_t hashAlg;
    struct PrivacyBlob element;
};
struct PrivacySearchRequest {
    struct PrivacyProtocol protocol;
    struct PrivacyBlobList ciphertexts;
    uint32_t hasSymmetric;
    struct PrivacyBlobList symmetric;
};
// size is the writable capacity on entry and the actual length on success.
struct PrivacyBuffer {
    uint8_t *data;
    uint32_t size;
};
#ifdef __cplusplus
}
#endif
#endif // HPIC_PRIVACY_TYPES_H
