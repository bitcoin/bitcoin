
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_UNBUFFERED_CHANNEL_H
#define BOOST_FIBERS_UNBUFFERED_CHANNEL_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <boost/config.hpp>

#include <boost/fiber/channel_op_status.hpp>
#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/convert.hpp>
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
#include <boost/fiber/detail/exchange.hpp>
#endif
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/waker.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

template< typename T >
class unbuffered_channel {
public:
    using value_type = typename std::remove_reference<T>::type;

private:
    struct slot {
        value_type  value;
        waker       w;

        slot( value_type const& value_, waker && w) :
            value{ value_ },
            w{ std::move(w) } {
        }

        slot( value_type && value_, waker && w) :
            value{ std::move( value_) },
            w{ std::move(w) } {
        }
    };

    // shared cacheline
    std::atomic< slot * >       slot_{ nullptr };
    // shared cacheline
    std::atomic_bool            closed_{ false };
    mutable detail::spinlock    splk_producers_{};
    wait_queue                  waiting_producers_{};
    mutable detail::spinlock    splk_consumers_{};
    wait_queue                  waiting_consumers_{};
    char                        pad_[cacheline_length];

    bool is_empty_() {
        return nullptr == slot_.load( std::memory_order_acquire);
    }

    bool try_push_( slot * own_slot) {
        for (;;) {
            slot * s = slot_.load( std::memory_order_acquire);
            if ( nullptr == s) {
                if ( ! slot_.compare_exchange_strong( s, own_slot, std::memory_order_acq_rel) ) {
                    continue;
                }
                return true;
            }
            return false;
        }
    }

    slot * try_pop_() {
        slot * nil_slot = nullptr;
        for (;;) {
            slot * s = slot_.load( std::memory_order_acquire);
            if ( nullptr != s) {
                if ( ! slot_.compare_exchange_strong( s, nil_slot, std::memory_order_acq_rel) ) {
                    continue;}
            }
            return s;
        }
    }

public:
    unbuffered_channel() = default;

    ~unbuffered_channel() {
        close();
    }

    unbuffered_channel( unbuffered_channel const&) = delete;
    unbuffered_channel & operator=( unbuffered_channel const&) = delete;

    bool is_closed() const noexcept {
        return closed_.load( std::memory_order_acquire);
    }

    void close() noexcept {
        // set flag
        if ( ! closed_.exchange( true, std::memory_order_acquire) ) {
            // notify current waiting  
            slot * s = slot_.load( std::memory_order_acquire);
            if ( nullptr != s) {
                // notify context
                s->w.wake();
            }
            detail::spinlock_lock lk1{ splk_producers_ };
            waiting_producers_.notify_all();

            detail::spinlock_lock lk2{ splk_consumers_ };
            waiting_consumers_.notify_all();
        }
    }

    channel_op_status push( value_type const& value) {
        context * active_ctx = context::active();
        slot s{ value, {} };
        for (;;) {
            if ( BOOST_UNLIKELY( is_closed() ) ) {
                return channel_op_status::closed;
            }
            s.w = active_ctx->create_waker();
            if ( try_push_( & s) ) {
                detail::spinlock_lock lk{ splk_consumers_ };
                waiting_consumers_.notify_one();
                // suspend till value has been consumed
                active_ctx->suspend( lk);
                // resumed
                if ( BOOST_UNLIKELY( is_closed() ) ) {
                    // channel was closed before value was consumed
                    return channel_op_status::closed;
                }
                // value has been consumed
                return channel_op_status::success;
            }
            detail::spinlock_lock lk{ splk_producers_ };
            if ( BOOST_UNLIKELY( is_closed() ) ) {
                return channel_op_status::closed;
            }
            if ( is_empty_() ) {
                continue;
            }

            waiting_producers_.suspend_and_wait( lk, active_ctx);
            // resumed, slot mabye free
        }
    }

    channel_op_status push( value_type && value) {
        context * active_ctx = context::active();
        slot s{ std::move( value), {} };
        for (;;) {
            if ( BOOST_UNLIKELY( is_closed() ) ) {
                return channel_op_status::closed;
            }
            s.w = active_ctx->create_waker();
            if ( try_push_( & s) ) {
                detail::spinlock_lock lk{ splk_consumers_ };
                waiting_consumers_.notify_one();
                // suspend till value has been consumed
                active_ctx->suspend( lk);
                // resumed
                if ( BOOST_UNLIKELY( is_closed() ) ) {
                    // channel was closed before value was consumed
                    return channel_op_status::closed;
                }
                // value has been consumed
                return channel_op_status::success;
            }
            detail::spinlock_lock lk{ splk_producers_ };
            if ( BOOST_UNLIKELY( is_closed() ) ) {
                return channel_op_status::closed;
            }
            if ( is_empty_() ) {
                continue;
            }
            waiting_producers_.suspend_and_wait( lk, active_ctx);
            // resumed, slot mabye free
        }
    }

