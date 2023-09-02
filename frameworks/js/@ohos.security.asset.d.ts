/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

import type { AsyncCallback } from './@ohos.base';

/**
 * This module provides the lifecycle management capability of assets (such as passwords and tokens),
 * including adding, removing, updating, and querying.
 *
 * @namespace asset
 * @syscap SystemCapability.Security.Asset
 * @since 11
 */
declare namespace asset {
  /**
   * Add an asset.
   *
   * @param { AssetMap } attributes - a map of attributes that describes the asset to add.
   * @param { AsyncCallback<void> } callback - the callback of add.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function add(attributes: AssetMap, callback: AsyncCallback<void>): void;

  /**
   * Add an asset.
   *
   * @param { AssetMap } attributes - a map of attributes that describes the asset to add.
   * @returns { Promise<void> } the promise returned by the function.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function add(attributes: AssetMap): Promise<void>;

  /**
   * Remove one or multiple assets that match a search query.
   *
   * @param { AssetMap } query - a map of attributes that describes the search for asset to remove.
   * @param { AsyncCallback<void> } callback - the callback of remove.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function remove(query: AssetMap, callback: AsyncCallback<void>): void;

  /**
   * Remove one or multiple assets that match a search query.
   *
   * @param { AssetMap } query - a map of attributes that describes the search for asset to remove.
   * @returns { Promise<void> } the promise returned by the function.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function remove(query: AssetMap): Promise<void>;

  /**
   * Update an asset that matches a search query.
   *
   * @param { AssetMap } query - a map of attributes that describes the search for asset to update.
   * @param { AssetMap } attributesToUpdate - a map of attributes to be updated with the new values.
   * @param { AsyncCallback<void> } callback - the callback of update.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function update(query: AssetMap, attributesToUpdate: AssetMap, callback: AsyncCallback<void>): void;

  /**
   * Update an asset that matches a search query.
   *
   * @param { AssetMap } query - a map of attributes that describes the search for asset to update.
   * @param { AssetMap } attributesToUpdate - a map of attributes to be updated with the new values.
   * @returns { Promise<void> } the promise returned by the function.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function update(query: AssetMap, attributesToUpdate: AssetMap): Promise<void>;

  /**
   * Prepare for querying one or multiple assets that require user authentication.
   *
   * @param { AssetMap } query - a map of attributes that describes the search for asset to query.
   * @param { AsyncCallback<Uint8Array> } callback - the callback of preQuery.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function preQuery(query: AssetMap, callback: AsyncCallback<Uint8Array>): void;

  /**
   * Prepare for querying one or multiple assets that require user authentication.
   *
   * @param { AssetMap } query - a map of attributes that describes the search for asset to query.
   * @returns { Promise<Uint8Array> } the promise returned by the function.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function preQuery(query: AssetMap): Promise<Uint8Array>;

  /**
   * Query one or multiple assets that match a search query.
   *
   * @param { AssetMap } query - a map of attributes that describes the search for asset to query.
   * @param { AsyncCallback<Array<AssetMap>> } callback - the callback of query containing query results.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function query(query: AssetMap, callback: AsyncCallback<Array<AssetMap>>): void;

  /**
   * Query one or multiple assets that match a search query.
   *
   * @param { AssetMap } query - a map of attributes that describes the search for asset to query.
   * @returns { Promise<Array<AssetMap>> } the promise returned by the function.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function query(query: AssetMap): Promise<Array<AssetMap>>;

  /**
   * Release the resource after querying multiple assets that require user authentication.
   *
   * @param { AssetMap } handle - a map contains the handle returned by {@link preQuery}.
   * @param { AsyncCallback<void> } callback - the callback of postQuery.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function postQuery(handle: AssetMap, callback: AsyncCallback<void>): void;

  /**
   * Release the resource after querying multiple assets that require user authentication.
   *
   * @param { AssetMap } handle - a map contains the handle returned by {@link preQuery}.
   * @returns { Promise<void> } the promise returned by the function.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function postQuery(handle: AssetMap): Promise<void>;

  /**
   * Get the version of {@link asset}.
   *
   * @returns { VersionInfo } the version info.
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  function getVersion(): Version;

  /**
   * The asset version.
   *
   * @typedef Version
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  export interface Version {
    /**
     * The major version.
     *
     * @type {number}
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    major: number,
    /**
     * The minor version.
     *
     * @type {number}
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    minor: number,
    /**
     * The patch version.
     *
     * @type {number}
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    patch: number,
  }

   /**
   * A type consisting of tag-value pairs that indicate asset attributes.
   *
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  export type AssetMap = Map<Tag, Value>;

  /**
   * A type that indicates the value of the asset attribute.
   *
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  export type Value = boolean | number | Uint8Array;

  export enum Accessibility {
    DEVICE_POWER_ON = 0,
    DEVICE_FIRST_UNLOCK = 1,
    DEVICE_UNLOCK = 2,
    DEVICE_SECURE = 3,
  }

  export enum AuthType {
    NONE = 0x00,
    ANY = 0xFF,
  }

  export enum SyncType {
    NEVER = 0,
    THIS_DEVICE = 1 << 0,
    TRUSTED_ACCOUNT = 1 << 1,
    TRUSTED_DEVICE = 1 << 2,
  }

  export enum ConflictPolicy {
    OVERRIDE = 0,
    REPORT = 1,
  }

  export enum ReturnType {
    ALL = 0,
    ATTRIBUTES = 1,
  }

  /**
   * An enum type that indicates the type of the asset attribute value.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  enum TagType {
    /**
     * The type of the asset attribute value is int32.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    INT32 = 1 << 28,
    /**
     * The type of the asset attribute value is uint32.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    UINT32 = 2 << 28,
    /**
     * The type of the asset attribute value is int64.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    INT64 = 3 << 28,
    /**
     * The type of the asset attribute value is uint64.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    UINT64 = 4 << 28,
    /**
     * The type of the asset attribute value is bool.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    BOOL = 5 << 28,
    /**
     * The type of the asset attribute value is byte array.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    BYTES = 6 << 28
  }

  /**
   * An emum type that indicates the tag of the asset attribute.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 11
   */
  export enum Tag {
    /**
     * A tag whose value is the asset, such as password and token.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    SECRET = TagType.BYTES | 1,
    /**
     * A tag whose value used to identify an asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    ALIAS = TagType.BYTES | 2,
    /**
     * A tag whose value indicates when the asset can be accessed.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    ACCESSIBILITY = TagType.UINT32 | 3,
    /**
     * A tag whose value indicates what type of user authentication is required.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    AUTH_TYPE = TagType.UINT32 | 4,
    /**
     * A tag whose value indicates the validity period of user authentication, in seconds.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    AUTH_VALIDITY_PERIOD = TagType.UINT32 | 5,
    /**
     * A tag whose value indicates the authentication challenge for anti-replay.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    AUTH_CHALLENGE = TagType.BYTES | 6,
    /**
     * A tag whose value indicates the credential after successful authentication of the user.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    AUTH_TOKEN = TagType.BYTES | 7,
    /**
     * A tag whose value indicates the type of asset synchronization.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    SYNC_TYPE = TagType.UINT32 | 8,
    /**
     * A tag whose value indicates the conflict handling policy for adding the asset with the same alias.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    CONFLICT_POLICY = TagType.UINT32 | 9,
    /**
     * A tag whose value indicates the first customized critical data of the asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    DATA_LABLE_CRITICAL_1 = TagType.BYTES | 10,
    /**
     * A tag whose value indicates the second customized critical data of the asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    DATA_LABLE_CRITICAL_2 = TagType.BYTES | 11,
    /**
     * A tag whose value indicates the third customized critical data of the asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    DATA_LABLE_CRITICAL_3 = TagType.BYTES | 12,
    /**
     * A tag whose value indicates the fourth customized critical data of the asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    DATA_LABLE_CRITICAL_4 = TagType.BYTES | 13,
    /**
     * A tag whose value indicates the first customized normal data of the asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    DATA_LABLE_NORMAL_1 = TagType.BYTES | 14,
    /**
     * A tag whose value indicates the second customized normal data of the asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    DATA_LABLE_NORMAL_2 = TagType.BYTES | 15,
    /**
     * A tag whose value indicates the third customized normal data of the asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    DATA_LABLE_NORMAL_3 = TagType.BYTES | 16,
    /**
     * A tag whose value indicates the fourth customized normal data of the asset.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    DATA_LABLE_NORMAL_4 = TagType.BYTES | 17,
    /**
     * A tag whose value indicates the type of the returned data.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    RETURN_TYPE = TagType.UINT32 | 18,
    /**
     * A tag whose value indicates the maximum number of assets that can be returned in a query.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    RETURN_LIMIT = TagType.UINT32 | 19,
    /**
     * A tag whose value indicates the offset of the batch query result.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    RETURN_OFFSET = TagType.UINT32 | 20,
    /**
     * A tag whose value indicates the order by which the query result is returned.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    RETURN_ORDER_BY = TagType.UINT32 | 21,
  }

  /**
   *  An enum type that indicates the asset error code.
   *
   * @enum { number }
   * @syscap SystemCapability.Security.Asset
   * @since 10
   */
  export enum ErrorCode {
    /**
     * The error code indicates that the permission is denied.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    PERMISSION_DENIED = 201,
    /**
     * The error code indicates that the parameter is invalid
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    INVALID_PARAMETER = 401,
    /**
     * The error code indicates that the capability is not supported.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    NOT_SUPPORTED = 801,
    /**
     * The error code indicates that the asset service is unavailable.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    SERVICE_UNAVAILABLE = 24000001,
    /**
     * The error code indicates that the asset to be queried is not found.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    NOT_FOUND = 24000002,
    /**
     * The error code indicates that the asset to be added is duplicate.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    DUPLICATED = 24000003,
    /**
     * The error code indicates that the asset access is denied.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    ACCESS_DENIED = 24000004,
    /**
     * The error code indicates that the authentication token has expired.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    AUTH_TOKEN_EXPIRED = 24000005,
    /**
     * The error code indicates that the system memory is insufficient.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    OUT_OF_MEMRORY = 24000006,
    /**
     * The error code indicates that the asset or key is corrupted.
     *
     * @syscap SystemCapability.Security.Asset
     * @since 11
     */
    DATA_CORRUPTED = 24000007,
  }
}

export default asset;
