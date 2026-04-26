// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BTCSIGNALS_H
#define BITCOIN_BTCSIGNALS_H

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
 * All usage is thread-safe except for interacting with a connection while
 * copying/moving it on another thread.
 */

namespace btcsignals {

/*
 * optional_last_value is the default and only supported combiner.
 * As such, its behavior is embedded into the signal functor.
 *
 * Because optional<void> is undefined, void must be special-cased.
 */

template <typename T>
class optional_last_value
{
public:
    using result_type = std::conditional_t<std::is_void_v<T>, void, std::optional<T>>;
};

template <typename Signature, typename Combiner = optional_last_value<typename std::function<Signature>::result_type>>
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

        void disconnect() { m_connected.store(false); }
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
     * Note that disconnected callbacks are not removed from their owning
     * signals here. They are garbage collected in signal::connect().
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
    scoped_connection(connection rhs) noexcept : m_conn{std::move(rhs)} {}

    scoped_connection(scoped_connection&&) noexcept = default;
    scoped_connection& operator=(scoped_connection&&) noexcept = default;

    /**
     * For simplicity, disable copy assignment and construction.
     */
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

    static_assert(std::is_same_v<Combiner, optional_last_value<typename function_type::result_type>>, "only the optional_last_value combiner is supported");

    /*
     * Helper struct for maintaining a callback and its associated connection liveness
     */
    struct connection_holder : connection::liveness {
        template <typename Callable>
        connection_holder(Callable&& callback) : m_callback{std::forward<Callable>(callback)}
        {
        }

        const function_type m_callback;
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
     * custom combiners, the behavior of optional_last_value is hard-coded
     * here. Return the value of the last executed callback, or nullopt if none
     * were executed.
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
    result_type operator()(Args&&... args) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        std::vector<std::shared_ptr<connection_holder>> connections;
        {
            LOCK(m_mutex);
            connections = m_connections;
        }
        if constexpr (std::is_void_v<result_type>) {
            for (const auto& connection : connections) {
                if (connection->connected()) {
                    connection->m_callback(args...);
                }
            }
        } else {
            result_type ret{std::nullopt};
            for (const auto& connection : connections) {
                if (connection->connected()) {
                    ret.emplace(connection->m_callback(args...));
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

#endif // BITCOIN_BTCSIGNALS_H
