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

#include "asset_ani_common.h"

#include <string>
#include <array>
#include <vector>

#include "securec.h"
#include <ani_signature_builder.h>

#include "asset_mem.h"
#include "asset_system_api.h"

#include "asset_api_check.h"
#include "asset_api_error_code.h"

using namespace OHOS::Security::Asset;

namespace OHOS {
namespace Security {
namespace Asset {
namespace {

bool IsMapType(ani_env *env, const ani_object &mapObj)
{
    ani_class mapType;
    std::string mapDesc = arkts::ani_signature::Builder::BuildClass({"escompat", "Map"}).Descriptor();
    if (env->FindClass(mapDesc.c_str(), &mapType) != ANI_OK) {
        LOGE("Not found Map");
        return false;
    }
    ani_boolean isMap;
    if (env->Object_InstanceOf(mapObj, static_cast<ani_type>(mapType), &isMap) != ANI_OK) {
        LOGE("Call Object_InstanceOf failed.");
        return false;
    }
    if (!isMap) {
        LOGE("It's not map object.");
        return false;
    }
    return true;
}

bool GetValueFromAniEnumItem(ani_env *env, ani_enum_item &enumObj, uint32_t &value)
{
    ani_int enumValue;
    if (env->EnumItem_GetValue_Int(enumObj, &enumValue) != ANI_OK) {
        LOGE("EnumItem_GetValue_Int failed.");
        return false;
    }
    value = static_cast<uint32_t>(enumValue);
    return true;
}

bool GetValueFromAniObject(ani_env *env, const ani_object &object, bool &value)
{
    ani_class booleanCls;
    std::string boolDesc = arkts::ani_signature::Builder::BuildClass({"std", "core", "Boolean"}).Descriptor();
    if (env->FindClass(boolDesc.c_str(), &booleanCls) != ANI_OK) {
        LOGE("Not found Boolean");
        return false;
    }
    ani_boolean isBoolean;
    if (env->Object_InstanceOf(object, booleanCls, &isBoolean) != ANI_OK) {
        LOGE("Object_InstanceOf boolean failed.");
        return false;
    }
    if (!isBoolean) {
        LOGE("It's not boolean object.");
        return false;
    }
    ani_boolean valueBoolean;
    std::string unboxedSignature = arkts::ani_signature::SignatureBuilder()
        .SetReturnBoolean().BuildSignatureDescriptor();
    if (env->Object_CallMethodByName_Boolean(object, "toBoolean", unboxedSignature.c_str(), &valueBoolean) != ANI_OK) {
        LOGE("Object_CallMethodByName_Boolean failed.");
        return false;
    }
    value = static_cast<bool>(valueBoolean);
    return true;
}

bool GetValueFromAniObject(ani_env *env, const ani_object &object, uint32_t &value)
{
    int32_t rawValue = 0;
    ani_class intCls;
    std::string intDesc = arkts::ani_signature::Builder::BuildClass({"std", "core", "Int"}).Descriptor();
    if (env->FindClass(intDesc.c_str(), &intCls) != ANI_OK) {
        LOGE("Not found Int");
        return false;
    }
    ani_boolean isInt;
    if (env->Object_InstanceOf(object, intCls, &isInt) != ANI_OK) {
        LOGE("Object_InstanceOf int failed");
        return false;
    }
    if (!isInt) {
        LOGE("It's not int object");
        return false;
    }
    std::string intValueSignature = arkts::ani_signature::SignatureBuilder().SetReturnInt().BuildSignatureDescriptor();
    if (env->Object_CallMethodByName_Int(object, "intValue", intValueSignature.c_str(), &rawValue) != ANI_OK) {
        LOGE("Object_CallMethodByName_Int failed.");
        return false;
    }
    value = static_cast<uint32_t>(rawValue);
    return true;
}

bool GetValueFromAniObject(ani_env *env, const ani_object &object, AssetBlob &blobOut)
{
    ani_ref buffer;
    if (env->Object_GetFieldByName_Ref(object, "buffer", &buffer) != ANI_OK) {
        LOGE("Object_GetFieldByName_Ref buffer failed.");
        return false;
    }
    void *data = nullptr;
    size_t length = 0;
    if (env->ArrayBuffer_GetInfo(static_cast<ani_arraybuffer>(buffer), &data, &length) != ANI_OK) {
        LOGE("ArrayBuffer_GetInfo failed.");
        return false;
    }

    blobOut.size = length;
    blobOut.data = static_cast<uint8_t *>(AssetMalloc(blobOut.size));
    if (blobOut.data == nullptr) {
        LOGE("Unable to allocate memory for AssetBlob!");
        return false;
    }
    if (memcpy_s(blobOut.data, blobOut.size, data, blobOut.size) != EOK) {
        LOGE("Copy memory to AssetBlob failed");
        AssetFree(blobOut.data);
        return false;
    }
    return true;
}

bool CreateInt32Object(ani_env *env, uint32_t value, ani_object &intObj)
{
    ani_class intCls;
    std::string intDesc = arkts::ani_signature::Builder::BuildClass({"std", "core", "Int"}).Descriptor();
    if (env->FindClass(intDesc.c_str(), &intCls) != ANI_OK) {
        LOGE("FindClass int failed.");
        return false;
    }
    ani_method ctor;
    std::string ctorSignature = arkts::ani_signature::SignatureBuilder().AddInt().BuildSignatureDescriptor();
    if (env->Class_FindMethod(intCls, "<ctor>", ctorSignature.c_str(), &ctor) != ANI_OK) {
        LOGE("Class_FindMethod <ctor> int32 failed.");
        return false;
    }
    if (env->Object_New(intCls, ctor, &intObj, static_cast<ani_int>(value)) != ANI_OK) {
        LOGE("Object_New <ctor> int32 failed.");
        return false;
    }
    return true;
}

bool CreateBooleanObject(ani_env *env, bool value, ani_object &boolObj)
{
    ani_class boolCls;
    std::string boolDesc = arkts::ani_signature::Builder::BuildClass({"std", "core", "Boolean"}).Descriptor();
    if (env->FindClass(boolDesc.c_str(), &boolCls) != ANI_OK) {
        LOGE("FindClass boolean failed.");
        return false;
    }
    ani_method ctor;
    std::string ctorSignature = arkts::ani_signature::SignatureBuilder().AddBoolean().BuildSignatureDescriptor();
    if (env->Class_FindMethod(boolCls, "<ctor>", ctorSignature.c_str(), &ctor) != ANI_OK) {
        LOGE("Class_FindMethod <ctor> boolean failed.");
        return false;
    }
    if (env->Object_New(boolCls, ctor, &boolObj, static_cast<ani_boolean>(value)) != ANI_OK) {
        LOGE("Object_New <ctor> boolean failed.");
        return false;
    }
    return true;
}

bool CreateAssetEnumItemObject(ani_env *env, uint32_t tag, ani_enum_item &enumTag)
{
    std::string assetTagEnumName = arkts::ani_signature::Builder::BuildClass({
        "@ohos", "security", "asset", "asset", "Tag"}).Descriptor();
    ani_enum enumType;
    if (env->FindEnum(assetTagEnumName.c_str(), &enumType) != ANI_OK) {
        LOGE("FindEnum failed.");
        return false;
    }
    if (env->Enum_GetEnumItemByName(enumType, TAG_MAP.at(tag), &enumTag) != ANI_OK) {
        LOGE("Enum_GetEnumItemByName failed.");
        return false;
    }
    return true;
}

bool SetAssetAttribute(ani_env *env, const AssetAttr &attr, const ani_enum_item &enumTag, ani_object &mapOut)
{
    ani_ref setRef;
    ani_object bufferObj = nullptr;
    switch (attr.tag & SEC_ASSET_TAG_TYPE_MASK) {
        case AssetTagType::SEC_ASSET_TYPE_BOOL:
            if (!CreateBooleanObject(env, attr.value.boolean, bufferObj)) {
                LOGE("CreateBooleanObject failed.");
                return false;
            }
            break;
        case AssetTagType::SEC_ASSET_TYPE_NUMBER:
            if (!CreateInt32Object(env, attr.value.u32, bufferObj)) {
                LOGE("CreateInt32Object failed.");
                return false;
            }
            break;
        case AssetTagType::SEC_ASSET_TYPE_BYTES:
            if (!CreateAniUint8Array(env, attr.value.blob, bufferObj)) {
                LOGE("CreateAniUint8Array with blob, but create ani object failed.");
                return false;
            }
            break;
        default:
            LOGE("Undefined AssetTagType, tag value is %{public}u", attr.tag);
            return false;
    }
    std::string setSignature = arkts::ani_signature::SignatureBuilder()
        .AddAny().AddAny()
        .SetReturnClass({"escompat", "Map"}).BuildSignatureDescriptor();
    if (env->Object_CallMethodByName_Ref(mapOut, "set", setSignature.c_str(),
        &setRef, enumTag, bufferObj) != ANI_OK) {
        LOGE("CreateAniMap set map failed.");
        return false;
    }
    return true;
}

bool ParseAssetTagFromAni(ani_env *env, ani_ref &next, ani_enum_item &tagEnumItem, AssetAttr &attr)
{
    ani_ref tagRef;
    if (env->Object_GetFieldByName_Ref(static_cast<ani_object>(next), "value", &tagRef) != ANI_OK) {
        LOGE("Failed to get key value");
        return false;
    }
    tagEnumItem = static_cast<ani_enum_item>(tagRef);
    if (!GetValueFromAniEnumItem(env, tagEnumItem, attr.tag)) {
        LOGE("GetValueFromAniEnumItem failed.");
        return false;
    }
    return true;
}

bool ParseAssetValueFromAni(ani_env *env, const ani_object &paramObj, const ani_enum_item &tagEnumItem, AssetAttr &attr)
{
    bool ret = false;
    ani_ref valueRef;
    std::string getSignature = arkts::ani_signature::SignatureBuilder()
        .AddAny().SetReturnAny().BuildSignatureDescriptor();
    if (env->Object_CallMethodByName_Ref(paramObj, "get", getSignature.c_str(), &valueRef,
        tagEnumItem) != ANI_OK) {
        LOGE("Failed to get value for key");
        return ret;
    }
    ani_object valueObject = reinterpret_cast<ani_object>(valueRef);
    switch (attr.tag & SEC_ASSET_TAG_TYPE_MASK) {
        case AssetTagType::SEC_ASSET_TYPE_BOOL:
            ret = GetValueFromAniObject(env, valueObject, attr.value.boolean);
            break;
        case AssetTagType::SEC_ASSET_TYPE_NUMBER:
            ret = GetValueFromAniObject(env, valueObject, attr.value.u32);
            break;
        case AssetTagType::SEC_ASSET_TYPE_BYTES:
            ret = GetValueFromAniObject(env, valueObject, attr.value.blob);
            break;
        default:
            LOGE("Undefined AssetTagType, tag value is %{public}u", attr.tag);
    }
    return ret;
}

bool CreateAniMap(ani_env *env, const AssetResult &result, ani_object &mapOut)
{
    ani_class mapCls;
    std::string mapDesc = arkts::ani_signature::Builder::BuildClass({"escompat", "Map"}).Descriptor();
    if (env->FindClass(mapDesc.c_str(), &mapCls) != ANI_OK) {
        LOGE("FindClass map failed.");
        return false;
    }
    ani_method mapCtor;
    std::string ctorSignature = arkts::ani_signature::SignatureBuilder().BuildSignatureDescriptor();
    if (env->Class_FindMethod(mapCls, "<ctor>", ctorSignature.c_str(), &mapCtor) != ANI_OK) {
        LOGE("Class_FindMethod failed.");
        return false;
    }
    if (env->Object_New(mapCls, mapCtor, &mapOut, nullptr) != ANI_OK) {
        LOGE("Object_New Map failed.");
        return false;
    }
    for (uint32_t i = 0; i < result.count; i++) {
        ani_enum_item enumTag;
        if (!CreateAssetEnumItemObject(env, result.attrs[i].tag, enumTag)) {
            LOGE("CreateAssetEnumItemObject failed");
            return false;
        }
        if (!SetAssetAttribute(env, result.attrs[i], enumTag, mapOut)) {
            LOGE("SetAssetAttribute failed.");
            return false;
        }
    }
    return true;
}
} // anonymous namespace

bool CreateAniUint8Array(ani_env *env, const AssetBlob &blob, ani_object &arrayOut)
{
    ani_class arrayCls;
    std::string u8arrDesc = arkts::ani_signature::Builder::BuildClass({"escompat", "Uint8Array"}).Descriptor();
    if (env->FindClass(u8arrDesc.c_str(), &arrayCls) != ANI_OK) {
        LOGE("FindClass Uint8Array failed.");
        return false;
    }
    ani_method arrayCtor;
    std::string ctorSignature = arkts::ani_signature::SignatureBuilder().AddInt().BuildSignatureDescriptor();
    if (env->Class_FindMethod(arrayCls, "<ctor>", ctorSignature.c_str(), &arrayCtor) != ANI_OK) {
        LOGE("Class_FindMethod failed.");
        return false;
    }
    if (env->Object_New(arrayCls, arrayCtor, &arrayOut, blob.size) != ANI_OK) {
        LOGE("Object_New Uint8Array failed.");
        return false;
    }
    ani_ref buffer;
    if (env->Object_GetFieldByName_Ref(arrayOut, "buffer", &buffer) != ANI_OK) {
        LOGE("Object_GetFieldByName_Ref Uint8Array failed.");
        return false;
    }
    void *bufData = nullptr;
    size_t bufLength = 0;
    if (env->ArrayBuffer_GetInfo(static_cast<ani_arraybuffer>(buffer), &bufData, &bufLength) != ANI_OK) {
        LOGE("ArrayBuffer_GetInfo failed.");
        return false;
    }
    if (memcpy_s(bufData, bufLength, blob.data, blob.size) != EOK) {
        LOGE("Failed: memcpy_s");
        return false;
    }
    return true;
}

bool CreateAniSyncResult(ani_env *env, const AssetSyncResult &syncResult, ani_object &syncResultOut)
{
    ani_class syncResultCls;
    std::string syncResultClassName = arkts::ani_signature::Builder::BuildClass({
        "@ohos", "security", "asset", "SyncResultInner"}).Descriptor();
    if (env->FindClass(syncResultClassName.c_str(), &syncResultCls) != ANI_OK) {
        LOGE("FindClass SyncResultInner failed.");
        return false;
    }
    ani_method syncResultCtor;
    if (env->Class_FindMethod(syncResultCls, "<ctor>", nullptr, &syncResultCtor) != ANI_OK) {
        LOGE("Class_FindMethod failed.");
        return false;
    }
    if (env->Object_New(syncResultCls, syncResultCtor, &syncResultOut) != ANI_OK) {
        LOGE("Object_New syncResult failed.");
        return false;
    }
    if (env->Object_SetPropertyByName_Int(syncResultOut,
        "resultCode", syncResult.resultCode) != ANI_OK) {
        LOGE("Object_SetPropertyByName_Int resultCode failed.");
        return false;
    }
    if (env->Object_SetPropertyByName_Int(syncResultOut,
        "totalCount", syncResult.totalCount) != ANI_OK) {
        LOGE("Object_SetPropertyByName_Int totalCount failed.");
        return false;
    }
    if (env->Object_SetPropertyByName_Int(syncResultOut,
        "failedCount", syncResult.failedCount) != ANI_OK) {
        LOGE("Object_SetPropertyByName_Int failedCount failed.");
        return false;
    }
    return true;
}

int32_t ParseAssetAttributeFromAni(ani_env *env, const ani_object &paramObj, std::vector<AssetAttr> &attrs)
{
    if (!IsMapType(env, paramObj)) {
        LOGE("It's not map type.");
        return SEC_ASSET_INVALID_ARGUMENT;
    }
    ani_ref keys;
    std::string keysSignature = arkts::ani_signature::SignatureBuilder()
        .SetReturnClass({"std", "core", "IterableIterator"}).BuildSignatureDescriptor();
    if (env->Object_CallMethodByName_Ref(paramObj, "keys", keysSignature.c_str(), &keys) != ANI_OK) {
        LOGE("Failed to get keys iterator.");
        return SEC_ASSET_INVALID_ARGUMENT;
    }
    while (true) {
        ani_ref next;
        std::string nextSignature = arkts::ani_signature::SignatureBuilder()
            .SetReturnClass({"std", "core", "IteratorResult"}).BuildSignatureDescriptor();
        if (env->Object_CallMethodByName_Ref(static_cast<ani_object>(keys),
            "next", nextSignature.c_str(), &next) != ANI_OK) {
            LOGE("Failed to get next key");
            return SEC_ASSET_INVALID_ARGUMENT;
        }
        ani_boolean done;
        if (env->Object_GetFieldByName_Boolean(static_cast<ani_object>(next), "done", &done) != ANI_OK) {
            LOGE("Failed to check iterator done");
            return SEC_ASSET_INVALID_ARGUMENT;
        }
        if (done) {
            break;
        }

        AssetAttr tempAttribute;
        ani_enum_item tagEnumItem;
        if (!ParseAssetTagFromAni(env, next, tagEnumItem, tempAttribute)) {
            LOGE("ParseAssetTagFromAni failed.");
            return SEC_ASSET_INVALID_ARGUMENT;
        }

        if (!ParseAssetValueFromAni(env, paramObj, tagEnumItem, tempAttribute)) {
            LOGE("ParseAssetValueFromAni failed.");
            return SEC_ASSET_INVALID_ARGUMENT;
        }
        attrs.emplace_back(tempAttribute);
    }
    return SEC_ASSET_SUCCESS;
}

int32_t CreateAniMapArray(ani_env *env, const AssetResultSet &resultSet, ani_object &mapArrayOut)
{
    ani_class arrayCls;
    std::string arrDesc = arkts::ani_signature::Builder::BuildClass({"escompat", "Array"}).Descriptor();
    if (env->FindClass(arrDesc.c_str(), &arrayCls) != ANI_OK) {
        LOGE("FindClass array failed.");
        return SEC_ASSET_INVALID_ARGUMENT;
    }
    ani_method arrayCtor;
    std::string ctorSignature = arkts::ani_signature::SignatureBuilder().AddInt().BuildSignatureDescriptor();
    if (env->Class_FindMethod(arrayCls, "<ctor>", ctorSignature.c_str(), &arrayCtor) != ANI_OK) {
        LOGE("Class_FindMethod failed.");
        return SEC_ASSET_INVALID_ARGUMENT;
    }
    if (env->Object_New(arrayCls, arrayCtor, &mapArrayOut, resultSet.count) != ANI_OK) {
        LOGE("Object_New Array failed.");
        return SEC_ASSET_INVALID_ARGUMENT;
    }
    for (uint32_t i = 0; i < resultSet.count; i++) {
        if (resultSet.results[i].attrs == nullptr || resultSet.results[i].count == 0) {
            LOGE("Invalid resultSet.");
            return SEC_ASSET_INVALID_ARGUMENT;
        }
        ani_object mapOut{};
        if (!CreateAniMap(env, resultSet.results[i], mapOut)) {
            LOGE("CreateAniMap failed.");
            return SEC_ASSET_INVALID_ARGUMENT;
        }
        std::string setSignature = arkts::ani_signature::SignatureBuilder()
            .AddInt().AddAny().BuildSignatureDescriptor();
        if (env->Object_CallMethodByName_Void(mapArrayOut, "$_set", setSignature.c_str(), i, mapOut) != ANI_OK) {
            LOGE("Object_CallMethodByName_Void Array failed.");
            return SEC_ASSET_INVALID_ARGUMENT;
        }
    }
    return SEC_ASSET_SUCCESS;
}

ani_object CreateAniError(ani_env *env, const int32_t result, const char *errMsg)
{
    ani_object aniResultObj{};
    ani_class cls;
    std::string businessErrorClassName = arkts::ani_signature::Builder::BuildClass({
        "@ohos", "base", "BusinessError"}).Descriptor();
    if (env->FindClass(businessErrorClassName.c_str(), &cls) != ANI_OK) {
        LOGE("FindClass Failed: Not found %{public}s", businessErrorClassName.c_str());
        return {};
    }
    ani_method ctor;
    std::string ctorSignature = arkts::ani_signature::SignatureBuilder().BuildSignatureDescriptor();
    if (env->Class_FindMethod(cls, "<ctor>", ctorSignature.c_str(), &ctor) != ANI_OK) {
        LOGE("Class_FindMethod Failed: <ctor> %{public}s", businessErrorClassName.c_str());
        return {};
    }
    if (env->Object_New(cls, ctor, &aniResultObj) != ANI_OK) {
        LOGE("Create Object failed %{public}s", businessErrorClassName.c_str());
        return {};
    }
    if (env->Object_SetPropertyByName_Int(aniResultObj, "code", result) != ANI_OK) {
        LOGE("Object_SetPropertyByName_Int result failed.");
        return {};
    }
    if (errMsg != nullptr) {
        ani_string errMsgStr;
        if (env->String_NewUTF8(errMsg, strlen(errMsg), &errMsgStr) != ANI_OK) {
            LOGE("String_NewUTF8 errMsg failed.");
            return {};
        }
        if (env->Object_SetPropertyByName_Ref(aniResultObj, "message", errMsgStr) != ANI_OK) {
            LOGE("Object_SetPropertyByName_Ref error failed.");
            return {};
        }
    }
    return aniResultObj;
}

ani_object CreateAniResult(ani_env *env, const int32_t result, const char *errMsg, const ani_object &resultObj)
{
    ani_object aniResultObj{};
    ani_class cls;
    std::string businessErrorClassName = arkts::ani_signature::Builder::BuildClass({
        "@ohos", "base", "BusinessError"}).Descriptor();
    if (env->FindClass(businessErrorClassName.c_str(), &cls) != ANI_OK) {
        LOGE("FindClass Failed: Not found %{public}s", businessErrorClassName.c_str());
        return {};
    }
    ani_method ctor;
    std::string ctorSignature = arkts::ani_signature::SignatureBuilder().BuildSignatureDescriptor();
    if (env->Class_FindMethod(cls, "<ctor>", ctorSignature.c_str(), &ctor) != ANI_OK) {
        LOGE("Class_FindMethod Failed: <ctor> %{public}s", businessErrorClassName.c_str());
        return {};
    }
    if (env->Object_New(cls, ctor, &aniResultObj) != ANI_OK) {
        LOGE("Create Object failed %{public}s", businessErrorClassName.c_str());
        return {};
    }
    if (env->Object_SetPropertyByName_Int(aniResultObj, "code", result) != ANI_OK) {
        LOGE("Object_SetPropertyByName_Int result failed.");
        return {};
    }
    if (errMsg != nullptr) {
        ani_string errMsgStr;
        if (env->String_NewUTF8(errMsg, strlen(errMsg), &errMsgStr) != ANI_OK) {
            LOGE("String_NewUTF8 errMsg failed.");
            return {};
        }
        if (env->Object_SetPropertyByName_Ref(aniResultObj, "message", errMsgStr) != ANI_OK) {
            LOGE("Object_SetPropertyByName_Ref error failed.");
            return {};
        }
    }

    if (resultObj != nullptr) {
        if (env->Object_SetPropertyByName_Ref(aniResultObj, "data", resultObj) != ANI_OK) {
            LOGE("Object_SetPropertyByName_Ref resultObj failed.");
            return {};
        }
    }
    return aniResultObj;
}

void FreeAssetAttrs(std::vector<AssetAttr> &attrs)
{
    for (auto attr : attrs) {
        if ((attr.tag & SEC_ASSET_TAG_TYPE_MASK) == SEC_ASSET_TYPE_BYTES) {
            AssetFreeBlob(&attr.value.blob);
        }
    }
    attrs.clear();
}
} // Asset
} // Security
} // OHOS
