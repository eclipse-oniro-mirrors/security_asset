/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ASSET_SYSTEM_TYPE_H
#define ASSET_SYSTEM_TYPE_H

/**
 * @file asset_system_type.h
 *
 * @brief Defines the enums, structs, and error codes used in the Asset APIs.
 * 
 * @since 11
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumerates the types of the asset attribute tags.

 */
typedef enum {
    /**
     * The asset attribute tag is a Boolean value.
     */
    ASSET_SYSTEM_TYPE_BOOL = 0x1 << 28,
    /**
     * The asset attribute tag is a number.
     */
    ASSET_SYSTEM_TYPE_NUMBER = 0x2 << 28,
    /**
     * The asset attribute tag is an array of bytes.
     */
    ASSET_SYSTEM_TYPE_BYTES = 0x3 << 28,
} AssetTagType;

/**
 * @brief Defines the mask used to obtain the type of the asset attribute tag.

 */
#define ASSET_SYSTEM_TAG_TYPE_MASK (0xF << 28)

/**
 * @brief Enumerates the asset attribute tags.

 */
typedef enum {
    /**
     * Sensitive user data in the form of bytes, such as passwords and tokens.
     */
    ASSET_SYSTEM_TAG_SECRET = ASSET_SYSTEM_TYPE_BYTES | 0x01,
    /**
     * Asset alias (identifier) in the form of bytes.
     */
    ASSET_SYSTEM_TAG_ALIAS = ASSET_SYSTEM_TYPE_BYTES | 0x02,
    /**
     * Time when the asset is accessible. The value is of the uint32 type, which is a 32-bit unsigned integer.
     */
    ASSET_SYSTEM_TAG_ACCESSIBILITY = ASSET_SYSTEM_TYPE_NUMBER | 0x03,
    /**
     * A Boolean value indicating whether the asset is available only with a lock screen password.
     */
    ASSET_SYSTEM_TAG_REQUIRE_PASSWORD_SET = ASSET_SYSTEM_TYPE_BOOL | 0x04,
    /**
     * User authentication type for the asset. The value is of the uint32 type.
     */
    ASSET_SYSTEM_TAG_AUTH_TYPE = ASSET_SYSTEM_TYPE_NUMBER | 0x05,
    /**
     * Validity period of the user authentication, in seconds. The value is of the uint32 type.
     */
    ASSET_SYSTEM_TAG_AUTH_VALIDITY_PERIOD = ASSET_SYSTEM_TYPE_NUMBER | 0x06,
    /**
     * Challenge value, in the form of bytes, used for anti-replay during the authentication.
     */
    ASSET_SYSTEM_TAG_AUTH_CHALLENGE = ASSET_SYSTEM_TYPE_BYTES | 0x07,
    /**
     * Authentication token, in the form of bytes, obtained after a successful user authentication.
     */
    ASSET_SYSTEM_TAG_AUTH_TOKEN = ASSET_SYSTEM_TYPE_BYTES | 0x08,
    /**
     * Asset synchronization type. The value is of the uint32 type.
     */
    ASSET_SYSTEM_TAG_SYNC_TYPE = ASSET_SYSTEM_TYPE_NUMBER | 0x10,
    /**
     * A Boolean value indicating whether the asset needs to be stored persistently.
     * The ohos.permission.STORE_PERSISTENT_DATA permission is required if <b>OH_Asset_Add</b> is called with this tag.
     *
     * @permission ohos.permission.STORE_PERSISTENT_DATA
     */
    ASSET_SYSTEM_TAG_IS_PERSISTENT = ASSET_SYSTEM_TYPE_BOOL | 0x11,
    /**
     * An immutable custom field, in the form of bytes.
     */
    ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_1 = ASSET_SYSTEM_TYPE_BYTES | 0x20,
    /**
     * An immutable custom field, in the form of bytes.
     */
    ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_2 = ASSET_SYSTEM_TYPE_BYTES | 0x21,
    /**
     * An immutable custom field, in the form of bytes.
     */
    ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_3 = ASSET_SYSTEM_TYPE_BYTES | 0x22,
    /**
     * An immutable custom field, in the form of bytes.
     */
    ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_4 = ASSET_SYSTEM_TYPE_BYTES | 0x23,
    /**
     * A mutable custom field, in the form of bytes.
     */
    ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_1 = ASSET_SYSTEM_TYPE_BYTES | 0x30,
    /**
     * A mutable custom field, in the form of bytes.
     */
    ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_2 = ASSET_SYSTEM_TYPE_BYTES | 0x31,
    /**
     * A mutable custom field, in the form of bytes.
     */
    ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_3 = ASSET_SYSTEM_TYPE_BYTES | 0x32,
    /**
     * A mutable custom field, in the form of bytes.
     */
    ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_4 = ASSET_SYSTEM_TYPE_BYTES | 0x33,
    /**
     * Return type of the queried asset. The value is of the uint32 type.
     */
    ASSET_SYSTEM_TAG_RETURN_TYPE = ASSET_SYSTEM_TYPE_NUMBER | 0x40,
    /**
     * Maximum number of assets that can be returned at a time if multiple asset records match the specified conditions.
     * The value is of the uint32 type.
     */
    ASSET_SYSTEM_TAG_RETURN_LIMIT = ASSET_SYSTEM_TYPE_NUMBER | 0x41,
    /**
     * Offset that indicates the start asset when multiple asset records are returned. The value is of the uint32 type.
     */
    ASSET_SYSTEM_TAG_RETURN_OFFSET = ASSET_SYSTEM_TYPE_NUMBER | 0x42,
    /**
     * Sorting order of the assets in the query result. The value is of the uint32 type.
     */
    ASSET_SYSTEM_TAG_RETURN_ORDERED_BY = ASSET_SYSTEM_TYPE_NUMBER | 0x43,
    /**
     * Policy used to resolve the conflict occurred when an asset is added. The value is of the uint32 type.
     */
    ASSET_SYSTEM_TAG_CONFLICT_RESOLUTION = ASSET_SYSTEM_TYPE_NUMBER | 0x44,
} AssetTag;

