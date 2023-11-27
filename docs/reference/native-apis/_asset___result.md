# Asset_Result


## 概述

该类型用于表示关键资产属性的键-值对集合。

**起始版本：** 11

**相关模块：**[AssetTypeApi](_asset_type_api.md)


## 汇总


### 成员变量

| 名称 | 描述 | 
| -------- | -------- |
| [count](#count) | uint32_t<br/>关键资产属性的键值对的数组大小。  | 
| [attrs](#attrs) | [Asset_Attr](_asset___attr.md) \*<br/>指向关键资产属性的键值对的数组。  | 


## 结构体成员变量说明


### attrs

```
Asset_Attr* Asset_Result::attrs
```
**描述**
指向关键资产属性的键值对的数组。


### count

```
uint32_t Asset_Result::count
```
**描述**
关键资产属性的键值对的数组大小。
