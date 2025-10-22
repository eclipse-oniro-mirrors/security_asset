/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { BusinessError } from '@ohos.base';

export class SyncResultInner {
  resultCode: int = 0;
  totalCount: int = 0;
  failedCount: int = 0;
}

native function AniAdd(attributes: asset.AssetMap): BusinessError<void>;

native function AniAddAsUser(userId: int, attributes: asset.AssetMap): BusinessError<void>;

native function AniRemove(query: asset.AssetMap): BusinessError<void>;

native function AniRemoveAsUser(userId: int, query: asset.AssetMap): BusinessError<void>;

native function AniUpdate(query: asset.AssetMap, attributesToUpdate: asset.AssetMap): BusinessError<void>;

native function AniUpdateAsUser(userId: int, query: asset.AssetMap, attributesToUpdate: asset.AssetMap): BusinessError<void>;

native function AniPreQuery(query: asset.AssetMap): BusinessError<Uint8Array>;

native function AniPreQueryAsUser(userId: int, query: asset.AssetMap): BusinessError<Uint8Array>;

native function AniQuery(query: asset.AssetMap): BusinessError<Array<asset.AssetMap>>;

native function AniQueryAsUser(userId: int, query: asset.AssetMap): BusinessError<Array<asset.AssetMap>>;

native function AniPostQuery(handle: asset.AssetMap): BusinessError<void>;

native function AniPostQueryAsUser(userId: int, handle: asset.AssetMap): BusinessError<void>;

native function AniQuerySyncResult(query: asset.AssetMap): BusinessError<asset.SyncResult>;

export namespace asset {
  loadLibrary("asset_ani.z")

  function HandleAssetSyncResult<T>(ret: BusinessError<T>): T {
    if (ret.code !== 0) {
      throw ret;
    }
    return ret.data as T;
  }

  function HandleAssetAsyncResult<T>(
    ret: BusinessError<T>,
    resolve: (v: T) => void,
    reject: (error: BusinessError<T>) => void
  ): void {
    if (ret.code === 0) {
      resolve(ret.data as T);
    } else {
      reject(ret);
    }
  }

  export function add(attributes: AssetMap): Promise<void> {
    return new Promise<void> ((resolve, reject): void => {
      let task = taskpool.execute(AniAdd, attributes);
      task.then((ret: NullishType) => {
        HandleAssetAsyncResult<void>(ret as BusinessError<void>, resolve, reject);
      });
    });
  }

  export function addAsUser(userId: int, attributes: AssetMap): Promise<void> {
    return new Promise<void> ((resolve, reject): void => {
      let task = taskpool.execute(AniAddAsUser, userId, attributes);
      task.then((ret: NullishType) => {
        HandleAssetAsyncResult<void>(ret as BusinessError<void>, resolve, reject);
      });
    });
  }

  export function addSync(attributes: AssetMap): void {
    let ret = AniAdd(attributes);
    return HandleAssetSyncResult<void>(ret);
  }

  export function remove(query: AssetMap): Promise<void> {
    return new Promise<void> ((resolve, reject): void => {
      let task = taskpool.execute(AniRemove, query);
      task.then((ret: NullishType) => {
        HandleAssetAsyncResult<void>(ret as BusinessError<void>, resolve, reject);
      });
    });
  }

  export function removeAsUser(userId: int, query: AssetMap): Promise<void> {
    return new Promise<void> ((resolve, reject): void => {
      let task = taskpool.execute(AniRemoveAsUser, userId, query);
      task.then((ret: NullishType) => {
        HandleAssetAsyncResult<void>(ret as BusinessError<void>, resolve, reject);
      });
    });
  }

  export function removeSync(query: AssetMap): void {
    let ret = AniRemove(query);
    return HandleAssetSyncResult<void>(ret);
  }

  export function update(query: AssetMap, attributesToUpdate: AssetMap): Promise<void> {
    return new Promise<void> ((resolve, reject): void => {
      let task = taskpool.execute(AniUpdate, query, attributesToUpdate);
      task.then((ret: NullishType) => {
        HandleAssetAsyncResult<void>(ret as BusinessError<void>, resolve, reject);
      });
    });
  }

