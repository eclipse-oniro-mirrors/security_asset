/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ASSET_API_H
#define ASSET_API_H

#include <stdint.h>
#include <stdlib.h>

#include "asset_type.h"

/**
 * @addtogroup AssetApi
 * @{
 *
 * @brief Describes the Asset capability of life cycle management of sensitive user data, such as passwords
 *    and tokens, including adding, removing, updating, and querying.
 *
 * @syscap SystemCapability.Security.Asset
 * @since 11
 */

/**
 * @file asset_api.h
 *
 * @brief Declares the APIs used to access the Asset.
 *
 * @since 11
 */

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Add an Asset.
 *
 * @param attributes Indicates the pointer to the array including attributes of the Asset to be added.
 * @param attrCnt Indicates the count of elements in attributes array.
 * @return Returns {@link Asset_ResultCode#ASSET_SUCCESS} if the operation is successful;
 *    returns an error code otherwise.
 * @since 11
 */
int32_t OH_Asset_Add(const Asset_Attr *attributes, uint32_t attrCnt);

/**
 * @brief Remove one or more Assets that match a search query.
 *
 * @param query Indicates the pointer to the array including attributes of the Asset to be removed.
 * @param queryCnt Indicates the count of elements in query array.
 * @return Returns {@link Asset_ResultCode#ASSET_SUCCESS} if the operation is successful;
 *    returns an error code otherwise.
 * @since 11
 */
int32_t OH_Asset_Remove(const Asset_Attr *query, uint32_t queryCnt);

/**
 * @brief Update an Asset that matches a search query.
 *
 * @param query Indicates the pointer to the array of parameters to add. of the Asset to be updated.
 * @param queryCnt Indicates the count of elements in query array.
 * @param attributesToUpdate Indicates the pointer to the array including attributes with new values.
 * @param updateCnt Indicates the count of elements in attributesToUpdate array.
 * @return Returns {@link Asset_ResultCode#ASSET_SUCCESS} if the operation is successful;
 *    returns an error code otherwise.
 * @since 11
 */
int32_t OH_Asset_Update(const Asset_Attr *query, uint32_t queryCnt,
    const Asset_Attr *attributesToUpdate, uint32_t updateCnt);

/**
 * @brief Preprocessing (e.g. get challenge) for querying one or more Assets that require user authentication.
 *
 * @param query Indicates the pointer to the array including attributes of the Asset to be queried.
 * @param queryCnt Indicates the count of elements in query array.
 * @param challenge Indicates the pointer to the challenge value obtained
*      which is used later in {@link OH_Asset_Query}.
 * @return Returns {@link Asset_ResultCode#ASSET_SUCCESS} if the operation is successful;
 *    returns an error code otherwise.
 * @since 11
 */
int32_t OH_Asset_PreQuery(const Asset_Attr *query, uint32_t queryCnt, Asset_Blob *challenge);

/**
 * @brief Query one or more Assets that match a search query.
 *
 * @param query Indicates the pointer to the array including attributes of the Asset to be queried.
 * @param queryCnt Indicates the count of elements in query array.
 * @param result Indicates pointer to the array including query results.
 * @return Returns {@link Asset_ResultCode#ASSET_SUCCESS} if the operation is successful;
 *    returns an error code otherwise.
 * @since 11
 */
int32_t OH_Asset_Query(const Asset_Attr *query, uint32_t queryCnt, Asset_ResultSet *result);

/**
 * @brief Post-processing (e.g. release cached resource) for querying multiple Assets that require user authentication.
 *
 * @param handle Indicates the pointer to the array including challenge obtained from {@link OH_Asset_PreQuery}.
 * @param handleCnt Indicates the count of elements in handle array.
 * @return Returns {@link Asset_ResultCode#ASSET_SUCCESS} if the operation is successful;
 *    returns an error code otherwise.
 * @since 11
 */
int32_t OH_Asset_PostQuery(const Asset_Attr *handle, uint32_t handleCnt);

/**
 * @brief Obtains the current Asset SDK version.
 *
 * @return Returns the current Asset SDK version in form of {@link #Asset_Version}.
 * @since 11
 */
Asset_Version OH_Asset_GetVersion(void);

/**
 * @brief Parse the AssetResult to get the specified attribute.
 *
 * @param result Indicates the pointer to the array including query results obtained from {@link OH_Asset_Query}.
 * @param tag Indicates the tag of specified attribute.
 * @return Returns the attribute in form of {@link #Asset_Attr} if the operation is successful which does not
 *    need to be released;
 *    returns NULL otherwise.
 * @since 11
 */
Asset_Attr *OH_Asset_ParseAttr(const Asset_Result *result, Asset_Tag tag);

/**
 * @brief Release the AssetBlob obtained from {@link #OH_Asset_PreQuery}.
 *
 * @param blob Indicates the pointer to blob which needs to be freed.
 * @since 11
 */
void OH_Asset_FreeBlob(Asset_Blob *blob);

/**
 * @brief Release the AssetResultSet obtained from {@link #OH_Asset_Query}.
 *
 * @param resultSet Indicates the pointer to the query results obtained from {@link #OH_Asset_Query}.
 * @since 11
 */
void OH_Asset_FreeResultSet(Asset_ResultSet *resultSet);

#ifdef __cplusplus
}
#endif

/** @} */
#endif /* ASSET_API_H */
