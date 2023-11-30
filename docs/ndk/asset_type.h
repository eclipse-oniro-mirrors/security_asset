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

/**
 * @addtogroup AssetType
 * @{
 *
 * @brief 提供调用ASSET接口需要使用的枚举值、数据结构和错误码。
 *
 * @syscap SystemCapability.Security.Asset
 * @since 11
 */

/**
 * @file asset_type.h
 *
 * @brief 声明调用ASSET接口需要使用的枚举值、数据结构和错误码。
 *
 * @since 11
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Asset属性值的数据类型定义的枚举类型。
 *
 * @since 11
 */
typedef enum {
    /**
     * Asset属性值是bool数据类型。
     */
    ASSET_TYPE_BOOL = 0x1 << 28,
    /**
     * Asset属性值是uint32数据类型。
     */
    ASSET_TYPE_NUMBER = 0x2 << 28,
    /**
     * Asset属性值是byte数据类型。
     */
    ASSET_TYPE_BYTES = 0x3 << 28,
} Asset_TagType;

/**
 * @brief 用于获取Asset属性值的数据类型的掩码。
 *
 * @since 11
 */
#define ASSET_TAG_TYPE_MASK (0xF << 28)

/**
 * @brief Asset属性标记枚举。
 *
 * @since 11
 */
typedef enum {
    /**
     * 表示用户敏感数据，如口令、令牌等，其值为bytes类型。
     */
    ASSET_TAG_SECRET = ASSET_TYPE_BYTES | 0x01,
    /**
     * 表示一个关键资产的标识，其值为bytes类型。
     */
    ASSET_TAG_ALIAS = ASSET_TYPE_BYTES | 0x02,
    /**
     * 表示关键资产何时可访问，其值为uint32类型。
     */
    ASSET_TAG_ACCESSIBILITY = ASSET_TYPE_NUMBER | 0x03,
    /**
     * 表示关键资产是否在设备是否设置了锁屏密码时可用，其值为bool类型。
     */
    ASSET_TAG_REQUIRE_PASSWORD_SET = ASSET_TYPE_BOOL | 0x04,
    /**
     * 表示关键资产需要的用户认证类型，其值为uint32类型。
     */
    ASSET_TAG_AUTH_TYPE = ASSET_TYPE_NUMBER | 0x05,
    /**
     * 表示用户认证的有效时间，其值为uint32类型，单位为秒。
     */
    ASSET_TAG_AUTH_VALIDITY_PERIOD = ASSET_TYPE_NUMBER | 0x06,
    /**
     * 表示认证时抗重放用的挑战值，其值为bytes类型。
     */
    ASSET_TAG_AUTH_CHALLENGE = ASSET_TYPE_BYTES | 0x07,
    /**
     * 表示用户认证后获取到的认证令牌，其值为bytes类型。
     */
    ASSET_TAG_AUTH_TOKEN = ASSET_TYPE_BYTES | 0x08,
    /**
     * 表示关键资产的同步类型，其值为uint32类型。
     */
    ASSET_TAG_SYNC_TYPE = ASSET_TYPE_NUMBER | 0x10,
    /**
     * 表示关键资产是否需持久化存储，其值为bool类型。
     * 仅在调用OH_Asset_Add函数时传入该属性需要校验权限。
     *
     * @permission ohos.permission.STORE_PERSISTENT_DATA
     */
    ASSET_TAG_IS_PERSISTENT = ASSET_TYPE_BOOL | 0x11,
    /**
     * 表示一个用户可自定义传入的字段，该字段不可被更新，其值为bytes类型。
     */
    ASSET_TAG_DATA_LABEL_CRITICAL_1 = ASSET_TYPE_BYTES | 0x20,
    /**
     * 表示一个用户可自定义传入的字段，该字段不可被更新，其值为bytes类型。
     */
    ASSET_TAG_DATA_LABEL_CRITICAL_2 = ASSET_TYPE_BYTES | 0x21,
    /**
     * 表示一个用户可自定义传入的字段，该字段不可被更新，其值为bytes类型。
     */
    ASSET_TAG_DATA_LABEL_CRITICAL_3 = ASSET_TYPE_BYTES | 0x22,
    /**
     * 表示一个用户可自定义传入的字段，该字段不可被更新，其值为bytes类型。
     */
    ASSET_TAG_DATA_LABEL_CRITICAL_4 = ASSET_TYPE_BYTES | 0x23,
    /**
     * 表示一个用户可自定义传入的字段，该字段可被更新，其值为bytes类型。
     */
    ASSET_TAG_DATA_LABEL_NORMAL_1 = ASSET_TYPE_BYTES | 0x30,
    /**
     * 表示一个用户可自定义传入的字段，该字段可被更新，其值为bytes类型。
     */
    ASSET_TAG_DATA_LABEL_NORMAL_2 = ASSET_TYPE_BYTES | 0x31,
    /**
     * 表示一个用户可自定义传入的字段，该字段可被更新，其值为bytes类型。
     */
    ASSET_TAG_DATA_LABEL_NORMAL_3 = ASSET_TYPE_BYTES | 0x32,
    /**
     * 表示一个用户可自定义传入的字段，该字段可被更新，其值为bytes类型。
     */
    ASSET_TAG_DATA_LABEL_NORMAL_4 = ASSET_TYPE_BYTES | 0x33,
    /**
     * 表示查询关键资产时的返回类型，其值为uint32类型。
     */
    ASSET_TAG_RETURN_TYPE = ASSET_TYPE_NUMBER | 0x40,
    /**
     * 表示查询关键资产时的最大返回数量，其值为uint32类型。
     */
    ASSET_TAG_RETURN_LIMIT = ASSET_TYPE_NUMBER | 0x41,
    /**
     * 表示查询关键资产时的偏移量，其值为uint32类型。
     */
    ASSET_TAG_RETURN_OFFSET = ASSET_TYPE_NUMBER | 0x42,
    /**
     * 表示查询关键资产时的排序依据，其值为uint32类型。
     */
    ASSET_TAG_RETURN_ORDERED_BY = ASSET_TYPE_NUMBER | 0x43,
    /**
     * 表示增加关键资产时的冲突处理策略，其值为uint32类型。
     */
    ASSET_TAG_CONFLICT_RESOLUTION = ASSET_TYPE_NUMBER | 0x44,
} Asset_Tag;

