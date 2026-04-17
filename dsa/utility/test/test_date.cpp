/**
 * @file test_date.cpp
 * @brief 日期工具单元测试
 * @version 1.0.0
 */

#include <string>

#include "../../test/test.h"
#include "../utility.h"

using namespace utils;

// =============================================================================
// 构造与验证
// =============================================================================

TEST(Date, ConstructBasic) {
    Date d(2024, 1, 15);
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 15);
    return true;
}

TEST(Date, LeapDay) {
    Date d(2024, 2, 29);
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 2);
    EXPECT_EQ(d.day(), 29);
    return true;
}

TEST(Date, IsLeapYear) {
    CHECK_COMPILE_TIME(Date::is_leap_year(2024));
    CHECK_COMPILE_TIME(Date::is_leap_year(2000));
    CHECK_COMPILE_TIME(!Date::is_leap_year(2023));
    CHECK_COMPILE_TIME(!Date::is_leap_year(1900));
    return true;
}

TEST(Date, IsValidDate) {
    CHECK_COMPILE_TIME(Date::is_valid_date(2024, 1, 1));
    CHECK_COMPILE_TIME(Date::is_valid_date(2024, 2, 29));
    CHECK_COMPILE_TIME(!Date::is_valid_date(2023, 2, 29));
    CHECK_COMPILE_TIME(!Date::is_valid_date(2024, 13, 1));
    CHECK_COMPILE_TIME(!Date::is_valid_date(2024, 0, 1));
    return true;
}

TEST(Date, InvalidDateThrows) {
    bool caught = false;
    try {
        Date d(2023, 2, 29);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    EXPECT_TRUE(caught);
    return true;
}

// =============================================================================
// 比较运算
// =============================================================================

TEST(Date, Comparison) {
    Date a(2024, 1, 1);
    Date b(2024, 1, 2);
    Date c(2024, 1, 1);

    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(a != b);
    return true;
}

// =============================================================================
// 日期加减
// =============================================================================

TEST(Date, AddDays) {
    Date d(2024, 1, 1);
    d.add_days(100);
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 4);
    EXPECT_EQ(d.day(), 10);
    return true;
}

TEST(Date, SubtractDays) {
    Date d(2024, 4, 10);
    d.add_days(-100);
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
    return true;
}

TEST(Date, AddMonths) {
    Date d(2024, 1, 15);
    d.add_months(3);
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 4);
    EXPECT_EQ(d.day(), 15);
    return true;
}

TEST(Date, AddMonthsOverflow) {
    Date d(2024, 11, 15);
    d.add_months(3);
    EXPECT_EQ(d.year(), 2025);
    EXPECT_EQ(d.month(), 2);
    EXPECT_EQ(d.day(), 15);
    return true;
}

TEST(Date, AddYears) {
    Date d(2024, 3, 15);
    d.add_years(2);
    EXPECT_EQ(d.year(), 2026);
    EXPECT_EQ(d.month(), 3);
    EXPECT_EQ(d.day(), 15);
    return true;
}

TEST(Date, DateDiff) {
    Date a(2024, 1, 1);
    Date b(2024, 1, 11);
    EXPECT_EQ(b - a, static_cast<int64_t>(10));
    EXPECT_EQ(a - b, static_cast<int64_t>(-10));
    return true;
}

TEST(Date, OperatorPlus) {
    Date d(2024, 1, 1);
    auto d2 = d + 30;
    EXPECT_EQ(d2.year(), 2024);
    EXPECT_EQ(d2.month(), 1);
    EXPECT_EQ(d2.day(), 31);
    return true;
}

TEST(Date, OperatorMinus) {
    Date d(2024, 1, 31);
    auto d2 = d - 30;
    EXPECT_EQ(d2.year(), 2024);
    EXPECT_EQ(d2.month(), 1);
    EXPECT_EQ(d2.day(), 1);
    return true;
}

