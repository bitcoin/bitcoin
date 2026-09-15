// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_TYPES_H
#define BITCOIN_INTERFACES_TYPES_H

#include <uint256.h>

#include <functional>
#include <utility>

namespace interfaces {

//! Hash/height pair to help track and identify blocks.
struct BlockRef {
    uint256 hash;
    int height = -1;
};

//! Called to cancel a blocking method.
using CancelFn = std::function<void()>;

//! RAII handle returned when a method registers a `CancelFn` through a
//! `CancelArg`. When destroyed, this guard invokes a function that destroys
//! the registered `CancelFn`. This prevents the server from invoking the
//! registered `CancelFn` after the wrapped method returns and its local
//! variables have been destroyed.
class [[nodiscard]] CancelGuard
{
public:
    CancelGuard() = default;
    explicit CancelGuard(std::function<void()> unregister) : m_unregister(std::move(unregister)) {}
    CancelGuard(CancelGuard&& other) noexcept : m_unregister(std::exchange(other.m_unregister, nullptr)) {}
    CancelGuard(const CancelGuard&) = delete;
    CancelGuard& operator=(const CancelGuard&) = delete;
    CancelGuard& operator=(CancelGuard&& other) noexcept
    {
        if (this != &other) {
            if (m_unregister) m_unregister();
            m_unregister = std::exchange(other.m_unregister, nullptr);
        }
        return *this;
    }
    ~CancelGuard()
    {
        if (m_unregister) m_unregister();
    }

private:
    std::function<void()> m_unregister;
};

//! Blocking methods that declare a `CancelArg` parameter allow callers to
//! cancel long-running operations.
//!
//! Inside the wrapped method (SERVER), passing a `CancelFn` callback registers
//! it to run when cancellation is detected, either because the caller invoked
//! `CancelFn` or because the caller dropped the promise (disconnected or
//! intentionally) and no longer needs the result. The callback remains
//! registered only while the returned `CancelGuard` is alive.
//!
//! As a caller (CLIENT), the `CancelFn` can be taken and then invoked from
//! another thread to interrupt the blocking method. In a single process, the
//! method then returns early. In multi-process, the call raises
//! `InterruptException` instead and the server's result is discarded.
using CancelArg = std::function<CancelGuard(CancelFn)>;

} // namespace interfaces

#endif // BITCOIN_INTERFACES_TYPES_H
