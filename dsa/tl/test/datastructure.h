#pragma once

#include "type_list.h"
namespace tl_test {

// 测试类型
struct A {};
struct B {};
struct C {};
struct D {};
struct E {};
struct F {};
struct X {};
struct Y {};
struct Z {};
struct user {};
struct product {};
struct order {};
struct user_dto {};
struct product_dto {};
struct order_dto {};
struct user_view {};
struct product_view {};
struct order_view {};

template <int N>
struct int_type {
    static constexpr int value = N;
};

template <typename T>
struct extract_first_type {
    using type = std::tuple_element_t<0, T>;
};
template <typename T>
using extract_first_type_t = typename extract_first_type<T>::type;

template <typename T>
struct add_index {
    template <std::size_t I>
    static auto make() {
        return std::tuple<T, std::integral_constant<std::size_t, I>>{};
    }
};

// 用于测试新transform功能的模板类

// 带索引的元组构造器
template <typename T, typename Index>
struct make_indexed_tuple {
    using type = std::tuple<T, Index>;
};
template <typename T, typename Index>
using make_indexed_tuple_t = typename make_indexed_tuple<T, Index>::type;

// 带标签的类型构造器
template <typename T, typename Tag>
struct make_tagged_type {
    using type = std::tuple<T, Tag>;
};
template <typename T, typename Tag>
using make_tagged_type_t = typename make_tagged_type<T, Tag>::type;

// 类型转换器，可以添加多个修饰
template <typename T, typename Modifier1, typename Modifier2>
struct decorate_type {
    using type = std::tuple<T, Modifier1, Modifier2>;
};
template <typename T, typename Modifier1, typename Modifier2>
using decorate_type_t = typename decorate_type<T, Modifier1, Modifier2>::type;

// 用于提取和修改元组的模板
template <typename T>
struct extract_and_modify {
    using type = T;  // 默认实现
};

template <typename T, std::size_t I>
struct extract_and_modify<std::tuple<T, std::integral_constant<std::size_t, I>>> {
    // 提取原始类型并添加新的元数据
    using type =
        std::tuple<T, std::integral_constant<std::size_t, I>, std::integral_constant<std::size_t, I * 2>>;
};

template <typename T>
using extract_and_modify_t = typename extract_and_modify<T>::type;

// 可配置的转换器
template <typename Extra1, typename Extra2>
struct configurable_transformer {
    template <typename T>
    struct apply {
        using type = std::tuple<T, Extra1, Extra2>;
    };
};
template <typename T, typename Extra1, typename Extra2>
using configurable_transformer_apply_t =
    typename configurable_transformer<Extra1, Extra2>::template apply<T>::type;

struct field_info {
    static constexpr const char *name = "field";
    static constexpr std::size_t alignment = 8;
};

// 添加字段信息的模板
template <typename T>
struct add_field_info {
    using type = std::tuple<T, field_info>;
};
template <typename T>
using add_field_info_t = typename add_field_info<T>::type;

// 添加类型特性的模板
template <typename T>
struct add_type_traits {
    using type = std::tuple<T, std::integral_constant<bool, std::is_trivially_default_constructible_v<T>>,
                            std::integral_constant<bool, std::is_default_constructible_v<T>>>;
};
template <typename T>
using add_type_traits_t = typename add_type_traits<T>::type;

// 序列化信息模板
template <typename T>
struct serialization_info {
    static constexpr std::size_t size = sizeof(T);
    static constexpr std::size_t alignment = alignof(T);

