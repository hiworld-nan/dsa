#pragma once

// generate by ai, don't work under hybrid mode or virtualization env
// todo : verify
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#include <windows.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#if defined(__linux__)
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>

#include <cstring>
#endif
#endif

namespace utils::hw {

struct CpuidRegs {
    uint32_t eax, ebx, ecx, edx;
};

[[gnu::always_inline, nodiscard]]
inline CpuidRegs cpuid(uint32_t leaf, uint32_t subleaf = 0) noexcept {
    CpuidRegs regs{};
#if defined(_MSC_VER)
    int regs_arr[4];
    __cpuidex(regs_arr, static_cast<int>(leaf), static_cast<int>(subleaf));
    regs.eax = static_cast<uint32_t>(regs_arr[0]);
    regs.ebx = static_cast<uint32_t>(regs_arr[1]);
    regs.ecx = static_cast<uint32_t>(regs_arr[2]);
    regs.edx = static_cast<uint32_t>(regs_arr[3]);
#elif defined(__GNUC__) || defined(__clang__)
    __cpuid_count(leaf, subleaf, regs.eax, regs.ebx, regs.ecx, regs.edx);
#endif
    return regs;
}

[[gnu::always_inline, nodiscard]]
inline uint64_t rdtsc() noexcept {
#if defined(_MSC_VER)
    return __rdtsc();
#elif defined(__GNUC__) || defined(__clang__)
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
#endif
}

// 获取当前核心 APIC ID
[[gnu::always_inline, nodiscard]]
inline uint32_t get_apic_id() noexcept {
    const auto r = cpuid(0xB, 0);
    return r.edx;
}

// 获取当前核心 x2APIC ID（如果支持）
[[gnu::always_inline, nodiscard]]
inline uint32_t get_x2apic_id() noexcept {
    const auto r = cpuid(0xB, 0);
    return r.edx;
}

// =============================================================================
// 核心信息结构
// =============================================================================

enum class CoreType : uint8_t {
    Invalid = 0,
    IntelAtom = 0x10,     // E-core
    IntelCore = 0x20,     // P-core
    IntelXeon = 0x30,     // P-core (服务器)
    IntelXeonAtom = 0x40  // E-core (服务器)
};

enum class CoreStatus : uint8_t {
    Unknown = 0,
    Online = 1,   // 在线可用
    Offline = 2,  // 离线
    Isolated = 3  // 被隔离（Linux isolcpus）
};

struct CoreInfo {
    uint32_t core_id;           // 逻辑核心 ID（OS 视角）
    uint32_t apic_id;           // APIC ID（硬件视角）
    uint32_t physical_package;  // 物理 CPU 包 ID
    uint32_t core_type;         // 核心类型（P-core/E-core）
    CoreStatus status;          // 在线状态
    bool is_hybrid;             // 是否混合架构中的特定类型
};

enum class Vendor : uint8_t { Unknown, Intel, AMD, VIA, Hygon, Zhaoxin, Other };

enum class Microarchitecture : uint8_t {
    Unknown,
    Core,
    Merom,
    Penryn,
    Nehalem,
    Westmere,
    SandyBridge,
    IvyBridge,
    Haswell,
    Broadwell,
    Skylake,
    KabyLake,
    CoffeeLake,
    CannonLake,
    IceLake,
    TigerLake,
    AlderLake,
    RaptorLake,
    MeteorLake,
    ArrowLake,
    K10,
    Bulldozer,
    Piledriver,
    Steamroller,
    Excavator,
    Zen,
    ZenPlus,
    Zen2,
    Zen3,
    Zen4,
    Zen5,
    Generic
};

struct CacheInfo {
    enum Type : uint8_t { Data, Instruction, Unified, Trace };
    Type type;
    uint8_t level;
    uint32_t size_kb;
    uint16_t line_size;
    uint16_t ways;
    uint32_t sets;
    bool fully_associative;
};

// =============================================================================
// 混合架构拓扑
// =============================================================================

struct HybridTopology {
    bool is_hybrid = false;
    uint16_t logical_cores = 0;
    uint16_t physical_cores = 0;

    uint16_t p_cores_physical = 0;
    uint16_t p_cores_logical = 0;
    uint8_t p_core_threads = 2;

    uint16_t e_cores_physical = 0;
    uint16_t e_cores_logical = 0;
    uint8_t e_core_threads = 1;

    uint8_t threads_per_core = 1;
    uint8_t cores_per_package = 0;
    uint16_t packages = 1;

    std::vector<CoreInfo> core_details;  // 每个核心的详细信息
};

enum class Feature : uint16_t {
    FPU = 0,
    VME = 1,
    DE = 2,
    PSE = 3,
    TSC = 4,
    MSR = 5,
    PAE = 6,
    MCE = 7,
    CX8 = 8,
    APIC = 9,
    SEP = 11,
    MTRR = 12,
    PGE = 13,
    MCA = 14,
    CMOV = 15,
    PAT = 16,
    PSE36 = 17,
    PSN = 18,
    CLFSH = 19,
    DS = 21,
    ACPI = 22,
    MMX = 23,
    FXSR = 24,
    SSE = 25,
    SSE2 = 26,
    SS = 27,
    HTT = 28,
    TM = 29,
    IA64 = 30,
    PBE = 31,

    SSE3 = 32 + 0,
    PCLMULQDQ = 32 + 1,
    DTES64 = 32 + 2,
    MONITOR = 32 + 3,
    DS_CPL = 32 + 4,
    VMX = 32 + 5,
    SMX = 32 + 6,
    EST = 32 + 7,
    TM2 = 32 + 8,
    SSSE3 = 32 + 9,
    CID = 32 + 10,
    SDBG = 32 + 11,
    FMA = 32 + 12,
    CX16 = 32 + 13,
    XTPR = 32 + 14,
    PDCM = 32 + 15,
    PCID = 32 + 17,
    DCA = 32 + 18,
    SSE4_1 = 32 + 19,
    SSE4_2 = 32 + 20,
    X2APIC = 32 + 21,
    MOVBE = 32 + 22,
    POPCNT = 32 + 23,
    TSC_DEADLINE = 32 + 24,
    AES = 32 + 25,
    XSAVE = 32 + 26,
    OSXSAVE = 32 + 27,
    AVX = 32 + 28,
    F16C = 32 + 29,
    RDRAND = 32 + 30,
    HYPERVISOR = 32 + 31,

