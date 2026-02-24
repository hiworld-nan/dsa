#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "common.h"

namespace utils {

template <std::size_t N>
struct CompileTimeString {
    static_assert(N > 0, "CompileTimeString must have at least one character");
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    using SelfT = CompileTimeString<N>;

    using value_type = char;
    using const_value_type = const value_type;

    using pointer = value_type*;
    using const_pointer = const_value_type*;

    using reference = value_type&;
    using const_reference = const_value_type&;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using iterator = pointer;
    using const_iterator = const_pointer;

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    std::array<value_type, N> data_{0};
    std::size_t length_ = 0;

    template <std::size_t M>
    struct rebind {
        static_assert(M != N, "Rebind to same size is not allowed");
        using type = CompileTimeString<M>;
    };

    template <std::size_t M = N>
    struct splitResult {
        std::array<SelfT, M> result_{};
        std::size_t size_ = 0;

        constexpr std::size_t size() const noexcept { return size_; }
        constexpr void push_back(const SelfT& str) { result_[size_++] = str; }

        constexpr auto& operator[](std::size_t i) noexcept { return result_[i]; }
        constexpr const auto& operator[](std::size_t i) const noexcept { return result_[i]; }

        constexpr auto begin() const noexcept { return result_.begin(); }
        constexpr auto end() const noexcept { return result_.begin() + size_; }
        constexpr auto cbegin() const noexcept { return result_.cbegin(); }
        constexpr auto cend() const noexcept { return result_.cbegin() + size_; }
    };

    constexpr CompileTimeString() = default;
    constexpr CompileTimeString(std::string_view sv) { assign(sv); }

    constexpr CompileTimeString(std::initializer_list<value_type> list)
        : CompileTimeString(std::string_view(list.begin(), std::min(N, list.size()))) {}

    constexpr CompileTimeString(const value_type (&str)[N]) noexcept
        : CompileTimeString(std::string_view(str, N - 1)) {}

    constexpr auto& operator=(std::string_view sv) {
        assign(sv);
        return *this;
    }

    constexpr auto& operator=(std::initializer_list<value_type> list) {
        return *this = std::string_view(list.begin(), std::min(N, list.size()));
    }

    template <std::size_t M>
    constexpr auto& operator=(const value_type (&str)[M]) noexcept {
        return *this = std::string_view(str, std::min(M - 1, N));
    }

    constexpr void swap(CompileTimeString&& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(length_, other.length_);
    }

    constexpr value_type& operator[](std::size_t i) noexcept { return data_[i]; }
    constexpr const value_type& operator[](std::size_t i) const noexcept { return data_[i]; }

    constexpr void clear() noexcept {
        length_ = 0;
        data_[0] = '\0';
    }
    constexpr std::size_t size() const noexcept { return N; }
    constexpr std::size_t capacity() const noexcept { return N; }
    constexpr bool empty() const noexcept { return length_ == 0; }
    constexpr std::size_t length() const noexcept { return length_; }

    constexpr const value_type& at(std::size_t pos) const {
        if (!std::is_constant_evaluated() && pos >= length_) {
            throw std::out_of_range("CompileTimeString::at");
        }
        return data_[pos];
    }
    constexpr const value_type& front() const noexcept { return data_[0]; }
    constexpr const value_type& back() const noexcept { return length_ > 0 ? data_[length_ - 1] : data_[0]; }
    constexpr const value_type* data() const noexcept { return data_.data(); }

    constexpr operator const value_type*() const noexcept { return data(); }
    constexpr operator std::string_view() const noexcept { return std::string_view(data(), length_); }

    constexpr auto begin() const noexcept { return data(); }
    constexpr auto end() const noexcept { return data() + length_; }
    constexpr auto cbegin() const noexcept { return data(); }
    constexpr auto cend() const noexcept { return data() + length_; }

    constexpr auto rbegin() const noexcept { return const_reverse_iterator(end()); }
    constexpr auto rend() const noexcept { return const_reverse_iterator(begin()); }
    constexpr auto crbegin() const noexcept { return const_reverse_iterator(cend()); }
    constexpr auto crend() const noexcept { return const_reverse_iterator(cbegin()); }

