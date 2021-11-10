
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FUTURE_HPP
#define BOOST_FIBERS_FUTURE_HPP

#include <algorithm>
#include <chrono>
#include <exception>
#include <utility>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/future/detail/shared_state.hpp>
#include <boost/fiber/future/future_status.hpp>

namespace boost {
namespace fibers {
namespace detail {

template< typename R >
struct future_base {
    typedef typename shared_state< R >::ptr_type   ptr_type;

    ptr_type           state_{};

    future_base() = default;

    explicit future_base( ptr_type  p) noexcept :
        state_{std::move( p )} {
    }

    ~future_base() = default;

    future_base( future_base const& other) :
        state_{ other.state_ } {
    }

    future_base( future_base && other) noexcept :
        state_{ other.state_ } {
        other.state_.reset();
    }

    future_base & operator=( future_base const& other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            state_ = other.state_;
        }
        return * this;
    }

    future_base & operator=( future_base && other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            state_ = other.state_;
            other.state_.reset();
        }
        return * this;
    }

    bool valid() const noexcept {
        return nullptr != state_.get();
    }

    std::exception_ptr get_exception_ptr() {
        if ( BOOST_UNLIKELY( ! valid() ) ) {
            throw future_uninitialized{};
        }
        return state_->get_exception_ptr();
    }

    void wait() const {
        if ( BOOST_UNLIKELY( ! valid() ) ) {
            throw future_uninitialized{};
        }
        state_->wait();
    }

    template< typename Rep, typename Period >
    future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const {
        if ( BOOST_UNLIKELY( ! valid() ) ) {
            throw future_uninitialized{};
        }
        return state_->wait_for( timeout_duration);
    }

    template< typename Clock, typename Duration >
    future_status wait_until( std::chrono::time_point< Clock, Duration > const& timeout_time) const {
        if ( BOOST_UNLIKELY( ! valid() ) ) {
            throw future_uninitialized{};
        }
        return state_->wait_until( timeout_time);
    }
};

template< typename R >
struct promise_base;

}

template< typename R >
class shared_future;

template< typename Signature >
class packaged_task;

template< typename R >
class future : private detail::future_base< R > {
private:
    typedef detail::future_base< R >  base_type;

    friend struct detail::promise_base< R >;
    friend class shared_future< R >;
    template< typename Signature >
    friend class packaged_task;

    explicit future( typename base_type::ptr_type const& p) noexcept :
        base_type{ p } {
    }

public:
    future() = default;

    future( future const&) = delete;
    future & operator=( future const&) = delete;

    future( future && other) noexcept :
        base_type{ std::move( other) } {
    }

    future & operator=( future && other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            base_type::operator=( std::move( other) );
        }
        return * this;
    }

    shared_future< R > share();

    R get() {
        if ( BOOST_UNLIKELY( ! base_type::valid() ) ) {
            throw future_uninitialized{};
        }
        typename base_type::ptr_type tmp{};
        tmp.swap( base_type::state_);
        return std::move( tmp->get() );
    }

    using base_type::valid;
    using base_type::get_exception_ptr;
    using base_type::wait;
    using base_type::wait_for;
    using base_type::wait_until;
};

template< typename R >
class future< R & > : private detail::future_base< R & > {
private:
    typedef detail::future_base< R & >  base_type;

    friend struct detail::promise_base< R & >;
    friend class shared_future< R & >;
    template< typename Signature >
    friend class packaged_task;

    explicit future( typename base_type::ptr_type const& p) noexcept :
        base_type{ p  } {
    }

public:
    future() = default;

    future( future const&) = delete;
    future & operator=( future const&) = delete;

    future( future && other) noexcept :
        base_type{ std::move( other) } {
    }

    future & operator=( future && other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            base_type::operator=( std::move( other) );
        }
        return * this;
    }

    shared_future< R & > share();

    R & get() {
        if ( BOOST_UNLIKELY( ! base_type::valid() ) ) {
            throw future_uninitialized{};
        }
        typename base_type::ptr_type tmp{};
        tmp.swap( base_type::state_);
        return tmp->get();
    }

    using base_type::valid;
    using base_type::get_exception_ptr;
    using base_type::wait;
    using base_type::wait_for;
    using base_type::wait_until;
};

template<>
class future< void > : private detail::future_base< void > {
private:
    typedef detail::future_base< void >  base_type;

    friend struct detail::promise_base< void >;
    friend class shared_future< void >;
    template< typename Signature >
    friend class packaged_task;

    explicit future( base_type::ptr_type const& p) noexcept :
        base_type{ p } {
    }

public:
    future() = default;

    future( future const&) = delete;
    future & operator=( future const&) = delete;

    inline
    future( future && other) noexcept :
        base_type{ std::move( other) } {
    }