  export function updateAsUser(userId: int, query: AssetMap, attributesToUpdate: AssetMap): Promise<void> {
    return new Promise<void> ((resolve, reject): void => {
      taskpool.execute(AniUpdateAsUser, userId, query, attributesToUpdate)
        .then((ret: NullishType) => {
          HandleAssetAsyncResult<void>(ret as BusinessError<void>, resolve, reject);
        });
    });
  }

  export function updateSync(query: AssetMap, attributesToUpdate: AssetMap): void {
    let ret = AniUpdate(query, attributesToUpdate);
    return HandleAssetSyncResult<void>(ret);
  }

  export function preQuery(query: AssetMap): Promise<Uint8Array> {
    return new Promise<Uint8Array> ((resolve, reject): void => {
      let task = taskpool.execute(AniPreQuery, query);
      task.then((ret: NullishType) => {
        HandleAssetAsyncResult<Uint8Array>(ret as BusinessError<Uint8Array>, resolve, reject);
      });
    });
  }

  export function preQueryAsUser(userId: int, query: AssetMap): Promise<Uint8Array> {
    return new Promise<Uint8Array> ((resolve, reject): void => {
      let task = taskpool.execute(AniPreQueryAsUser, userId, query);
      task.then((ret: NullishType) => {
        HandleAssetAsyncResult<Uint8Array>(ret as BusinessError<Uint8Array>, resolve, reject);
      });
    });
  }

  export function preQuerySync(query: AssetMap): Uint8Array {
    let ret = AniPreQuery(query);
    return HandleAssetSyncResult<Uint8Array>(ret);
  }

  export function query(query: AssetMap): Promise<Array<AssetMap>> {
    return new Promise<Array<AssetMap>> ((resolve, reject): void => {
      let task = taskpool.execute(AniQuery, query);
      task.then((ret: NullishType) => {
        HandleAssetAsyncResult<Array<AssetMap>>(ret as BusinessError<Array<AssetMap>>, resolve, reject);
      });
    });
  }

  export function queryAsUser(userId: int, query: AssetMap): Promise<Array<AssetMap>> {
    return new Promise<Array<AssetMap>> ((resolve, reject): void => {
      let task = taskpool.execute(AniQueryAsUser, userId, query);
      task.then((ret: NullishType) => {
        HandleAssetAsyncResult<Array<AssetMap>>(ret as BusinessError<Array<AssetMap>>, resolve, reject);
      });
    });
  }

  export function querySync(query: AssetMap): Array<AssetMap> {
    let ret = AniQuery(query);
    return HandleAssetSyncResult<Array<AssetMap>>(ret);
  }

  export function postQuery(handle: AssetMap): Promise<void> {
    return new Promise<void> ((resolve, reject): void => {
      let task = taskpool.execute(AniPostQuery, handle);
      task.then((ret: NullishType) => {
        HandleAssetAsyncResult<void>(ret as BusinessError<void>, resolve, reject);
      });
    });
  }

  export function postQueryAsUser(userId: int, handle: AssetMap): Promise<void> {
    return new Promise<void> ((resolve, reject): void => {
      let task = taskpool.execute(AniPostQueryAsUser, userId, handle);
      task.then((ret: NullishType) => {
        HandleAssetAsyncResult<void>(ret as BusinessError<void>, resolve, reject);
      });
    });
  }

  export function postQuerySync(handle: AssetMap): void {
    let ret = AniPostQuery(handle);
    return HandleAssetSyncResult(ret);
  }

  export function querySyncResult(query: AssetMap): Promise<SyncResult> {
    return new Promise<SyncResult> ((resolve, reject): void => {
      let task = taskpool.execute(AniQuerySyncResult, query);
      task.then((ret: NullishType) => {
        let result = ret as BusinessError<SyncResultInner>;
        if (result.code === 0) {
          let syncResultInner: SyncResultInner = result.data as SyncResultInner;
          let syncResult: SyncResult = {
            resultCode: syncResultInner.resultCode,
            totalCount: syncResultInner.totalCount,
            failedCount: syncResultInner.failedCount
          };
          resolve(syncResult);
        } else {
          reject(result);
        }
      });
    });
  }