    template <std::size_t M>
    constexpr bool operator==(const CompileTimeString<M>& other) const noexcept {
        return std::string_view(data(), length_) == std::string_view(other.data(), other.length());
    }
    template <std::size_t M>
    constexpr bool operator!=(const CompileTimeString<M>& other) const noexcept {
        return !(*this == other);
    }
    template <std::size_t M>
    constexpr bool operator>=(const CompileTimeString<M>& other) const noexcept {
        return std::string_view(data(), length_) >= std::string_view(other.data(), other.length());
    }
    template <std::size_t M>
    constexpr bool operator<=(const CompileTimeString<M>& other) const noexcept {
        return std::string_view(data(), length_) <= std::string_view(other.data(), other.length());
    }
    template <std::size_t M>
    constexpr bool operator>(const CompileTimeString<M>& other) const noexcept {
        return std::string_view(data(), length_) > std::string_view(other.data(), other.length());
    }
    template <std::size_t M>
    constexpr bool operator<(const CompileTimeString<M>& other) const noexcept {
        return std::string_view(data(), length_) < std::string_view(other.data(), other.length());
    }
    template <std::size_t M>
    constexpr bool operator==(const value_type (&str)[M]) const noexcept {
        return std::string_view(data(), length_) == std::string_view(str, M - 1);
    }
    template <std::size_t M>
    constexpr bool operator!=(const value_type (&str)[M]) const noexcept {
        return std::string_view(data(), length_) != std::string_view(str, M - 1);
    }
    template <std::size_t M>
    constexpr bool operator>=(const value_type (&str)[M]) const noexcept {
        return std::string_view(data(), length_) >= std::string_view(str, M - 1);
    }
    template <std::size_t M>
    constexpr bool operator<=(const value_type (&str)[M]) const noexcept {
        return std::string_view(data(), length_) <= std::string_view(str, M - 1);
    }
    template <std::size_t M>
    constexpr bool operator>(const value_type (&str)[M]) const noexcept {
        return std::string_view(data(), length_) > std::string_view(str, M - 1);
    }
    template <std::size_t M>
    constexpr bool operator<(const value_type (&str)[M]) const noexcept {
        return std::string_view(data(), length_) < std::string_view(str, M - 1);
    }

    template <std::size_t M>
    constexpr auto operator+(const CompileTimeString<M>& other) const noexcept {
        std::size_t i = 0;
        CompileTimeString<N + M> result;
        for (; i < length_; i++) {
            result[i] = data_[i];
        }
        for (std::size_t j = 0; j < other.length(); ++i, j++) {
            result[i] = other[j];
        }
        result.length_ = length_ + other.length();
        return result;
    }

    // string literal concatenation
    template <std::size_t M>
    constexpr auto operator+(const value_type (&str)[M]) const noexcept {
        return *this + CompileTimeString<M - 1>(std::string_view(str, M - 1));
    }

    constexpr bool starts_with(value_type ch) const noexcept {
        return std::string_view(data(), length_).starts_with(ch);
    }

    constexpr bool starts_with(const value_type* s) const {
        return std::string_view(data(), length_).starts_with(s);
    }

    constexpr bool ends_with(value_type ch) const noexcept {
        return std::string_view(data(), length_).ends_with(ch);
    }

    constexpr bool ends_with(const value_type* s) const {
        return std::string_view(data(), length_).ends_with(s);
    }

    template <std::size_t Pos = 0, std::size_t Count = SelfT::npos>
    constexpr auto substr() const {
        constexpr std::size_t Len = Count == npos ? N - Pos : Count;
        static_assert(Pos + Len <= N, "Out of range");

        std::size_t i = Pos;
        CompileTimeString<Len> result;

        for (std::size_t j = 0; j < Len && i < length_; i++, j++) {
            result[j] = data_[i];
        }
        result.length_ = i - Pos;

        return result;
    }

    constexpr auto substr(std::size_t pos, std::size_t count = npos) const {
        return CompileTimeString(std::string_view(data(), length_).substr(pos, count));
    }

    [[nodiscard]] constexpr auto trim() const noexcept { return ltrim().rtrim(); }

    [[nodiscard]] constexpr auto ltrim() const noexcept {
        size_type pos = 0;
        while (pos < length_ && utils::compileTimeIsspace(data_[pos])) {
            ++pos;
        }
        return substr(pos);
    }

