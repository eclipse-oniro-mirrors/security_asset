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

#ifndef ASSET_NAPI_ERROR_CODE_H
#define ASSET_NAPI_ERROR_CODE_H

#include <unordered_map>
#include <stdint.h>

#include "asset_type.h"

namespace OHOS {
namespace Security {
namespace Asset {

const std::unordered_map<int32_t, const char *> ERR_MSGS = {
    { ASSET_SUCCESS, "The operation is successful." },
    { ASSET_PERMISSION_DENIED, "The caller doesn't have the permission." },
    { ASSET_INVALID_ARGUMENT, "The argument is invalid." },
    { ASSET_UNSUPPORTED, "The capability is not supported." },
    { ASSET_SERVICE_UNAVAILABLE, "The ASSET Service is unavailable." },
    { ASSET_NOT_FOUND, "The queried Asset can not be found." },
    { ASSET_DUPLICATED, "The Asset already exists." },
    { ASSET_ACCESS_DENIED, "The access to Asset is denied." },
    { ASSET_STATUS_MISMATCH, "The screen lock status mismatches." },
    { ASSET_OUT_OF_MEMORY, "Insufficient memory." },
    { ASSET_DATA_CORRUPTED, "The Asset is corrupted." },
    { ASSET_DATABASE_ERROR, "The database operation is failed." },
    { ASSET_CRYPTO_ERROR, "The cryptography operation is failed." },
    { ASSET_IPC_ERROR, "IPC communication is failed." },
    { ASSET_BMS_ERROR, "The operation of calling Bundle Manager Service is failed." },
    { ASSET_ACCOUNT_ERROR, "The operation of calling OS Account Service is failed." },
    { ASSET_ACCESS_TOKEN_ERROR, "The operation of calling Access Token Service is failed." },
    { ASSET_FILE_OPERATION_ERROR, "The operation of file is failed." },
    { ASSET_GET_SYSTEM_TIME_ERROR, "The operation of getting system time is failed." },
    { ASSET_LIMIT_EXCEEDED, "The cache exceeds the limit." },
};

inline const char *GetErrorMessage(int32_t errCode)
{
    auto iter = ERR_MSGS.find(errCode);
    if (iter == ERR_MSGS.end()) {
        return "";
    }
    return ERR_MSGS.at(errCode);
}

} // Asset
} // Security
} // OHOS

#endif // ASSET_NAPI_ERROR_CODE_H