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
    INT32 = 1 << 28,
    /**
     * The type of the asset attribute value is uint32.
     */
    UINT32 = 2 << 28,
    /**
     * The type of the asset attribute value is int64.
     */
    INT64 = 3 << 28,
    /**
     * The type of the asset attribute value is uint64.
     */
    UINT64 = 4 << 28,
    /**
     * The type of the asset attribute value is bool.
     */
    BOOL = 5 << 28,
    /**
     * The type of the asset attribute value is byte array.
     */
    BYTES = 6 << 28,
} TagType;

/**
 * An emum type that indicates the tag of the asset attribute.
 */
typedef enum {
    /**
     * A tag whose value is the asset, such as password and token.
     */
    SECRET = BYTES | 1,
    /**
     * A tag whose value used to identify an asset.
     */
    ALIAS = BYTES | 2,
    /**
     * A tag whose value indicates when the asset can be accessed.
     */
    ACCESSIBILITY = UINT32 | 3,
    /**
     * A tag whose value indicates what type of user authentication is required.
     */
    AUTH_TYPE = UINT32 | 4,
    /**
     * A tag whose value indicates the validity period of user authentication, in seconds.
     */
    AUTH_VALIDITY_PERIOD = UINT32 | 5,
    /**
     * A tag whose value indicates the authentication challenge for anti-replay.
     */
    AUTH_CHALLENGE = BYTES | 6,
    /**
     * A tag whose value indicates the credential after successful authentication of the user.
     */
    AUTH_TOKEN = BYTES | 7,
    /**
     * A tag whose value indicates the type of asset synchronization.
     */
    SYNC_TYPE = UINT32 | 8,
    /**
     * A tag whose value indicates the conflict handling policy for adding the asset with the same alias.
     */
    CONFLICT_POLICY = UINT32 | 9,
    /**
     * A tag whose value indicates the first customized critical data of the asset.
     */
    DATA_LABLE_CRITICAL_1 = BYTES | 10,
    /**
     * A tag whose value indicates the second customized critical data of the asset.
     */
    DATA_LABLE_CRITICAL_2 = BYTES | 11,
    /**
     * A tag whose value indicates the third customized critical data of the asset.
     */
    DATA_LABLE_CRITICAL_3 = BYTES | 12,
    /**
     * A tag whose value indicates the fourth customized critical data of the asset.
     */
    DATA_LABLE_CRITICAL_4 = BYTES | 13,
    /**
     * A tag whose value indicates the first customized normal data of the asset.
     */
    DATA_LABLE_NORMAL_1 = BYTES | 14,
    /**
     * A tag whose value indicates the second customized normal data of the asset.
     */
    DATA_LABLE_NORMAL_2 = BYTES | 15,
    /**
     * A tag whose value indicates the third customized normal data of the asset.
     */
    DATA_LABLE_NORMAL_3 = BYTES | 16,
    /**
     * A tag whose value indicates the fourth customized normal data of the asset.
     */
    DATA_LABLE_NORMAL_4 = BYTES | 17,
    /**
     * A tag whose value indicates the type of the returned data.
     */
    RETURN_TYPE = UINT32 | 18,
    /**
     * A tag whose value indicates the maximum number of assets that can be returned in a query.
     */
    RETURN_LIMIT = UINT32 | 19,
    /**
     * A tag whose value indicates the offset of the batch query result.
     */
    RETURN_OFFSET = UINT32 | 20,
    /**
     * A tag whose value indicates the order by which the query result is returned.
     */
    RETURN_ORDER_BY = UINT32 | 21,
} Tag;


/**
 *  An enum type that indicates the asset error code.
 */
typedef enum {
    /**
     * The error code indicates that the permission is denied.
     */
    PERMISSION_DENIED = 201,
    /**
     * The error code indicates that the parameter is invalid
     */
    INVALID_PARAMETER = 401,
    /**
     * The error code indicates that the capability is not supported.
     */
    NOT_SUPPORTED = 801,
    /**
     * The error code indicates that the asset service is unavailable.
     */
    SERVICE_UNAVAILABLE = 24000001,
    /**
     * The error code indicates that the asset to be queried is not found.
     */
    NOT_FOUND = 24000002,
    /**
     * The error code indicates that the asset to be added is duplicate.
     */
    DUPLICATED = 24000003,
    /**
     * The error code indicates that the asset access is denied.
     */
    ACCESS_DENIED = 24000004,
    /**
     * The error code indicates that the authentication token has expired.
     */
    AUTH_TOKEN_EXPIRED = 24000005,
    /**
     * The error code indicates that the system memory is insufficient.
     */
    OUT_OF_MEMRORY = 24000006,
    /**
     * The error code indicates that the asset or key is corrupted.
     */
    DATA_CORRUPTED = 24000007,
} ErrorCode;

typedef enum {
    DEVICE_POWER_ON = 0,
    DEVICE_FIRST_UNLOCK = 1,
    DEVICE_UNLOCK = 2,
    DEVICE_SECURE = 3,
} Accessibility;

typedef enum {
    NONE = 0x00,
    ANY = 0xFF,
} AuthType;

typedef enum {
    NEVER = 0,
    THIS_DEVICE = 1 << 0,
    TRUSTED_ACCOUNT = 1 << 1,
    TRUSTED_DEVICE = 1 << 2,
} SyncType;

typedef enum {
    OVERRIDE = 0,
    REPORT = 1,
} ConflictPolicy;

typedef enum {
    ALL = 0,
    ATTRIBUTES = 1,
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

#ifdef __cplusplus
}
#endif

#endif // ASSET_TYPE_H