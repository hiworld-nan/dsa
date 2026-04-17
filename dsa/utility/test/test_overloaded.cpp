/**
 * @file test_overloaded.cpp
 * @brief overloaded 模板单元测试
 * @version 1.0.0
 */

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "../../test/test.h"
#include "../utility.h"

using namespace utils;

// =============================================================================
// 基础功能测试
// =============================================================================

TEST(Overloaded, BasicTwoTypes) {
    auto func = overloaded{[](int x) { return x * 2; }, [](double x) { return x * 3.0; }};

    EXPECT_EQ(func(5), 10);
    EXPECT_EQ(func(2.5), 7.5);
    return true;
}

TEST(Overloaded, ThreeTypes) {
    auto concat = overloaded{[](int x) { return std::to_string(x) + "_int"; },
                             [](const std::string& s) { return s + "_str"; },
                             [](double x) { return std::to_string(x) + "_double"; }};

    EXPECT_EQ(concat(42), std::string("42_int"));
    EXPECT_EQ(concat(std::string("hello")), std::string("hello_str"));
    return true;
}

TEST(Overloaded, CapturingLambdas) {
    int counter = 0;
    double total = 0.0;

    auto processor =
        overloaded{[&counter](int x) mutable { counter += x; }, [&total](double x) mutable { total += x; }};

    processor(10);
    processor(5.5);
    processor(20);
    processor(2.5);

    EXPECT_EQ(counter, 30);
    EXPECT_EQ(total, 8.0);
    return true;
}

TEST(Overloaded, ConstCorrectness) {
    const auto const_func = overloaded{[](int x) { return x + 1; },
                                       [](const std::string& s) { return static_cast<int>(s.length()); }};

    EXPECT_EQ(const_func(5), 6);
    EXPECT_EQ(const_func("test"), 4);
    return true;
}

TEST(Overloaded, WithStdVisit) {
    using Variant = std::variant<int, double, std::string>;

    auto visitor = overloaded{[](int i) { return "int: " + std::to_string(i); },
                              [](double d) { return "double: " + std::to_string(d); },
                              [](const std::string& s) { return "string: " + s; }};

    Variant v1 = 42;
    Variant v2 = 3.14;
    Variant v3 = std::string("hello");

    std::string r1 = std::visit(visitor, v1);
    std::string r3 = std::visit(visitor, v3);

    EXPECT_TRUE(r1.find("int: 42") != std::string::npos);
    EXPECT_EQ(r3, std::string("string: hello"));
    return true;
}

TEST(Overloaded, MoveSemantics) {
    auto handler = overloaded{[](std::unique_ptr<int>&& ptr) { return *ptr; },
                              [](std::string&& str) { return static_cast<int>(str.size()); }};

    auto ptr = std::make_unique<int>(42);
    EXPECT_EQ(handler(std::move(ptr)), 42);

    std::string str = "hello";
    EXPECT_EQ(handler(std::move(str)), 5);
    return true;
}

TEST(Overloaded, SingleOverload) {
    auto single = overloaded{[](int x) { return x + 1; }};
    EXPECT_EQ(single(5), 6);
    return true;
}

TEST(Overloaded, ManyOverloads) {
    auto many = overloaded{[](int) { return 1; }, [](double) { return 2; }, [](char) { return 3; },
                           [](float) { return 4; }, [](long) { return 5; }};

    EXPECT_EQ(many(10), 1);
    EXPECT_EQ(many(3.14), 2);
    EXPECT_EQ(many('a'), 3);
    return true;
}

TEST(Overloaded, NoexceptLambda) {
    auto noexcept_func = overloaded{[](int) noexcept { return 1; }, [](double) noexcept { return 2; }};
    EXPECT_EQ(noexcept_func(5), 1);
    EXPECT_EQ(noexcept_func(2.5), 2);
    return true;
}

TEST(Overloaded, VariantVector) {
    using Variant = std::variant<int, std::string>;

    std::vector<Variant> variants = {100, std::string("test"), -5};
    int int_count = 0;
    int str_count = 0;

    for (const auto& v : variants) {
        std::visit(overloaded{[&](int) { ++int_count; }, [&](const std::string&) { ++str_count; }}, v);
    }

    EXPECT_EQ(int_count, 2);
    EXPECT_EQ(str_count, 1);
    return true;
}

// =============================================================================
// 主函数
// =============================================================================

int main() { return testing::run_all_tests(); }
