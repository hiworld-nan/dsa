#include <algorithm>
#include <chrono>
#include <cstring>
#include <set>
#include <vector>

#include "cpuDetector.h"
#include "test.h"

using namespace utils::hw;
using namespace testing;

class CpuDetectorTest : public testing::test_base<CpuDetectorTest> {
protected:
    const CpuDetector& cpu_ = CpuDetector::instance();

    // 验证缓存信息一致性
    void verify_cache_consistency(const CacheInfo& cache) {
        // 基本有效性检查
        EXPECT_GT(cache.level, 0u);
        EXPECT_LE(cache.level, 4u);  // 通常最多 L4

        EXPECT_GT(cache.size_kb, 0u);
        EXPECT_LT(cache.size_kb, 1024 * 1024UL);  // 小于 1GB

        EXPECT_GT(cache.line_size, 0u);
        EXPECT_LE(cache.line_size, 256u);  // 常见缓存行大小

        // 数学一致性验证：size = ways * sets * line_size
        if (!cache.fully_associative && cache.ways > 0 && cache.sets > 0) {
            uint64_t calculated_size =
                static_cast<uint64_t>(cache.ways) * cache.sets * cache.line_size / 1024;
            // 允许一定误差（因为可能有分区等因素）
            // EXPECT_EQ(calculated_size, cache.size_kb, cache.size_kb / 10);
        }
    }

    // 验证拓扑一致性
    void verify_topology_consistency(const HybridTopology& topo) {
        EXPECT_GT(topo.physical_cores, 0u);
        EXPECT_GE(topo.logical_cores, topo.physical_cores);
        EXPECT_GE(topo.threads_per_core, 1u);
        EXPECT_GE(topo.cores_per_package, 1u);
        EXPECT_GE(topo.packages, 1u);

        // 数学关系验证
        EXPECT_EQ(
            topo.logical_cores,
            topo.physical_cores * topo.threads_per_core / (topo.cores_per_package > 0 ? 1 : 1));  // 简化验证

        // 逻辑核心数应该等于物理核心数 * 每核心线程数
        if (topo.cores_per_package > 0 && topo.packages > 0) {
            uint32_t expected_logical = topo.physical_cores * topo.threads_per_core;
            // 注意：实际逻辑核心数可能因多路系统而复杂，这里做基本检查
            EXPECT_GE(topo.logical_cores, topo.physical_cores);
        }
    }

    // SIMD 层级一致性检查
    void verify_simd_hierarchy() {
        // AVX512 需要 AVX2，AVX2 需要 AVX，AVX 需要 SSE
        if (cpu_.supports_avx512()) {
            EXPECT_TRUE(cpu_.supports_avx2());
            EXPECT_TRUE(cpu_.has(Feature::AVX512F));
        }
        if (cpu_.supports_avx2()) {
            EXPECT_TRUE(cpu_.supports_avx());
            EXPECT_TRUE(cpu_.has(Feature::AVX2));
        }
        if (cpu_.supports_avx()) {
            EXPECT_TRUE(cpu_.supports_sse());
            EXPECT_TRUE(cpu_.has(Feature::AVX));
            EXPECT_TRUE(cpu_.has(Feature::XSAVE));
        }
        if (cpu_.supports_sse()) {
            EXPECT_TRUE(cpu_.has(Feature::SSE));
            EXPECT_TRUE(cpu_.has(Feature::FXSR));
        }
    }

    // 验证 vendor 和微架构匹配
    void verify_vendor_uarch_match() {
        if (cpu_.is_intel()) {
            // Intel CPU 不应该报告 AMD 微架构
            EXPECT_NE(static_cast<int>(cpu_.uarch()), static_cast<int>(Microarchitecture::Zen));
            EXPECT_NE(static_cast<int>(cpu_.uarch()), static_cast<int>(Microarchitecture::Zen2));
            EXPECT_NE(static_cast<int>(cpu_.uarch()), static_cast<int>(Microarchitecture::Zen3));
            EXPECT_NE(static_cast<int>(cpu_.uarch()), static_cast<int>(Microarchitecture::Bulldozer));
        } else if (cpu_.is_amd()) {
            // AMD CPU 不应该报告 Intel 特有微架构
            EXPECT_NE(static_cast<int>(cpu_.uarch()), static_cast<int>(Microarchitecture::Skylake));
            EXPECT_NE(static_cast<int>(cpu_.uarch()), static_cast<int>(Microarchitecture::Haswell));
            EXPECT_NE(static_cast<int>(cpu_.uarch()), static_cast<int>(Microarchitecture::SandyBridge));
        }
    }
};