    FSGSBASE = 64 + 0,
    BMI1 = 64 + 3,
    HLE = 64 + 4,
    AVX2 = 64 + 5,
    SMEP = 64 + 7,
    BMI2 = 64 + 8,
    ERMS = 64 + 9,
    INVPCID = 64 + 10,
    RTM = 64 + 11,
    AVX512F = 64 + 16,
    AVX512DQ = 64 + 17,
    RDSEED = 64 + 18,
    ADX = 64 + 19,
    AVX512IFMA = 64 + 21,
    PCOMMIT = 64 + 22,
    CLFLUSHOPT = 64 + 23,
    CLWB = 64 + 24,
    AVX512PF = 64 + 26,
    AVX512ER = 64 + 27,
    AVX512CD = 64 + 28,
    SHA = 64 + 29,
    AVX512BW = 64 + 30,
    AVX512VL = 64 + 31,

    PREFETCHWT1 = 96 + 0,
    AVX512VBMI = 96 + 1,
    UMIP = 96 + 2,
    PKU = 96 + 3,
    OSPKE = 96 + 4,
    AVX512VBMI2 = 96 + 6,
    GFNI = 96 + 8,
    VAES = 96 + 9,
    AVX512VNNI = 96 + 11,
    AVX512BITALG = 96 + 12,
    AVX512VPOPCNTDQ = 96 + 14,
    RDPID = 96 + 22,

    AVX5124VNNIW = 128 + 2,
    AVX5124FMAPS = 128 + 3,
    FSRM = 128 + 4,
    AVX512VP2INTERSECT = 128 + 8,
    SRBDS_CTRL = 128 + 9,
    MD_CLEAR = 128 + 10,
    HYBRID = 128 + 15,
    IBPB = 128 + 26,
    STIBP = 128 + 27,
    L1D_FLUSH = 128 + 28,
    ARCH_CAP = 128 + 29,
    CORE_CAP = 128 + 30,
    SSBD = 128 + 31,

    SYSCALL = 160 + 11,
    NX = 160 + 20,
    MMXEXT = 160 + 22,
    RDTSCP = 160 + 27,
    LM = 160 + 29,
    _3DNOWEXT = 160 + 30,
    _3DNOW = 160 + 31,

    LAHF_LM = 192 + 0,
    CMP_LEGACY = 192 + 1,
    SVM = 192 + 2,
    EXTAPIC = 192 + 3,
    CR8_LEGACY = 192 + 4,
    ABM = 192 + 5,
    SSE4A = 192 + 6,
    MISALIGNSSE = 192 + 7,
    _3DNOWPREFETCH = 192 + 8,
    OSVW = 192 + 9,
    IBS = 192 + 10,
    XOP = 192 + 11,
    SKINIT = 192 + 12,
    WDT = 192 + 13,
    LWP = 192 + 15,
    FMA4 = 192 + 16,
    TCE = 192 + 17,
    NODEID_MSR = 192 + 19,
    TBM = 192 + 21,
    TOPOEXT = 192 + 22,
    PERFCTR_CORE = 192 + 23,
    PERFCTR_NB = 192 + 24,
    STREAM_PERF = 192 + 25,
    DBX = 192 + 26,
    PTSC = 192 + 27,
    PERFTSC = 192 + 28,
    MWAITX = 192 + 29,

    COUNT
};

// =============================================================================
// CPU 检测器
// =============================================================================

class CpuDetector {
public:
    [[nodiscard]] static const CpuDetector& instance() noexcept {
        static CpuDetector inst;
        return inst;
    }

    CpuDetector(const CpuDetector&) = delete;
    CpuDetector& operator=(const CpuDetector&) = delete;
    CpuDetector(CpuDetector&&) = delete;
    CpuDetector& operator=(CpuDetector&&) = delete;

    [[nodiscard, gnu::pure]] bool has(Feature f) const noexcept {
        const auto idx = static_cast<size_t>(f);
        return idx < features_.size() ? features_[idx] : false;
    }

    [[nodiscard, gnu::pure]] Vendor vendor() const noexcept { return vendor_; }
    [[nodiscard, gnu::pure]] Microarchitecture uarch() const noexcept { return uarch_; }
    [[nodiscard, gnu::pure]] const HybridTopology& hybrid_topology() const noexcept { return hybrid_topo_; }
    [[nodiscard, gnu::pure]] bool is_hybrid() const noexcept { return hybrid_topo_.is_hybrid; }

    [[nodiscard]] std::string_view vendor_string() const noexcept {
        return {vendor_str_.data(), vendor_str_.size()};
    }

    [[nodiscard]] std::string_view brand_string() const noexcept {
        size_t len = brand_str_.size();
        while (len > 0 && (brand_str_[len - 1] == '\0' || brand_str_[len - 1] == ' ')) --len;
        return {brand_str_.data(), len};
    }

    [[nodiscard, gnu::pure]] uint32_t family() const noexcept { return family_; }
    [[nodiscard, gnu::pure]] uint32_t model() const noexcept { return model_; }
    [[nodiscard, gnu::pure]] uint32_t stepping() const noexcept { return stepping_; }

    [[nodiscard, gnu::pure]] bool is_intel() const noexcept { return vendor_ == Vendor::Intel; }
    [[nodiscard, gnu::pure]] bool is_amd() const noexcept { return vendor_ == Vendor::AMD; }
    [[nodiscard, gnu::pure]] bool supports_sse() const noexcept { return has(Feature::SSE); }
    [[nodiscard, gnu::pure]] bool supports_avx() const noexcept { return has(Feature::AVX); }
    [[nodiscard, gnu::pure]] bool supports_avx2() const noexcept { return has(Feature::AVX2); }
    [[nodiscard, gnu::pure]] bool supports_avx512() const noexcept { return has(Feature::AVX512F); }

    [[nodiscard]] uint16_t performance_cores() const noexcept { return hybrid_topo_.p_cores_physical; }
    [[nodiscard]] uint16_t efficient_cores() const noexcept { return hybrid_topo_.e_cores_physical; }

    [[nodiscard]] const std::vector<CacheInfo>& caches() const noexcept { return caches_; }
    // =============================================================================
    // 新增功能 1-6
    // =============================================================================

    // 1. 获取所有 core ID
    [[nodiscard]] std::vector<uint32_t> get_all_core_ids() const noexcept {
        std::vector<uint32_t> ids;
        for (const auto& core : hybrid_topo_.core_details) {
            ids.push_back(core.core_id);
        }
        return ids;
    }

