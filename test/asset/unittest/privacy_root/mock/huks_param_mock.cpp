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

#include "huks_param_mock.h"
#include "hks_error_code.h"
#include "hks_param.h"

using namespace PrivacyRootTest;

namespace {
bool TakeParamFailure(ParamFailure failure)
{
    if (g_paramFault.failure != failure || g_paramFault.initCount != g_paramFault.paramSet) {
        return false;
    }
    g_paramFault.failure = ParamFailure::NONE;
    return true;
}

} // namespace

extern "C" int32_t MockInitParamSet(HksParamSet **params)
{
    ++g_paramFault.initCount;
    if (TakeParamFailure(ParamFailure::INIT)) {
        return HKS_ERROR_MALLOC_FAIL;
    }
    return HksInitParamSet(params);
}

extern "C" int32_t MockAddParams(HksParamSet *set, const HksParam *params, uint32_t count)
{
    if (TakeParamFailure(ParamFailure::ADD)) {
        return HKS_ERROR_INVALID_ARGUMENT;
    }
    return HksAddParams(set, params, count);
}

extern "C" int32_t MockBuildParamSet(HksParamSet **params)
{
    if (TakeParamFailure(ParamFailure::BUILD)) {
        return HKS_ERROR_INVALID_ARGUMENT;
    }
    return HksBuildParamSet(params);
}
