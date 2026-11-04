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
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>

#include "saf_gui_thread_pool.h"
#include "saf_result_code.h"

using namespace testing::ext;
using namespace OHOS::Security::SAF;

namespace UnitTest::SafGuiThreadPoolTest {

namespace {
// Shared atomic flag carried by value (via shared_ptr) into task lambdas so
// that lambdas can safely escape the declaring function without capturing a
// reference to a local (G.RES.05-CPP).
using FlagPtr = std::shared_ptr<std::atomic<bool>>;

// Poll interval used when waiting for an atomic flag to flip in the test main
// thread; kept small so the overall test stays fast.
constexpr int32_t POLL_INTERVAL_MS = 5;
// Upper bound on how long to wait for a worker to enter a task. Generous
// relative to TASK_HOLD_MS so the test is stable on loaded CI machines.
constexpr int32_t TASK_ENTER_TIMEOUT_MS = 500;
// Number of tasks to enqueue for the concurrent-stress case; small enough to
// stay fast but large enough to exercise repeated AddTask/execution cycles.
constexpr uint32_t CONCURRENT_TASK_COUNT = 20;
// Number of tasks queued behind a blocked worker; small fixed value chosen so
// the queued-while-busy path is exercised without filling the whole queue.
constexpr uint32_t QUEUED_TASK_COUNT = 5;
// Tolerance window used when checking that a task finished within an expected
// time after being submitted to a running pool.
constexpr int32_t TASK_DONE_TOLERANCE_MS = 500;
} // namespace

class SafGuiThreadPoolTest : public testing::Test {
public:
    void SetUp() override
    {
        auto pool = SafGuiThreadPool::GetInstance();
        if (pool != nullptr) {
            // Ensure each case starts from a clean, stopped state so that the
            // isRunning_ / pool_ branches are exercised predictably.
            pool->StopPool();
        }
    }

    void TearDown() override
    {
        auto pool = SafGuiThreadPool::GetInstance();
        if (pool != nullptr) {
            pool->StopPool();
        }
    }

protected:
    // Allocate a new shared atomic flag initialised to false.
    static FlagPtr MakeFlag()
    {
        return std::make_shared<std::atomic<bool>>(false);
    }

    // Wait until `flag` becomes true, polling at POLL_INTERVAL_MS, up to
    // `timeoutMs`. Returns true if the flag flipped in time.
    static bool WaitForFlag(const FlagPtr &flag, int32_t timeoutMs)
    {
        int32_t waited = 0;
        while (!flag->load() && waited < timeoutMs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
            waited += POLL_INTERVAL_MS;
        }
        return flag->load();
    }

    // Build a task that blocks until `release` is set, used to keep the single
    // worker thread busy so the queue depth can be controlled deterministically.
    // The shared_ptr is captured by value so the returned lambda is self-owned.
    static std::function<void()> MakeBlockingTask(const FlagPtr &entered, const FlagPtr &release)
    {
        return [entered, release]() {
            entered->store(true);
            while (!release->load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
            }
        };
    }

