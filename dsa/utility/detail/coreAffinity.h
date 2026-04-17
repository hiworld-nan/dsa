/**
 * @file coreAffinity.h
 * @brief CPU 亲和性管理工具
 * @version 1.0.0
 *
 * 提供 CPU 亲和性设置与查询：
 * - CpuAffinity::CpuSet: CPU 集合封装
 * - CpuAffinity: 线程/进程亲和性管理
 * - 字符串序列化/反序列化（"0-2,4,7" 格式）
 */

#pragma once

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../../common/constants.h"
#include "../../common/macros.h"

namespace utils {

using namespace common;

// =============================================================================
// CPU 集合
// =============================================================================

/// CPU 集合封装，对 cpu_set_t 的类型安全包装
class CpuSet {
public:
    CpuSet() { CPU_ZERO(&cpuset_); }

    bool add_cpu(std::size_t cpu_id) noexcept {
        if (cpu_id >= CPU_SETSIZE) {
            return false;
        }

        CPU_SET(cpu_id, &cpuset_);
        return true;
    }

    void remove_cpu(std::size_t cpu_id) noexcept {
        if (cpu_id < CPU_SETSIZE) {
            CPU_CLR(cpu_id, &cpuset_);
        }
    }

    [[nodiscard]] bool contains(std::size_t cpu_id) const noexcept {
        if (cpu_id >= CPU_SETSIZE) {
            return false;
        }

        return CPU_ISSET(cpu_id, &cpuset_) != 0;
    }

    void clear() noexcept { CPU_ZERO(&cpuset_); }

    [[nodiscard]] std::size_t count() const noexcept {
        std::size_t cnt = 0;
        for (std::size_t i = 0; i < CPU_SETSIZE; ++i) {
            if (CPU_ISSET(i, &cpuset_)) {
                ++cnt;
            }
        }
        return cnt;
    }

    [[nodiscard]] std::vector<std::size_t> get_cpus() const {
        std::vector<std::size_t> cpus;
        for (std::size_t i = 0; i < CPU_SETSIZE; ++i) {
            if (contains(i)) {
                cpus.push_back(i);
            }
        }
        return cpus;
    }

    [[nodiscard]] const cpu_set_t* native_handle() const noexcept { return &cpuset_; }
    [[nodiscard]] cpu_set_t* native_handle() noexcept { return &cpuset_; }

private:
    cpu_set_t cpuset_{};
};

// =============================================================================
// CPU 亲和性管理
// =============================================================================

/// CPU 亲和性管理工具
struct CpuAffinity {
    /// 获取在线 CPU 数量
    [[nodiscard]] static std::size_t get_available_cpus() noexcept {
        return static_cast<std::size_t>(::sysconf(_SC_NPROCESSORS_ONLN));
    }

    [[nodiscard]] static inline std::thread::id to_std_thread_id(pthread_t ptid) noexcept {
        return std::thread::id(ptid);
    }

    // std::thread::id → pthread_t
    [[nodiscard]] static inline pthread_t to_pthread_id(std::thread::id tid) noexcept {
        return *reinterpret_cast<const pthread_t*>(&tid);
    }

    /// 获取当前线程亲和性
    [[nodiscard]] static std::optional<CpuSet> get_thread_affinity(std::thread::id tid) noexcept {
        CpuSet cpuset;
        if (pthread_getaffinity_np(to_pthread_id(tid), sizeof(cpu_set_t), cpuset.native_handle()) != 0) {
            return std::nullopt;
        }
        return cpuset;
    }

    [[nodiscard]] static std::optional<CpuSet> get_thread_affinity() noexcept {
        return get_thread_affinity(to_std_thread_id(::pthread_self()));
    }

    /// 设置当前线程亲和性
    [[nodiscard]] static bool set_thread_affinity(std::thread::id tid, const CpuSet& cpuset) noexcept {
        if (cpuset.count() == 0) {
            return false;
        }

        return pthread_setaffinity_np(to_pthread_id(tid), sizeof(cpu_set_t), cpuset.native_handle()) == 0;
    }

    [[nodiscard]] static bool set_thread_affinity(const CpuSet& cpuset) noexcept {
        return set_thread_affinity(to_std_thread_id(::pthread_self()), cpuset);
    }

