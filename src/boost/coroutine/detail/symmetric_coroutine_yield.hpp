
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_YIELD_H
#define BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_YIELD_H

#include <algorithm>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility/explicit_operator_bool.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/exceptions.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template< typename R >
class symmetric_coroutine_yield
{
private:
    template< typename X, typename Y, typename Z >
    friend class symmetric_coroutine_object;

    typedef symmetric_coroutine_impl< R >       impl_type;

    struct dummy {};

    BOOST_MOVABLE_BUT_NOT_COPYABLE( symmetric_coroutine_yield)

    impl_type   *   impl_;
    R           *   result_;

    symmetric_coroutine_yield( impl_type * impl, R * result) BOOST_NOEXCEPT :
        impl_( impl),
        result_( result)
    {
        BOOST_ASSERT( 0 != impl_);
        BOOST_ASSERT( 0 != result_);
    }

public:
    symmetric_coroutine_yield() BOOST_NOEXCEPT :
        impl_( 0),
        result_( 0)
    {}

    symmetric_coroutine_yield( BOOST_RV_REF( symmetric_coroutine_yield) other) BOOST_NOEXCEPT :
        impl_( 0),
        result_( 0)
    { swap( other); }

    symmetric_coroutine_yield & operator=( BOOST_RV_REF( symmetric_coroutine_yield) other) BOOST_NOEXCEPT
    {
        symmetric_coroutine_yield tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_; }

    void swap( symmetric_coroutine_yield & other) BOOST_NOEXCEPT
    {
        std::swap( impl_, other.impl_);
        std::swap( result_, other.result_);
    }

    symmetric_coroutine_yield & operator()()
    {
        result_ = impl_->yield();
        return * this;
    }

    template< typename Coro >
    symmetric_coroutine_yield & operator()( Coro & other, typename Coro::value_type x,
                                            typename disable_if<
                                                is_same< typename Coro::value_type, void >,
                                                dummy*
                                            >::type = 0)
    {
        BOOST_ASSERT( other);

        result_ = impl_->yield_to( other.impl_, x);
        return * this;
    }

    template< typename Coro >
    symmetric_coroutine_yield & operator()( Coro & other,
                                            typename enable_if<
                                                is_same< typename Coro::value_type, void >,
                                                dummy*
                                            >::type = 0)
    {
        BOOST_ASSERT( other);

        result_ = impl_->yield_to( other.impl_);
        return * this;
    }

    R get() const
    {
        if ( 0 == result_)
            boost::throw_exception(
                invalid_result() );

        return * result_; 
    }
};

template< typename R >
class symmetric_coroutine_yield< R & >
{
private:
    template< typename X, typename Y, typename Z >
    friend class symmetric_coroutine_object;

    typedef symmetric_coroutine_impl< R & >     impl_type;

    struct dummy {};

    BOOST_MOVABLE_BUT_NOT_COPYABLE( symmetric_coroutine_yield)

    impl_type   *   impl_;
    R           *   result_;

    symmetric_coroutine_yield( impl_type * impl, R * result) BOOST_NOEXCEPT :
        impl_( impl),
        result_( result)
    {
        BOOST_ASSERT( 0 != impl_);
        BOOST_ASSERT( 0 != result_);
    }

public:
    symmetric_coroutine_yield() BOOST_NOEXCEPT :
        impl_( 0),
        result_( 0)
    {}

    symmetric_coroutine_yield( BOOST_RV_REF( symmetric_coroutine_yield) other) BOOST_NOEXCEPT :
        impl_( 0),
        result_( 0)
    { swap( other); }

    symmetric_coroutine_yield & operator=( BOOST_RV_REF( symmetric_coroutine_yield) other) BOOST_NOEXCEPT
    {
        symmetric_coroutine_yield tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_; }

    void swap( symmetric_coroutine_yield & other) BOOST_NOEXCEPT
    {
        std::swap( impl_, other.impl_);
        std::swap( result_, other.result_);
    }

    symmetric_coroutine_yield & operator()()
    {
        result_ = impl_->yield();
        return * this;
    }

    template< typename Coro >
    symmetric_coroutine_yield & operator()( Coro & other, typename Coro::value_type & x,
                                            typename disable_if<
                                                is_same< typename Coro::value_type, void >,
                                                dummy*
                                            >::type = 0)
    {
        BOOST_ASSERT( other);

        result_ = impl_->yield_to( other.impl_, x);
        return * this;
    }

    template< typename Coro >
    symmetric_coroutine_yield & operator()( Coro & other,
                                            typename enable_if<
                                                is_same< typename Coro::value_type, void >,
                                                dummy*
                                            >::type = 0)
    {
        BOOST_ASSERT( other);

        result_ = impl_->yield_to( other.impl_);
        return * this;
    }

    R & get() const
    {
        if ( 0 == result_)
            boost::throw_exception(
                invalid_result() );

        return * result_; 
    }
};

template<>
class symmetric_coroutine_yield< void >
{
private:
    template< typename X, typename Y, typename Z >
    friend class symmetric_coroutine_object;

    typedef symmetric_coroutine_impl< void >    impl_type;

    struct dummy {};

    BOOST_MOVABLE_BUT_NOT_COPYABLE( symmetric_coroutine_yield)

    impl_type   *   impl_;

    symmetric_coroutine_yield( impl_type * impl) BOOST_NOEXCEPT :
        impl_( impl)
    { BOOST_ASSERT( 0 != impl_); }

public:
    symmetric_coroutine_yield() BOOST_NOEXCEPT :
        impl_( 0)
    {}

    symmetric_coroutine_yield( BOOST_RV_REF( symmetric_coroutine_yield) other) BOOST_NOEXCEPT :
        impl_( 0)
    { swap( other); }

    symmetric_coroutine_yield & operator=( BOOST_RV_REF( symmetric_coroutine_yield) other) BOOST_NOEXCEPT
    {
        symmetric_coroutine_yield tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    inline bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_; }

    inline void swap( symmetric_coroutine_yield & other) BOOST_NOEXCEPT
    { std::swap( impl_, other.impl_); }

    inline symmetric_coroutine_yield & operator()()
    {
        impl_->yield();
        return * this;
    }

    template< typename Coro >
    symmetric_coroutine_yield & operator()( Coro & other, typename Coro::value_type & x,
                                            typename disable_if<
                                                is_same< typename Coro::value_type, void >,
                                                dummy*
                                            >::type = 0)
    {
        BOOST_ASSERT( other);

        impl_->yield_to( other.impl_, x);
        return * this;
    }

    template< typename Coro >
    symmetric_coroutine_yield & operator()( Coro & other,
                                            typename enable_if<
                                                is_same< typename Coro::value_type, void >,
                                                dummy*
                                            >::type = 0)
    {
        BOOST_ASSERT( other);

        impl_->yield_to( other.impl_);
        return * this;
    }
};

template< typename R >
void swap( symmetric_coroutine_yield< R > & l, symmetric_coroutine_yield< R > & r)
{ l.swap( r); }

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_YIELD_H