/**
 * @brief Enumerates the result codes used in the ASSET APIs.

 */
typedef enum {
    /**
     * The operation is successful.
     */
    ASSET_SYSTEM_SUCCESS = 0,
    /**
     * The caller does not have the required permission.
     */
    ASSET_SYSTEM_PERMISSION_DENIED = 201,
    /**
     * The parameter is invalid.
     */
    ASSET_SYSTEM_INVALID_ARGUMENT = 401,
    /**
     * The asset service is unavailable.
     */
    ASSET_SYSTEM_SERVICE_UNAVAILABLE = 24000001,
    /**
     * The asset is not found.
     */
    ASSET_SYSTEM_NOT_FOUND = 24000002,
    /**
     * The asset already exists.
     */
    ASSET_SYSTEM_DUPLICATED = 24000003,
    /**
     * The access to the asset is denied.
     */
    ASSET_SYSTEM_ACCESS_DENIED = 24000004,
    /**
     * The lock screen status does not match the access control type specified.
     */
    ASSET_SYSTEM_STATUS_MISMATCH = 24000005,
    /**
     * The system memory is insufficient.
     */
    ASSET_SYSTEM_OUT_OF_MEMORY = 24000006,
    /**
     * The asset is corrupted.
     */
    ASSET_SYSTEM_DATA_CORRUPTED = 24000007,
    /**
     * The database operation failed.
     */
    ASSET_SYSTEM_DATABASE_ERROR = 24000008,
    /**
     * The cryptography operation failed.
     */
    ASSET_SYSTEM_CRYPTO_ERROR = 24000009,
    /**
     * The inter-process communication (IPC) failed.
     */
    ASSET_SYSTEM_IPC_ERROR = 24000010,
    /**
     * The Bundle Manager service is abnormal.
     */
    ASSET_SYSTEM_BMS_ERROR = 24000011,
    /**
     * The Account service is abnormal.
     */
    ASSET_SYSTEM_ACCOUNT_ERROR = 24000012,
    /**
     * The Access Token service is abnormal.
     */
    ASSET_SYSTEM_ACCESS_TOKEN_ERROR = 24000013,
    /**
     * The file operation failed.
     */
    ASSET_SYSTEM_FILE_OPERATION_ERROR = 24000014,
    /**
     * The operation for obtaining the system time failed.
     */
    ASSET_SYSTEM_GET_SYSTEM_TIME_ERROR = 24000015,
    /**
     * The number of cached assets exceeds the limit.
     */
    ASSET_SYSTEM_LIMIT_EXCEEDED = 24000016,
    /**
     * The function is not supported.
     */
    ASSET_SYSTEM_UNSUPPORTED = 24000017,
} AssetResultCode;

