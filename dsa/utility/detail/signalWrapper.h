/**
 * @file signal.h
 * @brief Linux 信号处理工具
 * @version 1.0.0
 *
 * 提供类型安全、异步信号安全的信号处理框架：
 * - SignalHandler: 基于 signalfd + epoll 的高性能信号处理
 * - SignalGuard: RAII 信号作用域管理
 * - SignalSet: 信号集构建器
 *
 * 设计原则：
 * 1. 仅支持 Linux，利用 signalfd 实现信号到文件描述符的统一
 * 2. 异步信号安全：handler 内仅写入 pipe/signalfd，不做复杂操作
 * 3. 与事件循环无缝集成（epoll/io_uring）
 * 4. 零开销抽象，编译期确定信号集
 */

#pragma once

#include <pthread.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <functional>
#include <unordered_map>

namespace utils {

// =============================================================================
// 信号集构建器
// =============================================================================

/// 信号集构建器，支持链式调用
class SignalSet {
public:
    SignalSet() = default;

    /// 从可变参数构造信号集
    template <typename... Sigs>
    explicit SignalSet(int first, Sigs... rest) {
        add(first);
        (add(rest), ...);
    }

    SignalSet& add(int signum) noexcept {
        sigaddset(&set_, signum);
        return *this;
    }

    SignalSet& del(int signum) noexcept {
        sigdelset(&set_, signum);
        return *this;
    }

    [[nodiscard]] bool contains(int signum) const noexcept { return sigismember(&set_, signum); }

    [[nodiscard]] const sigset_t& native() const noexcept { return set_; }
    [[nodiscard]] sigset_t& native() noexcept { return set_; }

    /// 阻塞信号集中的所有信号，返回之前的信号掩码
    [[nodiscard]] sigset_t block() const noexcept {
        sigset_t old;
        pthread_sigmask(SIG_BLOCK, &set_, &old);
        return old;
    }

    /// 解除阻塞信号集中的所有信号
    [[nodiscard]] sigset_t unblock() const noexcept {
        sigset_t old;
        pthread_sigmask(SIG_UNBLOCK, &set_, &old);
        return old;
    }

    /// 设置信号掩码为当前集
    [[nodiscard]] sigset_t set_mask() const noexcept {
        sigset_t old;
        pthread_sigmask(SIG_SETMASK, &set_, &old);
        return old;
    }

    /// 常用信号预设
    static SignalSet terminate() { return SignalSet(SIGINT, SIGTERM, SIGQUIT); }

    static SignalSet graceful() { return SignalSet(SIGINT, SIGTERM); }

    static SignalSet all() {
        SignalSet s;
        sigfillset(&s.set_);
        return s;
    }

private:
    sigset_t set_{};
};

// =============================================================================
// 信号信息
// =============================================================================

/// 信号事件信息
struct SignalEvent {
    int signo_;      ///< 信号编号
    int code_;       ///< 信号来源代码
    int errno_;      ///< 关联的错误号
    pid_t pid_;      ///< 发送进程 PID
    uid_t uid_;      ///< 发送进程 UID
    void* addr_;     ///< 触发地址（SIGSEGV 等）
    int status_;     ///< 退出状态（SIGCHLD）
    int64_t value_;  ///< 信号值（sigval）
};

// =============================================================================
// 信号处理器
// =============================================================================

using SignalCallback = std::function<void(const SignalEvent&)>;

/// 基于 signalfd 的高性能信号处理器
///
/// 使用方式：
///   SignalHandler handler;
///   handler.on(SIGINT, [](auto& ev) { ... });
///   handler.on(SIGTERM, [](auto& ev) { ... });
///   handler.open();
///
///   // 方式1: 手动 poll
///   handler.poll();
///
///   // 方式2: 获取 fd 集成到自己的 epoll 循环
///   int fd = handler.fd();
///   handler.dispatch(signalfd_siginfo);
///
class SignalHandler {
public:
    SignalHandler() { sigemptyset(&mask_); }

    ~SignalHandler() { close(); }

    // 不可拷贝
    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;

    // 可移动
    SignalHandler(SignalHandler&& other) noexcept
        : mask_(other.mask_), callbacks_(std::move(other.callbacks_)), fd_(other.fd_) {
        other.fd_ = -1;
    }

    SignalHandler& operator=(SignalHandler&& other) noexcept {
        if (this != &other) {
            close();
            mask_ = other.mask_;
            callbacks_ = std::move(other.callbacks_);
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    /// 注册信号回调
    SignalHandler& on(int signum, SignalCallback cb) {
        sigaddset(&mask_, signum);
        callbacks_[signum] = std::move(cb);
        return *this;
    }

    /// 移除信号回调
    SignalHandler& off(int signum) {
        sigdelset(&mask_, signum);
        callbacks_.erase(signum);
        return *this;
    }

    /// 打开 signalfd 并阻塞信号
    /// @param nonblock 是否非阻塞模式
    /// @return true 成功
    [[nodiscard]] bool open(bool nonblock = true) {
        // 阻塞信号，防止默认处理
        pthread_sigmask(SIG_BLOCK, &mask_, &old_mask_);

        int flags = SFD_CLOEXEC | (nonblock ? SFD_NONBLOCK : 0);
        fd_ = signalfd(-1, &mask_, flags);
        return fd_ >= 0;
    }

    /// 关闭 signalfd 并恢复信号掩码
    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
            // 恢复信号掩码
            pthread_sigmask(SIG_SETMASK, &old_mask_, nullptr);
        }
    }

    /// 获取 signalfd 文件描述符（用于集成到 epoll）
    [[nodiscard]] int fd() const noexcept { return fd_; }

