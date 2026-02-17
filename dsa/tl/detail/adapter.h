#pragma once

#include <tuple>
#include <variant>

#include "core.h"
#include "utilities.h"

namespace detail {
template <class T>
struct container_traits {
    using type = T;
    static constexpr bool is_container = false;
};

template <class... Ts>
struct container_traits<tl::type_list<Ts...>> {
    using type = tl::type_list<Ts...>;
    static constexpr bool is_container = true;
    static constexpr size_t size = sizeof...(Ts);

    template <size_t I>
    using at = std::tuple_element_t<I, std::tuple<Ts...>>;
};

template <class... Ts>
struct container_traits<std::tuple<Ts...>> {
    using type = std::tuple<Ts...>;
    static constexpr bool is_container = true;
    static constexpr size_t size = sizeof...(Ts);

    template <size_t I>
    using at = std::tuple_element_t<I, type>;
};

template <class... Ts>
struct container_traits<std::variant<Ts...>> {
    using type = std::variant<Ts...>;
    static constexpr bool is_container = true;
    static constexpr size_t size = sizeof...(Ts);

    template <size_t I>
    using at = std::tuple_element_t<I, std::tuple<Ts...>>;
};

template <class, class>
struct convert_to;

template <template <class...> class From, class... Ts, template <class...> class To, class... Us>
struct convert_to<From<Ts...>, To<Us...>> {
    using type = To<Ts...>;
};

template <class Container>
using to_tuple_t = typename convert_to<Container, std::tuple<>>::type;

template <class Container>
using to_variant_t = typename convert_to<Container, std::variant<>>::type;

template <class Container>
using to_type_list_t = typename convert_to<Container, tl::type_list<>>::type;

template <class Fn>
struct from_function {
private:
    static_assert(std::is_function_v<std::remove_pointer_t<std::decay_t<Fn>>> ||
                      std::is_member_function_pointer_v<std::decay_t<Fn>> ||
                      std::is_class_v<std::decay_t<Fn>> || std::is_invocable_v<Fn>,
                  "from_function: Fn must be a function/callable type");

    template <class R, class... Params>
    static auto impl(R (*)(Params...)) -> tl::type_list<Params...>;

    template <class R, class... Params>
    static auto impl(R (&)(Params...)) -> tl::type_list<Params...>;

    template <class R, class... Params>
    static auto impl(std::function<R(Params...)>) -> tl::type_list<Params...>;

    template <class R, class Class, class... Params>
    static auto impl(R (Class::*)(Params...)) -> tl::type_list<Params...>;

    template <class R, class Class, class... Params>
    static auto impl(R (Class::*)(Params...) const) -> tl::type_list<Params...>;

    template <class R, class Class, class... Params>
    static auto impl(R (Class::*)(Params...) volatile) -> tl::type_list<Params...>;

    template <class R, class Class, class... Params>
    static auto impl(R (Class::*)(Params...) noexcept) -> tl::type_list<Params...>;

    template <class Callable>
    static auto impl(Callable &&) -> decltype(impl(&std::decay_t<Callable>::operator()));

    static auto impl(...) -> tl::type_list<>;

public:
    using type = decltype(impl(std::declval<std::decay_t<Fn>>()));
};
}  // namespace detail