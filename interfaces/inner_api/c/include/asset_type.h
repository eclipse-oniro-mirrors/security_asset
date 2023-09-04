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
 * An enum type that indicates the type of the asset attribute value.
 */
typedef enum {
    /**
     * The type of the asset attribute value is int32.
     */
    ASSET_TYPE_INT32 = 1 << 28,
    /**
     * The type of the asset attribute value is uint32.
     */
    ASSET_TYPE_UINT32 = 2 << 28,
    /**
     * The type of the asset attribute value is int64.
     */
    ASSET_TYPE_INT64 = 3 << 28,
    /**
     * The type of the asset attribute value is uint64.
     */
    ASSET_TYPE_UINT64 = 4 << 28,
    /**
     * The type of the asset attribute value is bool.
     */
    ASSET_TYPE_BOOL = 5 << 28,
    /**
     * The type of the asset attribute value is byte array.
     */
    ASSET_TYPE_BYTES = 6 << 28,
} AssetTagType;

#define ASSET_TAG_TYPE_MASK (0xF << 28)

/**
 * An emum type that indicates the tag of the asset attribute.
 */
typedef enum {
    /**
     * A tag whose value is the asset, such as password and token.
     */
    ASSET_TAG_SECRET = ASSET_TYPE_BYTES | 1,
    /**
     * A tag whose value used to identify an asset.
     */
    ASSET_TAG_ALIAS = ASSET_TYPE_BYTES | 2,
    /**
     * A tag whose value indicates when the asset can be accessed.
     */
    ASSET_TAG_ACCESSIBILITY = ASSET_TYPE_UINT32 | 3,
    /**
     * A tag whose value indicates what type of user authentication is required.
     */
    ASSET_TAG_AUTH_TYPE = ASSET_TYPE_UINT32 | 4,
    /**
     * A tag whose value indicates the validity period of user authentication, in seconds.
     */
    ASSET_TAG_AUTH_VALIDITY_PERIOD = ASSET_TYPE_UINT32 | 5,
    /**
     * A tag whose value indicates the authentication challenge for anti-replay.
     */
    ASSET_TAG_AUTH_CHALLENGE = ASSET_TYPE_BYTES | 6,
    /**
     * A tag whose value indicates the credential after successful authentication of the user.
     */
    ASSET_TAG_AUTH_TOKEN = ASSET_TYPE_BYTES | 7,
    /**
     * A tag whose value indicates the type of asset synchronization.
     */
    ASSET_TAG_SYNC_TYPE = ASSET_TYPE_UINT32 | 8,
    /**
     * A tag whose value indicates the conflict handling policy for adding the asset with the same alias.
     */
    ASSET_TAG_CONFLICT_POLICY = ASSET_TYPE_UINT32 | 9,
    /**
     * A tag whose value indicates the first customized critical data of the asset.
     */
    ASSET_TAG_DATA_LABLE_CRITICAL_1 = ASSET_TYPE_BYTES | 10,
    /**
     * A tag whose value indicates the second customized critical data of the asset.
     */
    ASSET_TAG_DATA_LABLE_CRITICAL_2 = ASSET_TYPE_BYTES | 11,
    /**
     * A tag whose value indicates the third customized critical data of the asset.
     */
    ASSET_TAG_DATA_LABLE_CRITICAL_3 = ASSET_TYPE_BYTES | 12,
    /**
     * A tag whose value indicates the fourth customized critical data of the asset.
     */
   ASSET_TAG_DATA_LABLE_CRITICAL_4 = ASSET_TYPE_BYTES | 13,
    /**
     * A tag whose value indicates the first customized normal data of the asset.
     */
    ASSET_TAG_DATA_LABLE_NORMAL_1 = ASSET_TYPE_BYTES | 14,
    /**
     * A tag whose value indicates the second customized normal data of the asset.
     */
    ASSET_TAG_DATA_LABLE_NORMAL_2 = ASSET_TYPE_BYTES | 15,
    /**
     * A tag whose value indicates the third customized normal data of the asset.
     */
    ASSET_TAG_DATA_LABLE_NORMAL_3 = ASSET_TYPE_BYTES | 16,
    /**
     * A tag whose value indicates the fourth customized normal data of the asset.
     */
    ASSET_TAG_DATA_LABLE_NORMAL_4 = ASSET_TYPE_BYTES | 17,
    /**
     * A tag whose value indicates the type of the returned data.
     */
    ASSET_TAG_RETURN_TYPE = ASSET_TYPE_UINT32 | 18,
    /**
     * A tag whose value indicates the maximum number of assets that can be returned in a query.
     */
    ASSET_TAG_RETURN_LIMIT = ASSET_TYPE_UINT32 | 19,
    /**
     * A tag whose value indicates the offset of the batch query result.
     */
    ASSET_TAG_RETURN_OFFSET = ASSET_TYPE_UINT32 | 20,
    /**
     * A tag whose value indicates the order by which the query result is returned.
     */
    ASSET_TAG_RETURN_ORDER_BY = ASSET_TYPE_UINT32 | 21,
} AssetTag;


