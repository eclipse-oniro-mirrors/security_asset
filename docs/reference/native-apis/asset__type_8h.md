# asset_type.h


## 概述

定义了Asset接口使用到的枚举值、数据结构和错误码。

**起始版本：** 11

**相关模块：**[AssetTypeApi](_asset_type_api.md)


## 汇总


### 结构体

| 名称 | 描述 | 
| -------- | -------- |
| [Asset_Blob](_asset___blob.md) | struct<br/>关键资产中使用的bytes类型，其值为字节数组。  | 
| [Asset_Value](union_asset___value.md) | union<br/>该类型用于传入关键资产属性。  | 
| [Asset_Attr](_asset___attr.md) | struct<br/>该类型用关键资产属性的键-值对。  | 
| [Asset_Result](_asset___result.md) | struct<br/>该类型用于表示关键资产属性的键-值对集合。  | 
| [Asset_ResultSet](_asset___result_set.md) | struct<br/>该类型用于表示查询关键资产返回结果集合的类型。  | 


### 宏定义

| 名称 | 描述 | 
| -------- | -------- |
| [ASSET_TAG_TYPE_MASK](_asset_type_api.md#asset_tag_type_mask)&nbsp;&nbsp;&nbsp;(0xF &lt;&lt; 28) | 用于获取Asset属性值的数据类型的掩码。  | 


### 枚举

| 名称 | 描述 | 
| -------- | -------- |
| [Asset_TagType](_asset_type_api.md#asset_tagtype) { [ASSET_TYPE_BOOL](_asset_type_api.md) = 0x1 &lt;&lt; 28, [ASSET_TYPE_NUMBER](_asset_type_api.md) = 0x2 &lt;&lt; 28, [ASSET_TYPE_BYTES](_asset_type_api.md) = 0x3 &lt;&lt; 28 } | Asset属性值的数据类型定义的枚举类型。  | 
| [Asset_Tag](_asset_type_api.md#asset_tag) {<br/>[ASSET_TAG_SECRET](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x01, [ASSET_TAG_ALIAS](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x02, [ASSET_TAG_ACCESSIBILITY](_asset_type_api.md) = ASSET_TYPE_NUMBER \| 0x03, [ASSET_TAG_REQUIRE_PASSWORD_SET](_asset_type_api.md) = ASSET_TYPE_BOOL \| 0x04,<br/>[ASSET_TAG_AUTH_TYPE](_asset_type_api.md) = ASSET_TYPE_NUMBER \| 0x05, [ASSET_TAG_AUTH_VALIDITY_PERIOD](_asset_type_api.md) = ASSET_TYPE_NUMBER \| 0x06, [ASSET_TAG_AUTH_CHALLENGE](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x07, [ASSET_TAG_AUTH_TOKEN](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x08,<br/>[ASSET_TAG_SYNC_TYPE](_asset_type_api.md) = ASSET_TYPE_NUMBER \| 0x10, [ASSET_TAG_IS_PERSISTENT](_asset_type_api.md) = ASSET_TYPE_BOOL \| 0x11, [ASSET_TAG_DATA_LABEL_CRITICAL_1](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x20, [ASSET_TAG_DATA_LABEL_CRITICAL_2](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x21,<br/>[ASSET_TAG_DATA_LABEL_CRITICAL_3](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x22, [ASSET_TAG_DATA_LABEL_CRITICAL_4](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x23, [ASSET_TAG_DATA_LABEL_NORMAL_1](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x30, [ASSET_TAG_DATA_LABEL_NORMAL_2](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x31,<br/>[ASSET_TAG_DATA_LABEL_NORMAL_3](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x32, [ASSET_TAG_DATA_LABEL_NORMAL_4](_asset_type_api.md) = ASSET_TYPE_BYTES \| 0x33, [ASSET_TAG_RETURN_TYPE](_asset_type_api.md) = ASSET_TYPE_NUMBER \| 0x40, [ASSET_TAG_RETURN_LIMIT](_asset_type_api.md) = ASSET_TYPE_NUMBER \| 0x41,<br/>[ASSET_TAG_RETURN_OFFSET](_asset_type_api.md) = ASSET_TYPE_NUMBER \| 0x42, [ASSET_TAG_RETURN_ORDERED_BY](_asset_type_api.md) = ASSET_TYPE_NUMBER \| 0x43, [ASSET_TAG_CONFLICT_RESOLUTION](_asset_type_api.md) = ASSET_TYPE_NUMBER \| 0x44<br/>} | Asset属性标记枚举。  | 
| [Asset_ResultCode](_asset_type_api.md#asset_resultcode) {<br/>[ASSET_SUCCESS](_asset_type_api.md) = 0, [ASSET_PERMISSION_DENIED](_asset_type_api.md) = 201, [ASSET_INVALID_ARGUMENT](_asset_type_api.md) = 401, [ASSET_SERVICE_UNAVAILABLE](_asset_type_api.md) = 24000001,<br/>[ASSET_NOT_FOUND](_asset_type_api.md) = 24000002, [ASSET_DUPLICATED](_asset_type_api.md) = 24000003, [ASSET_ACCESS_DENIED](_asset_type_api.md) = 24000004, [ASSET_STATUS_MISMATCH](_asset_type_api.md) = 24000005,<br/>[ASSET_OUT_OF_MEMRORY](_asset_type_api.md) = 24000006, [ASSET_DATA_CORRUPTED](_asset_type_api.md) = 24000007, [ASSET_DATABASE_ERROR](_asset_type_api.md) = 24000008, [ASSET_CRYPTO_ERROR](_asset_type_api.md) = 2400009,<br/>[ASSET_IPC_ERROR](_asset_type_api.md) = 24000010, [ASSET_BMS_ERROR](_asset_type_api.md) = 24000011, [ASSET_ACCOUNT_ERROR](_asset_type_api.md) = 24000012, [ASSET_ACCESS_TOKEN_ERROR](_asset_type_api.md) = 24000013,<br/>[ASSET_FILE_OPERATION_ERROR](_asset_type_api.md) = 24000014, [ASSET_GET_SYSTEM_TIME_ERROR](_asset_type_api.md) = 24000015, [ASSET_LIMIT_EXCEEDED](_asset_type_api.md) = 24000016, [ASSET_UNSUPPORTED](_asset_type_api.md) = 24000017<br/>} | Asset操作的返回结果码。  | 
| [Asset_Accessibility](_asset_type_api.md#asset_accessibility) { [ASSET_ACCESSIBILITY_DEVICE_POWER_ON](_asset_type_api.md) = 0, [ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED](_asset_type_api.md) = 1, [ASSET_ACCESSIBILITY_DEVICE_UNLOCKED](_asset_type_api.md) = 2 } | 该组枚举用来指定关键资产何时可访问。  | 
| [Asset_AuthType](_asset_type_api.md#asset_authtype) { [ASSET_AUTH_TYPE_NONE](_asset_type_api.md) = 0x00, [ASSET_AUTH_TYPE_ANY](_asset_type_api.md) = 0xFF } | 该组枚举用来指定关键资产需要的用户认证类型。  | 
| [Asset_SyncType](_asset_type_api.md#asset_synctype) { [ASSET_SYNC_TYPE_NEVER](_asset_type_api.md) = 0, [ASSET_SYNC_TYPE_THIS_DEVICE](_asset_type_api.md) = 1 &lt;&lt; 0, [ASSET_SYNC_TYPE_TRUSTED_DEVICE](_asset_type_api.md) = 1 &lt;&lt; 1 } | 该组枚举用来指定关键资产的同步类型。  | 
| [Asset_ConflictResolution](_asset_type_api.md#asset_conflictresolution) { [ASSET_CONFLICT_OVERWRITE](_asset_type_api.md) = 0, [ASSET_CONFLICT_THROW_ERROR](_asset_type_api.md) = 1 } | 该组枚举用来指定关键资产别名重复时的冲突处理策略。  | 
| [Asset_ReturnType](_asset_type_api.md#asset_returntype) { [ASSET_RETURN_ALL](_asset_type_api.md) = 0, [ASSET_RETURN_ATTRIBUTES](_asset_type_api.md) = 1 } | 该组枚举用来指定查询关键资产时的返回类型。  | 
