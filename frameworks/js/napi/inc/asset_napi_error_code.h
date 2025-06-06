/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include <cstdint>
#include <unordered_map>

#include "asset_system_type.h"

namespace OHOS {
namespace Security {
namespace Asset {

const std::unordered_map<int32_t, const char *> ERR_MSGS = {
    { SEC_ASSET_SUCCESS, "The operation is successful." },
    { SEC_ASSET_PERMISSION_DENIED, "The caller doesn't have the permission." },
    { SEC_ASSET_NOT_SYSTEM_APPLICATION, "Non-system applications use system APIs." },
    { SEC_ASSET_INVALID_ARGUMENT, "The argument is invalid." },
    { SEC_ASSET_SERVICE_UNAVAILABLE, "The ASSET service is unavailable." },
    { SEC_ASSET_NOT_FOUND, "The asset is not found." },
    { SEC_ASSET_DUPLICATED, "The asset already exists." },
    { SEC_ASSET_ACCESS_DENIED, "Access to the asset is denied." },
    { SEC_ASSET_STATUS_MISMATCH, "The screen lock status does not match." },
    { SEC_ASSET_OUT_OF_MEMORY, "Insufficient memory." },
    { SEC_ASSET_DATA_CORRUPTED, "The asset is corrupted." },
    { SEC_ASSET_DATABASE_ERROR, "The database operation failed." },
    { SEC_ASSET_CRYPTO_ERROR, "The cryptography operation failed." },
    { SEC_ASSET_IPC_ERROR, "IPC failed." },
    { SEC_ASSET_BMS_ERROR, "Calling the Bundle Manager service failed." },
    { SEC_ASSET_ACCOUNT_ERROR, "Calling the OS Account service failed." },
    { SEC_ASSET_ACCESS_TOKEN_ERROR, "Calling the Access Token service failed." },
    { SEC_ASSET_FILE_OPERATION_ERROR, "The file operation failed." },
    { SEC_ASSET_GET_SYSTEM_TIME_ERROR, "Getting the system time failed." },
    { SEC_ASSET_LIMIT_EXCEEDED, "The cache exceeds the limit." },
    { SEC_ASSET_UNSUPPORTED, "The capability is not supported." },
    { SEC_ASSET_PARAM_VERIFICATION_FAILED, "Parameter verification failed." },
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