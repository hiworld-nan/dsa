/**
 * @file coreDetector.h
 * @brief CPU 特性检测与拓扑发现
 * @version 1.0.0
 *
 * 提供 CPU 架构、缓存、指令集、NUMA 拓扑检测：
 * - Feature: CPU 特性枚举（对应 CPUID 位索引）
 * - CoreDetector: CPU 信息单例检测器
 * - CacheInfo: 缓存层级信息
 *
 * 仅支持 Linux x86_64
 */

#pragma once

#if __has_include(<numa.h>)
#include <numa.h>
#endif

#include <algorithm>
#include <bitset>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "../../common/intrinsics.h"
#include "../../common/singleton.h"

namespace utils {

using namespace common;

namespace fs = std::filesystem;

// =============================================================================
// 枚举定义
// =============================================================================

enum class CPUArch : uint8_t { INTEL = 0, AMD, UNKNOWN };
enum class CacheType : uint8_t { UNKNOWN = 0, DATA, INSTRUCTION, UNIFIED };

// =============================================================================
// CPU 特性枚举
// =============================================================================
///    https://en.wikipedia.org/wiki/CPUID
/// CPU 特性，值对应 CPUID 位索引
/// 分组基准值    对应 CPUID 寄存器/功能              范围
///     0        Leaf 1 EDX (基本特性)                0-31
///    32        Leaf 1 ECX (扩展特性)                32-63
///    64        Leaf 7 EBX (高级特性)                64-95
///    96        Leaf 7 ECX (更多扩展)                96-127
///   128        Leaf 7 EDX (安全/特殊特性)           128-159
///   160        AMD 扩展 (Leaf 80000001h EDX)        160-191
///   192        AMD 扩展 (Leaf 80000001h ECX)        192-223
enum class Feature : uint16_t {
    // ===== 第一组: 基本特性 (CPUID Leaf 1 EDX寄存器位0-31) =====
    FPU = 0,   // 浮点处理单元（Floating Point Unit）
    VME = 1,   // 虚拟8086模式扩展（Virtual 8086 Mode Extensions）
    DE = 2,    // 调试扩展（Debugging Extensions）
    PSE = 3,   // 页面大小扩展（Page Size Extension） - 支持4MB页面
    TSC = 4,   // 时间戳计数器（Time Stamp Counter）
    MSR = 5,   // 型号特定寄存器（Model-Specific Registers）
    PAE = 6,   // 物理地址扩展（Physical Address Extension） - 支持超过4GB内存
    MCE = 7,   // 机器检查异常（Machine Check Exception）
    CX8 = 8,   // CMPXCHG8B指令 - 8字节比较交换
    APIC = 9,  // 高级可编程中断控制器（Advanced Programmable Interrupt Controller）
    // 位10: 保留
    SEP = 11,    // SYSENTER/SYSEXIT指令 - 快速系统调用
    MTRR = 12,   // 内存类型范围寄存器（Memory Type Range Registers）
    PGE = 13,    // 全局页面（Page Global Enable） - TLB优化
    MCA = 14,    // 机器检查架构（Machine Check Architecture）
    CMOV = 15,   // 条件移动指令（Conditional Move Instructions）
    PAT = 16,    // 页面属性表（Page Attribute Table）
    PSE36 = 17,  // 36位页面大小扩展
    PSN = 18,    // 处理器序列号（Processor Serial Number） - 已弃用
    CLFSH = 19,  // CLFLUSH指令 - 缓存行刷新
    // 位20: 保留
    DS = 21,    // 调试存储（Debug Store） - 分支跟踪存储
    ACPI = 22,  // 热监控和时钟控制（Thermal Monitor and Clock Control）
    MMX = 23,   // MMX多媒体扩展（MultiMedia eXtensions）
    FXSR = 24,  // FXSAVE/FXRSTOR指令 - 快速浮点状态保存/恢复
    SSE = 25,   // 流式SIMD扩展（Streaming SIMD Extensions）
    SSE2 = 26,  // SSE2扩展
    SS = 27,    // 自调度（Self-Snoop） - 缓存优化
    HTT = 28,   // 超线程技术（Hyper-Threading Technology）
    TM = 29,    // 热监控（Thermal Monitor）
    IA64 = 30,  // IA64处理器仿真（已弃用）
    PBE = 31,   // 待启用中断（Pending Break Enable）

