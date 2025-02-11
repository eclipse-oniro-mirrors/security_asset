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
    SEC_ASSET_TYPE_BOOL = 0x1 << 28,
    /**
     * The asset attribute tag is a number.
     */
    SEC_ASSET_TYPE_NUMBER = 0x2 << 28,
    /**
     * The asset attribute tag is an array of bytes.
     */
    SEC_ASSET_TYPE_BYTES = 0x3 << 28,
} AssetTagType;

/**
 * @brief Defines the mask used to obtain the type of the asset attribute tag.
 */
#define SEC_ASSET_TAG_TYPE_MASK (0xF << 28)

/**
 * @brief Enumerates the asset attribute tags.
 */
typedef enum {
    /**
     * Sensitive user data in the form of bytes, such as passwords and tokens.
     */
    SEC_ASSET_TAG_SECRET = SEC_ASSET_TYPE_BYTES | 0x01,
    /**
     * Asset alias (identifier) in the form of bytes.
     */
    SEC_ASSET_TAG_ALIAS = SEC_ASSET_TYPE_BYTES | 0x02,
    /**
     * Time when the asset is accessible. The value is of the uint32 type, which is a 32-bit unsigned integer.
     */
    SEC_ASSET_TAG_ACCESSIBILITY = SEC_ASSET_TYPE_NUMBER | 0x03,
    /**
     * A Boolean value indicating whether the asset is available only with a lock screen password.
     */
    SEC_ASSET_TAG_REQUIRE_PASSWORD_SET = SEC_ASSET_TYPE_BOOL | 0x04,
    /**
     * User authentication type for the asset. The value is of the uint32 type.
     */
    SEC_ASSET_TAG_AUTH_TYPE = SEC_ASSET_TYPE_NUMBER | 0x05,
    /**
     * Validity period of the user authentication, in seconds. The value is of the uint32 type.
     */
    SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD = SEC_ASSET_TYPE_NUMBER | 0x06,
    /**
     * Challenge value, in the form of bytes, used for anti-replay during the authentication.
     */
    SEC_ASSET_TAG_AUTH_CHALLENGE = SEC_ASSET_TYPE_BYTES | 0x07,
    /**
     * Authentication token, in the form of bytes, obtained after a successful user authentication.
     */
    SEC_ASSET_TAG_AUTH_TOKEN = SEC_ASSET_TYPE_BYTES | 0x08,
    /**
     * Asset synchronization type. The value is of the uint32 type.
     */
    SEC_ASSET_TAG_SYNC_TYPE = SEC_ASSET_TYPE_NUMBER | 0x10,
    /**
     * A Boolean value indicating whether the asset needs to be stored persistently.
     * The ohos.permission.STORE_PERSISTENT_DATA permission is required if <b>OH_Asset_Add</b> is called with this tag.
     *
     * @permission ohos.permission.STORE_PERSISTENT_DATA
     */
    SEC_ASSET_TAG_IS_PERSISTENT = SEC_ASSET_TYPE_BOOL | 0x11,
    /**
     * An immutable custom field, in the form of bytes.
     */
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1 = SEC_ASSET_TYPE_BYTES | 0x20,
    /**
     * An immutable custom field, in the form of bytes.
     */
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2 = SEC_ASSET_TYPE_BYTES | 0x21,
    /**
     * An immutable custom field, in the form of bytes.
     */
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3 = SEC_ASSET_TYPE_BYTES | 0x22,
    /**
     * An immutable custom field, in the form of bytes.
     */
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4 = SEC_ASSET_TYPE_BYTES | 0x23,
    /**
     * A mutable custom field, in the form of bytes.
     */
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_1 = SEC_ASSET_TYPE_BYTES | 0x30,
    /**
     * A mutable custom field, in the form of bytes.
     */
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_2 = SEC_ASSET_TYPE_BYTES | 0x31,
    /**
     * A mutable custom field, in the form of bytes.
     */
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_3 = SEC_ASSET_TYPE_BYTES | 0x32,
    /**
     * A mutable custom field, in the form of bytes.
     */
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_4 = SEC_ASSET_TYPE_BYTES | 0x33,
    /**
     * A mutable custom field, in the form of bytes. The information of a local tag will not be synchronized.
     */
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1 = SEC_ASSET_TYPE_BYTES | 0x34,
    /**
     * A mutable custom field, in the form of bytes. The information of a local tag will not be synchronized.
     */
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2 = SEC_ASSET_TYPE_BYTES | 0x35,
    /**
     * A mutable custom field, in the form of bytes. The information of a local tag will not be synchronized.
     */
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3 = SEC_ASSET_TYPE_BYTES | 0x36,
    /**
     * A mutable custom field, in the form of bytes. The information of a local tag will not be synchronized.
     */
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4 = SEC_ASSET_TYPE_BYTES | 0x37,
    /**
     * Return type of the queried asset. The value is of the uint32 type.
     */
    SEC_ASSET_TAG_RETURN_TYPE = SEC_ASSET_TYPE_NUMBER | 0x40,
    /**
     * Maximum number of assets that can be returned at a time if multiple asset records match the specified conditions.
     * The value is of the uint32 type.
     */
    SEC_ASSET_TAG_RETURN_LIMIT = SEC_ASSET_TYPE_NUMBER | 0x41,
    /**
     * Offset that indicates the start asset when multiple asset records are returned. The value is of the uint32 type.
     */
    SEC_ASSET_TAG_RETURN_OFFSET = SEC_ASSET_TYPE_NUMBER | 0x42,
    /**
     * Sorting order of the assets in the query result. The value is of the uint32 type.
     */
    SEC_ASSET_TAG_RETURN_ORDERED_BY = SEC_ASSET_TYPE_NUMBER | 0x43,
    /**
     * Policy used to resolve the conflict occurred when an asset is added. The value is of the uint32 type.
     */
    SEC_ASSET_TAG_CONFLICT_RESOLUTION = SEC_ASSET_TYPE_NUMBER | 0x44,
    /**
     * A tag whose value is a byte array indicating the update time of an Asset.
     */
    SEC_ASSET_TAG_UPDATE_TIME = SEC_ASSET_TYPE_BYTES | 0x45,
    /**
     * A tag whose value is the uint32 type indicating the additional action.
     */
    SEC_ASSET_TAG_OPERATION_TYPE = SEC_ASSET_TYPE_NUMBER | 0x46,
    /**
     * A tag whose value is a bool indicating whether the attributes of an asset are required to be encrypted.
     */
    SEC_ASSET_TAG_REQUIRE_ATTR_ENCRYPTED = SEC_ASSET_TYPE_BOOL | 0x47,
    /**
     * A tag whose value is a byte array indicating the group id an asset belongs to.
     */
    SEC_ASSET_TAG_GROUP_ID = SEC_ASSET_TYPE_BYTES | 0x48,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the type of Asset encapsulation.
     */
    SEC_ASSET_TAG_WRAP_TYPE = SEC_ASSET_TYPE_NUMBER | 0x49,
    /**
     * Tag used to store specific user id. The value is of the uint32 type.
     */
    SEC_ASSET_TAG_USER_ID = SEC_ASSET_TYPE_NUMBER | 0x100,
} AssetTag;

