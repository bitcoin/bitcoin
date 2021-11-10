#ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_GCC_SPARC_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_GCC_SPARC_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  detail/sp_counted_base_gcc_sparc.hpp - g++ on Sparc V8+
//
//  Copyright (c) 2006 Piotr Wyderski
//  Copyright (c) 2006 Tomas Puverle
//  Copyright (c) 2006 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  Thanks to Michael van der Westhuizen

#include <boost/smart_ptr/detail/sp_typeinfo_.hpp>
#include <boost/smart_ptr/detail/sp_obsolete.hpp>
#include <boost/config.hpp>
#include <inttypes.h> // int32_t

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using g++/Sparc sp_counted_base")

#endif

BOOST_SP_OBSOLETE()

namespace boost
{

namespace detail
{

inline int32_t compare_and_swap( int32_t * dest_, int32_t compare_, int32_t swap_ )
{
    __asm__ __volatile__( "cas [%1], %2, %0"
                        : "+r" (swap_)
                        : "r" (dest_), "r" (compare_)
                        : "memory" );

    return swap_;
}

inline int32_t atomic_fetch_and_add( int32_t * pw, int32_t dv )
{
    // long r = *pw;
    // *pw += dv;
    // return r;

    for( ;; )
    {
        int32_t r = *pw;

        if( __builtin_expect((compare_and_swap(pw, r, r + dv) == r), 1) )
        {
            return r;
        }
    }
}

inline void atomic_increment( int32_t * pw )
{
    atomic_fetch_and_add( pw, 1 );
}

inline int32_t atomic_decrement( int32_t * pw )
{
    return atomic_fetch_and_add( pw, -1 );
}

inline int32_t atomic_conditional_increment( int32_t * pw )
{
    // long r = *pw;
    // if( r != 0 ) ++*pw;
    // return r;

    for( ;; )
    {
        int32_t r = *pw;

        if( r == 0 )
        {
            return r;
        }

        if( __builtin_expect( ( compare_and_swap( pw, r, r + 1 ) == r ), 1 ) )
        {
            return r;
        }
    }    
}

class BOOST_SYMBOL_VISIBLE sp_counted_base
{
private:

    sp_counted_base( sp_counted_base const & );
    sp_counted_base & operator= ( sp_counted_base const & );

    int32_t use_count_;        // #shared
    int32_t weak_count_;       // #weak + (#shared != 0)

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
        return const_cast< int32_t const volatile & >( use_count_ );
    }
};

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_GCC_SPARC_HPP_INCLUDED
