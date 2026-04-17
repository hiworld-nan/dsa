/**
 * @file core.h
 * @brief 通用工具库核心定义
 * @version 1.0.0
 *
 * 提供编译期工具、类型特征、缓存优化工具等通用功能
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "../../common/constants.h"
#include "../../common/macros.h"

namespace utils {

using namespace common;

// =============================================================================
// 编译期工具
// =============================================================================

/// 检查是否为 2 的幂
template <std::size_t N>
inline constexpr bool is_power_of_two_v = (N > 0) && ((N & (N - 1)) == 0);

/// 向上取整到 2 的幂（constexpr 函数实现）
constexpr std::size_t next_power_of_two_impl(std::size_t n) noexcept {
    if (n == 0 || n == 1) {
        return 1;
    }

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    if constexpr (sizeof(std::size_t) > 4) {
        n |= n >> 32;
    }

    return n + 1;
}

template <std::size_t N>
inline constexpr std::size_t next_power_of_two_v = next_power_of_two_impl(N);

/// 编译期 log2
template <std::size_t N>
struct log2 {
    static_assert(N > 0, "log2 of 0 is undefined");
    static constexpr std::size_t value = 1 + log2<N / 2>::value;
};

template <>
struct log2<1> {
    static constexpr std::size_t value = 0;
};

template <std::size_t N>
inline constexpr std::size_t log2_v = log2<N>::value;

/// 索引掩码（用于快速取模）
template <std::size_t N>
inline constexpr std::size_t index_mask_v = N - 1;

/// 向上对齐到指定边界
template <std::size_t Alignment>
[[nodiscard, gnu::always_inline, gnu::hot, gnu::flatten]]
inline constexpr std::size_t align_up(std::size_t size) noexcept {
    static_assert(is_power_of_two_v<Alignment>, "Alignment must be power of 2");
    return (size + Alignment - 1) & ~(Alignment - 1);
}

// =============================================================================
// 类型特征
// =============================================================================

/// 检查类型是否适合用于缓冲区
template <typename T>
struct is_buffer_compatible : std::conjunction<std::is_destructible<T>, std::is_move_constructible<T>> {};

template <typename T>
inline constexpr bool is_buffer_compatible_v = is_buffer_compatible<T>::value;

/// 检查类型是否自然对齐（避免未对齐内存访问的总线惩罚）
template <typename T>
struct is_naturally_aligned
    : std::bool_constant<alignof(T) >= sizeof(T) || sizeof(T) <= alignof(max_align_t)> {};

template <typename T>
inline constexpr bool is_naturally_aligned_v = is_naturally_aligned<T>::value;

/// 计算类型的最优对齐（用于缓存行对齐）
template <typename T>
struct optimal_alignment {
    static constexpr std::size_t value = (sizeof(T) <= 8)    ? 8
                                         : (sizeof(T) <= 16) ? 16
                                         : (sizeof(T) <= 32) ? 32
                                         : (sizeof(T) <= 64) ? 64
                                                             : alignof(T);
};

template <typename T>
inline constexpr std::size_t optimal_alignment_v = optimal_alignment<T>::value;

// =============================================================================
// 缓存优化工具
// =============================================================================

/// 缓存行填充
template <std::size_t Size>
struct Padding {
    // 修正后的填充大小计算逻辑
    static constexpr std::size_t remainder = Size % memory_constants::kCacheLineSize;
    static constexpr std::size_t kPaddingSize =
        (remainder == 0) ? 0 : (memory_constants::kCacheLineSize - remainder);

    // 静态断言：确保填充大小非负且不会超过缓存行大小（鲁棒性）
    static_assert(kPaddingSize >= 0, "Padding size cannot be negative");
    static_assert(kPaddingSize < memory_constants::kCacheLineSize, "Padding size is too large");

    int8_t padding_[kPaddingSize];
};

/// 生产者/消费者隔离存储
/// 将生产和消费数据放在不同的缓存行
template <typename T>
class alignas(memory_constants::kMaxCacheLineSize) ProducerConsumerSeparated {
public:
    constexpr ProducerConsumerSeparated() noexcept = default;

    /// 生产者数据（独占缓存行）
    struct alignas(memory_constants::kCacheLineSize) ProducerData {
        T value{};
        Padding<sizeof(T)> padding_;
    };

    /// 消费者数据（独占缓存行）
    struct alignas(memory_constants::kCacheLineSize) ConsumerData {
        T value{};
        Padding<sizeof(T)> padding_;
    };

    ProducerData producer_;
    ConsumerData consumer_;
};

// =============================================================================
// 缓存优化计算
// =============================================================================

/**
 * @brief 缓存层级枚举
 */
enum class CacheLevel {
    L1,  ///< L1 缓存
    L2,  ///< L2 缓存
    L3   ///< L3 缓存
};

/**
 * @brief 计算适配缓存层级的缓冲区大小
 *
 * 建议将缓冲区大小控制在 L2 缓存内（256KB），
 * 以获得最佳访问延迟（L2 约 3-5ns vs DDR4 约 80ns）
 */
template <typename T>
constexpr std::size_t cache_optimal_capacity(CacheLevel level = CacheLevel::L2) noexcept {
    const std::size_t cache_size = (level == CacheLevel::L1)   ? memory_constants::kL1CacheSize
                                   : (level == CacheLevel::L2) ? memory_constants::kL2CacheSize
                                                               : memory_constants::kL3CacheSize;
    return cache_size / sizeof(T);
}

/// 判断容量是否适合 L2 缓存
template <typename T, std::size_t Capacity>
inline constexpr bool fits_l2_cache_v = (Capacity * sizeof(T)) <= memory_constants::kL2CacheSize;

}  // namespace utils
