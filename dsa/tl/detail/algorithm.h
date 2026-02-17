#pragma once

#include <limits>

#include "adapter.h"
#include "core.h"
#include "utilities.h"

namespace detail {
template <class Container>
using container_t = typename container_traits<Container>::type;

template <class Container, std::size_t I>
using element_t = typename container_traits<Container>::template at<I>;

template <class Container>
static constexpr bool is_container_v = container_traits<Container>::is_container;

template <class Container>
static constexpr std::size_t container_size_v = container_traits<Container>::size;

template <class... Containers>
static constexpr bool all_are_container_v = (is_container_v<Containers> && ...);

template <class... Containers>
static constexpr std::size_t min_container_size_v = [] {
    static constexpr std::size_t size = sizeof...(Containers);
    static constexpr bool all_are_containers = all_are_container_v<Containers...>;
    static constexpr bool valid = size && all_are_containers;
    if constexpr (valid) {
        std::size_t min = std::numeric_limits<std::size_t>::max();
        ((min = (container_size_v<Containers> < min) ? container_size_v<Containers> : min), ...);
        return min;
    } else {
        return 0;
    }
}();

template <class T, class Container>
struct contains {
private:
    static_assert(is_container_v<Container>, "contains requires a type_list/tuple/variant");

    template <class>
    struct impl;

    template <class... Ts>
    struct impl<tl::type_list<Ts...>> {
        static constexpr bool value = (std::is_same_v<T, Ts> || ...);
    };

public:
    static constexpr bool value = impl<detail::to_type_list_t<Container>>::value;
};

template <class T, class Container>
static constexpr bool contains_v = contains<T, Container>::value;

template <class, class>
struct concat;

template <class... Ts, class... Us>
struct concat<tl::type_list<Ts...>, tl::type_list<Us...>> {
    using type = tl::type_list<Ts..., Us...>;
};

template <class L, class R>
struct concat {
private:
    static_assert(is_container_v<L> && is_container_v<R>,
                  "concat requires both parameters to be type_list/tuple/variant");
    using left = detail::to_type_list_t<L>;
    using right = detail::to_type_list_t<R>;

public:
    using type = typename convert_to<typename concat<left, right>::type, container_t<L>>::type;
};

template <class L, class R>
using concat_t = typename concat<L, R>::type;

template <class... Containers>
struct concat_all {
private:
    static_assert(sizeof...(Containers) > 0, "concat_all requires at least one list");
    static_assert(all_are_container_v<Containers...>,
                  "concat_all requires all parameters to be type_list/tuple/variant");

    template <class... Lists>
    struct impl;

    template <class... Ts>
    struct impl<tl::type_list<Ts...>> {
        using type = tl::type_list<Ts...>;
    };

    template <class... Ts, class... Us, class... Rest>
    struct impl<tl::type_list<Ts...>, tl::type_list<Us...>, Rest...> {
        using type = typename impl<tl::type_list<Ts..., Us...>, Rest...>::type;
    };

public:
    using type = typename impl<detail::to_type_list_t<Containers>...>::type;
};

template <class Container>
struct reverse {
private:
    static_assert(is_container_v<Container>, "reverse requires a type_list/variant/tuple");

    template <std::size_t... Is>
    static constexpr auto impl(std::index_sequence<Is...>)
        -> tl::type_list<element_t<Container, container_size_v<Container> - 1 - Is>...>;

    using Indices = std::make_index_sequence<container_size_v<Container>>;

public:
    using type = decltype(impl(Indices{}));
};

template <class U>
struct remove {
    template <class Acc, class T>
    struct impl {
        using type = std::conditional_t<std::is_same_v<U, T>, Acc, typename Acc::template append<T>>;
    };
};

template <class Acc, class T>
struct unique {
    using type = std::conditional_t<Acc::template contains_v<T>, Acc, typename Acc::template append<T>>;
};

template <template <class> class Pred>
struct filter {
    template <class Acc, class T>
    struct impl {
        using type = std::conditional_t<Pred<T>::value, typename Acc::template append<T>, Acc>;
    };
};

template <template <class> class Pred>
struct reject {
    template <class Acc, class T>
    struct impl {
        using type = std::conditional_t<Pred<T>::value, Acc, typename Acc::template append<T>>;
    };
};

template <class Container, std::size_t I>
struct select {
    static_assert(is_container_v<Container>, "select requires a type_list/variant/tuple");
    static_assert(I < container_size_v<Container>, "Index out of range for select");

    /*template <std::size_t, class>
    struct impl;

    template <class T, class... Ts>
    struct impl<0, tl::type_list<T, Ts...>> {
        using type = T;
    };

    template <std::size_t Idx, class T, class... Ts>
    struct impl<Idx, tl::type_list<T, Ts...>> {
        using type = typename impl<Idx - 1, tl::type_list<Ts...>>::type;
    };*/

    using type = std::tuple_element_t<I, detail::to_tuple_t<Container>>;
};

template <class List, std::size_t Start, std::size_t Length>
struct slice {
private:
    static_assert(is_container_v<List>, "slice requires a type_list/variant/tuple");
    static_assert(Start + Length <= container_size_v<List>, "Slice Start + Length out of range");

    template <std::size_t... Is>
    static auto impl(std::index_sequence<Is...>) -> tl::type_list<element_t<List, Start + Is>...>;

    using Indices = std::make_index_sequence<Length>;

public:
    using type = decltype(impl(Indices{}));
};

template <template <class, class> class Action, class Init, class... Ts>
struct accumulate {
private:
    template <class Acc, class... Rest>
    struct impl;