    // ===== 第二组: 扩展特性 (CPUID Leaf 1 ECX寄存器位0-31) =====
    // 基准值: 32（对应ECX寄存器位索引）
    SSE3 = 32 + 0,       // SSE3指令集
    PCLMULQDQ = 32 + 1,  // PCLMULQDQ指令 - 无进位乘法
    DTES64 = 32 + 2,     // 64位调试存储（Debug Trace Store）
    MONITOR = 32 + 3,    // MONITOR/MWAIT指令 - 节能和同步
    DS_CPL = 32 + 4,     // CPL限定的调试存储
    VMX = 32 + 5,        // 虚拟化技术（Intel VT-x）
    SMX = 32 + 6,        // 安全模式扩展（Safer Mode Extensions） - Intel TXT
    EST = 32 + 7,        // 增强型SpeedStep技术
    TM2 = 32 + 8,        // 热监控2（Thermal Monitor 2）
    SSSE3 = 32 + 9,      // 补充SSE3指令集
    CID = 32 + 10,       // 上下文ID（Context ID） - L1缓存优化
    SDBG = 32 + 11,      // Silicon Debug功能
    FMA = 32 + 12,       // 融合乘加指令（Fused Multiply-Add） - AVX2
    CX16 = 32 + 13,      // CMPXCHG16B指令 - 16字节比较交换
    XTPR = 32 + 14,      // 发送任务优先级寄存器（Send Task Priority Messages）
    PDCM = 32 + 15,      // 性能和调试能力（Perf/Debug Capability）
    // 位16: 保留
    PCID = 32 + 17,          // 进程上下文标识符（Process Context Identifiers）
    DCA = 32 + 18,           // 直接缓存访问（Direct Cache Access）
    SSE4_1 = 32 + 19,        // SSE4.1指令集
    SSE4_2 = 32 + 20,        // SSE4.2指令集
    X2APIC = 32 + 21,        // 扩展APIC（x2APIC）
    MOVBE = 32 + 22,         // 字节交换移动指令（Move Big Endian）
    POPCNT = 32 + 23,        // 位计数指令（Population Count）
    TSC_DEADLINE = 32 + 24,  // 时间戳计数器期限（TSC Deadline）
    AES = 32 + 25,           // AES指令集（AES-NI）
    XSAVE = 32 + 26,         // XSAVE/XRSTOR指令 - 扩展状态保存/恢复
    OSXSAVE = 32 + 27,       // 操作系统支持XSAVE
    AVX = 32 + 28,           // 高级矢量扩展（Advanced Vector Extensions）
    F16C = 32 + 29,          // 16位浮点转换指令
    RDRAND = 32 + 30,        // 随机数生成指令
    HYPERVISOR = 32 + 31,    // 虚拟化特性（CPU运行在hypervisor下）

    // ===== 第三组: 高级特性 (CPUID Leaf 7 EBX寄存器位0-31) =====
    // 基准值: 64
    FSGSBASE = 64 + 0,  // FS/GS基址寄存器访问指令
    // 位1-2: 保留
    BMI1 = 64 + 3,  // 位操作指令集1（Bit Manipulation Instructions 1）
    HLE = 64 + 4,   // 硬件锁省略（Hardware Lock Elision）
    AVX2 = 64 + 5,  // 高级矢量扩展2
    // 位6: 保留
    SMEP = 64 + 7,      // 监督模式执行保护（Supervisor Mode Execution Protection）
    BMI2 = 64 + 8,      // 位操作指令集2
    ERMS = 64 + 9,      // 增强型REP MOVSB/STOSB（Enhanced REP MOVSB/STOSB）
    INVPCID = 64 + 10,  // 无效进程上下文ID指令（INVLPCID）
    RTM = 64 + 11,      // 受限制的事务内存（Restricted Transactional Memory）
    // 位12-15: 保留
    AVX512F = 64 + 16,   // AVX-512基础指令集
    AVX512DQ = 64 + 17,  // AVX-512双字和四字指令
    RDSEED = 64 + 18,    // RDSEED指令 - 随机种子生成
    ADX = 64 + 19,       // 多精度加法进位扩展（ADCX/ADOX）
    // 位20: 保留
    AVX512IFMA = 64 + 21,  // AVX-512整数融合乘加指令
    PCOMMIT = 64 + 22,     // PCOMMIT指令 - 持久内存提交（已弃用）
    CLFLUSHOPT = 64 + 23,  // 优化的缓存行刷新指令
    CLWB = 64 + 24,        // 缓存行写回指令
    // 位25: 保留
    AVX512PF = 64 + 26,  // AVX-512预取指令
    AVX512ER = 64 + 27,  // AVX-512指数和倒数指令
    AVX512CD = 64 + 28,  // AVX-512冲突检测指令
    SHA = 64 + 29,       // SHA指令集（SHA-NI）
    AVX512BW = 64 + 30,  // AVX-512字节和字指令
    AVX512VL = 64 + 31,  // AVX-512矢量长度扩展

