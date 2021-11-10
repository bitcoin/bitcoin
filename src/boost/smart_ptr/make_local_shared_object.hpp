#ifndef BOOST_SMART_PTR_MAKE_LOCAL_SHARED_OBJECT_HPP_INCLUDED
#define BOOST_SMART_PTR_MAKE_LOCAL_SHARED_OBJECT_HPP_INCLUDED

//  make_local_shared_object.hpp
//
//  Copyright 2017 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.

#include <boost/smart_ptr/local_shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/config.hpp>
#include <utility>
#include <cstddef>

namespace boost
{

namespace detail
{

// lsp_if_not_array

template<class T> struct lsp_if_not_array
{
    typedef boost::local_shared_ptr<T> type;
};

template<class T> struct lsp_if_not_array<T[]>
{
};

template<class T, std::size_t N> struct lsp_if_not_array<T[N]>
{
};

// lsp_ms_deleter

template<class T, class A> class lsp_ms_deleter: public local_counted_impl_em
{
private:

    typedef typename sp_aligned_storage<sizeof(T), ::boost::alignment_of<T>::value>::type storage_type;

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

    explicit lsp_ms_deleter( A const & a ) BOOST_SP_NOEXCEPT : a_( a ), initialized_( false )
    {
    }

    // optimization: do not copy storage_
    lsp_ms_deleter( lsp_ms_deleter const & r ) BOOST_SP_NOEXCEPT : a_( r.a_), initialized_( false )
    {
    }

    ~lsp_ms_deleter() BOOST_SP_NOEXCEPT
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

} // namespace detail

template<class T, class A, class... Args> typename boost::detail::lsp_if_not_array<T>::type allocate_local_shared( A const & a, Args&&... args )
{
#if !defined( BOOST_NO_CXX11_ALLOCATOR )

    typedef typename std::allocator_traits<A>::template rebind_alloc<T> A2;

#else

    typedef typename A::template rebind<T>::other A2;

#endif

    A2 a2( a );

    typedef boost::detail::lsp_ms_deleter<T, A2> D;

    boost::shared_ptr<T> pt( static_cast< T* >( 0 ), boost::detail::sp_inplace_tag<D>(), a2 );

    D * pd = static_cast< D* >( pt._internal_get_untyped_deleter() );
    void * pv = pd->address();

#if !defined( BOOST_NO_CXX11_ALLOCATOR )

    std::allocator_traits<A2>::construct( a2, static_cast< T* >( pv ), std::forward<Args>( args )... );

#else

    ::new( pv ) T( std::forward<Args>( args )... );

#endif

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );
    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );

    pd->pn_ = pt._internal_count();

    return boost::local_shared_ptr<T>( boost::detail::lsp_internal_constructor_tag(), pt2, pd );
}

template<class T, class A> typename boost::detail::lsp_if_not_array<T>::type allocate_local_shared_noinit( A const & a )
{
#if !defined( BOOST_NO_CXX11_ALLOCATOR )

    typedef typename std::allocator_traits<A>::template rebind_alloc<T> A2;

#else

    typedef typename A::template rebind<T>::other A2;

#endif

    A2 a2( a );

    typedef boost::detail::lsp_ms_deleter< T, std::allocator<T> > D;

    boost::shared_ptr<T> pt( static_cast< T* >( 0 ), boost::detail::sp_inplace_tag<D>(), a2 );

    D * pd = static_cast< D* >( pt._internal_get_untyped_deleter() );
    void * pv = pd->address();

    ::new( pv ) T;

    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );
    boost::detail::sp_enable_shared_from_this( &pt, pt2, pt2 );

    pd->pn_ = pt._internal_count();

    return boost::local_shared_ptr<T>( boost::detail::lsp_internal_constructor_tag(), pt2, pd );
}

template<class T, class... Args> typename boost::detail::lsp_if_not_array<T>::type make_local_shared( Args&&... args )
{
    typedef typename boost::remove_const<T>::type T2;
    return boost::allocate_local_shared<T2>( std::allocator<T2>(), std::forward<Args>(args)... );
}

template<class T> typename boost::detail::lsp_if_not_array<T>::type make_local_shared_noinit()
{
    typedef typename boost::remove_const<T>::type T2;
    return boost::allocate_shared_noinit<T2>( std::allocator<T2>() );
}

} // namespace boost

#endif // #ifndef BOOST_SMART_PTR_MAKE_SHARED_OBJECT_HPP_INCLUDED
