#ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_GCC_PPC_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_GCC_PPC_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  detail/sp_counted_base_gcc_ppc.hpp - g++ on PowerPC
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2005 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
//  Lock-free algorithm by Alexander Terekhov
//
//  Thanks to Ben Hitchings for the #weak + (#shared != 0)
//  formulation
//

#include <boost/smart_ptr/detail/sp_typeinfo_.hpp>
#include <boost/smart_ptr/detail/sp_obsolete.hpp>
#include <boost/config.hpp>

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using g++/PowerPC sp_counted_base")

#endif

BOOST_SP_OBSOLETE()

namespace boost
{

namespace detail
{

inline void atomic_increment( int * pw )
{
    // ++*pw;

    int tmp;

    __asm__
    (
        "0:\n\t"
        "lwarx %1, 0, %2\n\t"
        "addi %1, %1, 1\n\t"
        "stwcx. %1, 0, %2\n\t"
        "bne- 0b":

        "=m"( *pw ), "=&b"( tmp ):
        "r"( pw ), "m"( *pw ):
        "cc"
    );
}

inline int atomic_decrement( int * pw )
{
    // return --*pw;

    int rv;

    __asm__ __volatile__
    (
        "sync\n\t"
        "0:\n\t"
        "lwarx %1, 0, %2\n\t"
        "addi %1, %1, -1\n\t"
        "stwcx. %1, 0, %2\n\t"
        "bne- 0b\n\t"
        "isync":

        "=m"( *pw ), "=&b"( rv ):
        "r"( pw ), "m"( *pw ):
        "memory", "cc"
    );

    return rv;
}

inline int atomic_conditional_increment( int * pw )
{
    // if( *pw != 0 ) ++*pw;
    // return *pw;

    int rv;

    __asm__
    (
        "0:\n\t"
        "lwarx %1, 0, %2\n\t"
        "cmpwi %1, 0\n\t"
        "beq 1f\n\t"
        "addi %1, %1, 1\n\t"
        "1:\n\t"
        "stwcx. %1, 0, %2\n\t"
        "bne- 0b":

        "=m"( *pw ), "=&b"( rv ):
        "r"( pw ), "m"( *pw ):
        "cc"
    );

    return rv;
}

class BOOST_SYMBOL_VISIBLE sp_counted_base
{
private:

    sp_counted_base( sp_counted_base const & );
    sp_counted_base & operator= ( sp_counted_base const & );

    int use_count_;        // #shared
    int weak_count_;       // #weak + (#shared != 0)

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
        if( atomic_decrement( &use_count_ ) == 0 )
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
        if( atomic_decrement( &weak_count_ ) == 0 )
        {
            destroy();
        }
    }

    long use_count() const // nothrow
    {
        return static_cast<int const volatile &>( use_count_ );
    }
};

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_GCC_PPC_HPP_INCLUDED
