/**
 * @file stdiobus.hpp
 * @brief C++ SDK for stdio_bus - AI agent transport layer
 * 
 * This is the main include file. It provides:
 * - Thin C++ wrapper (1:1 over C API)
 * - Idiomatic C++ facade with RAII
 * 
 * @example
 * ```cpp
 * #include <stdiobus.hpp>
 * 
 * int main() {
 *     stdiobus::Bus bus("config.json");
 *     
 *     bus.on_message([](std::string_view msg) {
 *         std::cout << "Received: " << msg << std::endl;
 *     });
 *     
 *     if (auto err = bus.start(); err) {
 *         std::cerr << "Failed to start: " << err.message() << std::endl;
 *         return 1;
 *     }
 *     
 *     bus.send(R"({"jsonrpc":"2.0","method":"echo","id":1})");
 *     
 *     while (bus.is_running()) {
 *         bus.step(100);  // 100ms timeout
 *     }
 *     
 *     return 0;
 * }
 * ```
 */

#ifndef STDIOBUS_HPP
#define STDIOBUS_HPP

#include <stdiobus/error.hpp>
#include <stdiobus/types.hpp>
#include <stdiobus/bus.hpp>

// Optional: thin wrapper for direct C API access
#include <stdiobus/ffi.hpp>

// Optional: async adaptor with std::future
#include <stdiobus/async.hpp>

#endif // STDIOBUS_HPP