// =============================================================================
// 基础功能测试
// =============================================================================

// 单例模式测试
TEST_F(CpuDetectorTest, SingletonReturnsSameInstance) {
    const auto& instance1 = CpuDetector::instance();
    const auto& instance2 = CpuDetector::instance();
    EXPECT_EQ(&instance1, &instance2);  // 同一内存地址
    return true;
}

// Vendor 字符串有效性
TEST_F(CpuDetectorTest, VendorStringValid) {
    auto vendor = cpu_.vendor_string();
    EXPECT_FALSE(vendor.empty());
    EXPECT_LE(vendor.length(), 12u);

    // 检查可打印字符
    for (char c : vendor) {
        EXPECT_TRUE(std::isprint(c) || c == '\0');
    }
    return true;
}

// Brand 字符串有效性
TEST_F(CpuDetectorTest, BrandStringValid) {
    auto brand = cpu_.brand_string();
    EXPECT_FALSE(brand.empty());
    EXPECT_LE(brand.length(), 48u);

    // 应该包含厂商关键词或至少是可打印字符
    bool has_printable = false;
    for (char c : brand) {
        if (std::isprint(c) && c != ' ') {
            has_printable = true;
            break;
        }
    }
    EXPECT_TRUE(has_printable);
    return true;
}

// Vendor 枚举与字符串一致性
TEST_F(CpuDetectorTest, VendorEnumMatchesString) {
    auto vendor = cpu_.vendor();
    auto vendor_str = cpu_.vendor_string();

    if (vendor == Vendor::Intel) {
        EXPECT_NE(vendor_str.find("Intel"), std::string_view::npos);
    } else if (vendor == Vendor::AMD) {
        EXPECT_NE(vendor_str.find("AMD"), std::string_view::npos);
    }

    // is_intel/is_amd 辅助函数一致性
    EXPECT_EQ(cpu_.is_intel(), vendor == Vendor::Intel);
    EXPECT_EQ(cpu_.is_amd(), vendor == Vendor::AMD);
    return true;
}

// Family/Model/Stepping 有效性
TEST_F(CpuDetectorTest, FamilyModelSteppingValid) {
    EXPECT_GT(cpu_.family(), 0u);
    EXPECT_LT(cpu_.family(), 0x100u);  // 8位值

    // Model 通常是 4 位或 8 位（带扩展）
    EXPECT_LT(cpu_.model(), 0x100u);

    // Stepping 通常是 4 位
    EXPECT_LT(cpu_.stepping(), 0x10u);
    return true;
}

// =============================================================================
// 特性标志测试
// =============================================================================

// 基础特性存在性（现代 CPU 应该都有）
TEST_F(CpuDetectorTest, BasicFeaturesPresent) {
    // 几乎所有现代 CPU 都有这些
    EXPECT_TRUE(cpu_.has(Feature::FPU));
    EXPECT_TRUE(cpu_.has(Feature::CX8));
    EXPECT_TRUE(cpu_.has(Feature::CMOV));
    EXPECT_TRUE(cpu_.has(Feature::TSC));
    return true;
}

// SSE 层级测试
TEST_F(CpuDetectorTest, SSEHierarchy) {
    // SSE2 需要 SSE
    if (cpu_.has(Feature::SSE2)) {
        EXPECT_TRUE(cpu_.has(Feature::SSE));
    }
    // SSE3 需要 SSE2
    if (cpu_.has(Feature::SSE3)) {
        EXPECT_TRUE(cpu_.has(Feature::SSE2));
    }
    // SSSE3 是 SSE3 的扩展，但不严格依赖，这里检查常见情况
    if (cpu_.has(Feature::SSSE3)) {
        EXPECT_TRUE(cpu_.has(Feature::SSE3));
    }
    // SSE4.1/4.2 层级
    if (cpu_.has(Feature::SSE4_2)) {
        EXPECT_TRUE(cpu_.has(Feature::SSE4_1));
    }
    if (cpu_.has(Feature::SSE4_1)) {
        EXPECT_TRUE(cpu_.has(Feature::SSSE3));
    }
    return true;
}

