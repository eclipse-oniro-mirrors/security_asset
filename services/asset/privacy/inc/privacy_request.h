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

#ifndef ASSET_PRIVACY_REQUEST_H
#define ASSET_PRIVACY_REQUEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct AssetPrivacyInput {
    uint32_t code;
    uint32_t uid;
    const uint8_t *data;
    uint32_t size;
};

struct AssetPrivacyOutput {
    uint8_t *data;
    uint32_t size;
};

// Borrowed buffers are valid for the synchronous call; output size is committed only on success.
int32_t AssetPrivacyRequest(const struct AssetPrivacyInput *input, struct AssetPrivacyOutput *output);

#ifdef __cplusplus
}
#endif

#endif // ASSET_PRIVACY_REQUEST_H
