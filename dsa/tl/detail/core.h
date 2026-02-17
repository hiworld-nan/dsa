#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>

namespace tl {

enum class conjunction_type : uint8_t { And = 0, Or = 1, Seq = 2 };

template <class... Ts>
struct type_list;

using empty_list = type_list<>;

}  // namespace tl