  /**
   * A Map type containing tag-value pairs that describe the attributes of an Asset.
   *
   * @typedef { Map<Tag, Value> }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  /**
   * A Map type containing tag-value pairs that describe the attributes of an Asset.
   *
   * @typedef { Map<Tag, Value> }
   * @syscap SystemCapability.Security.Asset
   * @atomicservice
   * @since 14
   */
  export type AssetMap = Map<Tag, Value>;

  /**
   * A type that indicates the secret or attribute value of an Asset tag.
   *
   * @typedef { boolean | number | Uint8Array }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  /**
   * Represents the value of each attribute in {@link AssetMap}.
   *
   * @typedef { boolean | number | Uint8Array }
   * @syscap SystemCapability.Security.Asset
   * @atomicservice
   * @since 14
   */
  export type Value = boolean | int | Uint8Array;

  /**
   * An enum type indicates when the Asset is accessible.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  /**
   * Enumerates the types of access control based on the lock screen status.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @atomicservice
   * @since 14
   */
  export enum Accessibility {
    /**
     * The secret value in the Asset can only be accessed after the device is powered on.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The asset can be accessed after the device is powered on.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DEVICE_POWERED_ON = 0,
    /**
     * The secret value in the Asset can only be accessed after the device is first unlocked.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The asset can be accessed only after the device is unlocked for the first time.
     * <p><strong>NOTE</strong>:
     * If no lock screen password is set, this option is equivalent to <strong>DEVICE_POWERED_ON</strong>.
     * </p>
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DEVICE_FIRST_UNLOCKED = 1,
    /**
     * The secret value in the Asset can only be accessed while the device is unlocked.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The asset can be accessed only when the device is unlocked.
     * <p><strong>NOTE</strong>:
     * If no lock screen password is set, this option is equivalent to <strong>DEVICE_POWERED_ON</strong>.
     * </p>
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DEVICE_UNLOCKED = 2,
  }

  /**
   * An enum type indicates the user authentication type for Asset access control.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  /**
   * Enumerates the types of user authentication supported by an asset.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @atomicservice
   * @since 14
   */
  export enum AuthType {
    /**
     * The access to an Asset doesn't require user authentication.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * No user authentication is required before the asset is accessed.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    NONE = 0x00,
    /**
     * The access to an Asset requires user authentication using either PIN/pattern/password or biometric traits.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The asset can be accessed if any user authentication (such as PIN, facial, or fingerprint authentication)
     * is successful.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    ANY = 0xFF,
  }

  /**
   * An enum type indicates the type of Asset synchronization.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  /**
   * Enumerates the sync types supported by an asset.
   * <p><strong>NOTE</strong>:
   * This field is an embedded parameter. Currently, asset sync is not supported.
   * </p>
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @atomicservice
   * @since 14
   */
  export enum SyncType {
    /**
     * An Asset with this attribute value is never allowed to be transferred out.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Asset sync is not allowed.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    NEVER = 0,
    /**
     * An Asset with this attribute value can only be restored to the device from which it was transferred out.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Asset sync is allowed only on the local device, for example, in data restore on the local device.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    THIS_DEVICE = 1 << 0,
    /**
     * An Asset with this attribute value can only be transferred out to a trusted device (user authorized).
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Asset sync is allowed only between trusted devices, for example, in the case of cloning.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    TRUSTED_DEVICE = 1 << 1,
    /**
     * An Asset with this attribute value can only be transferred out to devices logged in with trusted accounts.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 12
     */
    /**
     * Asset sync is allowed only between the devices that are logged in with trusted accounts, for example,
     * in cloud sync scenarios.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    TRUSTED_ACCOUNT = 1 << 2,
  }

  /**
   * An enum type indicates the type of Asset encapsulation.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 18
   */
  export enum WrapType {
    /**
     * An Asset with this attribute value is never allowed to be wrapped up.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 18
     */
    NEVER = 0,
    /**
     * An Asset with this attribute value can only be wrapped or unwrapped on devices logged in with trusted accounts.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 18
     */
    TRUSTED_ACCOUNT = 1,
  }

