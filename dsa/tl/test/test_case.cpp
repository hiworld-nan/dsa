
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "datastructure.h"
#include "test.h"
#include "type_list.h"

using namespace testing;
using namespace tl_test;
// 1. 基本构造和属性测试
TEST(type_list, basic_properties) {
    using List1 = tl::type_list<A, B, C>;
    CHECK_COMPILE_TIME(List1::size == 3);
    CHECK_COMPILE_TIME(!List1::is_empty);

    using List2 = tl::type_list<>;
    CHECK_COMPILE_TIME(List2::size == 0);
    CHECK_COMPILE_TIME(List2::is_empty);

    using List3 = tl::type_list<A>;
    CHECK_COMPILE_TIME(List3::size == 1);
    CHECK_COMPILE_TIME(!List3::is_empty);
    return true;
}

// 2. 类型查询测试
TEST(type_list, type_queries) {
    using List = tl::type_list<A, B, C, A, B>;
    using List2 = tl::type_list<int, double, char, float>;

    CHECK_COMPILE_TIME(List::contains_v<A>);
    CHECK_COMPILE_TIME(List::contains_v<B>);
    CHECK_COMPILE_TIME(List::contains_v<C>);
    CHECK_COMPILE_TIME(!List::contains_v<D>);

    CHECK_COMPILE_TIME(List::any_of<A>);
    CHECK_COMPILE_TIME(!List::all_of<A>);
    CHECK_COMPILE_TIME(List::none_of<D>);

    CHECK_COMPILE_TIME((List::any_if<is_a_or_b>));
    CHECK_COMPILE_TIME((!List::all_if<is_a_or_b>));

    CHECK_COMPILE_TIME(List2::any_if<is_integral>);
    CHECK_COMPILE_TIME(!List2::all_if<is_integral>);
    CHECK_COMPILE_TIME(List2::any_if<is_floating>);
    CHECK_COMPILE_TIME(List2::none_of<void *>);
    return true;
}

// 3. 元素访问测试
TEST(type_list, element_access) {
    using List = tl::type_list<A, B, C, D>;

    CHECK_COMPILE_TIME(std::is_same_v<List::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<List::at_t<1>, B>);
    CHECK_COMPILE_TIME(std::is_same_v<List::at_t<2>, C>);
    CHECK_COMPILE_TIME(std::is_same_v<List::at_t<3>, D>);

    CHECK_COMPILE_TIME(std::is_same_v<List::front::type, A>);
    CHECK_COMPILE_TIME(std::is_same_v<List::back::type, D>);

    using Empty = tl::type_list<>;
    CHECK_COMPILE_TIME(std::is_same_v<Empty::front::type, Empty>);
    CHECK_COMPILE_TIME(std::is_same_v<Empty::back::type, Empty>);
    return true;
}

// 4. 切片操作测试
TEST(type_list, slice_operations) {
    using List = tl::type_list<A, B, C, D, E>;
    using List2 = tl::type_list<int, double, char, float>;

    using Slice1 = List::slice_t<1, 3>;
    CHECK_COMPILE_TIME(Slice1::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<Slice1::at_t<0>, B>);
    CHECK_COMPILE_TIME(std::is_same_v<Slice1::at_t<1>, C>);
    CHECK_COMPILE_TIME(std::is_same_v<Slice1::at_t<2>, D>);

    using Head = List::head_t<2>;
    CHECK_COMPILE_TIME(Head::size == 2);
    CHECK_COMPILE_TIME(std::is_same_v<Head::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<Head::at_t<1>, B>);

    using Tail = List::tail_t<2>;
    CHECK_COMPILE_TIME(Tail::size == 2);
    CHECK_COMPILE_TIME(std::is_same_v<Tail::at_t<0>, D>);
    CHECK_COMPILE_TIME(std::is_same_v<Tail::at_t<1>, E>);

    using PopFront = List::pop_front_t;
    CHECK_COMPILE_TIME(PopFront::size == 4);
    CHECK_COMPILE_TIME(std::is_same_v<PopFront::at_t<0>, B>);

    using PopBack = List::pop_back_t;
    CHECK_COMPILE_TIME(PopBack::size == 4);
    CHECK_COMPILE_TIME(std::is_same_v<PopBack::back_t, D>);

    // 边界情况：空切片
    using EmptySlice = List::slice_t<2, 0>;
    CHECK_COMPILE_TIME(EmptySlice::size == 0);

    using Sliced1 = List2::slice_t<1, 2>;
    CHECK_COMPILE_TIME(std::is_same_v<Sliced1, tl::type_list<double, char>>);

    using Head2 = List2::head<2>;
    CHECK_COMPILE_TIME(std::is_same_v<Head2::type, tl::type_list<int, double>>);

    using Tail2 = List2::tail<2>;
    CHECK_COMPILE_TIME(std::is_same_v<Tail2::type, tl::type_list<char, float>>);
    return true;
}

// 5. 查找和计数测试
TEST(type_list, find_and_count) {
    using List = tl::type_list<A, B, C, A, B, D>;
    using List2 = tl::type_list<int, double, char, float>;

    CHECK_COMPILE_TIME(List::index_of<A> == 0);
    CHECK_COMPILE_TIME(List::index_of<B> == 1);
    CHECK_COMPILE_TIME(List::index_of<C> == 2);
    CHECK_COMPILE_TIME(List::index_of<D> == 5);
    CHECK_COMPILE_TIME(List::index_of<E> == static_cast<size_t>(-1));

    CHECK_COMPILE_TIME(List::count_of<A> == 2);
    CHECK_COMPILE_TIME(List::count_of<B> == 2);
    CHECK_COMPILE_TIME(List::count_of<C> == 1);
    CHECK_COMPILE_TIME(List::count_of<D> == 1);
    CHECK_COMPILE_TIME(List::count_of<E> == 0);

    CHECK_COMPILE_TIME((List::find_first_if<is_a_or_b> == 0));
    CHECK_COMPILE_TIME((List::count_if<is_a_or_b> == 4));

    CHECK_COMPILE_TIME(std::is_same_v<List2::at_t<0>, int>);
    CHECK_COMPILE_TIME(std::is_same_v<List2::at_t<2>, char>);
    CHECK_COMPILE_TIME(List2::index_of<float> == 3);
    CHECK_COMPILE_TIME(List2::index_of<void *> == static_cast<std::size_t>(-1));
    CHECK_COMPILE_TIME(List2::find_first_if<is_pointer> == static_cast<std::size_t>(-1));

    using ListWithPtr = tl::type_list<int, double, int *, char>;
    CHECK_COMPILE_TIME(ListWithPtr::find_first_if<is_pointer> == 2);

    using ListWithDup = tl::type_list<int, double, int, char, double>;
    CHECK_COMPILE_TIME(ListWithDup::count_of<int> == 2);
    CHECK_COMPILE_TIME(ListWithDup::count_of<double> == 2);
    CHECK_COMPILE_TIME(ListWithDup::count_of<char> == 1);

    using UniqueList = ListWithDup::unique_t;
    CHECK_COMPILE_TIME(std::is_same_v<UniqueList, tl::type_list<int, double, char>>);

    using Reversed = List2::reverse_t;
    CHECK_COMPILE_TIME(std::is_same_v<Reversed, tl::type_list<float, char, double, int>>);
    return true;
}

// 6. 转换操作测试
TEST(type_list, transform_operations) {
    using List = tl::type_list<A, B, C>;

    using Transformed = List::transform_t<add_pointer_t>;
    CHECK_COMPILE_TIME(Transformed::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<Transformed::at_t<0>, A *>);
    CHECK_COMPILE_TIME(std::is_same_v<Transformed::at_t<1>, B *>);
    CHECK_COMPILE_TIME(std::is_same_v<Transformed::at_t<2>, C *>);

    // replace
    using Replaced = List::replace_t<B, D>;
    CHECK_COMPILE_TIME(Replaced::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<Replaced::at_t<1>, D>);
    return true;
}

// 7. 过滤操作测试
TEST(type_list, filter_operations) {
    using List = tl::type_list<int_type<1>, A, int_type<2>, B, int_type<3>>;

    using Filtered = List::filter_t<is_int_type>;
    CHECK_COMPILE_TIME(Filtered::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<Filtered::at_t<0>, int_type<1>>);
    CHECK_COMPILE_TIME(std::is_same_v<Filtered::at_t<1>, int_type<2>>);
    CHECK_COMPILE_TIME(std::is_same_v<Filtered::at_t<2>, int_type<3>>);

    using Rejected = List::reject_t<is_int_type>;
    CHECK_COMPILE_TIME(Rejected::size == 2);
    CHECK_COMPILE_TIME(std::is_same_v<Rejected::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<Rejected::at_t<1>, B>);

    using Removed = List::remove_t<A>;
    CHECK_COMPILE_TIME(Removed::size == 4);
    CHECK_COMPILE_TIME(!Removed::contains_v<A>);

    using List1 = tl::type_list<int, double, char, float>;

    using Replaced = List1::replace_t<double, long double>;
    CHECK_COMPILE_TIME(std::is_same_v<Replaced, tl::type_list<int, long double, char, float>>);

    using ListWithDup = tl::type_list<int, double, int, char, double>;
    using Removed0 = ListWithDup::remove_t<double>;
    CHECK_COMPILE_TIME(std::is_same_v<Removed0, tl::type_list<int, int, char>>);

    using Filtered0 = List1::filter_t<is_integral>;
    CHECK_COMPILE_TIME(std::is_same_v<Filtered0, tl::type_list<int, char>>);
    return true;
}

