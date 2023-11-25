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
 * @brief 该模块描述Asset对用户敏感数据（如密码、Token等）的生命周期管理能力，包括添加、删除、更新、查询等。
 *
 * @syscap SystemCapability.Security.Asset
 * @since 11
 */

/**
 * @file asset_api.h
 *
 * @brief 描述用于访问Asset的接口。
 *
 * @since 11
 */

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief 增加一条Asset数据。
 *
 * @param attributes 指向包含待添加到Asset数据的属性数组的指针。
 * @param attrCnt attributes数组中元素的个数。
 * @return 如果操作成功，则返回{@link Asset_ResultCode#ASSET_SUCCESS}；否则返回错误代码。
 * @since 11
 */
int32_t OH_Asset_Add(const Asset_Attr *attributes, uint32_t attrCnt);

/**
 * @brief 删除符合匹配条件的一条或多条Asset数据。
 *
 * @param query 指向包含用来匹配待删除Asset数据的属性数组的指针。
 * @param queryCnt query数组中元素的个数。
 * @return 如果操作成功，则返回{@link Asset_ResultCode#ASSET_SUCCESS}；否则返回错误代码。
 * @since 11
 */
int32_t OH_Asset_Remove(const Asset_Attr *query, uint32_t queryCnt);

/**
 * @brief 更新符合匹配条件的一条Asset数据。
 *
 * @param query 指向包含用来匹配待更新Asset数据的属性数组的指针。
 * @param queryCnt query数组中元素的个数。
 * @param attributesToUpdate 指向包含更新的Asset数据的属性数组的指针。
 * @param updateCnt attributesToUpdate数组中元素的个数。
 * @return 如果操作成功，则返回{@link Asset_ResultCode#ASSET_SUCCESS}；否则返回错误代码。
 * @since 11
 */
int32_t OH_Asset_Update(const Asset_Attr *query, uint32_t queryCnt,
    const Asset_Attr *attributesToUpdate, uint32_t updateCnt);

/**
 * @brief 对于需要用户认证的Asset数据的查询前的预处理（例如获取挑战值challenge）。
 *
 * @param query 指向包含用来匹配待查询Asset数据的属性数组的指针。
 * @param queryCnt query数组中元素的个数。
 * @param challenge 获取到的挑战值指针，在后续调用{@link OH_Asset_Query}时使用。
 * @return 如果操作成功，则返回{@link Asset_ResultCode#ASSET_SUCCESS}；否则返回错误代码。
 * @since 11
 */
int32_t OH_Asset_PreQuery(const Asset_Attr *query, uint32_t queryCnt, Asset_Blob *challenge);

/**
 * @brief 查询一条或多条符合匹配条件的Asset数据。
 *
 * @param query 指向包含用来匹配待查询Asset数据的属性数组的指针。
 * @param queryCnt query数组中元素的个数。
 * @param result 指向包含查询结果的数组的指针。
 * @return 如果操作成功，则返回{@link Asset_ResultCode#ASSET_SUCCESS}；否则返回错误代码。
 * @since 11
 */
int32_t OH_Asset_Query(const Asset_Attr *query, uint32_t queryCnt, Asset_ResultSet *resultSet);

/**
 * @brief 对于需要用户认证的Asset数据的查询后的后置处理（例如释放资源）。
 *
 * @param handle 指向从{@link OH_Asset_PreQuery}中获取的包含挑战值的数组指针。
 * @param handleCnt handle数组中元素的个数。
 * @return 如果操作成功，则返回{@link Asset_ResultCode#ASSET_SUCCESS}；否则返回错误代码。
 * @since 11
 */
int32_t OH_Asset_PostQuery(const Asset_Attr *handle, uint32_t handleCnt);

/**
 * @brief 解析AssetResult以获取指定的属性。
 *
 * @param result 指向包含从{@link OH_Asset_Query}中获取的查询结果的数组指针。
 * @param tag 指定属性的标签。
 * @return 如果操作成功，则以{@link #Asset_Attr}的形式返回属性，该属性不需要业务进行释放；
 *    否则返回NULL。
 * @since 11
 */
Asset_Attr *OH_Asset_ParseAttr(const Asset_Result *result, Asset_Tag tag);

/**
 * @brief 释放从{@link #OH_Asset_PreQuery}中获取的AssetBlob的内存。
 *
 * @param blob 指向需要释放的AssetBlob的指针。
 * @since 11
 */
void OH_Asset_FreeBlob(Asset_Blob *blob);

/**
 * @brief 释放从{@link #OH_Asset_Query}中获取的AssetResultSet的内存。
 *
 * @param resultSet 指向从{@link #OH_Asset_Query}得到的查询结果的指针。
 * @since 11
 */
void OH_Asset_FreeResultSet(Asset_ResultSet *resultSet);

#ifdef __cplusplus
}
#endif

/** @} */
#endif // ASSET_API_H
