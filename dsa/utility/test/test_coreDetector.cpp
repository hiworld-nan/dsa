/**
 * @file test_coreDetector.cpp
 * @brief CPU 检测器单元测试
 * @version 1.0.0
 */

#include <iostream>

#include "../../test/test.h"
#include "../utility.h"

using namespace utils;

// =============================================================================
// Feature 枚举测试
// =============================================================================

TEST(Feature, EnumValues) {
    EXPECT_EQ(static_cast<int>(Feature::SSE), 25);
    EXPECT_EQ(static_cast<int>(Feature::AVX), 32 + 28);
    EXPECT_EQ(static_cast<int>(Feature::AVX2), 64 + 5);
    EXPECT_EQ(static_cast<int>(Feature::AVX512F), 64 + 16);
    EXPECT_EQ(static_cast<int>(Feature::HTT), 28);
    return true;
}

// =============================================================================
// CoreDetector 测试
// =============================================================================

TEST(CoreDetector, SingletonAccess) {
    auto& det = CoreDetector::instance();
    // 二次访问应返回同一实例
    auto& det2 = CoreDetector::instance();
    EXPECT_EQ(&det, &det2);
    return true;
}

TEST(CoreDetector, ArchDetection) {
    auto& det = CoreDetector::instance();
    CPUArch arch = det.get_arch();
    // 在 x86_64 Linux 上应该是 INTEL 或 AMD
    EXPECT_TRUE(arch == CPUArch::INTEL || arch == CPUArch::AMD);
    return true;
}

TEST(CoreDetector, ThreadCount) {
    auto& det = CoreDetector::instance();
    EXPECT_GT(det.get_num_of_threads(), static_cast<uint32_t>(0));
    return true;
}

TEST(CoreDetector, FeatureDetection) {
    auto& det = CoreDetector::instance();
    // x86_64 几乎一定支持 FPU 和 SSE
    EXPECT_TRUE(det.has(Feature::FPU));
    EXPECT_TRUE(det.has(Feature::SSE));
    EXPECT_TRUE(det.has(Feature::SSE2));
    return true;
}

TEST(CoreDetector, ConvenienceMethods) {
    auto& det = CoreDetector::instance();
    // 在现代 x86_64 上至少支持 SSE
    EXPECT_TRUE(det.support_sse());
    return true;
}

TEST(CoreDetector, CacheInfo) {
    auto& det = CoreDetector::instance();
    const auto& caches = det.get_cache_info();
    EXPECT_GT(caches.size(), static_cast<size_t>(0));

    for (const auto& c : caches) {
        EXPECT_GT(c.level_, static_cast<uint32_t>(0));
        EXPECT_GT(c.size_, static_cast<uint32_t>(0));
        EXPECT_GT(c.line_size_, static_cast<uint32_t>(0));
    }
    return true;
}

TEST(CoreDetector, NumaInfo) {
    auto& det = CoreDetector::instance();
    EXPECT_GT(det.get_num_of_numa_nodes(), static_cast<uint32_t>(0));
    return true;
}

TEST(CoreDetector, OnlineCpus) {
    auto& det = CoreDetector::instance();
    const auto& cpus = det.get_online_cpus();
    EXPECT_GT(cpus.size(), static_cast<size_t>(0));
    return true;
}

TEST(CoreDetector, VirtualizationDetection) {
    // 只要不崩溃就 OK
    bool is_virt = CoreDetector::is_virtualized_env();
    (void)is_virt;
    return true;
}

TEST(CoreDetector, OstreamOutput) {
    auto& det = CoreDetector::instance();
    std::ostringstream oss;
    oss << det;
    std::string output = oss.str();
    EXPECT_FALSE(output.empty());
    // 应包含 CPU Architecture
    EXPECT_TRUE(output.find("Architecture") != std::string::npos);
    return true;
}

// =============================================================================
// 主函数
// =============================================================================

int main() {
    auto& detector = CoreDetector::instance();
    std::cout << detector << "\n";

    return testing::run_all_tests();
}
