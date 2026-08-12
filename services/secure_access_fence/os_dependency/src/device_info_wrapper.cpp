/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "device_info_wrapper.h"

#include "parameter.h"

#include "saf_log.h"
#include "saf_result_code.h"

int32_t GetLocalDeviceId(uint8_t *udid, int32_t udidLen)
{
    constexpr int32_t INPUT_UDID_LEN = 65;
    constexpr int32_t MAX_INPUT_UDID_LEN = 200;
    if (udid == nullptr || udidLen < INPUT_UDID_LEN || udidLen > MAX_INPUT_UDID_LEN) {
        LOGE("[FATAL]Invalid params, udidLen is %{public}d", udidLen);
        return SAF_ERR_ARG_INVALID;
    }
    int32_t ret = GetDevUdid(reinterpret_cast<char*>(udid), udidLen);
    if (ret != 0) {
        LOGE("[FATAL]GetDevUdid failed, ret is %{public}d", ret);
        return ret;
    }
    return SAF_SUCCESS;
}