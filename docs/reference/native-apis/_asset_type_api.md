# AssetTypeApi


## 概述

该模块定义了Asset接口使用到的枚举值、数据结构和错误码。

**系统能力：** SystemCapability.Security.Asset

**起始版本：** 11


## 汇总


### 文件

| 名称 | 描述 | 
| -------- | -------- |
| [asset_type.h](asset__type_8h.md) | 定义了Asset接口使用到的枚举值、数据结构和错误码。  | 


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
| [ASSET_TAG_TYPE_MASK](#asset_tag_type_mask)&nbsp;&nbsp;&nbsp;(0xF &lt;&lt; 28) | 用于获取Asset属性值的数据类型的掩码。  | 


### 枚举

| 名称 | 描述 | 
| -------- | -------- |
| [Asset_TagType](#asset_tagtype) { ASSET_TYPE_BOOL = 0x1 &lt;&lt; 28, ASSET_TYPE_NUMBER = 0x2 &lt;&lt; 28, ASSET_TYPE_BYTES = 0x3 &lt;&lt; 28 } | Asset属性值的数据类型定义的枚举类型。  | 
| [Asset_Tag](#asset_tag) {<br/>ASSET_TAG_SECRET = ASSET_TYPE_BYTES \| 0x01, ASSET_TAG_ALIAS = ASSET_TYPE_BYTES \| 0x02, ASSET_TAG_ACCESSIBILITY = ASSET_TYPE_NUMBER \| 0x03, ASSET_TAG_REQUIRE_PASSWORD_SET = ASSET_TYPE_BOOL \| 0x04,<br/>ASSET_TAG_AUTH_TYPE = ASSET_TYPE_NUMBER \| 0x05, ASSET_TAG_AUTH_VALIDITY_PERIOD = ASSET_TYPE_NUMBER \| 0x06, ASSET_TAG_AUTH_CHALLENGE = ASSET_TYPE_BYTES \| 0x07, ASSET_TAG_AUTH_TOKEN = ASSET_TYPE_BYTES \| 0x08,<br/>ASSET_TAG_SYNC_TYPE = ASSET_TYPE_NUMBER \| 0x10, ASSET_TAG_IS_PERSISTENT = ASSET_TYPE_BOOL \| 0x11, ASSET_TAG_DATA_LABEL_CRITICAL_1 = ASSET_TYPE_BYTES \| 0x20, ASSET_TAG_DATA_LABEL_CRITICAL_2 = ASSET_TYPE_BYTES \| 0x21,<br/>ASSET_TAG_DATA_LABEL_CRITICAL_3 = ASSET_TYPE_BYTES \| 0x22, ASSET_TAG_DATA_LABEL_CRITICAL_4 = ASSET_TYPE_BYTES \| 0x23, ASSET_TAG_DATA_LABEL_NORMAL_1 = ASSET_TYPE_BYTES \| 0x30, ASSET_TAG_DATA_LABEL_NORMAL_2 = ASSET_TYPE_BYTES \| 0x31,<br/>ASSET_TAG_DATA_LABEL_NORMAL_3 = ASSET_TYPE_BYTES \| 0x32, ASSET_TAG_DATA_LABEL_NORMAL_4 = ASSET_TYPE_BYTES \| 0x33, ASSET_TAG_RETURN_TYPE = ASSET_TYPE_NUMBER \| 0x40, ASSET_TAG_RETURN_LIMIT = ASSET_TYPE_NUMBER \| 0x41,<br/>ASSET_TAG_RETURN_OFFSET = ASSET_TYPE_NUMBER \| 0x42, ASSET_TAG_RETURN_ORDERED_BY = ASSET_TYPE_NUMBER \| 0x43, ASSET_TAG_CONFLICT_RESOLUTION = ASSET_TYPE_NUMBER \| 0x44<br/>} | Asset属性标记枚举。  | 
| [Asset_ResultCode](#asset_resultcode) {<br/>ASSET_SUCCESS = 0, ASSET_PERMISSION_DENIED = 201, ASSET_INVALID_ARGUMENT = 401, ASSET_SERVICE_UNAVAILABLE = 24000001,<br/>ASSET_NOT_FOUND = 24000002, ASSET_DUPLICATED = 24000003, ASSET_ACCESS_DENIED = 24000004, ASSET_STATUS_MISMATCH = 24000005,<br/>ASSET_OUT_OF_MEMRORY = 24000006, ASSET_DATA_CORRUPTED = 24000007, ASSET_DATABASE_ERROR = 24000008, ASSET_CRYPTO_ERROR = 2400009,<br/>ASSET_IPC_ERROR = 24000010, ASSET_BMS_ERROR = 24000011, ASSET_ACCOUNT_ERROR = 24000012, ASSET_ACCESS_TOKEN_ERROR = 24000013,<br/>ASSET_FILE_OPERATION_ERROR = 24000014, ASSET_GET_SYSTEM_TIME_ERROR = 24000015, ASSET_LIMIT_EXCEEDED = 24000016, ASSET_UNSUPPORTED = 24000017<br/>} | Asset操作的返回结果码。  | 
| [Asset_Accessibility](#asset_accessibility) { ASSET_ACCESSIBILITY_DEVICE_POWER_ON = 0, ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED = 1, ASSET_ACCESSIBILITY_DEVICE_UNLOCKED = 2 } | 该组枚举用来指定关键资产何时可访问。  | 
| [Asset_AuthType](#asset_authtype) { ASSET_AUTH_TYPE_NONE = 0x00, ASSET_AUTH_TYPE_ANY = 0xFF } | 该组枚举用来指定关键资产需要的用户认证类型。  | 
| [Asset_SyncType](#asset_synctype) { ASSET_SYNC_TYPE_NEVER = 0, ASSET_SYNC_TYPE_THIS_DEVICE = 1 &lt;&lt; 0, ASSET_SYNC_TYPE_TRUSTED_DEVICE = 1 &lt;&lt; 1 } | 该组枚举用来指定关键资产的同步类型。  | 
| [Asset_ConflictResolution](#asset_conflictresolution) { ASSET_CONFLICT_OVERWRITE = 0, ASSET_CONFLICT_THROW_ERROR = 1 } | 该组枚举用来指定关键资产别名重复时的冲突处理策略。  | 
| [Asset_ReturnType](#asset_returntype) { ASSET_RETURN_ALL = 0, ASSET_RETURN_ATTRIBUTES = 1 } | 该组枚举用来指定查询关键资产时的返回类型。  | 


## 宏定义说明


### ASSET_TAG_TYPE_MASK

```
#define ASSET_TAG_TYPE_MASK   (0xF << 28)
```
**描述**
用于获取Asset属性值的数据类型的掩码。

**起始版本：** 11


## 枚举类型说明


### Asset_Accessibility

```
enum Asset_Accessibility
```
**描述**
该组枚举用来指定关键资产何时可访问。

**起始版本：** 11

| 枚举值 | 描述 | 
| -------- | -------- |
| ASSET_ACCESSIBILITY_DEVICE_POWER_ON  | 关键资产密码需要设备开机后可访问。 | 
| ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED  | 关键资产密码需要设备第一次解锁后可访问。 | 
| ASSET_ACCESSIBILITY_DEVICE_UNLOCKED  | 关键资产密码需要设备解锁状态可访问。 | 


### Asset_AuthType

```
enum Asset_AuthType
```
**描述**
该组枚举用来指定关键资产需要的用户认证类型。

**起始版本：** 11

| 枚举值 | 描述 | 
| -------- | -------- |
| ASSET_AUTH_TYPE_NONE  | 不需要用户认证。 | 
| ASSET_AUTH_TYPE_ANY  | 通过PIN、模式、密码或生物特征进行用户身份验证都可以。 | 


### Asset_ConflictResolution

```
enum Asset_ConflictResolution
```
**描述**
该组枚举用来指定关键资产别名重复时的冲突处理策略。

**起始版本：** 11

| 枚举值 | 描述 | 
| -------- | -------- |
| ASSET_CONFLICT_OVERWRITE  | 覆盖老的关键资产。 | 
| ASSET_CONFLICT_THROW_ERROR  | 抛出错误，以便调用者在检测到冲突时采取措施。 | 


### Asset_ResultCode

```
enum Asset_ResultCode
```
**描述**
Asset操作的返回结果码。

**起始版本：** 11

| 枚举值 | 描述 | 
| -------- | -------- |
| ASSET_SUCCESS  | 表示操作成功。 | 
| ASSET_PERMISSION_DENIED  | 表示调用者没有权限。 | 
| ASSET_INVALID_ARGUMENT  | 表示参数错误。 | 
| ASSET_SERVICE_UNAVAILABLE  | 表示关键资产服务不可用。 | 
| ASSET_NOT_FOUND  | 表示未找到关键资产。 | 
| ASSET_DUPLICATED  | 表示关键资产已存在。 | 
| ASSET_ACCESS_DENIED  | 表示拒绝访问关键资产。 | 
| ASSET_STATUS_MISMATCH  | 表示锁屏状态不匹配。 | 
| ASSET_OUT_OF_MEMRORY  | 表示系统内存不足。 | 
| ASSET_DATA_CORRUPTED  | 表示关键资产损坏。 | 
| ASSET_DATABASE_ERROR  | 表示数据库操作失败。 | 
| ASSET_CRYPTO_ERROR  | 表示算法库操作失败。 | 
| ASSET_IPC_ERROR  | 表示进程通信错误。 | 
| ASSET_BMS_ERROR  | 表示包管理服务异常。 | 
| ASSET_ACCOUNT_ERROR  | 表示账号系统异常。 | 
| ASSET_ACCESS_TOKEN_ERROR  | 表示访问控制服务异常。 | 
| ASSET_FILE_OPERATION_ERROR  | 表示文件操作失败。 | 
| ASSET_GET_SYSTEM_TIME_ERROR  | 表示获取系统时间失败。 | 
| ASSET_LIMIT_EXCEEDED  | 表示缓存数量超限。 | 
| ASSET_UNSUPPORTED  | 表示该子功能不支持。 | 


### Asset_ReturnType

```
enum Asset_ReturnType
```
**描述**
该组枚举用来指定查询关键资产时的返回类型。

**起始版本：** 11

| 枚举值 | 描述 | 
| -------- | -------- |
| ASSET_RETURN_ALL  | 表示返回数据应同时包含密码和属性。 | 
| ASSET_RETURN_ATTRIBUTES  | 表示返回数据时只包含属性。 | 


### Asset_SyncType

```
enum Asset_SyncType
```
**描述**
该组枚举用来指定关键资产的同步类型。

**起始版本：** 11

| 枚举值 | 描述 | 
| -------- | -------- |
| ASSET_SYNC_TYPE_NEVER  | 永不同步。 | 
| ASSET_SYNC_TYPE_THIS_DEVICE  | 具有此属性值的关键资产只能恢复到其转出的设备。 | 
| ASSET_SYNC_TYPE_TRUSTED_DEVICE  | 具有此属性值的关键资产只能转移到可信设备（用户授权）。 | 


### Asset_Tag

```
enum Asset_Tag
```
**描述**
Asset属性标记枚举。

**起始版本：** 11

| 枚举值 | 描述 | 
| -------- | -------- |
| ASSET_TAG_SECRET  | 表示用户敏感数据，如口令、令牌等，其值为bytes类型。 | 
| ASSET_TAG_ALIAS  | 表示一个关键资产的标识，其值为bytes类型。 | 
| ASSET_TAG_ACCESSIBILITY  | 表示关键资产何时可访问，其值为uint32类型。 | 
| ASSET_TAG_REQUIRE_PASSWORD_SET  | 表示关键资产是否在设备是否设置了锁屏密码时可用，其值为bool类型。 | 
| ASSET_TAG_AUTH_TYPE  | 表示关键资产需要的用户认证类型，其值为uint32类型。 | 
| ASSET_TAG_AUTH_VALIDITY_PERIOD  | 表示用户认证的有效时间，其值为uint32类型，单位为秒。 | 
| ASSET_TAG_AUTH_CHALLENGE  | 表示认证时抗重放用的挑战值，其值为bytes类型。 | 
| ASSET_TAG_AUTH_TOKEN  | 表示用户认证后获取到的认证令牌，其值为bytes类型。 | 
| ASSET_TAG_SYNC_TYPE  | 表示关键资产的同步类型，其值为uint32类型。 | 
| ASSET_TAG_IS_PERSISTENT  | 表示关键资产是否需持久化存储，其值为bool类型。 | 
| ASSET_TAG_DATA_LABEL_CRITICAL_1  | 表示一个用户可自定义传入的字段，该字段不可被更新，其值为bytes类型。 | 
| ASSET_TAG_DATA_LABEL_CRITICAL_2  | 表示一个用户可自定义传入的字段，该字段不可被更新，其值为bytes类型。 | 
| ASSET_TAG_DATA_LABEL_CRITICAL_3  | 表示一个用户可自定义传入的字段，该字段不可被更新，其值为bytes类型。 | 
| ASSET_TAG_DATA_LABEL_CRITICAL_4  | 表示一个用户可自定义传入的字段，该字段不可被更新，其值为bytes类型。 | 
| ASSET_TAG_DATA_LABEL_NORMAL_1  | 表示一个用户可自定义传入的字段，该字段可被更新，其值为bytes类型。 | 
| ASSET_TAG_DATA_LABEL_NORMAL_2  | 表示一个用户可自定义传入的字段，该字段可被更新，其值为bytes类型。 | 
| ASSET_TAG_DATA_LABEL_NORMAL_3  | 表示一个用户可自定义传入的字段，该字段可被更新，其值为bytes类型。 | 
| ASSET_TAG_DATA_LABEL_NORMAL_4  | 表示一个用户可自定义传入的字段，该字段可被更新，其值为bytes类型。 | 
| ASSET_TAG_RETURN_TYPE  | 表示查询关键资产时的返回类型，其值为uint32类型。 | 
| ASSET_TAG_RETURN_LIMIT  | 表示查询关键资产时的最大返回数量，其值为uint32类型。 | 
| ASSET_TAG_RETURN_OFFSET  | 表示查询关键资产时的偏移量，其值为uint32类型。 | 
| ASSET_TAG_RETURN_ORDERED_BY  | 表示查询关键资产时的排序依据，其值为uint32类型。 | 
| ASSET_TAG_CONFLICT_RESOLUTION  | 表示增加关键资产时的冲突处理策略，其值为uint32类型。 | 


### Asset_TagType

```
enum Asset_TagType
```
**描述**
Asset属性值的数据类型定义的枚举类型。

**起始版本：** 11

| 枚举值 | 描述 | 
| -------- | -------- |
| ASSET_TYPE_BOOL  | Asset属性值是bool数据类型。 | 
| ASSET_TYPE_NUMBER  | Asset属性值是uint32数据类型。 | 
| ASSET_TYPE_BYTES  | Asset属性值是byte数据类型。 | 
