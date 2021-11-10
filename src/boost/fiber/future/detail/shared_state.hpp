
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_SHARED_STATE_H
#define BOOST_FIBERS_DETAIL_SHARED_STATE_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <type_traits>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/future/future_status.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class shared_state_base {
private:
    std::atomic< std::size_t >  use_count_{ 0 };
    mutable condition_variable  waiters_{};

protected:
    mutable mutex       mtx_{};
    bool                ready_{ false };
    std::exception_ptr  except_{};

    void mark_ready_and_notify_( std::unique_lock< mutex > & lk) noexcept {
        BOOST_ASSERT( lk.owns_lock() );
        ready_ = true;
        lk.unlock();
        waiters_.notify_all();
    }

    void owner_destroyed_( std::unique_lock< mutex > & lk) {
        BOOST_ASSERT( lk.owns_lock() );
        if ( ! ready_) {
            set_exception_(
                    std::make_exception_ptr( broken_promise() ),
                    lk);
        }
    }

    void set_exception_( std::exception_ptr except, std::unique_lock< mutex > & lk) {
        BOOST_ASSERT( lk.owns_lock() );
        if ( BOOST_UNLIKELY( ready_) ) {
            throw promise_already_satisfied();
        }
        except_ = except;
        mark_ready_and_notify_( lk);
    }

    std::exception_ptr get_exception_ptr_( std::unique_lock< mutex > & lk) {
        BOOST_ASSERT( lk.owns_lock() );
        wait_( lk);
        return except_;
    }

    void wait_( std::unique_lock< mutex > & lk) const {
        BOOST_ASSERT( lk.owns_lock() );
        waiters_.wait( lk, [this](){ return ready_; });
    }

    template< typename Rep, typename Period >
    future_status wait_for_( std::unique_lock< mutex > & lk,
                             std::chrono::duration< Rep, Period > const& timeout_duration) const {
        BOOST_ASSERT( lk.owns_lock() );
        return waiters_.wait_for( lk, timeout_duration, [this](){ return ready_; })
                    ? future_status::ready
                    : future_status::timeout;
    }

    template< typename Clock, typename Duration >
    future_status wait_until_( std::unique_lock< mutex > & lk,
                               std::chrono::time_point< Clock, Duration > const& timeout_time) const {
        BOOST_ASSERT( lk.owns_lock() );
        return waiters_.wait_until( lk, timeout_time, [this](){ return ready_; })
                    ? future_status::ready
                    : future_status::timeout;
    }

    virtual void deallocate_future() noexcept = 0;

public:
    shared_state_base() = default;

    virtual ~shared_state_base() = default;

    shared_state_base( shared_state_base const&) = delete;
    shared_state_base & operator=( shared_state_base const&) = delete;

    void owner_destroyed() {
        std::unique_lock< mutex > lk{ mtx_ };
        owner_destroyed_( lk);
    }

    void set_exception( std::exception_ptr except) {
        std::unique_lock< mutex > lk{ mtx_ };
        set_exception_( except, lk);
    }

    std::exception_ptr get_exception_ptr() {
        std::unique_lock< mutex > lk{ mtx_ };
        return get_exception_ptr_( lk);
    }

    void wait() const {
        std::unique_lock< mutex > lk{ mtx_ };
        wait_( lk);
    }

    template< typename Rep, typename Period >
    future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const {
        std::unique_lock< mutex > lk{ mtx_ };
        return wait_for_( lk, timeout_duration);
    }

    template< typename Clock, typename Duration >
    future_status wait_until( std::chrono::time_point< Clock, Duration > const& timeout_time) const {
        std::unique_lock< mutex > lk{ mtx_ };
        return wait_until_( lk, timeout_time);
    }

    friend inline
    void intrusive_ptr_add_ref( shared_state_base * p) noexcept {
        p->use_count_.fetch_add( 1, std::memory_order_relaxed);
    }

    friend inline
    void intrusive_ptr_release( shared_state_base * p) noexcept {
        if ( 1 == p->use_count_.fetch_sub( 1, std::memory_order_release) ) {
            std::atomic_thread_fence( std::memory_order_acquire);
            p->deallocate_future();
        }
    }
};

