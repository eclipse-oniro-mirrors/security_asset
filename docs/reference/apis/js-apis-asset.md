# @ohos.security.asset (关键资产存储)

关键资产存储提供了用户短敏感数据的安全存储及管理能力。其中，短敏感数据可以是密码类（账号/密码）、Token类（应用凭据）、其他关键明文（如银行卡号）等长度较短的用户敏感数据。

>  **说明：**
>
> 本模块首批接口从API version 11 开始支持。后续版本的新增接口，采用上角标单独标记接口的起始版本。

## 导入模块

```typescript
import asset from '@ohos.security.asset';
```

## asset.add

function add(attributes: AssetMap, callback: AsyncCallback<void>): void

新增一条关键资产，使用Callback回调异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名     | 类型                 | 必填 | 说明                                                         |
| ---------- | -------------------- | ---- | ------------------------------------------------------------ |
| attributes | AssetMap             | 是   | 待新增关键资产的属性集合，包括关键资产明文、访问控制属性、自定义数据等。 |
| callback   | AsyncCallback\<void> | 是   | 关键资产新增结果的回调，未捕获到error代表关键资产新增成功，若捕获到error，则代表关键资产新增失败。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                   |
| -------- | ---------------------------------------------------------- |
| 201      | The caller doesn't have the permission.                    |
| 401      | The argument is invalid.                                   |
| 24000001 | The ASSET Service is unavailable.                          |
| 24000003 | The Asset already exists.                                  |
| 24000005 | The screen lock status mismatches.                         |
| 24000006 | Insufficient memory.                                       |
| 24000007 | The Asset is corrupted.                                    |
| 24000008 | The database operation is failed.                          |
| 24000009 | The cryptography operation is failed.                      |
| 24000010 | IPC communication is failed                                |
| 24000011 | The operation of calling Bundle Manager Service is failed. |
| 24000012 | The operation of calling OS Account Service is failed.     |
| 24000013 | The operation of calling Access Token Service is failed.   |
| 24000014 | The operation of file is failed.                           |
| 24000015 | The operation of getting system time is failed.            |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';
import util from '@ohos.util';
import { BusinessError } from '@ohos.base';

function stringToArray(str: string): Uint8Array {
  let textEncoder = new util.TextEncoder();
  return textEncoder.encodeInto(str);
}

let attr: asset.AssetMap = new Map();
attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'));
attr.set(asset.Tag.ALIAS, stringToArray('demo_alias'));
attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_FIRST_UNLOCKED);
attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray('demo_label'));
try {
    asset.add(attr, (error: BusinessError) => {
        if (error) {
            console.error(`Failed to add Asset.`);
        } else {
            console.info(`Asset added successfully.`);
        }
    });
} catch (error) {
    console.error(`Failed to add Asset.`);
}
```

## asset.add

function add(attributes: AssetMap): Promise<void>

新增一条关键资产，使用Promise方式异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名     | 类型     | 必填 | 说明                                                         |
| ---------- | -------- | ---- | ------------------------------------------------------------ |
| attributes | AssetMap | 是   | 待新增关键资产的属性集合，包括关键资产明文、访问控制属性、自定义数据等。 |

**返回值：**

| 类型          | 说明                    |
| ------------- | ----------------------- |
| Promise<void> | Promise对象，无返回值。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                   |
| -------- | ---------------------------------------------------------- |
| 201      | The caller doesn't have the permission.                    |
| 401      | The argument is invalid.                                   |
| 24000001 | The ASSET Service is unavailable.                          |
| 24000003 | The Asset already exists.                                  |
| 24000005 | The screen lock status mismatches.                         |
| 24000006 | Insufficient memory.                                       |
| 24000007 | The Asset is corrupted.                                    |
| 24000008 | The database operation is failed.                          |
| 24000009 | The cryptography operation is failed.                      |
| 24000010 | IPC communication is failed                                |
| 24000011 | The operation of calling Bundle Manager Service is failed. |
| 24000012 | The operation of calling OS Account Service is failed.     |
| 24000013 | The operation of calling Access Token Service is failed.   |
| 24000014 | The operation of file is failed.                           |
| 24000015 | The operation of getting system time is failed.            |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';
import util from '@ohos.util';

function stringToArray(str: string): Uint8Array {
  let textEncoder = new util.TextEncoder();
  return textEncoder.encodeInto(str);
}

let attr: asset.AssetMap = new Map();
attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'));
attr.set(asset.Tag.ALIAS, stringToArray('demo_alias'));
attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_FIRST_UNLOCKED);
attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray('demo_label'));
try {
    asset.add(attr).then(() => {
        console.info(`Asset added successfully.`);
    }).catch(() => {
        console.error(`Failed to add Asset.`);
    })
} catch (error) {
    console.error(`Failed to add Asset.`);
}
```