    // 2. 获取 online cores
    [[nodiscard]] std::vector<uint32_t> get_online_cores() const noexcept {
        std::vector<uint32_t> ids;
        for (const auto& core : hybrid_topo_.core_details) {
            if (core.status == CoreStatus::Online) {
                ids.push_back(core.core_id);
            }
        }
        return ids;
    }

    // 3. 获取 offline cores
    [[nodiscard]] std::vector<uint32_t> get_offline_cores() const noexcept {
        std::vector<uint32_t> ids;
        for (const auto& core : hybrid_topo_.core_details) {
            if (core.status == CoreStatus::Offline) {
                ids.push_back(core.core_id);
            }
        }
        return ids;
    }

    // 4. 获取 isolated cores
    [[nodiscard]] std::vector<uint32_t> get_isolated_cores() const noexcept {
        std::vector<uint32_t> ids;
        for (const auto& core : hybrid_topo_.core_details) {
            if (core.status == CoreStatus::Isolated) {
                ids.push_back(core.core_id);
            }
        }
        return ids;
    }

    // 5. 判断是否开启超线程
    [[nodiscard]] bool is_hyperthreading_enabled() const noexcept {
        // 方法1：检查 HTT 特性标志
        if (!has(Feature::HTT))
            return false;

        // 方法2：逻辑核心数 > 物理核心数
        if (hybrid_topo_.logical_cores <= hybrid_topo_.physical_cores)
            return false;

        // 方法3：检查每个物理核心的线程数
        // for (const auto& core : hybrid_topo_.core_details) {
        // 如果有任何一个核心支持多线程，则认为 HT 开启
        // 实际上需要更复杂的检测，这里简化处理
        // }

        // 方法4：通过 OS API 确认（最准确）
        return detect_os_hyperthreading();
    }

    // 6. 打印 CPU 信息
    friend inline std::ostream& operator<<(std::ostream& os, const CpuDetector& detector) {
        os << "========================================\n";
        os << "           CPU Information              \n";
        os << "========================================\n";

        os << "Vendor:        " << detector.vendor_string() << "\n";
        os << "Brand:         " << detector.brand_string() << "\n";
        os << "Family:        " << detector.family_ << "\n";
        os << "Model:         " << detector.model_ << "\n";
        os << "Stepping:      " << detector.stepping_ << "\n";
        os << "Microarch:     " << uarch_to_string(detector.uarch_) << "\n";

        os << "\n---------- Topology ----------\n";
        os << "Total Logical: " << detector.hybrid_topo_.logical_cores << "\n";
        os << "Total Physical: " << detector.hybrid_topo_.physical_cores << "\n";
        os << "Packages:      " << detector.hybrid_topo_.packages << "\n";
        os << "Hybrid Arch:   " << (detector.hybrid_topo_.is_hybrid ? "Yes" : "No") << "\n";

        if (detector.hybrid_topo_.is_hybrid) {
            os << "P-cores:       " << detector.hybrid_topo_.p_cores_physical
               << " (threads: " << (int)detector.hybrid_topo_.p_core_threads << ")\n";
            os << "E-cores:       " << detector.hybrid_topo_.e_cores_physical
               << " (threads: " << (int)detector.hybrid_topo_.e_core_threads << ")\n";
        }

        os << "Hyper-thread:  " << (detector.is_hyperthreading_enabled() ? "Enabled" : "Disabled") << "\n";

        os << "\n---------- Core Details ----------\n";
        auto online = detector.get_online_cores();
        auto offline = detector.get_offline_cores();
        auto isolated = detector.get_isolated_cores();

        os << "Online cores (" << online.size() << "): ";
        for (auto id : online) os << id << " ";
        os << "\n";

        if (!offline.empty()) {
            os << "Offline cores (" << offline.size() << "): ";
            for (auto id : offline) os << id << " ";
            os << "\n";
        }

        if (!isolated.empty()) {
            os << "Isolated cores (" << isolated.size() << "): ";
            for (auto id : isolated) os << id << " ";
            os << "\n";
        }

        os << "\n---------- Features ----------\n";
        os << "Max SIMD:      " << detector.max_simd_level() << "\n";
        os << "SSE:           " << (detector.has(Feature::SSE) ? "Yes" : "No") << "\n";
        os << "AVX:           " << (detector.has(Feature::AVX) ? "Yes" : "No") << "\n";
        os << "AVX2:          " << (detector.has(Feature::AVX2) ? "Yes" : "No") << "\n";
        os << "AVX-512:       " << (detector.has(Feature::AVX512F) ? "Yes" : "No") << "\n";

        os << "\n------------ Cache ------------\n";
        for (const auto& cache : detector.caches_) {
            os << "  L" << static_cast<int>(cache.level) << " ";
            switch (cache.type) {
            case CacheInfo::Data: os << "data"; break;
            case CacheInfo::Instruction: os << "instruction"; break;
            case CacheInfo::Unified: os << "unified"; break;
            case CacheInfo::Trace: os << "trace"; break;
            }
            os << " cache: " << cache.size_kb << " KB, " << cache.ways << "-way, line " << cache.line_size
               << " bytes";
            if (cache.fully_associative)
                os << " (fully associative)";
            os << "\n";
        }

        os << "========================================\n";
        os.flush();

        return os;
    }

    [[nodiscard]] const char* max_simd_level() const noexcept {
        if (has(Feature::AVX512F))
            return "AVX-512";
        if (has(Feature::AVX2))
            return "AVX2";
        if (has(Feature::AVX))
            return "AVX";
        if (has(Feature::SSE4_2))
            return "SSE4.2";
        if (has(Feature::SSE4_1))
            return "SSE4.1";
        if (has(Feature::SSE3))
            return "SSE3";
        if (has(Feature::SSE2))
            return "SSE2";
        if (has(Feature::SSE))
            return "SSE";
        return "None";
    }

    [[nodiscard]] uint16_t get_performance_cores() const noexcept { return hybrid_topo_.p_cores_physical; }

    [[nodiscard]] uint16_t get_efficient_cores() const noexcept { return hybrid_topo_.e_cores_physical; }

private:
    std::bitset<static_cast<size_t>(Feature::COUNT)> features_;
    std::array<char, 12> vendor_str_{};
    std::array<char, 48> brand_str_{};
    Vendor vendor_{Vendor::Unknown};
    Microarchitecture uarch_{Microarchitecture::Unknown};
    HybridTopology hybrid_topo_{};
    std::vector<CacheInfo> caches_;
    uint32_t family_{0}, model_{0}, stepping_{0};
    uint32_t max_basic_leaf_{0};
    uint32_t max_extended_leaf_{0};

