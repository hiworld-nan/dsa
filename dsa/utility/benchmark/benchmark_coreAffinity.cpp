/**
 * @file benchmark_coreAffinity.cpp
 * @brief CPU 亲和性性能基准测试
 * @version 1.0.0
 */

#include <print>
#include <thread>

#include "../benchmark.h"
#include "../utility.h"

using namespace utils;

// =============================================================================
// CpuSet 操作开销
// =============================================================================

BENCHMARK(cpu_set_add) {
    for (std::size_t i = 0; i < iterations; ++i) {
        CpuSet set;
        set.add_cpu(0);
        set.add_cpu(1);
        set.add_cpu(2);
        set.add_cpu(3);
        DONT_OPTIMIZE(&set);
    }
}

BENCHMARK(cpu_set_contains) {
    CpuSet set;
    set.add_cpu(0);
    set.add_cpu(2);
    set.add_cpu(4);
    set.add_cpu(6);
    bool result = false;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = set.contains(2);
    }
    DONT_OPTIMIZE(&result);
}

BENCHMARK(cpu_set_count) {
    CpuSet set;
    for (std::size_t i = 0; i < 8; ++i) {
        set.add_cpu(i);
    }

    std::size_t result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = set.count();
    }
    DONT_OPTIMIZE(&result);
}

// =============================================================================
// 亲和性操作开销
// =============================================================================

BENCHMARK(get_thread_affinity) {
    for (std::size_t i = 0; i < iterations; ++i) {
        auto affinity = CpuAffinity::get_thread_affinity();
        DONT_OPTIMIZE(&affinity);
    }
}

BENCHMARK(get_available_cpus) {
    std::size_t result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = CpuAffinity::get_available_cpus();
    }
    DONT_OPTIMIZE(&result);
}

// =============================================================================
// 字符串序列化开销
// =============================================================================

BENCHMARK(cpu_affinity_to_string) {
    CpuSet set;
    set.add_cpu(0);
    set.add_cpu(1);
    set.add_cpu(2);
    set.add_cpu(4);
    set.add_cpu(7);
    for (std::size_t i = 0; i < iterations; ++i) {
        auto s = CpuAffinity::to_string(set);
        DONT_OPTIMIZE(s.data());
    }
}

BENCHMARK(cpu_affinity_from_string) {
    for (std::size_t i = 0; i < iterations; ++i) {
        auto set = CpuAffinity::from_string("0-2,4,7");
        DONT_OPTIMIZE(&set);
    }
}

// =============================================================================
// 主函数
// =============================================================================

int main() {
    std::println("CPU Affinity Benchmark v{}\n", benchmark::version());

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
