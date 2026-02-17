#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#include "core.h"

namespace detail {
template <class T, class U>
static void print_failure_msg(const std::string &expr, const T &actual, const U &expected,
                              const std::string &fileName, int32_t lineNumber) {
    std::cerr << "[   ✗ FAIL   ] " << expr << " @ " << fileName << ":" << lineNumber << "\n";
    std::cerr << " Expected: " << expected << " Actual: " << actual << "\n";
    testing::test_results::instance().fail_check();
}

template <class T, class U>
static void print_success_msg(const std::string &expr, const T &actual, const U &expected,
                              const std::string &fileName, int32_t lineNumber) {
    std::cout << "[   ✓ PASS   ] " << actual << expr << expected << " @ " << fileName << ":" << lineNumber
              << "\n";
    testing::test_results::instance().pass_check();
}

template <class T>
static void expect(const T &condition, const std::string &fileName, int32_t lineNumber) {
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
static void expect_eq(const T &actual, const U &expected, const std::string &fileName, int32_t lineNumber) {
    if (actual != expected) {
        print_failure_msg("EXPECT_EQ", actual, expected, fileName, lineNumber);
    } else {
        print_success_msg(" == ", actual, expected, fileName, lineNumber);
    }
}

template <class T, class U>
static void expect_ne(const T &actual, const U &expected, const std::string &fileName, int32_t lineNumber) {
    if (actual == expected) {
        print_failure_msg("EXPECT_NE", actual, expected, fileName, lineNumber);
    } else {
        print_success_msg(" != ", actual, expected, fileName, lineNumber);
    }
}

template <class T, class U>
static void expect_le(const T &actual, const U &expected, const std::string &fileName, int32_t lineNumber) {
    if (actual > expected) {
        print_failure_msg("EXPECT_LE", actual, expected, fileName, lineNumber);
    } else {
        print_success_msg(" <=", actual, expected, fileName, lineNumber);
    }
}

template <class T, class U>
static void expect_ge(const T &actual, const U &expected, const std::string &fileName, int32_t lineNumber) {
    if (actual < expected) {
        print_failure_msg("EXPECT_GE", actual, expected, fileName, lineNumber);
    } else {
        print_success_msg(" >= ", actual, expected, fileName, lineNumber);
    }
}

template <class T, class U>
static void expect_lt(const T &actual, const U &expected, const std::string &fileName, int32_t lineNumber) {
    if (actual >= expected) {
        print_failure_msg("EXPECT_LT", actual, expected, fileName, lineNumber);
    } else {
        print_success_msg(" < ", actual, expected, fileName, lineNumber);
    }
}

template <class T, class U>
static void expect_gt(const T &actual, const U &expected, const std::string &fileName, int32_t lineNumber) {
    if (actual < expected) {
        print_failure_msg("EXPECT_GT", actual, expected, fileName, lineNumber);
    } else {
        print_success_msg(" > ", actual, expected, fileName, lineNumber);
    }
}

}  // namespace detail