    [[nodiscard]] constexpr auto rtrim() const noexcept {
        size_type pos = length_;
        while (pos > 0 && utils::compileTimeIsspace(data_[pos - 1])) {
            --pos;
        }
        return substr(0, pos);
    }

    [[nodiscard]] constexpr auto trim(value_type ch) const noexcept { return ltrim(ch).rtrim(ch); }

    [[nodiscard]] constexpr auto ltrim(value_type ch) const noexcept {
        size_type pos = 0;
        while (pos < length_ && data_[pos] == ch) {
            ++pos;
        }
        return substr(pos);
    }

    [[nodiscard]] constexpr auto rtrim(value_type ch) const noexcept {
        size_type pos = length_;
        while (pos > 0 && data_[pos - 1] == ch) {
            --pos;
        }
        return substr(0, pos);
    }

    constexpr auto split_result(value_type delimiter, std::size_t pos = 0) const noexcept {
        size_type start = pos;
        size_type end = 0;

        splitResult result;

        while ((end = find(delimiter, start)) != npos) {
            result.push_back(std::string_view(data() + start, end - start));
            start = end + 1;
        }

        if (start < length_) {
            result.push_back(std::string_view(data() + start, length_ - start));
        }

        return result;
    }

    constexpr auto split_result(const value_type* delimiters, std::size_t pos = 0) const noexcept {
        size_type start = pos;
        size_type end = 0;

        splitResult result;

        while ((end = find_first_of(delimiters, start)) != npos) {
            result.push_back(std::string_view(data() + start, end - start));
            start = end + 1;
        }

        if (start < length_) {
            result.push_back(std::string_view(data() + start, length_ - start));
        }

        return result;
    }

    // gcc 14.2 does not support constexpr std::views::split
    constexpr auto split(value_type delimiter, std::size_t pos = 0) const noexcept {
        return std::string_view(data() + pos, length_ - pos) | std::views::split(delimiter);
    }
    // gcc 14.2 does not support constexpr std::views::split
    constexpr auto split(const value_type* delimiter, std::size_t pos = 0) const noexcept {
        return std::string_view(data() + pos, length_ - pos) | std::views::split(std::string_view(delimiter));
    }

    constexpr std::size_t find_first_of(value_type ch, size_t pos = 0) const noexcept {
        return std::string_view(data(), length_).find_first_of(ch, pos);
    }

    constexpr std::size_t find_first_of(const value_type* s, size_t pos = 0) const noexcept {
        return std::string_view(data(), length_).find_first_of(s, pos);
    }

    constexpr std::size_t find_first_not_of(value_type ch, size_t pos = 0) const noexcept {
        return std::string_view(data(), length_).find_first_not_of(ch, pos);
    }

    constexpr std::size_t find_first_not_of(const value_type* s, size_t pos = 0) const noexcept {
        return std::string_view(data(), length_).find_first_not_of(s, pos);
    }

    constexpr std::size_t find_last_of(value_type ch, size_t pos = npos) const noexcept {
        return std::string_view(data(), length_).find_last_of(ch, pos);
    }

    constexpr std::size_t find_last_of(const value_type* s, size_t pos = npos) const noexcept {
        return std::string_view(data(), length_).find_last_of(s, pos);
    }

    constexpr std::size_t find_last_not_of(value_type ch, size_t pos = npos) const noexcept {
        return std::string_view(data(), length_).find_last_not_of(ch, pos);
    }

    constexpr std::size_t find_last_not_of(const value_type* s, size_t pos = npos) const noexcept {
        return std::string_view(data(), length_).find_last_not_of(s, pos);
    }

    constexpr std::size_t find(value_type ch, std::size_t pos = 0) const noexcept {
        return std::string_view(data(), length_).find(ch, pos);
    }

    constexpr std::size_t find(const value_type* s, std::size_t pos = 0) const noexcept {
        if (s == nullptr) {
            return npos;
        }
        return std::string_view(data(), length_).find(s, pos);
    }

    constexpr std::size_t rfind(value_type ch, std::size_t pos = npos) const noexcept {
        return std::string_view(data(), length_).rfind(ch, pos);
    }

    constexpr std::size_t rfind(const value_type* s, std::size_t pos = npos) const noexcept {
        if (s == nullptr) {
            return npos;
        }
        return std::string_view(data(), length_).rfind(s, pos);
    }