    // Build a simple task that flips `done` to true once it runs. The
    // shared_ptr is captured by value so the returned lambda is self-owned.
    static std::function<void()> MakeSimpleTask(const FlagPtr &done)
    {
        return [done]() { done->store(true); };
    }
};

// ============ IsRunning ============

/**
 * @tc.name: IsRunning_InitiallyFalse_001
 * @tc.desc: UT-TP-01: IsRunning returns false before StartPool is called.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, IsRunning_InitiallyFalse_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    EXPECT_FALSE(pool->IsRunning());
}

/**
 * @tc.name: IsRunning_TrueAfterStart_001
 * @tc.desc: UT-TP-02: IsRunning returns true after a successful StartPool.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, IsRunning_TrueAfterStart_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    ASSERT_EQ(pool->StartPool(), SAF_SUCCESS);
    EXPECT_TRUE(pool->IsRunning());
}

// ============ StartPool ============

/**
 * @tc.name: StartPool_StartSuccess_001
 * @tc.desc: UT-TP-03: StartPool on a fresh pool returns SAF_SUCCESS and creates the worker.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, StartPool_StartSuccess_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    EXPECT_EQ(pool->StartPool(), SAF_SUCCESS);
    EXPECT_TRUE(pool->IsRunning());

    // A submitted task must actually execute, proving the worker was started.
    auto done = MakeFlag();
    ASSERT_EQ(pool->AddTask(MakeSimpleTask(done)), SAF_SUCCESS);
    EXPECT_TRUE(WaitForFlag(done, TASK_DONE_TOLERANCE_MS));
}

/**
 * @tc.name: StartPool_AlreadyRunning_001
 * @tc.desc: UT-TP-04: StartPool called twice in a row returns SAF_SUCCESS without re-creating the pool.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, StartPool_AlreadyRunning_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    ASSERT_EQ(pool->StartPool(), SAF_SUCCESS);

    // Second call must hit the isRunning_ early-return branch and stay SUCCESS.
    EXPECT_EQ(pool->StartPool(), SAF_SUCCESS);
    EXPECT_TRUE(pool->IsRunning());
}

/**
 * @tc.name: StartPool_ReuseAfterStop_001
 * @tc.desc: UT-TP-05: StartPool after StopPool reuses the existing pool_ object and returns SAF_SUCCESS.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, StartPool_ReuseAfterStop_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    ASSERT_EQ(pool->StartPool(), SAF_SUCCESS);
    ASSERT_EQ(pool->StopPool(), SAF_SUCCESS);
    EXPECT_FALSE(pool->IsRunning());

    // After Stop, pool_ is non-null but isRunning_ is false, so StartPool must
    // take the "reuse existing" branch and flip isRunning_ back to true.
    EXPECT_EQ(pool->StartPool(), SAF_SUCCESS);
    EXPECT_TRUE(pool->IsRunning());

    // Confirm the reused pool actually executes a freshly submitted task.
    auto done = MakeFlag();
    ASSERT_EQ(pool->AddTask(MakeSimpleTask(done)), SAF_SUCCESS);
    EXPECT_TRUE(WaitForFlag(done, TASK_DONE_TOLERANCE_MS));
}

// ============ AddTask ============

/**
 * @tc.name: AddTask_NotRunning_001
 * @tc.desc: UT-TP-06: AddTask before StartPool returns SAF_ERR_THREAD_POOL_NOT_RUNNING.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, AddTask_NotRunning_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    EXPECT_FALSE(pool->IsRunning());

    auto done = MakeFlag();
    EXPECT_EQ(pool->AddTask(MakeSimpleTask(done)), SAF_ERR_THREAD_POOL_NOT_RUNNING);
    // Task must not have run.
    EXPECT_FALSE(done->load());
}

/**
 * @tc.name: AddTask_QueuesWhileWorkerBusy_001
 * @tc.desc: UT-TP-07: AddTask on a running pool with a busy worker queues tasks; they run once the worker is released.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, AddTask_QueuesWhileWorkerBusy_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    ASSERT_EQ(pool->StartPool(), SAF_SUCCESS);

    // Block the single worker so subsequent tasks must queue instead of running immediately.
    auto workerEntered = MakeFlag();
    auto releaseWorker = MakeFlag();
    ASSERT_EQ(pool->AddTask(MakeBlockingTask(workerEntered, releaseWorker)), SAF_SUCCESS);
    ASSERT_TRUE(WaitForFlag(workerEntered, TASK_ENTER_TIMEOUT_MS));

    // Tasks submitted while the worker is busy must be accepted (SAF_SUCCESS) and stay queued.
    std::vector<FlagPtr> pending;
    for (uint32_t i = 0; i < QUEUED_TASK_COUNT; ++i) {
        pending.emplace_back(MakeFlag());
        ASSERT_EQ(pool->AddTask(MakeSimpleTask(pending.back())), SAF_SUCCESS)
            << "queued AddTask failed at i=" << i;
    }

    // None of the queued tasks can have run while the worker is still blocked.
    for (const auto &flag : pending) {
        EXPECT_FALSE(flag->load());
    }

    // Release the worker; all queued tasks must now run to completion.
    releaseWorker->store(true);
    for (const auto &flag : pending) {
        EXPECT_TRUE(WaitForFlag(flag, TASK_DONE_TOLERANCE_MS));
    }
}

/**
 * @tc.name: AddTask_ExecutesTask_001
 * @tc.desc: UT-TP-08: AddTask on a running pool enqueues and the worker executes the task.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, AddTask_ExecutesTask_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    ASSERT_EQ(pool->StartPool(), SAF_SUCCESS);

    auto done = MakeFlag();
    EXPECT_EQ(pool->AddTask(MakeSimpleTask(done)), SAF_SUCCESS);
    EXPECT_TRUE(WaitForFlag(done, TASK_DONE_TOLERANCE_MS));
}

/**
 * @tc.name: AddTask_MultipleSequential_001
 * @tc.desc: UT-TP-09: Multiple tasks submitted one after another all execute with the single worker.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, AddTask_MultipleSequential_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    ASSERT_EQ(pool->StartPool(), SAF_SUCCESS);

    for (uint32_t i = 0; i < CONCURRENT_TASK_COUNT; ++i) {
        auto done = MakeFlag();
        ASSERT_EQ(pool->AddTask(MakeSimpleTask(done)), SAF_SUCCESS);
        EXPECT_TRUE(WaitForFlag(done, TASK_DONE_TOLERANCE_MS)) <<
            "task " << i << " did not execute in time";
    }
}

// ============ StopPool ============

/**
 * @tc.name: StopPool_AlreadyStopped_001
 * @tc.desc: UT-TP-10: StopPool when pool_ is null returns SAF_SUCCESS without crashing.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, StopPool_AlreadyStopped_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    // SetUp already called StopPool, so pool_ is null here. Calling again must
    // hit the pool_ == nullptr early-return branch and stay SUCCESS.
    EXPECT_EQ(pool->StopPool(), SAF_SUCCESS);
    EXPECT_FALSE(pool->IsRunning());
}

/**
 * @tc.name: StopPool_DrainsQueue_001
 * @tc.desc: UT-TP-11: StopPool waits for queued tasks to drain (GetTaskNum > 0 loop) before stopping.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, StopPool_DrainsQueue_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    ASSERT_EQ(pool->StartPool(), SAF_SUCCESS);

    // Submit a short task that will still be queued/running when StopPool is called.
    auto done = MakeFlag();
    ASSERT_EQ(pool->AddTask(MakeSimpleTask(done)), SAF_SUCCESS);

    // StopPool must spin on GetTaskNum() > 0 until the task finishes, then stop.
    EXPECT_EQ(pool->StopPool(), SAF_SUCCESS);
    EXPECT_TRUE(done->load()) << "queued task did not complete before StopPool returned";
    EXPECT_FALSE(pool->IsRunning());
}

/**
 * @tc.name: StopPool_RunningPool_001
 * @tc.desc: UT-TP-12: StopPool on a running, idle pool stops the worker and clears state.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafGuiThreadPoolTest, StopPool_RunningPool_001, TestSize.Level0)
{
    auto pool = SafGuiThreadPool::GetInstance();
    ASSERT_NE(pool, nullptr);
    ASSERT_EQ(pool->StartPool(), SAF_SUCCESS);
    EXPECT_TRUE(pool->IsRunning());

    EXPECT_EQ(pool->StopPool(), SAF_SUCCESS);
    EXPECT_FALSE(pool->IsRunning());

    // After stop, AddTask must be rejected on the not-running branch.
    auto done = MakeFlag();
    EXPECT_EQ(pool->AddTask(MakeSimpleTask(done)), SAF_ERR_THREAD_POOL_NOT_RUNNING);
    EXPECT_FALSE(done->load());
}

} // namespace UnitTest::SafGuiThreadPoolTest
