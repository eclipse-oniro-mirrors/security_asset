# AssetApi


## 概述

该模块描述Asset对用户敏感数据（如密码、Token等）的生命周期管理能力，包括添加、删除、更新、查询等。

**系统能力：** SystemCapability.Security.Asset

**起始版本：** 11


## 汇总


### 文件

| 名称 | 描述 |
| -------- | -------- |
| [asset_api.h](asset__api_8h.md) | 描述用于访问Asset的接口。  |


### 函数

| 名称 | 描述 |
| -------- | -------- |
| [OH_Asset_Add](#oh_asset_add) (const [Asset_Attr](_asset___attr.md) \*attributes, uint32_t attrCnt) | int32_t<br/>增加一条关键资产。  |
| [OH_Asset_Remove](#oh_asset_remove) (const [Asset_Attr](_asset___attr.md) \*query, uint32_t queryCnt) | int32_t<br/>删除符合匹配条件的一条或多条关键资产。  |
| [OH_Asset_Update](#oh_asset_update) (const [Asset_Attr](_asset___attr.md) \*query, uint32_t queryCnt, const [Asset_Attr](_asset___attr.md) \*attributesToUpdate, uint32_t updateCnt) | int32_t<br/>更新符合匹配条件的一条关键资产。  |
| [OH_Asset_PreQuery](#oh_asset_prequery) (const [Asset_Attr](_asset___attr.md) \*query, uint32_t queryCnt, [Asset_Blob](_asset___blob.md) \*challenge) | int32_t<br/>对于需要用户认证的关键资产的查询前的预处理（例如获取挑战值challenge）。  |
| [OH_Asset_Query](#oh_asset_query) (const [Asset_Attr](_asset___attr.md) \*query, uint32_t queryCnt, [Asset_ResultSet](_asset___result_set.md) \*resultSet) | int32_t<br/>查询一条或多条符合匹配条件的关键资产。  |
| [OH_Asset_PostQuery](#oh_asset_postquery) (const [Asset_Attr](_asset___attr.md) \*handle, uint32_t handleCnt) | int32_t<br/>对于需要用户认证的关键资产的查询后的后置处理（例如释放资源）。  |
| [OH_Asset_ParseAttr](#oh_asset_parseattr) (const [Asset_Result](_asset___result.md) \*result, [Asset_Tag](_asset_type_api.md#asset_tag) tag) | [Asset_Attr](_asset___attr.md) \*<br/>解析AssetResult以获取指定的属性。  |
| [OH_Asset_FreeBlob](#oh_asset_freeblob) ([Asset_Blob](_asset___blob.md) \*blob) | void<br/>释放从[OH_Asset_PreQuery](#oh_asset_prequery)中获取的AssetBlob的内存。  |
| [OH_Asset_FreeResultSet](#oh_asset_freeresultset) ([Asset_ResultSet](_asset___result_set.md) \*resultSet) | void<br/>释放从[OH_Asset_Query](#oh_asset_query)中获取的AssetResultSet的内存。  |


## 函数说明


### OH_Asset_Add()

```
int32_t OH_Asset_Add (const Asset_Attr * attributes, uint32_t attrCnt )
```
**描述**
增加一条关键资产。

**起始版本：** 11

**参数:**

| 名称 | 描述 |
| -------- | -------- |
| attributes | 指向包含待添加到关键资产的属性数组的指针。  |
| attrCnt | attributes数组中元素的个数。  |

**返回：**

如果操作成功，则返回[Asset_ResultCode#ASSET_SUCCESS](_asset_type_api.md#Asset_ResultCode)；否则返回错误代码。


### OH_Asset_FreeBlob()

```
void OH_Asset_FreeBlob (Asset_Blob * blob)
```
**描述**
释放从[OH_Asset_PreQuery](#oh_asset_prequery)中获取的AssetBlob的内存。

**起始版本：** 11

**参数:**

| 名称 | 描述 |
| -------- | -------- |
| blob | 指向需要释放的AssetBlob的指针。  |


### OH_Asset_FreeResultSet()

```
void OH_Asset_FreeResultSet (Asset_ResultSet * resultSet)
```
**描述**
释放从[OH_Asset_Query](#oh_asset_query)中获取的AssetResultSet的内存。

**起始版本：** 11

**参数:**

| 名称 | 描述 |
| -------- | -------- |
| resultSet | 指向从[OH_Asset_Query](#oh_asset_query)得到的查询结果的指针。  |


### OH_Asset_ParseAttr()

```
Asset_Attr* OH_Asset_ParseAttr (const Asset_Result * result, Asset_Tag tag )
```
**描述**
解析AssetResult以获取指定的属性。

**起始版本：** 11

**参数:**

| 名称 | 描述 |
| -------- | -------- |
| result | 指向包含从[OH_Asset_Query](#oh_asset_query)中获取的查询结果的数组指针。  |
| tag | 指定属性的标签。  |

**返回：**

如果操作成功，则以[Asset_Attr](_asset___attr.md)的形式返回属性，该属性不需要业务进行释放； 否则返回NULL。


### OH_Asset_PostQuery()

```
int32_t OH_Asset_PostQuery (const Asset_Attr * handle, uint32_t handleCnt )
```
**描述**
对于需要用户认证的关键资产的查询后的后置处理（例如释放资源）。

**起始版本：** 11

**参数:**

| 名称 | 描述 |
| -------- | -------- |
| handle | 指向从[OH_Asset_PreQuery](#oh_asset_prequery)中获取的包含挑战值的数组指针。  |
| handleCnt | handle数组中元素的个数。  |

**返回：**

如果操作成功，则返回[Asset_ResultCode#ASSET_SUCCESS](_asset_type_api.md)；否则返回错误代码。


### OH_Asset_PreQuery()

```
int32_t OH_Asset_PreQuery (const Asset_Attr * query, uint32_t queryCnt, Asset_Blob * challenge )
```
**描述**
对于需要用户认证的关键资产的查询前的预处理（例如获取挑战值challenge）。

**起始版本：** 11

**参数:**

| 名称 | 描述 |
| -------- | -------- |
| query | 指向包含用来匹配待查询关键资产的属性数组的指针。  |
| queryCnt | query数组中元素的个数。  |
| challenge | 获取到的挑战值指针，在后续调用[OH_Asset_Query](#oh_asset_query)时使用。  |

**返回：**

如果操作成功，则返回[Asset_ResultCode#ASSET_SUCCESS](_asset_type_api.md)；否则返回错误代码。


### OH_Asset_Query()

```
int32_t OH_Asset_Query (const Asset_Attr * query, uint32_t queryCnt, Asset_ResultSet * resultSet )
```
**描述**
查询一条或多条符合匹配条件的关键资产。

**起始版本：** 11

**参数:**

| 名称 | 描述 |
| -------- | -------- |
| query | 指向包含用来匹配待查询关键资产的属性数组的指针。  |
| queryCnt | query数组中元素的个数。  |
| result | 指向包含查询结果的数组的指针。  |

**返回：**

如果操作成功，则返回[Asset_ResultCode#ASSET_SUCCESS](_asset_type_api.md)；否则返回错误代码。


### OH_Asset_Remove()

```
int32_t OH_Asset_Remove (const Asset_Attr * query, uint32_t queryCnt )
```
**描述**
删除符合匹配条件的一条或多条关键资产。

**起始版本：** 11

**参数:**

| 名称 | 描述 |
| -------- | -------- |
| query | 指向包含用来匹配待删除关键资产的属性数组的指针。  |
| queryCnt | query数组中元素的个数。  |

**返回：**

如果操作成功，则返回[Asset_ResultCode#ASSET_SUCCESS](_asset_type_api.md)；否则返回错误代码。


### OH_Asset_Update()

```
int32_t OH_Asset_Update (const Asset_Attr * query, uint32_t queryCnt, const Asset_Attr * attributesToUpdate, uint32_t updateCnt )
```
**描述**
更新符合匹配条件的一条关键资产。

**起始版本：** 11

**参数:**

| 名称 | 描述 |
| -------- | -------- |
| query | 指向包含用来匹配待更新关键资产的属性数组的指针。  |
| queryCnt | query数组中元素的个数。  |
| attributesToUpdate | 指向包含更新的关键资产的属性数组的指针。  |
| updateCnt | attributesToUpdate数组中元素的个数。  |

**返回：**

如果操作成功，则返回[Asset_ResultCode#ASSET_SUCCESS](_asset_type_api.md)；否则返回错误代码。
