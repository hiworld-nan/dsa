/**
 * @file date.h
 * @brief 日期工具
 * @version 1.0.0
 *
 * 提供高性能日期运算：
 * - Date: 基于 Julian Day Number 的日期类
 * - 编译期日期验证与运算
 * - std::format / std::chrono 集成
 */

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <ctime>
#include <expected>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace utils {

using namespace common;

template <typename T>
concept DateInteger = std::integral<T> && !std::same_as<T, bool>;

/// 基于 Julian Day Number 的高性能日期类
class Date final {
public:
    static constexpr std::string_view kDateFormat{"%Y-%m-%d"};
    enum class Month : int32_t { Jan = 1, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec };

    static constexpr std::array<int32_t, 13> kDaysInMonth = {0,  31, 28, 31, 30, 31, 30,
                                                             31, 31, 30, 31, 30, 31};

    static constexpr std::array<int32_t, 13> kDaysBeforeMonth = {0,   0,   31,  59,  90,  120, 151,
                                                                 181, 212, 243, 273, 304, 334};

    Date() noexcept = default;

    template <DateInteger Y, DateInteger M, DateInteger D>
    constexpr Date(Y year, M month, D day) : year_(year), month_(month), day_(day) {
        check_date_validity(year_, month_, day_);
    }

    template <DateInteger Integer>
    constexpr Date(Integer date) : Date(date / 10000, (date / 100) % 100, date % 100) {}

    /// 从当前时间构造
    void from_now(bool utc = false) noexcept {
        namespace chr = std::chrono;

        chr::year_month_day ymd{};
        if (utc) {
            const chr::system_clock::time_point now =
                chr::clock_cast<chr::system_clock>(chr::utc_clock::now());
            ymd = chr::floor<chr::days>(now);
        } else {
            ymd = chr::floor<chr::days>(chr::system_clock::now());
        }

        year_ = static_cast<int32_t>(ymd.year());
        month_ = static_cast<unsigned>(ymd.month());
        day_ = static_cast<unsigned>(ymd.day());
    }

    constexpr std::expected<void, std::string> from_string(std::string_view date_view) {
        if (date_view.size() != 10 || date_view[4] != '-' || date_view[7] != '-') {
            return std::unexpected(std::format("Invalid format: {}", date_view));
        }

        auto parse_num = [](std::string_view s) -> int32_t {
            int32_t res = 0;
            for (char c : s) {
                if (!std::isdigit(c)) {
                    return -1;
                }

                res = res * 10 + (c - '0');
            }
            return res;
        };

        const int year = parse_num(date_view.substr(0, 4));
        const int month = parse_num(date_view.substr(5, 2));
        const int day = parse_num(date_view.substr(8, 2));

        set_date(year, month, day);
        return {};
    }

    template <DateInteger Y, DateInteger M, DateInteger D>
    constexpr void set_date(Y year, M month, D day) {
        year_ = year;
        month_ = month;
        day_ = day;
        check_date_validity(year_, month_, day_);
    }

    constexpr Date& add_days(int32_t days) {
        if (days) [[likely]] {
            from_jdn(to_jdn() + days);
            check_date_validity(year_, month_, day_);
        }
        return *this;
    }

    constexpr Date& add_months(int32_t months) {
        if (months) [[likely]] {
            int32_t new_year = year_;
            int32_t new_month = month_ + months;

            if (new_month > 12) {
                new_year += (new_month - 1) / 12;
                new_month = ((new_month - 1) % 12) + 1;
            } else if (new_month < 1) {
                new_year += (new_month - 12) / 12;
                new_month = ((new_month % 12) + 12) % 12;
            }

            year_ = new_year;
            month_ = new_month;
            day_ = std::min(day_, get_days_in_month(year_, month_));
            check_date_validity(year_, month_, day_);
        }
        return *this;
    }

    constexpr Date& add_years(int32_t years) {
        if (years) [[likely]] {
            year_ += years;
            day_ = std::min(day_, get_days_in_month(year_, month_));
            check_date_validity(year_, month_, day_);
        }

        return *this;
    }

    [[nodiscard]] constexpr auto operator<=>(const Date& other) const noexcept = default;
    constexpr explicit operator int32_t() const noexcept { return year_ * 10000 + month_ * 100 + day_; }

    constexpr Date& operator++() noexcept {
        if (++day_ > get_days_in_month(year_, month_)) [[unlikely]] {
            day_ = 1;
            month_ = next_month(month_);
            if (month_ == 1) [[unlikely]] {
                ++year_;
            }

            check_date_validity(year_, month_, day_);
        }
        return *this;
    }

    constexpr Date& operator--() noexcept {
        if (--day_ < 1) [[unlikely]] {
            month_ = prev_month(month_);
            if (month_ == 12) [[unlikely]] {
                --year_;
            }

            day_ = get_days_in_month(year_, month_);
            check_date_validity(year_, month_, day_);
        }
        return *this;
    }

    constexpr Date operator++(int) noexcept {
        Date temp = *this;
        ++(*this);
        return temp;
    }

    constexpr Date operator--(int) noexcept {
        Date temp = *this;
        --(*this);
        return temp;
    }

    constexpr Date& operator+=(int32_t days) noexcept { return add_days(days); }
    constexpr Date& operator-=(int32_t days) noexcept { return add_days(-days); }

    constexpr Date operator+(int32_t days) const noexcept {
        Date temp = *this;
        return temp.add_days(days);
    }

    constexpr Date operator-(int32_t days) const noexcept {
        Date temp = *this;
        return temp.add_days(-days);
    }

    [[nodiscard]] constexpr int64_t operator-(const Date& other) const noexcept {
        return to_jdn() - other.to_jdn();
    }