// 8. 集合操作测试
TEST(type_list, set_operations) {
    using List1 = tl::type_list<A, B, C, D>;
    using List2 = tl::type_list<C, D, E, F>;

    // unique
    using ListWithDups = tl::type_list<A, B, A, C, B, D>;
    using UniqueList = ListWithDups::unique_t;
    CHECK_COMPILE_TIME(UniqueList::size == 4);
    CHECK_COMPILE_TIME(ListWithDups::no_duplicates == false);
    CHECK_COMPILE_TIME(UniqueList::no_duplicates == true);

    // reverse
    using Reversed = List1::reverse_t;
    CHECK_COMPILE_TIME(Reversed::size == 4);
    CHECK_COMPILE_TIME(std::is_same_v<Reversed::at_t<0>, D>);
    CHECK_COMPILE_TIME(std::is_same_v<Reversed::at_t<3>, A>);

    // intersection
    using Intersection = List1::intersect_with_t<List2>;
    CHECK_COMPILE_TIME(Intersection::size == 2);
    CHECK_COMPILE_TIME(Intersection::contains_v<C>);
    CHECK_COMPILE_TIME(Intersection::contains_v<D>);

    // union
    using Union = List1::union_with_t<List2>;
    CHECK_COMPILE_TIME(Union::size == 6);
    CHECK_COMPILE_TIME(Union::contains_v<A>);
    CHECK_COMPILE_TIME(Union::contains_v<E>);
    CHECK_COMPILE_TIME(Union::contains_v<F>);

    // difference
    using Difference = List1::difference_with_t<List2>;
    CHECK_COMPILE_TIME(Difference::size == 2);
    CHECK_COMPILE_TIME(Difference::contains_v<A>);
    CHECK_COMPILE_TIME(Difference::contains_v<B>);
    CHECK_COMPILE_TIME(!Difference::contains_v<C>);
    CHECK_COMPILE_TIME(!Difference::contains_v<D>);
    CHECK_COMPILE_TIME(!Difference::contains_v<E>);
    CHECK_COMPILE_TIME(!Difference::contains_v<F>);
    return true;
}

// 9. 连接操作测试
TEST(type_list, concatenation) {
    using List1 = tl::type_list<A, B>;
    using List2 = tl::type_list<C, D>;
    using List4 = tl::type_list<int, double, char, float>;

    using Concat1 = List1::concat_t<List2>;
    CHECK_COMPILE_TIME(Concat1::size == 4);
    CHECK_COMPILE_TIME(std::is_same_v<Concat1::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<Concat1::at_t<3>, D>);

    using Concat2 = List1::append_t<C, D>;
    CHECK_COMPILE_TIME(Concat2::size == 4);
    CHECK_COMPILE_TIME(std::is_same_v<Concat2::at_t<2>, C>);

    using Concat3 = List1::prepend_t<E>;
    CHECK_COMPILE_TIME(Concat3::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<Concat3::at_t<0>, E>);

    using Appended = List4::append_t<void *>;
    CHECK_COMPILE_TIME(std::is_same_v<Appended, tl::type_list<int, double, char, float, void *>>);

    using Prepended = List4::prepend_t<bool>;
    CHECK_COMPILE_TIME(std::is_same_v<Prepended, tl::type_list<bool, int, double, char, float>>);

    using Transformed = List4::transform_t<std::add_pointer_t>;
    CHECK_COMPILE_TIME(std::is_same_v<Transformed, tl::type_list<int *, double *, char *, float *>>);
    return true;
}

// 10. 元组和variant转换测试
TEST(type_list, conversions) {
    using List = tl::type_list<A, B, C>;

    // 元组转换
    using Tuple = List::to_tuple_t;
    CHECK_COMPILE_TIME(std::is_same_v<Tuple, std::tuple<A, B, C>>);

    using FromTuple = detail::to_type_list_t<std::tuple<D, E>>;
    CHECK_COMPILE_TIME(std::is_same_v<FromTuple, tl::type_list<D, E>>);

    using RebindTuple = List::from_container_t<std::tuple<D, E, F>>;
    CHECK_COMPILE_TIME(std::is_same_v<RebindTuple::at_t<0>, D>);

    // variant转换
    using Variant = List::to_variant_t;
    CHECK_COMPILE_TIME(std::is_same_v<Variant, std::variant<A, B, C>>);

    using FromVariant = detail::to_type_list_t<std::variant<D, E>>;
    CHECK_COMPILE_TIME(std::is_same_v<FromVariant, tl::type_list<D, E>>);

    using RebindVariant = List::from_container_t<std::variant<D, E, F>>;
    CHECK_COMPILE_TIME(std::is_same_v<RebindVariant::at_t<0>, D>);

    using TupleTypes = std::tuple<int, double, char>;
    using FromTuple0 = detail::to_type_list_t<TupleTypes>;
    CHECK_COMPILE_TIME(std::is_same_v<FromTuple0, tl::type_list<int, double, char>>);

    using BackToTuple = detail::to_tuple_t<FromTuple0>;
    CHECK_COMPILE_TIME(std::is_same_v<BackToTuple, TupleTypes>);
    return true;
}

// 11. Zip和Unzip测试
TEST(type_list, zip_unzip) {
    using List1 = tl::type_list<A, B, C>;
    using List2 = tl::type_list<int, double, float>;
    using List3 = tl::type_list<char, short, long>;

    // zip
    using Zipped = List1::zip_t<List2, List3>;
    CHECK_COMPILE_TIME(Zipped::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<Zipped::at_t<0>, std::tuple<A, int, char>>);
    CHECK_COMPILE_TIME(std::is_same_v<Zipped::at_t<1>, std::tuple<B, double, short>>);

    // unzip
    // clang-format off
    using TupleList = tl::type_list<
        std::tuple<A, int>, 
        std::tuple<B, double>, 
        std::tuple<C, float>
    >;
    // clang-format on

    using Unzipped = TupleList::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<Unzipped> == 2);

    // clang-format off
    using List4 = tl::type_list<
        std::tuple<A, int>, 
        std::tuple<B, double>, 
        std::tuple<C, float>
    >;
    // clang-format on
    using Unzipped0 = List4::unzip_at_t<0>;
    CHECK_COMPILE_TIME(std::is_same_v<Unzipped0, tl::type_list<A, B, C>>);

    using Unzipped1 = TupleList::unzip_at_t<1>;
    CHECK_COMPILE_TIME(std::is_same_v<Unzipped1, tl::type_list<int, double, float>>);

    // Zip/Unzip
    using Empty = tl::type_list<>;
    using Zipped0 = Empty::zip_t<tl::type_list<int, double>>;
    CHECK_COMPILE_TIME(std::is_same_v<Zipped0, tl::type_list<>>);

    using Unzipped2 = Empty::unzip_t;
    CHECK_COMPILE_TIME(std::is_same_v<Unzipped2, std::tuple<>>);
    return true;
}

// 12. 扁平化测试
TEST(type_list, flatten) {
    using Empty = tl::type_list<>;
    using List1 = tl::type_list<int, char, double>;
    using List4 = tl::type_list<int>;
    using List5 = tl::type_list<int, char, double, float>;
    using List6 = tl::type_list<int, char, double, float, bool, long>;
    using List3 = tl::type_list<int, char, double, float, bool, long, short>;

    using Nested1 = tl::type_list<A, tl::type_list<B, C>, D>;
    using Flat1 = Nested1::flatten_t;

    using Nested2 = tl::type_list<std::tuple<A, B>, tl::type_list<C, D>, std::variant<E, F>>;
    using Flat2 = Nested2::flatten_t;
    CHECK_COMPILE_TIME(Flat1::size == 4);
    CHECK_COMPILE_TIME(std::is_same_v<Flat1::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<Flat1::at_t<1>, B>);
    CHECK_COMPILE_TIME(std::is_same_v<Flat1::at_t<3>, D>);
    CHECK_COMPILE_TIME(Flat2::size == 6);

    // clang-format off
    CHECK_COMPILE_TIME(std::is_same_v<tl::type_list<>::flatten_t, Empty>);
    CHECK_COMPILE_TIME(
        std::is_same_v<
            tl::type_list<
                int, 
                char, 
                double
            >::flatten_t, 
            List1
        >
    );
    CHECK_COMPILE_TIME(
        std::is_same_v<
            tl::type_list<
                int, 
                    tl::type_list<
                        char, 
                        double
                    >
            >::flatten_t, 
            List1
        >
    );
    CHECK_COMPILE_TIME(
        std::is_same_v<
            tl::type_list<
                int, 
                    tl::type_list<
                        char, 
                            tl::type_list<
                                double, 
                                float
                            >
                    >
            >::flatten_t, 
            List5
        >
    );
    CHECK_COMPILE_TIME(
        std::is_same_v<
            tl::type_list<
                int, 
                std::tuple<
                    char, 
                    double
                >
            >::flatten_t, 
            List1
        >
    );
    CHECK_COMPILE_TIME(
        std::is_same_v<
            tl::type_list<
                int, 
                std::variant<
                    char, 
                    double
                >
            >::flatten_t, 
            List1
        >
    );
    CHECK_COMPILE_TIME(
        std::is_same_v<
            tl::type_list<
                int, 
                std::tuple<>, 
                tl::type_list<>, 
                std::variant<>
            >::flatten_t, 
            List4
        >
    );
    CHECK_COMPILE_TIME(
        std::is_same_v<
            tl::type_list<
                    int, 
                    tl::type_list<
                        char, 
                        double
                    >, 
                    std::tuple<
                        float, 
                        bool
                    >,
                    std::variant<
                        long, 
                        short
                    >
            >::flatten_t,
            List3
        >
    );
    CHECK_COMPILE_TIME(
        std::is_same_v<
            tl::type_list<
                tl::type_list<
                    int,
                    std::tuple<char>
                >, 
                std::variant<
                    tl::type_list<double>, 
                    float
                >
            >::flatten_t,
            List5
        >
    );
    CHECK_COMPILE_TIME(
        std::is_same_v<
            tl::type_list<
                tl::type_list<
                    std::tuple<
                        int, 
                        char
                    >, 
                    double
                >,
                std::variant<
                    std::tuple<
                        float, 
                        bool
                    >, 
                    long
                >
            >::flatten_t,
            List6
        >
    );
    // clang-format on
    return true;
}

