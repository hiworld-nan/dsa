/**
 * @file benchmark_overloaded.cpp
 * @brief overloaded 模板性能基准测试
 * @version 1.0.0
 */

#include <print>
#include <string>
#include <variant>

#include "../benchmark.h"
#include "../utility.h"

using namespace utils;

// =============================================================================
// overloaded 构造开销
// =============================================================================

BENCHMARK(overloaded_construct_2) {
    for (std::size_t i = 0; i < iterations; ++i) {
        auto f = overloaded{[](int x) { return x * 2; }, [](double x) { return x * 3.0; }};
        DONT_OPTIMIZE(&f);
    }
}

BENCHMARK(overloaded_construct_3) {
    for (std::size_t i = 0; i < iterations; ++i) {
        auto f = overloaded{[](int x) { return x; }, [](double x) { return x; },
                            [](const std::string& s) { return s.size(); }};
        DONT_OPTIMIZE(&f);
    }
}

// =============================================================================
// overloaded 调用开销 vs 普通函数
// =============================================================================

BENCHMARK(overloaded_call_int) {
    auto f = overloaded{[](int x) { return x * 2; }, [](double x) { return x * 3.0; }};
    volatile int result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = f(42);
    }
    DONT_OPTIMIZE(&result);
}

BENCHMARK(plain_lambda_call_int) {
    auto f = [](int x) { return x * 2; };
    volatile int result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = f(42);
    }
    DONT_OPTIMIZE(&result);
}

// =============================================================================
// std::visit + overloaded 开销
// =============================================================================

using Variant3 = std::variant<int, double, std::string>;

BENCHMARK(visit_overloaded) {
    auto visitor = overloaded{[](int i) { return i; }, [](double d) { return static_cast<int>(d); },
                              [](const std::string& s) { return static_cast<int>(s.size()); }};

    Variant3 v = 42;
    volatile int result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        result = std::visit(visitor, v);
    }
    DONT_OPTIMIZE(&result);
}

BENCHMARK(visit_switch) {
    Variant3 v = 42;
    volatile int result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        switch (v.index()) {
        case 0: result = std::get<0>(v); break;
        case 1: result = static_cast<int>(std::get<1>(v)); break;
        case 2: result = static_cast<int>(std::get<2>(v).size()); break;
        }
    }
    DONT_OPTIMIZE(&result);
}

// =============================================================================
// 主函数
// =============================================================================

int main() {
    std::println("Overloaded Benchmark v{}\n", benchmark::version());

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
