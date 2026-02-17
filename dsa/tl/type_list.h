#pragma once

#include "detail/adapter.h"
#include "detail/algorithm.h"
#include "detail/core.h"
#include "detail/utilities.h"

namespace tl {

template <class... Ts>
struct type_list {
    using type = type_list<Ts...>;
    using Indices = std::index_sequence_for<Ts...>;

    static constexpr std::size_t size = sizeof...(Ts);
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);
    static constexpr bool is_empty = (type::size == 0);

    template <class T>
    using contains = detail::contains<T, type>;
    template <class T>
    static constexpr bool contains_v = contains<T>::value;

    template <class T>
    static constexpr bool none_of = !contains_v<T>;
    template <class T>
    static constexpr bool any_of = contains_v<T>;
    template <class T>
    static constexpr bool all_of = (std::is_same_v<T, Ts> && ...);

    template <template <class> class Pred>
    static constexpr bool any_if = (Pred<Ts>::value || ...);
    template <template <class> class Pred>
    static constexpr bool all_if = (Pred<Ts>::value && ...);

    template <template <class> class Pred>
    static constexpr std::size_t count_if = (0 + ... + (Pred<Ts>::value ? 1 : 0));
    template <class T>
    static constexpr std::size_t count_of = count_if<detail::is_same<T>::template impl>;

    template <std::size_t I>
    // using at = std::tuple_element<I, std::tuple<Ts...>>;
    using at = detail::select<type, I>;
    template <std::size_t I>
    using at_t = typename at<I>::type;

    template <template <class...> class Fn>
    using apply = Fn<Ts...>;
    template <template <class...> class Fn>
    using apply_t = typename apply<Fn>::type;

    template <std::size_t Start, std::size_t Length>
    using slice =
        std::conditional_t<Length == 0 || is_empty, type_list<>, detail::slice<type, Start, Length>>;
    template <std::size_t Start, std::size_t Length>
    using slice_t = typename slice<Start, Length>::type;

    template <std::size_t N>
    using head = slice<0, N>;
    template <std::size_t N>
    using head_t = typename head<N>::type;

    template <std::size_t N>
    using tail = slice<size - N, N>;
    template <std::size_t N>
    using tail_t = typename tail<N>::type;

    template <class... Us>
    using append = type_list<Ts..., Us...>;
    template <class... Us>
    using append_t = typename append<Us...>::type;

    template <class... Us>
    using prepend = type_list<Us..., Ts...>;
    template <class... Us>
    using prepend_t = typename prepend<Us...>::type;

    using front = std::conditional_t<is_empty, type_list<>, at<0>>;
    using front_t = typename front::type;

    using back = std::conditional_t<is_empty, type_list<>, at<size - 1>>;
    using back_t = typename back::type;

    using pop_front = std::conditional_t<is_empty, type_list<>, tail<size - 1>>;
    using pop_front_t = typename pop_front::type;

    using pop_back = std::conditional_t<is_empty, type_list<>, head<size - 1>>;
    using pop_back_t = typename pop_back::type;

    template <class... Us>
    using rebind = type_list<Us...>;
    template <class Container>
    using from_container_t = detail::to_type_list_t<Container>;
    template <class Fn>
    using from_function_t = typename detail::from_function<Fn>::type;

    using to_tuple_t = detail::to_tuple_t<type>;
    using to_variant_t = detail::to_variant_t<type>;
    template <template <class...> class Container>
    using to_container_t = Container<Ts...>;

    template <template <class...> class Action, class... Us>
    using transform = type_list<Action<Ts, Us...>...>;
    template <template <class...> class Action, class... Us>
    using transform_t = typename transform<Action, Us...>::type;

    template <template <class, class> class Action, class Init>
    using accumulate = detail::accumulate<Action, Init, Ts...>;
    template <template <class, class> class Action, class Init>
    using accumulate_t = typename accumulate<Action, Init>::type;

    template <class From, class To>
    using replace = type_list<std::conditional_t<std::is_same_v<Ts, From>, To, Ts>...>;
    template <class From, class To>
    using replace_t = typename replace<From, To>::type;

    template <std::size_t I, class T>
    using replace_at = detail::replace_at<type, I, T>;
    template <std::size_t I, class T>
    using replace_at_t = typename replace_at<I, T>::type;

    template <std::size_t I>
    using remove_at = detail::remove_at<type, I>;
    template <std::size_t I>
    using remove_at_t = typename remove_at<I>::type;

    template <std::size_t I, class T>
    using insert_at = detail::insert_at<type, I, T>;
    template <std::size_t I, class T>
    using insert_at_t = typename insert_at<I, T>::type;

    using unique = accumulate<detail::unique, type_list<>>;
    using unique_t = typename unique::type;

    using reverse = detail::reverse<type>;
    using reverse_t = typename reverse::type;

    template <class T>
    using remove = accumulate<detail::remove<T>::template impl, type_list<>>;
    template <class T>
    using remove_t = typename remove<T>::type;

    template <template <class> class Pred>
    using filter = accumulate<detail::filter<Pred>::template impl, type_list<>>;
    template <template <class> class Pred>
    using filter_t = typename filter<Pred>::type;