/**
 * @brief Enumerates the result codes used in the ASSET APIs.
 */
typedef enum {
    /**
     * The operation is successful.
     */
    SEC_ASSET_SUCCESS = 0,
    /**
     * The caller does not have the required permission.
     */
    SEC_ASSET_PERMISSION_DENIED = 201,
    /**
     * The caller not system application.
     */
    SEC_ASSET_NOT_SYSTEM_APPLICATION = 202,
    /**
     * The parameter is invalid.
     */
    SEC_ASSET_INVALID_ARGUMENT = 401,
    /**
     * The asset service is unavailable.
     */
    SEC_ASSET_SERVICE_UNAVAILABLE = 24000001,
    /**
     * The asset is not found.
     */
    SEC_ASSET_NOT_FOUND = 24000002,
    /**
     * The asset already exists.
     */
    SEC_ASSET_DUPLICATED = 24000003,
    /**
     * The access to the asset is denied.
     */
    SEC_ASSET_ACCESS_DENIED = 24000004,
    /**
     * The lock screen status does not match the access control type specified.
     */
    SEC_ASSET_STATUS_MISMATCH = 24000005,
    /**
     * The system memory is insufficient.
     */
    SEC_ASSET_OUT_OF_MEMORY = 24000006,
    /**
     * The asset is corrupted.
     */
    SEC_ASSET_DATA_CORRUPTED = 24000007,
    /**
     * The database operation failed.
     */
    SEC_ASSET_DATABASE_ERROR = 24000008,
    /**
     * The cryptography operation failed.
     */
    SEC_ASSET_CRYPTO_ERROR = 24000009,
    /**
     * The inter-process communication (IPC) failed.
     */
    SEC_ASSET_IPC_ERROR = 24000010,
    /**
     * The Bundle Manager service is abnormal.
     */
    SEC_ASSET_BMS_ERROR = 24000011,
    /**
     * The Account service is abnormal.
     */
    SEC_ASSET_ACCOUNT_ERROR = 24000012,
    /**
     * The Access Token service is abnormal.
     */
    SEC_ASSET_ACCESS_TOKEN_ERROR = 24000013,
    /**
     * The file operation failed.
     */
    SEC_ASSET_FILE_OPERATION_ERROR = 24000014,
    /**
     * The operation for obtaining the system time failed.
     */
    SEC_ASSET_GET_SYSTEM_TIME_ERROR = 24000015,
    /**
     * The number of cached assets exceeds the limit.
     */
    SEC_ASSET_LIMIT_EXCEEDED = 24000016,
    /**
     * The function is not supported.
     */
    SEC_ASSET_UNSUPPORTED = 24000017,
} AssetResultCode;

