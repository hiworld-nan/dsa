#pragma once

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#ifdef __linux__
#include <time.h>
#endif

#include "../utility/utility.h"
#include "constants.h"
#include "intrinsics.h"
#include "macros.h"
#include "singleton.h"

namespace common {

// =============================================================================
// 辅助函数：系统时钟与精确睡眠
// =============================================================================

/// 高精度单调时钟 - 使用 clock_gettime (Linux)
/// 返回从系统启动以来的纳秒数（单调递增，不受系统时间调整影响）
/// 性能：约 20-50ns 每次调用，比 std::chrono::steady_clock 更快
[[gnu::hot, gnu::always_inline]]
static inline uint64_t monotonic_clock_ns() noexcept {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL + static_cast<uint64_t>(ts.tv_nsec);
}

// =============================================================================
// TscClock - 高性能 TSC 时钟
// =============================================================================

/// 基于 TSC 的高精度时钟
///
/// 设计要点：
/// 1. 使用 RDTSC 指令获取 CPU 时钟周期，开销约 20 个周期
/// 2. 单例模式，全局唯一实例
/// 3. 缓存行对齐，避免 false sharing
///
/// 使用流程：
///   1. 程序启动时调用 TscClock::instance().init()
///   2. 使用 now() 获取当前纳秒时间戳
///
/// 性能指标：
///   - now(): 约 20 个 CPU 周期（RDTSC + 乘法）
///   - tsc_to_ns(): 约 5 个 CPU 周期（单次乘法）
///   - 精度：纳秒级（取决于 TSC 频率校准精度）
class TscClock : public singleton<TscClock> {
    friend class singleton<TscClock>;

public:
    // =========================================================================
    // 统计信息结构
    // =========================================================================

    /// 校准统计信息
    struct Stats {
        uint64_t tsc_frequency_{0};  // TSC 频率（Hz）
        double ns_per_cycle_{0.0};   // 每周期纳秒数 = 1e9 / frequency
        double cycles_per_ns_{0.0};  // 每纳秒周期数 = frequency / 1e9
        double std_deviation_{0.0};  // 频率测量的标准差（Hz）
        double confidence_{0.0};     // 校准置信度 [0, 1]
        uint32_t sample_count_{0};   // 有效样本数
        bool invariant_tsc_{false};  // CPU 是否支持不变 TSC

        friend inline std::ostream& operator<<(std::ostream& os, const Stats& s) {
            return os << std::format(
                       "TscClock Stats:\n"
                       "  frequency: {:.3f} GHz\n"
                       "  ns_per_cycle: {:.6f}\n"
                       "  cycles_per_ns: {:.6f}\n"
                       "  std_deviation: {:.2f} Hz (RSD: {:.4f}%)\n"
                       "  confidence: {:.2f}%\n"
                       "  sample_count: {}\n"
                       "  invariant_tsc: {}",
                       s.tsc_frequency_ / 1e9, s.ns_per_cycle_, s.cycles_per_ns_, s.std_deviation_,
                       (s.tsc_frequency_ > 0) ? (s.std_deviation_ / s.tsc_frequency_ * 100.0) : 0.0,
                       s.confidence_ * 100, s.sample_count_, s.invariant_tsc_ ? "true" : "false");
        }
    };

    // =========================================================================
    // 核心接口
    // =========================================================================

    /// 初始化校准（线程安全，启动时调用一次）
    /// 内部流程：检测不变 TSC → 预热 → 多次采样 → IQR 剔除异常值 → 计算均值和标准差
    void init() noexcept {
        std::call_once(init_flag_, [this] { calibrate(); });
    }

    /// 获取当前时间（纳秒）- 热路径，约 20 个 CPU 周期
    /// 实现：rdtsc() * nsPerCycle_（单次乘法）
    [[gnu::hot, gnu::always_inline]]
    inline uint64_t now() const noexcept {
        return tsc_to_ns(common::rdtsc());
    }