/**
 * @brief 调用ASSET返回的结果码。
 *
 * @since 11
 */
typedef enum {
    /**
     * 表示操作成功。
     */
    ASSET_SUCCESS = 0,
    /**
     * 表示调用者没有权限。
     */
    ASSET_PERMISSION_DENIED = 201,
    /**
     * 表示参数错误。
     */
    ASSET_INVALID_ARGUMENT = 401,
    /**
     * 表示关键资产服务不可用。
     */
    ASSET_SERVICE_UNAVAILABLE = 24000001,
    /**
     * 表示未找到关键资产。
     */
    ASSET_NOT_FOUND = 24000002,
    /**
     * 表示关键资产已存在。
     */
    ASSET_DUPLICATED = 24000003,
    /**
     * 表示拒绝访问关键资产。
     */
    ASSET_ACCESS_DENIED = 24000004,
    /**
     * 表示锁屏状态不匹配。
     */
    ASSET_STATUS_MISMATCH = 24000005,
    /**
     * 表示系统内存不足。
     */
    ASSET_OUT_OF_MEMRORY = 24000006,
    /**
     * 表示关键资产损坏。
     */
    ASSET_DATA_CORRUPTED = 24000007,
    /**
     * 表示数据库操作失败。
     */
    ASSET_DATABASE_ERROR = 24000008,
    /**
     * 表示算法库操作失败。
     */
    ASSET_CRYPTO_ERROR = 2400009,
    /**
     * 表示进程通信错误。
     */
    ASSET_IPC_ERROR = 24000010,
    /**
     * 表示包管理服务异常。
     */
    ASSET_BMS_ERROR = 24000011,
    /**
     * 表示账号系统异常。
     */
    ASSET_ACCOUNT_ERROR = 24000012,
    /**
     * 表示访问控制服务异常。
     */
    ASSET_ACCESS_TOKEN_ERROR = 24000013,
    /**
     * 表示文件操作失败。
     */
    ASSET_FILE_OPERATION_ERROR = 24000014,
    /**
     * 表示获取系统时间失败。
     */
    ASSET_GET_SYSTEM_TIME_ERROR = 24000015,
    /**
     * 表示缓存数量超限。
     */
    ASSET_LIMIT_EXCEEDED = 24000016,
    /**
     * 表示该子功能不支持。
     */
    ASSET_UNSUPPORTED = 24000017,
} Asset_ResultCode;