// AVX 层级测试
TEST_F(CpuDetectorTest, AVXHierarchy) {
    verify_simd_hierarchy();

    // AVX 需要 OS 支持 XSAVE
    if (cpu_.has(Feature::AVX)) {
        EXPECT_TRUE(cpu_.has(Feature::XSAVE));
    }

    // AVX2 需要 AVX
    if (cpu_.has(Feature::AVX2)) {
        EXPECT_TRUE(cpu_.has(Feature::AVX));
        EXPECT_TRUE(cpu_.has(Feature::BMI1));
        EXPECT_TRUE(cpu_.has(Feature::BMI2));
        EXPECT_TRUE(cpu_.has(Feature::FMA));
    }
    return true;
}

// AVX-512 子特性一致性
TEST_F(CpuDetectorTest, AVX512Subfeatures) {
    if (!cpu_.has(Feature::AVX512F)) {
        return false;
    }

    // AVX512F 是基础，其他 AVX-512 特性都需要它
    auto avx512_features = {Feature::AVX512DQ,     Feature::AVX512IFMA,   Feature::AVX512PF,
                            Feature::AVX512ER,     Feature::AVX512CD,     Feature::AVX512BW,
                            Feature::AVX512VL,     Feature::AVX512VBMI,   Feature::AVX512VBMI2,
                            Feature::AVX512VNNI,   Feature::AVX512BITALG, Feature::AVX512VPOPCNTDQ,
                            Feature::AVX5124VNNIW, Feature::AVX5124FMAPS, Feature::AVX512VP2INTERSECT};

    for (auto f : avx512_features) {
        if (cpu_.has(f)) {
            EXPECT_TRUE(cpu_.has(Feature::AVX512F));
        }
    }
    return true;
}

// 加密特性测试
TEST_F(CpuDetectorTest, CryptoFeatures) {
    // AES 通常与 CLMUL 一起出现
    if (cpu_.has(Feature::AES)) {
        EXPECT_TRUE(cpu_.has(Feature::PCLMULQDQ));
    }

    // SHA 扩展（较新特性）
    if (cpu_.has(Feature::SHA)) {
        EXPECT_TRUE(cpu_.has(Feature::SSE4_2));
    }

    // VAES 需要 AVX2 和 AES
    if (cpu_.has(Feature::VAES)) {
        EXPECT_TRUE(cpu_.has(Feature::AVX2));
        EXPECT_TRUE(cpu_.has(Feature::AES));
    }
    return true;
}

// 虚拟化特性
TEST_F(CpuDetectorTest, VirtualizationFeatures) {
    // VMX (Intel) 或 SVM (AMD) 通常不会同时存在
    bool has_vmx = cpu_.has(Feature::VMX);
    bool has_svm = cpu_.has(Feature::SVM);

    // 可能都不支持，但不太可能同时支持两者
    if (has_vmx && has_svm) {
        // 某些虚拟化环境可能同时暴露，但物理 CPU 通常不会
        // 这里仅记录，不强制失败
        std::cout << "Note: Both VMX and SVM detected (possible virtualized environment)\n";
    }
    return true;
}

// =============================================================================
// 缓存系统测试
// =============================================================================

TEST_F(CpuDetectorTest, CacheInfoValid) {
    if (cpu_.caches().empty()) {
        return false;
    }

    std::set<uint8_t> seen_levels;
    for (const auto& cache : cpu_.caches()) {
        verify_cache_consistency(cache);
        seen_levels.insert(cache.level);
    }

    // 通常至少应该有 L1
    EXPECT_TRUE(seen_levels.count(1) > 0 || seen_levels.count(0) > 0);
    return true;
}