    /// TSC 周期 → 纳秒（核心转换，热路径）
    [[gnu::hot, gnu::always_inline]]
    inline uint64_t tsc_to_ns(uint64_t tsc) const noexcept {
        return static_cast<uint64_t>(tsc * ns_per_cycle_);
    }

    /// 纳秒 → TSC 周期
    [[gnu::hot, gnu::always_inline]]
    inline uint64_t ns_to_tsc(uint64_t ns) const noexcept {
        return static_cast<uint64_t>(ns * cycles_per_ns_);
    }

    /// TSC 周期 → 微秒
    [[gnu::hot, gnu::always_inline]]
    inline uint64_t tsc_to_us(uint64_t tsc) const noexcept {
        return static_cast<uint64_t>(tsc * ns_per_cycle_ / time_constants::kNsPerUs);
    }

    /// 微秒 → TSC 周期
    [[gnu::hot, gnu::always_inline]]
    inline uint64_t us_to_tsc(uint64_t us) const noexcept {
        return static_cast<uint64_t>(us * cycles_per_ns_ * time_constants::kNsPerUs);
    }

    /// 获取 TSC 频率（Hz）
    [[gnu::hot, gnu::always_inline]]
    inline uint64_t tsc_frequency() const noexcept {
        return tsc_frequency_;
    }

    /// 获取每周期纳秒数
    [[gnu::hot, gnu::always_inline]]
    inline double ns_per_cycle() const noexcept {
        return ns_per_cycle_;
    }

    /// 获取每纳秒周期数
    [[gnu::hot, gnu::always_inline]]
    inline double cycles_per_ns() const noexcept {
        return cycles_per_ns_;
    }

    /// 检查是否支持不变 TSC（CPU 频率变化不影响 TSC 速率）
    inline bool is_invariant() const noexcept { return invariant_tsc_; }

    /// 计算经过的纳秒数（从 startTsc 到现在）
    [[gnu::hot, gnu::always_inline]]
    inline uint64_t elapsed_ns(uint64_t start_tsc) const noexcept {
        return tsc_to_ns(common::rdtsc() - start_tsc);
    }

    /// 计算经过的 TSC 周期数
    [[gnu::hot, gnu::always_inline]]
    inline uint64_t elapsed_tsc(uint64_t start_tsc) const noexcept {
        return common::rdtsc() - start_tsc;
    }

    // =========================================================================
    // 等待机制
    // =========================================================================

    /// 忙等待指定纳秒（高精度，消耗 CPU）
    [[gnu::hot]]
    void busy_wait_ns(uint64_t ns) const noexcept {
        uint64_t end = common::rdtsc() + ns_to_tsc(ns);
        while (common::rdtsc() < end) {
            common::pause();
        }
    }

    /// 混合等待（忙等待 + yield）
    [[gnu::hot]]
    void hybrid_wait_ns(uint64_t ns) const noexcept {
        uint64_t end = common::rdtsc() + ns_to_tsc(ns);
        uint64_t threshold = ns_to_tsc(1000);

        while (true) {
            uint64_t remaining = end - common::rdtsc();
            if (static_cast<int64_t>(remaining) <= 0) {
                break;
            }
            (remaining > threshold) ? std::this_thread::yield() : common::pause();
        }
    }

    // =========================================================================
    // 调整接口
    // =========================================================================

    /// 重新校准 TSC 频率
    void recalibrate() noexcept { calibrate(); }

    /// 手动调整频率（验证范围：0 ~ 10GHz）
    inline bool adjust_frequency(uint64_t new_frequency) noexcept {
        if (new_frequency == 0 || new_frequency > 10'000'000'000ULL) {
            return false;
        }
        apply_frequency(static_cast<double>(new_frequency));
        return true;
    }

    // =========================================================================
    // 状态查询
    // =========================================================================

