#include <mw/common/Logger.h>

void null_logger(const std::string&) {}

static std::function<void(const std::string&)> LOGGER_CALLBACK = null_logger;
static LoggerAPI::LogLevel MIN_LOG_LEVEL = LoggerAPI::DEBUG;

namespace LoggerAPI
{
    void Initialize(const std::function<void(const std::string&)>& log_callback)
    {
        if (log_callback) {
            LOGGER_CALLBACK = log_callback;
        } else {
            LOGGER_CALLBACK = null_logger;
        }
    }

    void Log(
        const LoggerAPI::LogLevel log_level,
        const std::string& function,
        const size_t line,
        const std::string& message) noexcept
    {
        if (log_level >= MIN_LOG_LEVEL) {
            std::string formatted = StringUtil::Format("{}({}) - {}", function, line, message);

            if (formatted.back() != '\n') {
                formatted += "\n";
            }

            LOGGER_CALLBACK(formatted);
        }
    }
}