
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_CONDITION_VARIABLE_H
#define BOOST_FIBERS_CONDITION_VARIABLE_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/detail/config.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/convert.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/operations.hpp>
#include <boost/fiber/waker.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#ifdef _MSC_VER
# pragma warning(push)
//# pragma warning(disable:4251)
#endif

namespace boost {
namespace fibers {

enum class cv_status {
    no_timeout = 1,
    timeout
};

class BOOST_FIBERS_DECL condition_variable_any {
private:
    detail::spinlock    wait_queue_splk_{};
    wait_queue          wait_queue_{};

public:
    condition_variable_any() = default;

    ~condition_variable_any() {
        BOOST_ASSERT( wait_queue_.empty() );
    }

    condition_variable_any( condition_variable_any const&) = delete;
    condition_variable_any & operator=( condition_variable_any const&) = delete;

    void notify_one() noexcept;

    void notify_all() noexcept;

    template< typename LockType >
    void wait( LockType & lt) {
        context * active_ctx = context::active();
        // atomically call lt.unlock() and block on *this
        // store this fiber in waiting-queue
        detail::spinlock_lock lk{ wait_queue_splk_ };
        lt.unlock();
        wait_queue_.suspend_and_wait( lk, active_ctx);

        // relock external again before returning
        try {
            lt.lock();
#if defined(BOOST_CONTEXT_HAS_CXXABI_H)
        } catch ( abi::__forced_unwind const&) {
            throw;
#endif
        } catch (...) {
            std::terminate();
        }
    }

    template< typename LockType, typename Pred >
    void wait( LockType & lt, Pred pred) {
        while ( ! pred() ) {
            wait( lt);
        }
    }

    template< typename LockType, typename Clock, typename Duration >
    cv_status wait_until( LockType & lt, std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        context * active_ctx = context::active();
        cv_status status = cv_status::no_timeout;
        std::chrono::steady_clock::time_point timeout_time = detail::convert( timeout_time_);
        // atomically call lt.unlock() and block on *this
        // store this fiber in waiting-queue
        detail::spinlock_lock lk{ wait_queue_splk_ };
        // unlock external lt
        lt.unlock();
        if ( ! wait_queue_.suspend_and_wait_until( lk, active_ctx, timeout_time)) {
            status = cv_status::timeout;
        }
        // relock external again before returning
        try {
            lt.lock();
#if defined(BOOST_CONTEXT_HAS_CXXABI_H)
        } catch ( abi::__forced_unwind const&) {
            throw;
#endif
        } catch (...) {
            std::terminate();
        }
        return status;
    }

    template< typename LockType, typename Clock, typename Duration, typename Pred >
    bool wait_until( LockType & lt,
                     std::chrono::time_point< Clock, Duration > const& timeout_time, Pred pred) {
        while ( ! pred() ) {
            if ( cv_status::timeout == wait_until( lt, timeout_time) ) {
                return pred();
            }
        }
        return true;
    }

    template< typename LockType, typename Rep, typename Period >
    cv_status wait_for( LockType & lt, std::chrono::duration< Rep, Period > const& timeout_duration) {
        return wait_until( lt,
                           std::chrono::steady_clock::now() + timeout_duration);
    }

    template< typename LockType, typename Rep, typename Period, typename Pred >
    bool wait_for( LockType & lt, std::chrono::duration< Rep, Period > const& timeout_duration, Pred pred) {
        return wait_until( lt,
                           std::chrono::steady_clock::now() + timeout_duration,
                           pred);
    }
};

class BOOST_FIBERS_DECL condition_variable {
private:
    condition_variable_any      cnd_;

public:
    condition_variable() = default;

    condition_variable( condition_variable const&) = delete;
    condition_variable & operator=( condition_variable const&) = delete;

    void notify_one() noexcept {
        cnd_.notify_one();
    }

    void notify_all() noexcept {
        cnd_.notify_all();
    }

    void wait( std::unique_lock< mutex > & lt) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        cnd_.wait( lt);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
    }

    template< typename Pred >
    void wait( std::unique_lock< mutex > & lt, Pred pred) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        cnd_.wait( lt, pred);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
    }

    template< typename Clock, typename Duration >
    cv_status wait_until( std::unique_lock< mutex > & lt,
                          std::chrono::time_point< Clock, Duration > const& timeout_time) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        cv_status result = cnd_.wait_until( lt, timeout_time);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        return result;
    }

    template< typename Clock, typename Duration, typename Pred >
    bool wait_until( std::unique_lock< mutex > & lt,
                     std::chrono::time_point< Clock, Duration > const& timeout_time, Pred pred) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        bool result = cnd_.wait_until( lt, timeout_time, pred);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        return result;
    }

    template< typename Rep, typename Period >
    cv_status wait_for( std::unique_lock< mutex > & lt,
                        std::chrono::duration< Rep, Period > const& timeout_duration) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        cv_status result = cnd_.wait_for( lt, timeout_duration);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        return result;
    }

    template< typename Rep, typename Period, typename Pred >
    bool wait_for( std::unique_lock< mutex > & lt,
                   std::chrono::duration< Rep, Period > const& timeout_duration, Pred pred) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        bool result = cnd_.wait_for( lt, timeout_duration, pred);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        return result;
    }
};

}}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_CONDITION_VARIABLE_H
