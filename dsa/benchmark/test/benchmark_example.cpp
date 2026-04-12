/**
 * @file benchmark_example.cpp
 * @brief 基准测试示例
 */

#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <print>
#include <vector>

#include "../benchmark.h"

// =============================================================================
// 基本测试
// =============================================================================

BENCHMARK(empty_loop) {
    for (std::size_t i = 0; i < iterations; ++i) DONT_OPTIMIZE(i);
}

BENCHMARK(vector_push) {
    std::vector<int> v;
    v.reserve(1000);
    for (std::size_t i = 0; i < iterations; ++i) {
        v.push_back(static_cast<int>(i));
    }
    DONT_OPTIMIZE(v);
}

BENCHMARK(sqrt_calc) {
    double r = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        r = std::sqrt(static_cast<double>(i));
    }
    DONT_OPTIMIZE(r);
}

// =============================================================================
// 时间度量测试
// =============================================================================
// in my environment, clockRef.now() spends about 6ns, so we use about 112ns to measure 100ns
BENCHMARK(100ns_interval) {
    std::size_t sum = 0;
    common::TscClock& clockRef = common::TscClock::instance();
    for (std::size_t i = 0; i < iterations; ++i) {
        auto start = clockRef.now();
        while ((clockRef.now() - start) <= 100) {
            common::pause();
        }
    }
    DONT_OPTIMIZE(sum);
}

// =============================================================================
// 配置测试
// =============================================================================

BENCHMARK_WITH_CONFIG(quick_test, benchmark::Config::quick()) {
    std::vector<int> v(100, 42);
    DONT_OPTIMIZE(v);
}

BENCHMARK_WITH_CONFIG(precise_test, benchmark::Config::precise()) {
    std::vector<double> v(1000);
    for (auto& x : v) x = std::sin(&x - v.data());
    DONT_OPTIMIZE(v);
}

BENCHMARK_WITH_CONFIG(custom, benchmark::Config{}.warmup(20).repetitions(3)) {
    std::vector<int> v;
    for (std::size_t i = 0; i < iterations; ++i) v.push_back(i);
}

// =============================================================================
// 多线程测试
// =============================================================================

BENCHMARK(atomic_single) {
    static std::atomic<int> cnt{0};
    for (std::size_t i = 0; i < iterations; ++i) cnt.fetch_add(1, std::memory_order_relaxed);
    DONT_OPTIMIZE(&cnt);
}

BENCHMARK_WITH_CONFIG(atomic_multi, benchmark::Config{}.threads(4).repetitions(3)) {
    static std::atomic<int> cnt{0};
    for (std::size_t i = 0; i < iterations / 4; ++i) cnt.fetch_add(1, std::memory_order_relaxed);
    DONT_OPTIMIZE(&cnt);
}

BENCHMARK_WITH_CONFIG(mutex_multi, benchmark::Config{}.threads(8).repetitions(5)) {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
}

// =============================================================================
// 带设置/清理
// =============================================================================

namespace {

struct Benchmark_SortData {
    std::vector<int> sort_data;
    void init() {
        sort_data.assign(1000, 0);
        for (auto& x : sort_data) x = rand();
    }
    void reset() { sort_data.clear(); }
};
}  // namespace

BENCHMARK_F(sort_test, Benchmark_SortData) {
    // auto copy = sort_data;
    std::sort(sort_data.begin(), sort_data.end());
    DONT_OPTIMIZE(sort_data);
}

// =============================================================================
// 比较测试
// =============================================================================

BENCHMARK(vector_no_reserve) {
    std::vector<int> v;
    for (int i = 0; i < 1000; ++i) v.push_back(i);
    DONT_OPTIMIZE(v);
}

BENCHMARK(vector_with_reserve) {
    std::vector<int> v;
    v.reserve(1000);
    for (int i = 0; i < 1000; ++i) v.push_back(i);
    DONT_OPTIMIZE(v);
}

BENCHMARK(atomic_seq_cst) {
    static std::atomic<int> x{0};
    for (std::size_t i = 0; i < iterations; ++i) x.fetch_add(1, std::memory_order_seq_cst);
    DONT_OPTIMIZE(&x);
}

BENCHMARK(atomic_relaxed) {
    static std::atomic<int> x{0};
    for (std::size_t i = 0; i < iterations; ++i) x.fetch_add(1, std::memory_order_relaxed);
    DONT_OPTIMIZE(&x);
}

// =============================================================================
// 主函数
// =============================================================================

int main() {
    std::println("Benchmark v{}\n", benchmark::version());

    auto results = benchmark::run_all_benchmarks();

    if (!results.empty()) {
        std::println("\n[Exporting results...]");
        benchmark::Reporter::save_to_file("results.json", benchmark::Reporter::to_json(results));
        benchmark::Reporter::save_to_file("results.csv", benchmark::Reporter::to_csv(results));
        std::println("\nTable:");
        benchmark::Reporter::print_table(results);
    }

    return 0;
}
