# asset_api.h


## 概述

描述用于访问Asset的接口。

**起始版本：** 11

**相关模块：**[AssetApi](_asset_api.md)


## 汇总


### 函数

| 名称 | 描述 | 
| -------- | -------- |
| [OH_Asset_Add](_asset_api.md#oh_asset_add) (const [Asset_Attr](_asset___attr.md) \*attributes, uint32_t attrCnt) | int32_t<br/>增加一条关键资产。  | 
| [OH_Asset_Remove](_asset_api.md#oh_asset_remove) (const [Asset_Attr](_asset___attr.md) \*query, uint32_t queryCnt) | int32_t<br/>删除符合匹配条件的一条或多条关键资产。  | 
| [OH_Asset_Update](_asset_api.md#oh_asset_update) (const [Asset_Attr](_asset___attr.md) \*query, uint32_t queryCnt, const [Asset_Attr](_asset___attr.md) \*attributesToUpdate, uint32_t updateCnt) | int32_t<br/>更新符合匹配条件的一条关键资产。  | 
| [OH_Asset_PreQuery](_asset_api.md#oh_asset_prequery) (const [Asset_Attr](_asset___attr.md) \*query, uint32_t queryCnt, [Asset_Blob](_asset___blob.md) \*challenge) | int32_t<br/>对于需要用户认证的关键资产的查询前的预处理（例如获取挑战值challenge）。  | 
| [OH_Asset_Query](_asset_api.md#oh_asset_query) (const [Asset_Attr](_asset___attr.md) \*query, uint32_t queryCnt, [Asset_ResultSet](_asset___result_set.md) \*resultSet) | int32_t<br/>查询一条或多条符合匹配条件的关键资产。  | 
| [OH_Asset_PostQuery](_asset_api.md#oh_asset_postquery) (const [Asset_Attr](_asset___attr.md) \*handle, uint32_t handleCnt) | int32_t<br/>对于需要用户认证的关键资产的查询后的后置处理（例如释放资源）。  | 
| [OH_Asset_ParseAttr](_asset_api.md#oh_asset_parseattr) (const [Asset_Result](_asset___result.md) \*result, [Asset_Tag](_asset_type_api.md#asset_tag) tag) | [Asset_Attr](_asset___attr.md) \*<br/>解析AssetResult以获取指定的属性。  | 
| [OH_Asset_FreeBlob](_asset_api.md#oh_asset_freeblob) ([Asset_Blob](_asset___blob.md) \*blob) | void<br/>释放从[OH_Asset_PreQuery](_asset_api.md#oh_asset_prequery)中获取的AssetBlob的内存。  | 
| [OH_Asset_FreeResultSet](_asset_api.md#oh_asset_freeresultset) ([Asset_ResultSet](_asset___result_set.md) \*resultSet) | void<br/>释放从[OH_Asset_Query](_asset_api.md#oh_asset_query)中获取的AssetResultSet的内存。  | 