## asset.remove

function remove(query: AssetMap, callback: AsyncCallback<void>): void

删除符合条件的一条或多条关键资产，使用Callback回调异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名   | 类型                 | 必填 | 说明                                                         |
| -------- | -------------------- | ---- | ------------------------------------------------------------ |
| query    | AssetMap             | 是   | 待删除关键资产的搜索条件，如别名、访问控制属性、自定义数据等。       |
| callback | AsyncCallback\<void> | 是   | 关键资产删除结果的回调，未捕获到error代表关键资产删除成功，若捕获到error，则代表关键资产删除失败。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                   |
| -------- | ---------------------------------------------------------- |
| 401      | The argument is invalid.                                   |
| 24000001 | The ASSET Service is unavailable.                          |
| 24000002 | The queried Asset can not be found.                        |
| 24000006 | Insufficient memory.                                       |
| 24000007 | The Asset is corrupted.                                    |
| 24000008 | The database operation is failed.                          |
| 24000009 | The cryptography operation is failed.                      |
| 24000010 | IPC communication is failed                                |
| 24000011 | The operation of calling Bundle Manager Service is failed. |
| 24000012 | The operation of calling OS Account Service is failed.     |
| 24000013 | The operation of calling Access Token Service is failed.   |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';
import util from '@ohos.util';
import { BusinessError } from '@ohos.base';

function stringToArray(str: string): Uint8Array {
  let textEncoder = new util.TextEncoder();
  return textEncoder.encodeInto(str);
}

let query: asset.AssetMap = new Map();
query.set(asset.Tag.ALIAS, stringToArray('demo_alias'));
try {
    asset.remove(query, (error: BusinessError) => {
        if (error) {
            console.error(`Failed to remove Asset.`);
        } else {
            console.info(`Asset removed successfully.`);
        }
    });
} catch (error) {
    console.error(`Failed to remove Asset.`);
}
```

## asset.remove

function remove(query: AssetMap): Promise<void>

删除符合条件的一条或多条关键资产，使用Promise方式异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名 | 类型     | 必填 | 说明                                                   |
| ------ | -------- | ---- | ------------------------------------------------------ |
| query  | AssetMap | 是   | 待删除关键资产的搜索条件，如别名、访问控制属性、自定义数据等。 |

**返回值：**

| 类型          | 说明                    |
| ------------- | ----------------------- |
| Promise<void> | Promise对象，无返回值。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                   |
| -------- | ---------------------------------------------------------- |
| 401      | The argument is invalid.                                   |
| 24000001 | The ASSET Service is unavailable.                          |
| 24000002 | The queried Asset can not be found.                        |
| 24000006 | Insufficient memory.                                       |
| 24000007 | The Asset is corrupted.                                    |
| 24000008 | The database operation is failed.                          |
| 24000009 | The cryptography operation is failed.                      |
| 24000010 | IPC communication is failed                                |
| 24000011 | The operation of calling Bundle Manager Service is failed. |
| 24000012 | The operation of calling OS Account Service is failed.     |
| 24000013 | The operation of calling Access Token Service is failed.   |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';
import util from '@ohos.util';

function stringToArray(str: string): Uint8Array {
  let textEncoder = new util.TextEncoder();
  return textEncoder.encodeInto(str);
}

let query: asset.AssetMap = new Map();
query.set(asset.Tag.ALIAS, stringToArray('demo_alias'));
try {
    asset.remove(query).then(() => {
        console.info(`Asset removed successfully.`);
    }).catch(() => {
        console.error(`Failed to remove Asset.`);
    });
} catch (error) {
    console.error(`Failed to remove Asset.`);
}
```

