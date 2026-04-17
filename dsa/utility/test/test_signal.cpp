/**
 * @file test_signal.cpp
 * @brief 信号处理单元测试
 * @version 1.0.0
 */

#include <atomic>
#include <chrono>
#include <thread>

#include "../../test/test.h"
#include "../utility.h"

using namespace utils;

// =============================================================================
// SignalSet 测试
// =============================================================================

TEST(SignalSet, Empty) {
    SignalSet s;
    EXPECT_EQ(s.contains(SIGINT), 0);
    EXPECT_EQ(s.contains(SIGTERM), 0);
    return true;
}

TEST(SignalSet, AddAndContains) {
    SignalSet s;
    s.add(SIGINT).add(SIGTERM);
    EXPECT_NE(s.contains(SIGINT), 0);
    EXPECT_NE(s.contains(SIGTERM), 0);
    EXPECT_EQ(s.contains(SIGUSR1), 0);
    return true;
}

TEST(SignalSet, Del) {
    SignalSet s;
    s.add(SIGINT).add(SIGTERM);
    s.del(SIGINT);
    EXPECT_EQ(s.contains(SIGINT), 0);
    EXPECT_NE(s.contains(SIGTERM), 0);
    return true;
}

TEST(SignalSet, VariadicConstructor) {
    SignalSet s(SIGINT, SIGTERM, SIGUSR1);
    EXPECT_NE(s.contains(SIGINT), 0);
    EXPECT_NE(s.contains(SIGTERM), 0);
    EXPECT_NE(s.contains(SIGUSR1), 0);
    EXPECT_EQ(s.contains(SIGUSR2), 0);
    return true;
}

TEST(SignalSet, Presets) {
    auto t = SignalSet::terminate();
    EXPECT_NE(t.contains(SIGINT), 0);
    EXPECT_NE(t.contains(SIGTERM), 0);
    EXPECT_NE(t.contains(SIGQUIT), 0);

    auto g = SignalSet::graceful();
    EXPECT_NE(g.contains(SIGINT), 0);
    EXPECT_NE(g.contains(SIGTERM), 0);
    EXPECT_EQ(g.contains(SIGQUIT), 0);
    return true;
}

TEST(SignalSet, BlockUnblock) {
    SignalSet s(SIGUSR1);
    auto old = s.block();
    // SIGUSR1 现在被阻塞
    // auto old2 = s.unblock();
    // 恢复
    pthread_sigmask(SIG_SETMASK, &old, nullptr);
    return true;
}

// =============================================================================
// signal_name 测试
// =============================================================================

TEST(SignalName, KnownSignals) {
    EXPECT_EQ(std::string(signal_name(SIGINT)), std::string("SIGINT"));
    EXPECT_EQ(std::string(signal_name(SIGTERM)), std::string("SIGTERM"));
    EXPECT_EQ(std::string(signal_name(SIGSEGV)), std::string("SIGSEGV"));
    EXPECT_EQ(std::string(signal_name(SIGKILL)), std::string("SIGKILL"));
    EXPECT_EQ(std::string(signal_name(999)), std::string("UNKNOWN"));
    return true;
}

// =============================================================================
// ignore_signal / default_signal 测试
// =============================================================================

TEST(SignalHelpers, IgnoreAndDefault) {
    ignore_signal(SIGUSR1);
    // SIGUSR1 现在被忽略
    default_signal(SIGUSR1);
    // SIGUSR1 恢复默认
    return true;
}

// =============================================================================
// SignalHandler 测试
// =============================================================================

TEST(SignalHandler, OpenClose) {
    SignalHandler h;
    h.on(SIGUSR1, [](const SignalEvent&) {});
    EXPECT_TRUE(h.open());
    EXPECT_GE(h.fd(), 0);
    h.close();
    EXPECT_EQ(h.fd(), -1);
    return true;
}

TEST(SignalHandler, MoveSemantics) {
    SignalHandler h1;
    h1.on(SIGUSR1, [](const SignalEvent&) {});
    EXPECT_TRUE(h1.open());
    int fd = h1.fd();
    EXPECT_GE(fd, 0);

    SignalHandler h2 = std::move(h1);
    EXPECT_EQ(h1.fd(), -1);  // NOLINT moved-from
    EXPECT_EQ(h2.fd(), fd);

    h2.close();
    return true;
}