    CpuDetector() noexcept { detect_all(); }

    void detect_all() noexcept {
        detect_vendor_and_basic();
        detect_features();
        detect_topology();
        detect_core_details();  // 新增：检测每个核心的详细信息
        detect_caches();
        detect_uarch();
    }

    // OS 级别的超线程检测
    [[nodiscard]] bool detect_os_hyperthreading() const noexcept {
#if defined(__linux__)
        // Linux: 检查 /proc/cpuinfo 中 siblings 和 cpu cores 的关系
        // 或者检查 thread_siblings_list
        return hybrid_topo_.logical_cores > hybrid_topo_.physical_cores;
#elif defined(_WIN32)
        // Windows: 使用 GetLogicalProcessorInformationEx
        return hybrid_topo_.logical_cores > hybrid_topo_.physical_cores;
#else
        return hybrid_topo_.logical_cores > hybrid_topo_.physical_cores;
#endif
    }

    // 检测每个核心的详细信息（OS API）
    void detect_core_details() noexcept {
        hybrid_topo_.core_details.clear();

#if defined(__linux__)
        detect_linux_core_details();
#elif defined(_WIN32)
        detect_windows_core_details();
#else
        // 通用回退：基于拓扑信息生成
        generate_fallback_core_details();
#endif
    }

#if defined(__linux__)
    void detect_linux_core_details() noexcept {
        // 读取 /proc/cpuinfo 获取每个核心的信息
        // 读取 /sys/devices/system/cpu/cpu*/online 获取在线状态
        // 读取 /sys/devices/system/cpu/cpu*/topology/thread_siblings_list 获取拓扑

        uint32_t max_cores = sysconf(_SC_NPROCESSORS_CONF);
        uint32_t online_cores = sysconf(_SC_NPROCESSORS_ONLN);

        for (uint32_t i = 0; i < max_cores; ++i) {
            CoreInfo info{};
            info.core_id = i;
            info.status = CoreStatus::Offline;

            // 检查是否在线
            char path[256];
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%u/online", i);
            int fd = open(path, O_RDONLY);
            if (fd >= 0) {
                char buf[2] = {0};
                if (read(fd, buf, 1) > 0) {
                    info.status = (buf[0] == '1') ? CoreStatus::Online : CoreStatus::Offline;
                }
                close(fd);
            } else if (i < online_cores) {
                // 如果文件不存在但核心号小于在线数，默认为在线（cpu0 通常没有 online 文件）
                info.status = CoreStatus::Online;
            }

            // 检查是否被隔离（isolcpus）
            // 读取 /proc/cmdline 检查 isolcpus 参数
            if (is_core_isolated_linux(i)) {
                info.status = CoreStatus::Isolated;
            }

            // 获取 APIC ID
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%u/topology/apicid", i);
            fd = open(path, O_RDONLY);
            if (fd >= 0) {
                char buf[32] = {0};
                if (read(fd, buf, sizeof(buf) - 1) > 0) {
                    info.apic_id = static_cast<uint32_t>(strtol(buf, nullptr, 0));
                }
                close(fd);
            }

            // 获取物理包 ID
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%u/topology/physical_package_id", i);
            fd = open(path, O_RDONLY);
            if (fd >= 0) {
                char buf[32] = {0};
                if (read(fd, buf, sizeof(buf) - 1) > 0) {
                    info.physical_package = static_cast<uint32_t>(strtol(buf, nullptr, 0));
                }
                close(fd);
            }

            // 推断核心类型（基于混合架构信息）
            if (hybrid_topo_.is_hybrid && i < hybrid_topo_.p_cores_logical) {
                info.core_type = static_cast<uint32_t>(CoreType::IntelCore);
                info.is_hybrid = true;
            } else if (hybrid_topo_.is_hybrid) {
                info.core_type = static_cast<uint32_t>(CoreType::IntelAtom);
                info.is_hybrid = true;
            }

            hybrid_topo_.core_details.push_back(info);
        }
    }

    [[nodiscard]] bool is_core_isolated_linux(uint32_t core_id) const noexcept {
        // 简化实现：检查 /proc/cmdline 中的 isolcpus
        // 实际实现应该解析 isolcpus 参数
        int fd = open("/proc/cmdline", O_RDONLY);
        if (fd < 0)
            return false;

        char buf[4096] = {0};
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);

        if (n <= 0)
            return false;

        // 简单字符串搜索 isolcpus
        std::string cmdline(buf, n);
        auto pos = cmdline.find("isolcpus");
        if (pos == std::string::npos)
            return false;

        // 解析 isolcpus 参数（简化版）
        // 实际应该完整解析 CPU 列表格式
        return cmdline.find(std::to_string(core_id)) != std::string::npos;
    }
#endif

#if defined(_WIN32)
    void detect_windows_core_details() noexcept {
        // 使用 GetLogicalProcessorInformationEx
        DWORD len = 0;
        GetLogicalProcessorInformationEx(RelationAll, nullptr, &len);

        if (len == 0) {
            generate_fallback_core_details();
            return;
        }

        std::vector<uint8_t> buffer(len);
        auto* info = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data());

        if (!GetLogicalProcessorInformationEx(RelationAll, info, &len)) {
            generate_fallback_core_details();
            return;
        }

        // 解析返回的信息
        uint8_t* ptr = buffer.data();
        while (len > 0) {
            auto* current = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(ptr);

            if (current->Relationship == RelationProcessorCore) {
                // 处理核心信息
                auto* core = &current->Processor;
                // 解析 core->Flags 判断是否为 SMT
                // 解析 core->GroupMask 获取核心掩码
            } else if (current->Relationship == RelationNumaNode) {
                // NUMA 节点信息
            } else if (current->Relationship == RelationCache) {
                // 缓存信息
            } else if (current->Relationship == RelationProcessorPackage) {
                // 物理包信息
            }

            ptr += current->Size;
            len -= current->Size;
        }

        // Windows 不直接支持 isolated cores，需要其他方法检测
        generate_fallback_core_details();
    }
