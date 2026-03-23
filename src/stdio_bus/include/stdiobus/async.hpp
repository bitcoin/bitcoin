/**
 * @file async.hpp
 * @brief Optional async adaptor using std::future
 * 
 * Provides request_async() for promise-based async operations.
 * This is a thin layer over the synchronous API.
 */

#ifndef STDIOBUS_ASYNC_HPP
#define STDIOBUS_ASYNC_HPP

#include <stdiobus/bus.hpp>
#include <stdiobus/error.hpp>

#include <future>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>

namespace stdiobus {

/**
 * @brief Async result type
 */
struct AsyncResult {
    Error error;
    std::string response;
    
    explicit operator bool() const noexcept { return !error; }
};

/**
 * @brief Async wrapper for Bus
 * 
 * Adds request_async() which returns std::future<AsyncResult>.
 * Requires calling pump() regularly to process responses.
 * 
 * @example
 * ```cpp
 * stdiobus::AsyncBus bus("config.json");
 * bus.start();
 * 
 * auto future = bus.request_async(R"({"jsonrpc":"2.0","method":"test","id":1})");
 * 
 * // Pump until ready
 * while (future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
 *     bus.pump(std::chrono::milliseconds(10));
 * }
 * 
 * auto result = future.get();
 * if (result) {
 *     std::cout << "Response: " << result.response << std::endl;
 * }
 * ```
 */
class AsyncBus {
public:
    explicit AsyncBus(std::string_view config_path)
        : bus_(config_path), next_id_(1) {
        setup_message_handler();
    }
    
    explicit AsyncBus(Options options)
        : bus_(std::move(options)), next_id_(1) {
        setup_message_handler();
    }
    
    // Non-copyable, movable
    AsyncBus(const AsyncBus&) = delete;
    AsyncBus& operator=(const AsyncBus&) = delete;
    AsyncBus(AsyncBus&&) = default;
    AsyncBus& operator=(AsyncBus&&) = default;
    
    /// Start the bus
    [[nodiscard]] Error start() { return bus_.start(); }
    
    /// Stop the bus
    [[nodiscard]] Error stop(std::chrono::seconds timeout = std::chrono::seconds(5)) {
        return bus_.stop(timeout);
    }
    
    /// Process I/O and resolve pending futures
    int pump(Duration timeout = Duration::zero()) {
        return bus_.step(timeout);
    }
    
    /// Send async request, returns future
    std::future<AsyncResult> request_async(std::string message, 
                                           std::chrono::milliseconds timeout = std::chrono::seconds(30)) {
        auto promise = std::make_shared<std::promise<AsyncResult>>();
        auto future = promise->get_future();
        
        // Extract or inject request ID
        std::string id = extract_or_inject_id(message);
        
        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            pending_[id] = PendingRequest{
                std::move(promise),
                std::chrono::steady_clock::now() + timeout
            };
        }
        
        if (auto err = bus_.send(message); err) {
            // Send failed, resolve immediately
            std::lock_guard<std::mutex> lock(pending_mutex_);
            auto it = pending_.find(id);
            if (it != pending_.end()) {
                it->second.promise->set_value(AsyncResult{err, ""});
                pending_.erase(it);
            }
        }
        
        return future;
    }
    
    /// Send notification (no response expected)
    [[nodiscard]] Error notify(std::string_view message) {
        return bus_.send(message);
    }
    
    /// Get underlying bus
    Bus& bus() { return bus_; }
    const Bus& bus() const { return bus_; }
    
    /// Check for timed out requests
    void check_timeouts() {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(pending_mutex_);
        
        for (auto it = pending_.begin(); it != pending_.end(); ) {
            if (now >= it->second.deadline) {
                it->second.promise->set_value(AsyncResult{
                    Error(ErrorCode::Timeout, "Request timed out"),
                    ""
                });
                it = pending_.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    struct PendingRequest {
        std::shared_ptr<std::promise<AsyncResult>> promise;
        std::chrono::steady_clock::time_point deadline;
    };
    
    Bus bus_;
    std::atomic<uint64_t> next_id_;
    std::unordered_map<std::string, PendingRequest> pending_;
    std::mutex pending_mutex_;
    
    void setup_message_handler() {
        bus_.on_message([this](std::string_view msg) {
            handle_response(msg);
        });
    }
    
    void handle_response(std::string_view msg) {
        // Simple JSON parsing for "id" field
        std::string id = extract_id(msg);
        if (id.empty()) return;  // Notification, no pending request
        
        std::lock_guard<std::mutex> lock(pending_mutex_);
        auto it = pending_.find(id);
        if (it != pending_.end()) {
            it->second.promise->set_value(AsyncResult{Error::ok(), std::string(msg)});
            pending_.erase(it);
        }
    }
    
    std::string extract_id(std::string_view json) {
        // Simple extraction - find "id": and parse value
        auto pos = json.find("\"id\"");
        if (pos == std::string_view::npos) return "";
        
        pos = json.find(':', pos);
        if (pos == std::string_view::npos) return "";
        
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        
        if (pos >= json.size()) return "";
        
        if (json[pos] == '"') {
            // String ID
            pos++;
            auto end = json.find('"', pos);
            if (end == std::string_view::npos) return "";
            return std::string(json.substr(pos, end - pos));
        } else if (std::isdigit(json[pos]) || json[pos] == '-') {
            // Numeric ID
            auto start = pos;
            while (pos < json.size() && (std::isdigit(json[pos]) || json[pos] == '-')) pos++;
            return std::string(json.substr(start, pos - start));
        }
        
        return "";
    }
    
    std::string extract_or_inject_id(std::string& json) {
        std::string id = extract_id(json);
        if (!id.empty()) return id;
        
        // Inject ID
        id = std::to_string(next_id_++);
        
        // Find position to inject (after opening brace)
        auto pos = json.find('{');
        if (pos != std::string::npos) {
            json.insert(pos + 1, "\"id\":" + id + ",");
        }
        
        return id;
    }
};

} // namespace stdiobus

#endif // STDIOBUS_ASYNC_HPP
