# 关键资产存储错误码

> **说明：**
>
> 以下仅介绍本模块特有错误码，通用错误码请参考[通用错误码说明文档](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/errorcodes/errorcode-universal.md)。

## 24000001 关键资产服务不可用

**错误信息**

The Asset service is unavailable.

**可能原因**

系统异常导致关键资产服务不可用。

**处理步骤**

重新发起关键资产处理请求。

## 24000002 未找到关键资产

**错误信息**

The queried Asset can not be found.

**可能原因**

1. 关键资产从未写入过。
2. 关键资产已经删除。

**处理步骤**

1. 根据别名确认该关键资产是否已经写入过，或已经删除。
2. 重新写入关键资产，再查询该关键资产。

## 24000003 关键资产已存在

**错误信息**

The added Asset already exists.

**可能原因**

业务请求写入同别名的关键资产，即asset.Tag.ALIAS属性相同的关键资产。

**处理步骤**

请先确认写入同别名的关键资产是否符合预期，如果不符合需排查别名是否错误，如果符合则可通过以下任意一种方式处理：

1. 先调用asset.remove销毁该别名的关键资产，再调用asset.add重新写入。
2. 调用asset.add时，需要指定参数asset.Tag.CONFLICT_RESOLUTION的值为asset.ConflictResolution.OVERWRITE

## 24000004 拒绝访问关键资产

**错误信息**

The access to Asset is denied.

**可能原因**

1. 业务在调用asset.query查询关键资产前，没有调用asset.preQuery预查询关键资产。

2. 用户在访问需要用户认证的关键资产前，没有进行用户认证。

**处理步骤**

1. 业务在调用asset.query查询关键资产前，先调用asset.preQuery预查询关键资产。
2. 用户在访问需要用户认证的关键资产前，先进行用户认证。

## 24000005 锁屏状态不匹配

**错误信息**

The screen lock status mismatches.

**可能原因**

1. 在设备处于未设置锁屏密码的状态下，访问仅设备设置密码时才允许访问的关键资产。
2. 在设备未完成首次解锁的状态下，访问仅设备首次解锁才允许访问的关键资产。
3. 在设备未处于解锁状态下，访问仅设备处于解锁才允许访问的关键资产。

**处理步骤**

给设备设置锁屏密码或解锁后，再访问关键资产。

## 24000006 系统内存不足

**错误信息**

Insufficient memory.

**可能原因**

系统内存不足。

**处理步骤**

清理后台，重新发起处理请求。

## 24000007 关键资产损坏

**错误信息**

The Asset is corrupted.

**可能原因**

因设备掉电导致存储的关键资产损坏。

**处理步骤**

调试阶段：删除data/service/el1/public/asset_service/asset.db文件，重新发起处理请求。

发布阶段：恢复出厂设置。

## 24000008 数据库操作失败

**错误信息**

The database operation is failed.

**可能原因**

数据库访问异常。

**处理步骤**

查看错误信息，排查数据库异常原因。

## 24000009 算法库操作失败

**错误信息**

The cryptography operation is failed.

**可能原因**

密码算法操作失败。

**处理步骤**

查看错误信息，排查算法库异常原因。

## 24000010 进程通信错误

**错误信息**

IPC communication is failed.

**可能原因**

进程通信错误。

**处理步骤**

查看错误信息，排查进程IPC通信异常原因。

## 24000011 包管理服务异常

**错误信息**

The operation of calling bundle manager service is failed.

**可能原因**

包管理（Bundle Framework）服务异常。

**处理步骤**

查看错误信息，排查包管理服务异常原因。

## 24000012 账号系统异常

**错误信息**

The operation of calling OS account service is failed.

**可能原因**

账号系统（OS Account）异常。

**处理步骤**

查看错误信息，排查账号系统异常原因。

## 24000013 访问控制服务异常

**错误信息**

The operation of calling access token service is failed.

**可能原因**

访问控制（Access Token）服务异常。

**处理步骤**

查看错误信息，排查访问控制服务异常原因。

## 24000014 文件操作失败

**错误信息**

The operation of file is failed.

**可能原因**

业务手动删除了设备中data/service/el1/public/asset_service某一级目录

**处理步骤**

重启设备。

## 24000015 获取系统时间失败

**错误信息**

The operation of getting system time is failed.

**可能原因**

系统时间被篡改。

**处理步骤**

调整成正确的系统时间。

## 24000016 缓存数量超限

**错误信息**

The cache exceeds the limit.

**可能原因**

业务调用asset.preQuery预查询后，没有通过asset.postQuery释放资源。

**处理步骤**

业务调用asset.preQuery预查询后，通过asset.postQuery释放资源。

## 24000017 该子功能不支持

**错误信息**

The capability is not supported.

**可能原因**

支持API，但是不支持API内部某些子特性（功能），如批量查询关键资产明文。

**处理步骤**

调整API参数，使用可替代的调用方式，如多次调用API查询关键资产。