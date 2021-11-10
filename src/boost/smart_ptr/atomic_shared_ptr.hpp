#ifndef BOOST_SMART_PTR_ATOMIC_SHARED_PTR_HPP_INCLUDED
#define BOOST_SMART_PTR_ATOMIC_SHARED_PTR_HPP_INCLUDED

//
//  atomic_shared_ptr.hpp
//
//  Copyright 2017 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.
//

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <cstring>

namespace boost
{

template<class T> class atomic_shared_ptr
{
private:

    boost::shared_ptr<T> p_;

    mutable boost::detail::spinlock l_;

    atomic_shared_ptr(const atomic_shared_ptr&);
    atomic_shared_ptr& operator=(const atomic_shared_ptr&);

private:

    bool compare_exchange( shared_ptr<T>& v, shared_ptr<T> w ) BOOST_SP_NOEXCEPT
    {
        l_.lock();

        if( p_._internal_equiv( v ) )
        {
            p_.swap( w );

            l_.unlock();
            return true;
        }
        else
        {
            shared_ptr<T> tmp( p_ );

            l_.unlock();

            tmp.swap( v );
            return false;
        }
    }

public:

#if !defined( BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX ) && !defined( BOOST_NO_CXX11_CONSTEXPR )

    constexpr atomic_shared_ptr() BOOST_SP_NOEXCEPT: l_ BOOST_DETAIL_SPINLOCK_INIT
    {
    }

    atomic_shared_ptr( shared_ptr<T> p ) BOOST_SP_NOEXCEPT
        : p_( std::move( p ) ), l_ BOOST_DETAIL_SPINLOCK_INIT
    {
    }

#else

    atomic_shared_ptr() BOOST_SP_NOEXCEPT
    {
        boost::detail::spinlock init = BOOST_DETAIL_SPINLOCK_INIT;
        std::memcpy( &l_, &init, sizeof( init ) );
    }

    atomic_shared_ptr( shared_ptr<T> p ) BOOST_SP_NOEXCEPT
#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )
        : p_( std::move( p ) )
#else
        : p_( p )
#endif
    {
        boost::detail::spinlock init = BOOST_DETAIL_SPINLOCK_INIT;
        std::memcpy( &l_, &init, sizeof( init ) );
    }

#endif

    atomic_shared_ptr& operator=( shared_ptr<T> r ) BOOST_SP_NOEXCEPT
    {
        boost::detail::spinlock::scoped_lock lock( l_ );
        p_.swap( r );

        return *this;
    }

    BOOST_CONSTEXPR bool is_lock_free() const BOOST_SP_NOEXCEPT
    {
        return false;
    }

    shared_ptr<T> load() const BOOST_SP_NOEXCEPT
    {
        boost::detail::spinlock::scoped_lock lock( l_ );
        return p_;
    }

    template<class M> shared_ptr<T> load( M ) const BOOST_SP_NOEXCEPT
    {
        boost::detail::spinlock::scoped_lock lock( l_ );
        return p_;
    }

    operator shared_ptr<T>() const BOOST_SP_NOEXCEPT
    {
        boost::detail::spinlock::scoped_lock lock( l_ );
        return p_;
    }

    void store( shared_ptr<T> r ) BOOST_SP_NOEXCEPT
    {
        boost::detail::spinlock::scoped_lock lock( l_ );
        p_.swap( r );
    }

    template<class M> void store( shared_ptr<T> r, M ) BOOST_SP_NOEXCEPT
    {
        boost::detail::spinlock::scoped_lock lock( l_ );
        p_.swap( r );
    }

    shared_ptr<T> exchange( shared_ptr<T> r ) BOOST_SP_NOEXCEPT
    {
        {
            boost::detail::spinlock::scoped_lock lock( l_ );
            p_.swap( r );
        }

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

        return std::move( r );

#else

        return r;

#endif
    }

    template<class M> shared_ptr<T> exchange( shared_ptr<T> r, M ) BOOST_SP_NOEXCEPT
    {
        {
            boost::detail::spinlock::scoped_lock lock( l_ );
            p_.swap( r );
        }

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

        return std::move( r );

#else

        return r;

#endif
    }

    template<class M> bool compare_exchange_weak( shared_ptr<T>& v, const shared_ptr<T>& w, M, M ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, w );
    }

    template<class M> bool compare_exchange_weak( shared_ptr<T>& v, const shared_ptr<T>& w, M ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, w );
    }

    bool compare_exchange_weak( shared_ptr<T>& v, const shared_ptr<T>& w ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, w );
    }

    template<class M> bool compare_exchange_strong( shared_ptr<T>& v, const shared_ptr<T>& w, M, M ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, w );
    }

    template<class M> bool compare_exchange_strong( shared_ptr<T>& v, const shared_ptr<T>& w, M ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, w );
    }

    bool compare_exchange_strong( shared_ptr<T>& v, const shared_ptr<T>& w ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, w );
    }

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    template<class M> bool compare_exchange_weak( shared_ptr<T>& v, shared_ptr<T>&& w, M, M ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, std::move( w ) );
    }

    template<class M> bool compare_exchange_weak( shared_ptr<T>& v, shared_ptr<T>&& w, M ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, std::move( w ) );
    }

    bool compare_exchange_weak( shared_ptr<T>& v, shared_ptr<T>&& w ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, std::move( w ) );
    }

    template<class M> bool compare_exchange_strong( shared_ptr<T>& v, shared_ptr<T>&& w, M, M ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, std::move( w ) );
    }

    template<class M> bool compare_exchange_strong( shared_ptr<T>& v, shared_ptr<T>&& w, M ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, std::move( w ) );
    }

    bool compare_exchange_strong( shared_ptr<T>& v, shared_ptr<T>&& w ) BOOST_SP_NOEXCEPT
    {
        return compare_exchange( v, std::move( w ) );
    }

#endif
};

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_ATOMIC_SHARED_PTR_HPP_INCLUDED