    template <class Acc>
    struct impl<Acc> {
        using type = Acc;
    };

    template <class Acc, class T>
    struct impl<Acc, T> {
        using type = typename Action<Acc, T>::type;
    };

    template <class Acc, class First, class Second, class... Rest>
    struct impl<Acc, First, Second, Rest...> {
    private:
        using first = typename Action<Acc, First>::type;
        using second = typename Action<first, Second>::type;

    public:
        using type = typename impl<second, Rest...>::type;
    };

public:
    using type = typename impl<Init, Ts...>::type;
};

template <class List, std::size_t I, class T>
struct replace_at {
private:
    static constexpr std::size_t size = container_size_v<List>;

    static_assert(is_container_v<List>, "replace_at requires type_list/tuple/variant");
    static_assert(I < size, "Index out of range for replace_at");

    using head = typename slice<List, 0, I>::type;
    using tail = typename slice<List, I + 1, size - I - 1>::type;

public:
    using type = typename concat<typename head::template append<T>, tail>::type;
};

template <class List, std::size_t I, class T>
struct insert_at {
private:
    static constexpr std::size_t size = container_size_v<List>;

    static_assert(I <= size, "Index out of range for insert_at");
    static_assert(is_container_v<List>, "insert_at requires type_list/tuple/variant");

    using head = typename slice<List, 0, I>::type;
    using tail = typename slice<List, I, size - I>::type;

public:
    using type = typename concat<typename head::template append<T>, tail>::type;
};

template <class List, std::size_t I>
struct remove_at {
private:
    static constexpr std::size_t size = container_size_v<List>;

    static_assert(I < size, "Index out of range for remove_at");
    static_assert(is_container_v<List>, "remove_at requires type_list/tuple/variant");

    using head = typename slice<List, 0, I>::type;
    using tail = typename slice<List, I + 1, size - I - 1>::type;

public:
    using type = typename concat<head, tail>::type;
};

template <class List, class... Containers>
struct zip {
private:
    static constexpr std::size_t min_size = min_container_size_v<List, Containers...>;
    static_assert(is_container_v<List> && (is_container_v<Containers> && ...),
                  "zip requires type_list/variant/tuple");
    static_assert(min_size, "zip requires at least one tuple");

    template <std::size_t I>
    using zip_element_t = std::tuple<element_t<List, I>, element_t<Containers, I>...>;

    template <std::size_t... Is>
    static auto impl(std::index_sequence<Is...>) -> tl::type_list<zip_element_t<Is>...>;

    using Indices = std::make_index_sequence<min_size>;

public:
    using type = decltype(impl(Indices{}));
};

template <std::size_t I, class List>
struct unzip_at {
    template <std::size_t, class>
    struct impl;

    template <std::size_t Idx, class... Containers>
    struct impl<Idx, tl::type_list<Containers...>> {
        static constexpr std::size_t size = sizeof...(Containers);
        static constexpr std::size_t min_container_size = min_container_size_v<Containers...>;

        static_assert(min_container_size && Idx < min_container_size,
                      " unzip_at requires at least one tuple or invalid index");
        using type = tl::type_list<element_t<Containers, Idx>...>;
    };

public:
    using type = typename impl<I, List>::type;
};

template <class List>
struct unzip {
private:
    template <class>
    struct impl;

    template <class... Containers>
    struct impl<tl::type_list<Containers...>> {
    private:
        static constexpr std::size_t size = sizeof...(Containers);
        static constexpr std::size_t min_container_size = min_container_size_v<Containers...>;

        static_assert(size && min_container_size, "unzip requires at least one tuple");

        template <std::size_t I>
        using unzip_element_t = tl::type_list<element_t<Containers, I>...>;

        template <std::size_t... Is>
        static auto get(std::index_sequence<Is...>) -> std::tuple<unzip_element_t<Is>...>;

        using Indices = std::make_index_sequence<min_container_size>;

    public:
        using type = decltype(get(Indices{}));
    };

public:
    using type = typename impl<List>::type;
};

template <class>
struct flatten;

template <class... Ts>
struct flatten<tl::type_list<Ts...>> {
private:
    template <class Us>
    struct flatten_with {
        using type = tl::type_list<Us>;
    };

    template <class... Us>
    struct flatten_with<tl::type_list<Us...>> {
        using type = typename flatten<tl::type_list<Us...>>::type;
    };

    template <class... Us>
    struct flatten_with<std::variant<Us...>> {
        using type = typename flatten<tl::type_list<Us...>>::type;
    };

    template <class... Us>
    struct flatten_with<std::tuple<Us...>> {
        using type = typename flatten<tl::type_list<Us...>>::type;
    };

    template <class List, class... Rest>
    struct impl;

    template <class... Us>
    struct impl<tl::type_list<Us...>> {
        using type = tl::type_list<Us...>;
    };

    template <class... Us, class T>
    struct impl<tl::type_list<Us...>, T> {
    private:
        using list = tl::type_list<Us...>;
        using first = typename flatten_with<T>::type;

    public:
        using type = typename list::concat<first>::type;
    };

    template <class... Us, class T1, class T2, class... Rest>
    struct impl<tl::type_list<Us...>, T1, T2, Rest...> {
    private:
        using list = tl::type_list<Us...>;
        using first = typename flatten_with<T1>::type;
        using second = typename flatten_with<T2>::type;
        using merged = typename concat_all<list, first, second>::type;

    public:
        using type = typename impl<merged, Rest...>::type;
    };

public:
    using type = typename impl<tl::empty_list, Ts...>::type;
};
}  // namespace detail