// =============================================================================
// 自增/自减
// =============================================================================

TEST(Date, Increment) {
    Date d(2024, 1, 31);
    ++d;
    EXPECT_EQ(d.month(), 2);
    EXPECT_EQ(d.day(), 1);
    return true;
}

TEST(Date, Decrement) {
    Date d(2024, 2, 1);
    --d;
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 31);
    return true;
}

TEST(Date, IncrementYearBoundary) {
    Date d(2024, 12, 31);
    ++d;
    EXPECT_EQ(d.year(), 2025);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
    return true;
}

TEST(Date, DecrementYearBoundary) {
    Date d(2025, 1, 1);
    --d;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 12);
    EXPECT_EQ(d.day(), 31);
    return true;
}

TEST(Date, PostIncrement) {
    Date d(2024, 1, 1);
    auto old = d++;
    EXPECT_EQ(old.day(), 1);
    EXPECT_EQ(d.day(), 2);
    return true;
}

TEST(Date, PostDecrement) {
    Date d(2024, 1, 2);
    auto old = d--;
    EXPECT_EQ(old.day(), 2);
    EXPECT_EQ(d.day(), 1);
    return true;
}

// =============================================================================
// day_of_week / day_of_year
// =============================================================================

TEST(Date, DayOfWeek) {
    // 2024-02-29 是周四
    CHECK_COMPILE_TIME(Date(2024, 2, 29).day_of_week() == 4);
    // 2024-12-30 是周一
    CHECK_COMPILE_TIME(Date(2024, 12, 30).day_of_week() == 1);
    // 2025-01-01 是周三
    CHECK_COMPILE_TIME(Date(2025, 1, 1).day_of_week() == 3);
    return true;
}

TEST(Date, DayOfYear) {
    Date d(2024, 3, 1);
    // 2024是闰年: 31(Jan) + 29(Feb) + 1 = 61
    EXPECT_EQ(d.day_of_year(), 61);
    return true;
}

TEST(Date, DayOfYearNonLeap) {
    Date d(2023, 3, 1);
    // 2023非闰年: 31(Jan) + 28(Feb) + 1 = 60
    EXPECT_EQ(d.day_of_year(), 60);
    return true;
}

// =============================================================================
// 序列化
// =============================================================================

TEST(Date, ToString) {
    Date d(2024, 3, 15);
    EXPECT_EQ(d.to_string(), std::string("20240315"));
    return true;
}

TEST(Date, FromString) {
    Date d;
    auto result = d.from_string("2023-10-05");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(d.year(), 2023);
    EXPECT_EQ(d.month(), 10);
    EXPECT_EQ(d.day(), 5);
    return true;
}

TEST(Date, FromStringInvalid) {
    Date d;
    auto result = d.from_string("invalid");
    EXPECT_FALSE(result.has_value());
    return true;
}

TEST(Date, IntConversion) {
    Date d(2024, 3, 15);
    EXPECT_EQ(static_cast<int32_t>(d), 20240315);
    return true;
}

TEST(Date, FormatIntegration) {
    Date d(2024, 3, 15);
    std::string s = std::format("{}", d);
    EXPECT_EQ(s, std::string("20240315"));
    return true;
}

// =============================================================================
// JDN 往返测试
// =============================================================================

TEST(Date, JdnRoundTrip) {
    Date original(2024, 6, 15);
    Date reconstructed = original + 0;
    EXPECT_EQ(original, reconstructed);
    return true;
}

TEST(Date, LargeRangeRoundTrip) {
    Date d(1, 1, 1);
    Date d2 = d + 0;
    EXPECT_EQ(d, d2);
    return true;
}

// =============================================================================
// from_now
// =============================================================================

TEST(Date, FromNow) {
    Date d;
    d.from_now();
    EXPECT_GE(d.year(), 2024);
    return true;
}

// =============================================================================
// 主函数
// =============================================================================

int main() { return testing::run_all_tests(); }