## asset.update

function update(query: AssetMap, attributesToUpdate: AssetMap, callback: AsyncCallback<void>): void

更新符合条件的一条关键资产，使用Callback回调异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名             | 类型                 | 必填 | 说明                                                         |
| ------------------ | -------------------- | ---- | ------------------------------------------------------------ |
| query              | AssetMap             | 是   | 待更新关键资产的搜索条件，如关键资产别名、访问控制属性、自定义数据等。 |
| attributesToUpdate | AssetMap             | 是   | 待更新关键资产的属性集合，如关键资产明文、自定义数据等。       |
| callback           | AsyncCallback\<void> | 是   | 关键资产更新结果的回调，未捕获到error代表关键资产更新成功，若捕获到error，则代表关键资产更新失败。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                   |
| -------- | ---------------------------------------------------------- |
| 401      | The argument is invalid.                                   |
| 24000001 | The ASSET Service is unavailable.                          |
| 24000002 | The queried Asset can not be found.                        |
| 24000005 | The screen lock status mismatches.                         |
| 24000006 | Insufficient memory.                                       |
| 24000007 | The Asset is corrupted.                                    |
| 24000008 | The database operation is failed.                          |
| 24000009 | The cryptography operation is failed.                      |
| 24000010 | IPC communication is failed                                |
| 24000011 | The operation of calling Bundle Manager Service is failed. |
| 24000012 | The operation of calling OS Account Service is failed.     |
| 24000013 | The operation of calling Access Token Service is failed.   |
| 24000015 | The operation of getting system time is failed.            |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';
import util from '@ohos.util';
import { BusinessError } from '@ohos.base';

function stringToArray(str: string): Uint8Array {
  let textEncoder = new util.TextEncoder();
  return textEncoder.encodeInto(str);
}

let query: asset.AssetMap = new Map();
query.set(asset.Tag.ALIAS, stringToArray('demo_alias'));
let attrsToUpdate: asset.AssetMap = new Map();
attrsToUpdate.set(asset.Tag.SECRET, stringToArray('demo_pwd_new'));
try {
    asset.update(query, attrsToUpdate, (error: BusinessError) => {
        if (error) {
            console.error(`Failed to update Asset.`);
        } else {
            console.info(`Asset updated successfully.`);
        }
    });
} catch (error) {
    console.error(`Failed to update Asset.`);
}
```

## asset.update

function update(query: AssetMap, attributesToUpdate: AssetMap): Promise<void>

更新符合条件的一条关键资产，使用Promise方式异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名             | 类型     | 必填 | 说明                                                         |
| ------------------ | -------- | ---- | ------------------------------------------------------------ |
| query              | AssetMap | 是   | 待更新关键资产的搜索条件，如关键资产别名、访问控制属性、自定义数据等。 |
| attributesToUpdate | AssetMap | 是   | 待更新关键资产的属性集合，如关键资产明文、自定义数据等。       |

**返回值：**

| 类型          | 说明                    |
| ------------- | ----------------------- |
| Promise<void> | Promise对象，无返回值。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                   |
| -------- | ---------------------------------------------------------- |
| 401      | The argument is invalid.                                   |
| 24000001 | The ASSET Service is unavailable.                          |
| 24000002 | The queried Asset can not be found.                        |
| 24000005 | The screen lock status mismatches.                         |
| 24000006 | Insufficient memory.                                       |
| 24000007 | The Asset is corrupted.                                    |
| 24000008 | The database operation is failed.                          |
| 24000009 | The cryptography operation is failed.                      |
| 24000010 | IPC communication is failed                                |
| 24000011 | The operation of calling Bundle Manager Service is failed. |
| 24000012 | The operation of calling OS Account Service is failed.     |
| 24000013 | The operation of calling Access Token Service is failed.   |
| 24000015 | The operation of getting system time is failed.            |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';
import util from '@ohos.util';

function stringToArray(str: string): Uint8Array {
  let textEncoder = new util.TextEncoder();
  return textEncoder.encodeInto(str);
}

let query: asset.AssetMap = new Map();
query.set(asset.Tag.ALIAS, stringToArray('demo_alias'));
let attrsToUpdate: asset.AssetMap = new Map();
attrsToUpdate.set(asset.Tag.SECRET, stringToArray('demo_pwd_new'));
try {
    asset.update(query, attrsToUpdate).then(() => {
        console.info(`Asset updated successfully.`);
    }).catch(() => {
        console.error(`Failed to update Asset.`);
    });
} catch (error) {
    console.error(`Failed to update Asset.`);
}
```