    inline
    future & operator=( future && other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            base_type::operator=( std::move( other) );
        }
        return * this;
    }

    shared_future< void > share();

    inline
    void get() {
        if ( BOOST_UNLIKELY( ! base_type::valid() ) ) {
            throw future_uninitialized{};
        }
        base_type::ptr_type tmp{};
        tmp.swap( base_type::state_);
        tmp->get();
    }

    using base_type::valid;
    using base_type::get_exception_ptr;
    using base_type::wait;
    using base_type::wait_for;
    using base_type::wait_until;
};


template< typename R >
class shared_future : private detail::future_base< R > {
private:
    typedef detail::future_base< R >   base_type;

    explicit shared_future( typename base_type::ptr_type const& p) noexcept :
        base_type{ p } {
    }

public:
    shared_future() = default;

    ~shared_future() = default;

    shared_future( shared_future const& other) :
        base_type{ other } {
    }

    shared_future( shared_future && other) noexcept :
        base_type{ std::move( other) } {
    }

    shared_future( future< R > && other) noexcept :
        base_type{ std::move( other) } {
    }

    shared_future & operator=( shared_future const& other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            base_type::operator=( other); 
        }
        return * this;
    }

    shared_future & operator=( shared_future && other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            base_type::operator=( std::move( other) ); 
        }
        return * this;
    }

    shared_future & operator=( future< R > && other) noexcept {
        base_type::operator=( std::move( other) ); 
        return * this;
    }

    R const& get() const {
        if ( BOOST_UNLIKELY( ! valid() ) ) {
            throw future_uninitialized{};
        }
        return base_type::state_->get();
    }

    using base_type::valid;
    using base_type::get_exception_ptr;
    using base_type::wait;
    using base_type::wait_for;
    using base_type::wait_until;
};

template< typename R >
class shared_future< R & > : private detail::future_base< R & > {
private:
    typedef detail::future_base< R & >  base_type;

    explicit shared_future( typename base_type::ptr_type const& p) noexcept :
        base_type{ p } {
    }

public:
    shared_future() = default;

    ~shared_future() = default;

    shared_future( shared_future const& other) :
        base_type{ other } {
    }

    shared_future( shared_future && other) noexcept :
        base_type{ std::move( other) } {
    }

    shared_future( future< R & > && other) noexcept :
        base_type{ std::move( other) } {
    }

    shared_future & operator=( shared_future const& other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            base_type::operator=( other);
        }
        return * this;
    }

    shared_future & operator=( shared_future && other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            base_type::operator=( std::move( other) );
        }
        return * this;
    }

    shared_future & operator=( future< R & > && other) noexcept {
        base_type::operator=( std::move( other) );
        return * this;
    }

    R & get() const {
        if ( BOOST_UNLIKELY( ! valid() ) ) {
            throw future_uninitialized{};
        }
        return base_type::state_->get();
    }

    using base_type::valid;
    using base_type::get_exception_ptr;
    using base_type::wait;
    using base_type::wait_for;
    using base_type::wait_until;
};

template<>
class shared_future< void > : private detail::future_base< void > {
private:
    typedef detail::future_base< void > base_type;

    explicit shared_future( base_type::ptr_type const& p) noexcept :
        base_type{ p } {
    }

public:
    shared_future() = default;

    ~shared_future() = default;

    inline
    shared_future( shared_future const& other) :
        base_type{ other } {
    }

    inline
    shared_future( shared_future && other) noexcept :
        base_type{ std::move( other) } {
    }

    inline
    shared_future( future< void > && other) noexcept :
        base_type{ std::move( other) } {
    }

    inline
    shared_future & operator=( shared_future const& other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            base_type::operator=( other);
        }
        return * this;
    }

    inline
    shared_future & operator=( shared_future && other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            base_type::operator=( std::move( other) );
        }
        return * this;
    }

    inline
    shared_future & operator=( future< void > && other) noexcept {
        base_type::operator=( std::move( other) );
        return * this;
    }

    inline
    void get() const {
        if ( BOOST_UNLIKELY( ! valid() ) ) {
            throw future_uninitialized{};
        }
        base_type::state_->get();
    }

    using base_type::valid;
    using base_type::get_exception_ptr;
    using base_type::wait;
    using base_type::wait_for;
    using base_type::wait_until;
};


template< typename R >
shared_future< R >
future< R >::share() {
    if ( BOOST_UNLIKELY( ! base_type::valid() ) ) {
        throw future_uninitialized{};
    }
    return shared_future< R >{ std::move( * this) };
}

template< typename R >
shared_future< R & >
future< R & >::share() {
    if ( BOOST_UNLIKELY( ! base_type::valid() ) ) {
        throw future_uninitialized{};
    }
    return shared_future< R & >{ std::move( * this) };
}

inline
shared_future< void >
future< void >::share() {
    if ( BOOST_UNLIKELY( ! base_type::valid() ) ) {
        throw future_uninitialized{};
    }
    return shared_future< void >{ std::move( * this) };
}

}}

#endif
