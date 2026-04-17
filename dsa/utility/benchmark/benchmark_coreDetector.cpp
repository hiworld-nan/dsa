/**
 * @file benchmark_coreDetector.cpp
 * @brief CPU 检测器性能基准测试
 * @version 1.0.0
 */

#include <print>

#include "../benchmark.h"
#include "../utility.h"

using namespace utils;

// =============================================================================
// 特性查询开销
// =============================================================================

BENCHMARK(feature_has_check) {
    auto& det = CoreDetector::instance();
    volatile bool result = false;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = det.has(Feature::AVX);
        result = det.has(Feature::AVX2);
        result = det.has(Feature::SSE);
    }
    DONT_OPTIMIZE(&result);
}

BENCHMARK(feature_convenience_check) {
    auto& det = CoreDetector::instance();
    volatile bool result = false;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = det.support_sse();
        result = det.support_avx();
        result = det.support_avx2();
    }
    DONT_OPTIMIZE(&result);
}

// =============================================================================
// 信息查询开销
// =============================================================================

BENCHMARK(cache_info_access) {
    auto& det = CoreDetector::instance();
    volatile uint32_t result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = det.get_cache_levels();
    }
    DONT_OPTIMIZE(&result);
}

BENCHMARK(online_cpus_access) {
    auto& det = CoreDetector::instance();
    for (std::size_t i = 0; i < iterations; ++i) {
        auto& cpus = det.get_online_cpus();
        DONT_OPTIMIZE(&cpus);
    }
}

// =============================================================================
// 虚拟化检测开销
// =============================================================================

BENCHMARK(virtualization_check) {
    volatile bool result = false;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = CoreDetector::is_virtualized_env();
    }
    DONT_OPTIMIZE(&result);
}

// =============================================================================
// cpuid 开销
// =============================================================================

BENCHMARK(cpuid_call) {
    for (std::size_t i = 0; i < iterations; ++i) {
        auto regs = common::cpuid(0x01);
        DONT_OPTIMIZE(&regs);
    }
}

// =============================================================================
// 主函数
// =============================================================================

int main() {
    std::println("CoreDetector Benchmark v{}\n", benchmark::version());

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