## asset.preQuery

function preQuery(query: AssetMap, callback: AsyncCallback<Uint8Array>): void

查询的预处理，用于需要用户认证的关键资产。在用户认证成功后，应当随后调用[asset.query](#asset.query)、[asset.postQuery](#asset.postQuery)。使用Callback回调异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名   | 类型                       | 必填 | 说明                                                         |
| -------- | -------------------------- | ---- | ------------------------------------------------------------ |
| query    | AssetMap                   | 是   | 关键资产的查询条件，如别名、访问控制属性、自定义数据等。       |
| callback | AsyncCallback\<Uint8Array> | 是   | 关键资产查询预处理结果的回调，未捕获到error时，返回挑战值，表示预处理成功，若捕获到error，则代表关键资产预处理失败。<br>注：挑战值用于后续用户认证。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                     |
| -------- | ------------------------------------------------------------ |
| 401      | The argument is invalid.                                     |
| 24000001 | The ASSET Service is unavailable.                            |
| 24000002 | The queried Asset can not be found.                          |
| 24000005 | The screen lock status mismatches.                           |
| 24000006 | Insufficient memory.                                         |
| 24000007 | The Asset is corrupted.                                      |
| 24000008 | The database operation is failed.                            |
| 24000009 | The cryptography operation is failed.                        |
| 24000010 | IPC communication is failed                                  |
| 24000011 | The operation of calling Bundle Manager Service is failed.   |
| 24000012 | The operation of calling OS Account Service is failed.       |
| 24000013 | The operation of calling Access Token Service is failed.     |
| 24000016 | The cache exceeds the limit.                                 |
| 24000017 | The capability is not supported.                             |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';
import util from '@ohos.util';
import { BusinessError } from '@ohos.base';

function stringToArray(str: string): Uint8Array {
  let textEncoder = new util.TextEncoder();
  return textEncoder.encodeInto(str);
}

let query: asset.AssetMap = new Map();
query.set(asset.Tag.ALIAS, stringToArray('demo_alias'));
try {
    asset.preQuery(query, (error: BusinessError) => {
        if (error) {
            console.error(`Failed to pre-query Asset.`);
        } else {
            console.info(`Succeeded in pre-querying Asset.`);
        }
    });
} catch (error) {
    console.error(`Failed to pre-query Asset.`);
}
```

## asset.preQuery

function preQuery(query: AssetMap): Promise<Uint8Array>

查询的预处理，用于需要用户认证的关键资产。在用户认证成功后，应当随后调用[asset.query](#asset.query)、[asset.postQuery](#asset.postQuery)。使用Promist方式异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名 | 类型     | 必填 | 说明                                                   |
| ------ | -------- | ---- | ------------------------------------------------------ |
| query  | AssetMap | 是   | 关键资产的查询条件，如别名、访问控制属性、自定义数据等。 |

**返回值：**

| 类型                | 说明                                                  |
| ------------------- | ----------------------------------------------------- |
| Promise<Uint8Array> | Promise对象，返回挑战值。<br>注：挑战值用于后续用户认证。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                     |
| -------- | ------------------------------------------------------------ |
| 401      | The argument is invalid.                                     |
| 24000001 | The ASSET Service is unavailable.                            |
| 24000002 | The queried Asset can not be found.                          |
| 24000005 | The screen lock status mismatches.                           |
| 24000006 | Insufficient memory.                                         |
| 24000007 | The Asset is corrupted.                                      |
| 24000008 | The database operation is failed.                            |
| 24000009 | The cryptography operation is failed.                        |
| 24000010 | IPC communication is failed                                  |
| 24000011 | The operation of calling Bundle Manager Service is failed.   |
| 24000012 | The operation of calling OS Account Service is failed.       |
| 24000013 | The operation of calling Access Token Service is failed.     |
| 24000016 | The cache exceeds the limit.                                 |
| 24000017 | The capability is not supported.                             |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';
import util from '@ohos.util';

function stringToArray(str: string): Uint8Array {
  let textEncoder = new util.TextEncoder();
  return textEncoder.encodeInto(str);
}

let query: asset.AssetMap = new Map();
query.set(asset.Tag.ALIAS, stringToArray('demo_alias'));
try {
    asset.preQuery(query).then(() => {
        console.info(`Succeeded in pre-querying Asset.`);
    }).catch (() => {
        console.error(`Failed to pre-query Asset.`);
    });
} catch (error) {
    console.error(`Failed to pre-query Asset.`);
}
```

## asset.query

function query(query: AssetMap, callback: AsyncCallback<Array<AssetMap>>): void

查询一条或多条符合条件的关键资产。若查询需要用户认证的关键资产，则需要在本函数前调用[asset.preQuery](#asset.preQuery)，在本函数后调用[asset.postQuery](#asset.postQuery)，开发步骤请参考[开发指导](../../security/asset-js-guidelines.md#查询需要用户认证的关键资产)。使用Callback回调异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名   | 类型                       | 必填 | 说明                                                         |
| -------- | -------------------------- | ---- | ------------------------------------------------------------ |
| query    | AssetMap                   | 是   | 关键资产的查询条件，如别名、访问控制属性、自定义数据等。       |
| callback | AsyncCallback\<Uint8Array> | 是   | 关键资产查询结果的回调，未捕获到error时，返回查询结果列表，表示查询成功，若捕获到error，则代表关键资产查询失败。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                   |
| -------- | ---------------------------------------------------------- |
| 401      | The argument is invalid.                                   |
| 24000001 | The ASSET Service is unavailable.                          |
| 24000002 | The queried Asset can not be found.                        |
| 24000004 | The access to Asset is denied.                             |
| 24000005 | The screen lock status mismatches.                         |
| 24000006 | Insufficient memory.                                       |
| 24000007 | The Asset is corrupted.                                    |
| 24000008 | The database operation is failed.                          |
| 24000009 | The cryptography operation is failed.                      |
| 24000010 | IPC communication is failed                                |
| 24000011 | The operation of calling Bundle Manager Service is failed. |
| 24000012 | The operation of calling OS Account Service is failed.     |
| 24000013 | The operation of calling Access Token Service is failed.   |
| 24000017 | The capability is not supported.                           |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';
import util from '@ohos.util';
import { BusinessError } from '@ohos.base';

function stringToArray(str: string): Uint8Array {
  let textEncoder = new util.TextEncoder();
  return textEncoder.encodeInto(str);
}

let query: asset.AssetMap = new Map();
query.set(asset.Tag.ALIAS, stringToArray('demo_alias'));
try {
    asset.query(query, (error: BusinessError) => {
        if (error) {
            console.error(`Failed to query Asset.`);
        } else {
            console.info(`Asset query succeeded.`);
        }
    });
} catch (error) {
    console.error(`Failed to query Asset.`);
}
```

## asset.query

function query(query: AssetMap): Promise<Array<AssetMap>>

查询一条或多条符合匹配条件的关键资产。若查询需要用户认证的关键资产，则需要在本函数前调用[asset.preQuery](#asset.preQuery)，在本函数后调用[asset.postQuery](#asset.postQuery)，开发步骤请参考[开发指导](../../security/asset-js-guidelines.md#查询需要用户认证的关键资产)。使用Promise回调异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名   | 类型                            | 必填 | 说明                                                         |
| -------- | ------------------------------- | ---- | ------------------------------------------------------------ |
| query    | AssetMap                        | 是   | 关键资产的查询条件，如别名、访问控制属性、自定义数据等。       |

**返回值：**

| 类型                     | 说明                                  |
| ------------------------ | ------------------------------------- |
| Promise<Array<AssetMap>> | Promise对象，返回查询结果列表。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                   |
| -------- | ---------------------------------------------------------- |
| 401      | The argument is invalid.                                   |
| 24000001 | The ASSET Service is unavailable.                          |
| 24000002 | The queried Asset can not be found.                        |
| 24000004 | The access to Asset is denied.                             |
| 24000005 | The screen lock status mismatches.                         |
| 24000006 | Insufficient memory.                                       |
| 24000007 | The Asset is corrupted.                                    |
| 24000008 | The database operation is failed.                          |
| 24000009 | The cryptography operation is failed.                      |
| 24000010 | IPC communication is failed                                |
| 24000011 | The operation of calling Bundle Manager Service is failed. |
| 24000012 | The operation of calling OS Account Service is failed.     |
| 24000013 | The operation of calling Access Token Service is failed.   |
| 24000017 | The capability is not supported.                           |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';
import util from '@ohos.util';
import { BusinessError } from '@ohos.base';

function stringToArray(str: string): Uint8Array {
  let textEncoder = new util.TextEncoder();
  return textEncoder.encodeInto(str);
}

let query: asset.AssetMap = new Map();
query.set(asset.Tag.ALIAS, stringToArray('demo_alias'));
try {
    asset.query(query).then(() => {
        console.info(`Asset query succeeded.`);
    }).catch (() => {
        console.error(`Failed to query Asset.`);
    });
} catch (error) {
    console.error(`Failed to query Asset.`);
}
```

## asset.postQuery

function postQuery(handle: AssetMap, callback: AsyncCallback<void>): void

查询的后置处理（如：资源释放），用于需要用户认证的关键资产。需与[asset.preQuery](#asset.preQuery)函数成对出现。使用Callback回调异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名   | 类型                 | 必填 | 说明                                                         |
| -------- | -------------------- | ---- | ------------------------------------------------------------ |
| handle   | AssetMap             | 是   | 待处理的查询句柄，当前包含[asset.preQuery](#asset.preQuery)执行成功返回的挑战值。 |
| callback | AsyncCallback\<void> | 是   | 后置处理的回调，未捕获到error代表处理成功，若捕获到error，则代表处理失败。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                   |
| -------- | ---------------------------------------------------------- |
| 401      | The argument is invalid.                                   |
| 24000001 | The ASSET Service is unavailable.                          |
| 24000006 | Insufficient memory.                                       |
| 24000010 | IPC communication is failed                                |
| 24000011 | The operation of calling Bundle Manager Service is failed. |
| 24000012 | The operation of calling OS Account Service is failed.     |
| 24000013 | The operation of calling Access Token Service is failed.   |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';
import { BusinessError } from '@ohos.base';

let handle: asset.AssetMap = new Map();
// 此处传入的new Uint8Array(32)仅作为示例，实际应传入asset.preQuery执行成功返回的挑战值。
handle.set(asset.Tag.AUTH_CHALLENGE, new Uint8Array(32));
try {
    asset.postQuery(handle, (error: BusinessError) => {
        if (error) {
            console.error(`Failed to post-query Asset.`);
        } else {
            console.info(`Succeeded in post-querying Asset.`);
        }
    });
} catch (error) {
    console.error(`Failed to post-query Asset.`);
}
```

## asset.postQuery

function postQuery(handle: AssetMap): Promise<void>

查询的后置处理，用于需要用户认证的关键资产。需与[asset.preQuery](#asset.preQuery)函数成对出现。使用Promise方式异步返回结果。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 参数名 | 类型     | 必填 | 说明                                                         |
| ------ | -------- | ---- | ------------------------------------------------------------ |
| handle | AssetMap | 是   | 待处理的查询句柄，当前包含[asset.preQuery](#asset.preQuery)执行成功返回的挑战值。 |

**返回值：**

| 类型          | 说明                    |
| ------------- | ----------------------- |
| Promise<void> | Promise对象，无返回值。 |

**错误码：**

以下错误码的详细介绍请参见[关键资产存储服务错误码](../errorcodes/errorcode-asset.md)

| 错误码ID | 错误信息                                                   |
| -------- | ---------------------------------------------------------- |
| 401      | The argument is invalid.                                   |
| 24000001 | The ASSET Service is unavailable.                          |
| 24000006 | Insufficient memory.                                       |
| 24000010 | IPC communication is failed                                |
| 24000011 | The operation of calling Bundle Manager Service is failed. |
| 24000012 | The operation of calling OS Account Service is failed.     |
| 24000013 | The operation of calling Access Token Service is failed.   |

**示例代码：**

```typescript
import asset from '@ohos.security.asset';

let handle: asset.AssetMap = new Map();
// 此处传入的new Uint8Array(32)仅作为示例，实际应传入asset.preQuery执行成功返回的安全随机数
handle.set(asset.Tag.AUTH_CHALLENGE, new Uint8Array(32));
try {
    asset.postQuery(handle).then(() => {
        console.info(`Succeeded in post-querying Asset.`);
    }).catch (() => {
        console.error(`Failed to post-query Asset.`);
    });
} catch (error) {
    console.error(`Failed to post-query Asset.`);
}
```

## asset.Tag

关键资产属性的名称，用作[asset.AssetMap](#asset.AssetMap)的键。

**系统能力：** SystemCapability.Security.Asset

## asset.Value

type Value = boolean | number | Uint8Array;

关键资产属性的内容，用作[asset.AssetMap](#asset.AssetMap)的值。

**系统能力：** SystemCapability.Security.Asset

## asset.AssetMap

type AssetMap = Map<Tag, Value>

关键资产属性的键-值对集合。

**系统能力：** SystemCapability.Security.Asset

**参数：**

| 属性名称（类型：asset.Tag） | 属性内容（类型：asset.Value）                                  | 说明                                                         |
| ------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| SECRET                    | 类型为Uint8Array，长度为1-1024字节                           | 关键资产明文                                                 |
| ALIAS                     | 类型为Uint8Array，长度为1-256字节                            | 关键资产别名，每条关键资产的唯一索引                         |
| ACCESSIBILITY             | 类型为number，取值范围详见[asset.Accessibility](#asset.Accessibility) | 基于锁屏状态的访问控制                                       |
| REQUIRE_PASSWORD_SET      | 类型为bool                                                   | 是否仅在设置了锁屏密码的情况下，可访问关键资产                 |
| AUTH_TYPE                 | 类型为number，取值范围详见[asset.AuthType](#asset.AuthType)  | 访问关键资产所需的用户认证类型                               |
| AUTH_VALIDITY_PERIOD      | 类型为number，取值范围：1-600，单位为秒                      | 用户认证的有效期                                             |
| AUTH_CHALLENGE            | 类型为Uint8Array，长度为32字节                               | 用户认证的挑战值                                         |
| AUTH_TOKEN                | 类型为Uint8Array，长度为148字节                              | 用户认证通过的授权令牌                                           |
| SYNC_TYPE                 | 类型为number，取值范围详见[asset.SyncType](#asset.SyncType)  | 关键资产支持的同步类型                                       |
| IS_PERSISTENT             | 类型为bool                                                   | 在应用卸载时是否需要保留关键资产<br>**需要权限：**ohos.permission.STORE_PERSISTENT_DATA<br/>**备注：**仅在调用[asset.add](#asset.add)函数并传入该属性时需要校验权限 |
| DATA_LABEL_CRITICAL_1<br>DATA_LABEL_CRITICAL_2<br>DATA_LABEL_CRITICAL_3<br>DATA_LABEL_CRITICAL_4     | 类型为Uint8Array，长度为1-512字节                            | 关键资产附属信息，内容由业务自定义且**有完整性保护**             |
| DATA_LABEL_NORMAL_1<br>DATA_LABEL_NORMAL_2<br>DATA_LABEL_NORMAL_3<br>DATA_LABEL_NORMAL_4       | 类型为Uint8Array，长度为1-512字节                            | 关键资产附属信息，内容由业务自定义且**无完整性保护**             |
| RETURN_TYPE               | 类型为number，取值范围详见[asset.ReturnType](#asset.ReturnType) | 关键资产查询返回的结果类型                                         |
| RETURN_LIMIT              | 类型为number                                                 | 关键资产查询返回的结果数量                                         |
| RETURN_OFFSET             | 类型为number，取值范围：1-65536                              | 关键资产查询返回的结果偏移量<br>注：用于分批查询场景，指定从第几个开始返回                                 |
| RETURN_ORDERED_BY         | 类型为number，取值范围：asset.Tag.DATA_LABEL_xxx             | 关键资产查询返回的结果排序依据，仅支持按照附属信息排序，不指定的情况下，默认按照关键资产新增的顺序返回。 |
| CONFLICT_RESOLUTION       | 类型为number，取值范围详见[asset.ConflictResolution](#asset.ConflictResolution) | 新增同别名的关键资产时的处理策略                             |

## asset.Accessibility

关键资产基于锁屏状态的访问控制。

**系统能力：** SystemCapability.Security.Asset

| 名称                  | 值   | 说明                                                         |
| --------------------- | ---- | ------------------------------------------------------------ |
| DEVICE_POWER_ON       | 0    | 开机后可访问                                   |
| DEVICE_FIRST_UNLOCKED | 1    | 首次解锁后可访问<br>**备注：**未设置锁屏密码时，等同于开机后可访问 |
| DEVICE_UNLOCKED       | 2    | 解锁状态时可访问<br/>**备注：**未设置锁屏密码时，等同于开机后可访问 |

## asset.AuthType

关键资产支持的用户认证类型。

**系统能力：** SystemCapability.Security.Asset

| 名称 | 值   | 说明                                                         |
| ---- | ---- | ------------------------------------------------------------ |
| NONE | 0    | 访问关键资产前无需用户认证                                   |
| ANY  | 255  | 任意一种用户认证方式（PIN码、人脸、指纹等）通过后，均可访问关键资产 |

## asset.SyncType

关键资产支持的同步类型。本字段属于能力预埋，当前不支持同步。

**系统能力：** SystemCapability.Security.Asset

| 名称           | 值   | 说明                                             |
| -------------- | ---- | ------------------------------------------------ |
| NEVER          | 0    | 不允许同步关键资产                               |
| THIS_DEVICE    | 1    | 只在本设备进行同步，如仅在本设备还原的备份场景。    |
| TRUSTED_DEVICE | 2    | 只在可信设备间进行同步，如克隆场景。               |

## asset.ReturnType

关键资产查询返回的结果类型。

**系统能力：** SystemCapability.Security.Asset

| 名称       | 值   | 说明                                                         |
| ---------- | ---- | ------------------------------------------------------------ |
| ALL        | 0    | 返回关键资产明文及属性<br/>**备注：**查询单条关键资产明文时，需设置此类型 |
| ATTRIBUTES | 1    | 返回关键资产属性，不含关键资产明文<br>**备注：**批量查询关键资产属性时，需设置此类型 |

## asset.ConflictResolution

新增关键资产时的冲突（如：别名相同）处理策略。

**系统能力：** SystemCapability.Security.Asset

| 名称        | 值   | 说明                         |
| ----------- | ---- | ---------------------------- |
| OVERWRITE   | 0    | 覆盖原有的关键资产     |
| THROW_ERROR | 1    | 抛出异常，由业务进行后续处理 |