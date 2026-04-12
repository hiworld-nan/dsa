#include <cmath>
#include <thread>

#include "../../test/test.h"
#include "../tsc_clock.h"

// =============================================================================
// TscClock 基础测试
// =============================================================================

TEST(TscClockTest, SingletonInstance) {
    auto& clock1 = common::TscClock::instance();
    auto& clock2 = common::TscClock::instance();
    EXPECT_EQ(&clock1, &clock2);
    return true;
}

TEST(TscClockTest, Initialization) {
    auto& clock = common::TscClock::instance();
    clock.init();
    EXPECT_TRUE(clock.is_initialized());
    return true;
}

TEST(TscClockTest, FrequencyValid) {
    auto& clock = common::TscClock::instance();
    clock.init();

    // 频率应在合理范围内 (100MHz - 10GHz)
    EXPECT_GT(clock.tsc_frequency(), 100'000'000ULL);
    EXPECT_LT(clock.tsc_frequency(), 10'000'000'000ULL);
    return true;
}

TEST(TscClockTest, ConversionFactors) {
    auto& clock = common::TscClock::instance();
    clock.init();

    // ns_per_cycle * cycles_per_ns ≈ 1
    double product = clock.ns_per_cycle() * clock.cycles_per_ns();
    double diff = std::fabs(product - 1.0);
    EXPECT_TRUE(diff < 1e-9);
    return true;
}

// =============================================================================
// 时间转换测试
// =============================================================================

TEST(TscClockTest, TscToNsRoundTrip) {
    auto& clock = common::TscClock::instance();
    clock.init();

    uint64_t originalNs = 1'000'000;  // 1ms
    uint64_t tsc = clock.ns_to_tsc(originalNs);
    uint64_t backNs = clock.tsc_to_ns(tsc);

    // 允许 0.1% 误差
    double diff = (backNs > originalNs) ? static_cast<double>(backNs - originalNs)
                                        : static_cast<double>(originalNs - backNs);
    double error = diff / static_cast<double>(originalNs);
    EXPECT_TRUE(error < 0.001);
    return true;
}

TEST(TscClockTest, TscToUsRoundTrip) {
    auto& clock = common::TscClock::instance();
    clock.init();

    uint64_t originalUs = 1000;  // 1ms
    uint64_t tsc = clock.us_to_tsc(originalUs);
    uint64_t backUs = clock.tsc_to_us(tsc);

    // 允许 0.2% 误差 (两次浮点转换累积误差)
    double diff = (backUs > originalUs) ? static_cast<double>(backUs - originalUs)
                                        : static_cast<double>(originalUs - backUs);
    double error = diff / static_cast<double>(originalUs);
    EXPECT_TRUE(error < 0.002);
    return true;
}

TEST(TscClockTest, NowIncreases) {
    auto& clock = common::TscClock::instance();
    clock.init();

    uint64_t t1 = clock.now();
    uint64_t t2 = clock.now();
    EXPECT_GE(t2, t1);
    return true;
}

// =============================================================================
// 经过时间测试
// =============================================================================

TEST(TscClockTest, ElapsedNs) {
    auto& clock = common::TscClock::instance();
    clock.init();

    uint64_t start = common::rdtsc();
    clock.busy_wait_ns(100'000);  // 100us
    uint64_t elapsed = clock.elapsed_ns(start);

    // 应至少 100us
    EXPECT_GE(elapsed, 100'000);
    // 不应超过 1ms (留余量)
    EXPECT_LT(elapsed, 1'000'000);
    return true;
}

TEST(TscClockTest, ElapsedTsc) {
    auto& clock = common::TscClock::instance();
    clock.init();

    uint64_t start = common::rdtsc();
    clock.busy_wait_ns(50'000);  // 50us
    uint64_t elapsed = clock.elapsed_tsc(start);

    EXPECT_GT(elapsed, 0ULL);
    return true;
}

// =============================================================================
// 等待机制测试
// =============================================================================

TEST(TscClockTest, BusyWaitNsAccuracy) {
    auto& clock = common::TscClock::instance();
    clock.init();

    uint64_t start = clock.now();
    uint64_t waitNs = 1'000'000;  // 1ms
    clock.busy_wait_ns(waitNs);
    uint64_t elapsed = clock.now() - start;

    // 允许 20% 误差 (TSC校准精度有限)
    double diff =
        (elapsed > waitNs) ? static_cast<double>(elapsed - waitNs) : static_cast<double>(waitNs - elapsed);
    double error = diff / static_cast<double>(waitNs);
    EXPECT_TRUE(error < 0.2);
    return true;
}

TEST(TscClockTest, HybridWaitNsAccuracy) {
    auto& clock = common::TscClock::instance();
    clock.init();

    uint64_t start = clock.now();
    uint64_t waitNs = 5'000'000;  // 5ms
    clock.hybrid_wait_ns(waitNs);
    uint64_t elapsed = clock.now() - start;

    // 允许 30% 误差 (hybrid 不够精确)
    double diff =
        (elapsed > waitNs) ? static_cast<double>(elapsed - waitNs) : static_cast<double>(waitNs - elapsed);
    double error = diff / static_cast<double>(waitNs);
    EXPECT_TRUE(error < 0.3);
    return true;
}

// =============================================================================
// 动态调整测试
// =============================================================================

TEST(TscClockTest, AdjustFrequency) {
    auto& clock = common::TscClock::instance();
    clock.init();

    uint64_t originalFreq = clock.tsc_frequency();
    uint64_t newFreq = 2'000'000'000ULL;  // 2GHz

    bool result = clock.adjust_frequency(newFreq);
    EXPECT_TRUE(result);
    EXPECT_EQ(clock.tsc_frequency(), newFreq);

    // 恢复原频率
    clock.adjust_frequency(originalFreq);
    return true;
}

TEST(TscClockTest, AdjustFrequencyInvalid) {
    auto& clock = common::TscClock::instance();
    clock.init();

    uint64_t originalFreq = clock.tsc_frequency();

    // 无效频率
    EXPECT_FALSE(clock.adjust_frequency(0));
    EXPECT_FALSE(clock.adjust_frequency(100'000'000'000ULL));  // 100GHz

    // 应保持不变
    EXPECT_EQ(clock.tsc_frequency(), originalFreq);
    return true;
}

// =============================================================================
// 统计信息测试
// =============================================================================

TEST(TscClockTest, GetStats) {
    auto& clock = common::TscClock::instance();
    clock.init();

    auto stats = clock.get_stats();
    EXPECT_GT(stats.tsc_frequency_, 0ULL);
    EXPECT_GT(stats.ns_per_cycle_, 0.0);
    EXPECT_GT(stats.cycles_per_ns_, 0.0);
    EXPECT_GT(stats.sample_count_, 0u);
    return true;
}

TEST(TscClockTest, PrintStats) {
    auto& clock = common::TscClock::instance();
    clock.init();

    // 不应抛异常
    clock.print();
    return true;
}

TEST(TscClockTest, OutputStream) {
    auto& clock = common::TscClock::instance();
    clock.init();

    // 不应抛异常
    std::cout << clock << '\n';
    return true;
}

// =============================================================================
// 编译时检查
// =============================================================================

TEST(TscClockTest, CompileTimeChecks) {
    CHECK_COMPILE_TIME(sizeof(common::TscClock::Stats) > 0);
    return true;
}

TEST(TscClockTest, SyncWithSystemClock) {
    auto& clock = common::TscClock::instance();
    clock.init();

    // Should not throw or crash
    clock.sync_with_system_clock();

    // Clock should still be functional
    uint64_t ns = clock.now();
    EXPECT_GT(ns, 0ULL);
    return true;
}

// =============================================================================
// 主函数
// =============================================================================

int main() { return testing::run_all_tests(); }
