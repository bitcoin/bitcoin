#ifndef BOOST_SMART_PTR_DETAIL_SHARED_COUNT_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SHARED_COUNT_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  detail/shared_count.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(__BORLANDC__) && !defined(__clang__)
# pragma warn -8027     // Functions containing try are not expanded inline
#endif

#include <boost/smart_ptr/bad_weak_ptr.hpp>
#include <boost/smart_ptr/detail/sp_counted_base.hpp>
#include <boost/smart_ptr/detail/sp_counted_impl.hpp>
#include <boost/smart_ptr/detail/sp_disable_deprecated.hpp>
#include <boost/smart_ptr/detail/sp_noexcept.hpp>
#include <boost/checked_delete.hpp>
#include <boost/throw_exception.hpp>
#include <boost/core/addressof.hpp>
#include <boost/config.hpp>
#include <boost/config/workaround.hpp>
#include <boost/cstdint.hpp>
#include <memory>            // std::auto_ptr
#include <functional>        // std::less
#include <cstddef>           // std::size_t

#ifdef BOOST_NO_EXCEPTIONS
# include <new>              // std::bad_alloc
#endif

#if defined( BOOST_SP_DISABLE_DEPRECATED )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace boost
{

namespace movelib
{

template< class T, class D > class unique_ptr;

} // namespace movelib

namespace detail
{

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)

int const shared_count_id = 0x2C35F101;
int const   weak_count_id = 0x298C38A4;

#endif

struct sp_nothrow_tag {};

template< class D > struct sp_inplace_tag
{
};

template< class T > class sp_reference_wrapper
{ 
public:

    explicit sp_reference_wrapper( T & t): t_( boost::addressof( t ) )
    {
    }

    template< class Y > void operator()( Y * p ) const
    {
        (*t_)( p );
    }

private:

    T * t_;
};

template< class D > struct sp_convert_reference
{
    typedef D type;
};

template< class D > struct sp_convert_reference< D& >
{
    typedef sp_reference_wrapper< D > type;
};

template<class T> std::size_t sp_hash_pointer( T* p ) BOOST_NOEXCEPT
{
    boost::uintptr_t v = reinterpret_cast<boost::uintptr_t>( p );

    // match boost::hash<T*>
    return static_cast<std::size_t>( v + ( v >> 3 ) );
}

class weak_count;

class shared_count
{
private:

    sp_counted_base * pi_;

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
    int id_;
#endif

    friend class weak_count;

public:

    BOOST_CONSTEXPR shared_count() BOOST_SP_NOEXCEPT: pi_(0)
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
    }

    BOOST_CONSTEXPR explicit shared_count( sp_counted_base * pi ) BOOST_SP_NOEXCEPT: pi_( pi )
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
    }

    template<class Y> explicit shared_count( Y * p ): pi_( 0 )
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
#ifndef BOOST_NO_EXCEPTIONS

        try
        {
            pi_ = new sp_counted_impl_p<Y>( p );
        }
        catch(...)
        {
            boost::checked_delete( p );
            throw;
        }

#else

        pi_ = new sp_counted_impl_p<Y>( p );

        if( pi_ == 0 )
        {
            boost::checked_delete( p );
            boost::throw_exception( std::bad_alloc() );
        }

#endif
    }

#if defined( BOOST_MSVC ) && BOOST_WORKAROUND( BOOST_MSVC, <= 1200 )
    template<class Y, class D> shared_count( Y * p, D d ): pi_(0)
#else
    template<class P, class D> shared_count( P p, D d ): pi_(0)
#endif
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
#if defined( BOOST_MSVC ) && BOOST_WORKAROUND( BOOST_MSVC, <= 1200 )
        typedef Y* P;
#endif
#ifndef BOOST_NO_EXCEPTIONS

        try
        {
            pi_ = new sp_counted_impl_pd<P, D>(p, d);
        }
        catch(...)
        {
            d(p); // delete p
            throw;
        }

