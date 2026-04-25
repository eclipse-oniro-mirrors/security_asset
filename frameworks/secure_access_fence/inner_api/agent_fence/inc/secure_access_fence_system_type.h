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

#ifndef SECURE_ACCESS_FENCE_SYSTEM_TYPE_H
#define SECURE_ACCESS_FENCE_SYSTEM_TYPE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumerates the result codes used in the SAF APIs.
 */
typedef enum {
    /// The operation is successful.
    SEC_SAF_SUCCESS = 0,

    /// The error code indicates that the caller doesn't have the permission.
    SEC_SAF_PERMISSION_DENIED = 201,

    /// The error code indicates that the caller is not system application.
    SEC_SAF_NOT_SYSTEM_APPLICATION = 202,

    /// The error code indicates that the SAF service is unavailable.
    SEC_SAF_SERVICE_UNAVAILABLE = 1023900001,

    /// The error code indicates that the ipc communication is abnormal.
    SEC_SAF_IPC_ERROR = 1023900002,

    /// The error code indicates that the operation of calling Bundle Manager Service is failed.
    SEC_SAF_BMS_ERROR = 1023900003,

    /// The error code indicates that the operation of calling OS Account Service is failed.
    SEC_SAF_ACCOUNT_ERROR = 1023900004,

    /// The error code indicates that the operation of calling userIAM Service is failed.
    SEC_SAF_USER_IAM_ERROR = 1023900005,

    /// The error code indicates that verifying the parameter failed.
    SEC_SAF_PARAM_VERICATION_FAILED = 1023900006,

    /// The error code indicates that file operation failed.
    SEC_SAF_FILE_OPERATION_ERROR = 1023900007,

    /// The error code indicates that file operation failed.
    SEC_SAF_SERVICE_IS_STOPPING = 1023901000,
} SecureAccessFenceResultCode;


#ifdef __cplusplus
}
#endif

#endif // SECURE_ACCESS_FENCE_SYSTEM_TYPE_H