    Stats get_stats() const noexcept {
        return {tsc_frequency_, ns_per_cycle_, cycles_per_ns_, std_deviation_,
                confidence_,    sample_count_, invariant_tsc_};
    }

    friend inline std::ostream& operator<<(std::ostream& os, const TscClock& clock) {
        return os << clock.get_stats();
    }

    bool is_initialized() const noexcept { return initialized_.load(std::memory_order_acquire); }

    /// 与系统时钟同步（基于累积误差调整频率）
    void sync_with_system_clock() noexcept {
        if (!initialized_.load(std::memory_order_acquire)) {
            return;
        }

        common::cpuid_serialize();
        uint64_t tsc = common::rdtsc();
        uint64_t sys_ns = monotonic_clock_ns();

        // 计算期望 TSC 值
        uint64_t expected_tsc = ns_to_tsc(sys_ns);

        // 计算误差
        int64_t tsc_error = static_cast<int64_t>(tsc) - static_cast<int64_t>(expected_tsc);
        if (tsc_error == 0) {
            return;
        }

        // 按误差比例调整频率
        double error_ratio = static_cast<double>(tsc_error) / static_cast<double>(tsc);
        double adjusted_freq = static_cast<double>(tsc_frequency_) * (1.0 - error_ratio * 0.01);

        apply_frequency(adjusted_freq);
    }

private:
    TscClock() = default;
    ~TscClock() {}

    // =========================================================================
    // 内部实现
    // =========================================================================

    /// 应用新频率（更新转换因子）
    inline void apply_frequency(double frequency) noexcept {
        if (!std::isfinite(frequency) || frequency < 1e8 || frequency > 1e10) {
            frequency = 2.5e9;  // 回退到 2.5 GHz
        }
        tsc_frequency_ = static_cast<uint64_t>(std::llround(frequency));
        ns_per_cycle_ = 1e9 / frequency;
        cycles_per_ns_ = frequency / 1e9;
        initialized_.store(true, std::memory_order_release);
    }

    /// 预热 CPU 缓存和流水线
    void warmup() noexcept {
        for (int i = 0; i < 100; ++i) {
            volatile uint64_t dummy = rdtsc();
            (void)dummy;
        }
        measure_frequency(100);

        for (int i = 0; i < 10; ++i) {
            volatile uint64_t dummy = monotonic_clock_ns();
            (void)dummy;
        }
        common::compiler_fence();
    }

    /// 测量 TSC 频率
    [[gnu::always_inline, gnu::hot]]
    inline double measure_frequency(int sleepUs) noexcept {
        common::cpuid_serialize();
        uint64_t sys_ns1 = monotonic_clock_ns();
        uint64_t tsc1 = common::rdtsc();

        std::this_thread::sleep_for(std::chrono::microseconds(sleepUs));

        uint64_t tsc2 = common::rdtsc();
        uint64_t sys_ns2 = monotonic_clock_ns();
        common::cpuid_serialize();

        int64_t duration_ns = static_cast<int64_t>(sys_ns2 - sys_ns1);
        uint64_t tsc_delta = tsc2 - tsc1;

        return (duration_ns > 0 && tsc_delta > 0) ? (static_cast<double>(tsc_delta) * 1e9 / duration_ns)
                                                  : 0.0;
    }

    /// 计算均值
    inline double calculate_mean(const std::vector<double>& samples, size_t start, size_t end) noexcept {
        if (start >= end || start >= samples.size()) {
            return 0.0;
        }
        end = std::min(end, samples.size());
        double sum = 0.0;
        for (size_t i = start; i < end; ++i) {
            sum += samples[i];
        }
        return sum / static_cast<double>(end - start);
    }

    /// 计算标准差
    double calculate_std_dev(const std::vector<double>& samples, size_t start, size_t end,
                             double mean) noexcept {
        if (start >= end || start >= samples.size()) {
            return 0.0;
        }

        double sum = 0.0;
        end = std::min(end, samples.size());
        for (size_t i = start; i < end; ++i) {
            double diff = samples[i] - mean;
            sum += diff * diff;
        }
        return std::sqrt(sum / static_cast<double>(end - start));
    }

