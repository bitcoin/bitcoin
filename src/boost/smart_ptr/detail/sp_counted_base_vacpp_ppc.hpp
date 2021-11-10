#ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_VACPP_PPC_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_VACPP_PPC_HPP_INCLUDED

//
//  detail/sp_counted_base_vacpp_ppc.hpp - xlC(vacpp) on POWER
//   based on: detail/sp_counted_base_w32.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2005 Peter Dimov
//  Copyright 2006 Michael van der Westhuizen
//  Copyright 2012 IBM Corp.
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
BOOST_PRAGMA_MESSAGE("Using xlC/PowerPC sp_counted_base")

#endif

BOOST_SP_OBSOLETE()

extern "builtin" void __lwsync(void);
extern "builtin" void __isync(void);
extern "builtin" int __fetch_and_add(volatile int* addr, int val);
extern "builtin" int __compare_and_swap(volatile int*, int*, int);

namespace boost
{

namespace detail
{

inline void atomic_increment( int *pw )
{
   // ++*pw;
   __lwsync();
   __fetch_and_add(pw, 1);
   __isync();
} 

inline int atomic_decrement( int *pw )
{
   // return --*pw;
   __lwsync();
   int originalValue = __fetch_and_add(pw, -1);
   __isync();

   return (originalValue - 1);
}

inline int atomic_conditional_increment( int *pw )
{
   // if( *pw != 0 ) ++*pw;
   // return *pw;

   __lwsync();
   int v = *const_cast<volatile int*>(pw);
   for (;;)
   // loop until state is known
   {
      if (v == 0) return 0;
      if (__compare_and_swap(pw, &v, v + 1))
      {
         __isync(); return (v + 1);
      }
   }
}

class BOOST_SYMBOL_VISIBLE sp_counted_base
{
private:

    sp_counted_base( sp_counted_base const & );
    sp_counted_base & operator= ( sp_counted_base const & );

    int use_count_;        // #shared
    int weak_count_;       // #weak + (#shared != 0)
    char pad[64] __attribute__((__aligned__(64)));
            // pad to prevent false sharing
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
        return *const_cast<volatile int*>(&use_count_); 
    }
};

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SP_COUNTED_BASE_VACPP_PPC_HPP_INCLUDED