/**
 * @brief 该组枚举用来指定关键资产何时可访问。
 *
 * @since 11
 */
typedef enum {
    /**
     * 关键资产密码需要设备开机后可访问。
     */
    ASSET_ACCESSIBILITY_DEVICE_POWER_ON = 0,
    /**
     * 关键资产密码需要设备第一次解锁后可访问。
     */
    ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED = 1,
    /**
     * 关键资产密码需要设备解锁状态可访问。
     */
    ASSET_ACCESSIBILITY_DEVICE_UNLOCKED = 2,
} Asset_Accessibility;

/**
 * @brief 该组枚举用来指定关键资产需要的用户认证类型。
 *
 * @since 11
 */
typedef enum {
    /**
     * 不需要用户认证。
     */
    ASSET_AUTH_TYPE_NONE = 0x00,
    /**
     * 通过PIN、模式、密码或生物特征进行用户身份验证都可以。
     */
    ASSET_AUTH_TYPE_ANY = 0xFF,
} Asset_AuthType;

/**
 * @brief 该组枚举用来指定关键资产的同步类型。
 *
 * @since 11
 */
typedef enum {
    /**
     * 永不同步。
     */
    ASSET_SYNC_TYPE_NEVER = 0,
    /**
     * 具有此属性值的关键资产只能恢复到其转出的设备。
     */
    ASSET_SYNC_TYPE_THIS_DEVICE = 1 << 0,
    /**
     * 具有此属性值的关键资产只能转移到可信设备（用户授权）。
     */
    ASSET_SYNC_TYPE_TRUSTED_DEVICE = 1 << 1,
} Asset_SyncType;

/**
 * @brief 该组枚举用来指定关键资产别名重复时的冲突处理策略。
 *
 * @since 11
 */
typedef enum {
    /**
     * 覆盖老的关键资产。
     */
    ASSET_CONFLICT_OVERWRITE = 0,
    /**
     * 抛出错误，以便调用者在检测到冲突时采取措施。
     */
    ASSET_CONFLICT_THROW_ERROR = 1,
} Asset_ConflictResolution;

/**
 * @brief 该组枚举用来指定查询关键资产时的返回类型。
 *
 * @since 11
 */
typedef enum {
    /**
     * 表示返回数据应同时包含密码和属性。
     */
    ASSET_RETURN_ALL = 0,
    /**
     * 表示返回数据时只包含属性。
     */
    ASSET_RETURN_ATTRIBUTES = 1,
} Asset_ReturnType;

/**
 * @brief 关键资产中使用的bytes类型，其值为字节数组。
 *
 * @since 11
 */
typedef struct {
    /**
     * 表示字节数组的大小。
     */
    uint32_t size;
    /**
     * 指向字节数组的数据。
     */
    uint8_t *data;
} Asset_Blob;

/**
 * @brief 该类型用于传入关键资产属性。
 *
 * @since 11
 */
typedef union {
    /**
     * 该字段用于传入bool类型的关键资产数据。
     */
    bool boolean;
    /**
     * 该字段用于传入uint32类型的关键资产数据。
     */
    uint32_t u32;
    /**
     * 该字段用于传入bytes类型的关键资产数据。
     */
    Asset_Blob blob;
} Asset_Value;

/**
 * @brief 该类型用关键资产属性的键-值对。
 *
 * @since 11
 */
typedef struct {
    /**
     * 关键资产属性名称。
     */
    uint32_t tag;
    /**
     * 关键资产属性对应值。
     */
    Asset_Value value;
} Asset_Attr;

/**
 * @brief 该类型用于表示关键资产属性的键-值对集合。
 *
 * @since 11
 */
typedef struct {
    /**
     * 关键资产属性的键值对的数组大小。
     */
    uint32_t count;
    /**
     * 指向关键资产属性的键值对的数组。
     */
    Asset_Attr *attrs;
} Asset_Result;

/**
 * @brief 该类型用于表示查询关键资产返回结果集合的类型。
 *
 * @since 11
 */
typedef struct {
    /**
     * 关键资产属性的键-值对集合数组的大小。
     */
    uint32_t count;
    /**
     * 指向关键资产属性的键-值对集合数组。
     */
    Asset_Result *results;
} Asset_ResultSet;

#ifdef __cplusplus
}
#endif

/** @} */
#endif // ASSET_TYPE_H