template< typename R >
class shared_state : public shared_state_base {
private:
    typename std::aligned_storage< sizeof( R), alignof( R) >::type  storage_{};

    void set_value_( R const& value, std::unique_lock< mutex > & lk) {
        BOOST_ASSERT( lk.owns_lock() );
        if ( BOOST_UNLIKELY( ready_) ) {
            throw promise_already_satisfied{};
        }
        ::new ( static_cast< void * >( std::addressof( storage_) ) ) R( value );
        mark_ready_and_notify_( lk);
    }

    void set_value_( R && value, std::unique_lock< mutex > & lk) {
        BOOST_ASSERT( lk.owns_lock() );
        if ( BOOST_UNLIKELY( ready_) ) {
            throw promise_already_satisfied{};
        }
        ::new ( static_cast< void * >( std::addressof( storage_) ) ) R( std::move( value) );
        mark_ready_and_notify_( lk);
    }

    R & get_( std::unique_lock< mutex > & lk) {
        BOOST_ASSERT( lk.owns_lock() );
        wait_( lk);
        if ( except_) {
            std::rethrow_exception( except_);
        }
        return * reinterpret_cast< R * >( std::addressof( storage_) );
    }

public:
    typedef intrusive_ptr< shared_state >    ptr_type;

    shared_state() = default;

    virtual ~shared_state() {
        if ( ready_ && ! except_) {
            reinterpret_cast< R * >( std::addressof( storage_) )->~R();
        }
    }

    shared_state( shared_state const&) = delete;
    shared_state & operator=( shared_state const&) = delete;

    void set_value( R const& value) {
        std::unique_lock< mutex > lk{ mtx_ };
        set_value_( value, lk);
    }

    void set_value( R && value) {
        std::unique_lock< mutex > lk{ mtx_ };
        set_value_( std::move( value), lk);
    }

    R & get() {
        std::unique_lock< mutex > lk{ mtx_ };
        return get_( lk);
    }
};

template< typename R >
class shared_state< R & > : public shared_state_base {
private:
    R   *   value_{ nullptr };

    void set_value_( R & value, std::unique_lock< mutex > & lk) {
        BOOST_ASSERT( lk.owns_lock() );
        if ( BOOST_UNLIKELY( ready_) ) {
            throw promise_already_satisfied();
        }
        value_ = std::addressof( value);
        mark_ready_and_notify_( lk);
    }

    R & get_( std::unique_lock< mutex > & lk) {
        BOOST_ASSERT( lk.owns_lock() );
        wait_( lk);
        if ( except_) {
            std::rethrow_exception( except_);
        }
        return * value_;
    }

public:
    typedef intrusive_ptr< shared_state >    ptr_type;

    shared_state() = default;

    virtual ~shared_state() = default;

    shared_state( shared_state const&) = delete;
    shared_state & operator=( shared_state const&) = delete;

    void set_value( R & value) {
        std::unique_lock< mutex > lk{ mtx_ };
        set_value_( value, lk);
    }

    R & get() {
        std::unique_lock< mutex > lk{ mtx_ };
        return get_( lk);
    }
};

template<>
class shared_state< void > : public shared_state_base {
private:
    inline
    void set_value_( std::unique_lock< mutex > & lk) {
        BOOST_ASSERT( lk.owns_lock() );
        if ( BOOST_UNLIKELY( ready_) ) {
            throw promise_already_satisfied();
        }
        mark_ready_and_notify_( lk);
    }

    inline
    void get_( std::unique_lock< mutex > & lk) {
        BOOST_ASSERT( lk.owns_lock() );
        wait_( lk);
        if ( except_) {
            std::rethrow_exception( except_);
        }
    }

public:
    typedef intrusive_ptr< shared_state >    ptr_type;

    shared_state() = default;

    virtual ~shared_state() = default;

    shared_state( shared_state const&) = delete;
    shared_state & operator=( shared_state const&) = delete;

    inline
    void set_value() {
        std::unique_lock< mutex > lk{ mtx_ };
        set_value_( lk);
    }

    inline
    void get() {
        std::unique_lock< mutex > lk{ mtx_ };
        get_( lk);
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_SHARED_STATE_H
