#pragma once

#include <atomic>
#include <bit>
#include <cassert>
#include <cstdint>

#include "constants.h"
#include "macros.h"

namespace common {
// The following comments explain the usage of inline assembly in C/C++ code.
/*
    asm ("") : does nothing (or at least, it's not supposed to do anything).
    asm volatile ("") : also does nothing.
    asm ("" ::: "memory") : is a simple compiler fence
    asm volatile ("" ::: "memory") : AFAIK is the same as the previous
    memory: hint all core register or core cache invalid

    '=' : output, a variable overwriting an existing value Means that this
    operand is written to by this instruction: the previous value is discarded and replaced by new data.

    '+' : when reading and writing Means that this operand is both read
    and written by the instruction

    '=' identifies an operand which is only written; ‘+’ identifies an operand
    that is both read and written; all other operands are assumed to only be read.

    'd', 'a' and 'f' are defined on the 68000/68020 to stand for data, address and floating point registers

    a : *ax register
    d : *dx register
    c : *cx register
    r : any register
    m : A memory operand is allowed, with any kind of address that the machine supports in general

    volatile : hint compiler do not optimize; do not out of order; do not instruction reorder
    asm volatile: prevents the asm instruction from being "moved significantly"
    memory: prevents GCC from keeping memory values cached in registers
                    across the assembler instruction
compiler fence:
    prevent compile reordering instruction
    asm volatile("":::"memory");
    fence:
    instruction is out of order----> compiler fence

memory fence:
    mutli-core fence/barrier:
    visible ----> cpu fence
    asm volatile("lfence");
    asm volatile("sfence");
    asm volatile("mfence");

compiler+memory barrier/fence:
    asm volatile("lfence":::"memory");
    asm volatile("sfence":::"memory");
    asm volatile("mfence":::"memory");

    lock addl: much better way
            asm volatile("lock;addl $0,0(%%esp)":::"memory")
            addl $0,0(%%esp):do nothing
            lock:to be a cpu memory barrier
            memory:to be a compiler memory barrier

    xchg
            asm volatile("xchgl (%0),%0":::"memory")
            Q:why xchg do not need lock prefix?
            A:the LOCK prefix is automatically assumed for XCHG instruction
*/

// clflush is a cache line flush instruction that flushes the cache line containing the specified address.
// It ensures that the data in the cache is written back to the main memory.
[[gnu::hot, gnu::always_inline, gnu::nonnull(1)]]
static inline void clflush(void* p) {
    asm volatile("clflush %0" : : "m"(*(char*)p) : "memory");
}

// Flush a single cache line (write back to memory and invalidate)
[[gnu::hot, gnu::always_inline, gnu::nonnull(1)]]
static inline void clflush_range(void* p, size_t size) {
    char* addr = static_cast<char*>(p);
    char* end = addr + size;
    const size_t step = common::memory_constants::kCacheLineSize;
    for (char* cur = addr; cur < end; cur += step) {
        clflush(cur);
    }
}

// Write back the cache line but keep it in the cache (requires CPU support; otherwise, it will trigger #UD)
[[gnu::hot, gnu::always_inline, gnu::nonnull(1)]]
static inline void clwb(void* p) {
    asm volatile("clwb %0" : : "m"(*(char*)p) : "memory");
}

// Write back into memory range (per cache line)
[[gnu::hot, gnu::always_inline, gnu::nonnull(1)]]
static inline void clwb_range(void* p, size_t size) {
    char* addr = static_cast<char*>(p);
    char* end = addr + size;
    const size_t step = common::memory_constants::kCacheLineSize;
    for (char* cur = addr; cur < end; cur += step) {
        clwb(cur);
    }
}

template <typename T>
[[nodiscard, gnu::hot, gnu::always_inline, gnu::nonnull(1)]]
static inline T xchg(T* ptr, T val) noexcept {
    assert(ptr != nullptr);
    static_assert(sizeof(T) <= 8, "unsupported size");
    if constexpr (sizeof(T) == 1) {
        asm volatile("xchgb %0, %1" : "+m"(*ptr), "+q"(val) : : "memory");
    } else if constexpr (sizeof(T) == 2) {
        asm volatile("xchgw %0, %1" : "+m"(*ptr), "+r"(val) : : "memory");
    } else if constexpr (sizeof(T) == 4) {
        asm volatile("xchgl %0, %1" : "+m"(*ptr), "+r"(val) : : "memory");
    } else if constexpr (sizeof(T) == 8) {
        asm volatile("xchgq %0, %1" : "+m"(*ptr), "+r"(val) : : "memory");
    }
    return val;  // 返回旧值
}

// renop is a no-operation instruction that does nothing.
// It is often used for timing or alignment purposes in assembly code.
[[gnu::hot, gnu::always_inline]]
static inline void nop() noexcept {
    asm volatile("nop");
}
[[gnu::hot, gnu::always_inline]]
static inline void renop() noexcept {
    asm volatile("rep;nop");
}

[[gnu::hot, gnu::always_inline]]
static inline void pause() noexcept {
#if ARCH_X86_64
    asm volatile("pause" ::: "memory");
    // asm volatile("pause" :::);   // without memory, only pause
    // __builtin_ia32_pause();
    // _mm_pause();
#else
    std::atomic_thread_fence(std::memory_order_acquire);
#endif
}

#define COMPILER_BARRIER() asm volatile("" ::: "memory")

// x86 compiler barrier, prevent compiler reordering
[[gnu::hot, gnu::always_inline]]
static inline void compiler_fence() noexcept {
#if ARCH_X86_64
    asm volatile("" ::: "memory");
#else
    std::atomic_thread_fence(std::memory_order_acquire);
#endif
}

// x86 read memory barrier, (hardware+compiler barrier)
[[gnu::hot, gnu::always_inline]]
static inline void lfence() noexcept {
    asm volatile("lfence" ::: "memory");
}

// x86 write memory barrier, (hardware+compiler barrier)
[[gnu::hot, gnu::always_inline]]
static inline void sfence() noexcept {
    asm volatile("sfence" ::: "memory");
}

// x86 full memory barrier, (hardware+compiler barrier)
[[gnu::hot, gnu::always_inline]]
static inline void mfence() noexcept {
    asm volatile("mfence" ::: "memory");
}

// release & require memory barrier
// More efficient sequential consistency barrier
[[gnu::hot, gnu::always_inline]]
static inline void mfence_light() noexcept {
    asm volatile("lock; addl $0, -4(%%rsp)" ::: "memory", "cc");
}

// Under the x86 TSO model, LoadLoad/LoadStore/StoreStore are guaranteed by hardware.
// Therefore, acquire/release only require compiler barriers.
// Semantically clear names are provided here, but they are actually just compiler barriers.

// read barrier (LoadLoad + LoadStore)
// x86: only compiler barrier
[[gnu::hot, gnu::always_inline]]
static inline void acquire_fence() noexcept {
#if ARCH_X86_64
    compiler_fence();
#else
    std::atomic_thread_fence(std::memory_order_acquire);
#endif
}

// write barrier (StoreStore)
// x86: only compiler barrier
[[gnu::hot, gnu::always_inline]]
static inline void release_fence() noexcept {
#if ARCH_X86_64
    compiler_fence();
#else
    std::atomic_thread_fence(std::memory_order_release);
#endif
}

// StoreLoad barrier (x86 only requires a CPU barrier)
[[gnu::hot, gnu::always_inline]]
static inline void storeload_fence() noexcept {
#if ARCH_X86_64
    mfence();
#else
    std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
}

// read time stamp counter
[[gnu::hot, gnu::always_inline]]
static inline uint64_t rdtsc() noexcept {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi)::"memory");
    return (uint64_t(hi) << 32) | lo;
}
// read time stamp counter and processor id
[[gnu::hot, gnu::always_inline]]
static inline uint64_t rdtscp() noexcept {
    uint32_t lo, hi, aux;
    asm volatile("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux)::"memory");
    return (uint64_t(hi) << 32) | lo;
}

struct CpuidRegs {
    uint32_t eax, ebx, ecx, edx;
};

[[gnu::hot, gnu::always_inline]]
static inline CpuidRegs cpuid(uint32_t leaf, uint32_t subleaf = 0) noexcept {
    CpuidRegs regs{};
    asm volatile("cpuid"
                 : "=a"(regs.eax), "=b"(regs.ebx), "=c"(regs.ecx), "=d"(regs.edx)
                 : "a"(leaf), "c"(subleaf)
                 : "memory");
    return regs;
}

// Execute cpuid(0) to serialize all previous instructions (often used for rdtsc precise timing)
[[gnu::hot, gnu::always_inline]]
static inline void cpuid_serialize() noexcept {
    CpuidRegs regs{};
    asm volatile("cpuid"
                 : "=a"(regs.eax), "=b"(regs.ebx), "=c"(regs.ecx), "=d"(regs.edx)
                 : "a"(0)
                 : "memory");
}

enum class PrefetchMode { Read = 0, Write = 1 };
enum class PrefetchLocality {
    /*The CPU prefetches the data into the L1 cache but marks it as "replaceable" immediately after use. It
       does not occupy cache space long-term.*/
    NoTemporalLocality = 0,  // No temporal locality (data is used once and discarded)
    /*The data is stored in L1/L2 cache and retained for a short period (a few loop iterations).*/
    LowTemporalLocality = 1,  // Low temporal locality
    /*The data is stored in L2/L3 cache and retained for a moderate period (multiple loop iterations).*/
    MidTemporalLocality = 2,  // Moderate temporal locality
    /*The data is prioritized for storage in L3 cache (or higher-level shared cache) and retained long-term.*/
    HighTemporalLocality = 3  // High temporal locality
};

template <PrefetchLocality Hint = PrefetchLocality::NoTemporalLocality>
[[gnu::hot, gnu::always_inline, gnu::nonnull(1)]]
static inline void prefetch_read(const void* addr) noexcept {
    PREFETCH(addr, static_cast<int>(PrefetchMode::Read), static_cast<int>(Hint));
}

template <PrefetchLocality Hint = PrefetchLocality::NoTemporalLocality>
[[gnu::hot, gnu::always_inline, gnu::nonnull(1)]]
static inline void prefetch_write(void* addr) noexcept {
    PREFETCH(addr, static_cast<int>(PrefetchMode::Write), static_cast<int>(Hint));
}

/// batch prefetch
template <std::size_t Stride = common::memory_constants::kCacheLineSize>
[[gnu::hot, gnu::always_inline, gnu::nonnull(1)]]
static inline void prefetch_range(const void* addr, std::size_t size) noexcept {
    const char* p = static_cast<const char*>(addr);
    const char* end = p + size;

    for (; p < end; p += Stride) {
        prefetch_read(p);
    }
}

enum class CacheLevel {
    L1,  // level 1 cache
    L2,  // level 2 cache
    L3   // level 3 cache
};

// prefetch multiple data into cache; sequential prefetch for contiguous data
template <typename T, std::size_t PrefetchDistance = 4>
[[gnu::hot, gnu::always_inline, gnu::nonnull(1)]]
static inline void aggressive_prefetch(const T* base_addr, std::size_t current_idx) noexcept {
    prefetch_read(&base_addr[current_idx + 1]);
    if constexpr (PrefetchDistance >= 2) {
        prefetch_read(&base_addr[current_idx + 2]);
    }
    if constexpr (PrefetchDistance >= 3) {
        prefetch_read(&base_addr[current_idx + 3]);
    }
    if constexpr (PrefetchDistance >= 4) {
        prefetch_read(&base_addr[current_idx + 4]);
    }
}

template <typename T>
class AdaptivePrefetcher {
public:
    // 根据数据大小和访问模式动态调整预取距离
    [[gnu::hot, gnu::always_inline, gnu::nonnull(1)]]
    inline void prefetch_read_adaptive(const T* base, std::size_t current_idx, std::size_t capacity,
                                       std::size_t available) {
        assert(capacity > 0 && std::has_single_bit(capacity));
        // 计算最优预取距离
        std::size_t distance = calculate_prefetch_distance(available);

        // 多级预取
        for (size_t i = 1; i <= distance; ++i) {
            std::size_t idx = (current_idx + i) & (capacity - 1);

            // 根据距离选择预取强度
            if (i <= 2) {
                // 近距离：L1预取（高时效性）
                prefetch_read<PrefetchLocality::HighTemporalLocality>(&base[idx]);
            } else if (i <= 4) {
                // 中距离：L2预取
                prefetch_read<PrefetchLocality::MidTemporalLocality>(&base[idx]);
            } else {
                // 远距离：L3预取（低时效性）
                prefetch_read<PrefetchLocality::LowTemporalLocality>(&base[idx]);
            }
        }
    }

private:
    [[gnu::hot, gnu::always_inline]]
    constexpr inline size_t calculate_prefetch_distance(std::size_t available) const noexcept {
        // 根据可用数据量调整预取距离
        if (available < 4) {
            return 1;
        }

        if (available < 16) {
            return 2;
        }

        if (available < 64) {
            return 4;
        }
        return 8;
    }
};

}  // namespace common
