/**
 * @file runner.h
 * @brief 基准测试运行器
 * @version 1.0.0
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <functional>
#include <latch>
#include <mutex>
#include <optional>
#include <print>
#include <string>
#include <thread>
#include <vector>

#include "core.h"
#include "statistics.h"
#include "timer.h"

namespace benchmark {

// =============================================================================
// 配置
// =============================================================================

struct Config {
    // max time for a single test
    NanoSeconds max_time_{1'000'000'000};  // 1000ms
    std::size_t warmup_{10};
    std::size_t min_iterations_{1};
    std::size_t max_iterations_{100'000};
    std::size_t repetitions_{1000};
    std::size_t threads_{1};
    bool verbose_{false};

    Config& max_time(NanoSeconds v) {
        max_time_ = v;
        return *this;
    }

    Config& warmup(std::size_t v) {
        warmup_ = v;
        return *this;
    }

    Config& min_iterations(std::size_t v) {
        min_iterations_ = v;
        return *this;
    }

    Config& max_iterations(std::size_t v) {
        max_iterations_ = v;
        return *this;
    }

    Config& repetitions(std::size_t v) {
        repetitions_ = v;
        return *this;
    }

    Config& threads(std::size_t v) {
        threads_ = v;
        return *this;
    }

    Config& verbose(bool v) {
        verbose_ = v;
        return *this;
    }

    static Config quick() noexcept {
        return Config{}.max_time(1e7).warmup(3).max_iterations(1e6).verbose(false);
    }

    static Config normal() noexcept { return Config{}; }

    static Config precise() noexcept {
        return Config{}.max_time(1e9).warmup(100).min_iterations(100).repetitions(5);
    }

    static Config concurrent(std::size_t n) noexcept { return Config{}.threads(n).repetitions(3); }
};

// =============================================================================
// 测试用例
// =============================================================================

// using FunctionT = std::function<void()>;
template <typename... Args>
using FunctionT = std::function<void(Args...)>;
using BenchmarkFunctionT = std::function<void(IterationCount&)>;

struct Benchmark_Case {
    std::string name_{};
    std::string suite_{};

    BenchmarkFunctionT func{};
    FunctionT<> init{};
    FunctionT<> reset{};
    Config config_{};
};

// =============================================================================
// 注册中心
// =============================================================================

class Benchmark_Registry : public common::singleton<Benchmark_Registry> {
    friend class common::singleton<Benchmark_Registry>;

public:
    void register_benchmark(const Benchmark_Case& bm) {
        std::lock_guard<std::mutex> lock(mutex_);
        benchmarks_.push_back(std::move(bm));
    }

    std::vector<Benchmark_Case>& get_benchmarks() { return benchmarks_; }
    const std::vector<Benchmark_Case>& get_benchmarks() const { return benchmarks_; }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        benchmarks_.clear();
    }
    std::size_t size() const noexcept { return benchmarks_.size(); }
    bool empty() const noexcept { return benchmarks_.empty(); }

private:
    Benchmark_Registry() = default;
    std::vector<Benchmark_Case> benchmarks_;
    mutable std::mutex mutex_;
};

// =============================================================================
// 运行器
// =============================================================================

class Runner {
public:
    Statistics run_single(Benchmark_Case& bm) {
        Statistics_Analyzer analyzer;
        const auto& cfg = bm.config_;
        TscTimer timer;

        for (std::size_t i = 0; i < cfg.warmup_; ++i) {
            if (bm.init) {
                bm.init();
            }

            IterationCount cnt = 1;
            if (cfg.threads_ > 1) {
                run_parallel(bm.func, cnt, cfg.threads_, timer);
            } else {
                bm.func(cnt);
            }

            if (bm.reset) {
                bm.reset();
            }
        }
        if (cfg.verbose_ && cfg.warmup_ > 0) {
            std::println("[ WARMUP ] {} ({} iters)", bm.name_, cfg.warmup_);
        }

        timer.reset();
        auto iters = determine_iterations(bm, cfg);

        for (std::size_t rep = 0; rep < cfg.repetitions_; ++rep) {
            if (bm.init) {
                bm.init();
            }

            if (cfg.threads_ > 1) {
                run_parallel(bm.func, iters, cfg.threads_, timer);
            } else {
                timer.start();
                bm.func(iters);
                timer.stop();
            }

            if (bm.reset) {
                bm.reset();
            }
            analyzer.add_sample(timer.elapsed_ns(), iters);

            if (cfg.verbose_) {
                std::println("[ RUN    ] {} rep {}/{}: {} iters, {:.2f} ms", bm.name_, rep + 1,
                             cfg.repetitions_, iters, timer.elapsed_ms());
            }
        }

        return analyzer.compute(bm.name_, cfg.threads_);
    }

    std::vector<Statistics> run_all(bool verbose = true) {
        std::vector<Statistics> results;
        auto& benchmarks = Benchmark_Registry::instance().get_benchmarks();

        if (verbose && !benchmarks.empty()) {
            std::println("\n[============= Running {} Benchmarks =============]", benchmarks.size());
        }

        for (auto& bm : benchmarks) {
            results.push_back(run_single(bm));
            if (verbose) {
                print_result(results.back());
            }
        }

        if (verbose && !results.empty()) {
            print_summary(results);
        }
        return results;
    }

private:
    IterationCount determine_iterations(Benchmark_Case& bm, const Config& cfg) {
        if (cfg.min_iterations_ == cfg.max_iterations_) {
            return cfg.min_iterations_;
        }

        if (bm.init) {
            bm.init();
        }

        TscTimer t;
        IterationCount cnt = cfg.warmup_;
        if (cfg.threads_ > 1) {
            run_parallel(bm.func, cnt, cfg.threads_, t);
        } else {
            t.start();
            bm.func(cnt);
            t.stop();
        }

        if (bm.reset) {
            bm.reset();
        }

        // constexpr IterationCount min_iters = 100'000;
        auto elapsed = t.elapsed_ns();
        if (elapsed <= 0) {
            return cfg.min_iterations_;
        }

        double ns_per_iter = static_cast<double>(elapsed) / cfg.warmup_;
        auto needed = static_cast<IterationCount>(cfg.max_time_ / ns_per_iter);
        needed = std::clamp(needed, cfg.min_iterations_, cfg.max_iterations_);

        // 向上取整到 10 的幂
        IterationCount rounded = cfg.min_iterations_;
        while (rounded < needed) {
            rounded *= 10;
        }
        return std::min(rounded, cfg.max_iterations_);
    }

    void run_parallel(const BenchmarkFunctionT& func, IterationCount& total, std::size_t threads,
                      TscTimer& timer) {
        std::vector<std::thread> workers;
        workers.reserve(threads);

        std::latch ready_latch(threads);  // 等待所有工作线程准备好
        std::latch start_latch(1);        // 主线程通知开始
        std::latch done_latch(threads);   // 等待所有工作线程完成

        for (std::size_t i = 0; i < threads; ++i) {
            workers.emplace_back([&] {
                ready_latch.count_down();  // 1. 表示已准备好
                start_latch.wait();        // 2. 等待开始信号

                func(total);

                done_latch.count_down();  // 3. 表示已完成
            });
        }

        ready_latch.wait();  // 等待所有工作线程准备好

        timer.start();             // 先启动计时
        start_latch.count_down();  // 再通知所有工作线程开始
        done_latch.wait();         // 等待所有工作线程完成
        timer.stop();

        for (auto& t : workers) t.join();
    }

    void print_result(const Statistics& s) {
        std::println("\n[ RESULT ] {}", s.name_);
        std::println("  Mean:    {:.2f} ns ({:.2f} ns/iter)", s.mean_, s.mean_per_iter_);
        std::println("  StdDev:  {:.2f} ns (RSD: {:.1f}%)", s.stddev_, s.rsd());
        std::println("  Range:   [{:.2f}, {:.2f}] ns", s.min_, s.max_);
        std::println("  P50/P95/P99/P999: {:.2f} / {:.2f} / {:.2f} / {:.2f} ns", s.p50_, s.p95_, s.p99_,
                     s.p999_);
        std::println("  Throughput: {:.1f} Mops/s", s.mops());
        if (s.threads_ > 1) {
            std::println("  Threads: {}", s.threads_);
        }
    }

    void print_summary(const std::vector<Statistics>& results) {
        auto [fastest, slowest] = std::minmax_element(
            results.begin(), results.end(),
            [](const auto& a, const auto& b) { return a.mean_per_iter_ < b.mean_per_iter_; });

        std::println("\n[============= Summary =============]");
        std::println("Total: {}, Fastest: {} ({:.2f} ns), Slowest: {} ({:.2f} ns)", results.size(),
                     fastest->name_, fastest->mean_per_iter_, slowest->name_, slowest->mean_per_iter_);
    }
};

// =============================================================================
// 便捷函数
// =============================================================================

inline std::vector<Statistics> run_all_benchmarks(bool verbose = true) {
    TscTimer timer;
    return Runner{}.run_all(verbose);
}

inline std::optional<Statistics> run_benchmark(const std::string& name) {
    auto& bm = Benchmark_Registry::instance().get_benchmarks();
    auto it = std::find_if(bm.begin(), bm.end(), [&](const auto& b) { return b.name_ == name; });
    if (it == bm.end()) {
        return std::nullopt;
    }
    return Runner{}.run_single(*it);
}

}  // namespace benchmark
