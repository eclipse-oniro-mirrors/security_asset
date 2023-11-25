# 关键资产存储错误码

> **说明：**
>
> 以下仅介绍本模块特有错误码，通用错误码请参考[通用错误码说明文档](errorcode-universal.md)。

## 24000001

**错误信息**

The Asset service is unavailable.

**可能原因**

关键资产进程崩溃。

**处理步骤**

重试，

## 24000002

**错误信息**

The queried Asset can not be found.

**可能原因**

关键资产未写入过，或已经删除。

**处理步骤**

1. 在关键资产写入成功、删除成功后加日志，确认在查询关键资产前是否已写入或删除过数据。
2. 重新写入关键资产，再查询该关键资产。

## 24000003

**错误信息**

The added Asset already exists.

**可能原因**

业务正写入同别名的关键资产，即asset.Tag.ALIAS属性相同的关键资产。

**处理步骤**

请先确认写入同别名的关键资产是否符合预期，如果不符合需排查别名是否错误，如果符合则可通过以下任意一种方式处理

1. 先调用asset.remove销毁该别名的关键资产，再调用asset.add重新写入。
2. 调用asset.add时，需要指定参数asset.Tag.CONFLICT_RESOLUTION的值为asset.ConflictResolution.OVERWRITE

## 24000004

**错误信息**

The access to Asset is denied.

**可能原因**

1. 业务在调用asset.query查询关键资产前，没有调用asset.preQuery预查询关键资产。

2. 用户在访问需要用户认证的关键资产前，没有进行用户认证。

**处理步骤**

1. 业务在调用asset.query查询关键资产前，先调用asset.preQuery预查询关键资产。
2. 用户在访问需要用户认证的关键资产前，先进行用户认证。

## 24000005

**错误信息**

The screen lock status mismatches.

**可能原因**

1. 在设备处于未设置锁屏密码的状态下，访问仅设备设置密码时才允许访问的关键资产。
2. 在设备未完成首次解锁的状态下，访问仅设备首次解锁才允许访问的关键资产。
3. 在设备未处于解锁状态下，访问仅设备处于解锁才允许访问的关键资产。

**处理步骤**

给设备设置锁屏密码或解锁后，再访问关键资产。

## 24000006

**错误信息**

Insufficient memory.

**可能原因**

系统内存不足。

**处理步骤**

清理后台，重新发起处理请求。

## 24000007

**错误信息**

The Asset is corrupted.

**可能原因**

因设备掉电导致关键资产损坏。

**处理步骤**

调试阶段：删除data/service/el1/public/asset_service/asset.db目录后重试

发布阶段：

## 24000008

**错误信息**

The database operation is failed.

**可能原因**

**处理步骤**

## 24000009

**错误信息**

The cryptography operation is failed.

**可能原因**

**处理步骤**

## 24000010

**错误信息**

IPC communication is failed.

**可能原因**

**处理步骤**

## 24000011

**错误信息**

The operation of calling bundle manager service is failed.

**可能原因**

**处理步骤**

## 24000012

**错误信息**

The operation of calling OS account service is failed.

**可能原因**

**处理步骤**

## 24000013

**错误信息**

The operation of calling access token service is failed.

**可能原因**

**处理步骤**

## 24000014

**错误信息**

The operation of file is failed.

**可能原因**

**处理步骤**

## 24000015

**错误信息**

The operation of getting system time is failed.

**可能原因**

**处理步骤**

## 24000016

**错误信息**

The amount of map element or other limited quotas exceed the limit.

**可能原因**

**处理步骤**

## 24000017

**错误信息**

The capability is not supported.

**可能原因**

**处理步骤**