  /**
   * An enum type indicates the strategy for conflict resolution when handling duplicated Asset alias.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  /**
   * Enumerates the policies for resolving conflicts (for example, a duplicate alias) when an asset is added.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @atomicservice
   * @since 14
   */
  export enum ConflictResolution {
    /**
     * Directly overwrite an Asset with duplicated alias when a conflict is detected.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Overwrite the original asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    OVERWRITE = 0,
    /**
     * Throw an error so that the caller can take measures when a conflict is detected.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Throw an exception for the service to perform subsequent processing.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    THROW_ERROR = 1,
  }

  /**
   * An enum type indicates the return type of the queried Asset.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  /**
   * Enumerates the type of information returned by an asset query operation.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @atomicservice
   * @since 14
   */

  /**
   * Enumerates the types of additional operation to perform.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 12
   */
  export enum OperationType {
    /**
     * Sync.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 12
     */
    NEED_SYNC = 0,
    /**
     * Logout.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 12
     */
    NEED_LOGOUT = 1,
  }

  /**
   * Interface of synchronization result.
   *
   * @typedef SyncResult
   * @syscap SystemCapability.Security.Asset
   * @since 20
   */
  export interface SyncResult {
    /**
     * The result code of synchronization.
     *
     * @type { int }
     * @readonly
     * @syscap SystemCapability.Security.Asset
     * @since 20
     */
    readonly resultCode: int;
    /**
     * The total count of synchronized Assets.
     *
     * @type { ?int }
     * @readonly
     * @syscap SystemCapability.Security.Asset
     * @since 20
     */
    readonly totalCount?: int;
    /**
     * The count of Assets that fail to synchronize.
     *
     * @type { ?int }
     * @readonly
     * @syscap SystemCapability.Security.Asset
     * @since 20
     */
    readonly failedCount?: int;
  }

