/**
 * @file benchmark_signal.cpp
 * @brief 信号处理性能基准测试
 * @version 1.0.0
 */

#include <atomic>
#include <csignal>
#include <print>
#include <thread>

#include "../benchmark.h"
#include "../utility.h"

using namespace utils;

// =============================================================================
// signalfd open/close 开销
// =============================================================================

BENCHMARK(signalfd_open_close) {
    for (std::size_t i = 0; i < iterations; ++i) {
        SignalHandler h;
        h.on(SIGUSR1, [](const SignalEvent&) {});
        (void)h.open();
        h.close();
    }
}

// =============================================================================
// SignalSet 操作开销
// =============================================================================

BENCHMARK(signal_set_add) {
    for (std::size_t i = 0; i < iterations; ++i) {
        SignalSet s;
        s.add(SIGINT).add(SIGTERM).add(SIGUSR1).add(SIGUSR2);
        DONT_OPTIMIZE(&s);
    }
}

BENCHMARK(signal_set_contains) {
    SignalSet s(SIGINT, SIGTERM, SIGUSR1, SIGUSR2);
    volatile int result = 0;
    for (std::size_t i = 0; i < iterations; ++i) {
        result |= s.contains(SIGINT);
        result |= s.contains(SIGUSR1);
    }
    DONT_OPTIMIZE(&result);
}

// =============================================================================
// signal dispatch 开销
// =============================================================================

BENCHMARK(signal_dispatch_empty) {
    SignalHandler h;
    h.on(SIGUSR1, [](const SignalEvent&) {});
    (void)h.open();

    for (std::size_t i = 0; i < iterations; ++i) {
        // 模拟空轮询（无信号时的开销）
        h.poll();
    }
    h.close();
}

BENCHMARK(signal_dispatch_with_signal) {
    std::atomic<int> counter{0};
    SignalHandler h;
    h.on(SIGUSR1, [&](const SignalEvent&) { counter.fetch_add(1, std::memory_order_relaxed); });
    (void)h.open();

    // 预发送信号
    for (std::size_t i = 0; i < iterations; ++i) {
        raise(SIGUSR1);
        h.poll();
    }
    h.close();
}

// =============================================================================
// signal_name 查表开销
// =============================================================================

BENCHMARK(signal_name_lookup) {
    volatile const char* name = nullptr;
    for (std::size_t i = 0; i < iterations; ++i) {
        name = signal_name(SIGINT);
        name = signal_name(SIGTERM);
        name = signal_name(SIGSEGV);
    }
    DONT_OPTIMIZE(name);
}

// =============================================================================
// SignalGuard 开销
// =============================================================================

BENCHMARK(signal_guard_create_destroy) {
    for (std::size_t i = 0; i < iterations; ++i) {
        SignalGuard guard(SIGUSR1, [](const SignalEvent&) {});
        DONT_OPTIMIZE(&guard);
    }
}

// =============================================================================
// 对比：传统 signal() vs signalfd
// =============================================================================

BENCHMARK(traditional_signal_set_get) {
    for (std::size_t i = 0; i < iterations; ++i) {
        struct sigaction sa {};
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        struct sigaction old;
        sigaction(SIGUSR1, &sa, &old);
        sigaction(SIGUSR1, &old, nullptr);
    }
}

BENCHMARK(pthread_sigmask) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigset_t old;

    for (std::size_t i = 0; i < iterations; ++i) {
        pthread_sigmask(SIG_BLOCK, &set, &old);
        pthread_sigmask(SIG_SETMASK, &old, nullptr);
    }
}

// =============================================================================
// 主函数
// =============================================================================

int main() {
    std::println("Signal Benchmark v{}\n", benchmark::version());

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
