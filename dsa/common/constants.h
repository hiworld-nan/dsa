#pragma once

#include <cstddef>
#include <cstdint>

namespace common {

struct memory_constants {
#if __cplusplus >= 201703L && defined(__cpp_lib_hardware_interference_size)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
    static inline constexpr std::size_t kCacheLineSize = std::hardware_destructive_interference_size;
#pragma GCC diagnostic pop
#else
    static inline constexpr std::size_t kCacheLineSize = 64;
#endif

    /// 最大缓存行大小（用于填充，覆盖不同CPU架构）
    static inline constexpr std::size_t kMaxCacheLineSize = 128;

    /// 页面大小
    static inline constexpr std::size_t kPageSize = 4096;

    /// 大页大小（2MB）
    static inline constexpr std::size_t kHugePageSize = 2 * 1024 * 1024;

    /// 默认容量
    static inline constexpr std::size_t kDefaultCapacity = 1024;

    /// 最小/最大容量
    static inline constexpr std::size_t kMinCapacity = 2;
    static inline constexpr std::size_t kMaxCapacity = 1ULL << 30;

    /// 批量操作默认大小（优化缓存预取，对应一个缓存行的元素数）
    static inline constexpr std::size_t kDefaultBatchSize = 64;

    /// 预取距离
    static inline constexpr std::size_t kPrefetchDistance = 8;

    /// 最大分区数
    static inline constexpr std::size_t kMaxPartitions = 16;

    /// L1 数据缓存大小（典型值，Intel 约 32KB/核心）
    static inline constexpr std::size_t kL1CacheSize = 32 * 1024;

    /// L2 缓存大小（典型值，Intel 约 256KB/核心）
    static inline constexpr std::size_t kL2CacheSize = 256 * 1024;

    /// L3 缓存大小（典型值，Intel 约 8MB-32MB）
    static inline constexpr std::size_t kL3CacheSize = 8 * 1024 * 1024;

    /// SIMD 向量宽度（AVX2: 256位 = 32字节，AVX-512: 512位 = 64字节）
    static inline constexpr std::size_t kSimdWidth = 32;

    /// 对齐边界
    static inline constexpr std::size_t kAlign8 = 8;
    static inline constexpr std::size_t kAlign16 = 16;
    static inline constexpr std::size_t kAlign32 = 32;
    static inline constexpr std::size_t kAlign64 = 64;
};

struct time_constants {
    static constexpr std::size_t kNsPerUs = 1'000;
    static constexpr std::size_t kNsPerMs = 1'000'000;
    static constexpr std::size_t kNsPerSec = 1'000'000'000;

    static constexpr std::size_t kUsPerMs = 1'000;
    static constexpr std::size_t kUsPerSec = 1'000'000;
    static constexpr std::size_t kMsPerSec = 1'000;
};

struct cpu_constants {
    /// 自适应自旋阈值
    static constexpr int kSpinPhase1 = 100;   // 纯自旋（~100ns）
    static constexpr int kSpinPhase2 = 500;   // 批量pause（~500ns）
    static constexpr int kSpinPhase3 = 1000;  // yield（~1μs）
    static constexpr int kSpinPhase4 = 2000;  // 短休眠（~2μs）

    /// CAS 重试次数
    static constexpr int kCasRetryLimit = 100;

    /// 投机读取最大批次
    static constexpr std::size_t kMaxSpeculativeBatch = 64;
};

struct numa_constants {
    /// 默认 NUMA 节点（本地节点）
    static constexpr int kLocalNode = -1;

    /// 最大 NUMA 节点数
    static constexpr std::size_t kMaxNumaNodes = 8;
};

}  // namespace common