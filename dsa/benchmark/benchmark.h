#pragma once

#include "detail/core.h"
#include "detail/report.h"
#include "detail/runner.h"
#include "detail/statistics.h"
#include "detail/timer.h"

namespace benchmark {

// =============================================================================
// 基准测试宏定义
// =============================================================================

/**
 * @brief 基础基准测试宏
 * @param Name 测试名称
 * @param Config 测试配置
 */
#define BENCHMARK_BASE(Name, Config)                                                          \
    [[maybe_unused]] static void BM_##Name(::benchmark::IterationCount& iterations);          \
    static const int BM_Reg_##Name = [] {                                                     \
        ::benchmark::Benchmark_Case bm{.name_ = #Name, .func = BM_##Name, .config_ = Config}; \
        ::benchmark::Benchmark_Registry::instance().register_benchmark(std::move(bm));        \
        return 0;                                                                             \
    }();                                                                                      \
    static void BM_##Name([[maybe_unused]] ::benchmark::IterationCount& iterations)

/**
 * @brief 带配置的基准测试
 * @param Name 测试名称
 * @param Config 测试配置
 */
#define BENCHMARK_WITH_CONFIG(Name, Config) BENCHMARK_BASE(Name, Config)

/**
 * @brief 标准基准测试（使用默认配置）
 * @param Name 测试名称
 */
#define BENCHMARK(Name) BENCHMARK_BASE(Name, benchmark::Config::normal())

/**
 * @brief 快速基准测试
 * @param Name 测试名称
 */
#define BENCHMARK_QUICK(Name) BENCHMARK_BASE(Name, benchmark::Config::quick())

/**
 * @brief 精确基准测试
 * @param Name 测试名称
 */
#define BENCHMARK_PRECISE(Name) BENCHMARK_BASE(Name, benchmark::Config::precise())

/**
 * @brief 多线程基准测试
 * @param Name 测试名称
 * @param Threads 线程数
 */
#define BENCHMARK_CONCURRENT(Name, Threads) BENCHMARK_BASE(Name, benchmark::Config::concurrent(Threads))

// =============================================================================
// 带状态的基准测试宏定义
// =============================================================================

/**
 * @brief 带状态的基础基准测试宏
 * @param Name 测试名称
 * @param CaseType 状态类型
 * @param Config 测试配置
 * @param InitArgs 初始化参数（可选）
 */
#define BENCHMARK_F_BASE(Name, CaseType, Config, ...)                                                        \
    struct BenchmarkImpl_##Name : public CaseType {                                                          \
        using CaseType::CaseType;                                                                            \
        void case_body([[maybe_unused]] ::benchmark::IterationCount& iterations);                            \
    };                                                                                                       \
    static BenchmarkImpl_##Name BenchmarkInstance_##Name;                                                    \
    static const int BM_Reg_##Name = [] {                                                                    \
        ::benchmark::Benchmark_Case bm{.name_ = #Name,                                                       \
                                       .func =                                                               \
                                           [](::benchmark::IterationCount iterations) mutable {              \
                                               BenchmarkInstance_##Name.case_body(iterations);               \
                                           },                                                                \
                                       .init = []() mutable { BenchmarkInstance_##Name.init(__VA_ARGS__); }, \
                                       .reset = []() mutable { BenchmarkInstance_##Name.reset(); },          \
                                       .config_ = Config};                                                   \
        ::benchmark::Benchmark_Registry::instance().register_benchmark(std::move(bm));                       \
        return 0;                                                                                            \
    }();                                                                                                     \
    void BenchmarkImpl_##Name::case_body([[maybe_unused]] ::benchmark::IterationCount& iterations)

/**
 * @brief 带状态和配置的基准测试
 * @param Name 测试名称
 * @param CaseType 状态类型
 * @param Config 测试配置
 */
#define BENCHMARK_F_WITH_CONFIG(Name, CaseType, Config) BENCHMARK_F_BASE(Name, CaseType, Config)

/**
 * @brief 带状态的标准基准测试（使用默认配置）
 * @param Name 测试名称
 * @param CaseType 状态类型
 */
#define BENCHMARK_F(Name, CaseType) BENCHMARK_F_BASE(Name, CaseType, benchmark::Config::normal())

/**
 * @brief 带状态和参数的基准测试
 * @param Name 测试名称
 * @param CaseType 状态类型
 * @param ... 初始化参数
 */
#define BENCHMARK_F_WITH_ARGS(Name, CaseType, ...) \
    BENCHMARK_F_BASE(Name, CaseType, benchmark::Config::normal(), __VA_ARGS__)

/**
 * @brief 带状态、配置和参数的基准测试
 * @param Name 测试名称
 * @param CaseType 状态类型
 * @param Config 测试配置
 * @param ... 初始化参数
 */
#define BENCHMARK_F_WITH_CONFIG_AND_ARGS(Name, CaseType, Config, ...) \
    BENCHMARK_F_BASE(Name, CaseType, Config, __VA_ARGS__)

// =============================================================================
// 辅助宏
// =============================================================================

/**
 * @brief 防止编译器优化变量
 * @param x 要保护的变量
 */
#define BENCHMARK_DONT_OPTIMIZE(x) ::benchmark::do_not_optimize(&(x))

inline constexpr int kVersionMajor = 1;
inline constexpr int kVersionMinor = 0;
inline constexpr int kVersionPatch = 0;

[[nodiscard]]
inline const char* version() noexcept {
    return "1.0.0";
}

}  // namespace benchmark