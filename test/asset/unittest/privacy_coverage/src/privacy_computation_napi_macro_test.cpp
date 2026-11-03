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

// Compile-time regression checks for the real macros; no NAPI runtime is mocked.
#include "privacy_computation_napi_common.h"

namespace {
using namespace OHOS::Security::PrivacyComputation;

constexpr napi_status CountStatus(int32_t &calls, napi_status status)
{
    ++calls;
    return status;
}

constexpr napi_status RunStatusGuard(int32_t &calls, int32_t &continued, napi_status status)
{
    IF_ERR_RETURN(CountStatus(calls, status));
    ++continued;
    return napi_ok;
}

constexpr bool StatusGuardEvaluatesOnce(napi_status status)
{
    int32_t calls = 0;
    int32_t continued = 0;
    const auto result = RunStatusGuard(calls, continued, status);
    return result == status && calls == 1 && continued == (status == napi_ok ? 1 : 0);
}

constexpr int32_t RunBooleanGuard(int32_t &calls, int32_t &continued, bool pass)
{
    IF_FALSE_RETURN((++calls, pass), 7);
    ++continued;
    return 0;
}

constexpr bool BooleanGuardEvaluatesOnce(bool pass)
{
    int32_t calls = 0;
    int32_t continued = 0;
    const int32_t result = RunBooleanGuard(calls, continued, pass);
    return calls == 1 && result == (pass ? 0 : 7) && continued == (pass ? 1 : 0);
}

constexpr int32_t CheckThrowCondition()
{
    int32_t calls = 0;
    // The false guard is constexpr: it must evaluate the condition exactly once,
    // without executing the non-constexpr JS throw helper.
    NAPI_THROW_BASE(nullptr, ++calls == 2, -1, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "test");
    return calls;
}

constexpr int32_t CheckIfElseBinding(bool firstBranch)
{
    if (firstBranch)
        IF_FALSE_RETURN(false, 7);
    else
        return 9;
    return 0;
}

static_assert(StatusGuardEvaluatesOnce(napi_ok), "Success must evaluate once and continue");
static_assert(StatusGuardEvaluatesOnce(napi_generic_failure), "Error must evaluate once and stop");
static_assert(BooleanGuardEvaluatesOnce(true), "True guard must continue");
static_assert(BooleanGuardEvaluatesOnce(false), "False guard must return once");
static_assert(CheckThrowCondition() == 1, "Throw guard condition must evaluate once");
static_assert(CheckIfElseBinding(true) == 7 && CheckIfElseBinding(false) == 9, "Macros must not capture else");
} // namespace
