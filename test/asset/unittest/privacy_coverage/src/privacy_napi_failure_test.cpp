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

#include "host_faults.h"
#include "napi_host.h"
#include "privacy_computation_napi_common.h"
#include "privacy_computation_napi_loader.h"
#include <gtest/gtest.h>
#include <new>

namespace {
using namespace testing::ext;
using namespace OHOS::Security::PrivacyComputation;
using PrivacyHost::State;

struct RegistrationFailure {
    const char *operation;
    uint32_t occurrence;
    const char *exportName = nullptr;
};

class PrivacyNapiFailureTest : public testing::Test {
public:
    void SetUp() override
    {
        PrivacyHost::Reset();
    }

    void FailAt(const char *operation, uint32_t occurrence = 1)
    {
        PrivacyHost::Reset();
        env_.exception = nullptr;
        State().failAt = operation;
        State().failOccurrence = occurrence;
    }

    napi_value Protocol(DataSetSize size, ProtocolType type)
    {
        auto value = PrivacyHost::Value(&env_);
        value->properties["dataSetSize"] = PrivacyHost::Number(&env_, static_cast<uint32_t>(size));
        value->properties["protocolType"] = PrivacyHost::Number(&env_, static_cast<uint32_t>(type));
        return value;
    }

    std::unique_ptr<PrivacyComputationBaseContext> AsyncContext()
    {
        auto context = std::unique_ptr<PrivacyComputationBaseContext>(new (std::nothrow) PrivacyComputationBaseContext);
        if (!context) {
            return nullptr;
        }
        context->parse = [](napi_env, napi_callback_info, PrivacyComputationBaseContext *) {
            return napi_ok;
        };
        context->execute = [](napi_env, void *data) {
            static_cast<PrivacyComputationBaseContext *>(data)->result = PRIVACY_COMPUTATION_SUCCESS;
        };
        context->resolve = [](napi_env env, PrivacyComputationBaseContext *, napi_value &output) {
            return napi_create_uint32(env, 1, &output);
        };
        return context;
    }