    // ===== 第四组: 更多扩展特性 (CPUID Leaf 7 ECX寄存器位0-31) =====
    // 基准值: 96
    PREFETCHWT1 = 96 + 0,  // PREFETCHWT1指令 - 线程感知预取
    AVX512VBMI = 96 + 1,   // AVX-512矢量字节操作指令
    UMIP = 96 + 2,         // 用户模式指令防护（User-Mode Instruction Prevention）
    PKU = 96 + 3,          // 内存保护密钥（Memory Protection Keys）
    OSPKE = 96 + 4,        // 操作系统支持PKU
    // 位5: 保留
    AVX512VBMI2 = 96 + 6,  // AVX-512矢量字节操作指令2
    // 位7: 保留
    GFNI = 96 + 8,  // Galois域新指令（Galois Field New Instructions）
    VAES = 96 + 9,  // 矢量AES指令
    // 位10: 保留
    AVX512VNNI = 96 + 11,    // AVX-512矢量神经网络指令
    AVX512BITALG = 96 + 12,  // AVX-512位算法指令
    // 位13: 保留
    AVX512VPOPCNTDQ = 96 + 14,  // AVX-512位计数指令
    // 位15-21: 保留
    RDPID = 96 + 22,  // 读取处理器ID指令（Read Processor ID）
    // 位23-31: 保留

    // ===== 第五组: 安全/特殊特性 (CPUID Leaf 7 EDX寄存器位0-31) =====
    // 基准值: 128
    // 位0-1: 保留
    AVX5124VNNIW = 128 + 2,  // AVX-512 4路神经网络指令
    AVX5124FMAPS = 128 + 3,  // AVX-512 4路浮点累加指令
    FSRM = 128 + 4,          // 快速短REP MOV（Fast Short REP MOV）
    // 位5-7: 保留
    AVX512VP2INTERSECT = 128 + 8,  // AVX-512矢量对交集指令
    SRBDS_CTRL = 128 + 9,          // 特殊寄存器缓冲区数据采样控制
    MD_CLEAR = 128 + 10,           // 微码更新清除缓冲
    // 位11-14: 保留
    HYBRID = 128 + 15,  // 混合CPU架构标识
    // 位16-25: 保留
    IBPB = 128 + 26,       // 间接分支预测屏障（Indirect Branch Prediction Barrier）
    STIBP = 128 + 27,      // 单线程间接分支预测（Single Thread Indirect Branch Predictors）
    L1D_FLUSH = 128 + 28,  // L1数据缓存刷新指令
    ARCH_CAP = 128 + 29,   // 架构能力（Architecture Capabilities）
    CORE_CAP = 128 + 30,   // 核心能力（Core Capabilities）
    SSBD = 128 + 31,       // 投机存储旁路禁用（Speculative Store Bypass Disable）

    // ===== 第六组: AMD扩展特性 (CPUID Leaf 80000001h EDX寄存器位0-31) =====
    // 基准值: 160
    // 位0-10: 保留
    SYSCALL = 160 + 11,  // SYSCALL/SYSRET指令（AMD64长模式系统调用）
    // 位12-19: 保留
    NX = 160 + 20,  // 不可执行位（No-Execute Page Protection） - 数据执行保护
    // 位21: 保留
    MMXEXT = 160 + 22,  // AMD扩展MMX指令
    // 位23-26: 保留
    RDTSCP = 160 + 27,  // 读取时间戳计数器与处理器ID
    // 位28: 保留
    LM = 160 + 29,         // 长模式（Long Mode） - AMD64 64位模式
    _3DNOWEXT = 160 + 30,  // AMD扩展3DNow!指令
    _3DNOW = 160 + 31,     // AMD 3DNow!指令