  /**
   * An enum type containing the data type definitions for Asset attribute value.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  /**
   * Enumerates the asset attribute types.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @atomicservice
   * @since 14
   */
  export enum TagType {
    /**
     * The data type of Asset attribute value is bool.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Boolean.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    BOOL = 0x01 << 28,
    /**
     * The data type of Asset attribute value is uint32.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Number.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    NUMBER = 0x02 << 28,
    /**
     * The data type of Asset attribute value is byte array.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Byte array.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    BYTES = 0x03 << 28,
  }

  /**
   * An enum type containing the Asset attribute tags.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  /**
   * Enumerate the keys of asset attributes ({@link AssetMap}), which are in key-value (KV) pairs.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @atomicservice
   * @since 14
   */
  export enum Tag {
    /**
     * A tag whose value is a byte array indicating the sensitive user data such as passwords and tokens.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Asset plaintext.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    SECRET = TagType.BYTES | 0x01,
    /**
     * A tag whose value is a byte array identifying an Asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Asset alias, which uniquely identifies an asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    ALIAS = TagType.BYTES | 0x02,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating when the Asset can be accessed.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Access control based on the lock screen status.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    ACCESSIBILITY = TagType.NUMBER | 0x03,
    /**
     * A tag whose value is a bool indicating whether a screen lock password is required for the device.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Whether the asset is accessible only when a lock screen password is set.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    REQUIRE_PASSWORD_SET = TagType.BOOL | 0x04,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the user authentication type for Asset access control.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Type of user authentication required for accessing the asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    AUTH_TYPE = TagType.NUMBER | 0x05,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the validity period in seconds of user authentication.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Validity period of the user authentication.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    AUTH_VALIDITY_PERIOD = TagType.NUMBER | 0x06,
    /**
     * A tag whose value is a byte array indicating the authentication challenge for anti-replay protection.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Challenge for the user authentication.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    AUTH_CHALLENGE = TagType.BYTES | 0x07,
    /**
     * A tag whose value is a byte array indicating the authentication token after a user is verified.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Authorization token obtained after the user authentication is successful.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    AUTH_TOKEN = TagType.BYTES | 0x08,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the type of Asset synchronization.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Asset sync type.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    SYNC_TYPE = TagType.NUMBER | 0x10,
    /**
     * Whether to retain the asset when the application is uninstalled.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    IS_PERSISTENT = TagType.BOOL | 0x11,
    /**
     * A tag whose value is a byte array indicating the first user-defined Asset data label (not allow to update).
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Additional asset data customized by the service with integrity protection.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_CRITICAL_1 = TagType.BYTES | 0x20,
    /**
     * A tag whose value is a byte array indicating the second user-defined Asset data label (not allow to update).
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Additional asset data customized by the service with integrity protection.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_CRITICAL_2 = TagType.BYTES | 0x21,
    /**
     * A tag whose value is a byte array indicating the third user-defined Asset data label (not allow to update).
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Additional asset data customized by the service with integrity protection.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_CRITICAL_3 = TagType.BYTES | 0x22,
    /**
     * A tag whose value is a byte array indicating the fourth user-defined Asset data label (not allow to update).
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Additional asset data customized by the service with integrity protection.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_CRITICAL_4 = TagType.BYTES | 0x23,
    /**
     * A tag whose value is a byte array indicating the first user-defined Asset data label (allow to update).
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Additional asset data customized by the service without integrity protection.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_NORMAL_1 = TagType.BYTES | 0x30,
    /**
     * A tag whose value is a byte array indicating the second user-defined Asset data label (allow to update).
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Additional asset data customized by the service without integrity protection.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_NORMAL_2 = TagType.BYTES | 0x31,
    /**
     * A tag whose value is a byte array indicating the third user-defined Asset data label (allow to update).
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Additional asset data customized by the service without integrity protection.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_NORMAL_3 = TagType.BYTES | 0x32,
    /**
     * A tag whose value is a byte array indicating the fourth user-defined Asset data label (allow to update).
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Additional asset data customized by the service without integrity protection.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_NORMAL_4 = TagType.BYTES | 0x33,
    /**
     * A local tag whose value is a byte array indicating the first user-defined Asset data label (allow to update).
     * The information of a local tag will not be synchronized.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 12
     */
    /**
     * Local information about the asset. The value is assigned by the service without integrity protection and
     * will not be synced.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_NORMAL_LOCAL_1 = TagType.BYTES | 0x34,
    /**
     * A local tag whose value is a byte array indicating the second user-defined Asset data label (allow to update).
     * The information of a local tag will not be synchronized.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 12
     */
    /**
     * Local information about the asset. The value is assigned by the service without integrity protection and
     * will not be synced.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_NORMAL_LOCAL_2 = TagType.BYTES | 0x35,
    /**
     * A local tag whose value is a byte array indicating the third user-defined Asset data label (allow to update).
     * The information of a local tag will not be synchronized.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 12
     */
    /**
     * Local information about the asset. The value is assigned by the service without integrity protection and
     * will not be synced.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_NORMAL_LOCAL_3 = TagType.BYTES | 0x36,
    /**
     * A local tag whose value is a byte array indicating the fourth user-defined Asset data label (allow to update).
     * The information of a local tag will not be synchronized.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 12
     */
    /**
     * Local information about the asset. The value is assigned by the service without integrity protection and
     * will not be synced.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_LABEL_NORMAL_LOCAL_4 = TagType.BYTES | 0x37,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the return type of the queried Asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Type of the asset query result to return.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    RETURN_TYPE = TagType.NUMBER | 0x40,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the maximum number of returned Assets in one query.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Maximum number of asset records to return.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    RETURN_LIMIT = TagType.NUMBER | 0x41,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the offset of return data in batch query.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Offset of the asset query result.
     * <p><strong>NOTE</strong>:
     * This parameter specifies the starting asset record to return in batch asset query.
     * </p>
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    RETURN_OFFSET = TagType.NUMBER | 0x42,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating how the query results are sorted.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Sorting order of the query results. Currently, the results can be sorted only by
     * <strong>ASSET_TAG_DATA_LABEL</strong>.
     * <p><strong>NOTE</strong>:
     * By default, assets are returned in the order in which they are added.
     * </p>
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    RETURN_ORDERED_BY = TagType.NUMBER | 0x43,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the strategy for resolving Asset conflicts.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * Policy for resolving the conflict (for example, a duplicate alias).
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    CONFLICT_RESOLUTION = TagType.NUMBER | 0x44,
    /**
     * A tag whose value is a byte array indicating the update time of an Asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 12
     */
    /**
     * Data update time, in timestamp.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    UPDATE_TIME = TagType.BYTES | 0x45,
    /**
     * Additional operation type.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 12
     */
    OPERATION_TYPE = TagType.NUMBER | 0x46,
    /**
     * Whether to encrypt the additional asset information customized by the service.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    REQUIRE_ATTR_ENCRYPTED = TagType.BOOL | 0x47,
    /**
     * Group to which the asset belongs.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 18
     */
    GROUP_ID = TagType.BYTES | 0x48,
    /**
     * A tag whose value is a 32-bit unsigned integer indicating the type of Asset encapsulation.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 18
     */
    WRAP_TYPE = TagType.NUMBER | 0x49,
  }