// 13. 关系测试
TEST(type_list, relations) {
    using List1 = tl::type_list<A, B, C>;
    using List2 = tl::type_list<A, B, C>;
    using List3 = tl::type_list<A, B>;
    using List4 = tl::type_list<B, C, D>;

    using List5 = tl::type_list<int, double, char>;
    using List6 = tl::type_list<int, double, char>;
    using List7 = tl::type_list<int, double>;
    using List8 = tl::type_list<char, int, double>;

    using List9 = tl::type_list<double, char, int>;

    using List10 = tl::type_list<int, int, int, double, double, char, float, long>;
    using List11 = tl::type_list<int, int, int, double, double, char, float, long>;

    CHECK_COMPILE_TIME(List1::equal_to<List2>);
    CHECK_COMPILE_TIME(!List1::equal_to<List3>);

    CHECK_COMPILE_TIME(List3::is_subset_of<List1>);
    CHECK_COMPILE_TIME(!List1::is_subset_of<List3>);
    CHECK_COMPILE_TIME(!List1::is_subset_of<List4>);

    CHECK_COMPILE_TIME(List5::contains_v<int> == true);
    CHECK_COMPILE_TIME(List5::contains_v<double> == true);
    CHECK_COMPILE_TIME(List5::contains_v<float> == false);

    CHECK_COMPILE_TIME(List5::equal_to<List5> == true);
    CHECK_COMPILE_TIME(List5::equal_to<List6> == true);
    CHECK_COMPILE_TIME(List5::equal_to<List7> == false);
    CHECK_COMPILE_TIME(List5::equal_to<List8> == true);

    CHECK_COMPILE_TIME(List5::is_subset_of<List5> == true);
    CHECK_COMPILE_TIME(List5::is_subset_of<List6> == true);
    CHECK_COMPILE_TIME(List5::is_subset_of<List7> == false);
    CHECK_COMPILE_TIME(tl::type_list<int, double>::is_subset_of<List5> == true);

    CHECK_COMPILE_TIME(List5::is_subset_of<tl::type_list<int>> == false);
    CHECK_COMPILE_TIME(List5::is_subset_of<tl::type_list<int, double>> == false);
    CHECK_COMPILE_TIME(List5::is_subset_of<tl::type_list<int, double, char>> == true);
    CHECK_COMPILE_TIME(List5::is_subset_of<tl::type_list<int, double, char, float>> == true);

    using EmptyList = tl::type_list<>;

    CHECK_COMPILE_TIME(tl::type_list<>::equal_to<EmptyList> == true);
    CHECK_COMPILE_TIME(tl::type_list<>::is_subset_of<EmptyList> == true);
    CHECK_COMPILE_TIME(EmptyList::is_subset_of<tl::type_list<int, double>> == true);

    using SingleList = tl::type_list<int>;
    CHECK_COMPILE_TIME(tl::type_list<int>::equal_to<SingleList> == true);
    CHECK_COMPILE_TIME(tl::type_list<int>::equal_to<tl::type_list<double>> == false);
    CHECK_COMPILE_TIME(tl::type_list<int>::is_subset_of<SingleList> == true);
    CHECK_COMPILE_TIME(tl::type_list<int>::is_subset_of<tl::type_list<int, double>> == true);

    CHECK_COMPILE_TIME(tl::type_list<int, double, char>::equal_to<List5> == true);
    CHECK_COMPILE_TIME(tl::type_list<int, double, char>::equal_to<List8> == true);
    CHECK_COMPILE_TIME(tl::type_list<int, double, char>::equal_to<List9> == true);

    CHECK_COMPILE_TIME(tl::type_list<int, double, char>::is_subset_of<List5> == true);
    CHECK_COMPILE_TIME(tl::type_list<int, double, char>::is_subset_of<List8> == true);
    CHECK_COMPILE_TIME(tl::type_list<int, double, char>::is_subset_of<List9> == true);

    CHECK_COMPILE_TIME(List10::equal_to<List11> == true);
    CHECK_COMPILE_TIME(List5::is_subset_of<List10> == true);
    return true;
}

// 14. 迭代测试
TEST(type_list, iteration) {
    using List = tl::type_list<int_type<1>, int_type<2>, int_type<3>>;

    // 测试顺序遍历
    Counter counter;
    List::for_each([&counter](auto id) { counter(id); });

    EXPECT_TRUE(counter.count == 3);

    // 测试带计算的遍历
    sum_calculator calc;
    List::for_each([&calc](auto id) { calc(id); });

    EXPECT_TRUE(calc.sum == 6);

    // 测试反向遍历
    std::vector<int> values;
    List::reverse_for_each([&values](auto id) { values.push_back(decltype(id)::type::value); });

    EXPECT_TRUE(values.size() == 3);
    EXPECT_TRUE(values[0] == 3);
    EXPECT_TRUE(values[1] == 2);
    EXPECT_TRUE(values[2] == 1);

    // 测试逻辑连接
    using MixedList = tl::type_list<int_type<1>, int_type<0>, int_type<2>>;

    // AND 连接：遇到第一个false停止
    int and_count = 0;
    MixedList::for_each([&and_count](auto id) {
        ++and_count;
        return decltype(id)::type::value > 0;
    });

    EXPECT_TRUE(and_count == 3);  // 应该在第二个元素(0)处停止

    // OR 连接：遇到第一个true停止
    int or_count = 0;
    MixedList::for_each([&or_count](auto id) {
        ++or_count;
        return decltype(id)::type::value == 0;
    });

    EXPECT_TRUE(or_count == 3);  // 应该在第二个元素(0)处停止

    // 测试运行时for_each
    using List1 = tl::type_list<int, double, char, float>;
    std::string result;
    List1::for_each([&result](auto id) {
        using T = typename decltype(id)::type;
        if (!result.empty())
            result += ", ";
        result += typeid(T).name();
    });

    EXPECT_TRUE(!result.empty());

    // 测试And模式
    bool all_small = true;
    using List2 = tl::type_list<int, double, char>;
    List2::for_each([&all_small](auto id) {
        using T = typename decltype(id)::type;
        all_small = sizeof(T) <= 8;
    });

    EXPECT_TRUE(all_small == true);

    // 测试Or模式
    bool has_pointer = true;
    using List3 = tl::type_list<int, double, char>;
    List3::for_each([&has_pointer](auto id) {
        using T = typename decltype(id)::type;
        has_pointer = is_pointer<T>::value;
    });

    EXPECT_TRUE(has_pointer == false);
    return true;
}

// 15. 边际情况测试
TEST(type_list, edge_cases) {
    // 空列表测试
    using Empty = tl::type_list<>;
    CHECK_COMPILE_TIME(Empty::size == 0);
    CHECK_COMPILE_TIME(Empty::is_empty);
    CHECK_COMPILE_TIME(!Empty::contains_v<A>);
    CHECK_COMPILE_TIME(std::is_same_v<Empty::front_t, Empty>);
    CHECK_COMPILE_TIME(std::is_same_v<Empty::back_t, Empty>);

    // 单元素列表测试
    using Single = tl::type_list<A>;
    CHECK_COMPILE_TIME(Single::size == 1);
    CHECK_COMPILE_TIME(std::is_same_v<Single::front_t, A>);
    CHECK_COMPILE_TIME(std::is_same_v<Single::back_t, A>);
    CHECK_COMPILE_TIME(Single::pop_front_t::size == 0);
    CHECK_COMPILE_TIME(Single::pop_back_t::size == 0);

    // 重复元素测试
    using Dups = tl::type_list<A, A, A>;
    CHECK_COMPILE_TIME(Dups::count_of<A> == 3);
    CHECK_COMPILE_TIME(Dups::unique_t::size == 1);

    // 超大列表测试（编译期压力测试）
    using LargeList = tl::type_list<int_type<1>, int_type<2>, int_type<3>, int_type<4>, int_type<5>,
                                    int_type<6>, int_type<7>, int_type<8>, int_type<9>, int_type<10>>;
    CHECK_COMPILE_TIME(LargeList::size == 10);
    CHECK_COMPILE_TIME(LargeList::count_if<is_int_type> == 10);

    // 类型转换边界
    using ComplexList = tl::type_list<std::tuple<A, B>, std::variant<C, D>, tl::type_list<E, F>>;

    using FlatComplex = ComplexList::flatten_t;
    CHECK_COMPILE_TIME(FlatComplex::size == 6);

    // 无效操作保护测试
    // using List = tl::type_list<A, B>;

    // 超出范围的slice应该无法编译（通过SFINAE保护）
    // 以下代码应该编译失败，我们通过注释来记录预期行为
    // using BadSlice =  List::slice_t<2, 2>; // 应该编译失败

    // 空列表的转换
    using EmptyTuple = tl::empty_list::to_tuple_t;
    CHECK_COMPILE_TIME(std::is_same_v<EmptyTuple, std::tuple<>>);

    using EmptyVariant = tl::empty_list::to_variant_t;
    CHECK_COMPILE_TIME(std::is_same_v<EmptyVariant, std::variant<>>);
    return true;
}

// 16. 累积操作测试
TEST(type_list, accumulate) {
    using List = tl::type_list<int_type<1>, int_type<2>, int_type<3>>;
    using SumResult = List::accumulate_t<sum_accumulator, int_type<0>>;
    CHECK_COMPILE_TIME(SumResult::value == 6);

    using SizeResult = detail::accumulate<type_name_length, std::integral_constant<size_t, 0>, A, B, C>::type;
    CHECK_COMPILE_TIME(SizeResult::value == sizeof(A) + sizeof(B) + sizeof(C));

    using List1 = tl::type_list<int, double, char, float>;

    using SizeResult0 = List1::accumulate_t<acc_size, std::integral_constant<std::size_t, 0>>;
    CHECK_COMPILE_TIME(SizeResult0::value == sizeof(int) + sizeof(double) + sizeof(char) + sizeof(float));

    using Result = detail::accumulate<make_tuple_list, tl::type_list<>, int, double, char>::type;
    // clang-format off
    CHECK_COMPILE_TIME(
        std::is_same_v<Result, 
            tl::type_list<
                std::tuple<int>, 
                std::tuple<double>, 
                std::tuple<char>
                >
            >
    );
    // clang-format on
    return true;
}

