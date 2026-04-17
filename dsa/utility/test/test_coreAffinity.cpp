/**
 * @file test_coreAffinity.cpp
 * @brief CPU 亲和性管理单元测试
 * @version 1.0.0
 */

#include <thread>

#include "../../test/test.h"
#include "../utility.h"

using namespace utils;

// =============================================================================
// CpuSet 测试
// =============================================================================

TEST(CpuSet, DefaultEmpty) {
    CpuSet set;
    EXPECT_EQ(set.count(), static_cast<std::size_t>(0));
    EXPECT_TRUE(set.get_cpus().empty());
    return true;
}

TEST(CpuSet, AddAndContains) {
    CpuSet set;
    EXPECT_TRUE(set.add_cpu(0));
    EXPECT_TRUE(set.add_cpu(2));
    EXPECT_TRUE(set.contains(0));
    EXPECT_TRUE(set.contains(2));
    EXPECT_FALSE(set.contains(1));
    EXPECT_EQ(set.count(), static_cast<std::size_t>(2));
    return true;
}

TEST(CpuSet, RemoveCpu) {
    CpuSet set;
    set.add_cpu(0);
    set.add_cpu(1);
    set.remove_cpu(0);
    EXPECT_FALSE(set.contains(0));
    EXPECT_TRUE(set.contains(1));
    EXPECT_EQ(set.count(), static_cast<std::size_t>(1));
    return true;
}

TEST(CpuSet, Clear) {
    CpuSet set;
    set.add_cpu(0);
    set.add_cpu(1);
    set.clear();
    EXPECT_EQ(set.count(), static_cast<std::size_t>(0));
    return true;
}

TEST(CpuSet, GetCpus) {
    CpuSet set;
    set.add_cpu(1);
    set.add_cpu(3);
    set.add_cpu(5);
    auto cpus = set.get_cpus();
    EXPECT_EQ(cpus.size(), static_cast<std::size_t>(3));
    EXPECT_EQ(cpus[0], static_cast<std::size_t>(1));
    EXPECT_EQ(cpus[1], static_cast<std::size_t>(3));
    EXPECT_EQ(cpus[2], static_cast<std::size_t>(5));
    return true;
}

TEST(CpuSet, OutOfRange) {
    CpuSet set;
    EXPECT_FALSE(set.add_cpu(CPU_SETSIZE));
    EXPECT_FALSE(set.contains(CPU_SETSIZE));
    return true;
}

// =============================================================================
// CpuAffinity 测试
// =============================================================================

TEST(CpuAffinity, GetAvailableCpus) {
    auto n = CpuAffinity::get_available_cpus();
    EXPECT_GT(n, static_cast<std::size_t>(0));
    return true;
}

TEST(CpuAffinity, GetSetThreadAffinity) {
    auto original = CpuAffinity::get_thread_affinity();
    EXPECT_TRUE(original.has_value());

    // 保存原始亲和性
    auto orig_cpus = original->get_cpus();

    // 绑定到 CPU 0
    bool pinned = CpuAffinity::pin_to_cpu(0);
    if (pinned) {
        auto new_affinity = CpuAffinity::get_thread_affinity();
        EXPECT_TRUE(new_affinity.has_value());
        EXPECT_TRUE(new_affinity->contains(0));

        // 恢复
        (void)CpuAffinity::reset_affinity();
    }
    return true;
}

TEST(CpuAffinity, PinToMultipleCpus) {
    std::size_t available = CpuAffinity::get_available_cpus();
    if (available >= 2) {
        bool pinned = CpuAffinity::pin_to_cpus({0, 1});
        if (pinned) {
            auto affinity = CpuAffinity::get_thread_affinity();
            EXPECT_TRUE(affinity.has_value());
            EXPECT_TRUE(affinity->contains(0));
            EXPECT_TRUE(affinity->contains(1));

            (void)CpuAffinity::reset_affinity();
        }
    }
    return true;
}

TEST(CpuAffinity, ResetAffinity) {
    bool reset = CpuAffinity::reset_affinity();
    EXPECT_TRUE(reset);

    auto affinity = CpuAffinity::get_thread_affinity();
    EXPECT_TRUE(affinity.has_value());
    EXPECT_GT(affinity->count(), static_cast<std::size_t>(0));
    return true;
}

TEST(CpuAffinity, ProcessAffinity) {
    auto proc = CpuAffinity::get_process_affinity();
    EXPECT_TRUE(proc.has_value());
    return true;
}

// =============================================================================
// 字符串序列化测试
// =============================================================================

TEST(CpuAffinity, ToStringSingle) {
    CpuSet set;
    set.add_cpu(0);
    EXPECT_EQ(CpuAffinity::to_string(set), std::string("0"));
    return true;
}

TEST(CpuAffinity, ToStringRange) {
    CpuSet set;
    set.add_cpu(0);
    set.add_cpu(1);
    set.add_cpu(2);
    EXPECT_EQ(CpuAffinity::to_string(set), std::string("0-2"));
    return true;
}

TEST(CpuAffinity, ToStringMixed) {
    CpuSet set;
    set.add_cpu(0);
    set.add_cpu(1);
    set.add_cpu(2);
    set.add_cpu(4);
    set.add_cpu(7);
    EXPECT_EQ(CpuAffinity::to_string(set), std::string("0-2,4,7"));
    return true;
}

TEST(CpuAffinity, ToStringEmpty) {
    CpuSet set;
    EXPECT_EQ(CpuAffinity::to_string(set), std::string("none"));
    return true;
}

TEST(CpuAffinity, FromString) {
    auto set = CpuAffinity::from_string("0-2,4,7");
    EXPECT_TRUE(set.contains(0));
    EXPECT_TRUE(set.contains(1));
    EXPECT_TRUE(set.contains(2));
    EXPECT_TRUE(set.contains(4));
    EXPECT_TRUE(set.contains(7));
    EXPECT_FALSE(set.contains(3));
    EXPECT_FALSE(set.contains(5));
    EXPECT_EQ(set.count(), static_cast<std::size_t>(5));
    return true;
}

TEST(CpuAffinity, FromStringRoundTrip) {
    auto set = CpuAffinity::from_string("0-2,4,7");
    auto str = CpuAffinity::to_string(set);
    EXPECT_EQ(str, std::string("0-2,4,7"));
    return true;
}

// =============================================================================
// 主函数
// =============================================================================

int main() { return testing::run_all_tests(); }
