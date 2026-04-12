#pragma once

#include "../common/macros.h"
#include "./detail/core.h"
#include "./detail/expect.h"

namespace testing {
using namespace common::macros;

#ifndef CHECK_COMPILE_TIME
#define CHECK_COMPILE_TIME(...)                                                                             \
    do {                                                                                                    \
        static_assert(__VA_ARGS__,                                                                          \
                      "compile-time check failed: " #__VA_ARGS__ " @  " __FILE__ " : " TOSTRING(__LINE__)); \
        std::cout << "[   ✓ PASS   ] " << #__VA_ARGS__ << " @ " << __FILE__ << ":" << __LINE__ << "\n";     \
        testing::test_results::instance().pass_check();                                                     \
    } while (0)

#endif

#ifndef CHECK
#define CHECK(...)                                                                                          \
    do {                                                                                                    \
        if (!(__VA_ARGS__)) {                                                                               \
            std::cerr << "[   ✗ FAIL   ] " << #__VA_ARGS__ << " @ " << __FILE__ << ":" << __LINE__ << "\n"; \
            testing::test_results::instance().fail_check();                                                 \
        } else {                                                                                            \
            std::cout << "[   ✓ PASS   ] " << #__VA_ARGS__ << " @ " << __FILE__ << ":" << __LINE__ << "\n"; \
            testing::test_results::instance().pass_check();                                                 \
        }                                                                                                   \
    } while (0)

#endif

#define CHECK_NOT(_expr_, ...) CHECK(!(_expr_), ##__VA_ARGS__)
#define CHECK_EQ(_a_, _b_, ...) CHECK(((_a_) == (_b_)), ##__VA_ARGS__)
#define CHECK_NE(_a_, _b_, ...) CHECK(((_a_) != (_b_)), ##__VA_ARGS__)
#define CHECK_GT(_a_, _b_, ...) CHECK(((_a_) > (_b_)), ##__VA_ARGS__)
#define CHECK_LT(_a_, _b_, ...) CHECK(((_a_) < (_b_)), ##__VA_ARGS__)
#define CHECK_GE(_a_, _b_, ...) CHECK(((_a_) >= (_b_)), ##__VA_ARGS__)
#define CHECK_LE(_a_, _b_, ...) CHECK(((_a_) <= (_b_)), ##__VA_ARGS__)

#define EXPECT_TRUE(condition) testing::expect(condition, __FILE__, __LINE__)
#define EXPECT_FALSE(condition) testing::expect(!(condition), __FILE__, __LINE__)
#define EXPECT_EQ(_a_, _b_) testing::expect_eq(_a_, _b_, __FILE__, __LINE__)
#define EXPECT_NE(_a_, _b_) testing::expect_ne(_a_, _b_, __FILE__, __LINE__)
#define EXPECT_GT(_a_, _b_) testing::expect_gt(_a_, _b_, __FILE__, __LINE__)
#define EXPECT_LT(_a_, _b_) testing::expect_lt(_a_, _b_, __FILE__, __LINE__)
#define EXPECT_GE(_a_, _b_) testing::expect_ge(_a_, _b_, __FILE__, __LINE__)
#define EXPECT_LE(_a_, _b_) testing::expect_le(_a_, _b_, __FILE__, __LINE__)

#define TEST_F(TypeName, TestName)                                                              \
    struct TypeName##_##TestName##_impl : public TypeName {                                     \
        using TypeName::TypeName;                                                               \
        bool test_body();                                                                       \
    };                                                                                          \
    static const int s_##TypeName##_##TestName = [] {                                           \
        testing::test_repository::instance().register_test(#TypeName "." #TestName,             \
                                                           &TypeName##_##TestName##_impl::run); \
        return 0;                                                                               \
    }();                                                                                        \
    bool TypeName##_##TestName##_impl::test_body()

#define TEST(SuiteName, TestName)                                                    \
    static bool SuiteName##_##TestName();                                            \
    static const int s_##SuiteName##_##TestName = [] {                               \
        testing::test_repository::instance().register_test(#SuiteName "." #TestName, \
                                                           &SuiteName##_##TestName); \
        return 0;                                                                    \
    }();                                                                             \
    static bool SuiteName##_##TestName()

}  // namespace testing