    /// 绑定当前线程到指定 CPU
    [[nodiscard]] static bool pin_to_cpu(std::thread::id tid, std::size_t cpu_id) noexcept {
        if (cpu_id >= get_available_cpus()) {
            return false;
        }

        CpuSet cpuset;
        cpuset.add_cpu(cpu_id);
        return set_thread_affinity(tid, cpuset);
    }

    [[nodiscard]] static bool pin_to_cpu(std::size_t cpu_id) noexcept {
        return pin_to_cpu(to_std_thread_id(::pthread_self()), cpu_id);
    }

    /// 绑定当前线程到指定 CPU 列表
    [[nodiscard]] static bool pin_to_cpus(std::thread::id tid,
                                          const std::vector<std::size_t>& cpu_ids) noexcept {
        CpuSet cpuset;
        const std::size_t available = get_available_cpus();
        for (auto cpu_id : cpu_ids) {
            if (cpu_id >= available) {
                return false;
            }

            cpuset.add_cpu(cpu_id);
        }

        if (cpuset.count() == 0) {
            return false;
        }

        return set_thread_affinity(tid, cpuset);
    }

    [[nodiscard]] static bool pin_to_cpus(const std::vector<std::size_t>& cpu_ids) noexcept {
        return pin_to_cpus(to_std_thread_id(::pthread_self()), cpu_ids);
    }

    /// 重置亲和性（允许所有 CPU）
    [[nodiscard]] static bool reset_affinity(std::thread::id tid) noexcept {
        CpuSet cpuset;
        const std::size_t available = get_available_cpus();
        for (std::size_t i = 0; i < available; ++i) {
            cpuset.add_cpu(i);
        }
        return set_thread_affinity(tid, cpuset);
    }

    [[nodiscard]] static bool reset_affinity() noexcept {
        return reset_affinity(to_std_thread_id(::pthread_self()));
    }

    /// 获取进程亲和性
    [[nodiscard]] static std::optional<CpuSet> get_process_affinity() noexcept {
        CpuSet cpuset;
        if (sched_getaffinity(::getpid(), sizeof(cpu_set_t), cpuset.native_handle()) != 0) {
            return std::nullopt;
        }
        return cpuset;
    }

    /// 设置进程亲和性
    [[nodiscard]] static bool set_process_affinity(const CpuSet& cpuset) noexcept {
        return sched_setaffinity(::getpid(), sizeof(cpu_set_t), cpuset.native_handle()) == 0;
    }

    /// CpuSet 序列化为字符串（如 "0-2,4,7"）
    [[nodiscard]] static std::string to_string(const CpuSet& cpuset) {
        auto cpus = cpuset.get_cpus();
        if (cpus.empty()) {
            return "none";
        }

        std::ostringstream oss;
        std::size_t start = cpus[0];
        std::size_t prev = cpus[0];

        for (std::size_t i = 1; i <= cpus.size(); ++i) {
            if (i == cpus.size() || cpus[i] != prev + 1) {
                if (start == prev) {
                    oss << start;
                } else {
                    oss << start << "-" << prev;
                }

                if (i < cpus.size()) {
                    oss << ",";
                    start = cpus[i];
                }
            }

            if (i < cpus.size()) {
                prev = cpus[i];
            }
        }

        return oss.str();
    }

    /// 从字符串反序列化 CpuSet（如 "0-2,4,7"）
    [[nodiscard]] static CpuSet from_string(const std::string& str) {
        CpuSet cpuset;
        std::istringstream iss(str);
        std::string token;

        while (std::getline(iss, token, ',')) {
            auto dash_pos = token.find('-');
            if (dash_pos == std::string::npos) {
                auto cpu = std::stoul(token);
                cpuset.add_cpu(cpu);
            } else {
                auto s = std::stoul(token.substr(0, dash_pos));
                auto e = std::stoul(token.substr(dash_pos + 1));
                for (auto cpu = s; cpu <= e; ++cpu) {
                    cpuset.add_cpu(cpu);
                }
            }
        }

        return cpuset;
    }
};

}  // namespace utils
