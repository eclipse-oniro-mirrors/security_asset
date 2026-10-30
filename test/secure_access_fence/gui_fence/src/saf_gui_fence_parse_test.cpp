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

#include <functional>
#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <vector>

#include "errors.h"
#include "gui_fence_inner_helper.h"
#include "saf_result_code.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Security::SAF;

namespace UnitTest::GuiFenceParseTest {

namespace {
constexpr int32_t INDEX_ZERO = 0;
constexpr int32_t INDEX_ONE = 1;
constexpr int32_t INDEX_TWO = 2;
constexpr int32_t NEGATIVE_INDEX_ONE = -1;
constexpr int32_t NEGATIVE_INDEX_TWO = -2;
constexpr int32_t LARGE_CALLER_INDEX = 99999;
constexpr int32_t LARGE_TARGET_INDEX = 123456;

// Multiple-pairs loop case: distinct indices so each parsed slot is verifiable.
constexpr int32_t MULTIPLE_PAIR_SECOND_CALLER_IDX = 1;
constexpr int32_t MULTIPLE_PAIR_THIRD_CALLER_IDX = 10;
constexpr int32_t MULTIPLE_PAIR_THIRD_TARGET_IDX = 20;

// BundleWithDots case: distinct positive indices paired with bundle names that
// contain dots, chosen only to differ from the trivial 0/1 used elsewhere.
constexpr int32_t BUNDLE_WITH_DOTS_CALLER_IDX = 5;
constexpr int32_t BUNDLE_WITH_DOTS_TARGET_IDX = 6;

// HandleIpcError generic non-dead error used to confirm pass-through of an
// arbitrary non-dead, non-success value.
constexpr int32_t GENERIC_IPC_ERROR = -1;

// ERR_REMOTE_DEAD mirrors the value used by the OpenHarmony IPC layer for a
// dead remote object. It is a literal in gui_fence_inner_helper.cpp (kept
// self-contained there); the test names the same value to avoid a magic
// number and to keep the assertion self-documenting.
constexpr int32_t ERR_REMOTE_DEAD = 29189;

// Number of valid pairs in the MultiplePairs case (must match the vector below).
constexpr size_t MULTIPLE_PAIR_COUNT = 3;
} // namespace

class GuiFenceParseTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

// ================== ParseDeniedPair tests =================
// ParseDeniedPair parses "callerBundle:callerIdx:targetBundle:targetIdx" into
// AppPairKey = std::tuple<std::string, int32_t, std::string, int32_t>.
// Returns false when the format is invalid (fewer than three colons, or the
// index fields cannot be parsed as integers via std::stoi).

/**
 * @tc.name: ParseDeniedPair_Valid_001
 * @tc.desc: Valid full-format pair -> true, all four fields parsed correctly.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_Valid_001, TestSize.Level0)
{
    AppPairKey key;
    bool ret = ParseDeniedPair("com.app.a:0:com.app.b:1", key);
    EXPECT_TRUE(ret);
    EXPECT_EQ(std::get<0>(key), "com.app.a");
    EXPECT_EQ(std::get<1>(key), INDEX_ZERO);
    EXPECT_EQ(std::get<2>(key), "com.app.b");
    EXPECT_EQ(std::get<3>(key), INDEX_ONE);
}

/**
 * @tc.name: ParseDeniedPair_NoColon_001
 * @tc.desc: No colon at all (pos1 == npos) -> false.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_NoColon_001, TestSize.Level0)
{
    AppPairKey key;
    bool ret = ParseDeniedPair("invalidNoColon", key);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: ParseDeniedPair_EmptyString_001
 * @tc.desc: Empty string (pos1 == npos) -> false.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_EmptyString_001, TestSize.Level0)
{
    AppPairKey key;
    bool ret = ParseDeniedPair("", key);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: ParseDeniedPair_OneColon_001
 * @tc.desc: Only one colon (pos2 == npos) -> false.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_OneColon_001, TestSize.Level0)
{
    AppPairKey key;
    bool ret = ParseDeniedPair("com.app.a:0", key);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: ParseDeniedPair_TwoColons_001
 * @tc.desc: Only two colons, missing target index (pos3 == npos) -> false.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_TwoColons_001, TestSize.Level0)
{
    AppPairKey key;
    bool ret = ParseDeniedPair("com.app.a:0:com.app.b", key);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: ParseDeniedPair_IndexZero_001
 * @tc.desc: Both indices zero -> true, indices are 0.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_IndexZero_001, TestSize.Level0)
{
    AppPairKey key;
    bool ret = ParseDeniedPair("a:0:b:0", key);
    EXPECT_TRUE(ret);
    EXPECT_EQ(std::get<1>(key), INDEX_ZERO);
    EXPECT_EQ(std::get<3>(key), INDEX_ZERO);
}

/**
 * @tc.name: ParseDeniedPair_NegativeIndex_001
 * @tc.desc: Negative caller/target indices -> true (std::stoi accepts sign).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_NegativeIndex_001, TestSize.Level0)
{
    AppPairKey key;
    bool ret = ParseDeniedPair("com.app.a:-1:com.app.b:-2", key);
    EXPECT_TRUE(ret);
    EXPECT_EQ(std::get<1>(key), NEGATIVE_INDEX_ONE);
    EXPECT_EQ(std::get<3>(key), NEGATIVE_INDEX_TWO);
}

/**
 * @tc.name: ParseDeniedPair_LargeIndex_001
 * @tc.desc: Large indices -> true, values preserved.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_LargeIndex_001, TestSize.Level0)
{
    AppPairKey key;
    std::string pair = std::string("com.app.a:") + std::to_string(LARGE_CALLER_INDEX) +
        ":com.app.b:" + std::to_string(LARGE_TARGET_INDEX);
    bool ret = ParseDeniedPair(pair, key);
    EXPECT_TRUE(ret);
    EXPECT_EQ(std::get<1>(key), LARGE_CALLER_INDEX);
    EXPECT_EQ(std::get<3>(key), LARGE_TARGET_INDEX);
}

/**
 * @tc.name: ParseDeniedPair_SingleCharBundle_001
 * @tc.desc: Single-character bundle names -> true, preserved exactly.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_SingleCharBundle_001, TestSize.Level0)
{
    AppPairKey key;
    bool ret = ParseDeniedPair("x:1:y:2", key);
    EXPECT_TRUE(ret);
    EXPECT_EQ(std::get<0>(key), "x");
    EXPECT_EQ(std::get<2>(key), "y");
    EXPECT_EQ(std::get<1>(key), INDEX_ONE);
    EXPECT_EQ(std::get<3>(key), INDEX_TWO);
}

/**
 * @tc.name: ParseDeniedPair_BundleWithDots_001
 * @tc.desc: Bundle names containing dots (no colon inside) -> true.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_BundleWithDots_001, TestSize.Level0)
{
    AppPairKey key;
    std::string pair = std::string("com.app.a.sub:") + std::to_string(BUNDLE_WITH_DOTS_CALLER_IDX) +
        ":com.app.b.sub:" + std::to_string(BUNDLE_WITH_DOTS_TARGET_IDX);
    bool ret = ParseDeniedPair(pair, key);
    EXPECT_TRUE(ret);
    EXPECT_EQ(std::get<0>(key), "com.app.a.sub");
    EXPECT_EQ(std::get<2>(key), "com.app.b.sub");
    EXPECT_EQ(std::get<1>(key), BUNDLE_WITH_DOTS_CALLER_IDX);
    EXPECT_EQ(std::get<3>(key), BUNDLE_WITH_DOTS_TARGET_IDX);
}

/**
 * @tc.name: ParseDeniedPair_EmptyCallerBundle_001
 * @tc.desc: Empty caller bundle name (":0:com.b:0") -> true; ParseDeniedPair
 *          does not validate emptiness, only the colon-delimited format.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_EmptyCallerBundle_001, TestSize.Level0)
{
    AppPairKey key;
    bool ret = ParseDeniedPair(":0:com.app.b:0", key);
    EXPECT_TRUE(ret);
    EXPECT_EQ(std::get<0>(key), "");
    EXPECT_EQ(std::get<1>(key), INDEX_ZERO);
    EXPECT_EQ(std::get<2>(key), "com.app.b");
    EXPECT_EQ(std::get<3>(key), INDEX_ZERO);
}

/**
 * @tc.name: ParseDeniedPair_MultiplePairs_001
 * @tc.desc: Several valid pairs parsed in a loop (mirrors RefreshDeniedPairs)
 *          -> all true with correct values.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, ParseDeniedPair_MultiplePairs_001, TestSize.Level0)
{
    // Build pair strings from named constants so no magic numbers appear inline.
    std::vector<std::string> pairs = {
        std::string("com.app.a:") + std::to_string(INDEX_ZERO) + ":com.app.b:" + std::to_string(INDEX_ZERO),
        std::string("com.app.c:") + std::to_string(MULTIPLE_PAIR_SECOND_CALLER_IDX) + ":com.app.d:" +
            std::to_string(INDEX_TWO),
        std::string("com.app.e:") + std::to_string(MULTIPLE_PAIR_THIRD_CALLER_IDX) + ":com.app.f:" +
            std::to_string(MULTIPLE_PAIR_THIRD_TARGET_IDX),
    };
    std::vector<AppPairKey> keys;
    for (const auto &pair : pairs) {
        AppPairKey key;
        if (ParseDeniedPair(pair, key)) {
            keys.push_back(key);
        }
    }
    EXPECT_EQ(keys.size(), MULTIPLE_PAIR_COUNT);
    EXPECT_EQ(std::get<0>(keys[INDEX_ZERO]), "com.app.a");
    EXPECT_EQ(std::get<1>(keys[INDEX_ONE]), MULTIPLE_PAIR_SECOND_CALLER_IDX);
    EXPECT_EQ(std::get<2>(keys[INDEX_TWO]), "com.app.f");
    EXPECT_EQ(std::get<3>(keys[INDEX_TWO]), MULTIPLE_PAIR_THIRD_TARGET_IDX);
}

// ================== HandleIpcError tests =================
// HandleIpcError is the helper invoked by all three component-click callbacks
// (and by RefreshDeniedPairs / AsyncStoreOperationRecord). It maps dead-object
// IPC results to SAF_ERR_SERVICE_UNAVAILABLE and otherwise returns the call
// result. The proxy reference is forwarded, not dereferenced, so a null proxy
// is sufficient to exercise every branch.

/**
 * @tc.name: HandleIpcError_NullCall_001
 * @tc.desc: call == nullptr -> SAF_ERR_SERVICE_UNAVAILABLE.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, HandleIpcError_NullCall_001, TestSize.Level0)
{
    sptr<ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = nullptr;
    int32_t ret = HandleIpcError(proxy, call);
    EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
}

/**
 * @tc.name: HandleIpcError_DeadObject_001
 * @tc.desc: call returns ERR_DEAD_OBJECT -> SAF_ERR_SERVICE_UNAVAILABLE.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, HandleIpcError_DeadObject_001, TestSize.Level0)
{
    sptr<ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return ERR_DEAD_OBJECT; };
    int32_t ret = HandleIpcError(proxy, call);
    EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
}

/**
 * @tc.name: HandleIpcError_RemoteDeadValue_001
 * @tc.desc: call returns ERR_REMOTE_DEAD (29189) -> SAF_ERR_SERVICE_UNAVAILABLE.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, HandleIpcError_RemoteDeadValue_001, TestSize.Level0)
{
    sptr<ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return ERR_REMOTE_DEAD; };
    int32_t ret = HandleIpcError(proxy, call);
    EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
}

/**
 * @tc.name: HandleIpcError_OtherError_001
 * @tc.desc: call returns a non-dead error (SAF_ERR_IPC_ERROR) -> returned as-is.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, HandleIpcError_OtherError_001, TestSize.Level0)
{
    sptr<ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return SAF_ERR_IPC_ERROR; };
    int32_t ret = HandleIpcError(proxy, call);
    EXPECT_EQ(ret, SAF_ERR_IPC_ERROR);
}

/**
 * @tc.name: HandleIpcError_ServiceStopping_001
 * @tc.desc: call returns SAF_ERR_SERVICE_IS_STOPPING -> returned as-is (the gui
 *          inner-API variant does not retry on this code).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, HandleIpcError_ServiceStopping_001, TestSize.Level0)
{
    sptr<ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return SAF_ERR_SERVICE_IS_STOPPING; };
    int32_t ret = HandleIpcError(proxy, call);
    EXPECT_EQ(ret, SAF_ERR_SERVICE_IS_STOPPING);
}

/**
 * @tc.name: HandleIpcError_NegativeOne_001
 * @tc.desc: call returns a generic non-dead error value -> returned as-is.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, HandleIpcError_NegativeOne_001, TestSize.Level0)
{
    sptr<ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return GENERIC_IPC_ERROR; };
    int32_t ret = HandleIpcError(proxy, call);
    EXPECT_EQ(ret, GENERIC_IPC_ERROR);
}

/**
 * @tc.name: HandleIpcError_Success_001
 * @tc.desc: call returns SAF_SUCCESS -> SAF_SUCCESS.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiFenceParseTest, HandleIpcError_Success_001, TestSize.Level0)
{
    sptr<ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return SAF_SUCCESS; };
    int32_t ret = HandleIpcError(proxy, call);
    EXPECT_EQ(ret, SAF_SUCCESS);
}

} // namespace UnitTest::GuiFenceParseTest
