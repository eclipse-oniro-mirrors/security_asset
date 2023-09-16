/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ASSET_TYPE_H
#define ASSET_TYPE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An enum type containing the data type definitions for Asset attribute value.
 */
typedef enum {
    /**
     * The data type of Asset attribute value is bool.
     */
    ASSET_TYPE_BOOL = 0x1 << 28,
    /**
     * The data type of Asset attribute value is uint32.
     */
    ASSET_TYPE_UINT32 = 0x2 << 28,
    /**
     * The data type of Asset attribute value is byte array.
     */
    ASSET_TYPE_BYTES = 0x3 << 28,
} AssetTagType;

/**
 * The mask used to obtain the data type of Asset attribute value.
 */
#define ASSET_TAG_TYPE_MASK (0xF << 28)

/**
 * An enum type containing the Asset attribute tags.
 */
typedef enum {
    /**
     * A tag whose value is a byte array indicating the sensitive user data such as passwords and tokens.
     */
    ASSET_TAG_SECRET = ASSET_TYPE_BYTES | 0x01,
    /**
     * A tag whose value is a byte array identifying an Asset.
     */
    ASSET_TAG_ALIAS = ASSET_TYPE_BYTES | 0x02,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating when the Asset can be accessed.
     */
    ASSET_TAG_ACCESSIBILITY = ASSET_TYPE_UINT32 | 0x03,
    /**
     * A tag whose value is a bool indicating whether a screen lock password is set for the device.
     */
    ASSET_TAG_REQUIRE_PASSWORD_SET = ASSET_TYPE_BOOL | 0x04,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the user authentication type for Asset access control.
     */
    ASSET_TAG_AUTH_TYPE = ASSET_TYPE_UINT32 | 0x05,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the validity period in seconds of user authentication.
     */
    ASSET_TAG_AUTH_VALIDITY_PERIOD = ASSET_TYPE_UINT32 | 0x06,
    /**
     * A tag whose value is a byte array indicating the authentication challenge for anti-replay protection.
     */
    ASSET_TAG_AUTH_CHALLENGE = ASSET_TYPE_BYTES | 0x07,
    /**
     * A tag whose value is a byte array indicating the authentication token after a user is verified.
     */
    ASSET_TAG_AUTH_TOKEN = ASSET_TYPE_BYTES | 0x08,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the type of Asset synchronization.
     */
    ASSET_TAG_SYNC_TYPE = ASSET_TYPE_UINT32 | 0x10,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the strategy for resolving Asset conflicts.
     */
    ASSET_TAG_CONFLICT_RESOLUTION = ASSET_TYPE_UINT32 | 0x11,
    /**
     * A tag whose value is a byte array indicating the first user-defined Asset data label (not allow to update).
     */
    ASSET_TAG_DATA_LABLE_CRITICAL_1 = ASSET_TYPE_BYTES | 0x20,
    /**
     * A tag whose value is a byte array indicating the second user-defined Asset data label (not allow to update).
     */
    ASSET_TAG_DATA_LABLE_CRITICAL_2 = ASSET_TYPE_BYTES | 0x21,
    /**
     * A tag whose value is a byte array indicating the third user-defined Asset data label (not allow to update).
     */
    ASSET_TAG_DATA_LABLE_CRITICAL_3 = ASSET_TYPE_BYTES | 0x22,
    /**
     * A tag whose value is a byte array indicating the fourth user-defined Asset data label (not allow to update).
     */
   ASSET_TAG_DATA_LABLE_CRITICAL_4 = ASSET_TYPE_BYTES | 0x23,
    /**
     * A tag whose value is a byte array indicating the first user-defined Asset data label (allow to update).
     */
    ASSET_TAG_DATA_LABLE_NORMAL_1 = ASSET_TYPE_BYTES | 0x24,
    /**
     * A tag whose value is a byte array indicating the second user-defined Asset data label (allow to update).
     */
    ASSET_TAG_DATA_LABLE_NORMAL_2 = ASSET_TYPE_BYTES | 0x25,
    /**
     * A tag whose value is a byte array indicating the third user-defined Asset data label (allow to update).
     */
    ASSET_TAG_DATA_LABLE_NORMAL_3 = ASSET_TYPE_BYTES | 0x26,
    /**
     * A tag whose value is a byte array indicating the fourth user-defined Asset data label (allow to update).
     */
    ASSET_TAG_DATA_LABLE_NORMAL_4 = ASSET_TYPE_BYTES | 0x27,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the return type of the queried Asset.
     */
    ASSET_TAG_RETURN_TYPE = ASSET_TYPE_UINT32 | 0x30,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the maximum number of returned Assets in one query.
     */
    ASSET_TAG_RETURN_LIMIT = ASSET_TYPE_UINT32 | 0x31,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the offset of return data in batch query.
     */
    ASSET_TAG_RETURN_OFFSET = ASSET_TYPE_UINT32 | 0x32,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating how the query results are sorted.
     */
    ASSET_TAG_RETURN_ORDER_BY = ASSET_TYPE_UINT32 | 0x33,
} AssetTag;

/**
 * An enum type containing the Asset result codes.
 */
