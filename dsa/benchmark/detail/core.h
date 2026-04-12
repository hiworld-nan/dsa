/**
 * @file core.h
 * @brief 核心定义 - x86 Linux 专用
 * @version 1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <new>

#include "../../common/constants.h"
#include "../../common/intrinsics.h"
#include "../../common/macros.h"
#include "../../common/singleton.h"
#include "../../common/tsc_clock.h"

namespace benchmark {
// 类型
using NanoSeconds = std::int64_t;
using IterationCount = std::size_t;

[[gnu::always_inline, gnu::hot, gnu::nonnull(1)]]
inline void do_not_optimize(void* x) noexcept {
    asm volatile("" : : "r,m"(x) : "memory");
}

}  // namespace benchmark
