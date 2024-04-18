/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file asset_system_api.h
 *
 * @brief Declares asset operation system interface.
 *
 * @since 11
 */

#ifndef ASSET_SYSTEM_API_H
#define ASSET_SYSTEM_API_H

#include <stdint.h>
#include <stdlib.h>

#include "asset_system_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Adds an asset.
 *
 * @param attributes Pointer to the attributes of the asset to add.
 * @param attributes Number of the attributes of the asset to add.
 * @return Returns <b>SEC_ASSET_SUCCESS</b> if the operation is successful; returns an error code otherwise.
 * @since 11
 */
int32_t AssetAdd(const AssetAttr *attributes, uint32_t attrCnt);

/**
 * @brief Removes one or more assets.
 *
 * @param query Pointer to the conditions for removing the assets.
 * @param queryCnt Number of conditions for removing the assets.
 * @return Returns <b>SEC_ASSET_SUCCESS</b> if the operation is successful; returns an error code otherwise.
 * @since 11
 */
int32_t AssetRemove(const AssetAttr *query, uint32_t queryCnt);

/**
 * @brief Updates an asset.
 *
 * @param query Pointer to the conditions for updating the asset.
 * @param queryCnt Number of conditions for updating the asset.
 * @param attributes Pointer to the attributes of the asset to update.
 * @param attributes Number of the attributes of the asset to update.
 * @return Returns <b>SEC_ASSET_SUCCESS</b> if the operation is successful; returns an error code otherwise.
 * @since 11
 */
int32_t AssetUpdate(const AssetAttr *query, uint32_t queryCnt,
    const AssetAttr *attributesToUpdate, uint32_t updateCnt);

/**
 * @brief Preprocesses data before querying the asset that can be accessed only after a successful user authentication.
 *
 * @param query Pointer to the search criteria of the asset.
 * @param queryCnt Number of the search criteria.
 * @param challenge Pointer to the challenge value to be used when <b>AssetQuery</b> is called.
 * @return Returns <b>SEC_ASSET_SUCCESS</b> if the operation is successful; returns an error code otherwise.
 * @since 11
 */
int32_t AssetPreQuery(const AssetAttr *query, uint32_t queryCnt, AssetBlob *challenge);

/**
 * @brief Queries assets.
 *
 * @param query Pointer to the search criteria.
 * @param queryCnt Number of the search criteria.
 * @param resultSet Pointer to the query result obtained.
 * @return Returns <b>SEC_ASSET_SUCCESS</b> if the operation is successful; returns an error code otherwise.
 * @since 11
 */
int32_t AssetQuery(const AssetAttr *query, uint32_t queryCnt, AssetResultSet *resultSet);

/**
 * @brief Processes data after the query of the asset that requires user authentication.
 *
 * @param handle Pointer to the handle of the data to process, which includes the challenge value returned by
 *     <b>AssetPreQuery</b>.
 * @param handleCnt Number of the elements in the handle attribute set.
 * @return Returns <b>SEC_ASSET_SUCCESS</b> if the operation is successful; returns an error code otherwise.
 * @since 11
 */
int32_t AssetPostQuery(const AssetAttr *handle, uint32_t handleCnt);

/**
 * @brief Parses the query result to obtain the specified attribute value.
 *
 * @param result Pointer to the query result to parse, which is obtained by <b>AssetQuery</b>.
 * @param tag Tag of the attribute to obtain.
 * @return Returns <b>AssetAttr</b> obtained if the operation is successful; returns <b>NULL</b> otherwise.
 *     The attribute does not need to be released by the service.
 * @since 11
 */
AssetAttr *AssetParseAttr(const AssetResult *result, AssetTag tag);

/**
 * @brief Releases the memory occupied by the challenge value.
 *
 * @param blob Pointer to the challenge value (obtained by <b>AssetPreQuery</b>) to release.
 * @since 11
 */
void AssetFreeBlob(AssetBlob *blob);

/**
 * @brief Releases the memory occupied by the query result.
 *
 * @param resultSet Pointer to the query result (obtained by <b>AssetQuery</b>) to release.
 * @since 11
 */
void AssetFreeResultSet(AssetResultSet *resultSet);

#ifdef __cplusplus
}
#endif

#endif // ASSET_SYSTEM_API_H
