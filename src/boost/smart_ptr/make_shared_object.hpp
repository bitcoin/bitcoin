#ifndef BOOST_SMART_PTR_MAKE_SHARED_OBJECT_HPP_INCLUDED
#define BOOST_SMART_PTR_MAKE_SHARED_OBJECT_HPP_INCLUDED

//  make_shared_object.hpp
//
//  Copyright (c) 2007, 2008, 2012 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.

#include <boost/config.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/detail/sp_forward.hpp>
#include <boost/smart_ptr/detail/sp_noexcept.hpp>
#include <boost/type_traits/type_with_alignment.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <cstddef>
#include <new>

namespace boost
{

namespace detail
{

template< std::size_t N, std::size_t A > struct sp_aligned_storage
{
    union type
    {
        char data_[ N ];
        typename boost::type_with_alignment< A >::type align_;
    };
};

template< class T > class sp_ms_deleter
{
private:

    typedef typename sp_aligned_storage< sizeof( T ), ::boost::alignment_of< T >::value >::type storage_type;

    bool initialized_;
    storage_type storage_;

private:

    void destroy() BOOST_SP_NOEXCEPT
    {
        if( initialized_ )
        {
#if defined( __GNUC__ )

            // fixes incorrect aliasing warning
            T * p = reinterpret_cast< T* >( storage_.data_ );
            p->~T();

#else

            reinterpret_cast< T* >( storage_.data_ )->~T();

#endif

            initialized_ = false;
        }
    }

public:

    sp_ms_deleter() BOOST_SP_NOEXCEPT : initialized_( false )
    {
    }

    template<class A> explicit sp_ms_deleter( A const & ) BOOST_SP_NOEXCEPT : initialized_( false )
    {
    }

    // optimization: do not copy storage_
    sp_ms_deleter( sp_ms_deleter const & ) BOOST_SP_NOEXCEPT : initialized_( false )
    {
    }

    ~sp_ms_deleter() BOOST_SP_NOEXCEPT
    {
        destroy();
    }

    void operator()( T * ) BOOST_SP_NOEXCEPT
    {
        destroy();
    }

    static void operator_fn( T* ) BOOST_SP_NOEXCEPT // operator() can't be static
    {
    }

    void * address() BOOST_SP_NOEXCEPT
    {
        return storage_.data_;
    }

    void set_initialized() BOOST_SP_NOEXCEPT
    {
        initialized_ = true;
    }
};

template< class T, class A > class sp_as_deleter
{
private:

    typedef typename sp_aligned_storage< sizeof( T ), ::boost::alignment_of< T >::value >::type storage_type;

    storage_type storage_;
    A a_;
    bool initialized_;

private:

    void destroy() BOOST_SP_NOEXCEPT
    {
        if( initialized_ )
        {
            T * p = reinterpret_cast< T* >( storage_.data_ );

#if !defined( BOOST_NO_CXX11_ALLOCATOR )

            std::allocator_traits<A>::destroy( a_, p );

#else

            p->~T();

#endif

            initialized_ = false;
        }
    }

public:

    sp_as_deleter( A const & a ) BOOST_SP_NOEXCEPT : a_( a ), initialized_( false )
    {
    }

    // optimization: do not copy storage_
    sp_as_deleter( sp_as_deleter const & r ) BOOST_SP_NOEXCEPT : a_( r.a_), initialized_( false )
    {
    }

    ~sp_as_deleter() BOOST_SP_NOEXCEPT
    {
        destroy();
    }

    void operator()( T * ) BOOST_SP_NOEXCEPT
    {
        destroy();
    }

    static void operator_fn( T* ) BOOST_SP_NOEXCEPT // operator() can't be static
    {
    }

    void * address() BOOST_SP_NOEXCEPT
    {
        return storage_.data_;
    }

    void set_initialized() BOOST_SP_NOEXCEPT
    {
        initialized_ = true;
    }
};

template< class T > struct sp_if_not_array
{
    typedef boost::shared_ptr< T > type;
};

#if !defined( BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION )

template< class T > struct sp_if_not_array< T[] >
{
};

#if !defined( BOOST_BORLANDC ) || !BOOST_WORKAROUND( BOOST_BORLANDC, < 0x600 )

template< class T, std::size_t N > struct sp_if_not_array< T[N] >
{
};

#endif

#endif

} // namespace detail

#if !defined( BOOST_NO_FUNCTION_TEMPLATE_ORDERING )
# define BOOST_SP_MSD( T ) boost::detail::sp_inplace_tag< boost::detail::sp_ms_deleter< T > >()
#else
# define BOOST_SP_MSD( T ) boost::detail::sp_ms_deleter< T >()
#endif

// _noinit versions

template< class T > typename boost::detail::sp_if_not_array< T >::type make_shared_noinit()
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T;
    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A > typename boost::detail::sp_if_not_array< T >::type allocate_shared_noinit( A const & a )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ), a );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T;
    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