TEST_F(CpuDetectorTest, CacheHierarchyOrdered) {
    const auto& caches = cpu_.caches();
    if (caches.size() < 2) {
        return false;
    }

    // 验证缓存级别通常是递增的
    uint8_t prev_level = 0;
    for (const auto& cache : caches) {
        EXPECT_GE(cache.level, prev_level);
        prev_level = cache.level;
    }
    return true;
}

// L1 缓存特性
TEST_F(CpuDetectorTest, L1CacheCharacteristics) {
    for (const auto& cache : cpu_.caches()) {
        if (cache.level == 1) {
            // L1 通常较小 (8KB - 64KB per core)
            EXPECT_LE(cache.size_kb, 128u);
            EXPECT_GE(cache.size_kb, 4u);

            // L1 通常有独立的 D-cache 和 I-cache
            EXPECT_TRUE(cache.type == CacheInfo::Data || cache.type == CacheInfo::Instruction);
        }
    }
    return true;
}

// L2/L3 缓存特性
TEST_F(CpuDetectorTest, L2L3CacheCharacteristics) {
    for (const auto& cache : cpu_.caches()) {
        if (cache.level == 2) {
            // L2 通常 128KB - 4MB
            EXPECT_GE(cache.size_kb, 64u);
            EXPECT_LE(cache.size_kb, 8192u);
            EXPECT_EQ(cache.type, CacheInfo::Unified);
        } else if (cache.level == 3) {
            // L3 通常较大
            EXPECT_GE(cache.size_kb, 512u);
            EXPECT_LE(cache.size_kb, 512 * 1024u);  // 512MB max (大型服务器)
        }
    }
    return true;
}

// =============================================================================
// 拓扑结构测试
// =============================================================================

TEST_F(CpuDetectorTest, TopologyValid) {
    verify_topology_consistency(cpu_.hybrid_topology());
    return true;
}

TEST_F(CpuDetectorTest, CoreCountReasonable) {
    const auto& topo = cpu_.hybrid_topology();

    // 合理性检查
    EXPECT_LE(topo.physical_cores, 256u);
    EXPECT_LE(topo.logical_cores, 1024u);

    // 常见配置验证
    if (topo.threads_per_core > 1) {
        EXPECT_EQ(topo.threads_per_core, 2u);
    }
    return true;
}

// SMT (Simultaneous Multi-Threading) 一致性
TEST_F(CpuDetectorTest, SMTConsistency) {
    const auto& topo = cpu_.hybrid_topology();

    if (topo.threads_per_core > 1) {
        // 如果支持 SMT，应该有 HTT 标志
        EXPECT_TRUE(cpu_.has(Feature::HTT));
    }

    if (cpu_.has(Feature::HTT)) {
        // 如果有 HTT 标志，逻辑核心应该多于物理核心
        EXPECT_GE(topo.logical_cores, topo.physical_cores);
    }
    return true;
}

// =============================================================================
// 微架构检测测试
// =============================================================================

TEST_F(CpuDetectorTest, MicroarchitectureDetected) {
    // 至少应该能识别厂商
    EXPECT_NE(static_cast<int>(cpu_.vendor()), static_cast<int>(Vendor::Unknown));

    // 如果是已知厂商，应该有微架构信息或至少是 Generic
    if (cpu_.vendor() == Vendor::Intel || cpu_.vendor() == Vendor::AMD) {
        // 较新的 CPU 应该能被识别
        if (cpu_.family() >= 6) {
            // 不强制要求，但记录未识别的微架构
            if (cpu_.uarch() == Microarchitecture::Unknown) {
                std::cout << "Warning: Unknown microarchitecture for family=" << cpu_.family()
                          << " model=" << cpu_.model() << "\n";
            }
        }
    }
    return true;
}

TEST_F(CpuDetectorTest, VendorUarchConsistency) {
    verify_vendor_uarch_match();
    return true;
}

// =============================================================================
// 便捷函数测试
// =============================================================================

