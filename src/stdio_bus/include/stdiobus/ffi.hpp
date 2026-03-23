/**
 * @file ffi.hpp
 * @brief Thin C++ wrapper over C API (1:1 mapping)
 * 
 * This provides direct access to the C API with minimal C++ conveniences.
 * For idiomatic C++ usage, use stdiobus::Bus instead.
 */

#ifndef STDIOBUS_FFI_HPP
#define STDIOBUS_FFI_HPP

#include <stdiobus/error.hpp>
#include <stdiobus/types.hpp>

// Include C header
extern "C" {
#include <stdio_bus_embed.h>
}

namespace stdiobus {
namespace ffi {

/**
 * @brief Thin wrapper over stdio_bus_t* handle
 * 
 * This is a non-owning wrapper. Use stdiobus::Bus for RAII.
 */
class Handle {
public:
    Handle() noexcept : handle_(nullptr) {}
    explicit Handle(stdio_bus_t* h) noexcept : handle_(h) {}
    
    stdio_bus_t* get() const noexcept { return handle_; }
    explicit operator bool() const noexcept { return handle_ != nullptr; }
    
    // Direct C API wrappers (1:1 mapping)
    
    int start() noexcept {
        return stdio_bus_start(handle_);
    }
    
    int step(int timeout_ms) noexcept {
        return stdio_bus_step(handle_, timeout_ms);
    }
    
    int stop(int timeout_sec) noexcept {
        return stdio_bus_stop(handle_, timeout_sec);
    }
    
    int ingest(const char* msg, size_t len) noexcept {
        return stdio_bus_ingest(handle_, msg, len);
    }
    
    stdio_bus_state_t get_state() const noexcept {
        return stdio_bus_get_state(handle_);
    }
    
    int worker_count() const noexcept {
        return stdio_bus_worker_count(handle_);
    }
    
    int session_count() const noexcept {
        return stdio_bus_session_count(handle_);
    }
    
    int pending_count() const noexcept {
        return stdio_bus_pending_count(handle_);
    }
    
    int client_count() const noexcept {
        return stdio_bus_client_count(handle_);
    }
    
    int get_poll_fd() const noexcept {
        return stdio_bus_get_poll_fd(handle_);
    }
    
    void get_stats(stdio_bus_stats_t* stats) const noexcept {
        stdio_bus_get_stats(handle_, stats);
    }

private:
    stdio_bus_t* handle_;
};

/**
 * @brief Create a new stdio_bus instance (thin wrapper)
 * 
 * @param options C API options struct
 * @return Handle (may be null on error)
 */
inline Handle create(const stdio_bus_options_t* options) noexcept {
    return Handle(stdio_bus_create(options));
}

/**
 * @brief Destroy a stdio_bus instance
 */
inline void destroy(Handle& h) noexcept {
    if (h) {
        stdio_bus_destroy(h.get());
    }
}

/**
 * @brief Convert C state to C++ State enum
 */
inline State to_state(stdio_bus_state_t s) noexcept {
    return static_cast<State>(s);
}

/**
 * @brief Convert C stats to C++ Stats struct
 */
inline Stats to_stats(const stdio_bus_stats_t& s) noexcept {
    return Stats{
        s.messages_in,
        s.messages_out,
        s.bytes_in,
        s.bytes_out,
        s.worker_restarts,
        s.routing_errors,
        s.client_connects,
        s.client_disconnects
    };
}

} // namespace ffi
} // namespace stdiobus

#endif // STDIOBUS_FFI_HPP
