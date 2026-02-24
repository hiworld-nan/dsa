#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace meta {

// ============================================================
// 基础：type_list
// ============================================================
template <class... Ts>
struct type_list {
    using type = type_list;
};

// ============================================================
// constexpr string hash（FNV-1a）
// ============================================================
constexpr std::uint64_t fnv1a(const char* s, std::uint64_t h = 14695981039346656037ull) {
    return *s ? fnv1a(s + 1, (h ^ std::uint64_t(*s)) * 1099511628211ull) : h;
}

// ============================================================
// string_key（类型级字符串）
// ============================================================
template <std::uint64_t H, const char* Str>
struct string_key {
    static constexpr std::uint64_t hash = H;
    static constexpr const char* name = Str;
};

// ============================================================
// type_pair / type_map
// ============================================================
template <class Key, class Value>
struct type_pair {
    using key = Key;
    using value = Value;
};

template <class... Pairs>
struct type_map {
    using type = type_map;
};

// contains_key
template <class Map, class Key>
struct contains_key;

template <class Key>
struct contains_key<type_map<>, Key> : std::false_type {};

template <class Head, class... Tail, class Key>
struct contains_key<type_map<Head, Tail...>, Key>
    : std::conditional<std::is_same<typename Head::key, Key>::value, std::true_type,
                       contains_key<type_map<Tail...>, Key>>::type {};

// get<Key>
template <class Map, class Key>
struct get;

template <class Key>
struct get<type_map<>, Key> {
    static_assert(!std::is_same<Key, Key>::value, "meta::get — key not found");
};

template <class Head, class... Tail, class Key>
struct get<type_map<Head, Tail...>, Key> {
    using type = typename std::conditional<std::is_same<typename Head::key, Key>::value, typename Head::value,
                                           typename get<type_map<Tail...>, Key>::type>::type;
};

// ============================================================
// member 描述（核心反射单元）
// ============================================================
template <class Struct, class T, T Struct::*Ptr, class Name>
struct member {
    using struct_type = Struct;
    using value_type = T;
    using name = Name;

    static constexpr T Struct::*pointer = Ptr;
};

// ============================================================
// member_list
// ============================================================
template <class... Members>
struct member_list {
    using type = type_list<Members...>;
};

// ============================================================
// member_map（name → member）
// ============================================================
template <class... Members>
struct member_map {
    using type = type_map<type_pair<typename Members::name, Members>...>;
};

// ============================================================
// reflect<T> 主入口
// ============================================================
template <class T>
struct reflect;  // 未注册直接编译失败

// ============================================================
// for_each_member
// ============================================================
template <class List>
struct for_each_member_impl;

template <class... Members>
struct for_each_member_impl<type_list<Members...>> {
    template <typename Obj, typename F>
    static void apply(Obj& obj, F&& f) {
        (void)std::initializer_list<int>{(f(Members{}, obj.*(Members::pointer)), 0)...};
    }
};

template <class Obj, class F>
void for_each_member(Obj& obj, F&& f) {
    using members = typename reflect<Obj>::members::type;
    for_each_member_impl<members>::apply(obj, std::forward<F>(f));
}

// ============================================================
// visitor
// ============================================================
template <class... Fs>
struct visitor : Fs... {
    using Fs::operator()...;
};

template <class... Fs>
visitor<Fs...> make_visitor(Fs... fs) {
    return {fs...};
}

template <class Obj, class Visitor>
void visit(Obj& obj, Visitor&& v) {
    for_each_member(obj, [&](auto member, auto& value) { v(member, value); });
}

// ============================================================
// 工具：member_name
// ============================================================
template <class Member>
constexpr const char* member_name() {
    return Member::name::name;
}

}  // namespace meta

// ============================================================
// 宏区（工业级、最小侵入）
// ============================================================

// 声明 name key（必须是全局静态）
#define META_DECLARE_NAME(NAME) static constexpr char NAME##_str[] = #NAME;

// 单字段
#define META_MEMBER(STRUCT, FIELD)                                \
    meta::member<STRUCT, decltype(STRUCT::FIELD), &STRUCT::FIELD, \
                 meta::string_key<meta::fnv1a(#FIELD), FIELD##_str>>

// struct 反射绑定
#define META_REFLECT(STRUCT, ...)                            \
    namespace meta {                                         \
    template <>                                              \
    struct reflect<STRUCT> {                                 \
        using members = member_list<__VA_ARGS__>;            \
        using type = typename member_map<__VA_ARGS__>::type; \
    };                                                       \
    }

#ifdef UTEST

#include <iostream>
#include <string>

#include "reflect.h"

struct Person {
    int id;
    std::string name;
    int age;
};

META_DECLARE_NAME(id)
META_DECLARE_NAME(name)
META_DECLARE_NAME(age)

META_REFLECT(Person, META_MEMBER(Person, id), META_MEMBER(Person, name), META_MEMBER(Person, age))

int main() {
    Person p{1, "Alice", 18};

    meta::for_each_member(p, [](auto member, auto& value) {
        std::cout << meta::member_name<decltype(member)>() << " = " << value << "\n";
    });

    meta::visit(p, meta::make_visitor([](auto, int& v) { v += 1; }, [](auto, std::string& s) { s += "!"; }));
}

#endif