    constexpr bool contains(value_type ch) const noexcept {
        return std::string_view(data(), length_).find(ch) != npos;
    }

    template <AnyString T>
    constexpr bool contains(const T& s) const noexcept {
        return std::string_view(data(), length_).find(std::string_view(s)) != npos;
    }

    template <std::size_t M>
    constexpr bool contains(const value_type (&str)[M]) const noexcept {
        return std::string_view(data(), length_).find(str, 0, M - 1) != npos;
    }

    template <AnyString T>
    constexpr int32_t compare(const T& s) const noexcept {
        return std::string_view(data(), length_).compare(std::string_view(s));
    }

    template <std::size_t M>
    constexpr int32_t compare(const CompileTimeString<M>& other) const {
        return std::string_view(data(), length_).compare(std::string_view(other.data(), other.length()));
    }

    template <std::size_t M>
    constexpr int32_t compare(const value_type (&s)[M]) const {
        return std::string_view(data(), length_).compare(std::string_view(s, M - 1));
    }

    constexpr auto remove_prefix(std::size_t n) const noexcept {
        if (n >= length_) {
            return SelfT();
        }
        return substr(n);
    }
    constexpr auto remove_suffix(std::size_t n) const noexcept {
        if (n >= length_) {
            return SelfT();
        }
        return substr(0, length_ - n);
    }

    constexpr auto replace(value_type cOld, value_type cNew) const noexcept {
        auto result = *this;
        for (std::size_t i = 0; i < length_; ++i) {
            auto& c = result[i];
            if (c == cOld) {
                c = cNew;
            }
        }
        return result;
    }

    constexpr auto to_upper() const noexcept {
        auto result = *this;
        for (std::size_t i = 0; i < length_; ++i) {
            auto& c = result[i];
            if (c >= 'a' && c <= 'z') {
                c = static_cast<value_type>(c - 'a' + 'A');
            }
        }
        return result;
    }

    constexpr auto to_lower() const noexcept {
        auto result = *this;
        for (std::size_t i = 0; i < length_; ++i) {
            auto& c = result[i];
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<value_type>(c - 'A' + 'a');
            }
        }
        return result;
    }

    // fnv-1a hash algorithm
    // ??? ryuhash or wyhash
    constexpr std::size_t hash() const noexcept {
        std::size_t result = 14695981039346656037ULL;
        for (std::size_t i = 0; i < length_; ++i) {
            result ^= std::char_traits<char>::to_int_type(data_[i]);
            result *= 1099511628211ULL;
        }
        return result;
    }

    std::string to_string() const { return std::string(data(), length_); }
    friend std::ostream& operator<<(std::ostream& os, const CompileTimeString& str) {
        return os << std::string_view(str.data(), str.length_);
    }

private:
    constexpr void assign(const std::string_view& sv) {
        const std::size_t size = std::min(sv.size(), N);
        if (!std::is_constant_evaluated() && sv.size() > N) {
            throw std::length_error("CompileTimeString: sv is too long");
        }

        std::copy_n(sv.begin(), size, data_.begin());
        length_ = size;
    }
};

template <CompileTimeString Fmt, class... Args>
[[nodiscard]] static std::string Format(Args&&... args) {
    constexpr std::format_string<Args...> fmt_check(Fmt.data());
    (void)fmt_check;

    std::string result;
    result.reserve(Fmt.size() + sizeof...(args) * 16);
    // auto [it, count] = std::format_to_n(std::back_inserter(result),  // 输出迭代器（字符数组起始）
    std::format_to_n(std::back_inserter(result),  // 输出迭代器（字符数组起始）
                     result.capacity(),           // 最大写入长度
                     Fmt.data(),                  // 动态格式字符串
                     std::forward<Args>(args)...);
    result.shrink_to_fit();
    return result;
}

template <class CharT = char, CharT... chars>
consteval auto operator""_cs() {
    return CompileTimeString<sizeof...(chars)>({chars...});
}
/*template <class CharT = char, CharT... chars>
struct LiteralStringType {
    static constexpr std::size_t kSize = sizeof...(Chars);
    static constexpr char value[] = {Chars...};

    constexpr operator auto() const { return CompileTimeString<kSize>(value); }
};*/
}  // namespace utils