    napi_env__ env_;
};

/*
 * @tc.name: RegistrationPreservesPendingErrorAndUsesLastErrorFallback
 * @tc.desc: Exercise each enum export and module registration with NAPI errors and incomplete error metadata.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyNapiFailureTest, RegistrationPreservesPendingErrorAndUsesLastErrorFallback, TestSize.Level2)
{
    const RegistrationFailure failures[] = {
        {"create_object", 1, "ProtocolType"},
        {"create_object", 2, "DataSetSize"},
        {"create_object", 3, "HashAlg"},
        {"set_property", 1, "ProtocolType"},
        {"set_property", 2, "ProtocolType"},
        {"set_property", 3, "DataSetSize"},
        {"set_property", 4, "DataSetSize"},
        {"set_property", 5, "DataSetSize"},
        {"set_property", 6, "HashAlg"},
        {"set_property", 7, "HashAlg"},
        {"set_property", 8, "HashAlg"},
        {"set_property", 9, "HashAlg"},
        {"define_properties", 1, nullptr},
    };
    enum class ErrorInfo { PENDING, MESSAGE, NO_MESSAGE, NO_INFO };
    for (const auto &failure : failures) {
        SCOPED_TRACE(failure.operation);
        SCOPED_TRACE(failure.occurrence);
        for (auto errorInfo : {ErrorInfo::PENDING, ErrorInfo::MESSAGE, ErrorInfo::NO_MESSAGE, ErrorInfo::NO_INFO}) {
            SCOPED_TRACE(static_cast<uint32_t>(errorInfo));
            PrivacyHost::Reset();
            napi_env__ env;
            auto exports = PrivacyHost::Value(&env);
            if (errorInfo == ErrorInfo::PENDING) {
                env.exception = PrivacyHost::Value(&env);
            }
            const auto original = env.exception;
            env.omitLastError = errorInfo == ErrorInfo::NO_INFO;
            env.omitLastErrorMessage = errorInfo == ErrorInfo::NO_MESSAGE;
            // Simulate a failed structured throw so the outer registration macro consults the last error.
            env.failThrow = true;
            State().failAt = failure.operation;
            State().failOccurrence = failure.occurrence;
            auto result = PrivacyHost::RegisterExports(&env, exports);
            if (failure.exportName == nullptr) {
                EXPECT_EQ(result, nullptr);
                EXPECT_TRUE(exports->properties.empty());
            } else {
                EXPECT_EQ(result, exports);
                EXPECT_EQ(exports->properties.at(failure.exportName), nullptr);
            }
            if (errorInfo == ErrorInfo::PENDING || errorInfo == ErrorInfo::NO_INFO) {
                EXPECT_EQ(env.exception, original);
            } else {
                ASSERT_NE(env.exception, nullptr);
                if (errorInfo == ErrorInfo::NO_MESSAGE) {
                    EXPECT_EQ(env.exception->text, "empty error message");
                } else {
                    EXPECT_EQ(env.exception->text, "host boundary failure");
                }
            }
        }
    }
}

/*
 * @tc.name: ErrorConversionHonorsRuntimeFailures
 * @tc.desc: Preserve pending exceptions and propagate error construction and Promise rejection failures.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyNapiFailureTest, ErrorConversionHonorsRuntimeFailures, TestSize.Level2)
{
    constexpr auto CODE = PRIVACY_COMPUTATION_SERVICE_ERROR;
    const char *message = GetErrorMessage(CODE);
    for (const auto operation : {"create_string", "create_error", "create_int32", "set_property"}) {
        SCOPED_TRACE(operation);
        FailAt(operation);
        EXPECT_EQ(CreateJsError(&env_, CODE, message), nullptr);
        EXPECT_EQ(env_.exception, nullptr);
    }
    FailAt("create_error");
    EXPECT_EQ(ThrowJsError(&env_, CODE, message), napi_generic_failure);
    FailAt("exception_pending");
    EXPECT_EQ(ThrowJsError(&env_, CODE, message), napi_generic_failure);
    for (const auto operation : {"create_promise", "exception_pending", "create_error", "reject"}) {
        SCOPED_TRACE(operation);
        FailAt(operation);
        EXPECT_EQ(CreateRejectedPromise(&env_, CODE, message), nullptr);
    }
    FailAt("clear_exception");
    const auto previous = PrivacyHost::Value(&env_);
    env_.exception = previous;
    EXPECT_EQ(CreateRejectedPromise(&env_, CODE, message), nullptr);
    EXPECT_EQ(env_.exception, previous);
}

/*
 * @tc.name: AsyncWorkReleasesContextOnRuntimeFailure
 * @tc.desc: Verify setup, output conversion and cleanup failures with an observable Promise or error.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyNapiFailureTest, AsyncWorkReleasesContextOnRuntimeFailure, TestSize.Level2)
{
    constexpr auto NAME = "PrivacyAsyncWork";
    EXPECT_EQ(CreateAsyncWork(&env_, nullptr, nullptr, NAME), nullptr);
    for (uint32_t missingCallback = 0; missingCallback < 3; ++missingCallback) {
        FailAt("");
        auto context = AsyncContext();
        ASSERT_NE(context, nullptr);
        switch (missingCallback) {
            case 0:
                context->parse = nullptr;
                break;
            case 1:
                context->execute = nullptr;
                break;
            default:
                context->resolve = nullptr;
                break;
        }
        EXPECT_EQ(CreateAsyncWork(&env_, nullptr, std::move(context), NAME), nullptr);
        EXPECT_NE(env_.exception, nullptr);
        EXPECT_TRUE(env_.queued.empty());
    }
    FailAt("create_promise");
    EXPECT_EQ(CreateAsyncWork(&env_, nullptr, AsyncContext(), NAME), nullptr);
    EXPECT_TRUE(env_.queued.empty());
    FailAt("create_string");
    auto promise = CreateAsyncWork(&env_, nullptr, AsyncContext(), NAME);
    ASSERT_NE(promise, nullptr);
    EXPECT_TRUE(promise->rejected);
    EXPECT_TRUE(env_.queued.empty());

    FailAt("reject");
    auto context = AsyncContext();
    ASSERT_NE(context, nullptr);
    context->parse = [](napi_env, napi_callback_info, PrivacyComputationBaseContext *) {
        return napi_invalid_arg;
    };
    EXPECT_EQ(CreateAsyncWork(&env_, nullptr, std::move(context), NAME), nullptr);
    EXPECT_TRUE(env_.queued.empty());

    FailAt("delete_work");
    context = AsyncContext();
    ASSERT_NE(context, nullptr);
    context->resolve = [](napi_env, PrivacyComputationBaseContext *, napi_value &) {
        return napi_ok;
    };
    promise = CreateAsyncWork(&env_, nullptr, std::move(context), NAME);
    ASSERT_NE(promise, nullptr);
    ASSERT_EQ(env_.queued.size(), 1U);
    env_.queued.front()->complete(&env_, napi_ok, nullptr);
    EXPECT_FALSE(promise->settled);
    const uint32_t deleted = env_.deletedWorks;
    PrivacyHost::Drain(&env_);
    EXPECT_TRUE(promise->rejected);
    EXPECT_EQ(env_.deletedWorks, deleted + 1);

    FailAt("resolve");
    promise = CreateAsyncWork(&env_, nullptr, AsyncContext(), NAME);
    ASSERT_NE(promise, nullptr);
    PrivacyHost::Drain(&env_);
    EXPECT_FALSE(promise->settled);
    EXPECT_NE(env_.exception, nullptr);
    EXPECT_EQ(env_.deletedWorks, deleted + 2);
}

/*
 * @tc.name: SearchAndResultEntryPointsRejectInvalidArguments
 * @tc.desc: Distinguish argument errors from evaluation errors and process valid asynchronous context safely.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyNapiFailureTest, SearchAndResultEntryPointsRejectInvalidArguments, TestSize.Level2)
{
    const auto target = PrivacyHost::Bytes(&env_, {1});
    const auto emptyTarget = PrivacyHost::Bytes(&env_, {});
    const auto invalid = PrivacyHost::Value(&env_);
    const auto protocol = Protocol(DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL);
    auto element = PrivacyHost::Value(&env_);
    element->properties["elemKey"] = PrivacyHost::Bytes(&env_, Bytes(32, 1));
    const auto elements = PrivacyHost::Array(&env_, {element});
    const std::vector<std::vector<napi_value>> invalidSearchArgs{
        {}, {invalid, elements, protocol}, {emptyTarget, elements, protocol}, {target, elements, invalid},
    };
    for (const auto &args : invalidSearchArgs) {
        auto promise = PrivacyHost::Call(&env_, "privacySearch", args);
        ASSERT_NE(promise, nullptr);
        EXPECT_TRUE(promise->rejected);
        EXPECT_EQ(promise->result->properties.at("code")->number, PRIVACY_COMPUTATION_INVALID_ARGUMENT);
        EXPECT_TRUE(env_.queued.empty());
    }
    for (const std::vector<napi_value> &args : {std::vector<napi_value>{}, {invalid, invalid}}) {
        auto promise = PrivacyHost::Call(&env_, "getSearchResult", args);
        ASSERT_NE(promise, nullptr);
        EXPECT_TRUE(promise->rejected);
        EXPECT_EQ(promise->result->properties.at("code")->number, PRIVACY_COMPUTATION_INVALID_ARGUMENT);
        EXPECT_TRUE(env_.queued.empty());
    }
    auto promise = PrivacyHost::Call(&env_, "privacySearch", {target, elements, protocol});
    ASSERT_NE(promise, nullptr);
    ASSERT_EQ(env_.queued.size(), 1U);
    env_.queued.front()->execute(&env_, nullptr);
    EXPECT_FALSE(promise->settled);
    PrivacyHost::Drain(&env_);
    EXPECT_TRUE(promise->rejected);
    EXPECT_NE(promise->result->properties.at("code")->number, PRIVACY_COMPUTATION_INVALID_ARGUMENT);
}

/*
 * @tc.name: ClosedLoaderRejectsCallsWithoutConfiguredLibrary
 * @tc.desc: Reject a direct closed loader call in the software-only variant before symbol lookup.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyNapiFailureTest, ClosedLoaderRejectsCallsWithoutConfiguredLibrary, TestSize.Level2)
{
    EXPECT_FALSE(IsComputationLibConfigured());
    auto promise = CallComputationNapiFunc(&env_, nullptr, "GenPrivacyTarget");
    ASSERT_NE(promise, nullptr);
    ASSERT_TRUE(promise->rejected);
    EXPECT_EQ(promise->result->properties.at("code")->number, PRIVACY_COMPUTATION_UNAVAILABLE);
    EXPECT_TRUE(env_.queued.empty());
}

/*
 * @tc.name: ObjectAndArrayReadFailuresPropagate
 * @tc.desc: Verify that property, number, byte array and element readers return the runtime error.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyNapiFailureTest, ObjectAndArrayReadFailuresPropagate, TestSize.Level2)
{
    auto key = PrivacyHost::Value(&env_);
    key->properties["elemData"] = PrivacyHost::Bytes(&env_, Bytes(32, 1));
    key->properties["hashAlg"] = PrivacyHost::Number(&env_, HASH_ALG_NONE);
    const RegistrationFailure reads[] = {
        {"get_property", 1}, {"typeof", 2}, {"is_typedarray", 1}, {"typedarray_info", 1},
        {"get_property", 2}, {"typeof", 3}, {"typeof", 4}, {"get_double", 1},
    };
    for (const auto &failure : reads) {
        SCOPED_TRACE(failure.operation);
        SCOPED_TRACE(failure.occurrence);
        FailAt(failure.operation, failure.occurrence);
        TargetInput target;
        EXPECT_EQ(NapiGetProperty(&env_, key, target), napi_generic_failure);
        EXPECT_NE(env_.exception, nullptr);
    }

    const auto pir = Protocol(DataSetSize::SIZE_128, ProtocolType::PIR_PROTOCOL);
    for (uint32_t occurrence : {1U, 2U}) {
        FailAt("get_double", occurrence);
        PrivacyProtocol protocol;
        EXPECT_EQ(NapiGetProperty(&env_, pir, protocol), napi_generic_failure);
    }
    FailAt("");
    PrivacyProtocol protocol;
    EXPECT_EQ(NapiGetProperty(&env_, pir, protocol), napi_ok);
    EXPECT_EQ(protocol.dataSetSize, DataSetSize::SIZE_128);
    EXPECT_EQ(protocol.protocolType, ProtocolType::PIR_PROTOCOL);
    auto unsupported = Protocol(DataSetSize::SIZE_128, ProtocolType::PSI_PROTOCOL);
    EXPECT_EQ(NapiGetProperty(&env_, unsupported, protocol), napi_invalid_arg);

    const auto array = PrivacyHost::Array(&env_, {PrivacyHost::Bytes(&env_, {1})});
    for (const auto operation : {"is_array", "array_length", "get_element"}) {
        SCOPED_TRACE(operation);
        FailAt(operation);
        std::vector<Element> elements;
        EXPECT_EQ(NapiGetProperty(&env_, array, elements, ProtocolType::PSI_PROTOCOL), napi_generic_failure);
        EXPECT_TRUE(elements.empty());
    }
    FailAt("");
    std::vector<Element> elements;
    EXPECT_EQ(NapiGetProperty(&env_, PrivacyHost::Array(&env_, {}), elements, ProtocolType::PSI_PROTOCOL),
        napi_invalid_arg);

    auto result = PrivacyHost::Value(&env_);
    result->properties["resultCipherText"] = array;
    result->properties["valueCipherText"] = array;
    const RegistrationFailure resultReads[] = {
        {"get_property", 1}, {"array_length", 1}, {"get_element", 1}, {"is_typedarray", 1}, {"get_property", 2},
        {"array_length", 2},
    };
    for (const auto &failure : resultReads) {
        SCOPED_TRACE(failure.operation);
        SCOPED_TRACE(failure.occurrence);
        FailAt(failure.operation, failure.occurrence);
        PrivacySearchResult output;
        EXPECT_EQ(NapiGetProperty(&env_, result, output, ProtocolType::PIR_PROTOCOL), napi_generic_failure);
    }
}

/*
 * @tc.name: ResultConversionReturnsRuntimeFailure
 * @tc.desc: Verify that array, typed array, element and property creation errors stop result conversion.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyNapiFailureTest, ResultConversionReturnsRuntimeFailure, TestSize.Level2)
{
    const PrivacySearchResult batches{{Bytes{1}}, std::vector<Bytes>{{2}}};
    const SearchResult result{true, std::vector<Bytes>{{2}}};
    const RegistrationFailure writes[] = {
        {"create_object", 1}, {"create_array", 1}, {"create_array", 2}, {"create_buffer", 1},
        {"create_typedarray", 1}, {"set_element", 1}, {"set_property", 1}, {"set_property", 2},
    };
    for (const auto &failure : writes) {
        SCOPED_TRACE(failure.operation);
        SCOPED_TRACE(failure.occurrence);
        FailAt(failure.operation, failure.occurrence);
        napi_value output = nullptr;
        EXPECT_EQ(NapiSetProperty(&env_, batches, output), napi_generic_failure);
        EXPECT_NE(env_.exception, nullptr);
    }
    for (const auto operation : {"create_object", "get_boolean", "create_array", "set_property"}) {
        SCOPED_TRACE(operation);
        FailAt(operation);
        napi_value output = nullptr;
        EXPECT_EQ(NapiSetProperty(&env_, result, output), napi_generic_failure);
        EXPECT_NE(env_.exception, nullptr);
    }
    FailAt("set_property", 2);
    napi_value output = nullptr;
    EXPECT_EQ(NapiSetProperty(&env_, result, output), napi_generic_failure);

    FailAt("");
    ASSERT_EQ(NapiSetProperty(&env_, Bytes{}, output), napi_ok);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(output->count, 0U);
}
} // namespace
