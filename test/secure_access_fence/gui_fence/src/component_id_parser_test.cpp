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

#include <gtest/gtest.h>
#include <string>

#include "secure_access_fence_type.h"
#include "component_id_parser.h"

using namespace testing::ext;
using namespace OHOS::Security::SAF;

namespace UnitTest::ComponentIdParserTest {

class ComponentIdParserTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}

protected:
    AppInfo MakeAppInfo(const std::string &bundleName, int32_t index)
    {
        AppInfo app;
        app.bundleName = bundleName;
        app.index = index;
        return app;
    }
};

// ================== SplitComponentId tests =================
// SplitComponentId decodes the componentId JSON reported by the click executor:
// {"bundleName":"com.x.y","index":"0",<optional extra fields>}. The app
// identity is extracted into appInfo, then the two identity fields are removed
// from the object, so residualComponentId carries only the remaining
// component descriptor; a componentId without extra fields splits to "{}".
// The "index" field accepts both the JSON string and JSON number forms; any
// other shape must be rejected (with both outputs reset) so the napi layer
// delivers code=24010000 with the raw string and an empty appInfo instead.

/**
 * @tc.name: SplitComponentId_Valid_001
 * @tc.desc: Identity-only componentId -> parsed, residual is "{}".
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Valid_001, TestSize.Level0)
{
    AppInfo appInfo = MakeAppInfo("stale", 99); // pre-filled to observe overwrite
    std::string residual = "stale";            // pre-filled to observe overwrite
    EXPECT_TRUE(SplitComponentId(
        "{\"bundleName\":\"com.example.myapplication\",\"index\":\"0\"}", appInfo, residual));
    EXPECT_EQ(appInfo.bundleName, "com.example.myapplication");
    EXPECT_EQ(appInfo.index, 0);
    EXPECT_EQ(residual, "{}");
}

/**
 * @tc.name: SplitComponentId_Valid_002
 * @tc.desc: String index with non-zero value -> parsed, residual is "{}".
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Valid_002, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_TRUE(SplitComponentId("{\"bundleName\":\"com.app.b\",\"index\":\"3\"}", appInfo, residual));
    EXPECT_EQ(appInfo.bundleName, "com.app.b");
    EXPECT_EQ(appInfo.index, 3);
    EXPECT_EQ(residual, "{}");
}

/**
 * @tc.name: SplitComponentId_Valid_003
 * @tc.desc: JSON number index -> parsed, residual is "{}".
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Valid_003, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_TRUE(SplitComponentId("{\"bundleName\":\"com.app.c\",\"index\":7}", appInfo, residual));
    EXPECT_EQ(appInfo.bundleName, "com.app.c");
    EXPECT_EQ(appInfo.index, 7);
    EXPECT_EQ(residual, "{}");
}

/**
 * @tc.name: SplitComponentId_Valid_004
 * @tc.desc: Negative index in both forms -> parsed, residual is "{}".
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Valid_004, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_TRUE(SplitComponentId("{\"bundleName\":\"com.app.d\",\"index\":\"-1\"}", appInfo, residual));
    EXPECT_EQ(appInfo.index, -1);
    EXPECT_EQ(residual, "{}");
    EXPECT_TRUE(SplitComponentId("{\"bundleName\":\"com.app.d\",\"index\":-2}", appInfo, residual));
    EXPECT_EQ(appInfo.index, -2);
    EXPECT_EQ(residual, "{}");
}

/**
 * @tc.name: SplitComponentId_Valid_005
 * @tc.desc: Extra fields stay in the residual after the identity is stripped.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Valid_005, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_TRUE(SplitComponentId(
        "{\"bundleName\":\"com.app.e\",\"index\":\"0\",\"other\":\"ignored\"}", appInfo, residual));
    EXPECT_EQ(appInfo.bundleName, "com.app.e");
    EXPECT_EQ(appInfo.index, 0);
    EXPECT_EQ(residual, "{\"other\":\"ignored\"}");
}

/**
 * @tc.name: SplitComponentId_Valid_006
 * @tc.desc: Multiple extra fields keep their original order in the residual.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Valid_006, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_TRUE(SplitComponentId(
        "{\"bundleName\":\"com.app.f\",\"index\":\"0\",\"x\":\"1\",\"y\":\"2\",\"flag\":true}",
        appInfo, residual));
    EXPECT_EQ(appInfo.bundleName, "com.app.f");
    EXPECT_EQ(appInfo.index, 0);
    EXPECT_EQ(residual, "{\"x\":\"1\",\"y\":\"2\",\"flag\":true}");
}

/**
 * @tc.name: SplitComponentId_Valid_007
 * @tc.desc: Identity fields in the middle of the object -> neighbours survive, order kept.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Valid_007, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_TRUE(SplitComponentId(
        "{\"pre\":\"a\",\"bundleName\":\"com.app.g\",\"index\":\"0\",\"post\":\"c\"}", appInfo, residual));
    EXPECT_EQ(appInfo.bundleName, "com.app.g");
    EXPECT_EQ(appInfo.index, 0);
    EXPECT_EQ(residual, "{\"pre\":\"a\",\"post\":\"c\"}");
}

/**
 * @tc.name: SplitComponentId_Invalid_001
 * @tc.desc: Empty componentId -> rejected, both outputs reset.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_001, TestSize.Level0)
{
    AppInfo appInfo = MakeAppInfo("stale", 99);
    std::string residual = "stale";
    EXPECT_FALSE(SplitComponentId("", appInfo, residual));
    EXPECT_TRUE(appInfo.bundleName.empty());
    EXPECT_EQ(appInfo.index, 0);
    EXPECT_TRUE(residual.empty());
}

/**
 * @tc.name: SplitComponentId_Invalid_002
 * @tc.desc: Not a JSON document -> rejected, both outputs reset.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_002, TestSize.Level0)
{
    AppInfo appInfo = MakeAppInfo("stale", 99);
    std::string residual = "stale";
    EXPECT_FALSE(SplitComponentId("not a json", appInfo, residual));
    EXPECT_TRUE(appInfo.bundleName.empty());
    EXPECT_EQ(appInfo.index, 0);
    EXPECT_TRUE(residual.empty());
}

/**
 * @tc.name: SplitComponentId_Invalid_003
 * @tc.desc: Missing bundleName -> rejected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_003, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_FALSE(SplitComponentId("{\"index\":\"0\"}", appInfo, residual));
    EXPECT_TRUE(residual.empty());
}

/**
 * @tc.name: SplitComponentId_Invalid_004
 * @tc.desc: Empty bundleName -> rejected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_004, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_FALSE(SplitComponentId("{\"bundleName\":\"\",\"index\":\"0\"}", appInfo, residual));
    EXPECT_TRUE(residual.empty());
}

/**
 * @tc.name: SplitComponentId_Invalid_005
 * @tc.desc: bundleName is not a JSON string -> rejected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_005, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_FALSE(SplitComponentId("{\"bundleName\":123,\"index\":\"0\"}", appInfo, residual));
    EXPECT_TRUE(residual.empty());
}

/**
 * @tc.name: SplitComponentId_Invalid_006
 * @tc.desc: Missing index -> rejected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_006, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_FALSE(SplitComponentId("{\"bundleName\":\"com.app.f\"}", appInfo, residual));
    EXPECT_TRUE(residual.empty());
}

/**
 * @tc.name: SplitComponentId_Invalid_007
 * @tc.desc: index is a non-numeric string -> rejected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_007, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_FALSE(SplitComponentId("{\"bundleName\":\"com.app.g\",\"index\":\"abc\"}", appInfo, residual));
    EXPECT_TRUE(residual.empty());
}

/**
 * @tc.name: SplitComponentId_Invalid_008
 * @tc.desc: index is neither string nor number -> rejected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_008, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_FALSE(SplitComponentId("{\"bundleName\":\"com.app.h\",\"index\":true}", appInfo, residual));
    EXPECT_TRUE(residual.empty());
}

/**
 * @tc.name: SplitComponentId_Invalid_009
 * @tc.desc: index string with trailing garbage -> rejected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_009, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_FALSE(SplitComponentId("{\"bundleName\":\"com.app.i\",\"index\":\"0x1\"}", appInfo, residual));
    EXPECT_TRUE(residual.empty());
}

/**
 * @tc.name: SplitComponentId_Invalid_010
 * @tc.desc: JSON number index out of int32 range -> rejected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_010, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_FALSE(SplitComponentId(
        "{\"bundleName\":\"com.app.j\",\"index\":2147483648}", appInfo, residual));
    EXPECT_TRUE(residual.empty());
}

/**
 * @tc.name: SplitComponentId_Invalid_011
 * @tc.desc: String index out of int32 range -> rejected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_011, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_FALSE(SplitComponentId(
        "{\"bundleName\":\"com.app.k\",\"index\":\"99999999999999999999\"}", appInfo, residual));
    EXPECT_TRUE(residual.empty());
}

/**
 * @tc.name: SplitComponentId_Invalid_012
 * @tc.desc: JSON array instead of object -> rejected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ComponentIdParserTest, SplitComponentId_Invalid_012, TestSize.Level0)
{
    AppInfo appInfo;
    std::string residual;
    EXPECT_FALSE(SplitComponentId("[\"com.app.l\",\"0\"]", appInfo, residual));
    EXPECT_TRUE(residual.empty());
}

} // namespace UnitTest::ComponentIdParserTest
