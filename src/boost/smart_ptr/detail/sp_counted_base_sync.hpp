#ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_SYNC_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_SYNC_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  detail/sp_counted_base_sync.hpp - g++ 4.1+ __sync intrinsics
//
//  Copyright (c) 2007 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/smart_ptr/detail/sp_typeinfo_.hpp>
#include <boost/config.hpp>
#include <limits.h>

#if defined( __ia64__ ) && defined( __INTEL_COMPILER )
# include <ia64intrin.h>
#endif

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using __sync sp_counted_base")

#endif

namespace boost
{

namespace detail
{

#if INT_MAX >= 2147483647

typedef int sp_int32_t;

#else

typedef long sp_int32_t;

#endif

inline void atomic_increment( sp_int32_t * pw )
{
    __sync_fetch_and_add( pw, 1 );
}

inline sp_int32_t atomic_decrement( sp_int32_t * pw )
{
    return __sync_fetch_and_add( pw, -1 );
}

inline sp_int32_t atomic_conditional_increment( sp_int32_t * pw )
{
    // long r = *pw;
    // if( r != 0 ) ++*pw;
    // return r;

    sp_int32_t r = *pw;

    for( ;; )
    {
        if( r == 0 )
        {
            return r;
        }

        sp_int32_t r2 = __sync_val_compare_and_swap( pw, r, r + 1 );

        if( r2 == r )
        {
            return r;
        }
        else
        {
            r = r2;
        }
    }    
}

class BOOST_SYMBOL_VISIBLE sp_counted_base
{
private:

    sp_counted_base( sp_counted_base const & );
    sp_counted_base & operator= ( sp_counted_base const & );

    sp_int32_t use_count_;        // #shared
    sp_int32_t weak_count_;       // #weak + (#shared != 0)

public:

    sp_counted_base(): use_count_( 1 ), weak_count_( 1 )
    {
    }

    virtual ~sp_counted_base() // nothrow
    {
    }

    // dispose() is called when use_count_ drops to zero, to release
    // the resources managed by *this.

    virtual void dispose() = 0; // nothrow

    // destroy() is called when weak_count_ drops to zero.

    virtual void destroy() // nothrow
    {
        delete this;
    }

    virtual void * get_deleter( sp_typeinfo_ const & ti ) = 0;
    virtual void * get_local_deleter( sp_typeinfo_ const & ti ) = 0;
    virtual void * get_untyped_deleter() = 0;

    void add_ref_copy()
    {
        atomic_increment( &use_count_ );
    }

    bool add_ref_lock() // true on success
    {
        return atomic_conditional_increment( &use_count_ ) != 0;
    }

    void release() // nothrow
    {
        if( atomic_decrement( &use_count_ ) == 1 )
        {
            dispose();
            weak_release();
        }
    }

    void weak_add_ref() // nothrow
    {
        atomic_increment( &weak_count_ );
    }

    void weak_release() // nothrow
    {
        if( atomic_decrement( &weak_count_ ) == 1 )
        {
            destroy();
        }
    }

    long use_count() const // nothrow
    {
        return const_cast< sp_int32_t const volatile & >( use_count_ );
    }
};

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_SYNC_HPP_INCLUDED