/**
 * @brief Enumerates the types of the access control based on the lock screen status.

 */
typedef enum {
    /**
     * The asset can be accessed after the device is powered on.
     */
    ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON = 0,
    /**
     * The asset can be accessed only after the device is unlocked for the first time.
     */
    ASSET_SYSTEM_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED = 1,
    /**
     * The asset can be accessed only after the device is unlocked.
     */
    ASSET_SYSTEM_ACCESSIBILITY_DEVICE_UNLOCKED = 2,
} AssetAccessibility;

/**
 * @brief Enumerates the user authentication types supported for assets.

 */
typedef enum {
    /**
     * No user authentication is required before the asset is accessed.
     */
    ASSET_SYSTEM_AUTH_TYPE_NONE = 0x00,
    /**
     * The asset can be accessed if any user authentication (such as PIN, facial, or fingerprint authentication) is
     * successful.
     */
    ASSET_SYSTEM_AUTH_TYPE_ANY = 0xFF,
} AssetAuthType;

/**
 * @brief Enumerates the asset synchronization types.

 */
typedef enum {
    /**
     * Asset synchronization is not allowed.
     */
    ASSET_SYSTEM_SYNC_TYPE_NEVER = 0,
    /**
     * Asset synchronization is allowed only on the local device, for example, in data restoration on the local device.
     */
    ASSET_SYSTEM_SYNC_TYPE_THIS_DEVICE = 1 << 0,
    /**
     * Asset synchronization is allowed only between trusted devices, for example, in the case of cloning.
     */
    ASSET_SYSTEM_SYNC_TYPE_TRUSTED_DEVICE = 1 << 1,
} AssetSyncType;

/**
 * @brief Enumerates the policies for resolving the conflict (for example, duplicate alias) occurred when
 * an asset is added.

 */
typedef enum {
    /**
     * Overwrite the existing asset.
     */
    ASSET_SYSTEM_CONFLICT_OVERWRITE = 0,
    /**
     * Throw an exception for the service to perform subsequent processing.
     */
    ASSET_SYSTEM_CONFLICT_THROW_ERROR = 1,
} AssetConflictResolution;

/**
 * @brief Enumerates the types of the asset query result.

 */
typedef enum {
    /**
     * The query result contains the asset in plaintext and its attributes.
     */
    ASSET_SYSTEM_RETURN_ALL = 0,
    /**
     * The query result contains only the asset attributes.
     */
    ASSET_SYSTEM_RETURN_ATTRIBUTES = 1,
} AssetReturnType;

/**
 * @brief Defines an asset value in the forma of a binary array, that is, a variable-length byte array.
 */
typedef struct {
    /**
     * Size of the byte array.
     */
    uint32_t size;
    /**
     * Pointer to the byte array.
     */
    uint8_t *data;
} AssetBlob;

/**
 * @brief Defines the value (content) of an asset attribute.
 */
typedef union {
    /**
     * Asset of the Boolean type.
     */
    bool boolean;
    /**
     * Asset of the uint32 type.
     */
    uint32_t u32;
    /**
     * Asset of the bytes type.
     */
    AssetBlob blob;
} AssetValue;

/**
 * @brief Defines an asset attribute.
 */
typedef struct {
    /**
     * Tag of the asset attribute.
     */
    uint32_t tag;
    /**
     * Value of the asset attribute.
     */
    AssetValue value;
} AssetAttr;

/**
 * @brief Represents information about an asset.
 */
typedef struct {
    /**
     * Number of asset attributes.
     */
    uint32_t count;
    /**
     * Pointer to the array of the asset attributes.
     */
    AssetAttr *attrs;
} AssetResult;

/**
 * @brief Represents information about a set of assets.
 */
typedef struct {
    /**
     * Number of assets.
     */
    uint32_t count;
    /**
     * Pointer to the array of the assets.
     */
    AssetResult *results;
} AssetResultSet;

#ifdef __cplusplus
}
#endif

/** @} */
#endif // ASSET_SYSTEM_TYPE_H