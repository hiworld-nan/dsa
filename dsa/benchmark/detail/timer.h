#pragma once

#include <chrono>
#include <cstdint>

#include "core.h"

namespace benchmark {
class Timer {
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

public:
    constexpr Timer() = default;

    [[gnu::always_inline, gnu::hot]]
    inline void start() noexcept {
        start_ = Clock::now();
        running_ = true;
    }

    [[gnu::always_inline, gnu::hot]]
    inline void stop() noexcept {
        end_ = Clock::now();
        running_ = false;
    }

    [[gnu::always_inline, gnu::hot]]
    inline NanoSeconds elapsed_ns() const noexcept {
        auto d = running_ ? (Clock::now() - start_) : (end_ - start_);
        return std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    }

    [[gnu::always_inline, gnu::hot]]
    inline double elapsed_us() const noexcept {
        return elapsed_ns() / 1e3;
    }

    [[gnu::always_inline, gnu::hot]]
    inline double elapsed_ms() const noexcept {
        return elapsed_ns() / 1e6;
    }

    [[gnu::always_inline, gnu::hot]]
    inline double elapsed_s() const noexcept {
        return elapsed_ns() / 1e9;
    }

    [[gnu::always_inline, gnu::hot]]
    inline void reset() noexcept {
        start_ = {};
        end_ = {};
        running_ = false;
    }

    [[gnu::always_inline, gnu::hot]]
    inline bool is_running() const noexcept {
        return running_;
    }

private:
    TimePoint start_{}, end_{};
    bool running_{false};
};

// =============================================================================
// TscTimer
// =============================================================================
/// 构造时记录起始 TSC，析构时或调用 elapsedNs() 获取经过时间
/// 零开销抽象，适用于性能测量和代码块计时
class TscTimer {
public:
    TscTimer() { clockRef.init(); }

    [[gnu::always_inline, gnu::hot]]
    inline void start() noexcept {
        start_ = clockRef.now();
        running_ = true;
    }

    [[gnu::always_inline, gnu::hot]]
    inline void stop() noexcept {
        end_ = clockRef.now();
        running_ = false;
    }

    [[gnu::always_inline, gnu::hot]]
    inline NanoSeconds elapsed_ns() const noexcept {
        return end_ - start_;
    }

    [[gnu::always_inline, gnu::hot]]
    inline double elapsed_us() const noexcept {
        return elapsed_ns() / 1e3;
    }

    [[gnu::always_inline, gnu::hot]]
    inline double elapsed_ms() const noexcept {
        return elapsed_ns() / 1e6;
    }

    [[gnu::always_inline, gnu::hot]]
    inline double elapsed_s() const noexcept {
        return elapsed_ns() / 1e9;
    }

    [[gnu::always_inline, gnu::hot]]
    inline void reset() noexcept {
        start_ = 0;
        end_ = 0;
        running_ = false;
    }

    [[gnu::always_inline, gnu::hot]]
    inline bool is_running() const noexcept {
        return running_;
    }

private:
    common::TscClock& clockRef = common::TscClock::instance();

    uint64_t start_{}, end_{};
    bool running_{false};
};

// =============================================================================
// 辅助函数
// =============================================================================

inline NanoSeconds now_ns() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

inline std::uint64_t now_cycles() noexcept { return common::rdtscp(); }

}  // namespace benchmark
