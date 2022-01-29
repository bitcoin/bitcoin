#pragma once

#include <mw/util/StringUtil.h>
#include <functional>

namespace LoggerAPI
{
    enum LogLevel : uint8_t
    {
        NONE = 0,
        TRACE = 1,
        DEBUG = 2,
        INFO = 3,
        WARN = 4,
        ERR = 5
    };

    void Initialize(const std::function<void(const std::string&)>& log_callback);

    void Log(
        const LoggerAPI::LogLevel log_level,
        const std::string& function,
        const size_t line,
        const std::string& message
    ) noexcept;
}

template<typename ... Args>
static void LOG_F(
    const LoggerAPI::LogLevel log_level,
    const std::string& function,
    const size_t line,
    const char* format,
    const Args& ... args) noexcept
{
    try
    {
        std::string message = StringUtil::Format(format, args...);
        LoggerAPI::Log(log_level, function, line, message);
    }
    catch (std::exception&)
    {
        // Logger failure should not disrupt program flow
    }
}

// Node Logger
#define LOG_TRACE(message) LoggerAPI::Log(LoggerAPI::LogLevel::TRACE, __FUNCTION__, __LINE__, message)
#define LOG_DEBUG(message) LoggerAPI::Log(LoggerAPI::LogLevel::DEBUG, __FUNCTION__, __LINE__, message)
#define LOG_INFO(message) LoggerAPI::Log(LoggerAPI::LogLevel::INFO, __FUNCTION__, __LINE__, message)
#define LOG_WARNING(message) LoggerAPI::Log(LoggerAPI::LogLevel::WARN, __FUNCTION__, __LINE__, message)
#define LOG_ERROR(message) LoggerAPI::Log(LoggerAPI::LogLevel::ERR, __FUNCTION__, __LINE__, message)

#define LOG_TRACE_F(message, ...) LOG_F(LoggerAPI::LogLevel::TRACE, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_DEBUG_F(message, ...) LOG_F(LoggerAPI::LogLevel::DEBUG, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_INFO_F(message, ...) LOG_F(LoggerAPI::LogLevel::INFO, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_WARNING_F(message, ...) LOG_F(LoggerAPI::LogLevel::WARN, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_ERROR_F(message, ...) LOG_F(LoggerAPI::LogLevel::ERR, __FUNCTION__, __LINE__, message, __VA_ARGS__)

// Wallet Logger
#define WALLET_TRACE(message) LoggerAPI::Log(LoggerAPI::LogLevel::TRACE, __FUNCTION__, __LINE__, message)
#define WALLET_DEBUG(message) LoggerAPI::Log(LoggerAPI::LogLevel::DEBUG, __FUNCTION__, __LINE__, message)
#define WALLET_INFO(message) LoggerAPI::Log(LoggerAPI::LogLevel::INFO, __FUNCTION__, __LINE__, message)
#define WALLET_WARNING(message) LoggerAPI::Log(LoggerAPI::LogLevel::WARN, __FUNCTION__, __LINE__, message)
#define WALLET_ERROR(message) LoggerAPI::Log(LoggerAPI::LogLevel::ERR, __FUNCTION__, __LINE__, message)

#define WALLET_TRACE_F(message, ...) LOG_F(LoggerAPI::LogLevel::TRACE, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define WALLET_DEBUG_F(message, ...) LOG_F(LoggerAPI::LogLevel::DEBUG, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define WALLET_INFO_F(message, ...) LOG_F(LoggerAPI::LogLevel::INFO, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define WALLET_WARNING_F(message, ...) LOG_F(LoggerAPI::LogLevel::WARN, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define WALLET_ERROR_F(message, ...) LOG_F(LoggerAPI::LogLevel::ERR, __FUNCTION__, __LINE__, message, __VA_ARGS__)