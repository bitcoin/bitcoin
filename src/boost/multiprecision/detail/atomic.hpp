///////////////////////////////////////////////////////////////////////////////
//  Copyright 2017 John Maddock
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MT_ATOMIC_DETAIL_HPP
#define BOOST_MT_ATOMIC_DETAIL_HPP

#include <boost/config.hpp>

#ifdef BOOST_HAS_THREADS

#  include <atomic>
#  define BOOST_MATH_ATOMIC_NS std
namespace boost {
   namespace multiprecision {
      namespace detail {
#if ATOMIC_INT_LOCK_FREE == 2
         using atomic_counter_type = std::atomic<int>;
         using atomic_unsigned_type = std::atomic<unsigned>;
         using atomic_integer_type = int;
         using atomic_unsigned_integer_type = unsigned;
#elif ATOMIC_SHORT_LOCK_FREE == 2
         using atomic_counter_type = std::atomic<short>;
         using atomic_unsigned_type = std::atomic<unsigned short>;
         using atomic_integer_type = short;
         using atomic_unsigned_integer_type = unsigned short;
#elif ATOMIC_LONG_LOCK_FREE == 2
         using atomic_unsigned_integer_type = std::atomic<long>;
         using atomic_unsigned_type = std::atomic<unsigned long>;
         using atomic_unsigned_integer_type = unsigned long;
         using atomic_integer_type = long;
#elif ATOMIC_LLONG_LOCK_FREE == 2
         using atomic_unsigned_integer_type = std::atomic<long long>;
         using atomic_unsigned_type = std::atomic<unsigned long long>;
         using atomic_integer_type = long long;
         using atomic_unsigned_integer_type = unsigned long long;
#else

#define BOOST_MT_NO_ATOMIC_INT

#endif
      }
   }}
#else // BOOST_HAS_THREADS

#define BOOST_MT_NO_ATOMIC_INT

#endif // BOOST_HAS_THREADS

namespace boost { namespace multiprecision { namespace detail {

#ifdef BOOST_MT_NO_ATOMIC_INT
using precision_type = unsigned;
#else
using precision_type = atomic_unsigned_type;
#endif

} } }

#endif // BOOST_MATH_ATOMIC_DETAIL_HPP