#if !defined( BOOST_NO_CXX11_VARIADIC_TEMPLATES ) && !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

// Variadic templates, rvalue reference

template< class T, class... Args > typename boost::detail::sp_if_not_array< T >::type make_shared( Args && ... args )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T( boost::detail::sp_forward<Args>( args )... );
    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A, class... Args > typename boost::detail::sp_if_not_array< T >::type allocate_shared( A const & a, Args && ... args )
{
#if !defined( BOOST_NO_CXX11_ALLOCATOR )

    typedef typename std::allocator_traits<A>::template rebind_alloc<T> A2;
    A2 a2( a );

    typedef boost::detail::sp_as_deleter< T, A2 > D;

    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), boost::detail::sp_inplace_tag<D>(), a2 );

#else

    typedef boost::detail::sp_ms_deleter< T > D;

    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), boost::detail::sp_inplace_tag<D>(), a );

#endif

    D * pd = static_cast< D* >( pt._internal_get_untyped_deleter() );
    void * pv = pd->address();

#if !defined( BOOST_NO_CXX11_ALLOCATOR )

    std::allocator_traits<A2>::construct( a2, static_cast< T* >( pv ), boost::detail::sp_forward<Args>( args )... );

#else

    ::new( pv ) T( boost::detail::sp_forward<Args>( args )... );

#endif

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

#else // !defined( BOOST_NO_CXX11_VARIADIC_TEMPLATES ) && !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

// Common zero-argument versions

template< class T > typename boost::detail::sp_if_not_array< T >::type make_shared()
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T();
    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A > typename boost::detail::sp_if_not_array< T >::type allocate_shared( A const & a )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ), a );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T();
    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

// C++03 version