#endif

    void generate_fallback_core_details() noexcept {
        // 基于检测到的拓扑信息生成核心详情
        for (uint32_t i = 0; i < hybrid_topo_.logical_cores; ++i) {
            CoreInfo info{};
            info.core_id = i;
            info.status = CoreStatus::Online;  // 假设全部在线
            info.apic_id = i;                  // 简化
            info.physical_package = 0;

            // 推断核心类型
            if (hybrid_topo_.is_hybrid) {
                if (i < hybrid_topo_.p_cores_logical) {
                    info.core_type = static_cast<uint32_t>(CoreType::IntelCore);
                    info.is_hybrid = true;
                } else {
                    info.core_type = static_cast<uint32_t>(CoreType::IntelAtom);
                    info.is_hybrid = true;
                }
            }

            hybrid_topo_.core_details.push_back(info);
        }
    }

    [[nodiscard]] static const char* uarch_to_string(Microarchitecture uarch) noexcept {
        switch (uarch) {
        case Microarchitecture::Unknown: return "Unknown";
        case Microarchitecture::Merom: return "Core2 (Merom)";
        case Microarchitecture::Penryn: return "Core2 (Penryn)";
        case Microarchitecture::Nehalem: return "Nehalem";
        case Microarchitecture::Westmere: return "Westmere";
        case Microarchitecture::SandyBridge: return "Sandy Bridge";
        case Microarchitecture::IvyBridge: return "Ivy Bridge";
        case Microarchitecture::Haswell: return "Haswell";
        case Microarchitecture::Broadwell: return "Broadwell";
        case Microarchitecture::Skylake: return "Skylake";
        case Microarchitecture::KabyLake: return "Kaby Lake";
        case Microarchitecture::CoffeeLake: return "Coffee Lake";
        case Microarchitecture::CannonLake: return "Cannon Lake";
        case Microarchitecture::IceLake: return "Ice Lake";
        case Microarchitecture::TigerLake: return "Tiger Lake";
        case Microarchitecture::AlderLake: return "Alder Lake";
        case Microarchitecture::RaptorLake: return "Raptor Lake";
        case Microarchitecture::MeteorLake: return "Meteor Lake";
        case Microarchitecture::ArrowLake: return "Arrow Lake";
        case Microarchitecture::Zen: return "Zen";
        case Microarchitecture::Zen2: return "Zen 2";
        case Microarchitecture::Zen3: return "Zen 3";
        case Microarchitecture::Zen4: return "Zen 4";
        case Microarchitecture::Zen5: return "Zen 5";
        default: return "Generic";
        }
    }

    void detect_vendor_and_basic() noexcept {
        const auto id = cpuid(0);
        max_basic_leaf_ = id.eax;

        *reinterpret_cast<uint32_t*>(&vendor_str_[0]) = id.ebx;
        *reinterpret_cast<uint32_t*>(&vendor_str_[4]) = id.edx;
        *reinterpret_cast<uint32_t*>(&vendor_str_[8]) = id.ecx;

        if (vendor_str_ == std::array<char, 12>{'G', 'e', 'n', 'u', 'i', 'n', 'e', 'I', 'n', 't', 'e', 'l'})
            vendor_ = Vendor::Intel;
        else if (vendor_str_ ==
                 std::array<char, 12>{'A', 'u', 't', 'h', 'e', 'n', 't', 'i', 'c', 'A', 'M', 'D'})
            vendor_ = Vendor::AMD;
        else if (vendor_str_[0] == 'H' && vendor_str_[1] == 'y' && vendor_str_[2] == 'g')
            vendor_ = Vendor::Hygon;
        else if (vendor_str_[0] == 'C' && vendor_str_[1] == 'e')
            vendor_ = Vendor::Zhaoxin;
        else if (vendor_str_[0] == 'V' && vendor_str_[1] == 'I' && vendor_str_[2] == 'A')
            vendor_ = Vendor::VIA;

        const auto ext_id = cpuid(0x80000000);
        max_extended_leaf_ = ext_id.eax;

        if (max_extended_leaf_ >= 0x80000004) {
            for (int i = 0; i < 3; ++i) {
                const auto b = cpuid(0x80000002 + i);
                auto* dst = reinterpret_cast<uint32_t*>(&brand_str_[i * 16]);
                dst[0] = b.eax;
                dst[1] = b.ebx;
                dst[2] = b.ecx;
                dst[3] = b.edx;
            }
        }

        if (max_basic_leaf_ >= 1) {
            const auto info = cpuid(1);
            stepping_ = info.eax & 0xF;
            model_ = (info.eax >> 4) & 0xF;
            family_ = (info.eax >> 8) & 0xF;
            const uint32_t ext_model = (info.eax >> 16) & 0xF;
            const uint32_t ext_family = (info.eax >> 20) & 0xFF;

            if (family_ == 0x0F)
                family_ += ext_family;
            if (family_ == 0x06 || family_ == 0x0F)
                model_ += (ext_model << 4);
        }
    }

    void detect_features() noexcept {
        if (max_basic_leaf_ < 1)
            return;

        const auto r1 = cpuid(1);
        set_features_from_edx(r1.edx, 0);
        set_features_from_ecx(r1.ecx, 32);

        if (max_basic_leaf_ >= 7) {
            const auto r7 = cpuid(7);
            set_features_from_ebx(r7.ebx, 64);
            set_features_from_ecx(r7.ecx, 96);
            set_features_from_edx(r7.edx, 128);
        }

        if (max_extended_leaf_ >= 0x80000001) {
            const auto re = cpuid(0x80000001);
            set_features_from_edx(re.edx, 160);
            set_features_from_ecx(re.ecx, 192);
        }
    }

    void set_features_from_edx(uint32_t val, uint16_t base) noexcept {
        for (int i = 0; i < 32; ++i) {
            if (val & (1u << i)) {
                auto f = static_cast<Feature>(base + i);
                if (static_cast<size_t>(f) < features_.size())
                    features_.set(static_cast<size_t>(f));
            }
        }
    }

    void set_features_from_ecx(uint32_t val, uint16_t base) noexcept { set_features_from_edx(val, base); }

    void set_features_from_ebx(uint32_t val, uint16_t base) noexcept { set_features_from_edx(val, base); }

    void detect_topology() noexcept {
        std::cout << "=== Topology detection ===\n";
        std::cout << "Vendor: " << vendor_string() << " (" << (is_intel() ? "Intel" : "non-Intel") << ")\n";
        std::cout << "max_basic_leaf_: 0x" << std::hex << max_basic_leaf_ << std::dec << "\n";
        std::cout << "Feature::HYBRID: " << has(Feature::HYBRID) << "\n";
        if (vendor_ == Vendor::Intel && has(Feature::HYBRID) && max_basic_leaf_ >= 0x1F) {
            std::cout << " Intel hybrid topology detected" << std::endl;
            detect_intel_hybrid_topology();
        } else if (max_basic_leaf_ >= 0xB) {
            std::cout << " Intel standard topology detected" << std::endl;
            detect_intel_standard_topology();
        } else if (vendor_ == Vendor::AMD && max_extended_leaf_ >= 0x80000008) {
            std::cout << " amd topology detected" << std::endl;
            detect_amd_topology();
        } else {
            std::cout << " legacy topology detected" << std::endl;
            detect_legacy_topology();
        }

        if (hybrid_topo_.physical_cores > 0) {
            hybrid_topo_.threads_per_core =
                static_cast<uint8_t>(hybrid_topo_.logical_cores / hybrid_topo_.physical_cores);
        }
    }

    void detect_intel_hybrid_topology() noexcept {
        hybrid_topo_.is_hybrid = true;

        CoreType current_core_type = CoreType::Invalid;
        if (max_basic_leaf_ >= 0x1A) {
            const auto r1a = cpuid(0x1A);
            current_core_type = static_cast<CoreType>(r1a.eax & 0x1F);
        }

        const auto r0b_0 = cpuid(0xB, 0);
        hybrid_topo_.logical_cores = r0b_0.ebx & 0xFFFF;

        struct LevelInfo {
            uint32_t type;
            uint32_t count;
            uint32_t shift;
            uint32_t level_number;
        };

        std::vector<LevelInfo> levels;
        uint32_t level = 0;

        while (true) {
            const auto r = cpuid(0x1F, level);
            uint32_t level_type = (r.ecx >> 8) & 0xFF;
            if (level_type == 0 || r.ebx == 0)
                break;

            LevelInfo info;
            info.type = level_type;
            info.count = r.ebx & 0xFFFF;
            info.shift = r.eax & 0x1F;
            info.level_number = r.ecx & 0xFF;
            levels.push_back(info);
            ++level;
        }

        if (levels.size() >= 3) {
            auto smt_it =
                std::find_if(levels.begin(), levels.end(), [](const LevelInfo& l) { return l.type == 1; });
            auto core_it =
                std::find_if(levels.begin(), levels.end(), [](const LevelInfo& l) { return l.type == 2; });

            if (smt_it != levels.end() && core_it != levels.end()) {
                uint32_t smt_shift = smt_it->shift;
                uint32_t core_shift = core_it->shift;
                uint32_t threads_per_core = (core_shift > smt_shift) ? (1u << (core_shift - smt_shift)) : 1;
                uint32_t total_cores_reported = core_it->count;

                infer_from_current_core_type(current_core_type, total_cores_reported, threads_per_core,
                                             smt_it->count);
            }
        }

        if (hybrid_topo_.p_cores_physical == 0 && hybrid_topo_.e_cores_physical == 0) {
            detect_hybrid_by_leaf_0b();
        }

        finalize_hybrid_counts();
    }

    void infer_from_current_core_type(CoreType current_type, uint32_t total_cores, uint32_t threads_per_core,
                                      uint32_t total_threads) noexcept {
        if (current_type == CoreType::IntelCore || current_type == CoreType::IntelXeon) {
            hybrid_topo_.p_cores_physical = static_cast<uint16_t>(total_cores);
            hybrid_topo_.p_core_threads = static_cast<uint8_t>(threads_per_core);

            uint32_t p_logical = total_cores * threads_per_core;
            if (total_threads > p_logical && threads_per_core == 2) {
                uint32_t e_logical = total_threads - p_logical;
                hybrid_topo_.e_cores_physical = static_cast<uint16_t>(e_logical);
                hybrid_topo_.e_core_threads = 1;
            }
        } else if (current_type == CoreType::IntelAtom || current_type == CoreType::IntelXeonAtom) {
            hybrid_topo_.e_cores_physical = static_cast<uint16_t>(total_cores);
            hybrid_topo_.e_core_threads = 1;

            if (total_threads > total_cores) {
                uint32_t remaining = total_threads - total_cores;
                hybrid_topo_.p_cores_physical = static_cast<uint16_t>(remaining / 2);
                hybrid_topo_.p_core_threads = 2;
            }
        }
    }

    void detect_hybrid_by_leaf_0b() noexcept {
        const auto r0 = cpuid(0xB, 0);
        const auto r1 = cpuid(0xB, 1);

        hybrid_topo_.logical_cores = r0.ebx & 0xFFFF;

        if (((r1.ecx >> 8) & 0xFF) == 2) {
            uint32_t core_shift = r1.eax & 0x1F;
            uint32_t smt_shift = r0.eax & 0x1F;
            uint32_t threads_per_core = (core_shift > smt_shift) ? (1u << (core_shift - smt_shift)) : 1;
            uint32_t total_cores = r1.ebx & 0xFFFF;

            solve_hybrid_core_counts(hybrid_topo_.logical_cores, total_cores, threads_per_core);
        }
    }

    void solve_hybrid_core_counts(uint32_t total_threads, uint32_t total_cores_reported,
                                  uint32_t threads_per_core_reported) noexcept {
        if (threads_per_core_reported == 2) {
            for (uint16_t p = 1; p <= total_threads / 2; ++p) {
                uint16_t e = static_cast<uint16_t>(total_threads - 2 * p);
                if (e > 0 && is_plausible_hybrid_ratio(p, e)) {
                    hybrid_topo_.p_cores_physical = p;
                    hybrid_topo_.e_cores_physical = e;
                    hybrid_topo_.p_core_threads = 2;
                    hybrid_topo_.e_core_threads = 1;
                    return;
                }
            }
        }

        hybrid_topo_.p_cores_physical = static_cast<uint16_t>(total_cores_reported);
        hybrid_topo_.p_core_threads = static_cast<uint8_t>(threads_per_core_reported);
    }

    [[nodiscard]] bool is_plausible_hybrid_ratio(uint16_t p, uint16_t e) const noexcept {
        if (p == 0 || e == 0)
            return false;
        double ratio = static_cast<double>(p) / e;
        const double valid_ratios[] = {0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 4.0};
        for (double valid : valid_ratios) {
            if (std::abs(ratio - valid) < 0.1)
                return true;
        }
        return false;
    }

    void finalize_hybrid_counts() noexcept {
        if (hybrid_topo_.p_cores_physical == 0 && hybrid_topo_.e_cores_physical == 0) {
            detect_intel_standard_topology();
            hybrid_topo_.is_hybrid = false;
            return;
        }

        hybrid_topo_.p_cores_logical = hybrid_topo_.p_cores_physical * hybrid_topo_.p_core_threads;
        hybrid_topo_.e_cores_logical = hybrid_topo_.e_cores_physical * hybrid_topo_.e_core_threads;

        uint16_t calc_total = hybrid_topo_.p_cores_logical + hybrid_topo_.e_cores_logical;
        if (calc_total != hybrid_topo_.logical_cores && hybrid_topo_.logical_cores > 0) {
            int16_t diff = static_cast<int16_t>(hybrid_topo_.logical_cores) - calc_total;
            if (diff > 0) {
                hybrid_topo_.e_cores_physical += diff;
                hybrid_topo_.e_cores_logical += diff;
            } else if (diff < 0 && static_cast<uint16_t>(-diff) <= hybrid_topo_.e_cores_physical) {
                hybrid_topo_.e_cores_physical -= static_cast<uint16_t>(-diff);
                hybrid_topo_.e_cores_logical = hybrid_topo_.e_cores_physical;
            }
        }

        hybrid_topo_.physical_cores = hybrid_topo_.p_cores_physical + hybrid_topo_.e_cores_physical;
    }

    void detect_intel_standard_topology() noexcept {
        const auto r0 = cpuid(0xB, 0);
        hybrid_topo_.logical_cores = r0.ebx & 0xFFFF;

        const auto r1 = cpuid(0xB, 1);
        if (((r1.ecx >> 8) & 0xFF) == 2) {
            uint32_t core_count = r1.ebx & 0xFFFF;
            hybrid_topo_.physical_cores = static_cast<uint16_t>(core_count);
            hybrid_topo_.cores_per_package = static_cast<uint8_t>(core_count);
            hybrid_topo_.p_cores_physical = hybrid_topo_.physical_cores;
            hybrid_topo_.p_cores_logical = hybrid_topo_.logical_cores;

            uint32_t smt_shift = r0.eax & 0x1F;
            uint32_t core_shift = r1.eax & 0x1F;
            if (core_shift > smt_shift) {
                hybrid_topo_.p_core_threads = static_cast<uint8_t>(1u << (core_shift - smt_shift));
            }
        } else {
            hybrid_topo_.physical_cores = hybrid_topo_.logical_cores;
            hybrid_topo_.p_cores_physical = hybrid_topo_.physical_cores;
            hybrid_topo_.p_cores_logical = hybrid_topo_.logical_cores;
            hybrid_topo_.p_core_threads = has(Feature::HTT) ? 2 : 1;
        }
    }

    void detect_amd_topology() noexcept {
        const auto r8 = cpuid(0x80000008);
        uint32_t nc = (r8.ecx & 0xFF) + 1;
        uint32_t threads_per_core = ((r8.ecx >> 8) & 0xF) + 1;

        hybrid_topo_.physical_cores = static_cast<uint16_t>(nc);
        hybrid_topo_.logical_cores = static_cast<uint16_t>(nc * threads_per_core);
        hybrid_topo_.p_cores_physical = hybrid_topo_.physical_cores;
        hybrid_topo_.p_cores_logical = hybrid_topo_.logical_cores;
        hybrid_topo_.p_core_threads = static_cast<uint8_t>(threads_per_core);
        hybrid_topo_.cores_per_package = static_cast<uint8_t>(nc);
    }

    void detect_legacy_topology() noexcept {
        if (max_basic_leaf_ < 1)
            return;

        const auto r1 = cpuid(1);
        hybrid_topo_.logical_cores = (r1.ebx >> 16) & 0xFF;

        if (max_basic_leaf_ >= 4) {
            const auto r4 = cpuid(4);
            uint32_t max_cores = ((r4.eax >> 26) & 0x3F) + 1;
            hybrid_topo_.physical_cores = static_cast<uint16_t>(max_cores);
            hybrid_topo_.cores_per_package = static_cast<uint8_t>(max_cores);
        } else {
            hybrid_topo_.physical_cores = hybrid_topo_.logical_cores;
        }

        hybrid_topo_.p_cores_physical = hybrid_topo_.physical_cores;
        hybrid_topo_.p_cores_logical = hybrid_topo_.logical_cores;
        hybrid_topo_.p_core_threads = (hybrid_topo_.logical_cores > hybrid_topo_.physical_cores) ? 2 : 1;
    }

    void detect_caches() noexcept {
        if (max_basic_leaf_ < 4) {
            if (vendor_ == Vendor::AMD && max_extended_leaf_ >= 0x80000005) {
                detect_amd_caches();
            }
            return;
        }

        for (uint32_t i = 0;; ++i) {
            const auto r = cpuid(4, i);
            const uint32_t type = r.eax & 0x1F;
            if (type == 0)
                break;

            CacheInfo cache{};
            cache.type = static_cast<CacheInfo::Type>(type - 1);
            cache.level = (r.eax >> 5) & 0x7;
            cache.fully_associative = (r.eax >> 9) & 1;

            const uint32_t ways = ((r.ebx >> 22) & 0x3FF) + 1;
            const uint32_t partitions = ((r.ebx >> 12) & 0x3FF) + 1;
            const uint32_t line_size = (r.ebx & 0xFFF) + 1;
            const uint32_t sets = r.ecx + 1;

            cache.ways = static_cast<uint16_t>(ways);
            cache.line_size = static_cast<uint16_t>(line_size);
            cache.sets = sets;
            cache.size_kb = (ways * partitions * line_size * sets) / 1024;

            caches_.push_back(cache);
        }
    }

    void detect_amd_caches() noexcept {
        if (max_extended_leaf_ >= 0x80000005) {
            const auto r5 = cpuid(0x80000005);

            CacheInfo l1d{};
            l1d.type = CacheInfo::Data;
            l1d.level = 1;
            l1d.line_size = (r5.ecx >> 0) & 0xFF;
            l1d.ways = (r5.ecx >> 8) & 0xFF;
            l1d.size_kb = (r5.ecx >> 24) & 0xFF;
            caches_.push_back(l1d);

            CacheInfo l1i{};
            l1i.type = CacheInfo::Instruction;
            l1i.level = 1;
            l1i.line_size = (r5.edx >> 0) & 0xFF;
            l1i.ways = (r5.edx >> 8) & 0xFF;
            l1i.size_kb = (r5.edx >> 24) & 0xFF;
            caches_.push_back(l1i);
        }

        if (max_extended_leaf_ >= 0x80000006) {
            const auto r6 = cpuid(0x80000006);

            CacheInfo l2{};
            l2.type = CacheInfo::Unified;
            l2.level = 2;
            l2.line_size = r6.ecx & 0xFF;
            uint32_t ways = (r6.ecx >> 12) & 0xF;
            l2.ways = (ways == 0xF) ? 0 : (1 << ways);
            l2.fully_associative = (ways == 0xF);
            l2.size_kb = (r6.ecx >> 16) & 0xFFFF;
            caches_.push_back(l2);

            uint32_t l3_size = ((r6.edx >> 18) & 0x3FFF) * 512;
            if (l3_size > 0) {
                CacheInfo l3{};
                l3.type = CacheInfo::Unified;
                l3.level = 3;
                l3.line_size = r6.edx & 0xFF;
                uint32_t ways = (r6.edx >> 12) & 0xF;
                l3.ways = (ways == 0xF) ? 0 : (1 << ways);
                l3.fully_associative = (ways == 0xF);
                l3.size_kb = l3_size;
                caches_.push_back(l3);
            }
        }
    }

    void detect_uarch() noexcept {
        if (vendor_ == Vendor::Intel) {
            uarch_ = detect_intel_uarch();
        } else if (vendor_ == Vendor::AMD || vendor_ == Vendor::Hygon) {
            uarch_ = detect_amd_uarch();
        }
    }

    [[nodiscard]] Microarchitecture detect_intel_uarch() const noexcept {
        if (family_ != 6)
            return Microarchitecture::Unknown;

        switch (model_) {
        case 0x0F:
        case 0x16: return Microarchitecture::Merom;
        case 0x17:
        case 0x1D: return Microarchitecture::Penryn;
        case 0x1A:
        case 0x1E:
        case 0x1F:
        case 0x2E: return Microarchitecture::Nehalem;
        case 0x25:
        case 0x2C:
        case 0x2F: return Microarchitecture::Westmere;
        case 0x2A:
        case 0x2D: return Microarchitecture::SandyBridge;
        case 0x3A:
        case 0x3E: return Microarchitecture::IvyBridge;
        case 0x3C:
        case 0x3F:
        case 0x45:
        case 0x46: return Microarchitecture::Haswell;
        case 0x3D:
        case 0x47:
        case 0x4F:
        case 0x56: return Microarchitecture::Broadwell;
        case 0x4E:
        case 0x5E:
        case 0x55: return Microarchitecture::Skylake;
        case 0x8E: return Microarchitecture::KabyLake;
        case 0x9E: return (stepping_ >= 0xA) ? Microarchitecture::CoffeeLake : Microarchitecture::KabyLake;
        case 0x66: return Microarchitecture::CannonLake;
        case 0x7D:
        case 0x7E:
        case 0x9D: return Microarchitecture::IceLake;
        case 0x8C:
        case 0x8D: return Microarchitecture::TigerLake;
        case 0x97:
        case 0x9A:
        case 0xBE: return Microarchitecture::AlderLake;
        case 0xB7:
        case 0xBA:
        case 0xBF: return Microarchitecture::RaptorLake;
        case 0xAA:
        case 0xAC: return Microarchitecture::MeteorLake;
        case 0xC5:
        case 0xC6: return Microarchitecture::ArrowLake;
        default: return Microarchitecture::Unknown;
        }
    }

    [[nodiscard]] Microarchitecture detect_amd_uarch() const noexcept {
        if (family_ == 0x10)
            return Microarchitecture::K10;
        if (family_ == 0x15) {
            switch (model_ >> 4) {
            case 0:
            case 1: return Microarchitecture::Bulldozer;
            case 2: return Microarchitecture::Piledriver;
            case 3: return Microarchitecture::Steamroller;
            case 4:
            case 5:
            case 6: return Microarchitecture::Excavator;
            default: return Microarchitecture::Unknown;
            }
        }
        if (family_ == 0x17) {
            if (model_ <= 0x1F)
                return Microarchitecture::Zen;
            if (model_ <= 0x2F)
                return Microarchitecture::ZenPlus;
            if (model_ <= 0x71)
                return Microarchitecture::Zen2;
            return Microarchitecture::Zen3;
        }
        if (family_ == 0x19) {
            if (model_ <= 0x0F || (model_ >= 0x20 && model_ <= 0x2F))
                return Microarchitecture::Zen3;
            if (model_ >= 0x30 && model_ <= 0x5F)
                return Microarchitecture::Zen4;
            if (model_ >= 0x60)
                return Microarchitecture::Zen5;
        }
        return Microarchitecture::Unknown;
    }
};

