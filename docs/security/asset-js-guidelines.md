# 关键资产存储服务（ASSET）开发指导

[toc]

## 场景1：保护密码类数据
### 场景说明

用户在应用/浏览器上登录账号时，可以选择“记住密码”，如下图所示。此时应用/浏览器可以将用户密码存储在ASSET中，由ASSET保证用户密码的机密性。

用户再次进入登录界面时，应用/浏览器可从ASSET中查询用户密码明文，并自动填充到密码输入框，用户只需点击“登录”按钮即可完成账号登录，极大地提升了用户的登录体验。

 <img src="../../figures/login.png" alt="login" style="zoom:40%;" />



> **说明：**
>
> 密码类数据可以是密码、登录令牌、信用卡号等用户敏感数据。

### 关键流程

应用/浏览器调用ASSET完成上述功能，可以参照以下流程进行开发，场景



###  <img src="../../figures/flowchat-no-auth-required.png" alt="flowchat" style="zoom:40%;" />

1. 业务查询符合条件的关键资产属性，根据是否查询出结果判断关键资产是否存在，开发步骤参考[查询无需用户认证的关键资产](#查询无需用户认证的关键资产)  **// todo 待修改**
2. 如果关键资产不存在，新增关键资产，开发步骤参考  **// todo 待修改***
  * 删除关键资产，开发步骤参考  **// todo 待修改**
  * 修改关键资产，开发步骤参考  **// todo 待修改**
  * 查询关键资产明文，开发步骤参考  **// todo 待修改**

## 场景2：保护需要用户身份认证的密码 



xxx

应用程序有时对于关键资产数据的存储有着更高的要求，他们不满足于 *用户场景1（后面第一个确定好标题后改成标题1）* ，而是希望只有获得用户的授权时，关键资产数据才可被访问。

Asset在基础关键资产管理能力之外，提供了可选的用户认证访问控制能力。在向Asset写入数据时，如果指定了该关键资产数据需要用户认证才可访问，在读取数据时便需要先通过*（IDM，后面改名和跳转）*进行用户认证，并将用户认证成功的Token传入Asset，方可对该关键资产数据进行访问。

### 流程图

  <img src="../../figures/flowchat-auth-required.png" alt="flowchat" style="zoom:40%;" />



## 新增关键资产

- 接口和必选参数介绍

接口和使用方式可参考：

[function add(attributes: AssetMap, callback: AsyncCallback<void>): void](../reference/apis/js-apis-asset.md#asset.add)

[function add(attributes: AssetMap): Promise<void>](../reference/apis/js-apis-asset.md#asset.add-1)

| 必选参数名称 | 描述 |
| -------- | -------- |
| ALIAS    | 关键资产别名，每条关键资产的唯一索引 |
| SECRET   | 关键资产明文 |


- 代码示例

  xxx


- 可选参数介绍

| 可选参数名称 | 描述 |
| -------- | -------- |
| ACCESSIBILITY    | 访问控制属性，取值范围详见[asset.Accessibility](../reference/apis/js-apis-asset.md#asset.Accessibility) |
| REQUIRE_PASSWORD_SET   | 关键资产是否仅在设置了锁屏密码的情况下可访问 |
| AUTH_TYPE   | 访问关键资产所需的用户认证类，取值范围详见[asset.AuthType](../reference/apis/js-apis-asset.md#asset.AuthType) |
| SYNC_TYPE   | 关键资产支持的同步类，取值范围详见[asset.SyncType](../reference/apis/js-apis-asset.md#asset.SyncType) |
| CONFLICT_RESOLUTION   | 写入同别名的关键资产时的处理策略，取值范围详见[asset.ConflictResolution](../reference/apis/js-apis-asset.md#asset.ConflictResolution) |
| DATA_LABEL_CRITICAL_1   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_2   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_3   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_4   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_NORMAL_1   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_2   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_3   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_4   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| IS_PERSISTENT   | 关键资产在应用卸载时是否需要保留<br>**需要权限：ohos.permission.STORE_PERSISTENT_DATA** |


- 约束和限制（会话？？性能？？并发？？存储数量？？）


## 查询关键资产

### 查询无需用户认证的关键资产
- 使用场景

业务向Asset中写入关键资产数据后，可以通过该能力将数据读取出来。读取关键资产数据可分为单条查询和批量查询两种。精确查询时可获取到关键资产（asset.Tag.SECRET），必传关键资产别名（asset.Tag.SECRET.ALIAS）；批量查询时可获取到关键资产属性，不传关键资产别名（asset.Tag.SECRET.ALIAS）。

- 接口和必选参数介绍

接口和使用方式可参考：

[function query(query: AssetMap, callback: AsyncCallback<Array<AssetMap>>): void](../reference/apis/js-apis-asset.md#asset.query)

[function query(query: AssetMap): Promise<Array<AssetMap>>](../reference/apis/js-apis-asset.md#asset.query-1)

无必选参数。当参数为空时，批量查询属主所有关键资产属性。

- 代码示例


- 可选参数介绍

| 可选参数名称 | 描述 |
| -------- | -------- |
| ALIAS | 关键资产别名，每条关键资产的唯一索引。单条查询时必传。 |
| ACCESSIBILITY    | 访问控制属性，取值范围详见[asset.Accessibility](../reference/apis/js-apis-asset.md#asset.Accessibility) |
| AUTH_TYPE   | 访问关键资产所需的用户认证类，取值范围详见[asset.AuthType](../reference/apis/js-apis-asset.md#asset.AuthType) |
| DATA_LABEL_CRITICAL_1   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_2   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_3   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_4   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_NORMAL_1   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_2   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_3   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_4   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| SYNC_TYPE   | 关键资产支持的同步类，取值范围详见[asset.SyncType](../reference/apis/js-apis-asset.md#asset.SyncType) |
| RETURN_TYPE   | 关键资产查询结果类型，取值范围详见[asset.ReturnType](../reference/apis/js-apis-asset.md#asset.ReturnType)。**当需要查询关键资产明文时必传asset.ReturnType.ALL，仅支持单条查询时使用。** |
| RETURN_LIMIT   | 关键资产查询结果数量 |
| RETURN_OFFSET   | 满足查询条件的关键资产偏移量 |
| RETURN_ORDERED_BY   | 关键资产查询结果排序依据，仅支持指定按照附属信息排序，不指定的情况下，默认按照关键资产写入的顺序排序。取值范围：asset.Tag.DATA_LABEL_xxx |


- 约束限制（会话？？性能？？并发？？存储数量？？）


### 需要用户认证

- 使用场景

当访问需要用户授权才能访问的关键资产数据时，如仅查询关键资产属性，与上文不传RETURN_TYPE或传入ATTRIBUTES时相同；如需查询关键资产明文，除了上述步骤外，需要使用预查询、后查询接口进行用户认证相关操作。

- 接口和必选参数介绍（参数名、参数类型、参数限制）

接口和使用方式可参考：
[function preQuery(query: AssetMap, callback: AsyncCallback<Uint8Array>): void](../reference/apis/js-apis-asset.md#asset.preQuery)

[function preQuery(query: AssetMap): Promise<Uint8Array>](../reference/apis/js-apis-asset.md#asset.preQuery-1)

无必选参数。当参数为空时，为所有关键资产的查询做预处理准备。

| 可选参数名称 | 描述 |
| -------- | -------- |
| ALIAS | 关键资产别名，每条关键资产的唯一索引。单条查询时必传。 |
| ACCESSIBILITY    | 访问控制属性，取值范围详见[asset.Accessibility](../reference/apis/js-apis-asset.md#asset.Accessibility) |
| AUTH_TYPE   | 访问关键资产所需的用户认证类，取值范围详见[asset.AuthType](../reference/apis/js-apis-asset.md#asset.AuthType) |
| SYNC_TYPE   | 关键资产支持的同步类，取值范围详见[asset.SyncType](../reference/apis/js-apis-asset.md#asset.SyncType) |
| AUTH_VALIDITY_PERIOD   | 用户认证的有效期，取值范围：1-600，单位为秒 |
| DATA_LABEL_CRITICAL_1   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_2   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_3   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_4   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_NORMAL_1   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_2   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_3   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_4   | 关键资产附属信息，内容由业务自定义且无完整性保护 |

[function query(query: AssetMap, callback: AsyncCallback<Array<AssetMap>>): void](../reference/apis/js-apis-asset.md#asset.query)

[function query(query: AssetMap): Promise<Array<AssetMap>>](../reference/apis/js-apis-asset.md#asset.query-1)

| 必选参数名称 | 描述 |
| -------- | -------- |
| RETURN_TYPE   | 关键资产查询结果类型，取值范围详见[asset.ReturnType](../reference/apis/js-apis-asset.md#asset.ReturnType)。需传入asset.ReturnType.ALL |
| AUTH_CHALLENGE   | 用户认证使用的挑战值 |
| AUTH_TOKEN   | 认证通过的授权令牌 |

| 可选参数名称 | 描述 |
| -------- | -------- |
| ALIAS | 关键资产别名，每条关键资产的唯一索引。单条查询时必传。 |
| ACCESSIBILITY    | 访问控制属性，取值范围详见[asset.Accessibility](../reference/apis/js-apis-asset.md#asset.Accessibility) |
| AUTH_TYPE   | 访问关键资产所需的用户认证类，取值范围详见[asset.AuthType](../reference/apis/js-apis-asset.md#asset.AuthType) |
| DATA_LABEL_CRITICAL_1   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_2   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_3   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_4   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_NORMAL_1   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_2   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_3   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_4   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| SYNC_TYPE   | 关键资产支持的同步类，取值范围详见[asset.SyncType](../reference/apis/js-apis-asset.md#asset.SyncType) |
| RETURN_LIMIT   | 关键资产查询结果数量 |
| RETURN_OFFSET   | 满足查询条件的关键资产偏移量 |
| RETURN_ORDERED_BY   | 关键资产查询结果排序依据，仅支持指定按照附属信息排序，不指定的情况下，默认按照关键资产写入的顺序排序。取值范围：asset.Tag.DATA_LABEL_xxx |

[function postQuery(handle: AssetMap, callback: AsyncCallback<void>): void](../reference/apis/js-apis-asset.md#asset.postQuery)

[function postQuery(handle: AssetMap): Promise<void>](../reference/apis/js-apis-asset.md#asset.postQuery-1)

| 必选参数名称 | 描述 |
| -------- | -------- |
| AUTH_CHALLENGE   | 用户认证使用的挑战值 |


- 代码示例




- 约束限制（会话？？性能？？并发？？存储数量？？）


## 更新关键资产

- 使用场景

业务可以更改在Asset存储的部分关键资产数据，当前支持更改关键资产（asset.Tag.SECRET）和自定义的Normal Label字段（asset.Tag.DATA_LABEL_NORMAL_xxx）。

- 接口和必选参数介绍（参数名、参数类型、参数限制）

接口和使用方式可参考：

[function update(query: AssetMap, attributesToUpdate: AssetMap, callback: AsyncCallback<void>): void](../reference/apis/js-apis-asset.md#asset.update)

[function update(query: AssetMap, attributesToUpdate: AssetMap): Promise<void>](../reference/apis/js-apis-asset.md#asset.update-1)

该接口有query和attributesToUpdate两个参数集。其中query是待更新关键资产的查询条件，如关键资产别名、访问控制属性、自定义数据等。attributesToUpdate是待更新关键资产及其属性，如关键资产明文、自定义数据等。

query的必选参数有：

| 必选参数名称 | 描述 |
| -------- | -------- |
| ALIAS | 关键资产别名，每条关键资产的唯一索引。单条查询时必传。 |

query无可选参数。

attributesToUpdate无必选参数。

attributesToUpdate的可选参数有：

| 可选参数名称 | 描述 |
| -------- | -------- |
| DATA_LABEL_NORMAL_1   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_2   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_3   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_4   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| SECRET   | 关键资产明文 |


- 代码示例



## 删除关键资产

- 使用场景

业务在该阶段将需要Asset管理的关键资产数据传给Asset，并在此阶段可以指定该关键资产数据的访问控制策略、同步策略等属性。

- 接口和必选参数介绍（参数名、参数类型、参数限制）

接口和使用方式可参考：

[function remove(query: AssetMap, callback: AsyncCallback<void>): void](../reference/apis/js-apis-asset.md#asset.remove)

[function remove(query: AssetMap): Promise<void>](../reference/apis/js-apis-asset.md#asset.remove-1)

该接口无必选参数，当如空参时，删除该业务所属所有关键资产数据。


- 代码示例


- 可选参数介绍

| 可选参数名称 | 描述 |
| -------- | -------- |
| ALIAS | 关键资产别名，每条关键资产的唯一索引。 |
| ACCESSIBILITY    | 访问控制属性，取值范围详见[asset.Accessibility](../reference/apis/js-apis-asset.md#asset.Accessibility) |
| AUTH_TYPE   | 访问关键资产所需的用户认证类，取值范围详见[asset.AuthType](../reference/apis/js-apis-asset.md#asset.AuthType) |
| SYNC_TYPE   | 关键资产支持的同步类，取值范围详见[asset.SyncType](../reference/apis/js-apis-asset.md#asset.SyncType) |
| DATA_LABEL_CRITICAL_1   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_2   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_3   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_CRITICAL_4   | 关键资产附属信息，内容由业务自定义且有完整性保护 |
| DATA_LABEL_NORMAL_1   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_2   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_3   | 关键资产附属信息，内容由业务自定义且无完整性保护 |
| DATA_LABEL_NORMAL_4   | 关键资产附属信息，内容由业务自定义且无完整性保护 |