    /// 检测 CPU 是否支持不变 TSC
    inline void check_invariant_tsc() noexcept {
#if ARCH_X86_64
        auto regs = common::cpuid(0x80000000);
        if (regs.eax >= 0x80000007) {
            regs = common::cpuid(0x80000007);
            invariant_tsc_ = (regs.edx & (1 << 8)) != 0;
        }
#endif
    }

    /// 校准 TSC 频率
    [[gnu::always_inline, gnu::hot]]
    inline void calibrate() noexcept {
        constexpr int kSingleSampleLoopCnt = 73;
        constexpr int kSampleCount = kSingleSampleLoopCnt * 3;

        std::vector<double> samples;
        samples.reserve(kSampleCount);

        check_invariant_tsc();
        warmup();

        for (int i = 0; i < kSingleSampleLoopCnt; ++i) {
            if (double f = measure_frequency(500); f > 0) {
                samples.push_back(f);
            }
        }
        for (int i = 0; i < kSingleSampleLoopCnt; ++i) {
            if (double f = measure_frequency(1000); f > 0) {
                samples.push_back(f);
            }
        }
        for (int i = 0; i < kSingleSampleLoopCnt; ++i) {
            if (double f = measure_frequency(2000); f > 0) {
                samples.push_back(f);
            }
        }

        if (samples.empty()) {
            apply_frequency(2.5e9);
            std_deviation_ = 0.0;
            confidence_ = 0.0;
            sample_count_ = 0;
            return;
        }

        std::sort(samples.begin(), samples.end());

        size_t n = samples.size();
        double q1 = samples[n / 4];
        double q3 = samples[(3 * n) / 4];
        double iqr = q3 - q1;
        double lower_bound = q1 - 1.5 * iqr;
        double upper_bound = q3 + 1.5 * iqr;

        size_t valid_start = 0, valid_end = n;
        while (valid_start < n && samples[valid_start] < lower_bound) {
            ++valid_start;
        }
        while (valid_end > valid_start && samples[valid_end - 1] > upper_bound) {
            --valid_end;
        }
        if (valid_end - valid_start < 5) {
            valid_start = n / 5;
            valid_end = (4 * n) / 5;
        }

        double mean = calculate_mean(samples, valid_start, valid_end);

        if (mean < 1e8 || mean > 1e10) {
            apply_frequency(2.5e9);
            std_deviation_ = 0.0;
            confidence_ = 0.0;
            sample_count_ = 0;
            return;
        }

        double std_dev = calculate_std_dev(samples, valid_start, valid_end, mean);
        double std_dev_percent = (mean > 0) ? (std_dev / mean * 100.0) : 100.0;

        apply_frequency(mean);
        std_deviation_ = std_dev;
        confidence_ = std::max(0.0, 1.0 - std_dev_percent / 5.0) *
                      (static_cast<double>(valid_end - valid_start) / kSampleCount);
        sample_count_ = static_cast<uint32_t>(valid_end - valid_start);
    }

    // =========================================================================
    // 成员变量
    // =========================================================================

    // 核心成员（缓存行对齐，避免 false sharing）
    std::once_flag init_flag_;
    alignas(memory_constants::kCacheLineSize) std::atomic<bool> initialized_{false};
    alignas(memory_constants::kCacheLineSize) uint64_t tsc_frequency_{0};
    alignas(memory_constants::kCacheLineSize) double ns_per_cycle_{0.0};
    alignas(memory_constants::kCacheLineSize) double cycles_per_ns_{0.0};
    Padding<sizeof(double)> padding_;

    // 校准统计
    double std_deviation_{0.0};
    double confidence_{0.0};
    uint32_t sample_count_{0};
    bool invariant_tsc_{false};
};

}  // namespace common