TEST(SignalHandler, OnOff) {
    SignalHandler h;
    h.on(SIGUSR1, [](const SignalEvent&) {});
    h.on(SIGUSR2, [](const SignalEvent&) {});
    h.off(SIGUSR1);
    // SIGUSR1 不再注册
    EXPECT_TRUE(h.open());
    h.close();
    return true;
}

TEST(SignalHandler, DispatchSignal) {
    SignalHandler h;
    std::atomic<int> received{0};

    h.on(SIGUSR1, [&](const SignalEvent& ev) {
        EXPECT_EQ(ev.signo_, SIGUSR1);
        received.fetch_add(1, std::memory_order_relaxed);
    });

    EXPECT_TRUE(h.open());

    // 发送信号给自身
    raise(SIGUSR1);

    // 轮询处理
    int n = h.poll();
    EXPECT_EQ(n, 1);
    EXPECT_EQ(received.load(), 1);

    h.close();
    return true;
}

TEST(SignalHandler, MultipleSignals) {
    SignalHandler h;
    std::atomic<int> usr1_count{0};
    std::atomic<int> usr2_count{0};

    const int SIG1 = SIGRTMIN + 1;
    const int SIG2 = SIGRTMIN + 2;

    h.on(SIG1, [&](const SignalEvent& ev) {
        EXPECT_EQ(ev.signo_, SIG1);
        usr1_count.fetch_add(1, std::memory_order_relaxed);
    });
    h.on(SIG2, [&](const SignalEvent& ev) {
        EXPECT_EQ(ev.signo_, SIG2);
        usr2_count.fetch_add(1, std::memory_order_relaxed);
    });

    EXPECT_TRUE(h.open());

    raise(SIG1);
    raise(SIG2);
    raise(SIG1);

    int total = h.poll(10);
    EXPECT_EQ(total, 3);
    EXPECT_EQ(usr1_count.load(), 2);
    EXPECT_EQ(usr2_count.load(), 1);

    h.close();
    return true;
}

TEST(SignalHandler, SignalEventFields) {
    SignalHandler h;
    int code_ = -1;
    pid_t pid_ = -1;
    uid_t uid_ = -1;

    h.on(SIGUSR1, [&](const SignalEvent& ev) {
        code_ = ev.code_;
        pid_ = ev.pid_;
        uid_ = ev.uid_;
    });

    EXPECT_TRUE(h.open());
    raise(SIGUSR1);
    h.poll();

    // raise() 发送的信号 code_ == SI_TKILL 或 SI_USER
    EXPECT_TRUE(code_ == SI_TKILL || code_ == SI_USER);
    EXPECT_EQ(pid_, getpid());
    EXPECT_EQ(uid_, getuid());

    h.close();
    return true;
}

// =============================================================================
// SignalGuard 测试
// =============================================================================

TEST(SignalGuard, RAII) {
    std::atomic<bool> flag{false};

    {
        SignalGuard guard(SIGUSR1, [&](const SignalEvent&) { flag.store(true, std::memory_order_relaxed); });
        EXPECT_GE(guard.fd(), 0);

        raise(SIGUSR1);
        guard.poll();
        EXPECT_TRUE(flag.load());
    }
    // guard 析构后 fd 已关闭
    return true;
}

TEST(SignalGuard, MultipleSignals) {
    std::atomic<int> count{0};

    {
        SignalGuard guard(SignalSet(SIGUSR1, SIGUSR2), [&](const SignalEvent& ev) {
            EXPECT_TRUE(ev.signo_ == SIGUSR1 || ev.signo_ == SIGUSR2);
            count.fetch_add(1, std::memory_order_relaxed);
        });

        raise(SIGUSR1);
        raise(SIGUSR2);
        guard.poll(10);
        EXPECT_EQ(count.load(), 2);
    }
    return true;
}

// =============================================================================
// 主函数
// =============================================================================

int main() { return testing::run_all_tests(); }
