/**
 * @file bus.hpp
 * @brief Idiomatic C++ Bus class with RAII
 */

#ifndef STDIOBUS_BUS_HPP
#define STDIOBUS_BUS_HPP

#include <stdiobus/error.hpp>
#include <stdiobus/types.hpp>
#include <stdiobus/ffi.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <chrono>

namespace stdiobus {

/**
 * @brief RAII wrapper for stdio_bus
 * 
 * Provides idiomatic C++ interface with:
 * - RAII (automatic cleanup in destructor)
 * - std::string / std::string_view for strings
 * - std::chrono for durations
 * - Callbacks via std::function
 * - Status-style error handling (or exceptions if STDIOBUS_CPP_EXCEPTIONS)
 * 
 * @example
 * ```cpp
 * stdiobus::Bus bus("config.json");
 * 
 * bus.on_message([](std::string_view msg) {
 *     std::cout << "Got: " << msg << std::endl;
 * });
 * 
 * if (auto err = bus.start(); err) {
 *     std::cerr << "Error: " << err.message() << std::endl;
 *     return 1;
 * }
 * 
 * bus.send(R"({"jsonrpc":"2.0","method":"test","id":1})");
 * 
 * while (bus.is_running()) {
 *     bus.step(std::chrono::milliseconds(100));
 * }
 * ```
 */
class Bus {
public:
    /**
     * @brief Construct from config file path
     */
    explicit Bus(std::string_view config_path);
    
    /**
     * @brief Construct from Options struct
     */
    explicit Bus(Options options);
    
    /**
     * @brief Destructor - stops and destroys bus
     */
    ~Bus();
    
    // Non-copyable
    Bus(const Bus&) = delete;
    Bus& operator=(const Bus&) = delete;
    
    // Movable
    Bus(Bus&& other) noexcept;
    Bus& operator=(Bus&& other) noexcept;
    
    // ========== Lifecycle ==========
    
    /**
     * @brief Start the bus (spawn workers)
     * @return Error (check with if(err))
     */
    [[nodiscard]] Error start();
    
    /**
     * @brief Process pending I/O (non-blocking)
     * @param timeout Maximum wait time (0 = non-blocking)
     * @return Number of events processed, or negative error
     */
    int step(Duration timeout = Duration::zero());
    
    /**
     * @brief Process pending I/O with chrono duration
     */
    template<typename Rep, typename Period>
    int step(std::chrono::duration<Rep, Period> timeout) {
        return step(std::chrono::duration_cast<Duration>(timeout));
    }
    
    /**
     * @brief Initiate graceful shutdown
     * @param timeout Maximum time to wait for workers
     * @return Error
     */
    [[nodiscard]] Error stop(std::chrono::seconds timeout = std::chrono::seconds(5));
    
    // ========== Messaging ==========
    
    /**
     * @brief Send a message to workers
     * @param message JSON message string
     * @return Error
     */
    [[nodiscard]] Error send(std::string_view message);
    
    /**
     * @brief Send a message (std::string overload)
     */
    [[nodiscard]] Error send(const std::string& message) {
        return send(std::string_view(message));
    }
    
    /**
     * @brief Send a message (C string overload)
     */
    [[nodiscard]] Error send(const char* message) {
        return send(std::string_view(message));
    }
    
    // ========== State Queries ==========
    
    /**
     * @brief Get current state
     */
    State state() const;
    
    /**
     * @brief Check if running
     */
    bool is_running() const { return state() == State::Running; }
    
    /**
     * @brief Check if created (not yet started)
     */
    bool is_created() const { return state() == State::Created; }
    
    /**
     * @brief Check if stopped
     */
    bool is_stopped() const { return state() == State::Stopped; }
    
    /**
     * @brief Get statistics
     */
    Stats stats() const;
    
    /**
     * @brief Get number of active workers
     */
    int worker_count() const;
    
    /**
     * @brief Get number of active sessions
     */
    int session_count() const;
    
    /**
     * @brief Get number of pending requests
     */
    int pending_count() const;
    
    /**
     * @brief Get number of connected clients (TCP/Unix modes)
     */
    int client_count() const;
    
    /**
     * @brief Get poll file descriptor for external event loop integration
     * @return fd or -1 if not available
     */
    int poll_fd() const;
    
    // ========== Callbacks ==========
    
    /**
     * @brief Set message callback
     */
    void on_message(MessageCallback cb);
    
    /**
     * @brief Set error callback
     */
    void on_error(ErrorCallback cb);
    
    /**
     * @brief Set log callback
     */
    void on_log(LogCallback cb);
    
    /**
     * @brief Set worker event callback
     */
    void on_worker(WorkerCallback cb);
    
    /**
     * @brief Set client connect callback (TCP/Unix modes)
     */
    void on_client_connect(ClientConnectCallback cb);
    
    /**
     * @brief Set client disconnect callback (TCP/Unix modes)
     */
    void on_client_disconnect(ClientDisconnectCallback cb);
    
    // ========== Advanced ==========
    
    /**
     * @brief Get raw C handle (for advanced use)
     */
    stdio_bus_t* raw_handle() const;
    
    /**
     * @brief Check if bus is valid (has handle)
     */
    explicit operator bool() const { return impl_ != nullptr; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Builder pattern for Bus construction
 * 
 * @example
 * ```cpp
 * auto bus = stdiobus::BusBuilder()
 *     .config_path("config.json")
 *     .log_level(2)
 *     .on_message([](auto msg) { std::cout << msg << std::endl; })
 *     .build();
 * ```
 */
class BusBuilder {
public:
    BusBuilder() = default;
    
    /// Set config file path
    BusBuilder& config_path(std::string path) {
        options_.config_path = std::move(path);
        return *this;
    }
    
    /// Set inline JSON config
    BusBuilder& config_json(std::string json) {
        options_.config_json = std::move(json);
        return *this;
    }
    
    /// Set log level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR)
    BusBuilder& log_level(int level) {
        options_.log_level = level;
        return *this;
    }
    
    /// Configure TCP listener
    BusBuilder& listen_tcp(std::string host, uint16_t port) {
        options_.listener.mode = ListenMode::Tcp;
        options_.listener.tcp_host = std::move(host);
        options_.listener.tcp_port = port;
        return *this;
    }
    
    /// Configure Unix socket listener
    BusBuilder& listen_unix(std::string path) {
        options_.listener.mode = ListenMode::Unix;
        options_.listener.unix_path = std::move(path);
        return *this;
    }
    
    /// Set message callback
    BusBuilder& on_message(MessageCallback cb) {
        options_.on_message = std::move(cb);
        return *this;
    }
    
    /// Set error callback
    BusBuilder& on_error(ErrorCallback cb) {
        options_.on_error = std::move(cb);
        return *this;
    }
    
    /// Set log callback
    BusBuilder& on_log(LogCallback cb) {
        options_.on_log = std::move(cb);
        return *this;
    }
    
    /// Set worker event callback
    BusBuilder& on_worker(WorkerCallback cb) {
        options_.on_worker = std::move(cb);
        return *this;
    }
    
    /// Build the Bus instance
    Bus build() {
        return Bus(std::move(options_));
    }

private:
    Options options_;
};

} // namespace stdiobus

#endif // STDIOBUS_BUS_HPP
