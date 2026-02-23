#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>

#include "core.h"

namespace detail {

struct Compare {
    template <class T, class U>
    struct common {
        using type = std::common_type_t<T, U>;

        static constexpr bool is_floating_point =
            std::is_floating_point<T>::value || std::is_floating_point<U>::value;
        static constexpr type epsilon = std::is_same<type, float>::value ? 1e-6 : 1e-9;
    };

    template <class T, class U>
    static inline bool equal(T actual, U expected) {
        if constexpr (std::is_arithmetic<T>::value && std::is_arithmetic<U>::value) {
            using common_t = typename common<T, U>::type;
            const common_t a = static_cast<common_t>(actual);
            const common_t b = static_cast<common_t>(expected);
            if constexpr (std::is_floating_point<T>::value || std::is_floating_point<U>::value) {
                if (std::isnan(a) || std::isnan(b)) {
                    return false;
                }
                // 处理Inf：Inf和自身相等
                if (std::isinf(a) && std::isinf(b)) {
                    return (a > 0) == (b > 0);
                }

                const common_t epsilon = common<T, U>::epsilon;
                return std::fabs(a - b) < epsilon;
            } else {
                return a == b;
            }
        } else {
            return actual == expected;
        }
    }

    template <class T, class U>
    static inline bool gt(T actual, U expected) {
        if constexpr (std::is_arithmetic<T>::value && std::is_arithmetic<U>::value) {
            using common_t = typename common<T, U>::type;
            const common_t a = static_cast<common_t>(actual);
            const common_t b = static_cast<common_t>(expected);

            if constexpr (std::is_floating_point<T>::value || std::is_floating_point<U>::value) {
                if (std::isnan(a) || std::isnan(b)) {
                    return false;
                }

                const common_t epsilon = common<T, U>::epsilon;
                return (a - b) > epsilon;
            } else {
                return a > b;
            }
        } else {
            return actual > expected;
        }
    }

    template <class T, class U>
    static inline bool lt(T actual, U expected) {
        if constexpr (std::is_arithmetic<T>::value && std::is_arithmetic<U>::value) {
            using common_t = typename common<T, U>::type;
            const common_t a = static_cast<common_t>(actual);
            const common_t b = static_cast<common_t>(expected);
            if constexpr (std::is_floating_point<T>::value || std::is_floating_point<U>::value) {
                if (std::isnan(a) || std::isnan(b)) {
                    return false;
                }

                const common_t epsilon = common<T, U>::epsilon;
                return (a - b) < -epsilon;
            } else {
                return a < b;
            }
        } else {
            return actual < expected;
        }
    }

    template <class T, class U>
    static inline bool ge(T actual, U expected) {
        return !lt(actual, expected);
    }

    template <class T, class U>
    static inline bool le(T actual, U expected) {
        return !gt(actual, expected);
    }
};

template <class T, class U>
static void print_failure_msg(const std::string& expr, const T& actual, const U& expected,
                              const std::string& fileName, int32_t lineNumber) {
    std::cerr << "[   ✗ FAIL   ] " << expr << " @ " << fileName << ":" << lineNumber << "\n";
    std::cerr << " Expected: " << expected << " Actual: " << actual << "\n";
    testing::test_results::instance().fail_check();
}

template <class T, class U>
static void print_success_msg(const std::string& expr, const T& actual, const U& expected,
                              const std::string& fileName, int32_t lineNumber) {
    std::cout << "[   ✓ PASS   ] " << actual << expr << expected << " @ " << fileName << ":" << lineNumber
              << "\n";
    testing::test_results::instance().pass_check();
}

template <class T>
static void expect(const T& condition, const std::string& fileName, int32_t lineNumber) {
    if (condition) {
        std::cout << "[   ✓ PASS   ] @ " << fileName << ":" << lineNumber << "\n";
        testing::test_results::instance().pass_check();
    } else {
        std::cerr << "[   ✗ FAIL   ] @ " << fileName << ":" << lineNumber << "\n";
        std::cerr << "  Condition is false\n";
        testing::test_results::instance().fail_check();
    }
}

template <class T, class U>
static void expect_eq(const T& actual, const U& expected, const std::string& fileName, int32_t lineNumber) {
    if (Compare::equal(actual, expected)) {
        print_success_msg(" == ", actual, expected, fileName, lineNumber);
    } else {
        print_failure_msg("EXPECT_EQ", actual, expected, fileName, lineNumber);
    }
}

template <class T, class U>
static void expect_ne(const T& actual, const U& expected, const std::string& fileName, int32_t lineNumber) {
    if (!Compare::equal(actual, expected)) {
        print_success_msg(" != ", actual, expected, fileName, lineNumber);
    } else {
        print_failure_msg("EXPECT_NE", actual, expected, fileName, lineNumber);
    }
}

template <class T, class U>
static void expect_le(const T& actual, const U& expected, const std::string& fileName, int32_t lineNumber) {
    if (Compare::le(actual, expected)) {
        print_success_msg(" <=", actual, expected, fileName, lineNumber);
    } else {
        print_failure_msg("EXPECT_LE", actual, expected, fileName, lineNumber);
    }
}

template <class T, class U>
static void expect_ge(const T& actual, const U& expected, const std::string& fileName, int32_t lineNumber) {
    if (Compare::ge(actual, expected)) {
        print_success_msg(" >= ", actual, expected, fileName, lineNumber);
    } else {
        print_failure_msg("EXPECT_GE", actual, expected, fileName, lineNumber);
    }
}

template <class T, class U>
static void expect_lt(const T& actual, const U& expected, const std::string& fileName, int32_t lineNumber) {
    if (Compare::lt(actual, expected)) {
        print_success_msg(" < ", actual, expected, fileName, lineNumber);
    } else {
        print_failure_msg("EXPECT_LT", actual, expected, fileName, lineNumber);
    }
}

template <class T, class U>
static void expect_gt(const T& actual, const U& expected, const std::string& fileName, int32_t lineNumber) {
    if (Compare::gt(actual, expected)) {
        print_success_msg(" > ", actual, expected, fileName, lineNumber);
    } else {
        print_failure_msg("EXPECT_GT", actual, expected, fileName, lineNumber);
    }
}

}  // namespace detail