  /**
   * An enum type containing the Asset error codes.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  /**
   * Enumerates the error codes.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @atomicservice
   * @since 14
   */
  export enum ErrorCode {
    /**
     * The error code indicates that the caller doesn't have the permission.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    PERMISSION_DENIED = 201,
    /**
     * The error code indicates that the caller is not a system application.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 12
     */
    NOT_SYSTEM_APPLICATION = 202,
    /**
     * The error code indicates that the argument is invalid.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the argument is invalid.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    INVALID_ARGUMENT = 401,
    /**
     * The error code indicates that the ASSET service is unavailable.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the ASSET service is unavailable.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    SERVICE_UNAVAILABLE = 24000001,
    /**
     * The error code indicates that the asset is not found.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the asset is not found.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    NOT_FOUND = 24000002,
    /**
     * The error code indicates that the asset already exists.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the asset already exists.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DUPLICATED = 24000003,
    /**
     * The error code indicates that access to the asset is denied.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that access to the asset is denied.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    ACCESS_DENIED = 24000004,
    /**
     * The error code indicates that the screen lock status does not match.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the screen lock status does not match.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    STATUS_MISMATCH = 24000005,
    /**
     * The error code indicates insufficient memory.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates insufficient memory.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    OUT_OF_MEMORY = 24000006,
    /**
     * The error code indicates that the asset is corrupted.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the asset is corrupted.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATA_CORRUPTED = 24000007,
    /**
     * The error code indicates that the database operation failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the database operation failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    DATABASE_ERROR = 24000008,
    /**
     * The error code indicates that the cryptography operation failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the cryptography operation failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    CRYPTO_ERROR = 24000009,
    /**
     * The error code indicates that the ipc failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the ipc failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    IPC_ERROR = 24000010,
    /**
     * The error code indicates that calling the Bundle Manager service failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that calling the Bundle Manager service failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    BMS_ERROR = 24000011,
    /**
     * The error code indicates that calling the OS Account service failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that calling the OS Account service failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    ACCOUNT_ERROR = 24000012,
    /**
     * The error code indicates that calling the Access Token service failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that calling the Access Token service failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    ACCESS_TOKEN_ERROR = 24000013,
    /**
     * The error code indicates that the file operation failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the file operation failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    FILE_OPERATION_ERROR = 24000014,
    /**
     * The error code indicates that getting the system time failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that getting the system time failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    GET_SYSTEM_TIME_ERROR = 24000015,
    /**
     * The error code indicates that the cache exceeds the limit.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the cache exceeds the limit.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    LIMIT_EXCEEDED = 24000016,
    /**
     * The error code indicates that the capability is not supported.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    /**
     * The error code indicates that the capability is not supported.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 14
     */
    UNSUPPORTED = 24000017,
    /**
     * The error code indicates that verifying the parameter failed.
     *
     * @syscap SystemCapability.Security.Asset
     * @atomicservice
     * @since 20
     */
    PARAM_VERIFICATION_FAILED = 24000018,
  }
}

export default asset;