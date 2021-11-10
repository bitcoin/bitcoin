#ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_SNC_PS3_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_SNC_PS3_HPP_INCLUDED

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  detail/sp_counted_base_gcc_snc_ps3.hpp - PS3 Cell
//
//  Copyright (c) 2006 Piotr Wyderski
//  Copyright (c) 2006 Tomas Puverle
//  Copyright (c) 2006 Peter Dimov
//  Copyright (c) 2011 Emil Dotchevski
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  Thanks to Michael van der Westhuizen

#include <boost/smart_ptr/detail/sp_typeinfo_.hpp>
#include <boost/smart_ptr/detail/sp_obsolete.hpp>
#include <boost/config.hpp>
#include <inttypes.h> // uint32_t

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using PS3 sp_counted_base")

#endif

BOOST_SP_OBSOLETE()

namespace boost
{

namespace detail
{

inline uint32_t compare_and_swap( uint32_t * dest_, uint32_t compare_, uint32_t swap_ )
{
    return __builtin_cellAtomicCompareAndSwap32(dest_,compare_,swap_);
}

inline uint32_t atomic_fetch_and_add( uint32_t * pw, uint32_t dv )
{
    // long r = *pw;
    // *pw += dv;
    // return r;

    for( ;; )
    {
        uint32_t r = *pw;

        if( __builtin_expect((compare_and_swap(pw, r, r + dv) == r), 1) )
        {
            return r;
        }
    }
}

inline void atomic_increment( uint32_t * pw )
{
    (void) __builtin_cellAtomicIncr32( pw );
}

inline uint32_t atomic_decrement( uint32_t * pw )
{
    return __builtin_cellAtomicDecr32( pw );
}

inline uint32_t atomic_conditional_increment( uint32_t * pw )
{
    // long r = *pw;
    // if( r != 0 ) ++*pw;
    // return r;

    for( ;; )
    {
        uint32_t r = *pw;

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

    uint32_t use_count_;        // #shared
    uint32_t weak_count_;       // #weak + (#shared != 0)

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
        return const_cast< uint32_t const volatile & >( use_count_ );
    }
};

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_SNC_PS3_HPP_INCLUDED