    // ===== 第七组: AMD更多扩展特性 (CPUID Leaf 80000001h ECX寄存器位0-31) =====
    // 基准值: 192
    LAHF_LM = 192 + 0,         // LAHF/SAHF在长模式下可用
    CMP_LEGACY = 192 + 1,      // 核心多处理传统模式（Legacy mode for multi-core）
    SVM = 192 + 2,             // 安全虚拟机（Secure Virtual Machine） - AMD-V虚拟化
    EXTAPIC = 192 + 3,         // 扩展APIC空间（Extended APIC space）
    CR8_LEGACY = 192 + 4,      // CR8在传统模式下可用
    ABM = 192 + 5,             // 高级位操作（Advanced Bit Manipulation） - LZCNT/POPCNT
    SSE4A = 192 + 6,           // AMD SSE4A指令集
    MISALIGNSSE = 192 + 7,     // 不对齐SSE模式（Misaligned SSE mode）
    _3DNOWPREFETCH = 192 + 8,  // 3DNow!预取指令
    OSVW = 192 + 9,            // 操作系统可见工作区（OS Visible Workaround）
    IBS = 192 + 10,            // 指令基于采样（Instruction Based Sampling）
    XOP = 192 + 11,            // 扩展操作指令集（eXtended Operations）
    SKINIT = 192 + 12,         // SKINIT指令 - 安全启动
    WDT = 192 + 13,            // 看门狗定时器（Watchdog Timer）
    // 位14: 保留
    LWP = 192 + 15,   // 轻量级性能监控（Lightweight Profiling）
    FMA4 = 192 + 16,  // 4操作数融合乘加指令
    TCE = 192 + 17,   // 转换缓存扩展（Translation Cache Extension）
    // 位18: 保留
    NODEID_MSR = 192 + 19,  // 节点ID MSR（Node ID MSR）
    // 位20: 保留
    TBM = 192 + 21,           // 尾位操作指令（Trailing Bit Manipulation）
    TOPOEXT = 192 + 22,       // 拓扑扩展（Topology Extensions）
    PERFCTR_CORE = 192 + 23,  // 核心性能计数器（Core Performance Counter Extensions）
    PERFCTR_NB = 192 + 24,    // 北桥性能计数器（NorthBridge Performance Counter Extensions）
    STREAM_PERF = 192 + 25,   // 流性能监控（Data Streaming Performance Monitor）
    DBX = 192 + 26,           // 调试扩展（Debug Extensions）
    PTSC = 192 + 27,          // 每核心时间戳计数器（Perf Time Stamp Counter）
    PERFTSC = 192 + 28,       // 性能时间戳计数器（Performance Time Stamp Counter）
    MWAITX = 192 + 29,        // MWAIT扩展指令
    // 位30-31: 保留

    // 特殊值：枚举成员总数，用于分配数组大小或遍历
    // 注意：这是枚举值，不是特性。实际值为MWAITX+1 = 222
    COUNT
};

// =============================================================================
// NUMA 特性
// =============================================================================

struct NumaFeatures {
#if __has_include(<numa.h>)
    static constexpr bool kNumactlSupported = true;
#else
    static constexpr bool kNumactlSupported = false;
#endif
};

// =============================================================================
// 缓存信息
// =============================================================================

struct CacheInfo {
    CacheType type_{CacheType::UNKNOWN};
    uint32_t level_{0};
    uint32_t size_{0};  // 字节
    uint32_t ways_{0};
    uint32_t line_size_{0};  // 字节

    CacheInfo(CacheType type, uint32_t level, uint32_t size, uint32_t ways, uint32_t line_size)
        : type_(type), level_(level), size_(size), ways_(ways), line_size_(line_size) {}
};

// =============================================================================
// CPU 检测器
// =============================================================================

/// CPU 特性检测与拓扑发现（单例）
class CoreDetector : public common::singleton<CoreDetector> {
    friend class common::singleton<CoreDetector>;
    static constexpr size_t kFeatureCount = static_cast<size_t>(Feature::COUNT);

public:
    CoreDetector(CoreDetector&&) = delete;
    CoreDetector(const CoreDetector&) = delete;
    CoreDetector& operator=(CoreDetector&&) = delete;
    CoreDetector& operator=(const CoreDetector&) = delete;

