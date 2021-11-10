#ifndef BOOST_THROW_EXCEPTION_HPP_INCLUDED
#define BOOST_THROW_EXCEPTION_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  boost/throw_exception.hpp
//
//  Copyright (c) 2002, 2018, 2019 Peter Dimov
//  Copyright (c) 2008-2009 Emil Dotchevski and Reverge Studios, Inc.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  http://www.boost.org/libs/throw_exception
//

#include <boost/exception/exception.hpp>
#include <boost/assert/source_location.hpp>
#include <boost/config.hpp>
#include <boost/config/workaround.hpp>
#include <exception>

#if !defined( BOOST_EXCEPTION_DISABLE ) && defined( BOOST_BORLANDC ) && BOOST_WORKAROUND( BOOST_BORLANDC, BOOST_TESTED_AT(0x593) )
# define BOOST_EXCEPTION_DISABLE
#endif

namespace boost
{

#if defined( BOOST_NO_EXCEPTIONS )

BOOST_NORETURN void throw_exception( std::exception const & e ); // user defined
BOOST_NORETURN void throw_exception( std::exception const & e, boost::source_location const & loc ); // user defined

#endif

// boost::wrapexcept<E>

namespace detail
{

typedef char (&wrapexcept_s1)[ 1 ];
typedef char (&wrapexcept_s2)[ 2 ];

template<class T> wrapexcept_s1 wrapexcept_is_convertible( T* );
template<class T> wrapexcept_s2 wrapexcept_is_convertible( void* );

template<class E, class B, int I = sizeof( wrapexcept_is_convertible<B>( static_cast< E* >( 0 ) ) ) > struct wrapexcept_add_base;

template<class E, class B> struct wrapexcept_add_base<E, B, 1>
{
    struct type {};
};

template<class E, class B> struct wrapexcept_add_base<E, B, 2>
{
    typedef B type;
};

} // namespace detail

template<class E> struct BOOST_SYMBOL_VISIBLE wrapexcept:
    public detail::wrapexcept_add_base<E, boost::exception_detail::clone_base>::type,
    public E,
    public detail::wrapexcept_add_base<E, boost::exception>::type
{
private:

    struct deleter
    {
        wrapexcept * p_;
        ~deleter() { delete p_; }
    };

private:

    void copy_from( void const* )
    {
    }

    void copy_from( boost::exception const* p )
    {
        static_cast<boost::exception&>( *this ) = *p;
    }

public:

    explicit wrapexcept( E const & e ): E( e )
    {
        copy_from( &e );
    }

    explicit wrapexcept( E const & e, boost::source_location const & loc ): E( e )
    {
        copy_from( &e );

        set_info( *this, throw_file( loc.file_name() ) );
        set_info( *this, throw_line( loc.line() ) );
        set_info( *this, throw_function( loc.function_name() ) );
    }

    virtual boost::exception_detail::clone_base const * clone() const BOOST_OVERRIDE
    {
        wrapexcept * p = new wrapexcept( *this );
        deleter del = { p };

        boost::exception_detail::copy_boost_exception( p, this );

        del.p_ = 0;
        return p;
    }

    virtual void rethrow() const BOOST_OVERRIDE
    {
#if defined( BOOST_NO_EXCEPTIONS )

        boost::throw_exception( *this );

#else

        throw *this;

#endif
    }
};

// All boost exceptions are required to derive from std::exception,
// to ensure compatibility with BOOST_NO_EXCEPTIONS.

inline void throw_exception_assert_compatibility( std::exception const & ) {}

// boost::throw_exception

#if !defined( BOOST_NO_EXCEPTIONS )

#if defined( BOOST_EXCEPTION_DISABLE )

template<class E> BOOST_NORETURN void throw_exception( E const & e )
{
    throw_exception_assert_compatibility( e );
    throw e;
}

template<class E> BOOST_NORETURN void throw_exception( E const & e, boost::source_location const & )
{
    throw_exception_assert_compatibility( e );
    throw e;
}

#else // defined( BOOST_EXCEPTION_DISABLE )

template<class E> BOOST_NORETURN void throw_exception( E const & e )
{
    throw_exception_assert_compatibility( e );
    throw wrapexcept<E>( e );
}

template<class E> BOOST_NORETURN void throw_exception( E const & e, boost::source_location const & loc )
{
    throw_exception_assert_compatibility( e );
    throw wrapexcept<E>( e, loc );
}

#endif // defined( BOOST_EXCEPTION_DISABLE )

#endif // !defined( BOOST_NO_EXCEPTIONS )

} // namespace boost

// BOOST_THROW_EXCEPTION

#define BOOST_THROW_EXCEPTION(x) ::boost::throw_exception(x, BOOST_CURRENT_LOCATION)

#endif // #ifndef BOOST_THROW_EXCEPTION_HPP_INCLUDED