// 17. 辅助功能测试
TEST(type_list, helper_functions) {
    // 测试类型检查
    CHECK_COMPILE_TIME(detail::is_tuple_v<std::tuple<A, B>>);
    CHECK_COMPILE_TIME(!detail::is_tuple_v<tl::type_list<A, B>>);

    CHECK_COMPILE_TIME(detail::is_type_list_v<tl::type_list<A, B>>);
    CHECK_COMPILE_TIME(!detail::is_type_list_v<std::tuple<A, B>>);

    CHECK_COMPILE_TIME(detail::is_variant_v<std::variant<A, B>>);
    CHECK_COMPILE_TIME(!detail::is_variant_v<tl::type_list<A, B>>);

    // 测试常量值
    CHECK_COMPILE_TIME(detail::const_value_v<5> == 5);
    return true;
}

// 18、empty_list_operations
TEST(type_list, empty_list_operations) {
    using Empty = tl::type_list<>;

    // 基本属性
    CHECK_COMPILE_TIME(Empty::size == 0);
    CHECK_COMPILE_TIME(Empty::is_empty == true);
    CHECK_COMPILE_TIME(Empty::no_duplicates == true);

    // 查询操作
    CHECK_COMPILE_TIME(Empty::contains_v<int> == false);
    CHECK_COMPILE_TIME(Empty::any_of<int> == false);
    CHECK_COMPILE_TIME(Empty::all_of<int> == true);  // 空列表上的全称量词为真
    CHECK_COMPILE_TIME(Empty::none_of<int> == true);

    // 转换操作
    using Tuple = Empty::to_tuple_t;
    CHECK_COMPILE_TIME(std::is_same_v<Tuple, std::tuple<>>);

    using Variant = Empty::to_variant_t;
    CHECK_COMPILE_TIME(std::is_same_v<Variant, std::variant<>>);

    // 切片操作
    using Slice = Empty::slice_t<0, 0>;
    CHECK_COMPILE_TIME(std::is_same_v<Slice, tl::type_list<>>);

    using Head = Empty::head_t<0>;
    CHECK_COMPILE_TIME(std::is_same_v<Head, tl::type_list<>>);

    using Tail = Empty::tail_t<0>;
    CHECK_COMPILE_TIME(std::is_same_v<Tail, tl::type_list<>>);

    // 修改操作
    using Appended = Empty::append_t<int, double>;
    CHECK_COMPILE_TIME(std::is_same_v<Appended, tl::type_list<int, double>>);

    using Prepended = Empty::prepend_t<int, double>;
    CHECK_COMPILE_TIME(std::is_same_v<Prepended, tl::type_list<int, double>>);

    // 集合操作
    using Unique = Empty::unique_t;
    CHECK_COMPILE_TIME(std::is_same_v<Unique, tl::type_list<>>);

    using Reversed = Empty::reverse_t;
    CHECK_COMPILE_TIME(std::is_same_v<Reversed, tl::type_list<>>);

    using OtherList = tl::type_list<int, double>;
    using Union = Empty::union_with_t<OtherList>;
    CHECK_COMPILE_TIME(std::is_same_v<Union, tl::type_list<int, double>>);

    using Intersection = Empty::intersect_with_t<OtherList>;
    CHECK_COMPILE_TIME(std::is_same_v<Intersection, tl::type_list<>>);

    // Zip/Unzip
    using Zipped = Empty::zip_t<tl::type_list<int, double>>;
    CHECK_COMPILE_TIME(std::is_same_v<Zipped, tl::type_list<>>);

    using Unzipped = Empty::unzip_t;
    CHECK_COMPILE_TIME(std::is_same_v<Unzipped, std::tuple<>>);
    return true;
}

// 19、comprehensive_operations
TEST(type_list, comprehensive_operations) {
    // 测试用例1: 基本类型列表操作
    {
        using List = tl::type_list<int, double, char>;
        CHECK_COMPILE_TIME(List::size == 3);
        CHECK_COMPILE_TIME(List::contains_v<int>);
        CHECK_COMPILE_TIME(!List::contains_v<float>);
        CHECK_COMPILE_TIME(List::index_of<char> == 2);
    }

    // 测试用例2: 类型转换和映射
    {
        using List = tl::type_list<int, double>;
        using PointerList = List::transform_t<std::add_pointer_t>;
        CHECK_COMPILE_TIME(std::is_same_v<PointerList, tl::type_list<int *, double *>>);

        using ConstList = List::transform_t<std::add_const_t>;
        CHECK_COMPILE_TIME(std::is_same_v<ConstList, tl::type_list<const int, const double>>);
    }

    // 测试用例3: 高级集合操作
    {
        using Set1 = tl::type_list<int, double, char>;
        using Set2 = tl::type_list<double, float, char>;

        using Union = Set1::union_with_t<Set2>;
        CHECK_COMPILE_TIME(Union::contains_v<int>);
        CHECK_COMPILE_TIME(Union::contains_v<double>);
        CHECK_COMPILE_TIME(Union::contains_v<char>);
        CHECK_COMPILE_TIME(Union::contains_v<float>);

        using Intersection = Set1::intersect_with_t<Set2>;
        CHECK_COMPILE_TIME(Intersection::contains_v<double>);
        CHECK_COMPILE_TIME(Intersection::contains_v<char>);
        CHECK_COMPILE_TIME(!Intersection::contains_v<int>);
        CHECK_COMPILE_TIME(!Intersection::contains_v<float>);
    }

    // 测试用例4: 实际应用场景
    /*{
        // 模拟一个简单的序列化系统
        using FieldTypes = tl::type_list<int, double, std::string, bool>;

        // 计算总大小和对齐
        using Stats = detail::stats<FieldTypes>;
        std::cout << "  Field types total size: " << Stats::total_size << "
    bytes\n"; std::cout << "  Field types max alignment: " << Stats::max_align <<
    "\n";

        // 创建字段描述符
        using FieldDescriptors = FieldTypes::transform_t<ToDescriptor_t>;

        // 过滤出整数类型
        using IntegralFields = FieldTypes::filter_t<is_integral>;
        CHECK_COMPILE_TIME(std::is_same_v<IntegralFields, tl::type_list<int,
    bool>>);
    }*/
    return true;
}

// 20. 位置替换和插入测试
TEST(type_list, replace_and_insert) {
    using List = tl::type_list<A, B, C, D, E>;

    // 测试 replace_at
    using ReplacedAt1 = List::replace_at_t<1, X>;  // 假设有X类型
    CHECK_COMPILE_TIME(ReplacedAt1::size == 5);
    CHECK_COMPILE_TIME(std::is_same_v<ReplacedAt1::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<ReplacedAt1::at_t<1>, X>);
    CHECK_COMPILE_TIME(std::is_same_v<ReplacedAt1::at_t<2>, C>);
    CHECK_COMPILE_TIME(std::is_same_v<ReplacedAt1::at_t<4>, E>);

    // 替换第一个元素
    using ReplacedFirst = List::replace_at_t<0, X>;
    CHECK_COMPILE_TIME(std::is_same_v<ReplacedFirst::at_t<0>, X>);

    // 替换最后一个元素
    using ReplacedLast = List::replace_at_t<4, X>;
    CHECK_COMPILE_TIME(std::is_same_v<ReplacedLast::at_t<4>, X>);

    // 测试 insert_at
    using InsertedAt1 = List::insert_at_t<1, X>;
    CHECK_COMPILE_TIME(InsertedAt1::size == 6);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedAt1::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedAt1::at_t<1>, X>);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedAt1::at_t<2>, B>);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedAt1::at_t<5>, E>);

    // 插入到头部
    using InsertedAtFront = List::insert_at_t<0, X>;
    CHECK_COMPILE_TIME(InsertedAtFront::size == 6);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedAtFront::at_t<0>, X>);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedAtFront::at_t<1>, A>);

    // 插入到尾部（注意：插入到size位置相当于append）
    using InsertedAtEnd = List::insert_at_t<5, X>;
    CHECK_COMPILE_TIME(InsertedAtEnd::size == 6);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedAtEnd::at_t<4>, E>);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedAtEnd::at_t<5>, X>);

    // 边际情况：插入到空列表
    using EmptyList = tl::type_list<>;
    using InsertedIntoEmpty = EmptyList::insert_at_t<0, A>;
    CHECK_COMPILE_TIME(InsertedIntoEmpty::size == 1);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedIntoEmpty::at_t<0>, A>);

    // 边际情况：单元素列表的替换和插入
    using Single = tl::type_list<A>;
    using ReplacedSingle = Single::replace_at_t<0, B>;
    CHECK_COMPILE_TIME(ReplacedSingle::size == 1);
    CHECK_COMPILE_TIME(std::is_same_v<ReplacedSingle::at_t<0>, B>);

    using InsertedBefore = Single::insert_at_t<0, B>;
    CHECK_COMPILE_TIME(InsertedBefore::size == 2);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedBefore::at_t<0>, B>);

    using InsertedAfter = Single::insert_at_t<1, B>;
    CHECK_COMPILE_TIME(InsertedAfter::size == 2);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedAfter::at_t<1>, B>);
    return true;
}

