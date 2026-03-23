/**
 * @file error.hpp
 * @brief Error handling for stdiobus C++ SDK
 * 
 * By default, uses status-style error handling (no exceptions).
 * Define STDIOBUS_CPP_EXCEPTIONS=1 to enable exception mode.
 */

#ifndef STDIOBUS_ERROR_HPP
#define STDIOBUS_ERROR_HPP

#include <string>
#include <string_view>
#include <system_error>

namespace stdiobus {

/**
 * @brief Error codes matching C API
 */
enum class ErrorCode : int {
    Ok = 0,
    Error = -1,
    Again = -2,
    Eof = -3,
    Full = -4,
    NotFound = -5,
    Invalid = -6,
    Config = -10,
    Worker = -11,
    Routing = -12,
    Buffer = -13,
    State = -15,
    Timeout = -20,
    PolicyDenied = -21,
};

/**
 * @brief Convert error code to string
 */
constexpr std::string_view error_code_name(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Ok: return "Ok";
        case ErrorCode::Error: return "Error";
        case ErrorCode::Again: return "Again";
        case ErrorCode::Eof: return "Eof";
        case ErrorCode::Full: return "Full";
        case ErrorCode::NotFound: return "NotFound";
        case ErrorCode::Invalid: return "Invalid";
        case ErrorCode::Config: return "Config";
        case ErrorCode::Worker: return "Worker";
        case ErrorCode::Routing: return "Routing";
        case ErrorCode::Buffer: return "Buffer";
        case ErrorCode::State: return "State";
        case ErrorCode::Timeout: return "Timeout";
        case ErrorCode::PolicyDenied: return "PolicyDenied";
        default: return "Unknown";
    }
}

/**
 * @brief Check if error is retryable
 */
constexpr bool is_retryable(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Again:
        case ErrorCode::Full:
        case ErrorCode::Timeout:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Error class for status-style error handling
 * 
 * Lightweight, copyable, no heap allocation for common cases.
 */
class Error {
public:
    Error() noexcept : code_(ErrorCode::Ok) {}
    Error(ErrorCode code) noexcept : code_(code) {}
    Error(ErrorCode code, std::string message) noexcept 
        : code_(code), message_(std::move(message)) {}
    
    /// Check if error occurred (true = error, false = ok)
    constexpr explicit operator bool() const noexcept { 
        return code_ != ErrorCode::Ok; 
    }
    
    /// Get error code
    constexpr ErrorCode code() const noexcept { return code_; }
    
    /// Get error message
    std::string_view message() const noexcept {
        if (!message_.empty()) return message_;
        return error_code_name(code_);
    }
    
    /// Check if error is retryable
    constexpr bool is_retryable() const noexcept {
        return stdiobus::is_retryable(code_);
    }
    
    /// Create OK result
    static Error ok() noexcept { return Error{}; }
    
    /// Create error from C API return code
    static Error from_c(int code) noexcept {
        return Error{static_cast<ErrorCode>(code)};
    }

private:
    ErrorCode code_;
    std::string message_;
};

#ifdef STDIOBUS_CPP_EXCEPTIONS

/**
 * @brief Exception class (only when STDIOBUS_CPP_EXCEPTIONS is defined)
 */
class Exception : public std::exception {
public:
    explicit Exception(Error error) : error_(std::move(error)) {
        what_msg_ = std::string(error_.message());
    }
    
    const char* what() const noexcept override {
        return what_msg_.c_str();
    }
    
    const Error& error() const noexcept { return error_; }
    ErrorCode code() const noexcept { return error_.code(); }

private:
    Error error_;
    std::string what_msg_;
};

/// Throw exception if error
inline void throw_if_error(const Error& err) {
    if (err) throw Exception(err);
}

#endif // STDIOBUS_CPP_EXCEPTIONS

} // namespace stdiobus

#endif // STDIOBUS_ERROR_HPP