    [[nodiscard]] inline bool has(Feature f) const noexcept {
        const size_t idx = static_cast<size_t>(f);
        return idx < kFeatureCount && features_[idx];
    }

    [[nodiscard]] inline CPUArch get_arch() const noexcept { return arch_; }
    [[nodiscard]] inline uint32_t get_num_of_threads() const noexcept { return num_threads_; }
    [[nodiscard]] inline uint32_t get_num_of_numa_nodes() const noexcept { return num_numa_nodes_; }
    [[nodiscard]] inline uint32_t get_threads_per_core() const noexcept { return threads_per_core_; }
    [[nodiscard]] inline uint32_t get_cache_levels() const noexcept {
        return static_cast<uint32_t>(caches_.size());
    }

    [[nodiscard]] inline bool support_sse() const noexcept { return has(Feature::SSE); }
    [[nodiscard]] inline bool support_avx() const noexcept { return has(Feature::AVX); }
    [[nodiscard]] inline bool support_avx2() const noexcept { return has(Feature::AVX2); }
    [[nodiscard]] inline bool support_avx512() const noexcept { return has(Feature::AVX512F); }
    [[nodiscard]] inline bool support_hyper_thread() const noexcept { return has(Feature::HTT); }
    [[nodiscard]] inline bool is_ht_enabled() const noexcept { return ht_enabled_; }

    [[nodiscard]] inline const std::vector<CacheInfo>& get_cache_info() const noexcept { return caches_; }
    [[nodiscard]] inline const CacheInfo& get_cache_info(uint32_t idx) const noexcept {
        return caches_.empty() ? empty_cache_ : caches_[idx % caches_.size()];
    }

    [[nodiscard]] inline bool is_numa_aware() const noexcept { return num_numa_nodes_ > 1; }
    [[nodiscard]] inline const std::set<int32_t>& get_online_cpus() const noexcept { return online_cpus_; }
    [[nodiscard]] inline const std::set<int32_t>& get_isolated_cpus() const noexcept {
        return isolated_cpus_;
    }
    [[nodiscard]] inline const std::set<int32_t>& get_cpulist(uint32_t node) const noexcept {
        return numa_cpus_.empty() ? empty_set_ : numa_cpus_[node % num_numa_nodes_];
    }
    [[nodiscard]] inline const std::vector<std::set<int32_t>>& get_cpulists() const noexcept {
        return numa_cpus_;
    }

    [[nodiscard]] static inline bool is_virtualized_env() noexcept {
        return (common::cpuid(0x01).ecx & (1U << 31)) != 0;
    }

