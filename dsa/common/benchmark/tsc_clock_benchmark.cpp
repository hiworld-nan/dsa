/**
 * @file tsc_clock_benchmark.cpp
 * @brief TscClock performance benchmark
 */

#include <thread>

#include "../../benchmark/benchmark.h"
#include "../tsc_clock.h"

static common::TscClock& s_clock = common::TscClock::instance();
// =============================================================================
// 基础操作性能
// =============================================================================

BENCHMARK(rdtsc_raw) {
    uint64_t tsc = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        tsc = common::rdtsc();
    }
    DONT_OPTIMIZE(tsc);
}

BENCHMARK(builtin_rdtsc) {
    uint64_t tsc = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        tsc = __builtin_ia32_rdtsc();
    }
    DONT_OPTIMIZE(tsc);
}

BENCHMARK(now) {
    uint64_t ns = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        ns = s_clock.now();
    }
    DONT_OPTIMIZE(ns);
}

BENCHMARK(tsc_to_ns) {
    uint64_t ns = 0;
    uint64_t tsc = common::rdtsc();
    for (std::size_t i = 0; i < iterations; ++i) {
        ns = s_clock.tsc_to_ns(tsc);
    }
    DONT_OPTIMIZE(ns);
}

BENCHMARK(ns_to_tsc) {
    uint64_t tsc = 0;
    uint64_t ns = 1'000'000;
    for (std::size_t i = 0; i < iterations; ++i) {
        tsc = s_clock.ns_to_tsc(ns);
    }
    DONT_OPTIMIZE(tsc);
}

// =============================================================================
// 与标准库对比
// =============================================================================

BENCHMARK(steady_clock_now) {
    std::chrono::steady_clock::time_point t;
    for (std::size_t i = 0; i < iterations; ++i) {
        t = std::chrono::steady_clock::now();
    }
    DONT_OPTIMIZE(&t);
}

BENCHMARK(high_resolution_clock_now) {
    auto t = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        t = std::chrono::high_resolution_clock::now();
    }
    DONT_OPTIMIZE(&t);
}

BENCHMARK(tsc_clock_now) {
    uint64_t t = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        t = s_clock.now();
    }
    DONT_OPTIMIZE(&t);
}

// =============================================================================
// 等待机制性能
// =============================================================================

BENCHMARK(busy_wait_100ns) { s_clock.busy_wait_ns(100); }

BENCHMARK(busy_wait_1us) { s_clock.busy_wait_ns(1000); }

BENCHMARK(busy_wait_10us) { s_clock.busy_wait_ns(10000); }

BENCHMARK(hybrid_wait_1us) { s_clock.hybrid_wait_ns(1000); }

BENCHMARK(hybrid_wait_100us) { s_clock.hybrid_wait_ns(100000); }

// =============================================================================
// 多线程测试
// =============================================================================

BENCHMARK_WITH_CONFIG(now_multi_thread, benchmark::Config{}.threads(4)) {
    uint64_t t = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        t = s_clock.now();
    }
    DONT_OPTIMIZE(t);
}

BENCHMARK_WITH_CONFIG(now_multi_thread_8, benchmark::Config{}.threads(8)) {
    uint64_t t = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        t = s_clock.now();
    }
    DONT_OPTIMIZE(t);
}

BENCHMARK_WITH_CONFIG(tsc2ns_multi_thread, benchmark::Config{}.threads(4)) {
    uint64_t tsc = common::rdtsc();
    uint64_t ns = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        ns = s_clock.tsc_to_ns(tsc);
    }
    DONT_OPTIMIZE(ns);
}

// =============================================================================
// 转换精度对比
// =============================================================================

BENCHMARK(double_conversion) {
    uint64_t ns = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        uint64_t tsc = common::rdtsc();
        ns = s_clock.tsc_to_ns(tsc);
    }
    DONT_OPTIMIZE(ns);
}

BENCHMARK(batch_elapsed) {
    uint64_t e = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        uint64_t start = common::rdtsc();
        e = s_clock.elapsed_ns(start);
    }
    DONT_OPTIMIZE(e);
}

// =============================================================================
// 主函数
// =============================================================================

int main() {
    // 初始化 TscClock
    s_clock.init();

    std::cout << "TscClock Benchmark v" << benchmark::version() << "\n\n";
    std::cout << "TSC Frequency: " << common::TscClock::instance().tsc_frequency() / 1e9 << " GHz\n\n";

    auto results = benchmark::run_all_benchmarks();

    if (!results.empty()) {
        std::cout << "\n[Exporting results...]\n";
        benchmark::Reporter::save_to_file("tsc_clock_results.json", benchmark::Reporter::to_json(results));
        benchmark::Reporter::save_to_file("tsc_clock_results.csv", benchmark::Reporter::to_csv(results));
        std::cout << "\nTable:\n";
        benchmark::Reporter::print_table(results);
    }

    return 0;
}