// =============================================================================
// 便捷函数
// =============================================================================

[[nodiscard]] inline bool cpu_has(Feature f) noexcept { return CpuDetector::instance().has(f); }

[[nodiscard]] inline const char* cpu_vendor_name() noexcept {
    return CpuDetector::instance().vendor_string().data();
}

[[nodiscard]] inline const char* cpu_brand() noexcept {
    return CpuDetector::instance().brand_string().data();
}

[[nodiscard]] inline bool cpu_supports_sse() noexcept { return CpuDetector::instance().supports_sse(); }

[[nodiscard]] inline bool cpu_supports_avx() noexcept { return CpuDetector::instance().supports_avx(); }

[[nodiscard]] inline bool cpu_supports_avx2() noexcept { return CpuDetector::instance().supports_avx2(); }

[[nodiscard]] inline const char* cpu_max_simd() noexcept { return CpuDetector::instance().max_simd_level(); }

[[nodiscard]] inline const HybridTopology& cpu_hybrid_topology() noexcept {
    return CpuDetector::instance().hybrid_topology();
}

// 新增便捷函数
[[nodiscard]] inline std::vector<uint32_t> cpu_all_cores() noexcept {
    return CpuDetector::instance().get_all_core_ids();
}

[[nodiscard]] inline std::vector<uint32_t> cpu_online_cores() noexcept {
    return CpuDetector::instance().get_online_cores();
}

[[nodiscard]] inline std::vector<uint32_t> cpu_offline_cores() noexcept {
    return CpuDetector::instance().get_offline_cores();
}

[[nodiscard]] inline std::vector<uint32_t> cpu_isolated_cores() noexcept {
    return CpuDetector::instance().get_isolated_cores();
}

[[nodiscard]] inline bool cpu_hyperthreading_enabled() noexcept {
    return CpuDetector::instance().is_hyperthreading_enabled();
}
}  // namespace utils::hw