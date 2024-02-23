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
 * @brief 提供用户短敏感数据的安全存储及管理能力，包括新增、删除、更新、查询等。
 * 其中，短敏感数据可以是密码类（账号/密码）、Token类（应用凭据）、其它关键明文（如银行卡号）等长度较短的用户敏感数据。
 *
 * @since 11
 */

/**
 * @file asset_api.h
 *
 * @brief 声明用于访问关键资产的接口。
 *
 * @kit Asset Store Kit
 * @syscap SystemCapability.Security.Asset
 * @since 11
 */

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief 新增一条关键资产。
 *
 * @param attributes 待新增关键资产的属性集合。
 * @param attrCnt 待新增关键资产的属性数量。
 * @return 如果操作成功，则返回ASSET_SUCCESS；否则返回错误码。
 * @since 11
 */
int32_t OH_Asset_Add(const Asset_Attr *attributes, uint32_t attrCnt);

/**
 * @brief 删除符合条件的一条或多条关键资产。
 *
 * @param query 待删除关键资产的搜索条件。
 * @param queryCnt 待删除关键资产搜索条件的个数。
 * @return 如果操作成功，则返回ASSET_SUCCESS；否则返回错误码。
 * @since 11
 */
int32_t OH_Asset_Remove(const Asset_Attr *query, uint32_t queryCnt);

/**
 * @brief 更新符合条件的一条关键资产。
 *
 * @param query 待更新关键资产的搜索条件。
 * @param queryCnt 待更新关键资产搜索条件的个数。
 * @param attributesToUpdate 待更新关键资产的属性集合。
 * @param updateCnt 待更新关键资产的属性数量。
 * @return 如果操作成功，则返回ASSET_SUCCESS；否则返回错误码。
 * @since 11
 */
int32_t OH_Asset_Update(const Asset_Attr *query, uint32_t queryCnt,
    const Asset_Attr *attributesToUpdate, uint32_t updateCnt);

/**
 * @brief 查询的预处理，用于需要用户校验的关键资产。
 *
 * @param query 关键资产的查询条件。
 * @param queryCnt 关键资产查询条件的个数。
 * @param challenge 挑战值，在后续调用OH_Asset_Query时使用。
 * @return 如果操作成功，则返回ASSET_SUCCESS；否则返回错误码。
 * @since 11
 */
int32_t OH_Asset_PreQuery(const Asset_Attr *query, uint32_t queryCnt, Asset_Blob *challenge);

/**
 * @brief 查询一条或多条符合条件的关键资产。
 *
 * @param query 关键资产的查询条件。
 * @param queryCnt 关键资产查询条件的个数。
 * @param resultSet 查询结果列表。
 * @return 如果操作成功，则返回ASSET_SUCCESS；否则返回错误码。
 * @since 11
 */
int32_t OH_Asset_Query(const Asset_Attr *query, uint32_t queryCnt, Asset_ResultSet *resultSet);

/**
 * @brief 查询的后置处理，用于需要用户校验的关键资产。
 *
 * @param handle 待处理的查询句柄，当前包含OH_Asset_PreQuery执行成功返回的挑战值。
 * @param handleCnt 句柄属性集合中元素的个数。
 * @return 如果操作成功，则返回ASSET_SUCCESS；否则返回错误码。
 * @since 11
 */
int32_t OH_Asset_PostQuery(const Asset_Attr *handle, uint32_t handleCnt);

/**
 * @brief 解析查询结果，并获取指定的属性值。
 *
 * @param result 从OH_Asset_Query中获取的查询结果。
 * @param tag 待获取的属性标签。
 * @return 如果操作成功，则以Asset_Attr的形式返回属性，该属性不需要业务进行释放；否则返回NULL。
 * @since 11
 */
Asset_Attr *OH_Asset_ParseAttr(const Asset_Result *result, Asset_Tag tag);

/**
 * @brief 释放挑战值所占用的内存。
 *
 * @param blob 从OH_Asset_PreQuery获取的挑战值。
 * @since 11
 */
void OH_Asset_FreeBlob(Asset_Blob *blob);

/**
 * @brief 释放查询结果所占用的内存。
 *
 * @param resultSet 从OH_Asset_Query得到的查询结果列表。
 * @since 11
 */
void OH_Asset_FreeResultSet(Asset_ResultSet *resultSet);

#ifdef __cplusplus
}
#endif

/** @} */
#endif // ASSET_API_H
