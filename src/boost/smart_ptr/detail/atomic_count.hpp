#ifndef BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  boost/detail/atomic_count.hpp - thread/SMP safe reference counter
//
//  Copyright (c) 2001, 2002 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2013 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  typedef <implementation-defined> boost::detail::atomic_count;
//
//  atomic_count a(n);
//
//    (n is convertible to long)
//
//    Effects: Constructs an atomic_count with an initial value of n
//
//  a;
//
//    Returns: (long) the current value of a
//    Memory Ordering: acquire
//
//  ++a;
//
//    Effects: Atomically increments the value of a
//    Returns: (long) the new value of a
//    Memory Ordering: acquire/release
//
//  --a;
//
//    Effects: Atomically decrements the value of a
//    Returns: (long) the new value of a
//    Memory Ordering: acquire/release
//

#include <boost/smart_ptr/detail/sp_has_gcc_intrinsics.hpp>
#include <boost/smart_ptr/detail/sp_has_sync_intrinsics.hpp>
#include <boost/config.hpp>

#if defined( BOOST_AC_DISABLE_THREADS )
# include <boost/smart_ptr/detail/atomic_count_nt.hpp>

#elif defined( BOOST_AC_USE_STD_ATOMIC )
# include <boost/smart_ptr/detail/atomic_count_std_atomic.hpp>

#elif defined( BOOST_AC_USE_SPINLOCK )
# include <boost/smart_ptr/detail/atomic_count_spin.hpp>

#elif defined( BOOST_AC_USE_PTHREADS )
# include <boost/smart_ptr/detail/atomic_count_pt.hpp>

#elif defined( BOOST_SP_DISABLE_THREADS )
# include <boost/smart_ptr/detail/atomic_count_nt.hpp>

#elif defined( BOOST_SP_USE_STD_ATOMIC )
# include <boost/smart_ptr/detail/atomic_count_std_atomic.hpp>

#elif defined( BOOST_SP_USE_SPINLOCK )
# include <boost/smart_ptr/detail/atomic_count_spin.hpp>

#elif defined( BOOST_SP_USE_PTHREADS )
# include <boost/smart_ptr/detail/atomic_count_pt.hpp>

#elif defined( BOOST_DISABLE_THREADS ) && !defined( BOOST_SP_ENABLE_THREADS ) && !defined( BOOST_DISABLE_WIN32 )
# include <boost/smart_ptr/detail/atomic_count_nt.hpp>

#elif defined( BOOST_SP_HAS_GCC_INTRINSICS )
# include <boost/smart_ptr/detail/atomic_count_gcc_atomic.hpp>

#elif !defined( BOOST_NO_CXX11_HDR_ATOMIC )
# include <boost/smart_ptr/detail/atomic_count_std_atomic.hpp>

#elif defined( BOOST_SP_HAS_SYNC_INTRINSICS )
# include <boost/smart_ptr/detail/atomic_count_sync.hpp>

#elif defined( __GNUC__ ) && ( defined( __i386__ ) || defined( __x86_64__ ) ) && !defined( __PATHSCALE__ )
# include <boost/smart_ptr/detail/atomic_count_gcc_x86.hpp>

#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
# include <boost/smart_ptr/detail/atomic_count_win32.hpp>

#elif defined(__GLIBCPP__) || defined(__GLIBCXX__)
# include <boost/smart_ptr/detail/atomic_count_gcc.hpp>

#elif !defined( BOOST_HAS_THREADS )
# include <boost/smart_ptr/detail/atomic_count_nt.hpp>

#else
# include <boost/smart_ptr/detail/atomic_count_spin.hpp>

#endif

#endif // #ifndef BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_HPP_INCLUDED