    [[nodiscard]] constexpr int32_t year() const noexcept { return year_; }
    [[nodiscard]] constexpr int32_t month() const noexcept { return month_; }
    [[nodiscard]] constexpr int32_t day() const noexcept { return day_; }

    /// 星期几（0=周日, 1=周一, ..., 6=周六）Tomohiko Sakamoto 算法
    [[nodiscard]] constexpr int32_t day_of_week() const noexcept {
        static constexpr std::array<int32_t, 12> offset_table = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        int32_t y = year_;
        if (month_ < 3) [[unlikely]] {
            y -= 1;
        }

        return ((y + y / 4 - y / 100 + y / 400 + offset_table[month_ - 1] + day_) % 7 + 7) % 7;
    }

    /// 一年中的第几天
    [[nodiscard]] constexpr int32_t day_of_year() const noexcept {
        int32_t days = kDaysBeforeMonth[month_] + day_;
        if (month_ > 2) [[likely]] {
            days += is_leap_year(year_);
        }

        return days;
    }

    /// 转为字符串 "YYYYMMDD"
    [[nodiscard]] std::string to_string() const {
        return std::format("{:04d}{:02d}{:02d}", year_, month_, day_);
    }

    friend std::ostream& operator<<(std::ostream& os, const Date& date) {
        os << date.to_string();
        return os;
    }

    [[nodiscard]] static constexpr bool is_leap_year(int32_t year) noexcept {
        return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    }

    [[nodiscard]] static constexpr bool is_valid_date(int32_t year, int32_t month, int32_t day) noexcept {
        return year >= 1 && is_valid_month(month) && is_valid_day(year, month, day);
    }

private:
    int32_t year_{1};
    int32_t month_{1};
    int32_t day_{1};

    static constexpr void check_date_validity(int32_t year, int32_t month, int32_t day) {
        if (!is_valid_date(year, month, day)) {
            throw std::invalid_argument(std::format("Invalid date: {:04d}-{:02d}-{:02d}", year, month, day));
        }
    }

    [[nodiscard]] static constexpr bool is_valid_month(int32_t month) noexcept {
        return month >= 1 && month <= 12;
    }

    [[nodiscard]] static constexpr bool is_valid_day(int32_t year, int32_t month, int32_t day) noexcept {
        return day >= 1 && day <= get_days_in_month(year, month);
    }

    [[nodiscard]] static constexpr int32_t get_days_in_month(int32_t year, int32_t month) noexcept {
        if (month != 2) [[likely]] {
            return kDaysInMonth[month];
        }

        return 28 + is_leap_year(year);
    }

    [[nodiscard]] static constexpr int32_t next_month(uint32_t month) noexcept { return (month % 12) + 1; }
    [[nodiscard]] static constexpr int32_t prev_month(uint32_t month) noexcept {
        return (month + 10) % 12 + 1;
    }

    /// Julian Day Number (JDN) Conversion
    /// Valid date range: 4713 BCE (year -4713) to 3267 CE (year 3267)
    /// Formula reference: https://en.wikipedia.org/wiki/Julian_day
    /// Core logic: Convert Gregorian calendar date (year/month/day) to a continuous Julian Day count
    [[nodiscard]] constexpr int64_t to_jdn() const noexcept {
        constexpr int64_t JDN_MONTH_COEFFICIENT = 153;
        constexpr int64_t JDN_MONTH_OFFSET = 2;
        constexpr int64_t JDN_YEAR_DAYS = 365;
        constexpr int64_t JDN_BASE_OFFSET = -32045;

        const int32_t a = static_cast<int64_t>((14 - month_) / 12);
        const int64_t y = static_cast<int64_t>(year_ + 4800 - a);
        const int64_t m = static_cast<int64_t>(month_ + 12 * a - 3);
        const int64_t d = static_cast<int64_t>(day_);

        return d + (JDN_MONTH_COEFFICIENT * m + JDN_MONTH_OFFSET) / 5 + (JDN_YEAR_DAYS * y) + y / 4 -
               y / 100 + y / 400 + JDN_BASE_OFFSET;
    }

    /// convert Julian Day Number to date
    /// range of Julian Day: -4713-01-01 to 3267-01-01
    /// convert Date to Julian Day
    /// https://en.wikipedia.org/wiki/Julian_day
    constexpr void from_jdn(int64_t jd) noexcept {
        constexpr int64_t JULIAN_EPOCH = 32044;
        constexpr int64_t DAYS_PER_400_YEARS = 146097;
        constexpr int64_t DAYS_PER_4_YEARS = 1461;
        constexpr int64_t DAYS_PER_5_MONTHS = 153;

        const int64_t a = jd + JULIAN_EPOCH;
        const int64_t b = (4 * a + 3) / DAYS_PER_400_YEARS;
        const int64_t c = a - (DAYS_PER_400_YEARS * b) / 4;
        const int64_t d = (4 * c + 3) / DAYS_PER_4_YEARS;
        const int64_t e = c - (DAYS_PER_4_YEARS * d) / 4;
        const int64_t m = (5 * e + 2) / DAYS_PER_5_MONTHS;

        day_ = static_cast<int32_t>(e - (DAYS_PER_5_MONTHS * m + 2) / 5 + 1);
        month_ = static_cast<int32_t>(m + 3 - 12 * (m / 10));
        year_ = static_cast<int32_t>(100 * b + d - 4800 + m / 10);
    }
};

}  // namespace utils

namespace std {
template <>
struct formatter<utils::Date> {
    template <class ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <class FmtContext>
    FmtContext::iterator format(const utils::Date& date, FmtContext& ctx) const {
        return std::format_to(ctx.out(), "{}", static_cast<int32_t>(date));
    }
};
}  // namespace std
