#pragma once

#include <string_view>
#include <type_traits>

// #include "concepts.h"

#include "core.h"

namespace detail {

template <class T>
struct identity {
    using type = T;
};

template <class T>
using identity_t = typename identity<T>::type;

template <size_t N>
struct fixed_string_base {
    char data[N] = {};
    std::size_t size = N;

    constexpr std::size_t length() const noexcept { return N - 1; }
    constexpr fixed_string_base(const char (&str)[N]) {
        for (size_t i = 0; i < N; ++i) {
            data[i] = str[i];
        }
    }

    constexpr operator std::string_view() const noexcept { return {data, N - 1}; }
};

template <size_t N>
struct fixed_string : fixed_string_base<N> {
    using fixed_string_base<N>::fixed_string_base;
};

template <size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

template <class Name, class Type, std::size_t Offset = 0>
struct field_descriptor {
    using type = Type;

    static constexpr std::size_t size = sizeof(type);
    static constexpr std::size_t offset = Offset;
    static constexpr fixed_string<Name::size> name = Name::value;
};

template <class Container>
struct field_descriptors;

template <class... Fs>
struct field_descriptors<tl::type_list<Fs...>> {
    using fields_list = tl::type_list<Fs...>;
    static constexpr std::size_t size = sizeof...(Fs);
    static constexpr std::size_t total_size = (0 + ... + sizeof(Fs::type));
};

template <class T>
struct is_same {
    template <class U>
    using impl = std::is_same<T, U>;

    template <class U>
    static constexpr bool value = impl<U>::value;
};

template <class>
struct is_tuple : std::false_type {};

template <class... Us>
struct is_tuple<std::tuple<Us...>> : std::true_type {};

template <class T>
static constexpr bool is_tuple_v = is_tuple<T>::value;

template <class>
struct is_type_list : std::false_type {};

template <class... Us>
struct is_type_list<tl::type_list<Us...>> : std::true_type {};

template <class T>
static constexpr bool is_type_list_v = is_type_list<T>::value;

template <class>
struct is_variant : std::false_type {};

template <class... Us>
struct is_variant<std::variant<Us...>> : std::true_type {};

template <class T>
static constexpr bool is_variant_v = is_variant<T>::value;

template <class T, template <class...> class Tc>
struct is_instance_of : std::false_type {};

template <template <class...> class Tc, class... Ts>
struct is_instance_of<Tc<Ts...>, Tc> : std::true_type {};

template <class T, template <class...> class Tc>
inline constexpr bool is_instance_of_v = is_instance_of<T, Tc>::value;

/*template <class T>
static constexpr bool is_variant_v = is_instance_of_v<T, std::variant>;
template <class T>
static constexpr bool is_tuple_v = is_instance_of_v<T, std::tuple>;
template <class T>
static constexpr bool is_type_list_v = is_instance_of_v<T, tl::type_list>;*/

template <std::size_t N>
using const_value = std::integral_constant<std::size_t, N>;

template <std::size_t N>
using const_value_t = typename const_value<N>::type;

template <std::size_t N>
static constexpr std::size_t const_value_v = const_value<N>::value;

// template <class List>
// concept type_list_like = is_type_list_v<List>;

// template <class Tuple>
// concept tuple_like = is_tuple_v<Tuple>;

// template <class Variant>
// concept variant_like = is_variant_v<Variant>;
}  // namespace detail