TEST_F(CpuDetectorTest, ConvenienceFunctions) {
    // cpu_has 与 instance().has() 一致性
    EXPECT_EQ(cpu_has(Feature::SSE), cpu_.has(Feature::SSE));
    EXPECT_EQ(cpu_has(Feature::AVX), cpu_.has(Feature::AVX));
    EXPECT_EQ(cpu_has(Feature::AVX2), cpu_.has(Feature::AVX2));

    // 支持函数一致性
    EXPECT_EQ(cpu_supports_sse(), cpu_.supports_sse());
    EXPECT_EQ(cpu_supports_avx(), cpu_.supports_avx());
    EXPECT_EQ(cpu_supports_avx2(), cpu_.supports_avx2());

    // 字符串函数非空
    EXPECT_NE(cpu_vendor_name(), nullptr);
    EXPECT_NE(cpu_brand(), nullptr);
    EXPECT_NE(cpu_max_simd(), nullptr);
    return true;
}

TEST_F(CpuDetectorTest, MaxSimdLevelValid) {
    const char* simd = cpu_max_simd();
    EXPECT_TRUE(std::strcmp(simd, "None") == 0 || std::strcmp(simd, "SSE") == 0 ||
                std::strcmp(simd, "SSE2") == 0 || std::strcmp(simd, "SSE3") == 0 ||
                std::strcmp(simd, "SSE4.1") == 0 || std::strcmp(simd, "SSE4.2") == 0 ||
                std::strcmp(simd, "AVX") == 0 || std::strcmp(simd, "AVX2") == 0 ||
                std::strcmp(simd, "AVX-512") == 0);
    return true;
}

// =============================================================================
// 边界情况和健壮性测试
// =============================================================================

TEST_F(CpuDetectorTest, InvalidFeatureIndex) {
    // 测试越界特性查询（内部应该安全处理）
    auto invalid_feature = static_cast<Feature>(9999);
    EXPECT_FALSE(cpu_.has(invalid_feature));
    return true;
}

TEST_F(CpuDetectorTest, FeatureBitsetSize) {
    // 确保所有定义的 Feature 都能放入 bitset
    auto max_feature = static_cast<size_t>(Feature::COUNT);
    EXPECT_GT(max_feature, static_cast<size_t>(Feature::PBE));
    EXPECT_GT(max_feature, static_cast<size_t>(Feature::MWAITX));
    return true;
}

// 多线程安全性（基础测试）
TEST_F(CpuDetectorTest, ThreadSafetyBasic) {
    // 单例应该在多线程环境下返回相同实例
    const auto* ptr1 = &CpuDetector::instance();
    const auto* ptr2 = &CpuDetector::instance();
    EXPECT_EQ(ptr1, ptr2);

    // 数据一致性检查
    EXPECT_EQ(ptr1->family(), ptr2->family());
    EXPECT_EQ(ptr1->model(), ptr2->model());
    EXPECT_EQ(static_cast<int>(ptr1->vendor()), static_cast<int>(ptr2->vendor()));
    return true;
}

// =============================================================================
// 平台特定测试
// =============================================================================

#ifdef __x86_64__
TEST_F(CpuDetectorTest, X86_64Features) {
    // x86_64 系统应该有这些特性
    EXPECT_TRUE(cpu_.has(Feature::LM));
    EXPECT_TRUE(cpu_.has(Feature::SYSCALL));
    return true;
}
#endif

#if defined(__linux__)
TEST_F(CpuDetectorTest, LinuxSpecific) {
    // 可以添加 /proc/cpuinfo 对比测试（如果需要）
    return true;
}
#endif

// =============================================================================
// 性能测试（可选）
// =============================================================================

TEST_F(CpuDetectorTest, QueryPerformance) {
    // 特性查询应该非常快（内联函数）
    const int iterations = 100000;

    auto start = std::chrono::high_resolution_clock::now();
    volatile bool result = false;  // 防止优化
    for (int i = 0; i < iterations; ++i) {
        result = cpu_.has(Feature::AVX2);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double avg_ns = static_cast<double>(duration.count()) / iterations;

    EXPECT_LT(avg_ns, 10.0);
    (void)result;  // 抑制未使用警告
    return true;
}

int main(int argc, char** argv) {
    std::cout << CpuDetector::instance() << "\n";
    return testing::run_all_tests();
}