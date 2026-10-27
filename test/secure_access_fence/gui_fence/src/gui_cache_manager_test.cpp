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

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "gui_cache_manager.h"

using namespace testing::ext;
using namespace OHOS::Security::SAF;

namespace UnitTest::GuiCacheManagerTest {

class GuiCacheManagerTest : public testing::Test {
public:
    void SetUp() override
    {
        GuiCacheManager::ResetForTesting();
    }

    void TearDown() override
    {
        GuiCacheManager::ResetForTesting();
    }
};

namespace {
constexpr int CONCURRENT_THREAD_COUNT = 8;
constexpr int CONCURRENT_ITERATIONS = 1000;
constexpr int32_t TEST_APP_INDEX = 0;
constexpr int32_t PERF_RECORD_COUNT = 1000;
constexpr int32_t PERF_QUERY_INDEX = 500;
constexpr int64_t CACHE_HIT_LATENCY_THRESHOLD_US = 2000; // 2ms = 2000us

// Worker function for concurrent IsDenied test: queries cache repeatedly and counts errors.
void ConcurrentIsDeniedWorker(GuiCacheManager &cache, std::atomic<int> &errorCount)
{
    for (int i = 0; i < CONCURRENT_ITERATIONS; ++i) {
        bool deniedResult = cache.IsDenied("com.app.a", TEST_APP_INDEX, "com.app.b", TEST_APP_INDEX);
        if (!deniedResult) {
            errorCount++;
        }
        bool allowedResult = cache.IsDenied("com.app.x", TEST_APP_INDEX, "com.app.y", TEST_APP_INDEX);
        if (allowedResult) {
            errorCount++;
        }
    }
}
} // namespace

/**
 * @tc.name: IsDenied_Uninitialized_001
 * @tc.desc: UT-C-01: IsDenied on uninitialized cache returns false (default ALLOW).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, IsDenied_Uninitialized_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    EXPECT_FALSE(cache.IsInitialized());
    EXPECT_FALSE(cache.IsDenied("com.app.a", 0, "com.app.b", 0));
}

/**
 * @tc.name: UpdateFromFullList_DeniedPair_001
 * @tc.desc: UT-C-02: After UpdateFromFullList with denied pairs, query pair in list returns true (DENY).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, UpdateFromFullList_DeniedPair_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    std::vector<AppPairKey> denied = {
        std::make_tuple(std::string("com.app.a"), 0, std::string("com.app.b"), 0),
    };
    cache.UpdateFromFullList(denied);
    EXPECT_TRUE(cache.IsInitialized());
    EXPECT_TRUE(cache.IsDenied("com.app.a", 0, "com.app.b", 0));
}

/**
 * @tc.name: IsDenied_NotInList_001
 * @tc.desc: UT-C-03: After UpdateFromFullList, query pair not in list returns false (ALLOW).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, IsDenied_NotInList_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    std::vector<AppPairKey> denied = {
        std::make_tuple(std::string("com.app.a"), 0, std::string("com.app.b"), 0),
    };
    cache.UpdateFromFullList(denied);
    EXPECT_FALSE(cache.IsDenied("com.app.a", 0, "com.app.c", 0));
    EXPECT_FALSE(cache.IsDenied("com.app.x", 1, "com.app.b", 0));
}

/**
 * @tc.name: UpdateFromFullList_EmptyList_001
 * @tc.desc: UT-C-04: After UpdateFromFullList with empty list, query any pair returns false (ALLOW).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, UpdateFromFullList_EmptyList_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    std::vector<AppPairKey> empty;
    cache.UpdateFromFullList(empty);
    EXPECT_TRUE(cache.IsInitialized());
    EXPECT_FALSE(cache.IsDenied("com.app.a", TEST_APP_INDEX, "com.app.b", TEST_APP_INDEX));
    EXPECT_FALSE(cache.IsDenied("com.anything", TEST_APP_INDEX, "com.other", TEST_APP_INDEX));
}

/**
 * @tc.name: UpdateFromFullList_FullReplacement_001
 * @tc.desc: UT-C-05: Second UpdateFromFullList fully replaces old data; old pairs gone, new pairs present.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, UpdateFromFullList_FullReplacement_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    // First update: deny (A->B)
    std::vector<AppPairKey> first = {
        std::make_tuple(std::string("com.app.a"), 0, std::string("com.app.b"), 0),
    };
    cache.UpdateFromFullList(first);
    EXPECT_TRUE(cache.IsDenied("com.app.a", 0, "com.app.b", 0));

    // Second update: deny (C->D) instead - old pair should be gone
    std::vector<AppPairKey> second = {
        std::make_tuple(std::string("com.app.c"), 0, std::string("com.app.d"), 0),
    };
    cache.UpdateFromFullList(second);
    EXPECT_TRUE(cache.IsInitialized());
    EXPECT_FALSE(cache.IsDenied("com.app.a", 0, "com.app.b", 0));
    EXPECT_TRUE(cache.IsDenied("com.app.c", 0, "com.app.d", 0));
}

/**
 * @tc.name: IsDenied_Concurrent_001
 * @tc.desc: UT-C-06: Concurrent IsDenied queries from multiple threads - no crash, correct results.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, IsDenied_Concurrent_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    std::vector<AppPairKey> denied = {
        std::make_tuple(std::string("com.app.a"), 0, std::string("com.app.b"), 0),
    };
    cache.UpdateFromFullList(denied);

    std::vector<std::thread> threads;
    std::atomic<int> errorCount(0);

    for (int t = 0; t < CONCURRENT_THREAD_COUNT; ++t) {
        threads.emplace_back(ConcurrentIsDeniedWorker, std::ref(cache), std::ref(errorCount));
    }
    for (auto &t : threads) {
        t.join();
    }
    EXPECT_EQ(0, errorCount.load());
}

/**
 * @tc.name: IsDenied_Performance_001
 * @tc.desc: UT-C-07/07b: Performance with 1000 denied records. First call verifies success; second call verifies < 2ms.
 * @tc.type: PERF
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, IsDenied_Performance_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    std::vector<AppPairKey> denied;
    denied.reserve(PERF_RECORD_COUNT);
    for (int i = 0; i < PERF_RECORD_COUNT; ++i) {
        denied.push_back(std::make_tuple(
            std::string("com.app.caller"), i, std::string("com.app.target"), i));
    }
    cache.UpdateFromFullList(denied);
    EXPECT_TRUE(cache.IsInitialized());

    // UT-C-07: First call - verify success only (may include first-access overhead)
    bool firstResult = cache.IsDenied("com.app.caller", PERF_QUERY_INDEX, "com.app.target", PERF_QUERY_INDEX);
    EXPECT_TRUE(firstResult);

    // UT-C-07b: Second call - verify latency < 2ms (2000us)
    auto start = std::chrono::steady_clock::now();
    bool secondResult = cache.IsDenied("com.app.caller", PERF_QUERY_INDEX, "com.app.target", PERF_QUERY_INDEX);
    auto end = std::chrono::steady_clock::now();
    auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    EXPECT_TRUE(secondResult);
    EXPECT_LT(durationUs, CACHE_HIT_LATENCY_THRESHOLD_US);
}

/**
 * @tc.name: UpdateFromFullList_MemoryObservation_001
 * @tc.desc: UT-C-08: Memory observation with 1000 records - print stats only, no assertion (200KB by design).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, UpdateFromFullList_MemoryObservation_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    constexpr int recordCount = 1000;
    std::vector<AppPairKey> denied;
    denied.reserve(recordCount);
    for (int i = 0; i < recordCount; ++i) {
        denied.push_back(std::make_tuple(
            std::string("com.app.caller.bundle.name"), i,
            std::string("com.app.target.bundle.name"), i));
    }
    cache.UpdateFromFullList(denied);
    EXPECT_TRUE(cache.IsInitialized());

    // Print memory statistics for observation.
    // 200KB is a design constraint (only denied pairs stored, expected far below 200KB).
    // No reliable portable way to measure set dynamic memory in a unit test.
    // Estimate: 1000 entries x (~60 bytes per tuple + ~32 bytes per set node) ~ 92KB.
    printf("UT-C-08: GuiCacheManager with %d denied pairs. "
           "Estimated memory: ~92KB (well below 200KB design constraint).\n",
           recordCount);
}

// ============ Refresh synchronization (BeginRefresh/EndRefresh/WaitForRefresh) ============

/**
 * @tc.name: WaitForRefresh_NotRefreshing_001
 * @tc.desc: UT-C-09: WaitForRefresh returns immediately when no refresh is in progress (early-return branch).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, WaitForRefresh_NotRefreshing_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    // After ResetForTesting, refreshing_ is false, so WaitForRefresh must return immediately.
    auto start = std::chrono::steady_clock::now();
    cache.WaitForRefresh();
    auto end = std::chrono::steady_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(durationMs, 1000); // near-instant, covers the !refreshing_ early return
}

/**
 * @tc.name: BeginEndRefresh_WaitUnblocks_001
 * @tc.desc: UT-C-10: After BeginRefresh, WaitForRefresh blocks until EndRefresh is called.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, BeginEndRefresh_WaitUnblocks_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    cache.BeginRefresh();

    std::atomic<bool> waitingStarted(false);
    std::atomic<bool> waitingFinished(false);

    std::thread waiter([&waitingStarted, &cache, &waitingFinished]() {
        waitingStarted = true;
        cache.WaitForRefresh(); // should block because refreshing_ == true
        waitingFinished = true;
    });

    // Wait for the waiter thread to start, then give it time to enter the CV wait.
    while (!waitingStarted.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Confirm the waiter is still blocked.
    EXPECT_FALSE(waitingFinished.load());

    // EndRefresh clears refreshing_ and notifies the waiter -> unblocks.
    cache.EndRefresh();
    waiter.join();
    EXPECT_TRUE(waitingFinished.load());
}

/**
 * @tc.name: EndRefresh_NotifiesAllWaiters_001
 * @tc.desc: UT-C-11: EndRefresh notifies all waiting threads (notify_all branch).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, EndRefresh_NotifiesAllWaiters_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    cache.BeginRefresh();

    int waiterCount = 4;
    std::atomic<int> finishedCount(0);
    std::vector<std::thread> waiters;
    for (int i = 0; i < waiterCount; ++i) {
        waiters.emplace_back([&cache, &finishedCount]() {
            cache.WaitForRefresh();
            finishedCount++;
        });
    }

    // Confirm all waiters are blocked.
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_EQ(finishedCount.load(), 0);

    cache.EndRefresh();
    for (auto &t : waiters) {
        t.join();
    }
    EXPECT_EQ(finishedCount.load(), waiterCount);
}

/**
 * @tc.name: EndRefresh_NoWaiter_NotifiesNoop_001
 * @tc.desc: UT-C-12: EndRefresh when no thread is waiting completes without error.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, EndRefresh_NoWaiter_NotifiesNoop_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    cache.BeginRefresh();
    // EndRefresh with no waiter: notify_all on cv with no waiters is a no-op.
    cache.EndRefresh();
    // After EndRefresh, refreshing_ is false, WaitForRefresh returns immediately.
    auto start = std::chrono::steady_clock::now();
    cache.WaitForRefresh();
    auto end = std::chrono::steady_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(durationMs, 1000);
}

/**
 * @tc.name: ResetForTesting_ClearsRefreshing_001
 * @tc.desc: UT-C-13: ResetForTesting clears the refreshing flag and notifies waiting threads.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, ResetForTesting_ClearsRefreshing_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    cache.BeginRefresh();

    std::atomic<bool> waitingFinished(false);
    std::thread waiter([&cache, &waitingFinished]() {
        cache.WaitForRefresh();
        waitingFinished = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(waitingFinished.load());

    // ResetForTesting should clear refreshing_ and notify the waiter.
    GuiCacheManager::ResetForTesting();
    waiter.join();
    EXPECT_TRUE(waitingFinished.load());

    // After reset, refreshing_ is false, WaitForRefresh returns immediately.
    auto start = std::chrono::steady_clock::now();
    cache.WaitForRefresh();
    auto end = std::chrono::steady_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(durationMs, 1000);
}

/**
 * @tc.name: BeginRefresh_Twice_WaitUnblocks_001
 * @tc.desc: UT-C-14: Repeated BeginRefresh/EndRefresh cycle works; WaitForRefresh blocks each refresh cycle.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiCacheManagerTest, BeginRefresh_Twice_WaitUnblocks_001, TestSize.Level0)
{
    auto &cache = GuiCacheManager::GetInstance();
    for (int cycle = 0; cycle < 2; ++cycle) {
        cache.BeginRefresh();
        std::atomic<bool> waitingFinished(false);
        std::thread waiter([&cache, &waitingFinished]() {
            cache.WaitForRefresh();
            waitingFinished = true;
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        EXPECT_FALSE(waitingFinished.load());
        cache.EndRefresh();
        waiter.join();
        EXPECT_TRUE(waitingFinished.load());
    }
}

} // namespace UnitTest::GuiCacheManagerTest
