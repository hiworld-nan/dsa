#pragma once

#include <bit>
#include <concepts>
#include <limits>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace utils {

template <typename T>
concept IntegerType = std::is_integral_v<T> && !std::is_same_v<T, bool>;

template <typename T>
concept UIntegerType = std::is_integral_v<T> && std::is_unsigned_v<T>;

template <typename T>
concept FloatType = std::is_floating_point_v<T>;

template <typename T>
concept NumberType = IntegerType<T> || FloatType<T>;

template <class T>
concept AnyString = std::convertible_to<T, std::string_view>;

template <class T>
concept StringViewLike = std::is_same_v<std::decay_t<T>, std::string_view>;

template <class T>
concept CharArray = std::is_same_v<T, char[]> || std::is_same_v<T, const char[]>;

template <class T>
concept CharPointer =
    std::is_pointer_v<T> && std::is_same_v<std::remove_const_t<std::remove_pointer_t<T>>, char>;

template <typename T>
concept StringLike = requires(const T& s) {
    { s } -> std::convertible_to<std::string_view>;
};

template <class T>
concept LiteralType = std::is_trivially_destructible_v<T> && (std::is_scalar_v<T> || std::is_array_v<T> ||
                                                              (std::is_class_v<T> && std::is_aggregate_v<T>));

template <CharPointer Pointer>
[[gnu::always_inline]]
static inline constexpr std::size_t compileTimeStrlen(const Pointer ptr) {
    if (nullptr == ptr) {
        return 0;
    }

    std::size_t size = 0;
    if constexpr (requires { __builtin_strlen(""); }) {
        size = __builtin_strlen(ptr);
    } else {
        Pointer start = ptr;
        while (*start++) {
            ++size;
        }
    }
    return size;
}

[[gnu::always_inline]]
static inline constexpr bool compileTimeIsspace(int32_t c) noexcept {
    if (c == EOF) {
        return false;
    }

    const uint8_t uc = static_cast<uint8_t>(c);

    return (uc == 0x20)      // 空格
           || (uc == 0x09)   // \t 水平制表符
           || (uc == 0x0A)   // \n 换行符
           || (uc == 0x0B)   // \v 垂直制表符
           || (uc == 0x0C)   // \f 换页符
           || (uc == 0x0D);  // \r 回车符
}

[[gnu::always_inline]] static inline constexpr bool compileTimeIsspace(char c) noexcept {
    return compileTimeIsspace(std::char_traits<char>::to_int_type(c));
}

template <IntegerType T>
[[gnu::always_inline]] static inline constexpr T clamp(T value, T min, T max) {
    return value < min ? min : value > max ? max : value;
}

template <IntegerType T>
[[gnu::always_inline]] static inline constexpr T clamp_to_range(T value) {
    return clamp(value, std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
}

template <UIntegerType T>
[[gnu::always_inline]] static inline constexpr T is_power_of_two(T value) {
    return std::has_single_bit(value);
}

}  // namespace utils