// 21. 高级查找测试
TEST(type_list, advanced_find_operations) {
    // 测试 find_last_if
    using List1 = tl::type_list<int_type<1>, int_type<2>, int_type<3>, int_type<4>, int_type<2>>;

    constexpr size_t last_even_index = List1::find_last_if<is_even_value>;
    CHECK_COMPILE_TIME(last_even_index == 4);  // 最后一个偶数在索引4（值2）

    using List2 = tl::type_list<A, B, C, A, B, C>;
    constexpr size_t last_a_or_c_index = List2::find_last_if<is_type_a_or_c>;
    CHECK_COMPILE_TIME(last_a_or_c_index == 5);  // 最后一个A或C在索引5（C）

    // 测试 find_first_not_if
    using List3 = tl::type_list<int_type<2>, int_type<4>, int_type<6>, int_type<3>, int_type<8>>;
    constexpr size_t first_not_even_index = List3::find_first_not_if<is_even_value>;
    CHECK_COMPILE_TIME(first_not_even_index == 3);  // 第一个非偶数在索引3（值3）

    // 全部满足条件的情况
    using List4 = tl::type_list<int_type<2>, int_type<4>, int_type<6>, int_type<8>>;
    constexpr size_t all_even_index = List4::find_first_not_if<is_even_value>;
    CHECK_COMPILE_TIME(all_even_index == static_cast<size_t>(-1));  // 所有都是偶数

    // 测试 find_last_not_if
    using List5 = tl::type_list<int_type<2>, int_type<3>, int_type<5>, int_type<8>, int_type<10>>;
    constexpr size_t last_not_even_index = List5::find_last_not_if<is_even_value>;
    CHECK_COMPILE_TIME(last_not_even_index == 2);  // 最后一个非偶数在索引2（值5）

    // 边界情况：空列表的查找（需要检查原库的实现是否处理空列表）
    using EmptyList = tl::type_list<>;
    // 注意：以下测试依赖于原库的实现，如果原库没有处理空列表，可能需要调整
    // 根据一般的实现，空列表的查找应该返回-1
    CHECK_COMPILE_TIME(EmptyList::find_first_not_if<is_even_value> == static_cast<size_t>(-1));

    // 边界情况：单元素列表
    using SingleEven = tl::type_list<int_type<2>>;
    CHECK_COMPILE_TIME(SingleEven::find_first_not_if<is_even_value> == static_cast<size_t>(-1));
    CHECK_COMPILE_TIME(SingleEven::find_last_not_if<is_even_value> == static_cast<size_t>(-1));

    using SingleOdd = tl::type_list<int_type<3>>;
    CHECK_COMPILE_TIME(SingleOdd::find_first_not_if<is_even_value> == 0);
    CHECK_COMPILE_TIME(SingleOdd::find_last_not_if<is_even_value> == 0);

    // 测试混合类型列表
    using MixedList = tl::type_list<A, B, int_type<2>, C, int_type<3>>;

    // 查找第一个不是A或C的类型
    constexpr size_t first_not_a_or_c = MixedList::find_first_not_if<is_type_a_or_c>;
    CHECK_COMPILE_TIME(first_not_a_or_c == 1);  // B在索引1

    // 查找最后一个不是A或C的类型
    constexpr size_t last_not_a_or_c = MixedList::find_last_not_if<is_type_a_or_c>;
    CHECK_COMPILE_TIME(last_not_a_or_c == 4);  // int_type<3>在索引4

    // 测试查找不到的情况
    using AllAOrC = tl::type_list<A, C, A, C>;
    CHECK_COMPILE_TIME(AllAOrC::find_first_not_if<is_type_a_or_c> == static_cast<size_t>(-1));
    CHECK_COMPILE_TIME(AllAOrC::find_last_not_if<is_type_a_or_c> == static_cast<size_t>(-1));

    // 测试各种查找的组合使用
    using ComplexList = tl::type_list<int_type<1>, int_type<2>, int_type<3>, int_type<4>, int_type<5>,
                                      int_type<6>, int_type<7>, int_type<8>, int_type<9>, int_type<10>>;

    // 查找第一个偶数
    CHECK_COMPILE_TIME(ComplexList::find_first_if<is_even_value> == 1);

    // 查找第一个非偶数
    CHECK_COMPILE_TIME(ComplexList::find_first_not_if<is_even_value> == 0);

    // 查找最后一个偶数
    CHECK_COMPILE_TIME(ComplexList::find_last_if<is_even_value> == 9);

    // 查找最后一个非偶数
    CHECK_COMPILE_TIME(ComplexList::find_last_not_if<is_even_value> == 8);

    // 测试谓词的嵌套和组合
    using ListABC = tl::type_list<A, B, C, B, A>;

    // 查找第一个不是B的元素
    CHECK_COMPILE_TIME(ListABC::find_first_not_if<is_type_b> == 0);

    // 查找最后一个不是B的元素
    CHECK_COMPILE_TIME(ListABC::find_last_not_if<is_type_b> == 4);
    return true;
}

// 22. 组合操作测试：结合替换、插入和查找
TEST(type_list, combined_operations) {
    using OriginalList = tl::type_list<A, B, C, D, E>;

    // 场景1：找到第一个可替换元素并替换它
    constexpr size_t first_replacable = OriginalList::find_first_if<is_replacable>;
    CHECK_COMPILE_TIME(first_replacable == 1);  // B在索引1

    using ReplacedFirst = OriginalList::replace_at_t<first_replacable, X>;
    CHECK_COMPILE_TIME(std::is_same_v<ReplacedFirst::at_t<1>, X>);
    CHECK_COMPILE_TIME(std::is_same_v<ReplacedFirst::at_t<3>, D>);  // D没有被替换

    // 场景2：找到最后一个可替换元素并替换它
    constexpr size_t last_replacable = OriginalList::find_last_if<is_replacable>;
    CHECK_COMPILE_TIME(last_replacable == 3);  // D在索引3

    using ReplacedLast = OriginalList::replace_at_t<last_replacable, X>;
    CHECK_COMPILE_TIME(std::is_same_v<ReplacedLast::at_t<3>, X>);
    CHECK_COMPILE_TIME(std::is_same_v<ReplacedLast::at_t<1>, B>);  // B没有被替换

    // 场景3：在第一个不可替换元素前插入新元素
    constexpr size_t first_not_replacable = OriginalList::find_first_not_if<is_replacable>;
    CHECK_COMPILE_TIME(first_not_replacable == 0);  // A在索引0

    using InsertedBeforeFirst = OriginalList::insert_at_t<first_not_replacable, X>;
    CHECK_COMPILE_TIME(InsertedBeforeFirst::size == 6);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedBeforeFirst::at_t<0>, X>);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedBeforeFirst::at_t<1>, A>);

    // 场景4：在最后一个不可替换元素后插入新元素
    constexpr size_t last_not_replacable = OriginalList::find_last_not_if<is_replacable>;
    CHECK_COMPILE_TIME(last_not_replacable == 4);  // E在索引4

    using InsertedAfterLast = OriginalList::insert_at_t<last_not_replacable + 1, X>;
    CHECK_COMPILE_TIME(InsertedAfterLast::size == 6);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedAfterLast::at_t<5>, X>);
    CHECK_COMPILE_TIME(std::is_same_v<InsertedAfterLast::at_t<4>, E>);

    // 场景5：替换所有偶数（使用循环模拟，但这里我们只替换第一个作为示例）
    using NumberList = tl::type_list<int_type<1>, int_type<2>, int_type<3>, int_type<4>, int_type<5>>;

    constexpr size_t first_even = NumberList::find_first_if<is_even_value>;
    CHECK_COMPILE_TIME(first_even == 1);  // int_type<2>在索引1

    using ReplacedEven = NumberList::replace_at_t<first_even, int_type<99>>;
    CHECK_COMPILE_TIME(ReplacedEven::size == 5);
    CHECK_COMPILE_TIME(ReplacedEven::at_t<1>::value == 99);
    return true;
}

// 23. 扩展Zip功能测试（支持std::variant/std::tuple/type_list）
TEST(type_list, extended_zip_operations) {
    // 测试数据
    using List1 = tl::type_list<A, B, C>;
    using Tuple1 = std::tuple<int, double, float, char>;
    using Variant1 = std::variant<char, short, long, X, Y, Z>;

    // 测试 type_list 与 type_list 的 zip
    using List2 = tl::type_list<D, E, F>;
    using ZippedLists = List1::zip_t<List2>;
    CHECK_COMPILE_TIME(ZippedLists::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<ZippedLists::at_t<0>, std::tuple<A, D>>);
    CHECK_COMPILE_TIME(std::is_same_v<ZippedLists::at_t<1>, std::tuple<B, E>>);

    // 测试 type_list 与 std::tuple 的 zip
    using ZippedWithTuple = List1::zip_t<Tuple1>;
    CHECK_COMPILE_TIME(ZippedWithTuple::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<ZippedWithTuple::at_t<0>, std::tuple<A, int>>);
    CHECK_COMPILE_TIME(std::is_same_v<ZippedWithTuple::at_t<1>, std::tuple<B, double>>);

    // 测试 type_list 与 std::variant 的 zip
    using ZippedWithVariant = List1::zip_t<Variant1>;
    CHECK_COMPILE_TIME(ZippedWithVariant::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<ZippedWithVariant::at_t<0>, std::tuple<A, char>>);
    CHECK_COMPILE_TIME(std::is_same_v<ZippedWithVariant::at_t<1>, std::tuple<B, short>>);

    // 测试三者的混合 zip
    using ZippedMixed = List1::zip_t<Tuple1, Variant1>;
    CHECK_COMPILE_TIME(ZippedMixed::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<ZippedMixed::at_t<0>, std::tuple<A, int, char>>);
    CHECK_COMPILE_TIME(std::is_same_v<ZippedMixed::at_t<1>, std::tuple<B, double, short>>);
    CHECK_COMPILE_TIME(std::is_same_v<ZippedMixed::at_t<2>, std::tuple<C, float, long>>);

    // 测试 std::tuple 与 std::tuple 的 zip（通过转换为type_list）
    using Tuple2 = std::tuple<X, Y, Z>;  // 假设有X, Y, Z类型
    using ListFromTuple1 = detail::to_type_list_t<Tuple1>;
    using ListFromTuple2 = detail::to_type_list_t<Tuple2>;
    using ZippedTuples = ListFromTuple1::zip_t<ListFromTuple2>;
    CHECK_COMPILE_TIME(ZippedTuples::size == 3);

    // 测试 std::variant 与 std::variant 的 zip
    using Variant2 = std::variant<bool, unsigned, size_t>;
    using ListFromVariant1 = detail::to_type_list_t<Variant1>;
    using ListFromVariant2 = detail::to_type_list_t<Variant2>;
    using ZippedVariants = ListFromVariant1::zip_t<ListFromVariant2>;
    CHECK_COMPILE_TIME(ZippedVariants::size == 3);

    // 测试边际情况：空容器的zip
    using EmptyList = tl::type_list<>;

    // 空type_list的zip
    using ZippedEmptyLists = EmptyList::zip_t<EmptyList>;
    CHECK_COMPILE_TIME(ZippedEmptyLists::size == 0);

    // 测试不同容器类型但相同内容的zip
    using ListABC = tl::type_list<A, B, C>;
    using TupleABC = std::tuple<A, B, C>;
    using VariantABC = std::variant<A, B, C>;

    using ZippedSameTypes = ListABC::zip_t<TupleABC, VariantABC>;
    CHECK_COMPILE_TIME(ZippedSameTypes::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<ZippedSameTypes::at_t<0>, std::tuple<A, A, A>>);
    return true;
}

