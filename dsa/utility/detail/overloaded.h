/**
 * @file overloaded.h
 * @brief 重载函数对象聚合器
 * @version 1.0.0
 *
 * 提供 overloaded 工具，将多个可调用对象聚合为一个重载集，
 * 常与 std::variant + std::visit 配合使用。
 */

#pragma once

namespace utils {

// =============================================================================
// 重载函数对象聚合器
// =============================================================================

/// 将多个可调用对象聚合为一个重载集
///
/// 使用方式：
///   auto visitor = overloaded{
///       [](int i) { return i * 2; },
///       [](double d) { return d * 3.0; },
///       [](const std::string& s) { return s + "!"; }
///   };
///
///   std::variant<int, double, std::string> v = 42;
///   std::visit(visitor, v);  // 调用 int 重载
///
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

/// CTAD 推导引导（C++17）
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace utils
