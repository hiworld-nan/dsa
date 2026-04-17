/**
 * @file benchmark_date.cpp
 * @brief 日期工具性能基准测试
 * @version 1.0.0
 */

#include <print>
#include <string>

#include "../benchmark.h"
#include "../utility.h"

using namespace utils;

// =============================================================================
// 日期构造开销
// =============================================================================

BENCHMARK(date_construct) {
    for (std::size_t i = 0; i < iterations; ++i) {
        Date d(2024, 6, 15);
        DONT_OPTIMIZE(&d);
    }
}

// =============================================================================
// JDN 转换开销
// =============================================================================

BENCHMARK(date_to_jdn) {
    Date d(2024, 6, 15);
    for (std::size_t i = 0; i < iterations; ++i) {
        auto jdn = d - Date(1, 1, 1);
        DONT_OPTIMIZE(&jdn);
    }
}

BENCHMARK(date_add_days) {
    Date d(2024, 1, 1);
    for (std::size_t i = 0; i < iterations; ++i) {
        d.add_days(1);
        if (d.year() > 2026)
            d = Date(2024, 1, 1);
    }
    DONT_OPTIMIZE(&d);
}

BENCHMARK(date_add_months) {
    Date d(2024, 1, 1);
    for (std::size_t i = 0; i < iterations; ++i) {
        d.add_months(1);
        if (d.year() > 2026)
            d = Date(2024, 1, 1);
    }
    DONT_OPTIMIZE(&d);
}

// =============================================================================
// day_of_week 开销
// =============================================================================

BENCHMARK(date_day_of_week) {
    Date d(2024, 6, 15);
    volatile int32_t result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = d.day_of_week();
    }
    DONT_OPTIMIZE(&result);
}

BENCHMARK(date_day_of_year) {
    Date d(2024, 6, 15);
    volatile int32_t result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = d.day_of_year();
    }
    DONT_OPTIMIZE(&result);
}

// =============================================================================
// 日期差计算开销
// =============================================================================

BENCHMARK(date_diff) {
    Date a(2024, 1, 1);
    Date b(2024, 12, 31);
    volatile int64_t result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = b - a;
    }
    DONT_OPTIMIZE(&result);
}

// =============================================================================
// 自增/自减开销
// =============================================================================

BENCHMARK(date_increment) {
    Date d(2024, 1, 1);
    for (std::size_t i = 0; i < iterations; ++i) {
        ++d;
        if (d.year() > 2026)
            d = Date(2024, 1, 1);
    }
    DONT_OPTIMIZE(&d);
}

// =============================================================================
// 序列化开销
// =============================================================================

BENCHMARK(date_to_string) {
    Date d(2024, 6, 15);
    for (std::size_t i = 0; i < iterations; ++i) {
        auto s = d.to_string();
        DONT_OPTIMIZE(s.data());
    }
}

BENCHMARK(date_int_conversion) {
    Date d(2024, 6, 15);
    volatile int32_t result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = static_cast<int32_t>(d);
    }
    DONT_OPTIMIZE(&result);
}

// =============================================================================
// is_leap_year 开销
// =============================================================================

BENCHMARK(date_is_leap_year) {
    volatile bool result = false;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = Date::is_leap_year(2024);
        result = Date::is_leap_year(2023);
        result = Date::is_leap_year(2000);
    }
    DONT_OPTIMIZE(&result);
}

// =============================================================================
// 主函数
// =============================================================================

int main() {
    std::println("Date Benchmark v{}\n", benchmark::version());

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