    template <template <class> class Pred>
    using reject = accumulate<detail::reject<Pred>::template impl, type_list<>>;
    template <template <class> class Pred>
    using reject_t = typename reject<Pred>::type;

    static constexpr bool no_duplicates = (unique_t::size == size);

    template <template <class> class Pred>
    static constexpr std::size_t find_first_if = [] {
        std::size_t index = 0;
        return ((Pred<Ts>::value ? true : (++index, false)) || ...) ? index : npos;
    }();

    template <template <class> class Pred>
    static constexpr std::size_t find_last_if = [] {
        constexpr std::size_t index = type::reverse_t::template find_first_if<Pred>;
        return index == npos ? npos : type::size - 1 - index;
    }();

    template <template <class> class Pred>
    static constexpr std::size_t find_first_not_if = [] {
        std::size_t index = 0;
        return ((Pred<Ts>::value ? (++index, false) : true) || ...) ? index : npos;
    }();

    template <template <class> class Pred>
    static constexpr std::size_t find_last_not_if = [] {
        constexpr std::size_t index = type::reverse_t::template find_first_not_if<Pred>;
        return index == npos ? npos : type::size - 1 - index;
    }();

    template <class T>
    static constexpr std::size_t index_of = find_first_if<detail::is_same<T>::template impl>;

    /*template <class List>
    static constexpr bool is_subset_of = [] {
        if constexpr (size > List::size) {
            return false;
        } else {
            return (List::template contains_v<Ts> && ...);
        }
    }();*/

    template <class List>
    static constexpr bool is_subset_of = (List::template contains_v<Ts> && ...);

    template <class List>
    static constexpr bool equal_to = (size == List::size && is_subset_of<List>);

    template <class List>
    using union_with = accumulate<detail::unique, typename List::unique_t>;
    template <class List>
    using union_with_t = typename union_with<List>::type;

    template <class List>
    using difference_with = reject<List::template contains>;
    template <class List>
    using difference_with_t = typename difference_with<List>::type;

    template <class List>
    using intersect_with = filter<List::template contains>;
    template <class List>
    using intersect_with_t = typename intersect_with<List>::type;

    template <class Other>
    using concat = detail::concat<type, Other>;
    template <class Other>
    using concat_t = typename concat<Other>::type;

    using flatten = detail::flatten<type>;
    using flatten_t = typename flatten::type;

    template <class... Us>
    using zip =
        std::conditional_t<is_empty, type_list<>,
                           std::conditional_t<detail::all_are_container_v<Us...>, detail::zip<type, Us...>,
                                              detail::zip<type, type_list<Us...>>>>;
    template <class... Us>
    using zip_t = typename zip<Us...>::type;

    using unzip = std::conditional_t<(is_empty || !detail::all_are_container_v<Ts...> ||
                                      !detail::min_container_size_v<Ts...>),
                                     detail::identity<std::tuple<>>, detail::unzip<type>>;
    using unzip_t = typename unzip::type;

    template <std::size_t I>
    using unzip_at = std::conditional_t<(is_empty || !detail::all_are_container_v<Ts...>), type_list<>,
                                        detail::unzip_at<I, type>>;
    template <std::size_t I>
    using unzip_at_t = typename unzip_at<I>::type;

    // todo: improve this
    template <conjunction_type CT = conjunction_type::Seq, class F, class Init>
    static constexpr auto fold(F &&f, Init &&init) {
        auto acc = std::forward<Init>(init);
        if constexpr (CT == conjunction_type::And) {
            (void)((acc = std::forward<F>(f)(acc, detail::identity<Ts>{})) && ...);
        } else if constexpr (CT == conjunction_type::Or) {
            (void)((acc = std::forward<F>(f)(acc, detail::identity<Ts>{})) || ...);
        } else {
            ((acc = std::forward<F>(f)(acc, detail::identity<Ts>{})), ...);
        }
        return acc;
    }

    // todo: improve this
    template <class F, class... Args>
    static constexpr void for_each(F &&f, Args &&...args) {
        (std::forward<F>(f)(detail::identity<Ts>{}, std::forward<Args>(args)...), ...);
    }

    // todo: improve this
    template <class F, class... Args>
    static constexpr void reverse_for_each(F &&f, Args &&...args) {
        if constexpr (size > 0) {
            [&]<size_t... Is>(std::index_sequence<Is...>) {
                (std::forward<F>(f)(detail::identity<at_t<size - 1 - Is>>{}, std::forward<Args>(args)...),
                 ...);
            }(Indices{});
        }
    }

    // todo: improve this
    template <std::size_t I, class F, class... Args>
    static constexpr auto dispatch_at(F &&f, Args &&...args) {
        static_assert(I < size, "dispatch_at: Index out of range");
        return std::invoke(std::forward<F>(f), detail::identity<at_t<I>>{}, std::forward<Args>(args)...);
    }

    // todo: improve this
    template <class F, class... Args>
    static constexpr void dispatch_all(F &&f, Args &&...args) {
        if constexpr (size > 0) {
            [&]<size_t... Is>(std::index_sequence<Is...>) {
                (dispatch_at<Is>(std::forward<F>(f), std::forward<Args>(args)...), ...);
            }(Indices{});
        }
    }
};

}  // namespace tl