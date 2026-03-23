/**
 * @file types.hpp
 * @brief Core types for stdiobus C++ SDK
 */

#ifndef STDIOBUS_TYPES_HPP
#define STDIOBUS_TYPES_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <chrono>
#include <functional>
#include <optional>

namespace stdiobus {

/**
 * @brief Bus state enum
 */
enum class State {
    Created = 0,
    Starting = 1,
    Running = 2,
    Stopping = 3,
    Stopped = 4
};

constexpr std::string_view state_name(State state) noexcept {
    switch (state) {
        case State::Created: return "Created";
        case State::Starting: return "Starting";
        case State::Running: return "Running";
        case State::Stopping: return "Stopping";
        case State::Stopped: return "Stopped";
        default: return "Unknown";
    }
}

/**
 * @brief Listener mode for external connections
 */
enum class ListenMode {
    None = 0,   ///< Embedded mode (no external listener)
    Tcp = 1,    ///< TCP socket listener
    Unix = 2    ///< Unix domain socket listener
};

/**
 * @brief Statistics structure
 */
struct Stats {
    uint64_t messages_in = 0;
    uint64_t messages_out = 0;
    uint64_t bytes_in = 0;
    uint64_t bytes_out = 0;
    uint64_t worker_restarts = 0;
    uint64_t routing_errors = 0;
    uint64_t client_connects = 0;
    uint64_t client_disconnects = 0;
};

/**
 * @brief Listener configuration
 */
struct ListenerConfig {
    ListenMode mode = ListenMode::None;
    std::string tcp_host;
    uint16_t tcp_port = 0;
    std::string unix_path;
};

/**
 * @brief Callback types
 */
using MessageCallback = std::function<void(std::string_view message)>;
using ErrorCallback = std::function<void(ErrorCode code, std::string_view message)>;
using LogCallback = std::function<void(int level, std::string_view message)>;
using WorkerCallback = std::function<void(int worker_id, std::string_view event)>;
using ClientConnectCallback = std::function<void(int client_id, std::string_view peer_info)>;
using ClientDisconnectCallback = std::function<void(int client_id, std::string_view reason)>;

/**
 * @brief Options for creating a Bus instance
 */
struct Options {
    /// Path to JSON config file (one of config_path or config_json required)
    std::string config_path;
    
    /// Inline JSON config string (alternative to config_path)
    std::string config_json;
    
    /// Listener configuration (optional)
    ListenerConfig listener;
    
    /// Log level: 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR
    int log_level = 1;
    
    /// Callbacks
    MessageCallback on_message;
    ErrorCallback on_error;
    LogCallback on_log;
    WorkerCallback on_worker;
    ClientConnectCallback on_client_connect;
    ClientDisconnectCallback on_client_disconnect;
};

/**
 * @brief Duration helper using std::chrono
 */
using Duration = std::chrono::milliseconds;

/**
 * @brief Convert chrono duration to milliseconds int
 */
template<typename Rep, typename Period>
constexpr int to_ms(std::chrono::duration<Rep, Period> d) noexcept {
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(d).count());
}

} // namespace stdiobus

#endif // STDIOBUS_TYPES_HPP
