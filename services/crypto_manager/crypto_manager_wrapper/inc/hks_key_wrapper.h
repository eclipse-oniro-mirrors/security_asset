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

#ifndef HKS_KEY_WRAPPER
#define HKS_KEY_WRAPPER

#include "hks_api.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t InitParamSet(struct HksParamSet **paramSet, const struct HksParam *params, uint32_t paramcount);
int32_t GenerateKey(const struct HksBlob *keyData, bool needUserAuth);
int32_t DeleteKey(const struct HksBlob *keyData);
int32_t KeyExist(const struct HksBlob *keyData);

#ifdef __cplusplus
}
#endif

#endif