    template< typename Rep, typename Period >
    channel_op_status push_wait_for( value_type const& value,
                                     std::chrono::duration< Rep, Period > const& timeout_duration) {
        return push_wait_until( value,
                                std::chrono::steady_clock::now() + timeout_duration);
    }

    template< typename Rep, typename Period >
    channel_op_status push_wait_for( value_type && value,
                                     std::chrono::duration< Rep, Period > const& timeout_duration) {
        return push_wait_until( std::forward< value_type >( value),
                                std::chrono::steady_clock::now() + timeout_duration);
    }

    template< typename Clock, typename Duration >
    channel_op_status push_wait_until( value_type const& value,
                                       std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        context * active_ctx = context::active();
        slot s{ value, {} };
        std::chrono::steady_clock::time_point timeout_time = detail::convert( timeout_time_);
        for (;;) {
            if ( BOOST_UNLIKELY( is_closed() ) ) {
                return channel_op_status::closed;
            }
            s.w = active_ctx->create_waker();
            if ( try_push_( & s) ) {
                detail::spinlock_lock lk{ splk_consumers_ };
                waiting_consumers_.notify_one();
                // suspend this producer
                if ( ! active_ctx->wait_until(timeout_time, lk, waker(s.w))) {
                    // clear slot
                    slot * nil_slot = nullptr, * own_slot = & s;
                    slot_.compare_exchange_strong( own_slot, nil_slot, std::memory_order_acq_rel);
                    // resumed, value has not been consumed
                    return channel_op_status::timeout;
                }
                // resumed
                if ( BOOST_UNLIKELY( is_closed() ) ) {
                    // channel was closed before value was consumed
                    return channel_op_status::closed;
                }
                // value has been consumed
                return channel_op_status::success;
            }
            detail::spinlock_lock lk{ splk_producers_ };
            if ( BOOST_UNLIKELY( is_closed() ) ) {
                return channel_op_status::closed;
            }
            if ( is_empty_() ) {
                continue;
            }

            if (! waiting_producers_.suspend_and_wait_until( lk, active_ctx, timeout_time))
            {
                return channel_op_status::timeout;
            }
            // resumed, slot maybe free
        }
    }

    template< typename Clock, typename Duration >
    channel_op_status push_wait_until( value_type && value,
                                       std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        context * active_ctx = context::active();
        slot s{ std::move( value), {} };
        std::chrono::steady_clock::time_point timeout_time = detail::convert( timeout_time_);
        for (;;) {
            if ( BOOST_UNLIKELY( is_closed() ) ) {
                return channel_op_status::closed;
            }
            s.w = active_ctx->create_waker();
            if ( try_push_( & s) ) {
                detail::spinlock_lock lk{ splk_consumers_ };
                waiting_consumers_.notify_one();
                // suspend this producer
                if ( ! active_ctx->wait_until(timeout_time, lk, waker(s.w))) {
                    // clear slot
                    slot * nil_slot = nullptr, * own_slot = & s;
                    slot_.compare_exchange_strong( own_slot, nil_slot, std::memory_order_acq_rel);
                    // resumed, value has not been consumed
                    return channel_op_status::timeout;
                }
                // resumed
                if ( BOOST_UNLIKELY( is_closed() ) ) {
                    // channel was closed before value was consumed
                    return channel_op_status::closed;
                }
                // value has been consumed
                return channel_op_status::success;
            }
            detail::spinlock_lock lk{ splk_producers_ };
            if ( BOOST_UNLIKELY( is_closed() ) ) {
                return channel_op_status::closed;
            }
            if ( is_empty_() ) {
                continue;
            }
            if (! waiting_producers_.suspend_and_wait_until( lk, active_ctx, timeout_time))
            {
                return channel_op_status::timeout;
            }
            // resumed, slot maybe free
        }
    }

    channel_op_status pop( value_type & value) {
        context * active_ctx = context::active();
        slot * s = nullptr;
        for (;;) {
            if ( nullptr != ( s = try_pop_() ) ) {
                {
                    detail::spinlock_lock lk{ splk_producers_ };
                    waiting_producers_.notify_one();
                }
                value = std::move( s->value);
                // notify context
                s->w.wake();
                return channel_op_status::success;
            }
            detail::spinlock_lock lk{ splk_consumers_ };
            if ( BOOST_UNLIKELY( is_closed() ) ) {
                return channel_op_status::closed;
            }
            if ( ! is_empty_() ) {
                continue;
            }
            waiting_consumers_.suspend_and_wait( lk, active_ctx);
            // resumed, slot mabye set
        }
    }