    /// 获取当前信号掩码
    [[nodiscard]] const sigset_t& mask() const noexcept { return mask_; }

    /// 轮询并分发信号事件
    /// @param max_events 单次最多处理的事件数
    /// @return 处理的事件数，-1 表示出错
    int poll(int max_events = 32) {
        if (fd_ < 0) {
            return -1;
        }

        int total = 0;
        constexpr size_t BUF_SIZE = 32;
        std::array<signalfd_siginfo, BUF_SIZE> buf{};

        while (total < max_events) {
            const int remaining = max_events - total;
            const int to_read = std::min(static_cast<int>(BUF_SIZE), remaining);

            ssize_t bytes_read =
                ::read(fd_, buf.data(), static_cast<size_t>(to_read) * sizeof(signalfd_siginfo));

            if (bytes_read > 0) {
                // 处理读取到的信号
                const int count = static_cast<int>(bytes_read / sizeof(signalfd_siginfo));
                for (int i = 0; i < count; ++i) {
                    dispatch(buf[i]);
                }
                total += count;
            } else {
                // 非阻塞模式：无数据/被中断，退出循环
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                    break;
                }
                // 致命错误
                return -1;
            }
        }

        return total;
    }

    /// 分发单个 signalfd_siginfo 事件（用于外部 epoll 集成）
    void dispatch(const struct signalfd_siginfo& info) {
        int signo = static_cast<int>(info.ssi_signo);
        auto it = callbacks_.find(signo);
        if (it != callbacks_.end()) {
            SignalEvent ev{
                .signo_ = signo,
                .code_ = static_cast<int>(info.ssi_code),
                .errno_ = static_cast<int>(info.ssi_errno),
                .pid_ = static_cast<pid_t>(info.ssi_pid),
                .uid_ = static_cast<uid_t>(info.ssi_uid),
                .addr_ = reinterpret_cast<void*>(info.ssi_addr),
                .status_ = static_cast<int>(info.ssi_status),
                .value_ = static_cast<int64_t>(info.ssi_int),
            };
            it->second(ev);
        }
    }

private:
    sigset_t mask_{};
    sigset_t old_mask_{};
    std::unordered_map<int, SignalCallback> callbacks_;
    int fd_ = -1;
};

// =============================================================================
// 信号守护（RAII）
// =============================================================================

/// RAII 信号作用域管理
/// 构造时注册回调并阻塞信号，析构时恢复
///
/// 使用方式：
///   {
///       SignalGuard guard(SIGINT, [](auto&) { g_running = false; });
///       // ... 在此作用域内 SIGINT 由 guard 处理
///   }
///   // 离开作用域后 SIGINT 恢复默认行为
///
class SignalGuard {
public:
    /// 单信号 + 回调
    SignalGuard(int signum, SignalCallback cb) : handler_() {
        handler_.on(signum, std::move(cb));
        (void)handler_.open();
    }

    /// 多信号共用同一回调（通过 signal_set）
    SignalGuard(const SignalSet& set, SignalCallback cb) : handler_() {
        for (int signum = 1; signum < NSIG; ++signum) {
            if (set.contains(signum)) {
                handler_.on(signum, cb);  // std::function 复制
            }
        }
        (void)handler_.open();
    }

    ~SignalGuard() = default;

    SignalGuard(const SignalGuard&) = delete;
    SignalGuard& operator=(const SignalGuard&) = delete;

    SignalGuard(SignalGuard&&) noexcept = default;
    SignalGuard& operator=(SignalGuard&&) noexcept = default;

    /// 获取底层 handler
    [[nodiscard]] SignalHandler& handler() noexcept { return handler_; }
    [[nodiscard]] const SignalHandler& handler() const noexcept { return handler_; }

    /// 获取 signalfd（用于集成到事件循环）
    [[nodiscard]] int fd() const noexcept { return handler_.fd(); }

    /// 轮询信号事件
    int poll(int max_events = 32) { return handler_.poll(max_events); }

private:
    SignalHandler handler_;
};

// =============================================================================
// 便捷函数
// =============================================================================

/// 设置忽略指定信号
inline void ignore_signal(int signum) {
    struct sigaction sa {};
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(signum, &sa, nullptr);
}

/// 恢复信号默认处理
inline void default_signal(int signum) {
    struct sigaction sa {};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(signum, &sa, nullptr);
}

/// 发送信号给自身
inline void raise_signal(int signum) { raise(signum); }

/// 获取信号名称
[[nodiscard]] inline const char* signal_name(int signum) noexcept {
#define CASE_SIG(s) \
    case s: return #s
    switch (signum) {
        CASE_SIG(SIGHUP);
        CASE_SIG(SIGINT);
        CASE_SIG(SIGQUIT);
        CASE_SIG(SIGILL);
        CASE_SIG(SIGTRAP);
        CASE_SIG(SIGABRT);
        CASE_SIG(SIGBUS);
        CASE_SIG(SIGFPE);
        CASE_SIG(SIGKILL);
        CASE_SIG(SIGUSR1);
        CASE_SIG(SIGSEGV);
        CASE_SIG(SIGUSR2);
        CASE_SIG(SIGPIPE);
        CASE_SIG(SIGALRM);
        CASE_SIG(SIGTERM);
        CASE_SIG(SIGCHLD);
        CASE_SIG(SIGCONT);
        CASE_SIG(SIGSTOP);
        CASE_SIG(SIGTSTP);
        CASE_SIG(SIGTTIN);
        CASE_SIG(SIGTTOU);
    default: return "UNKNOWN";
    }
#undef CASE_SIG
}

}  // namespace utils