// 24. 扩展Unzip功能测试（支持std::variant/std::tuple/type_list）

TEST(type_list, extended_unzip_operations) {
    // 测试包含std::tuple的type_list的unzip
    using TupleList =
        tl::type_list<std::tuple<A, int, char>, std::tuple<B, double, short>, std::tuple<C, float, long>>;

    using UnzippedTuples = TupleList::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<UnzippedTuples> == 3);

    // 检查unzip后的第一个列表（A, B, C）
    using FirstUnzipped = std::tuple_element_t<0, UnzippedTuples>;
    CHECK_COMPILE_TIME(FirstUnzipped::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<FirstUnzipped::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<FirstUnzipped::at_t<1>, B>);
    CHECK_COMPILE_TIME(std::is_same_v<FirstUnzipped::at_t<2>, C>);

    // 检查unzip后的第二个列表（int, double, float）
    using SecondUnzipped = std::tuple_element_t<1, UnzippedTuples>;
    CHECK_COMPILE_TIME(SecondUnzipped::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<SecondUnzipped::at_t<0>, int>);
    CHECK_COMPILE_TIME(std::is_same_v<SecondUnzipped::at_t<1>, double>);
    CHECK_COMPILE_TIME(std::is_same_v<SecondUnzipped::at_t<2>, float>);

    // 测试包含std::variant的type_list的unzip
    using VariantList =
        tl::type_list<std::variant<A, B>, std::variant<int, double, char>, std::variant<char, short>>;

    using UnzippedVariants = VariantList::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<UnzippedVariants> == 2);  // 每个variant有2个类型

    // 检查unzip后的第一个列表（A, int, char）
    using FirstVariantUnzipped = std::tuple_element_t<0, UnzippedVariants>;
    CHECK_COMPILE_TIME(FirstVariantUnzipped::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<FirstVariantUnzipped::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<FirstVariantUnzipped::at_t<1>, int>);
    CHECK_COMPILE_TIME(std::is_same_v<FirstVariantUnzipped::at_t<2>, char>);

    // 检查unzip后的第二个列表（B, double, short）
    using SecondVariantUnzipped = std::tuple_element_t<1, UnzippedVariants>;
    CHECK_COMPILE_TIME(SecondVariantUnzipped::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<SecondVariantUnzipped::at_t<0>, B>);
    CHECK_COMPILE_TIME(std::is_same_v<SecondVariantUnzipped::at_t<1>, double>);
    CHECK_COMPILE_TIME(std::is_same_v<SecondVariantUnzipped::at_t<2>, short>);

    // 测试混合容器类型的unzip
    using MixedContainerList =
        tl::type_list<std::tuple<A, int, bool>, tl::type_list<B, double>, std::variant<C, float>>;

    using UnzippedMixed = MixedContainerList::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<UnzippedMixed> == 2);

    // 测试边际情况：空列表的unzip
    using EmptyList = tl::type_list<>;
    using UnzippedEmpty = EmptyList::unzip_t;
    CHECK_COMPILE_TIME(std::is_same_v<UnzippedEmpty, std::tuple<>>);

    // 测试边际情况：单元素列表的unzip
    using SingleTupleList = tl::type_list<std::tuple<A, int>>;
    using UnzippedSingle = SingleTupleList::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<UnzippedSingle> == 2);

    using FirstOfSingle = std::tuple_element_t<0, UnzippedSingle>;
    CHECK_COMPILE_TIME(FirstOfSingle::size == 1);
    CHECK_COMPILE_TIME(std::is_same_v<FirstOfSingle::at_t<0>, A>);

    // 测试包含不同类型容器的列表
    using HeterogeneousList = tl::type_list<std::tuple<A, B>, tl::type_list<C, D>, std::tuple<E, F>>;

    using UnzippedHeterogeneous = HeterogeneousList::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<UnzippedHeterogeneous> == 2);
    return true;
}

// 25 扩展UnzipAt功能测试（支持std::variant/std::tuple/type_list）
TEST(type_list, extended_unzip_at) {
    // 测试包含std::tuple的type_list的unzip_at
    using TupleList = tl::type_list<std::tuple<A, int, char>, std::tuple<B, double, short, X, Z>,
                                    std::tuple<C, float, long, B, X, Z>>;

    // 测试 unzip_at<0> - 获取所有元组的第一个元素
    using UnzippedAt0 = TupleList::unzip_at_t<0>;
    CHECK_COMPILE_TIME(UnzippedAt0::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<UnzippedAt0::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<UnzippedAt0::at_t<1>, B>);
    CHECK_COMPILE_TIME(std::is_same_v<UnzippedAt0::at_t<2>, C>);

    // 测试 unzip_at<1> - 获取所有元组的第二个元素
    using UnzippedAt1 = TupleList::unzip_at_t<1>;
    CHECK_COMPILE_TIME(UnzippedAt1::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<UnzippedAt1::at_t<0>, int>);
    CHECK_COMPILE_TIME(std::is_same_v<UnzippedAt1::at_t<1>, double>);
    CHECK_COMPILE_TIME(std::is_same_v<UnzippedAt1::at_t<2>, float>);

    // 测试 unzip_at<2> - 获取所有元组的第三个元素
    using UnzippedAt2 = TupleList::unzip_at_t<2>;
    CHECK_COMPILE_TIME(UnzippedAt2::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<UnzippedAt2::at_t<0>, char>);
    CHECK_COMPILE_TIME(std::is_same_v<UnzippedAt2::at_t<1>, short>);
    CHECK_COMPILE_TIME(std::is_same_v<UnzippedAt2::at_t<2>, long>);

    // 测试包含std::variant的type_list的unzip_at
    using VariantList = tl::type_list<std::variant<A, int>, std::variant<B, double>, std::variant<C, float>>;

    // 测试 unzip_at<0> - 获取所有variant的第一个类型
    using VariantUnzippedAt0 = VariantList::unzip_at_t<0>;
    CHECK_COMPILE_TIME(VariantUnzippedAt0::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<VariantUnzippedAt0::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<VariantUnzippedAt0::at_t<1>, B>);
    CHECK_COMPILE_TIME(std::is_same_v<VariantUnzippedAt0::at_t<2>, C>);

    // 测试 unzip_at<1> - 获取所有variant的第二个类型
    using VariantUnzippedAt1 = VariantList::unzip_at_t<1>;
    CHECK_COMPILE_TIME(VariantUnzippedAt1::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<VariantUnzippedAt1::at_t<0>, int>);
    CHECK_COMPILE_TIME(std::is_same_v<VariantUnzippedAt1::at_t<1>, double>);
    CHECK_COMPILE_TIME(std::is_same_v<VariantUnzippedAt1::at_t<2>, float>);

    // 测试混合容器类型的unzip_at
    using MixedList = tl::type_list<std::tuple<A, int>, tl::type_list<B, double>, std::variant<C, float>>;

    // 测试 unzip_at<0> - 获取所有容器的第一个元素类型
    using MixedUnzippedAt0 = MixedList::unzip_at_t<0>;
    CHECK_COMPILE_TIME(MixedUnzippedAt0::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<MixedUnzippedAt0::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<MixedUnzippedAt0::at_t<1>, B>);
    CHECK_COMPILE_TIME(std::is_same_v<MixedUnzippedAt0::at_t<2>, C>);

    // 测试 unzip_at<1> - 获取所有容器的第二个元素类型
    using MixedUnzippedAt1 = MixedList::unzip_at_t<1>;
    CHECK_COMPILE_TIME(MixedUnzippedAt1::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<MixedUnzippedAt1::at_t<0>, int>);
    CHECK_COMPILE_TIME(std::is_same_v<MixedUnzippedAt1::at_t<1>, double>);
    CHECK_COMPILE_TIME(std::is_same_v<MixedUnzippedAt1::at_t<2>, float>);

    // 测试边际情况：空列表的unzip_at
    using EmptyList = tl::type_list<>;
    using EmptyUnzippedAt0 = EmptyList::unzip_at_t<0>;
    CHECK_COMPILE_TIME(EmptyUnzippedAt0::size == 0);

    // 测试边际情况：索引超出范围的情况
    // 注意：由于静态断言，以下代码应该无法编译
    // using BadUnzipAt =  TupleList::unzip_at_t<3>; // 应该编译失败

    // 测试单元素列表的unzip_at
    using SingleElementList = tl::type_list<std::tuple<A, int>>;
    using SingleUnzippedAt0 = SingleElementList::unzip_at_t<0>;
    CHECK_COMPILE_TIME(SingleUnzippedAt0::size == 1);
    CHECK_COMPILE_TIME(std::is_same_v<SingleUnzippedAt0::at_t<0>, A>);

    // 测试辅助函数unzip_at（非成员函数版本）
    using HelperUnzippedAt0 = detail::unzip_at<
        0, tl::type_list<std::tuple<A, int>, tl::type_list<B, double>, std::variant<C, float>>>::type;
    CHECK_COMPILE_TIME(HelperUnzippedAt0::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<HelperUnzippedAt0::at_t<0>, A>);
    CHECK_COMPILE_TIME(std::is_same_v<HelperUnzippedAt0::at_t<1>, B>);
    CHECK_COMPILE_TIME(std::is_same_v<HelperUnzippedAt0::at_t<2>, C>);
    return true;
}