    friend std::ostream& operator<<(std::ostream& out, const CoreDetector& detector) {
        out << "CPU Architecture: "
            << (detector.arch_ == CPUArch::INTEL ? "Intel\n"
                : detector.arch_ == CPUArch::AMD ? "AMD\n"
                                                 : "Unknown\n")
            << "    Has AVX: " << (detector.has(Feature::AVX) ? "Yes" : "No") << "\n"
            << "    Has AVX2: " << (detector.has(Feature::AVX2) ? "Yes" : "No") << "\n"
            << "    Has AVX-512: " << (detector.has(Feature::AVX512F) ? "Yes" : "No") << "\n"
            << "    Support Hyper Thread: " << (detector.support_ht_ ? "Yes" : "No") << "\n"
            << "    Is HT enabled: " << (detector.ht_enabled_ ? "Yes" : "No") << "\n"
            << "    Is virtualized env: " << (is_virtualized_env() ? "Yes" : "No") << "\n"
            << "    Threads per core: " << detector.threads_per_core_ << "\n"
            << "    Number of Threads: " << detector.num_threads_ << "\n"
            << "    Number of NUMA Nodes: " << detector.num_numa_nodes_ << "\n";

        auto print_cpu_set = [&](std::string_view name, const std::set<int32_t>& s) {
            if (s.empty()) {
                return;
            }

            out << "    " << name << ": ";
            for (auto it = s.begin(); it != s.end(); ++it) {
                if (it != s.begin()) {
                    out << ",";
                }
                out << *it;
            }
            out << "\n";
        };

        print_cpu_set("Online CPUs", detector.online_cpus_);
        print_cpu_set("Isolated CPUs", detector.isolated_cpus_);

        out << "    Cache Info:\n"
            << "    Cache Levels: " << detector.cache_levels_ << "\n";
        for (const auto& cache : detector.caches_) {
            out << "        Level " << cache.level_ << ": " << cache.size_ / 1024 << " KB, " << cache.ways_
                << " ways, " << cache.line_size_ << " bytes cache line size\n";
        }

        const auto& cpulists = detector.numa_cpus_;
        if (cpulists.size() > 0) {
            out << "    NUMA Nodes:\n";
            for (uint32_t i = 0; i < cpulists.size(); ++i) {
                out << "        Node" << i << "\'s CPU:";
                uint32_t j = 0;
                for (const auto& v : cpulists[i]) {
                    out << v << (++j == cpulists[i].size() ? "" : ",");
                }
                out << "\n";
            }
        }
        return out;
    }

private:
    CoreDetector() {
        detect_cpu();
        detect_cache();
        detect_numa();
        num_threads_ = static_cast<uint32_t>(std::thread::hardware_concurrency());
        if (num_numa_nodes_ == 0) {
            num_numa_nodes_ = 1;
        }
    }

    void detect_cpu() {
        analyze_vendor();
        detect_features();

        if (auto opt = parse_cpu_list_file("/sys/devices/system/cpu/isolated")) {
            isolated_cpus_ = std::move(*opt);
        }
        if (auto opt = parse_cpu_list_file("/sys/devices/system/cpu/online")) {
            online_cpus_ = std::move(*opt);
        }

        detect_hyper_thread();
    }

