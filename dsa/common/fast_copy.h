#pragma once

// x86 SIMD
#include <emmintrin.h>
#include <immintrin.h>
#include <xmmintrin.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace common {

template <typename T>
struct can_memcpy : std::is_trivially_copyable<T> {};

template <typename T>
static constexpr bool can_memcpy_v = can_memcpy<T>::value;

/// 检查类型是否适合 SIMD 操作
/// 条件：平凡可复制 + 大小是 1/2/4/8/16/32/64 字节
template <typename T>
struct is_simd_friendly
    : std::conjunction<std::is_trivially_copyable<T>,
                       std::bool_constant<sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ||
                                          sizeof(T) == 8 || sizeof(T) == 16 || sizeof(T) == 32>> {};

template <typename T>
static constexpr bool is_simd_friendly_v = is_simd_friendly<T>::value;

/// batch copy with SIMD avx2
[[gnu::optimize("Ofast"), gnu::always_inline, gnu::hot, gnu::target("avx2")]]
static inline void copy_avx2(void* dst, const void* src, std::size_t bytes) noexcept {
    if (bytes == 0) {
        return;
    }

    constexpr std::size_t vec_size = 32;
    const std::size_t vec_count = bytes / vec_size;

    // 安全边界：确保不会读取超出范围
    // const std::size_t safe_count = (vec_count >= 4) ? vec_count - 4 : 0;

    const __m256i* s = static_cast<const __m256i*>(src);
    __m256i* d = static_cast<__m256i*>(dst);

    std::size_t i = 0;
    // 使用展开循环，但确保不会越界
    for (; i + 4 <= vec_count; i += 4) {
        __m256i v0 = _mm256_loadu_si256(s + i);
        __m256i v1 = _mm256_loadu_si256(s + i + 1);
        __m256i v2 = _mm256_loadu_si256(s + i + 2);
        __m256i v3 = _mm256_loadu_si256(s + i + 3);
        _mm256_storeu_si256(d + i, v0);
        _mm256_storeu_si256(d + i + 1, v1);
        _mm256_storeu_si256(d + i + 2, v2);
        _mm256_storeu_si256(d + i + 3, v3);
    }

    // 处理剩余完整向量
    for (; i < vec_count; ++i) {
        _mm256_storeu_si256(d + i, _mm256_loadu_si256(s + i));
    }

    // 处理尾部字节
    const std::size_t processed_bytes = i * vec_size;
    if (processed_bytes < bytes) {
        const char* tail_src = static_cast<const char*>(src) + processed_bytes;
        char* tail_dst = static_cast<char*>(dst) + processed_bytes;
        const std::size_t tail_bytes = bytes - processed_bytes;

        // 使用memcpy处理尾部，编译器通常会优化小内存复制
        std::memcpy(tail_dst, tail_src, tail_bytes);
    }
}

/// batch copy with SIMD sse2
[[gnu::optimize("Ofast"), gnu::always_inline, gnu::hot, gnu::target("sse2")]]
static inline void copy_sse2(void* dst, const void* src, std::size_t bytes) noexcept {
    if (bytes == 0) {
        return;
    }

    constexpr std::size_t vec_size = 16;
    const std::size_t vec_count = bytes / vec_size;

    const __m128i* s = static_cast<const __m128i*>(src);
    __m128i* d = static_cast<__m128i*>(dst);

    std::size_t i = 0;
    for (; i + 4 <= vec_count; i += 4) {
        __m128i v0 = _mm_loadu_si128(s + i);
        __m128i v1 = _mm_loadu_si128(s + i + 1);
        __m128i v2 = _mm_loadu_si128(s + i + 2);
        __m128i v3 = _mm_loadu_si128(s + i + 3);
        _mm_storeu_si128(d + i, v0);
        _mm_storeu_si128(d + i + 1, v1);
        _mm_storeu_si128(d + i + 2, v2);
        _mm_storeu_si128(d + i + 3, v3);
    }

    for (; i < vec_count; ++i) {
        _mm_storeu_si128(d + i, _mm_loadu_si128(s + i));
    }

    // 处理尾部字节
    const std::size_t processed_bytes = i * vec_size;
    if (processed_bytes < bytes) {
        const char* tail_src = static_cast<const char*>(src) + processed_bytes;
        char* tail_dst = static_cast<char*>(dst) + processed_bytes;
        const std::size_t tail_bytes = bytes - processed_bytes;
        std::memcpy(tail_dst, tail_src, tail_bytes);
    }
}

/// fast copy with SIMD optimization
template <typename T>
[[gnu::optimize("Ofast"), gnu::always_inline, gnu::hot, gnu::flatten, gnu::target("avx2")]]
static inline void fast_copy(T* dst, const T* src, std::size_t count) noexcept {
    if (count == 0) [[unlikely]] {
        return;
    }

    const std::size_t bytes = count * sizeof(T);
    if constexpr (!can_memcpy_v<T>) {
        // non trivially, copy one by one
        for (std::size_t i = 0; i < count; ++i) {
            dst[i] = src[i];
        }
    } else if constexpr (is_simd_friendly_v<T>) {
        // 根据数据大小选择策略
        if (bytes >= 64 && bytes <= 4096) {
#if defined(__AVX2__)
            if (bytes >= 128) {
                copy_avx2(dst, src, bytes);
            } else {
                copy_sse2(dst, src, bytes);
            }
#else
            copy_sse2(dst, src, bytes);
#endif
        } else {
            std::memcpy(dst, src, bytes);
        }
    } else {
        std::memcpy(dst, src, bytes);
    }
}

/// SIMD batch copy
template <typename T, std::size_t BatchSize = 64>
[[gnu::optimize("Ofast"), gnu::always_inline, gnu::hot, gnu::target("avx2")]]
static inline void fast_copy_batch(T* dst, const T* src) noexcept {
    static_assert(can_memcpy_v<T>, "T must be trivially copyable for batch copy");
    static_assert(BatchSize > 0, "BatchSize must be greater than 0");

    constexpr std::size_t bytes = BatchSize * sizeof(T);

    // 根据编译器和架构选择最优复制方式
    if constexpr (bytes >= 32) {
#if defined(__AVX2__)
        copy_avx2(dst, src, bytes);
#elif defined(__SSE2__)
        copy_sse2(dst, src, bytes);
#else
        std::memcpy(dst, src, bytes);
#endif
    } else {
        std::memcpy(dst, src, bytes);
    }
}
}  // namespace common
