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

#ifndef ASSET_PRIVACY_HIFHE_API_MOCK_REDIRECT_H
#define ASSET_PRIVACY_HIFHE_API_MOCK_REDIRECT_H

#include "hifhe.h"
#include <cstdint>

// The unit test objects resolve these calls to the mock library.
#pragma redefine_extname HIFHE_Encode MockEncode
#pragma redefine_extname HIFHE_Encrypt MockEncrypt
#pragma redefine_extname HIFHE_Decrypt MockDecrypt
#pragma redefine_extname HIFHE_Deserialize MockDeserialize
#pragma redefine_extname HIFHE_Add MockAdd
#pragma redefine_extname HIFHE_Mul MockMul
#pragma redefine_extname HIFHE_Ctrl MockCtrl
#endif