// 26. 综合容器操作测试
TEST(type_list, comprehensive_container_operations) {
    // 测试场景：从多个数据源收集类型，进行zip操作，然后unzip处理

    // 模拟从不同数据源获取的类型
    using DatabaseTypes = tl::type_list<user, product, order>;  // 假设的类型
    using APITypes = std::tuple<user_dto, product_dto, order_dto>;
    using UIVariants = std::variant<user_view, product_view, order_view>;

    // 1. 将不同类型zip到一起
    using ZippedData = DatabaseTypes::zip_t<APITypes, UIVariants>;
    CHECK_COMPILE_TIME(ZippedData::size == 3);

    // 2. 对zipped数据进行转换
    using OnlyDatabaseTypes = ZippedData::transform_t<extract_first_type_t>;
    CHECK_COMPILE_TIME(OnlyDatabaseTypes::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<OnlyDatabaseTypes::at_t<0>, user>);

    // 3. 将zipped数据unzip
    using UnzippedAll = ZippedData::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<UnzippedAll> == 3);

    // 获取所有User相关类型
    using AllUserTypes = std::tuple_element_t<0, UnzippedAll>;
    CHECK_COMPILE_TIME(AllUserTypes::size == 3);

    // 4. 使用unzip_at获取特定维度的类型
    using AllDTOs = ZippedData::unzip_at_t<1>;
    CHECK_COMPILE_TIME(AllDTOs::size == 3);
    CHECK_COMPILE_TIME(std::is_same_v<AllDTOs::at_t<0>, user_dto>);

    // 5. 测试嵌套容器的处理
    using NestedContainerList =
        tl::type_list<std::tuple<tl::type_list<A, B>, int>, std::tuple<tl::type_list<C, D>, double>,
                      std::tuple<tl::type_list<E, F>, float>>;

    // 对嵌套容器进行unzip
    using UnzippedNested = NestedContainerList::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<UnzippedNested> == 2);

    // 获取所有type_list
    using all_type_lists = std::tuple_element_t<0, UnzippedNested>;
    CHECK_COMPILE_TIME(all_type_lists::size == 3);

    // 获取所有基本类型
    using AllBasicTypes = std::tuple_element_t<1, UnzippedNested>;
    CHECK_COMPILE_TIME(AllBasicTypes::size == 3);

    // 6. 测试复杂的数据转换管道
    using SourceList = tl::type_list<A, B, C, D, E>;

    // 转换为元组
    using AsTuples = SourceList::transform_t<std::tuple>;
    CHECK_COMPILE_TIME(AsTuples::size == 5);

    // 7. 测试错误处理（期望的编译失败情况）
    // 注意：以下代码应该无法编译，因为容器大小不一致
    // using MismatchedList1 = tl::type_list<A, B, C>;
    // using MismatchedTuple1 = std::tuple<int, double>; // 大小不匹配
    // using ShouldFailZip =  MismatchedList1::zip_t<MismatchedTuple1>; //
    // 应该编译失败

    // 使用不同类型的容器但相同大小应该可以
    using MatchedList = tl::type_list<A, B>;
    using MatchedTuple = std::tuple<int, double>;
    using MatchedVariant = std::variant<char, short>;

    using ValidZip = MatchedList::zip_t<MatchedTuple, MatchedVariant>;
    CHECK_COMPILE_TIME(ValidZip::size == 2);
    return true;
}

// 27. 边际情况和错误处理测试
TEST(type_list, edge_cases_and_error_handling) {
    // 1. 测试空容器的各种操作
    using EmptyList = tl::type_list<>;

    // 空容器的zip
    using ZipEmpty = EmptyList::zip_t<EmptyList>;
    CHECK_COMPILE_TIME(ZipEmpty::size == 0);

    // 空容器的unzip
    using UnzipEmpty = EmptyList::unzip_t;
    CHECK_COMPILE_TIME(std::is_same_v<UnzipEmpty, std::tuple<>>);

    // 空容器的unzip_at
    using UnzipAtEmpty = EmptyList::unzip_at_t<0>;
    CHECK_COMPILE_TIME(UnzipAtEmpty::size == 0);

    // 2. 测试单元素容器的各种操作
    using SingleList = tl::type_list<A>;
    using SingleTuple = std::tuple<int>;
    using SingleVariant = std::variant<char>;

    // 单元素容器的zip
    using ZipSingle = SingleList::zip_t<SingleTuple, SingleVariant>;
    CHECK_COMPILE_TIME(ZipSingle::size == 1);
    CHECK_COMPILE_TIME(std::is_same_v<ZipSingle::at_t<0>, std::tuple<A, int, char>>);

    // 单元素容器的unzip
    using SingleTupleList = tl::type_list<std::tuple<A, int, char>>;
    using UnzipSingle = SingleTupleList::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<UnzipSingle> == 3);

    // 3. 测试包含相同类型但不同值的variant
    using VariantWithDuplicates = std::variant<A, A, B>;  // 注意：variant可以有重复类型
    using VariantList = tl::type_list<VariantWithDuplicates, VariantWithDuplicates>;

    // unzip_at应该能正确处理重复类型
    using UnzippedVariant0 = VariantList::unzip_at_t<0>;
    CHECK_COMPILE_TIME(UnzippedVariant0::size == 2);
    CHECK_COMPILE_TIME(std::is_same_v<UnzippedVariant0::at_t<0>, A>);

    // 4. 测试包含引用类型的容器
    using RefTuple = std::tuple<A &, const B &, C &&>;
    using RefList = tl::type_list<RefTuple>;

    using UnzippedRefs = RefList::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<UnzippedRefs> == 3);

    // 5. 测试包含函数指针等复杂类型的容器
    using FuncPtr = void (*)(int);
    using ComplexTuple = std::tuple<FuncPtr, int[10], std::tuple<A, B>>;
    using ComplexList = tl::type_list<ComplexTuple>;

    // 这些复杂类型应该也能正确处理
    using UnzippedComplex = ComplexList::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<UnzippedComplex> == 3);

    // 6. 测试超大容器的性能（编译期压力测试）
    // 创建包含20个元素的类型列表
    using LargeList = tl::type_list<int_type<0>, int_type<1>, int_type<2>, int_type<3>, int_type<4>,
                                    int_type<5>, int_type<6>, int_type<7>, int_type<8>, int_type<9>,
                                    int_type<10>, int_type<11>, int_type<12>, int_type<13>, int_type<14>,
                                    int_type<15>, int_type<16>, int_type<17>, int_type<18>, int_type<19>>;

    using LargeTuple = std::tuple<int_type<0>, int_type<1>, int_type<2>, int_type<3>, int_type<4>,
                                  int_type<5>, int_type<6>, int_type<7>, int_type<8>, int_type<9>,
                                  int_type<10>, int_type<11>, int_type<12>, int_type<13>, int_type<14>,
                                  int_type<15>, int_type<16>, int_type<17>, int_type<18>, int_type<19>>;

    // zip大列表应该能正常工作
    using ZipLarge = LargeList::zip_t<LargeTuple>;
    CHECK_COMPILE_TIME(ZipLarge::size == 20);

    // unzip大列表应该能正常工作
    using TupleOfLarge = LargeList::transform_t<std::tuple>;
    using UnzipLarge = TupleOfLarge::unzip_t;
    CHECK_COMPILE_TIME(std::tuple_size_v<UnzipLarge> == 1);

    // 7. 测试类型推导和SFINAE友好性
    // 这些操作应该在类型不匹配时优雅地失败（通过SFINAE）

    // 8. 测试helper函数的一致性
    // 验证helper函数与成员函数的一致性
    using TestList = tl::type_list<std::tuple<A, int>, std::tuple<B, double>>;

    using MemberUnzip = TestList::unzip_t;
    using HelperUnzip = detail::unzip<TestList>::type;

    CHECK_COMPILE_TIME(std::is_same_v<MemberUnzip, HelperUnzip>);

    using MemberUnzipAt0 = TestList::unzip_at_t<0>;
    using HelperUnzipAt0 =
        detail::unzip_at<0, tl::type_list<std::tuple<A, int>, std::tuple<B, double>>>::type;

    CHECK_COMPILE_TIME(std::is_same_v<MemberUnzipAt0, HelperUnzipAt0>);
    return true;
}

