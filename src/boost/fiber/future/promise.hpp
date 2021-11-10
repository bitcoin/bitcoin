
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_PROMISE_HPP
#define BOOST_FIBERS_PROMISE_HPP

#include <algorithm>
#include <memory>
#include <utility>

#include <boost/config.hpp>
#include <boost/core/pointer_traits.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/future/detail/shared_state.hpp>
#include <boost/fiber/future/detail/shared_state_object.hpp>
#include <boost/fiber/future/future.hpp>

namespace boost {
namespace fibers {
namespace detail {

template< typename R >
struct promise_base {
    typedef typename shared_state< R >::ptr_type   ptr_type;

    bool            obtained_{ false };
    ptr_type        future_{};

    promise_base() :
        promise_base{ std::allocator_arg, std::allocator< promise_base >{} } {
    }

    template< typename Allocator >
    promise_base( std::allocator_arg_t, Allocator alloc) {
        typedef detail::shared_state_object< R, Allocator >  object_type;
        typedef std::allocator_traits< typename object_type::allocator_type > traits_type;
        typedef pointer_traits< typename traits_type::pointer > ptrait_type;
        typename object_type::allocator_type a{ alloc };
        typename traits_type::pointer ptr{ traits_type::allocate( a, 1) };
        typename ptrait_type::element_type* p = boost::to_address(ptr);

        try {
            traits_type::construct( a, p, a);
        } catch (...) {
            traits_type::deallocate( a, ptr, 1);
            throw;
        }
        future_.reset(p);
    }

    ~promise_base() {
        if ( future_ && obtained_) {
            future_->owner_destroyed();
        }
    }

    promise_base( promise_base const&) = delete;
    promise_base & operator=( promise_base const&) = delete;

    promise_base( promise_base && other) noexcept :
        obtained_{ other.obtained_ },
        future_{ std::move( other.future_) } {
        other.obtained_ = false;
    }

    promise_base & operator=( promise_base && other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            promise_base tmp{ std::move( other) };
            swap( tmp);
        }
        return * this;
    }

    future< R > get_future() {
        if ( BOOST_UNLIKELY( obtained_) ) {
            throw future_already_retrieved{};
        }
        if ( BOOST_UNLIKELY( ! future_) ) {
            throw promise_uninitialized{};
        }
        obtained_ = true;
        return future< R >{ future_ };
    }

    void swap( promise_base & other) noexcept {
        std::swap( obtained_, other.obtained_);
        future_.swap( other.future_);
    }

    void set_exception( std::exception_ptr p) {
        if ( BOOST_UNLIKELY( ! future_) ) {
            throw promise_uninitialized{};
        }
        future_->set_exception( p);
    }
};

}

template< typename R >
class promise : private detail::promise_base< R > {
private:
    typedef detail::promise_base< R >  base_type;

public:
    promise() = default;

    template< typename Allocator >
    promise( std::allocator_arg_t, Allocator alloc) :
        base_type{ std::allocator_arg, alloc } {
    }

    promise( promise const&) = delete;
    promise & operator=( promise const&) = delete;

    promise( promise && other) noexcept = default;
    promise & operator=( promise && other) = default;

    void set_value( R const& value) {
        if ( BOOST_UNLIKELY( ! base_type::future_) ) {
            throw promise_uninitialized{};
        }
        base_type::future_->set_value( value);
    }

    void set_value( R && value) {
        if ( BOOST_UNLIKELY( ! base_type::future_) ) {
            throw promise_uninitialized{};
        }
        base_type::future_->set_value( std::move( value) );
    }

    void swap( promise & other) noexcept {
        base_type::swap( other);
    }

    using base_type::get_future;
    using base_type::set_exception;
};

template< typename R >
class promise< R & > : private detail::promise_base< R & > {
private:
    typedef detail::promise_base< R & >  base_type;

public:
    promise() = default;

    template< typename Allocator >
    promise( std::allocator_arg_t, Allocator alloc) :
        base_type{ std::allocator_arg, alloc } {
    }

    promise( promise const&) = delete;
    promise & operator=( promise const&) = delete;

    promise( promise && other) noexcept = default;
    promise & operator=( promise && other) noexcept = default;

    void set_value( R & value) {
        if ( BOOST_UNLIKELY( ! base_type::future_) ) {
            throw promise_uninitialized{};
        }
        base_type::future_->set_value( value);
    }

    void swap( promise & other) noexcept {
        base_type::swap( other);
    }

    using base_type::get_future;
    using base_type::set_exception;
};

template<>
class promise< void > : private detail::promise_base< void > {
private:
    typedef detail::promise_base< void >  base_type;

public:
    promise() = default;

    template< typename Allocator >
    promise( std::allocator_arg_t, Allocator alloc) :
        base_type{ std::allocator_arg, alloc } {
    }

    promise( promise const&) = delete;
    promise & operator=( promise const&) = delete;

    promise( promise && other) noexcept = default;
    promise & operator=( promise && other) noexcept = default;

    inline
    void set_value() {
        if ( BOOST_UNLIKELY( ! base_type::future_) ) {
            throw promise_uninitialized{};
        }
        base_type::future_->set_value();
    }

    inline
    void swap( promise & other) noexcept {
        base_type::swap( other);
    }

    using base_type::get_future;
    using base_type::set_exception;
};

template< typename R >
void swap( promise< R > & l, promise< R > & r) noexcept {
    l.swap( r);
}

}}

#endif // BOOST_FIBERS_PROMISE_HPP
