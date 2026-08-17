// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BTCSIGNALS_H
#define BITCOIN_UTIL_BTCSIGNALS_H

#include <sync.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * btcsignals is a simple mechanism for signaling events to multiple subscribers.
 * It is api-compatible with a minimal subset of boost::signals2.
 *
 * Rather than using a custom slot type, and the features/complexity that they
 * imply, std::function is used to store the callbacks. Lifetime management of
 * the callbacks is left up to the user.
 *
 * Disconnecting a connection destroys the associated callback object eagerly
 * (or, if the callback is currently executing on another thread, as soon as
 * the last in-flight invocation completes), matching boost::signals2
 * behavior. This makes callback lifetime predictable when callbacks own
 * resources whose release has side effects, instead of tying it to unrelated
 * future connect() calls and to the lifetimes of connection handles.
 *
 * All usage is thread-safe except for interacting with a connection while
 * copying/moving it on another thread.
 */

namespace btcsignals {

/// The default combiner, which only returns void.
class null_value
{
public:
    using result_type = void;
};

/// A combiner, which checks if at least one callback returned true.
class any_of
{
public:
    // This is the only supported combiner with a non-void return type. As
    // such, its behavior is embedded into the signal functor.
    using result_type = bool;
};

template <typename Signature, typename Combiner = null_value>
class signal;

/*
 * State object representing the liveness of a registered callback.
 * signal::connect() returns an enabled connection which can be held and
 * disabled in the future.
 */
class connection
{
    template <typename Signature, typename Combiner>
    friend class signal;
    /**
     * Track liveness. Also serves as a tag for the constructor used by signal.
     */
    class liveness
    {
        friend class connection;
        std::atomic_bool m_connected{true};

        void disconnect()
        {
            m_connected.store(false);
            // Destroy the owned callback now instead of waiting for the next
            // signal::connect() garbage collection, which may never happen. An
            // invocation that already loaded the owner keeps the callback
            // alive until it returns.
#ifdef __cpp_lib_atomic_shared_ptr
            m_owner.store(nullptr);
#else
            std::atomic_store(&m_owner, std::shared_ptr<const void>{});
#endif
        }

    protected:
        //! Type-erased owner of the callback stored by the derived
        //! signal::connection_holder. Loaded by signal invocation to keep the
        //! callback alive while it runs, cleared by disconnect().
        //!
        //! std::atomic<shared_ptr<T>> is a C++20 specialization (feature-test
        //! macro __cpp_lib_atomic_shared_ptr). Fall back to the C++14 free
        //! functions atomic_load/atomic_store, which are specifically overloaded
        //! for shared_ptr, on implementations that do not provide it.
#ifdef __cpp_lib_atomic_shared_ptr
        std::atomic<std::shared_ptr<const void>> m_owner{};
#else
        std::shared_ptr<const void> m_owner{};
#endif

    public:
        bool connected() const { return m_connected.load(); }
    };

    /**
     * connections have shared_ptr-like copy and move semantics.
     */
    std::shared_ptr<liveness> m_state{};

    /**
     * Only a signal can create an enabled connection.
     */
    explicit connection(std::shared_ptr<liveness>&& state) : m_state{std::move(state)}{}

public:
    /**
     * The default constructor creates a connection with no associated signal
     */
    constexpr connection() noexcept = default;

    /**
     * If a callback is associated with this connection, prevent it from being
     * called in the future.
     *
     * If a connection is disabled as part of a signal's callback function, it
     * will _not_ be executed in the current signal invocation.
     *
     * The callback object is destroyed here (or when the last concurrently
     * executing invocation of it completes). The empty holder entry is not
     * removed from the owning signal; it is garbage collected in
     * signal::connect().
     */
    void disconnect()
    {
        if (m_state) {
            m_state->disconnect();
        }
    }

    /**
     * Returns true if this connection was created by a signal and has not been
     * disabled.
     */
    bool connected() const
    {
        return m_state && m_state->connected();
    }
};

/*
 * RAII-style connection management
 */
class scoped_connection
{
    connection m_conn;

public:
    explicit scoped_connection(connection rhs) noexcept : m_conn{std::move(rhs)} {}

    scoped_connection(scoped_connection&&) noexcept = default;

    /**
     * For simplicity, disable copy construction and copy/move assignment.
     */
    scoped_connection& operator=(scoped_connection&&) = delete;
    scoped_connection& operator=(const scoped_connection&) = delete;
    scoped_connection(const scoped_connection&) = delete;

