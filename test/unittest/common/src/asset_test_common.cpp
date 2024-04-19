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

#include "asset_test_common.h"

#include <string>

#include "asset_api.h"
#include "asset_system_api.h"

int32_t RemoveByAliasNdk(const char *alias)
{
    Asset_Attr attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = {
                .size = static_cast<uint32_t>(strlen(alias)),
                .data = reinterpret_cast<uint8_t*>(const_cast<char*>(alias))
            }
        }
    };
    return OH_Asset_Remove(attr, ARRAY_SIZE(attr));
}

int32_t RemoveByAliasSdk(const char *alias)
{
    AssetAttr attr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS,
          .value.blob = { .size = static_cast<uint32_t>(strlen(alias)),
              .data = reinterpret_cast<uint8_t*>(const_cast<char*>(alias)) } },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID }
    };
    return AssetRemove(attr, ARRAY_SIZE(attr));
}

int32_t QueryByAliasNdk(const char *alias, Asset_ResultSet *resultSet)
{
    Asset_Attr attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = {
                .size = static_cast<uint32_t>(strlen(alias)),
                .data = reinterpret_cast<uint8_t*>(const_cast<char*>(alias))
            }
        }, {
            .tag = ASSET_TAG_RETURN_TYPE,
            .value.u32 = ASSET_RETURN_ALL
        }
    };
    return OH_Asset_Query(attr, ARRAY_SIZE(attr), resultSet);
}

int32_t QueryByAliasSdk(const char *alias, AssetResultSet *resultSet)
{
    AssetAttr attr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS,
          .value.blob = { .size = static_cast<uint32_t>(strlen(alias)),
              .data = reinterpret_cast<uint8_t*>(const_cast<char*>(alias)) } },
        { .tag = SEC_ASSET_TAG_RETURN_TYPE, .value.u32 = SEC_ASSET_RETURN_ALL },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID }
    };
    return AssetQuery(attr, ARRAY_SIZE(attr), resultSet);
}

bool CompareBlobNdk(const Asset_Blob *blob1, const Asset_Blob *blob2)
{
    return CompareBlobSdk((const AssetBlob *)blob1, (const AssetBlob *)blob2);
}

bool CompareBlobSdk(const AssetBlob *blob1, const AssetBlob *blob2)
{
    if (blob1->size != blob2->size) {
        return false;
    }
    return memcmp(blob1->data, blob2->data, blob1->size) == 0;
}

bool CheckMatchAttrResultNdk(const Asset_Attr *attrs, uint32_t attrCnt, const Asset_Result *result)
{
    return CheckMatchAttrResultSdk((const AssetAttr *)attrs, attrCnt, (const AssetResult *)result);
}

bool CheckMatchAttrResultSdk(const AssetAttr *attrs, uint32_t attrCnt, const AssetResult *result)
{
    for (uint32_t i = 0; i < attrCnt; i++) {
        if (attrs[i].tag == SEC_ASSET_TAG_CONFLICT_RESOLUTION || attrs[i].tag == SEC_ASSET_TAG_USER_ID) {
            continue;
        }
        AssetAttr *res = AssetParseAttr(result, static_cast<AssetTag>(attrs[i].tag));
        if (res == nullptr) {
            return false;
        }
        switch (attrs[i].tag & SEC_ASSET_TAG_TYPE_MASK) {
            case SEC_ASSET_TYPE_BOOL:
                if (attrs[i].value.boolean != res->value.boolean) {
                    printf("tag is %x, %u vs %u", attrs[i].tag, attrs[i].value.boolean, res->value.boolean);
                    return false;
                }
                break;
            case SEC_ASSET_TYPE_NUMBER:
                if (attrs[i].value.u32 != res->value.u32) {
                    printf("tag is %x, %u vs %u", attrs[i].tag, attrs[i].value.u32, res->value.u32);
                    return false;
                }
                break;
            case SEC_ASSET_TYPE_BYTES:
                if (!CompareBlobSdk(&attrs[i].value.blob, &res->value.blob)) {
                    printf("tag is %x, len %u vs len %u", attrs[i].tag, attrs[i].value.blob.size, res->value.blob.size);
                    return false;
                }
                break;
            default:
                return false;
        };
    }
    return true;
}