    using type = std::tuple<T, std::integral_constant<std::size_t, size>,
                            std::integral_constant<std::size_t, alignment>>;
};
template <typename T>
using serialization_info_t = typename serialization_info<T>::type;

// 条件转换模板
template <typename T>
struct conditional_transform {
    using type = std::conditional_t<std::is_trivially_default_constructible_v<T>,
                                    std::tuple<T, std::true_type>, std::tuple<T, std::false_type>>;
};
template <typename T>
using conditional_transform_t = typename conditional_transform<T>::type;

// 用于索引转换的模板
template <typename T, std::size_t I>
struct make_indexed {
    using type = std::tuple<T, std::integral_constant<std::size_t, I>>;
};
template <typename T, std::size_t I>
using make_indexed_t = typename make_indexed<T, I>::type;

// 带索引的序列化模板
template <typename T, std::size_t Index>
struct serialize_with_index {
    static constexpr std::size_t offset = Index * sizeof(T);
    using type = std::tuple<T, std::integral_constant<std::size_t, Index>,
                            std::integral_constant<std::size_t, sizeof(T)>,
                            std::integral_constant<std::size_t, offset>>;
};

template <typename T, std::size_t Index>
using serialize_with_index_t = typename serialize_with_index<T, Index>::type;

// 用于应用序列化的模板
template <typename Tuple>
struct apply_serialize;

template <typename T, typename Index>
struct apply_serialize<std::tuple<T, Index>> {
    using type = typename serialize_with_index<T, Index::value>::type;
};
template <typename Tuple>
using apply_serialize_t = typename apply_serialize<Tuple>::type;

// 累积偏移量序列化模板
template <typename Tuple>
struct apply_accumulated_serialize;

template <typename T, typename Index, typename Offset>
struct apply_accumulated_serialize<std::tuple<T, Index, Offset>> {
    using type = std::tuple<T, std::integral_constant<std::size_t, Index::value>,
                            std::integral_constant<std::size_t, sizeof(T)>, Offset>;
};

template <typename Tuple>
using apply_accumulated_serialize_t = typename apply_accumulated_serialize<Tuple>::type;

template <std::size_t... Is>
auto make_index_list(std::index_sequence<Is...>) -> tl::type_list<std::integral_constant<std::size_t, Is>...>;

template <typename T>
struct is_even_value : std::false_type {};

template <int N>
struct is_even_value<int_type<N>> : std::bool_constant<N % 2 == 0> {};

template <typename T>
struct is_odd_value : std::false_type {};

template <int N>
struct is_odd_value<int_type<N>> : std::bool_constant<N % 2 == 1> {};

template <typename T>
struct is_type_a_or_c : std::bool_constant<std::is_same_v<T, A> || std::is_same_v<T, C>> {};

template <typename T>
struct is_type_b : std::bool_constant<std::is_same_v<T, B>> {};

template <typename T>
struct is_int_type : std::false_type {};

template <int N>
struct is_int_type<int_type<N>> : std::true_type {};

template <typename T>
struct is_a_or_b : std::bool_constant<std::is_same_v<T, A> || std::is_same_v<T, B>> {};

template <typename T>
struct is_not_b : std::bool_constant<!std::is_same_v<T, B>> {};

template <typename T>
struct is_replacable : std::bool_constant<std::is_same_v<T, B> || std::is_same_v<T, D>> {};

template <class T>
struct is_pointer {
    static constexpr bool value = false;
};

template <class T>
struct is_pointer<T *> {
    static constexpr bool value = true;
};

template <class T>
struct is_integral {
    static constexpr bool value = std::is_integral_v<T>;
};

template <class T>
struct is_floating {
    static constexpr bool value = std::is_floating_point_v<T>;
};

template <class T>
struct is_reference {
    static constexpr bool value = false;
};

template <class T>
struct is_reference<T &> {
    static constexpr bool value = true;
};

struct test_accumulator {
    std::size_t count = 0;
    std::size_t total_size = 0;

    template <class T>
    bool operator()(test_accumulator &acc, detail::identity<T>) {
        acc.count++;
        acc.total_size += sizeof(T);
        return true;
    }
};

template <class Acc, class T>
struct acc_size {
    using type = std::integral_constant<std::size_t, Acc::value + sizeof(T)>;
};

template <class Acc, class T>
struct make_tuple_list {
    using type = typename Acc::template append<std::tuple<T>>;
};

// transform
template <typename T>
struct add_pointer {
    using type = T *;
};
template <typename T>
using add_pointer_t = typename add_pointer<T>::type;

// for_each 测试
struct Counter {
    int count = 0;
    template <typename T>
    void operator()(detail::identity<T>) {
        ++count;
    }
};

struct sum_calculator {
    int sum = 0;
    template <typename T>
    void operator()(detail::identity<T>) {
        sum += T::value;
    }
};

template <typename Acc, typename T>
struct sum_accumulator {
    using type = int_type<Acc::value + T::value>;
};

// 字符串长度累积
template <typename Acc, typename T>
struct type_name_length {
    static constexpr size_t value = Acc::value + sizeof(T);
    using type = int_type<value>;
};

// replace string_view with CompileTimeString
/*template <class T>
struct ToDescriptor {
    using type = field_descriptor<std::integral_constant<std::string_view,
"field">, T>;
};
template <class T>
using ToDescriptor_t = typename ToDescriptor<T>::type;*/

}  // namespace tl_test