#else

        pi_ = new sp_counted_impl_pd<P, D>(p, d);

        if(pi_ == 0)
        {
            d(p); // delete p
            boost::throw_exception(std::bad_alloc());
        }

#endif
    }

#if !defined( BOOST_NO_FUNCTION_TEMPLATE_ORDERING )

    template< class P, class D > shared_count( P p, sp_inplace_tag<D> ): pi_( 0 )
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
#ifndef BOOST_NO_EXCEPTIONS

        try
        {
            pi_ = new sp_counted_impl_pd< P, D >( p );
        }
        catch( ... )
        {
            D::operator_fn( p ); // delete p
            throw;
        }

#else

        pi_ = new sp_counted_impl_pd< P, D >( p );

        if( pi_ == 0 )
        {
            D::operator_fn( p ); // delete p
            boost::throw_exception( std::bad_alloc() );
        }

#endif // #ifndef BOOST_NO_EXCEPTIONS
    }

#endif // !defined( BOOST_NO_FUNCTION_TEMPLATE_ORDERING )

    template<class P, class D, class A> shared_count( P p, D d, A a ): pi_( 0 )
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        typedef sp_counted_impl_pda<P, D, A> impl_type;

#if !defined( BOOST_NO_CXX11_ALLOCATOR )

        typedef typename std::allocator_traits<A>::template rebind_alloc< impl_type > A2;

#else

        typedef typename A::template rebind< impl_type >::other A2;

#endif

        A2 a2( a );

#ifndef BOOST_NO_EXCEPTIONS

        try
        {
            pi_ = a2.allocate( 1 );
            ::new( static_cast< void* >( pi_ ) ) impl_type( p, d, a );
        }
        catch(...)
        {
            d( p );

            if( pi_ != 0 )
            {
                a2.deallocate( static_cast< impl_type* >( pi_ ), 1 );
            }

            throw;
        }

#else

        pi_ = a2.allocate( 1 );

        if( pi_ != 0 )
        {
            ::new( static_cast< void* >( pi_ ) ) impl_type( p, d, a );
        }
        else
        {
            d( p );
            boost::throw_exception( std::bad_alloc() );
        }

#endif
    }

#if !defined( BOOST_NO_FUNCTION_TEMPLATE_ORDERING )

    template< class P, class D, class A > shared_count( P p, sp_inplace_tag< D >, A a ): pi_( 0 )
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        typedef sp_counted_impl_pda< P, D, A > impl_type;

#if !defined( BOOST_NO_CXX11_ALLOCATOR )

        typedef typename std::allocator_traits<A>::template rebind_alloc< impl_type > A2;

#else

        typedef typename A::template rebind< impl_type >::other A2;

#endif

        A2 a2( a );

#ifndef BOOST_NO_EXCEPTIONS

        try
        {
            pi_ = a2.allocate( 1 );
            ::new( static_cast< void* >( pi_ ) ) impl_type( p, a );
        }
        catch(...)
        {
            D::operator_fn( p );

            if( pi_ != 0 )
            {
                a2.deallocate( static_cast< impl_type* >( pi_ ), 1 );
            }

            throw;
        }

#else

        pi_ = a2.allocate( 1 );

        if( pi_ != 0 )
        {
            ::new( static_cast< void* >( pi_ ) ) impl_type( p, a );
        }
        else
        {
            D::operator_fn( p );
            boost::throw_exception( std::bad_alloc() );
        }

#endif // #ifndef BOOST_NO_EXCEPTIONS
    }

#endif // !defined( BOOST_NO_FUNCTION_TEMPLATE_ORDERING )

#ifndef BOOST_NO_AUTO_PTR

    // auto_ptr<Y> is special cased to provide the strong guarantee

    template<class Y>
    explicit shared_count( std::auto_ptr<Y> & r ): pi_( new sp_counted_impl_p<Y>( r.get() ) )
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
#ifdef BOOST_NO_EXCEPTIONS

        if( pi_ == 0 )
        {
            boost::throw_exception(std::bad_alloc());
        }