/**
 * @brief Enumerates the types of the access control based on the lock screen status.
 */
typedef enum {
    /**
     * The asset can be accessed after the device is powered on.
     */
    SEC_ASSET_ACCESSIBILITY_DEVICE_POWERED_ON = 0,
    /**
     * The asset can be accessed only after the device is unlocked for the first time.
     */
    SEC_ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED = 1,
    /**
     * The asset can be accessed only after the device is unlocked.
     */
    SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED = 2,
} AssetAccessibility;

/**
 * @brief Enumerates the user authentication types supported for assets.
 */
typedef enum {
    /**
     * No user authentication is required before the asset is accessed.
     */
    SEC_ASSET_AUTH_TYPE_NONE = 0x00,
    /**
     * The asset can be accessed if any user authentication (such as PIN, facial, or fingerprint authentication) is
     * successful.
     */
    SEC_ASSET_AUTH_TYPE_ANY = 0xFF,
} AssetAuthType;

/**
 * @brief Enumerates the asset synchronization types.
 */
typedef enum {
    /**
     * Asset synchronization is not allowed.
     */
    SEC_ASSET_SYNC_TYPE_NEVER = 0,
    /**
     * Asset synchronization is allowed only on the local device, for example, in data restoration on the local device.
     */
    SEC_ASSET_SYNC_TYPE_THIS_DEVICE = 1 << 0,
    /**
     * Asset synchronization is allowed only between trusted devices, for example, in the case of cloning.
     */
    SEC_ASSET_SYNC_TYPE_TRUSTED_DEVICE = 1 << 1,
    /**
     * Asset synchronization is allowed only between trusted devices, for example, in the case of cloning.
     */
    SEC_ASSET_SYNC_TYPE_TRUSTED_ACCOUNT = 1 << 2,
} AssetSyncType;

/**
 * @brief An enum type indicates the type of Asset encapsulation.
 */
typedef enum {
    /**
     * An Asset with this attribute value is never allowed to be wrapped up.
     */
    SEC_ASSET_WRAP_TYPE_NEVER = 0,
    /**
     * An Asset with this attribute value can only be wrapped or unwrapped on devices logged in with trusted accounts.
     */
    SEC_ASSET_WRAP_TYPE_TRUSTED_ACCOUNT = 1,
} AssetWrapType;

/**
 * @brief Enumerates the policies for resolving the conflict (for example, duplicate alias) occurred when
 * an asset is added.
 */
typedef enum {
    /**
     * Overwrite the existing asset.
     */
    SEC_ASSET_CONFLICT_OVERWRITE = 0,
    /**
     * Throw an exception for the service to perform subsequent processing.
     */
    SEC_ASSET_CONFLICT_THROW_ERROR = 1,
} AssetConflictResolution;

/**
 * @brief Enumerates the types of the asset query result.
 */
typedef enum {
    /**
     * The query result contains the asset in plaintext and its attributes.
     */
    SEC_ASSET_RETURN_ALL = 0,
    /**
     * The query result contains only the asset attributes.
     */
    SEC_ASSET_RETURN_ATTRIBUTES = 1,
} AssetReturnType;

/**
 * @brief Enumerates the types of the asset query result.
 */
typedef enum {
    /**
     * Trigger Sync.
     */
    SEC_ASSET_NEED_SYNC = 0,
    /**
     * Logout account to clean cloud flag.
     */
    SEC_ASSET_NEED_LOGOUT = 1,
    /**
     * Delete cloud data.
     */
    SEC_ASSET_NEED_DELETE_CLOUD_DATA = 2,
} AssetOperationType;

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

#endif // ASSET_SYSTEM_TYPE_H