/**
 *  An enum type that indicates the asset error code.
 */
typedef enum {
    ASSET_SUCCESS = 0,
    /**
     * The error code indicates that the permission is denied.
     */
    ASSET_PERMISSION_DENIED = 201,
    /**
     * The error code indicates that the parameter is invalid
     */
    ASSET_INVALID_ARGUMENT = 401,
    /**
     * The error code indicates that the capability is not supported.
     */
    ASSET_NOT_SUPPORTED = 801,
    /**
     * The error code indicates that the asset service is unavailable.
     */
    ASSET_SERVICE_UNAVAILABLE = 24000001,
    /**
     * The error code indicates that the asset to be queried is not found.
     */
    ASSET_NOT_FOUND = 24000002,
    /**
     * The error code indicates that the asset to be added is duplicate.
     */
    ASSET_DUPLICATED = 24000003,
    /**
     * The error code indicates that the asset access is denied.
     */
    ASSET_ACCESS_DENIED = 24000004,
    /**
     * The error code indicates that the authentication token has expired.
     */
    ASSET_AUTH_TOKEN_EXPIRED = 24000005,
    /**
     * The error code indicates that the system memory is insufficient.
     */
    ASSET_OUT_OF_MEMRORY = 24000006,
    /**
     * The error code indicates that the asset or key is corrupted.
     */
    ASSET_DATA_CORRUPTED = 24000007,
} AssetErrorCode;

typedef enum {
    ACCESSIBILITY_DEVICE_POWER_ON = 0,
    ACCESSIBILITY_DEVICE_FIRST_UNLOCK = 1,
    ACCESSIBILITY_DEVICE_UNLOCK = 2,
    ACCESSIBILITY_DEVICE_SECURE = 3,
} AssetAccessibility;

typedef enum {
    AUTH_TYPE_NONE = 0x00,
    AUTH_TYPE_ANY = 0xFF,
} AssetAuthType;

typedef enum {
    SYNC_TYPE_NEVER = 0,
    SYNC_TYPE_THIS_DEVICE = 1 << 0,
    SYNC_TYPE_TRUSTED_ACCOUNT = 1 << 1,
    SYNC_TYPE_TRUSTED_DEVICE = 1 << 2,
} SyncType;

typedef enum {
    CONFLICT_OVERWRITE = 0,
    CONFLICT_THROW_ERROR = 1,
} ConflictResolution;

typedef enum {
    RETURN_ALL = 0,
    RETURN_ATTRIBUTES = 1,
} ReturnType;

/**
 * The asset version.
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

typedef struct {
    uint32_t size;
    uint8_t *data;
} AssetBlob;

typedef union {
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    bool boolean;
    AssetBlob blob;
} AssetValue;

typedef struct {
    uint32_t tag;
    AssetValue value;
} AssetParam;

typedef struct {
    AssetParam *params;
    uint32_t count;
} AssetResult;

typedef struct {
    AssetResult *results;
    uint32_t count;
} AssetResultSet;

// todo: 同步最新JS doc

#ifdef __cplusplus
}
#endif

#endif // ASSET_TYPE_H