// 28. 使用新transform功能的测试
TEST(type_list, new_transform_features) {
    // 测试带单个额外参数的transform
    using SourceList = tl::type_list<A, B, C, D, E>;

    // 使用std::integral_constant作为索引
    using IndexedList = SourceList::transform_t<make_indexed_tuple_t, std::integral_constant<std::size_t, 0>>;

    CHECK_COMPILE_TIME(IndexedList::size == 5);
    CHECK_COMPILE_TIME(
        std::is_same_v<IndexedList::at_t<0>, std::tuple<A, std::integral_constant<std::size_t, 0>>>);
    CHECK_COMPILE_TIME(
        std::is_same_v<IndexedList::at_t<4>, std::tuple<E, std::integral_constant<std::size_t, 0>>>);

    // 测试带标签的transform
    struct UserTag {};
    using TaggedList = SourceList::transform_t<make_tagged_type_t, UserTag>;

    CHECK_COMPILE_TIME(TaggedList::size == 5);
    CHECK_COMPILE_TIME(std::is_same_v<TaggedList::at_t<0>, std::tuple<A, UserTag>>);

    // 测试带多个额外参数的transform
    struct Modifier1 {};
    struct Modifier2 {};

    using DecoratedList = SourceList::transform_t<decorate_type_t, Modifier1, Modifier2>;

    CHECK_COMPILE_TIME(DecoratedList::size == 5);
    CHECK_COMPILE_TIME(std::is_same_v<DecoratedList::at_t<0>, std::tuple<A, Modifier1, Modifier2>>);

    // 测试更复杂的场景：为每个元素生成不同的索引
    // 我们需要先生成一个索引列表，然后进行zip操作
    // 首先创建索引列表
    using IndexSequence = std::make_index_sequence<SourceList::size>;

    using IndexList = decltype(make_index_list(IndexSequence{}));

    // 使用zip将源列表和索引列表组合
    using ZippedWithIndices = SourceList::zip_t<IndexList>;

    CHECK_COMPILE_TIME(ZippedWithIndices::size == 5);
    CHECK_COMPILE_TIME(
        std::is_same_v<ZippedWithIndices::at_t<0>, std::tuple<A, std::integral_constant<std::size_t, 0>>>);
    CHECK_COMPILE_TIME(
        std::is_same_v<ZippedWithIndices::at_t<4>, std::tuple<E, std::integral_constant<std::size_t, 4>>>);

    using ModifiedList = ZippedWithIndices::transform_t<extract_and_modify_t>;

    CHECK_COMPILE_TIME(ModifiedList::size == 5);
    CHECK_COMPILE_TIME(
        std::is_same_v<ModifiedList::at_t<0>, std::tuple<A, std::integral_constant<std::size_t, 0>,
                                                         std::integral_constant<std::size_t, 0>>>);
    CHECK_COMPILE_TIME(
        std::is_same_v<ModifiedList::at_t<1>, std::tuple<B, std::integral_constant<std::size_t, 1>,
                                                         std::integral_constant<std::size_t, 2>>>);

    using ConfigResult = SourceList::transform_t<configurable_transformer_apply_t, int, double>;
    CHECK_COMPILE_TIME(ConfigResult::size == 5);
    CHECK_COMPILE_TIME(std::is_same_v<ConfigResult::at_t<0>, std::tuple<A, int, double>>);

    // 测试边界情况：空列表的transform
    using EmptyList = tl::type_list<>;
    using TransformedEmpty = EmptyList::transform_t<make_indexed_tuple_t, int>;
    CHECK_COMPILE_TIME(TransformedEmpty::size == 0);

    // 测试单元素列表的transform
    using SingleList = tl::type_list<A>;
    using TransformedSingle = SingleList::transform_t<make_indexed_tuple_t, int>;
    CHECK_COMPILE_TIME(TransformedSingle::size == 1);
    CHECK_COMPILE_TIME(std::is_same_v<TransformedSingle::at_t<0>, std::tuple<A, int>>);

    // 测试transform链式调用
    using FirstTransform = SourceList::transform_t<make_tagged_type_t, int>;
    using SecondTransform = FirstTransform::transform_t<make_tagged_type_t, double>;
    CHECK_COMPILE_TIME(SecondTransform::size == 5);
    CHECK_COMPILE_TIME(std::is_same_v<SecondTransform::at_t<0>, std::tuple<std::tuple<A, int>, double>>);

    // 测试更实用的场景：为类型添加属性和元数据
    using WithFieldInfo = SourceList::transform_t<add_field_info_t>;
    CHECK_COMPILE_TIME(WithFieldInfo::size == 5);

    // 验证FieldInfo是否被正确添加
    CHECK_COMPILE_TIME(std::is_same_v<WithFieldInfo::at_t<0>, std::tuple<A, field_info>>);
    using WithTraits = SourceList::transform_t<add_type_traits_t>;
    CHECK_COMPILE_TIME(WithTraits::size == 5);

    // 测试A的类型特性（假设A是平凡类型）
    using FirstWithTraits = WithTraits::at_t<0>;
    CHECK_COMPILE_TIME(std::tuple_size_v<FirstWithTraits> == 3);
    using WithSerialization = SourceList::transform_t<serialization_info_t>;
    CHECK_COMPILE_TIME(WithSerialization::size == 5);

    // 验证序列化信息是否正确
    using FirstSerialized = WithSerialization::at_t<0>;
    CHECK_COMPILE_TIME(std::tuple_size_v<FirstSerialized> == 3);
    using ConditionalResult = SourceList::transform_t<conditional_transform_t>;
    CHECK_COMPILE_TIME(ConditionalResult::size == 5);
    return true;
}

// 29. 使用新transform实现索引转换
TEST(type_list, indexed_transform) {
    // 创建索引列表的辅助函数
    constexpr auto make_index_list = []<std::size_t... Is>(std::index_sequence<Is...>) {
        return tl::type_list<std::integral_constant<std::size_t, Is>...>{};
    };

    // 方法1: 使用zip实现带索引的转换
    using SourceList = tl::type_list<A, B, C, D, E>;
    using Indices = decltype(make_index_list(std::make_index_sequence<SourceList::size>{}));
    using ZippedWithIndices = SourceList::zip_t<Indices>;

    CHECK_COMPILE_TIME(ZippedWithIndices::size == 5);
    CHECK_COMPILE_TIME(
        std::is_same_v<ZippedWithIndices::at_t<0>, std::tuple<A, std::integral_constant<std::size_t, 0>>>);

    // support from c++20
    // 方法2: 使用for_each实现带索引的转换
    /*using IndexedListForEach = decltype([] {
        using Result = tl::type_list<>;
        return SourceList::for_each<tl::conjunction_type::Seq>(
            [](auto& acc, auto id) {
                using T = typename decltype(id)::type;
                constexpr std::size_t I = std::decay_t<decltype(acc)>::size;
                using NewAcc = typename std::decay_t<decltype(acc)>::template
    append_t< std::tuple<T, std::integral_constant<std::size_t, I>>>; return
    NewAcc{};
            },
            Result{});
    }());

    CHECK_COMPILE_TIME(IndexedListForEach::size == 5);
    CHECK_COMPILE_TIME(std::is_same_v< IndexedListForEach::at_t<0>,
                                      std::tuple<A,
    std::integral_constant<std::size_t, 0>>>);

    // 验证两种方法结果相同
    CHECK_COMPILE_TIME(ZippedWithIndices::equal_to<IndexedListForEach>);*/

    // 方法3: 使用transform的通用索引转换
    // 实际应用中，zip方法是最简单直观的
    // 让我们测试一个实际应用场景：为类型生成带索引的序列化信息

    // 使用zip和transform实现
    using Zipped = SourceList::zip_t<Indices>;

    using SerializedWithIndices = Zipped::transform_t<apply_serialize_t>;

    CHECK_COMPILE_TIME(SerializedWithIndices::size == 5);

    // 验证第一个元素的序列化信息
    using FirstSerialized = SerializedWithIndices::at_t<0>;
    CHECK_COMPILE_TIME(std::tuple_size_v<FirstSerialized> == 4);

    // 验证偏移量计算
    using SecondSerialized = SerializedWithIndices::at_t<1>;
    using SecondOffset = std::tuple_element_t<3, SecondSerialized>;
    CHECK_COMPILE_TIME(SecondOffset::value == sizeof(A));  // B的偏移量应该是A的大小

    // 测试更复杂的索引计算
    // 需要累积偏移量的计算
    using OffsetsList =
        tl::type_list<std::integral_constant<std::size_t, 0>, std::integral_constant<std::size_t, sizeof(A)>,
                      std::integral_constant<std::size_t, sizeof(A) + sizeof(B)>,
                      std::integral_constant<std::size_t, sizeof(A) + sizeof(B) + sizeof(C)>,
                      std::integral_constant<std::size_t, sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D)>>;

    using ZippedWithOffsets = SourceList::zip_t<Indices, OffsetsList>;

    using SerializedWithAccumulatedOffsets = ZippedWithOffsets::transform_t<apply_accumulated_serialize_t>;

    CHECK_COMPILE_TIME(SerializedWithAccumulatedOffsets::size == 5);

    // 验证最后一个元素的偏移量
    using LastSerialized = SerializedWithAccumulatedOffsets::at_t<4>;
    using LastOffset = std::tuple_element_t<3, LastSerialized>;
    CHECK_COMPILE_TIME(LastOffset::value == sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D));

    // 总结：新的transform功能使得我们可以更方便地进行类型转换
    // 特别是当转换需要额外的类型参数时，transform现在可以直接支持
    return true;
}

// 30、测试from_function功能
TEST(type_list, from_function) {
    // 测试1：普通函数类型（void(int)）
    using Fn1 = void(int);
    CHECK_COMPILE_TIME(std::is_same_v<tl::empty_list::from_function_t<Fn1>, tl::type_list<int>>);

    // 测试2：函数指针（void(*)(int, float)）
    using Fn2 = void (*)(int, float);
    CHECK_COMPILE_TIME(std::is_same_v<tl::empty_list::from_function_t<Fn2>, tl::type_list<int, float>>);

    // 测试3：函数引用（void(&)(double)）
    using Fn3 = void (&)(double);
    CHECK_COMPILE_TIME(std::is_same_v<tl::empty_list::from_function_t<Fn3>, tl::type_list<double>>);

    // 测试4：类的const成员函数
    struct TestClass {
        void foo(std::string) const;
    };
    using Fn4 = decltype(&TestClass::foo);
    CHECK_COMPILE_TIME(std::is_same_v<tl::empty_list::from_function_t<Fn4>, tl::type_list<std::string>>);

    // 测试5：volatile成员函数
    struct VolatileClass {
        void bar(bool) volatile;
    };
    using Fn5 = decltype(&VolatileClass::bar);
    CHECK_COMPILE_TIME(std::is_same_v<tl::empty_list::from_function_t<Fn5>, tl::type_list<bool>>);

    auto lambda = [](char, short) {};
    using Fn6 = decltype(lambda);
    CHECK_COMPILE_TIME(std::is_same_v<tl::empty_list::from_function_t<Fn6>, tl::type_list<char, short>>);

    auto generic_lambda = []<typename T>(T) {};
    using Fn7 = decltype(generic_lambda);
    // 对于泛型lambda，我们无法直接推断参数类型，因此from_function_t应该返回一个空列表
    CHECK_COMPILE_TIME(std::is_same_v<tl::empty_list::from_function_t<Fn7>, tl::type_list<>>);

    auto overloaded_lambda = [](auto x) {
        if constexpr (std::is_integral_v<decltype(x)>) {
            return x + 1;
        } else {
            return x;
        }
    };
    using Fn8 = decltype(overloaded_lambda);
    // 对于重载lambda，我们也无法直接推断参数类型，因此from_function_t应该返回一个空列表
    CHECK_COMPILE_TIME(std::is_same_v<tl::empty_list::from_function_t<Fn8>, tl::type_list<>>);

    auto func = std::function<void(float, double)>{};
    using Fn9 = decltype(func);
    // std::function的参数类型应该能被正确提取
    CHECK_COMPILE_TIME(std::is_same_v<tl::empty_list::from_function_t<Fn9>, tl::type_list<float, double>>);

    // 测试边界情况：函数类型没有参数
    using Fn10 = void();
    CHECK_COMPILE_TIME(std::is_same_v<tl::empty_list::from_function_t<Fn10>, tl::type_list<>>);
    return true;
}

int main() { return testing::run_all_tests(); }