#endif

        r.release();
    }

#endif 

#if !defined( BOOST_NO_CXX11_SMART_PTR )

    template<class Y, class D>
    explicit shared_count( std::unique_ptr<Y, D> & r ): pi_( 0 )
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        typedef typename sp_convert_reference<D>::type D2;

        D2 d2( static_cast<D&&>( r.get_deleter() ) );
        pi_ = new sp_counted_impl_pd< typename std::unique_ptr<Y, D>::pointer, D2 >( r.get(), d2 );

#ifdef BOOST_NO_EXCEPTIONS

        if( pi_ == 0 )
        {
            boost::throw_exception( std::bad_alloc() );
        }

#endif

        r.release();
    }

#endif

    template<class Y, class D>
    explicit shared_count( boost::movelib::unique_ptr<Y, D> & r ): pi_( 0 )
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        typedef typename sp_convert_reference<D>::type D2;

        D2 d2( r.get_deleter() );
        pi_ = new sp_counted_impl_pd< typename boost::movelib::unique_ptr<Y, D>::pointer, D2 >( r.get(), d2 );

#ifdef BOOST_NO_EXCEPTIONS

        if( pi_ == 0 )
        {
            boost::throw_exception( std::bad_alloc() );
        }

#endif

        r.release();
    }

    ~shared_count() /*BOOST_SP_NOEXCEPT*/
    {
        if( pi_ != 0 ) pi_->release();
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        id_ = 0;
#endif
    }

    shared_count(shared_count const & r) BOOST_SP_NOEXCEPT: pi_(r.pi_)
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        if( pi_ != 0 ) pi_->add_ref_copy();
    }

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    shared_count(shared_count && r) BOOST_SP_NOEXCEPT: pi_(r.pi_)
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        r.pi_ = 0;
    }

#endif

    explicit shared_count(weak_count const & r); // throws bad_weak_ptr when r.use_count() == 0
    shared_count( weak_count const & r, sp_nothrow_tag ) BOOST_SP_NOEXCEPT; // constructs an empty *this when r.use_count() == 0

    shared_count & operator= (shared_count const & r) BOOST_SP_NOEXCEPT
    {
        sp_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if( tmp != 0 ) tmp->add_ref_copy();
            if( pi_ != 0 ) pi_->release();
            pi_ = tmp;
        }

        return *this;
    }

    void swap(shared_count & r) BOOST_SP_NOEXCEPT
    {
        sp_counted_base * tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    long use_count() const BOOST_SP_NOEXCEPT
    {
        return pi_ != 0? pi_->use_count(): 0;
    }

    bool unique() const BOOST_SP_NOEXCEPT
    {
        return use_count() == 1;
    }

    bool empty() const BOOST_SP_NOEXCEPT
    {
        return pi_ == 0;
    }

    bool operator==( shared_count const & r ) const BOOST_SP_NOEXCEPT
    {
        return pi_ == r.pi_;
    }

    bool operator==( weak_count const & r ) const BOOST_SP_NOEXCEPT;

    bool operator<( shared_count const & r ) const BOOST_SP_NOEXCEPT
    {
        return std::less<sp_counted_base *>()( pi_, r.pi_ );
    }

    bool operator<( weak_count const & r ) const BOOST_SP_NOEXCEPT;

    void * get_deleter( sp_typeinfo_ const & ti ) const BOOST_SP_NOEXCEPT
    {
        return pi_? pi_->get_deleter( ti ): 0;
    }

    void * get_local_deleter( sp_typeinfo_ const & ti ) const BOOST_SP_NOEXCEPT
    {
        return pi_? pi_->get_local_deleter( ti ): 0;
    }

    void * get_untyped_deleter() const BOOST_SP_NOEXCEPT
    {
        return pi_? pi_->get_untyped_deleter(): 0;
    }

    std::size_t hash_value() const BOOST_SP_NOEXCEPT
    {
        return sp_hash_pointer( pi_ );
    }
};


class weak_count
{
private:

    sp_counted_base * pi_;

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
    int id_;
#endif

    friend class shared_count;

public:

    BOOST_CONSTEXPR weak_count() BOOST_SP_NOEXCEPT: pi_(0)
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(weak_count_id)
#endif
    {
    }

    weak_count(shared_count const & r) BOOST_SP_NOEXCEPT: pi_(r.pi_)
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(weak_count_id)
#endif
    {
        if(pi_ != 0) pi_->weak_add_ref();
    }

    weak_count(weak_count const & r) BOOST_SP_NOEXCEPT: pi_(r.pi_)
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(weak_count_id)
#endif
    {
        if(pi_ != 0) pi_->weak_add_ref();
    }

// Move support

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    weak_count(weak_count && r) BOOST_SP_NOEXCEPT: pi_(r.pi_)
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(weak_count_id)
#endif
    {
        r.pi_ = 0;
    }

#endif

    ~weak_count() /*BOOST_SP_NOEXCEPT*/
    {
        if(pi_ != 0) pi_->weak_release();
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        id_ = 0;
#endif
    }

    weak_count & operator= (shared_count const & r) BOOST_SP_NOEXCEPT
    {
        sp_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if(tmp != 0) tmp->weak_add_ref();
            if(pi_ != 0) pi_->weak_release();
            pi_ = tmp;
        }

        return *this;
    }

    weak_count & operator= (weak_count const & r) BOOST_SP_NOEXCEPT
    {
        sp_counted_base * tmp = r.pi_;

        if( tmp != pi_ )
        {
            if(tmp != 0) tmp->weak_add_ref();
            if(pi_ != 0) pi_->weak_release();
            pi_ = tmp;
        }

        return *this;
    }

    void swap(weak_count & r) BOOST_SP_NOEXCEPT
    {
        sp_counted_base * tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    long use_count() const BOOST_SP_NOEXCEPT
    {
        return pi_ != 0? pi_->use_count(): 0;
    }

    bool empty() const BOOST_SP_NOEXCEPT
    {
        return pi_ == 0;
    }

    bool operator==( weak_count const & r ) const BOOST_SP_NOEXCEPT
    {
        return pi_ == r.pi_;
    }

    bool operator==( shared_count const & r ) const BOOST_SP_NOEXCEPT
    {
        return pi_ == r.pi_;
    }

    bool operator<( weak_count const & r ) const BOOST_SP_NOEXCEPT
    {
        return std::less<sp_counted_base *>()( pi_, r.pi_ );
    }

    bool operator<( shared_count const & r ) const BOOST_SP_NOEXCEPT
    {
        return std::less<sp_counted_base *>()( pi_, r.pi_ );
    }

    std::size_t hash_value() const BOOST_SP_NOEXCEPT
    {
        return sp_hash_pointer( pi_ );
    }
};

inline shared_count::shared_count( weak_count const & r ): pi_( r.pi_ )
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
{
    if( pi_ == 0 || !pi_->add_ref_lock() )
    {
        boost::throw_exception( boost::bad_weak_ptr() );
    }
}

inline shared_count::shared_count( weak_count const & r, sp_nothrow_tag ) BOOST_SP_NOEXCEPT: pi_( r.pi_ )
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
{
    if( pi_ != 0 && !pi_->add_ref_lock() )
    {
        pi_ = 0;
    }
}

inline bool shared_count::operator==( weak_count const & r ) const BOOST_SP_NOEXCEPT
{
    return pi_ == r.pi_;
}

inline bool shared_count::operator<( weak_count const & r ) const BOOST_SP_NOEXCEPT
{
    return std::less<sp_counted_base *>()( pi_, r.pi_ );
}

} // namespace detail

} // namespace boost

#if defined( BOOST_SP_DISABLE_DEPRECATED )
#pragma GCC diagnostic pop
#endif

#if defined(__BORLANDC__) && !defined(__clang__)
# pragma warn .8027     // Functions containing try are not expanded inline
#endif

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SHARED_COUNT_HPP_INCLUDED
