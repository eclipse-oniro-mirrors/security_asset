# Asset_ResultSet


## 概述

该类型用于表示查询关键资产返回结果集合的类型。

**起始版本：** 11

**相关模块：**[AssetTypeApi](_asset_type_api.md)


## 汇总


### 成员变量

| 名称 | 描述 | 
| -------- | -------- |
| [count](#count) | uint32_t<br/>关键资产属性的键-值对集合数组的大小。  | 
| [results](#results) | [Asset_Result](_asset___result.md) \*<br/>指向关键资产属性的键-值对集合数组。  | 


## 结构体成员变量说明


### count

```
uint32_t Asset_ResultSet::count
```
**描述**
关键资产属性的键-值对集合数组的大小。


### results

```
Asset_Result* Asset_ResultSet::results
```
**描述**
指向关键资产属性的键-值对集合数组。
