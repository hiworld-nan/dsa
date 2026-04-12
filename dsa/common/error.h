/**
 * @file error.h
 * @brief 错误处理框架 - std::expected 封装
 * @version 1.0.0
 *
 * 使用 std::expected 替代异常，提供函数式错误处理
 */

#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <variant>

namespace common {

// =============================================================================
// 错误类型定义
// =============================================================================

/// 错误类别
enum class ErrorCategory : uint8_t {
    None = 0,
    InvalidArgument,
    RuntimeError,
    BadAlloc,
    IOError,
    MathError,
    DimensionMismatch,
    SingularMatrix,
};

/// 错误信息
class Error {
public:
    Error() = default;

    Error(ErrorCategory cat, std::string msg = "") : category_(cat), message_(std::move(msg)) {}

    [[nodiscard]] ErrorCategory category() const noexcept { return category_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] explicit operator bool() const noexcept { return category_ != ErrorCategory::None; }

    [[nodiscard]] std::string what() const {
        switch (category_) {
        case ErrorCategory::InvalidArgument: return "InvalidArgument: " + message_;
        case ErrorCategory::RuntimeError: return "RuntimeError: " + message_;
        case ErrorCategory::BadAlloc: return "BadAlloc: " + message_;
        case ErrorCategory::IOError: return "IOError: " + message_;
        case ErrorCategory::MathError: return "MathError: " + message_;
        case ErrorCategory::DimensionMismatch: return "DimensionMismatch: " + message_;
        case ErrorCategory::SingularMatrix: return "SingularMatrix: " + message_;
        default: return "Unknown error: " + message_;
        }
    }

private:
    static constexpr std::string_view category_name(ErrorCategory cat) {
        struct CategoryName {
            ErrorCategory cat;
            std::string_view name;
        };
        static constexpr CategoryName names[] = {
            {ErrorCategory::InvalidArgument, "InvalidArgument"},
            {ErrorCategory::RuntimeError, "RuntimeError"},
            {ErrorCategory::BadAlloc, "BadAlloc"},
            {ErrorCategory::IOError, "IOError"},
            {ErrorCategory::MathError, "MathError"},
            {ErrorCategory::DimensionMismatch, "DimensionMismatch"},
            {ErrorCategory::SingularMatrix, "SingularMatrix"},
            {ErrorCategory::None, "None"},
        };

        return names[static_cast<size_t>(cat) % (sizeof(names) / sizeof(CategoryName))].name;
    }

private:
    ErrorCategory category_{ErrorCategory::None};
    std::string message_;
};

// =============================================================================
// 工厂函数 - 创建错误
// =============================================================================

[[nodiscard]] inline Error make_error(ErrorCategory cat, std::string msg = "") {
    return Error(cat, std::move(msg));
}

[[nodiscard]] inline Error invalid_argument(std::string msg = "") {
    return Error(ErrorCategory::InvalidArgument, std::move(msg));
}

[[nodiscard]] inline Error runtime_error(std::string msg = "") {
    return Error(ErrorCategory::RuntimeError, std::move(msg));
}

[[nodiscard]] inline Error bad_alloc(std::string msg = "memory allocation failed") {
    return Error(ErrorCategory::BadAlloc, std::move(msg));
}

[[nodiscard]] inline Error io_error(std::string msg = "") {
    return Error(ErrorCategory::IOError, std::move(msg));
}

[[nodiscard]] inline Error math_error(std::string msg = "") {
    return Error(ErrorCategory::MathError, std::move(msg));
}

[[nodiscard]] inline Error dimension_mismatch(std::string msg = "") {
    return Error(ErrorCategory::DimensionMismatch, std::move(msg));
}

[[nodiscard]] inline Error singular_matrix(std::string msg = "matrix is singular") {
    return Error(ErrorCategory::SingularMatrix, std::move(msg));
}

// =============================================================================
// expected 类型别名
// =============================================================================

template <typename T>
using Expected = std::expected<T, Error>;

/// 无值返回的 expected 类型
using ExpectedVoid = std::expected<void, Error>;

// =============================================================================
// 辅助宏 - 简化错误处理
// =============================================================================

/// 尝试解包 expected，失败时返回错误
#define CRAFT_TRY(expr)                              \
    ({                                               \
        auto&& _result = (expr);                     \
        if (!_result) [[unlikely]]                   \
            return std::unexpected(_result.error()); \
        std::move(_result.value());                  \
    })

/// 尝试解包 expected 到变量
#define CRAFT_TRY_ASSIGN(var, expr)                    \
    auto _result_##var = (expr);                       \
    if (!_result_##var) [[unlikely]]                   \
        return std::unexpected(_result_##var.error()); \
    var = std::move(_result_##var.value())

// =============================================================================
// 值包装器 - 用于返回 void 或值的场景
// =============================================================================

/// 成功值
template <typename T>
[[nodiscard]] constexpr Expected<T> make_expected(T&& value) {
    return std::expected<T, Error>(std::forward<T>(value));
}

template <typename T>
[[nodiscard]] constexpr Expected<T> make_expected(const T& value) {
    return std::expected<T, Error>(value);
}

/// 成功的无值返回
[[nodiscard]] constexpr ExpectedVoid make_expected() { return ExpectedVoid(); }

/// 失败返回
template <typename T>
[[nodiscard]] constexpr Expected<T> make_unexpected(Error err) {
    return std::unexpected(err);
}

template <typename T>
[[nodiscard]] constexpr Expected<T> make_unexpected(ErrorCategory cat, std::string msg = "") {
    return std::unexpected(Error(cat, std::move(msg)));
}

}  // namespace common
