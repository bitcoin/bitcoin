#ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_GCC_IA64_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_GCC_IA64_HPP_INCLUDED

//
//  detail/sp_counted_base_gcc_ia64.hpp - g++ on IA64
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2006 Peter Dimov
//  Copyright 2005 Ben Hutchings
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
//  Lock-free algorithm by Alexander Terekhov
//

#include <boost/smart_ptr/detail/sp_typeinfo_.hpp>
#include <boost/smart_ptr/detail/sp_obsolete.hpp>
#include <boost/config.hpp>

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using g++/IA64 sp_counted_base")

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

    // No barrier is required here but fetchadd always has an acquire or
    // release barrier associated with it.  We choose release as it should be
    // cheaper.
    __asm__ ("fetchadd4.rel %0=%1,1" :
         "=r"(tmp), "=m"(*pw) :
         "m"( *pw ));
}

inline int atomic_decrement( int * pw )
{
    // return --*pw;

    int rv;

    __asm__ ("     fetchadd4.rel %0=%1,-1 ;; \n"
             "     cmp.eq        p7,p0=1,%0 ;; \n"
             "(p7) ld4.acq       %0=%1    " :
             "=&r"(rv), "=m"(*pw) :
             "m"( *pw ) :
             "p7");

    return rv;
}

inline int atomic_conditional_increment( int * pw )
{
    // if( *pw != 0 ) ++*pw;
    // return *pw;

    int rv, tmp, tmp2;

    __asm__ ("0:   ld4          %0=%3           ;; \n"
         "     cmp.eq       p7,p0=0,%0        ;; \n"
         "(p7) br.cond.spnt 1f                \n"
         "     mov          ar.ccv=%0         \n"
         "     add          %1=1,%0           ;; \n"
         "     cmpxchg4.acq %2=%3,%1,ar.ccv ;; \n"
         "     cmp.ne       p7,p0=%0,%2       ;; \n"
         "(p7) br.cond.spnt 0b                \n"
         "     mov          %0=%1             ;; \n"
         "1:" : 
         "=&r"(rv), "=&r"(tmp), "=&r"(tmp2), "=m"(*pw) :
         "m"( *pw ) :
         "ar.ccv", "p7");

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
        return static_cast<int const volatile &>( use_count_ ); // TODO use ld.acq here
    }
};

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_GCC_IA64_HPP_INCLUDED