typedef enum {
    /**
     * The result code indicates that the operation is successful.
     */
    ASSET_SUCCESS = 0,
    /**
     * The error code indicates that the caller doesn't have permission to operate.
     */
    ASSET_PERMISSION_DENIED = 201,
    /**
     * The error code indicates that the argument is invalid.
     */
    ASSET_INVALID_ARGUMENT = 401,
    /**
     * The error code indicates that the capability is not supported.
     */
    ASSET_NOT_SUPPORTED = 801,
    /**
     * The error code indicates that the Asset service is unavailable.
     */
    ASSET_SERVICE_UNAVAILABLE = 24000001,
    /**
     * The error code indicates that the queried Asset can not be found.
     */
    ASSET_NOT_FOUND = 24000002,
    /**
     * The error code indicates that the added Asset already exists.
     */
    ASSET_DUPLICATED = 24000003,
    /**
     * The error code indicates that the access to Asset is denied.
     */
    ASSET_ACCESS_DENIED = 24000004,
    /**
     * The error code indicates that the authentication token has expired.
     */
    ASSET_AUTH_TOKEN_EXPIRED = 24000005,
    /**
     * The error code indicates insufficient memory.
     */
    ASSET_OUT_OF_MEMRORY = 24000006,
    /**
     * The error code indicates that the Asset or encryption key is corrupted.
     */
    ASSET_DATA_CORRUPTED = 24000007,
} ErrCode;

/**
 * An enum type indicates when the Asset is accessible.
 */
typedef enum {
    /**
     * The secret value in the Asset can only be accessed after the device is first unlocked.
     */
    ACCESSIBILITY_DEVICE_FIRST_UNLOCK = 1,
    /**
     * The secret value in the Asset can only be accessed while the device is unlocked.
     */
    ACCESSIBILITY_DEVICE_UNLOCK = 2,
    /**
     * The secret value in the Asset can only be accessed
     * when the device is unlocked and a PIN/pattern/password is set on the device.
     */
    ACCESSIBILITY_DEVICE_SECURE = 3,
} AssetAccessibility;

/**
 * An enum type indicates the user authentication type for Asset access control.
 */
typedef enum {
    /**
     * The access to an Asset doesn't require user authentication.
     */
    AUTH_TYPE_NONE = 0x00,
    /**
     * The access to an Asset requires user authentication using either PIN/pattern/password or biometric traits.
     */
    AUTH_TYPE_ANY = 0xFF,
} AuthType;

/**
 * An enum type indicates the type of Asset synchronization.
 */
typedef enum {
    /**
     * An Asset with this attribute value is never allowed to be transferred out.
     */
    SYNC_TYPE_NEVER = 0,
    /**
     * An Asset with this attribute value can only be restored to the device from which it was transferred out.
     */
    SYNC_TYPE_THIS_DEVICE = 1 << 0,
    /**
     * An Asset with this attribute value can only be transferred out to a device of trusted account.
     */
    SYNC_TYPE_TRUSTED_ACCOUNT = 1 << 1,
    /**
     * An Asset with this attribute value can only be transferred out to a trusted device (user authorized).
     */
    SYNC_TYPE_TRUSTED_DEVICE = 1 << 2,
} SyncType;

/**
 * An enum type indicates the strategy for conflict resolution when handling duplicated Asset alias.
 */
typedef enum {
    /**
     * Directly overwrite an Asset with duplicated alias when a conflict is detected.
     */
    CONFLICT_OVERWRITE = 0,
    /**
     * Throw an error so that the caller can take measures when a conflict is detected.
     */
    CONFLICT_THROW_ERROR = 1,
} ConflictResolution;

/**
 * An enum type indicates the return type of the queried Asset.
 */
typedef enum {
    /**
     * Specify that the return data should contain both secret value and attributes.
     */
    RETURN_ALL = 0,
    /**
     * Specify that the return data contains only attributes.
     */
    RETURN_ATTRIBUTES = 1,
} ReturnType;

/**
 * The version structure returned by {@link GetVersion} function.
 */
typedef struct {
    /**
     * The major version.
     */
    uint32_t major;
    /**
     * The minor version.
     */
    uint32_t minor;
    /**
     * The patch version.
     */
    uint32_t patch;
} Version;

/**
 * A type that indicates the Asset attribute whose value is a byte array.
 */
typedef struct {
    /**
     * The size of byte array.
     */
    uint32_t size;
    /**
     * The data of byte array.
     */
    uint8_t *data;
} AssetBlob;

/**
 * A type that indicates the secret or attribute value of an Asset tag.
 */
typedef union {
    /**
     * Value of the asset attribute whose data type is bool.
     */
    bool boolean;
    /**
     * Value of the asset attribute whose data type is uint32.
     */
    uint32_t u32;
    /**
     * Value of the asset attribute whose data type is byte array.
     */
    AssetBlob blob;
} AssetValue;

/**
 * A type that indicates the tag-value pair of the Asset attribute.
 */
typedef struct {
    /**
     * The tag of the Asset attribute.
     */
    uint32_t tag;
    /**
     * The value of the Asset attribute.
     */
    AssetValue value;
} AssetAttr;

/**
 * A type that indicates the query result of an Asset record.
 */
typedef struct {
    /**
     * The attributes of an Asset.
     */
    AssetAttr *attrs;
    /**
     * The count of an Asset attributes.
     */
    uint32_t count;
} AssetResult;

/**
 * A type that indicates the Asset query result set.
 */
typedef struct {
    /**
     * The query result set.
     */
    AssetResult *results;
    /**
     * The count of the result set.
     */
    uint32_t count;
} AssetResultSet;

#ifdef __cplusplus
}
#endif

#endif // ASSET_TYPE_H