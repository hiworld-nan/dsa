#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace common {

namespace macros {
// uncomment to disable assert()
#define NDEBUG

#if defined(__x86_64__) || defined(_M_X64)
#define ARCH_X86_64 1
#else
#define ARCH_X86_64 0
#endif

#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif

#ifndef TOSTRING
#define TOSTRING(x) STRINGIFY(x)
#endif

#ifndef container_of
#define container_of(ptr, type, member)                                                       \
    ({                                                                                        \
        static_assert(std::is_standard_layout<type>::value, "Type must be standard layout!"); \
        const typeof(((type *)0)->member) *__mptr = (ptr);                                    \
        (type *)((char *)__mptr - offsetof(type, member));                                    \
    })

#endif

#define DONT_OPTIMIZE(x) asm volatile("" : : "r,m"(x) : "memory")

// 编译器内置函数 (GCC/Clang)
#define PREFETCH(addr, rw, locality) __builtin_prefetch(addr, rw, locality)

// OpenMP 辅助宏 - 在没有 OpenMP 时自动忽略
#ifdef _OPENMP
#define CRAFT_OMP(x) _Pragma(#x)
#else
#define CRAFT_OMP(x)
#endif

}  // namespace macros
}  // namespace common