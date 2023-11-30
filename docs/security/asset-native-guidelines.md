# 关键资产存储服务开发指导（Native）

[toc]

## 场景：保护密码类数据

> **说明：**
>
> 密码类数据可以是密码、登录令牌、信用卡号等用户敏感数据。

### 场景描述

用户在应用/浏览器中登录账号时，可以选择“记住密码”（如图）。针对此种场景，应用/浏览器可以将用户密码存储在ASSET中，由ASSET保证用户密码的安全性。

用户再次打开登录界面时，应用/浏览器可以从ASSET中查询用户密码，并将其自动填充到密码输入框，用户只需点击“登录”按钮即可完成账号登录，极大地提升了用户体验。

 <img src="Z:/workspace/hmos_dev/base/security/asset/figures/login.png" alt="login" style="zoom:40%;" />

### 关键流程

业务调用ASSET保护密码类数据（后文统称为“关键资产”），可以参照以下流程进行开发。

 <img src="Z:/workspace/hmos_dev/base/security/asset/figures/flowchat-no-auth-required.png" alt="flowchat" style="zoom:40%;" />

1. 业务查询符合条件的关键资产属性，根据查询成功/失败，判断关键资产是否存在。开发步骤参考[查询关键资产](#查询关键资产)，代码示例参考[查询单条关键资产属性](#查询单条关键资产属性)
2. 如果关键资产不存在，业务可选择：
   * 新增关键资产，开发步骤参考[新增关键资产](#新增关键资产)
3. 如果关键资产存在，业务可选择：
   * 删除关键资产，开发步骤参考[删除关键资产](#删除关键资产)
   * 更新关键资产，开发步骤参考[更新关键资产](#更新关键资产)
   * 查询关键资产明文，开发步骤参考[查询关键资产](#查询关键资产)，代码示例参考[查询单条关键资产明文](#查询单条关键资产明文)

## 新增关键资产

### 接口介绍

接口文档链接：

[int32_t OH_Asset_Add(const Asset_Attr *attributes, uint32_t attrCnt)](../reference/native-apis/_asset_api.md#OH_Asset_Add())

参数列表：

| 属性名（asset.Tag）             | 属性值（asset.Value）                                        | 是否必选 | 说明                                                         |
| ------------------------------- | ------------------------------------------------------------ | -------- | ------------------------------------------------------------ |
| ASSET_TAG_SECRET                | 类型为uint8[]，长度为1-1024字节                              | 必选     | 关键资产明文                                                 |
| ASSET_TAG_ALIAS                 | 类型为uint8[]，长度为1-256字节                               | 必选     | 关键资产别名，每条关键资产的唯一索引                         |
| ASSET_TAG_ACCESSIBILITY         | 类型为uint32_t，取值范围详见[Asset_Accessibility](../reference/native-apis/_asset_type_api.md#Asset_Accessibility) | 可选     | 访问控制属性                                                 |
| ASSET_TAG_REQUIRE_PASSWORD_SET  | 类型为bool                                                   | 可选     | 关键资产是否仅在设置了锁屏密码的情况下可访问                 |
| ASSET_TAG_AUTH_TYPE             | 类型为uint32_t，取值范围详见[Asset_AuthType](../reference/native-apis/_asset_type_api.md#Asset_AuthType) | 可选     | 访问关键资产所需的用户认证类型                               |
| ASSET_TAG_SYNC_TYPE             | 类型为uint32_t，取值范围详见[Asset_SyncType](../reference/native-apis/_asset_type_api.md#Asset_SyncType) | 可选     | 关键资产支持的同步类型                                       |
| ASSET_TAG_IS_PERSISTENT         | 类型为bool                                                   | 可选     | 关键资产在应用卸载时是否需要保留<br>**需要权限：**ohos.permission.STORE_PERSISTENT_DATA |
| ASSET_TAG_DATA_LABEL_CRITICAL_1 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护             |
| ASSET_TAG_DATA_LABEL_CRITICAL_2 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护             |
| ASSET_TAG_DATA_LABEL_CRITICAL_3 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护             |
| ASSET_TAG_DATA_LABEL_CRITICAL_4 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护             |
| ASSET_TAG_DATA_LABEL_NORMAL_1   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护             |
| ASSET_TAG_DATA_LABEL_NORMAL_2   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护             |
| ASSET_TAG_DATA_LABEL_NORMAL_3   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护             |
| ASSET_TAG_DATA_LABEL_NORMAL_4   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护             |
| ASSET_TAG_CONFLICT_RESOLUTION   | 类型为uint32_t，取值范围详见[Asset_ConflictResolution](../reference/native-apis/_asset_type_api.md#Asset_ConflictResolution) | 可选     | 写入同别名的关键资产时的处理策略                             |

### 代码示例

写入一条密码是demo_pwd，别名是demo_alias，附加属性是demo_label的数据，该数据在用户首次解锁设备后可被访问。

```c
#include <string.h>
#include "napi/native_api.h"

#include "asset_api.h"

static napi_value AddAsset(napi_env env, napi_callback_info info) {
    static const char *SECRET = "demo_pwd";
    static const char *ALIAS = "demo_alias";
    static const char *LABEL = "demo_label";

    Asset_Blob secret = { (uint32_t)(strlen(SECRET)), (uint8_t *)SECRET };
    Asset_Blob alias = { (uint32_t)(strlen(ALIAS)), (uint8_t *)ALIAS };
    Asset_Blob label = { (uint32_t)(strlen(LABEL)), (uint8_t *)LABEL };
    Asset_Attr attr[] = {
        { .tag = ASSET_TAG_ACCESSIBILITY, .value.u32 = ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED },
        { .tag = ASSET_TAG_SECRET, .value.blob = secret },
        { .tag = ASSET_TAG_ALIAS, .value.blob = alias },
        { .tag = ASSET_TAG_DATA_LABEL_NORMAL_1, .value.blob = label },
    };

    int32_t ret = OH_Asset_Add(attr, sizeof(attr) / sizeof(attr[0]));
    napi_value result;
    napi_create_int32(env, ret, &result);
    return result;
}
```


### 约束和限制

* 基于别名的访问

关键资产以密文的形式存储在ASSET数据库中，以业务身份 + 别名作为唯一索引。故业务需要保证每条关键资产的别名唯一。

* 业务自定义数据存储

ASSET为业务预留了8个关键资产自定义属性，命名以"ASSET_TAG_DATA_LABEL"开头。对于超过8个自定义属性的情况，业务可以将多段数据按照一定的格式（如JSON）拼接到同一个ASSET属性中。

ASSET对部分属性会进行完整性保护，这部分属性命名以"ASSET_TAG_DATA_LABEL_CRITICAL"开头，且写入后不支持更新。


## 查询关键资产

接口文档链接：

[int32_t OH_Asset_Query(const Asset_Attr *query, uint32_t queryCnt, Asset_ResultSet *resultSet)](../reference/native-apis/_asset_api.md#OH_Asset_Query())

参数列表：

| 属性名（asset.Tag）             | 属性值（asset.Value）                                        | 是否必选 | 说明                                                         |
| ------------------------------- | ------------------------------------------------------------ | -------- | ------------------------------------------------------------ |
| ASSET_TAG_ALIAS                 | 类型为uint8[]，长度为1-256字节                               | 可选     | 关键资产别名，每条关键资产的唯一索引;                        |
| ASSET_TAG_ACCESSIBILITY         | 类型为uint32_t，取值范围详见[Asset_Accessibility](../reference/native-apis/_asset_type_api.md#Asset_Accessibility) | 可选     | 访问控制属性                                                 |
| ASSET_TAG_REQUIRE_PASSWORD_SET  | 类型为bool                                                   | 可选     | 关键资产是否仅在设置了锁屏密码的情况下可访问                 |
| ASSET_TAG_AUTH_TYPE             | 类型为uint32_t，取值范围详见[Asset_AuthType](../reference/native-apis/_asset_type_api.md#Asset_AuthType) | 可选     | 访问关键资产所需的用户认证类型                               |
| ASSET_TAG_SYNC_TYPE             | 类型为uint32_t，取值范围详见[Asset_SyncType](../reference/native-apis/_asset_type_api.md#Asset_SyncType) | 可选     | 关键资产支持的同步类型                                       |
| ASSET_TAG_IS_PERSISTENT         | 类型为bool                                                   | 可选     | 关键资产在应用卸载时是否需要保留                             |
| ASSET_TAG_DATA_LABEL_CRITICAL_1 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护             |
| ASSET_TAG_DATA_LABEL_CRITICAL_2 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护             |
| ASSET_TAG_DATA_LABEL_CRITICAL_3 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护             |
| ASSET_TAG_DATA_LABEL_CRITICAL_4 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护             |
| ASSET_TAG_DATA_LABEL_NORMAL_1   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护             |
| ASSET_TAG_DATA_LABEL_NORMAL_2   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护             |
| ASSET_TAG_DATA_LABEL_NORMAL_3   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护             |
| ASSET_TAG_DATA_LABEL_NORMAL_4   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护             |
| ASSET_TAG_RETURN_TYPE           | 类型为uint32_t，取值范围详见[Asset_ReturnType](../reference/native-apis/_asset_type_api.md#Asset_ReturnType) | 可选     | 关键资产查询结果类型                                         |
| ASSET_TAG_RETURN_LIMIT          | 类型为uint32_t                                               | 可选     | 关键资产查询结果数量                                         |
| ASSET_TAG_RETURN_OFFSET         | 类型为uint32_t，取值范围：1-65536                            | 可选     | 满足查询条件的关键资产偏移量                                 |
| ASSET_TAG_RETURN_ORDERED_BY     | 类型为uint32_t，取值范围：ASSET_TAG_DATA_LABEL_xxx           | 可选     | 关键资产查询结果排序依据，仅支持指定按照附属信息排序，不指定的情况下，默认按照关键资产写入的顺序排序。 |

### 代码示例

#### 查询单条关键资产明文

查询别名是demo_alias的关键资产明文。

```c
#include <string.h>
#include "napi/native_api.h"

#include "asset_api.h"

static napi_value QueryAsset(napi_env env, napi_callback_info info) {
    static const char *ALIAS = "demo_alias";
    Asset_Blob alias = { (uint32_t)(strlen(ALIAS)), (uint8_t *)ALIAS };
    Asset_Attr attr[] = {
        { .tag = ASSET_TAG_ALIAS, .value.blob = alias },  // 指定了关键资产别名，最多查询到一条满足条件的关键资产
        { .tag = ASSET_TAG_RETURN_TYPE, .value.u32 = ASSET_RETURN_ALL },  // 此处表示需要返回每条关键资产的所有信息，即属性+明文
    };

    Asset_ResultSet resultSet = {0};
    int32_t ret = OH_Asset_Query(attr, sizeof(attr) / sizeof(attr[0]), &resultSet);
    if (ret == ASSET_SUCCESS) {
        // Parse the resultSet.
        for (uint32_t i = 0; i < resultSet.count; i++) {
            // Parse the secret.
            Asset_Attr *secret = OH_Asset_ParseAttr(resultSet.results + i, ASSET_TAG_SECRET);
        }
    }
    OH_Asset_FreeResultSet(&resultSet);
    napi_value result;
    napi_create_int32(env, ret, &result);
    return result;
}
```

#### 查询单条关键资产属性

查询别名是demo_alias的关键资产属性。

```c
#include <string.h>
#include "napi/native_api.h"

#include "asset_api.h"

static napi_value QueryAttributes(napi_env env, napi_callback_info info) {
    static const char *ALIAS = "demo_alias";
    Asset_Blob alias = { (uint32_t)(strlen(ALIAS)), (uint8_t *)ALIAS };
    Asset_Attr attr[] = {
        { .tag = ASSET_TAG_ALIAS, .value.blob = alias }, // 指定了关键资产别名，最多查询到一条满足条件的关键资产
        { .tag = ASSET_TAG_RETURN_TYPE, .value.u32 = ASSET_RETURN_ATTRIBUTES }, // 此处表示需要返回仅返回关键资产属性，不包含关键资产明文
    };

    Asset_ResultSet resultSet = {0};
    int32_t ret = OH_Asset_Query(attr, sizeof(attr) / sizeof(attr[0]), &resultSet);
    if (ret == ASSET_SUCCESS) {
        // Parse the result.
        for (uint32_t i = 0; i < resultSet.count; i++) {
        // Parse the data label.
            Asset_Attr *label = OH_Asset_ParseAttr(resultSet.results + i, ASSET_TAG_DATA_LABEL_NORMAL_1);
        }
    }
    OH_Asset_FreeResultSet(&resultSet);
    napi_value result;
    napi_create_int32(env, ret, &result);
    return result;
}
```

#### 批量查询关键资产属性

以Callback形式的接口调用为例，批量查询标签1是demo_label的关键资产属性，从第5条满足条件的结果开始返回，一共返回10条，且返回结果以DATA_LABEL_NORMAL_1属性内容排序。

```c
#include <string.h>
#include "napi/native_api.h"

#include "asset_api.h"

static napi_value BatchQuery(napi_env env, napi_callback_info info) {
    static const char *LABEL = "demo_label";
    Asset_Blob label = { (uint32_t)(strlen(LABEL)), (uint8_t *)LABEL };

    Asset_Attr attr[] = {
        { .tag = ASSET_TAG_RETURN_TYPE, .value.u32 = ASSET_RETURN_ATTRIBUTES },
        { .tag = ASSET_TAG_DATA_LABEL_NORMAL_1, .value.blob = label },
        { .tag = ASSET_TAG_RETURN_OFFSET, .value.u32 = 5 },
        { .tag = ASSET_TAG_RETURN_LIMIT, .value.u32 = 10 },
        { .tag = ASSET_TAG_RETURN_ORDERED_BY, .value.u32 = ASSET_TAG_DATA_LABEL_NORMAL_1 },
    };

    Asset_ResultSet resultSet = { 0 };
    int32_t ret = OH_Asset_Query(attr, sizeof(attr) / sizeof(attr[0]), &resultSet);
    if (ret == ASSET_SUCCESS) {
        // Parse the result.
        for (uint32_t i = 0; i < resultSet.count; i++) {
            // Parse the data alias.
            Asset_Attr *alias = OH_Asset_ParseAttr(resultSet.results + i, ASSET_TAG_ALIAS);
        }
    }
    OH_Asset_FreeResultSet(&resultSet);
    napi_value result;
    napi_create_int32(env, ret, &result);
    return result;
}
```

### 约束和限制

* 批量查询关键资产

批量查询出的关键资产需要通过IPC通道传输给业务，受IPC缓冲区大小限制，建议对查询超过40条关键资产时，进行分批查询，且每次查询数量不超过40条。

## 更新关键资产

### 接口介绍

接口文档链接：

[int32_t OH_Asset_Update(const Asset_Attr *query, uint32_t queryCnt, const Asset_Attr *attributesToUpdate, uint32_t updateCnt)](../reference/native-apis/_asset_api.md#OH_Asset_Update())

query的参数列表：

| 属性名（asset.Tag）             | 属性值（asset.Value）                                        | 是否必选 | 说明                                             |
| ------------------------------- | ------------------------------------------------------------ | -------- | ------------------------------------------------ |
| ASSET_TAG_ALIAS                 | 类型为uint8[]，长度为1-256字节                               | 必选     | 关键资产别名，每条关键资产的唯一索引;            |
| ASSET_TAG_ACCESSIBILITY         | 类型为uint32_t，取值范围详见[Asset_Accessibility](../reference/native-apis/_asset_type_api.md#Asset_Accessibility) | 可选     | 访问控制属性                                     |
| ASSET_TAG_REQUIRE_PASSWORD_SET  | 类型为bool                                                   | 可选     | 关键资产是否仅在设置了锁屏密码的情况下可访问     |
| ASSET_TAG_AUTH_TYPE             | 类型为uint32_t，取值范围详见[Asset_AuthType](../reference/native-apis/_asset_type_api.md#Asset_AuthType) | 可选     | 访问关键资产所需的用户认证类型                   |
| ASSET_TAG_SYNC_TYPE             | 类型为uint32_t，取值范围详见[Asset_SyncType](../reference/native-apis/_asset_type_api.md#Asset_SyncType) | 可选     | 关键资产支持的同步类型                           |
| ASSET_TAG_IS_PERSISTENT         | 类型为bool                                                   | 可选     | 关键资产在应用卸载时是否需要保留                 |
| ASSET_TAG_DATA_LABEL_CRITICAL_1 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| ASSET_TAG_DATA_LABEL_CRITICAL_2 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| ASSET_TAG_DATA_LABEL_CRITICAL_3 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| ASSET_TAG_DATA_LABEL_CRITICAL_4 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| ASSET_TAG_DATA_LABEL_NORMAL_1   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| ASSET_TAG_DATA_LABEL_NORMAL_2   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| ASSET_TAG_DATA_LABEL_NORMAL_3   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| ASSET_TAG_DATA_LABEL_NORMAL_4   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |


attributesToUpdate的参数列表：


| 属性名（asset.Tag） | 属性值（asset.Value）           | 是否必选 | 说明                                             |
| ------------------- | ------------------------------- | -------- | ------------------------------------------------ |
| SECRET              | 类型为uint8[]，长度为1-1024字节 | 可选     | 关键资产明文                                     |
| DATA_LABEL_NORMAL_1 | 类型为uint8[]，长度为1-512字节  | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_2 | 类型为uint8[]，长度为1-512字节  | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_3 | 类型为uint8[]，长度为1-512字节  | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_4 | 类型为uint8[]，长度为1-512字节  | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |

### 代码示例

更新别名是demo_alias的关键资产，将关键资产明文更新为demo_pwd_new，附加属性更新成demo_label_new。

```c
#include <string.h>
#include "napi/native_api.h"

#include "asset_api.h"

static napi_value UpdateAsset(napi_env env, napi_callback_info info) {
    static const char *ALIAS = "demo_alias";
    static const char *SECRET = "demo_pwd_new";
    static const char *LABEL = "demo_label_new";

    Asset_Blob alias = { (uint32_t)(strlen(ALIAS)), (uint8_t *)ALIAS };
    Asset_Blob new_secret = { (uint32_t)(strlen(SECRET)), (uint8_t *)SECRET };
    Asset_Blob new_label = { (uint32_t)(strlen(LABEL)), (uint8_t *)LABEL };
    Asset_Attr query[] = { { .tag = ASSET_TAG_ALIAS, .value.blob = alias } };
    Asset_Attr attributesToUpdate[] = {
        { .tag = ASSET_TAG_SECRET, .value.blob = new_secret },
        { .tag = ASSET_TAG_DATA_LABEL_NORMAL_1, .value.blob = new_label },
    };

    int32_t ret = OH_Asset_Update(query, sizeof(query) / sizeof(query[0]), attributesToUpdate,
                                  sizeof(attributesToUpdate) / sizeof(attributesToUpdate[0]));
    napi_value result;
    napi_create_int32(env, ret, &result);
    return result;
}
```

### 约束和限制

NA


## 删除关键资产

### 接口介绍

接口文档链接：

[int32_t OH_Asset_Remove(const Asset_Attr *query, uint32_t queryCnt)](../reference/native-apis/_asset_api.md#OH_Asset_Remove())

参数列表：

| 属性名（asset.Tag）             | 属性值（asset.Value）                                        | 是否必选 | 说明                                             |
| ------------------------------- | ------------------------------------------------------------ | -------- | ------------------------------------------------ |
| ASSET_TAG_ALIAS                 | 类型为uint8[]，长度为1-256字节                               | 可选     | 关键资产别名，每条关键资产的唯一索引;            |
| ASSET_TAG_ACCESSIBILITY         | 类型为uint32_t，取值范围详见[Asset_Accessibility](../reference/native-apis/_asset_type_api.md#Asset_Accessibility) | 可选     | 访问控制属性                                     |
| ASSET_TAG_REQUIRE_PASSWORD_SET  | 类型为bool                                                   | 可选     | 关键资产是否仅在设置了锁屏密码的情况下可访问     |
| ASSET_TAG_AUTH_TYPE             | 类型为uint32_t，取值范围详见[Asset_AuthType](../reference/native-apis/_asset_type_api.md#Asset_AuthType) | 可选     | 访问关键资产所需的用户认证类型                   |
| ASSET_TAG_SYNC_TYPE             | 类型为uint32_t，取值范围详见[Asset_SyncType](../reference/native-apis/_asset_type_api.md#Asset_SyncType) | 可选     | 关键资产支持的同步类型                           |
| ASSET_TAG_IS_PERSISTENT         | 类型为bool                                                   | 可选     | 关键资产在应用卸载时是否需要保留                 |
| ASSET_TAG_DATA_LABEL_CRITICAL_1 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| ASSET_TAG_DATA_LABEL_CRITICAL_2 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| ASSET_TAG_DATA_LABEL_CRITICAL_3 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| ASSET_TAG_DATA_LABEL_CRITICAL_4 | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| ASSET_TAG_DATA_LABEL_NORMAL_1   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| ASSET_TAG_DATA_LABEL_NORMAL_2   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| ASSET_TAG_DATA_LABEL_NORMAL_3   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| ASSET_TAG_DATA_LABEL_NORMAL_4   | 类型为uint8[]，长度为1-512字节                               | 可选     | 关键资产附属信息，内容由业务自定义且无完整性保护 |

### 代码示例

删除别名是demo_alias的关键资产。

```c
#include <string.h>
#include "napi/native_api.h"

#include "asset_api.h"

static napi_value RemoveAsset(napi_env env, napi_callback_info info) {
    static const char *ALIAS = "demo_alias";
    Asset_Blob alias = { (uint32_t)(strlen(ALIAS)), (uint8_t *)ALIAS };

    Asset_Attr attr[] = {
        { .tag = ASSET_TAG_ALIAS, .value.blob = alias }, // 此处指定别名删除，也可不指定别名删除多条数据
    };

    int32_t ret = OH_Asset_Remove(attr, sizeof(attr) / sizeof(attr[0]));
    napi_value result;
    napi_create_int32(env, ret, &result);
    return result;
}
```

### 约束和限制

NA

