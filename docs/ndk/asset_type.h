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
 * @brief 提供关键资产存储服务中通用的枚举值、数据结构和错误码。
 *
 * @syscap SystemCapability.Security.Asset
 * @since 11
 */

/**
 * @file asset_type.h
 *
 * @brief 定义关键资产存储服务中通用的枚举值、数据结构和错误码。
 *
 * @since 11
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 关键资产属性的类型定义。
 *
 * @since 11
 */
typedef enum {
    /**
     * 标识关键资产属性的类型是布尔类型。
     */
    ASSET_TYPE_BOOL = 0x1 << 28,
    /**
     * 标识关键资产属性的类型是整型。
     */
    ASSET_TYPE_NUMBER = 0x2 << 28,
    /**
     * 标识关键资产属性的类型是字节数组类型。
     */
    ASSET_TYPE_BYTES = 0x3 << 28,
} Asset_TagType;

/**
 * @brief 用于获取关键资产属性类型的掩码。
 *
 * @since 11
 */
#define ASSET_TAG_TYPE_MASK (0xF << 28)

/**
 * @brief 关键资产属性的名称。
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
     * 表示认证时防重放用的挑战值，其值为bytes类型。
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
     * 表示新增关键资产时的冲突处理策略，其值为uint32类型。
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
    ASSET_CRYPTO_ERROR = 24000009,
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
 * @brief 基于锁屏状态的访问控制类型。
 *
 * @since 11
 */
typedef enum {
    /**
     * 开机后可访问。
     */
    ASSET_ACCESSIBILITY_DEVICE_POWER_ON = 0,
    /**
     * 首次解锁后可访问。
     */
    ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED = 1,
    /**
     * 解锁时可访问。
     */
    ASSET_ACCESSIBILITY_DEVICE_UNLOCKED = 2,
} Asset_Accessibility;

/**
 * @brief 关键资产支持的用户认证类型。
 *
 * @since 11
 */
typedef enum {
    /**
     * 访问关键资产前无需用户认证。
     */
    ASSET_AUTH_TYPE_NONE = 0x00,
    /**
     * 任意一种用户认证方式（PIN码、人脸、指纹等）通过后，均可访问关键资产。
     */
    ASSET_AUTH_TYPE_ANY = 0xFF,
} Asset_AuthType;

/**
 * @brief 关键资产支持的同步类型。
 *
 * @since 11
 */
typedef enum {
    /**
     * 不允许同步关键资产。
     */
    ASSET_SYNC_TYPE_NEVER = 0,
    /**
     * 只在本设备进行同步，如仅在本设备还原的备份场景。
     */
    ASSET_SYNC_TYPE_THIS_DEVICE = 1 << 0,
    /**
     * 只在可信设备间进行同步，如克隆场景。
     */
    ASSET_SYNC_TYPE_TRUSTED_DEVICE = 1 << 1,
} Asset_SyncType;

/**
 * @brief 新增关键资产时的冲突（如：别名相同）处理策略。
 *
 * @since 11
 */
typedef enum {
    /**
     * 覆盖原本的关键资产。
     */
    ASSET_CONFLICT_OVERWRITE = 0,
    /**
     * 抛出异常，由业务进行后续处理。
     */
    ASSET_CONFLICT_THROW_ERROR = 1,
} Asset_ConflictResolution;

/**
 * @brief 关键资产查询返回的结果类型。
 *
 * @since 11
 */
typedef enum {
    /**
     * 返回关键资产明文及属性。
     */
    ASSET_RETURN_ALL = 0,
    /**
     * 返回关键资产属性，不含关键资产明文。
     */
    ASSET_RETURN_ATTRIBUTES = 1,
} Asset_ReturnType;

/**
 * @brief 二进制数组类型，即不定长的字节数组。
 *
 * @since 11
 */
typedef struct {
    /**
     * 表示字节数组的大小。
     */
    uint32_t size;
    /**
     * 指向字节数组的指针。
     */
    uint8_t *data;
} Asset_Blob;

/**
 * @brief 关键资产属性内容。
 *
 * @since 11
 */
typedef union {
    /**
     * 该字段用于传入bool类型的关键资产。
     */
    bool boolean;
    /**
     * 该字段用于传入uint32类型的关键资产。
     */
    uint32_t u32;
    /**
     * 该字段用于传入bytes类型的关键资产。
     */
    Asset_Blob blob;
} Asset_Value;

/**
 * @brief 关键资产属性。
 *
 * @since 11
 */
typedef struct {
    /**
     * 关键资产属性名称。
     */
    uint32_t tag;
    /**
     * 关键资产属性内容。
     */
    Asset_Value value;
} Asset_Attr;

/**
 * @brief 关键资产查询结果，用于定义一条关键资产。
 *
 * @since 11
 */
typedef struct {
    /**
     * 关键资产属性的个数。
     */
    uint32_t count;
    /**
     * 指向关键资产属性数组的指针。
     */
    Asset_Attr *attrs;
} Asset_Result;

/**
 * @brief 关键资产查询结果集合，用于定义多条关键资产。
 *
 * @since 11
 */
typedef struct {
    /**
     * 关键资产的条数。
     */
    uint32_t count;
    /**
     * 指向关键资产数组的指针。
     */
    Asset_Result *results;
} Asset_ResultSet;

#ifdef __cplusplus
}
#endif

/** @} */
#endif // ASSET_TYPE_H