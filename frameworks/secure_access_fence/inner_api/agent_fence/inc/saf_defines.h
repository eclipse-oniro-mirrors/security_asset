/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SAF_DEFINES_H
#define SAF_DEFINES_H

#include "saf_result_defs.h"
#include "saf_result_code.h"
#include "saf_log.h"

namespace OHOS::Security::SAF {

#define IF_TRUE_LOGE_RETURN_NULL(ret, errMsg, ...)          \
do {                                                        \
    decltype(ret) _expr_result = (ret);                     \
    if ((_expr_result)) {                                   \
        LOGE(errMsg, ##__VA_ARGS__);                        \
        return nullptr;                                     \
    }                                                       \
} while (0)

#define IF_TRUE_LOGE_RETURN_ERR(ret, errCode, errMsg, ...)  \
do {                                                        \
    decltype(ret) _expr_result = (ret);                     \
    if ((_expr_result)) {                                   \
        LOGE(errMsg, ##__VA_ARGS__);                        \
        return errCode;                                     \
    }                                                       \
} while (0)

#define IF_ERROR_LOGE_RETURN_ERR(ret, errCode, errMsg, ...) \
do {                                                        \
    decltype(ret) _expr_result = (ret);                     \
    if ((_expr_result) != SAF_SUCCESS) {                    \
        LOGE(errMsg, ##__VA_ARGS__);                        \
        return errCode;                                     \
    }                                                       \
} while (0)

} // namespace OHOS::Security::SAF
#endif // SAF_DEFINES_H