    void detect_cache() {
        for (uint32_t i = 0;; ++i) {
            fs::path path("/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i));
            if (!fs::exists(path)) {
                break;
            }

            auto type = read_file<std::string>(path / "type", "");
            CacheType ct = CacheType::UNKNOWN;
            if (type == "Data") {
                ct = CacheType::DATA;
            } else if (type == "Unified") {
                ct = CacheType::UNIFIED;
            } else if (type == "Instruction") {
                ct = CacheType::INSTRUCTION;
            }

            if (ct == CacheType::DATA || ct == CacheType::UNIFIED) {
                caches_.emplace_back(ct, read_file<uint32_t>(path / "level", 0),
                                     read_file<uint32_t>(path / "size", 0) * 1024,
                                     read_file<uint32_t>(path / "ways_of_associativity", 0),
                                     read_file<uint32_t>(path / "coherency_line_size", 0));
            }
        }
        cache_levels_ = static_cast<uint32_t>(caches_.size());
    }

    void detect_numa() {
        int32_t i = 0;
        while (true) {
            auto path = fs::path("/sys/devices/system/node/node" + std::to_string(i++) + "/cpulist");
            if (!fs::exists(path)) {
                break;
            }

            std::string cpulist = read_file<std::string>(path, "");
            if (!cpulist.empty()) {
                num_numa_nodes_++;
                numa_cpus_.emplace_back(parse_cpulist(cpulist));
            }
        }
    }

    void detect_hyper_thread() {
        const auto& id = common::cpuid(0x01);
        support_ht_ = (id.edx & (1 << 28)) != 0;
        if (support_ht_) {
            threads_per_core_ = (id.ebx >> 16) & 0xFF;
            ht_enabled_ = threads_per_core_ > 1;
        }
    }

    void analyze_vendor() {
        const auto id = common::cpuid(0);
        max_basic_leaf_ = id.eax;

        char vendor[13] = {0};
        std::memcpy(&vendor[0], &id.ebx, 4);
        std::memcpy(&vendor[4], &id.edx, 4);
        std::memcpy(&vendor[8], &id.ecx, 4);

        if (std::strcmp(vendor, "GenuineIntel") == 0) {
            arch_ = CPUArch::INTEL;
        } else if (std::strcmp(vendor, "AuthenticAMD") == 0) {
            arch_ = CPUArch::AMD;
        } else {
            arch_ = CPUArch::UNKNOWN;
        }

        max_extended_leaf_ = common::cpuid(0x80000000).eax;
    }

    void detect_features() noexcept {
        if (max_basic_leaf_ < 1) {
            return;
        }

        const auto& r1 = common::cpuid(0x01);
        set_features(r1.edx, 0);
        set_features(r1.ecx, 32);

        if (max_basic_leaf_ >= 7) {
            const auto& r7 = common::cpuid(0x07);
            set_features(r7.ebx, 64);
            set_features(r7.ecx, 96);
            set_features(r7.edx, 128);
        }

        if (max_extended_leaf_ >= 0x80000001) {
            const auto& re = common::cpuid(0x80000001);
            set_features(re.edx, 160);
            set_features(re.ecx, 192);
        }
    }

    void set_features(uint32_t val, uint16_t base) noexcept {
        for (int i = 0; i < 32; ++i) {
            if (val & (1u << i)) {
                auto f = static_cast<Feature>(base + i);
                if (static_cast<size_t>(f) < features_.size()) {
                    features_.set(static_cast<size_t>(f));
                }
            }
        }
    }

    template <typename T>
    static T read_file(const fs::path& path, T default_val = T{}) {
        std::ifstream f(path);
        if (!f) {
            return default_val;
        }

        T value;
        f >> value;
        return f.fail() ? default_val : value;
    }

    static std::optional<std::set<int32_t>> parse_cpu_list_file(const fs::path& path) {
        std::string values = read_file<std::string>(path, "");
        if (!values.empty()) {
            return parse_cpulist(values);
        }

        return std::nullopt;
    }

    static std::set<int32_t> parse_cpulist(const std::string& cpulist_str) {
        std::string trimmed;
        std::set<int32_t> result;
        std::remove_copy_if(cpulist_str.begin(), cpulist_str.end(), std::back_inserter(trimmed),
                            [](unsigned char c) { return std::isspace(c); });

        for (const auto& range_view : split(trimmed, ',')) {
            if (range_view.empty()) {
                continue;
            }

            if (auto range_opt = parse_cpu_range(range_view);) {
                auto [start, end] = *range_opt;
                for (auto i = start; i <= end; ++i) {
                    result.insert(i);
                }
            }
        }
        return result;
    }

    static std::optional<std::tuple<int32_t, int32_t>> parse_cpu_range(std::string_view range_view) {
        std::vector<int32_t> tmp;
        for (const auto& range : split(range_view, '-')) {
            int32_t result = 0;
            if (std::from_chars(range.data(), range.data() + range.size(), result).ec == std::errc()) {
                tmp.push_back(result);
            }
        }
        if (tmp.size() == 1) {
            return std::make_tuple(tmp[0], tmp[0]);
        }

        if (tmp.size() == 2) {
            return std::make_tuple(tmp[0], tmp[1]);
        }

        return std::nullopt;
    }

    static std::vector<std::string_view> split(std::string_view str, char delimiter) {
        std::vector<std::string_view> result;
        size_t start = 0;
        while (true) {
            auto end = str.find(delimiter, start);
            if (end == std::string_view::npos) {
                if (start < str.size()) {
                    result.push_back(str.substr(start));
                }

                break;
            }
            result.push_back(str.substr(start, end - start));
            start = end + 1;
        }
        return result;
    }

    std::bitset<kFeatureCount> features_;
    CPUArch arch_{CPUArch::UNKNOWN};
    uint32_t num_numa_nodes_{0};
    uint32_t num_threads_{1};
    uint32_t threads_per_core_{1};
    uint32_t cache_levels_{3};
    uint32_t max_basic_leaf_{0};
    uint32_t max_extended_leaf_{0};
    bool support_ht_{false};
    bool ht_enabled_{false};

    std::vector<CacheInfo> caches_;
    std::set<int32_t> isolated_cpus_;
    std::set<int32_t> online_cpus_;
    std::vector<std::set<int32_t>> numa_cpus_;

    const CacheInfo empty_cache_{CacheType::UNKNOWN, 0, 0, 0, 0};
    const std::set<int32_t> empty_set_{};
};

}  // namespace utils