template< class T, class A1 >
typename boost::detail::sp_if_not_array< T >::type make_shared( BOOST_FWD_REF(A1) a1 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A, class A1 >
typename boost::detail::sp_if_not_array< T >::type allocate_shared( A const & a, BOOST_FWD_REF(A1) a1 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ), a );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A1, class A2 >
typename boost::detail::sp_if_not_array< T >::type make_shared( BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A, class A1, class A2 >
typename boost::detail::sp_if_not_array< T >::type allocate_shared( A const & a, BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ), a );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A1, class A2, class A3 >
typename boost::detail::sp_if_not_array< T >::type make_shared( BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A, class A1, class A2, class A3 >
typename boost::detail::sp_if_not_array< T >::type allocate_shared( A const & a, BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ), a );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A1, class A2, class A3, class A4 >
typename boost::detail::sp_if_not_array< T >::type make_shared( BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A, class A1, class A2, class A3, class A4 >
typename boost::detail::sp_if_not_array< T >::type allocate_shared( A const & a, BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ), a );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A1, class A2, class A3, class A4, class A5 >
typename boost::detail::sp_if_not_array< T >::type make_shared( BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4, BOOST_FWD_REF(A5) a5 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 ),
        boost::forward<A5>( a5 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A, class A1, class A2, class A3, class A4, class A5 >
typename boost::detail::sp_if_not_array< T >::type allocate_shared( A const & a, BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4, BOOST_FWD_REF(A5) a5 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ), a );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 ),
        boost::forward<A5>( a5 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A1, class A2, class A3, class A4, class A5, class A6 >
typename boost::detail::sp_if_not_array< T >::type make_shared( BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4, BOOST_FWD_REF(A5) a5, BOOST_FWD_REF(A6) a6 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 ),
        boost::forward<A5>( a5 ),
        boost::forward<A6>( a6 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A, class A1, class A2, class A3, class A4, class A5, class A6 >
typename boost::detail::sp_if_not_array< T >::type allocate_shared( A const & a, BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4, BOOST_FWD_REF(A5) a5, BOOST_FWD_REF(A6) a6 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ), a );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 ),
        boost::forward<A5>( a5 ),
        boost::forward<A6>( a6 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7 >
typename boost::detail::sp_if_not_array< T >::type make_shared( BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4, BOOST_FWD_REF(A5) a5, BOOST_FWD_REF(A6) a6, BOOST_FWD_REF(A7) a7 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 ),
        boost::forward<A5>( a5 ),
        boost::forward<A6>( a6 ),
        boost::forward<A7>( a7 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A, class A1, class A2, class A3, class A4, class A5, class A6, class A7 >
typename boost::detail::sp_if_not_array< T >::type allocate_shared( A const & a, BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4, BOOST_FWD_REF(A5) a5, BOOST_FWD_REF(A6) a6, BOOST_FWD_REF(A7) a7 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ), a );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 ),
        boost::forward<A5>( a5 ),
        boost::forward<A6>( a6 ),
        boost::forward<A7>( a7 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 >
typename boost::detail::sp_if_not_array< T >::type make_shared( BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4, BOOST_FWD_REF(A5) a5, BOOST_FWD_REF(A6) a6, BOOST_FWD_REF(A7) a7, BOOST_FWD_REF(A8) a8 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 ),
        boost::forward<A5>( a5 ),
        boost::forward<A6>( a6 ),
        boost::forward<A7>( a7 ),
        boost::forward<A8>( a8 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 >
typename boost::detail::sp_if_not_array< T >::type allocate_shared( A const & a, BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4, BOOST_FWD_REF(A5) a5, BOOST_FWD_REF(A6) a6, BOOST_FWD_REF(A7) a7, BOOST_FWD_REF(A8) a8 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ), a );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 ),
        boost::forward<A5>( a5 ),
        boost::forward<A6>( a6 ),
        boost::forward<A7>( a7 ),
        boost::forward<A8>( a8 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9 >
typename boost::detail::sp_if_not_array< T >::type make_shared( BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4, BOOST_FWD_REF(A5) a5, BOOST_FWD_REF(A6) a6, BOOST_FWD_REF(A7) a7, BOOST_FWD_REF(A8) a8, BOOST_FWD_REF(A9) a9 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ) );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 ),
        boost::forward<A5>( a5 ),
        boost::forward<A6>( a6 ),
        boost::forward<A7>( a7 ),
        boost::forward<A8>( a8 ),
        boost::forward<A9>( a9 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9 >
typename boost::detail::sp_if_not_array< T >::type allocate_shared( A const & a, BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2, BOOST_FWD_REF(A3) a3, BOOST_FWD_REF(A4) a4, BOOST_FWD_REF(A5) a5, BOOST_FWD_REF(A6) a6, BOOST_FWD_REF(A7) a7, BOOST_FWD_REF(A8) a8, BOOST_FWD_REF(A9) a9 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), BOOST_SP_MSD( T ), a );

    boost::detail::sp_ms_deleter< T > * pd = static_cast<boost::detail::sp_ms_deleter< T > *>( pt._internal_get_untyped_deleter() );

    void * pv = pd->address();

    ::new( pv ) T(
        boost::forward<A1>( a1 ),
        boost::forward<A2>( a2 ),
        boost::forward<A3>( a3 ),
        boost::forward<A4>( a4 ),
        boost::forward<A5>( a5 ),
        boost::forward<A6>( a6 ),
        boost::forward<A7>( a7 ),
        boost::forward<A8>( a8 ),
        boost::forward<A9>( a9 )
        );

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );
    return boost::shared_ptr< T >( pt, pt2 );
}

#endif // !defined( BOOST_NO_CXX11_VARIADIC_TEMPLATES ) && !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

#undef BOOST_SP_MSD

} // namespace boost

#endif // #ifndef BOOST_SMART_PTR_MAKE_SHARED_OBJECT_HPP_INCLUDED