    void disconnect()
    {
        m_conn.disconnect();
    }

    ~scoped_connection()
    {
        disconnect();
    }
};

/*
 * Functor for calling zero or more connected callbacks
 */
template <typename Signature, typename Combiner>
class signal
{
    using function_type = std::function<Signature>;

    /*
     * Helper struct for maintaining a callback and its associated connection liveness
     */
    struct connection_holder : connection::liveness {
        template <typename Callable>
        connection_holder(Callable&& callback)
        {
            auto owner{std::make_shared<const function_type>(std::forward<Callable>(callback))};
            m_callback = owner.get();
#ifdef __cpp_lib_atomic_shared_ptr
            m_owner.store(std::move(owner));
#else
            std::atomic_store(&m_owner, std::shared_ptr<const void>(std::move(owner)));
#endif
        }

        //! Load the owner, keeping the callback alive while the returned
        //! shared_ptr is held. Returns null if disconnected.
        std::shared_ptr<const void> owner() const
        {
#ifdef __cpp_lib_atomic_shared_ptr
            return m_owner.load();
#else
            return std::atomic_load(&m_owner);
#endif
        }

        //! Raw pointer to the callback, only valid while holding a shared_ptr
        //! returned by owner().
        const function_type* m_callback;
    };

    mutable Mutex m_mutex;

    std::vector<std::shared_ptr<connection_holder>> m_connections GUARDED_BY(m_mutex){};

public:
    using result_type = Combiner::result_type;

    constexpr signal() noexcept = default;
    ~signal() = default;

    /*
     * For simplicity, disable all moving/copying/assigning.
     */
    signal(const signal&) = delete;
    signal(signal&&) = delete;
    signal& operator=(const signal&) = delete;
    signal& operator=(signal&&) = delete;

    /*
     * Execute all enabled callbacks for the signal. Rather than allowing for
     * custom combiners, the behavior of any_of is hard-coded here.
     *
     * Callbacks which return void require special handling.
     *
     * In order to avoid locking during the callbacks, the list of callbacks is
     * cached before they are called. This allows a callback to call connect(),
     * but the newly connected callback will not be run during the current
     * signal invocation.
     *
     * Note that the parameters are accepted as universal references, though
     * they are not perfectly forwarded as that could cause a use-after-move if
     * more than one callback is enabled.
     */
    template <typename... Args>
    [[nodiscard]] result_type operator()(Args&&... args) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        std::vector<std::shared_ptr<connection_holder>> connections;
        {
            LOCK(m_mutex);
            connections = m_connections;
        }
        if constexpr (std::is_void_v<result_type>) {
            static_assert(std::is_same_v<result_type, typename function_type::result_type>,
                          "Callback result type must be equal to the combiner result type (void).");
            for (const auto& connection : connections) {
                // Loading the owner both checks that the connection is still
                // connected and keeps the callback alive for the duration of
                // the call if it is disconnected concurrently.
                if (const auto owner{connection->owner()}) {
                    (*connection->m_callback)(args...);
                }
            }
        } else {
            static_assert(std::is_same_v<Combiner, any_of>,
                          "only the any_of combiner is supported and hard-coded into this functor.");
            static_assert(std::is_same_v<result_type, typename function_type::result_type>,
                          "Callback result type must be equal to the combiner result type (bool).");
            result_type ret{false};
            for (const auto& connection : connections) {
                if (const auto owner{connection->owner()}) {
                    ret |= (*connection->m_callback)(args...);
                }
            }
            return ret;
        }
    }

    /*
     * Connect a new callback to the signal. A forwarding callable accepts
     * anything that can be stored in a std::function.
     */
    template <typename Callable>
    connection connect(Callable&& func) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);

        // Garbage-collect disconnected connections to prevent unbounded growth
        std::erase_if(m_connections, [](const auto& holder) { return !holder->connected(); });

        const auto& entry = m_connections.emplace_back(std::make_shared<connection_holder>(std::forward<Callable>(func)));
        return connection(entry);
    }

    /*
     * Returns true if there are no enabled callbacks
     */
    [[nodiscard]] bool empty() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        return std::ranges::none_of(m_connections, [](const auto& holder) {
            return holder->connected();
        });
    }
};

} // namespace btcsignals

#endif // BITCOIN_UTIL_BTCSIGNALS_H
