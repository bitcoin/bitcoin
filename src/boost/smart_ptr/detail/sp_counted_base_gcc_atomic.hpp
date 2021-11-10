#ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_GCC_ATOMIC_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_GCC_ATOMIC_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

// detail/sp_counted_base_gcc_atomic.hpp - g++ 4.7+ __atomic intrinsics
//
// Copyright 2007, 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/smart_ptr/detail/sp_typeinfo_.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using __atomic sp_counted_base")

#endif

namespace boost
{

namespace detail
{

inline void atomic_increment( boost::uint_least32_t * pw )
{
    __atomic_fetch_add( pw, 1, __ATOMIC_RELAXED );
}

inline boost::uint_least32_t atomic_decrement( boost::uint_least32_t * pw )
{
    return __atomic_fetch_sub( pw, 1, __ATOMIC_ACQ_REL );
}

inline boost::uint_least32_t atomic_conditional_increment( boost::uint_least32_t * pw )
{
    // long r = *pw;
    // if( r != 0 ) ++*pw;
    // return r;

    boost::uint_least32_t r = __atomic_load_n( pw, __ATOMIC_RELAXED );

    for( ;; )
    {
        if( r == 0 )
        {
            return r;
        }

        if( __atomic_compare_exchange_n( pw, &r, r + 1, true, __ATOMIC_RELAXED, __ATOMIC_RELAXED ) )
        {
            return r;
        }
    }
}

inline boost::uint_least32_t atomic_load( boost::uint_least32_t const * pw )
{
    return __atomic_load_n( pw, __ATOMIC_ACQUIRE );
}

class BOOST_SYMBOL_VISIBLE sp_counted_base
{
private:

    sp_counted_base( sp_counted_base const & );
    sp_counted_base & operator= ( sp_counted_base const & );

    boost::uint_least32_t use_count_;        // #shared
    boost::uint_least32_t weak_count_;       // #weak + (#shared != 0)

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
        return atomic_load( &use_count_ );
    }
};

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_SYNC_HPP_INCLUDED
