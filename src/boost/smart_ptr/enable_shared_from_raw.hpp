#ifndef BOOST_ENABLE_SHARED_FROM_RAW_HPP_INCLUDED
#define BOOST_ENABLE_SHARED_FROM_RAW_HPP_INCLUDED

//
//  enable_shared_from_raw.hpp
//
//  Copyright 2002, 2009, 2014 Peter Dimov
//  Copyright 2008-2009 Frank Mori Hess
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/assert.hpp>
#include <boost/config/workaround.hpp>

namespace boost
{
template<typename T> boost::shared_ptr<T> shared_from_raw(T *);
template<typename T> boost::weak_ptr<T> weak_from_raw(T *);

namespace detail
{
template< class X, class Y > inline void sp_enable_shared_from_this( boost::shared_ptr<X> * ppx, Y const * py, boost::enable_shared_from_raw const * pe );

} // namespace detail

class enable_shared_from_raw
{
protected:

    enable_shared_from_raw()
    {
    }

    enable_shared_from_raw( enable_shared_from_raw const & )
    {
    }

    enable_shared_from_raw & operator=( enable_shared_from_raw const & )
    {
        return *this;
    }

    ~enable_shared_from_raw()
    {
        BOOST_ASSERT( shared_this_.use_count() <= 1 ); // make sure no dangling shared_ptr objects exist
    }

private:

    void init_if_expired() const
    {
        if( weak_this_.expired() )
        {
            shared_this_.reset( static_cast<void*>(0), detail::esft2_deleter_wrapper() );
            weak_this_ = shared_this_;
        }
    }

    void init_if_empty() const
    {
        if( weak_this_._empty() )
        {
            shared_this_.reset( static_cast<void*>(0), detail::esft2_deleter_wrapper() );
            weak_this_ = shared_this_;
        }
    }

#ifdef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
public:
#else
private:
    template<class Y> friend class shared_ptr;
    template<typename T> friend boost::shared_ptr<T> shared_from_raw(T *);
    template<typename T> friend boost::weak_ptr<T> weak_from_raw(T *);
    template< class X, class Y > friend inline void detail::sp_enable_shared_from_this( boost::shared_ptr<X> * ppx, Y const * py, boost::enable_shared_from_raw const * pe );
#endif

    shared_ptr<void const volatile> shared_from_this() const
    {
        init_if_expired();
        return shared_ptr<void const volatile>( weak_this_ );
    }

    shared_ptr<void const volatile> shared_from_this() const volatile
    {
        return const_cast< enable_shared_from_raw const * >( this )->shared_from_this();
    }

    weak_ptr<void const volatile> weak_from_this() const
    {
        init_if_empty();
        return weak_this_;
    }

    weak_ptr<void const volatile> weak_from_this() const volatile
    {
        return const_cast< enable_shared_from_raw const * >( this )->weak_from_this();
    }

    // Note: invoked automatically by shared_ptr; do not call
    template<class X, class Y> void _internal_accept_owner( shared_ptr<X> * ppx, Y * ) const
    {
        BOOST_ASSERT( ppx != 0 );

        if( weak_this_.expired() )
        {
            weak_this_ = *ppx;
        }
        else if( shared_this_.use_count() != 0 )
        {
            BOOST_ASSERT( ppx->unique() ); // no weak_ptrs should exist either, but there's no way to check that

            detail::esft2_deleter_wrapper * pd = boost::get_deleter<detail::esft2_deleter_wrapper>( shared_this_ );
            BOOST_ASSERT( pd != 0 );

            pd->set_deleter( *ppx );

            ppx->reset( shared_this_, ppx->get() );
            shared_this_.reset();
        }
    }

    mutable weak_ptr<void const volatile> weak_this_;

private:

    mutable shared_ptr<void const volatile> shared_this_;
};

template<typename T>
boost::shared_ptr<T> shared_from_raw(T *p)
{
    BOOST_ASSERT(p != 0);
    return boost::shared_ptr<T>(p->enable_shared_from_raw::shared_from_this(), p);
}

template<typename T>
boost::weak_ptr<T> weak_from_raw(T *p)
{
    BOOST_ASSERT(p != 0);
    boost::weak_ptr<T> result(p->enable_shared_from_raw::weak_from_this(), p);
    return result;
}

namespace detail
{
    template< class X, class Y > inline void sp_enable_shared_from_this( boost::shared_ptr<X> * ppx, Y const * py, boost::enable_shared_from_raw const * pe )
    {
        if( pe != 0 )
        {
            pe->_internal_accept_owner( ppx, const_cast< Y* >( py ) );
        }
    }
} // namepsace detail

} // namespace boost

#endif  // #ifndef BOOST_ENABLE_SHARED_FROM_RAW_HPP_INCLUDED