    value_type value_pop() {
        context * active_ctx = context::active();
        slot * s = nullptr;
        for (;;) {
            if ( nullptr != ( s = try_pop_() ) ) {
                {
                    detail::spinlock_lock lk{ splk_producers_ };
                    waiting_producers_.notify_one();
                }
                // consume value
                value_type value = std::move( s->value);
                // notify context
                s->w.wake();
                return std::move( value);
            }
            detail::spinlock_lock lk{ splk_consumers_ };
            if ( BOOST_UNLIKELY( is_closed() ) ) {
                throw fiber_error{
                        std::make_error_code( std::errc::operation_not_permitted),
                        "boost fiber: channel is closed" };
            }
            if ( ! is_empty_() ) {
                continue;
            }
            waiting_consumers_.suspend_and_wait( lk, active_ctx);
            // resumed, slot mabye set
        }
    }

    template< typename Rep, typename Period >
    channel_op_status pop_wait_for( value_type & value,
                                    std::chrono::duration< Rep, Period > const& timeout_duration) {
        return pop_wait_until( value,
                               std::chrono::steady_clock::now() + timeout_duration);
    }

    template< typename Clock, typename Duration >
    channel_op_status pop_wait_until( value_type & value,
                                      std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        context * active_ctx = context::active();
        slot * s = nullptr;
        std::chrono::steady_clock::time_point timeout_time = detail::convert( timeout_time_);
        for (;;) {
            if ( nullptr != ( s = try_pop_() ) ) {
                {
                    detail::spinlock_lock lk{ splk_producers_ };
                    waiting_producers_.notify_one();
                }
                // consume value
                value = std::move( s->value);
                // notify context
                s->w.wake();
                return channel_op_status::success;
            }
            detail::spinlock_lock lk{ splk_consumers_ };
            if ( BOOST_UNLIKELY( is_closed() ) ) {
                return channel_op_status::closed;
            }
            if ( ! is_empty_() ) {
                continue;
            }
            if ( ! waiting_consumers_.suspend_and_wait_until( lk, active_ctx, timeout_time)) {
                return channel_op_status::timeout;
            }
        }
    }

    class iterator {
    private:
        typedef typename std::aligned_storage< sizeof( value_type), alignof( value_type) >::type  storage_type;

        unbuffered_channel  *   chan_{ nullptr };
        storage_type            storage_;

        void increment_( bool initial = false) {
            BOOST_ASSERT( nullptr != chan_);
            try {
                if ( ! initial) {
                    reinterpret_cast< value_type * >( std::addressof( storage_) )->~value_type();
                }
                ::new ( static_cast< void * >( std::addressof( storage_) ) ) value_type{ chan_->value_pop() };
            } catch ( fiber_error const&) {
                chan_ = nullptr;
            }
        }

    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

        using pointer_t = pointer;
        using reference_t = reference;

        iterator() noexcept = default;

        explicit iterator( unbuffered_channel< T > * chan) noexcept :
            chan_{ chan } {
            increment_( true);
        }

        iterator( iterator const& other) noexcept :
            chan_{ other.chan_ } {
        }

        iterator & operator=( iterator const& other) noexcept {
            if ( this == & other) return * this;
            chan_ = other.chan_;
            return * this;
        }

        bool operator==( iterator const& other) const noexcept {
            return other.chan_ == chan_;
        }

        bool operator!=( iterator const& other) const noexcept {
            return other.chan_ != chan_;
        }

        iterator & operator++() {
            reinterpret_cast< value_type * >( std::addressof( storage_) )->~value_type();
            increment_();
            return * this;
        }

        const iterator operator++( int) = delete;

        reference_t operator*() noexcept {
            return * reinterpret_cast< value_type * >( std::addressof( storage_) );
        }

        pointer_t operator->() noexcept {
            return reinterpret_cast< value_type * >( std::addressof( storage_) );
        }
    };

    friend class iterator;
};

template< typename T >
typename unbuffered_channel< T >::iterator
begin( unbuffered_channel< T > & chan) {
    return typename unbuffered_channel< T >::iterator( & chan);
}

template< typename T >
typename unbuffered_channel< T >::iterator
end( unbuffered_channel< T > &) {
    return typename unbuffered_channel< T >::iterator();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_UNBUFFERED_CHANNEL_H
