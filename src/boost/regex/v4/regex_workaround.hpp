/*
 *
 * Copyright (c) 1998-2005
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_workarounds.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares Misc workarounds.
  */

#ifndef BOOST_REGEX_WORKAROUND_HPP
#define BOOST_REGEX_WORKAROUND_HPP

#include <boost/config.hpp>
#include <new>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <cassert>
#include <cstdio>
#include <climits>
#include <string>
#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <iosfwd>
#include <vector>
#include <set>
#include <map>
#include <boost/limits.hpp>
#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/throw_exception.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/mpl/bool_fwd.hpp>
#include <boost/regex/config.hpp>
#ifndef BOOST_NO_STD_LOCALE
#   include <locale>
#endif

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
   using ::sprintf; using ::strcpy; using ::strcat; using ::strlen;
}
#endif

namespace boost{ namespace BOOST_REGEX_DETAIL_NS{
#ifdef BOOST_NO_STD_DISTANCE
template <class T>
std::ptrdiff_t distance(const T& x, const T& y)
{ return y - x; }
#else
using std::distance;
#endif
}}


#ifdef BOOST_REGEX_NO_BOOL
#  define BOOST_REGEX_MAKE_BOOL(x) static_cast<bool>((x) ? true : false)
#else
#  define BOOST_REGEX_MAKE_BOOL(x) static_cast<bool>(x)
#endif

/*****************************************************************************
 *
 *  Fix broken namespace support:
 *
 ****************************************************************************/

#if defined(BOOST_NO_STDC_NAMESPACE) && defined(__cplusplus)

namespace std{
   using ::ptrdiff_t;
   using ::size_t;
   using ::abs;
   using ::memset;
   using ::memcpy;
}

#endif

/*****************************************************************************
 *
 *  helper functions pointer_construct/pointer_destroy:
 *
 ****************************************************************************/

#ifdef __cplusplus
namespace boost{ namespace BOOST_REGEX_DETAIL_NS{

#ifdef BOOST_MSVC
#pragma warning (push)
#pragma warning (disable : 4100)
#endif

template <class T>
inline void pointer_destroy(T* p)
{ p->~T(); (void)p; }

#ifdef BOOST_MSVC
#pragma warning (pop)
#endif

template <class T>
inline void pointer_construct(T* p, const T& t)
{ new (p) T(t); }

}} // namespaces
#endif

/*****************************************************************************
 *
 *  helper function copy:
 *
 ****************************************************************************/

#ifdef __cplusplus
namespace boost{ namespace BOOST_REGEX_DETAIL_NS{
#if BOOST_WORKAROUND(BOOST_MSVC,>=1400) && BOOST_WORKAROUND(BOOST_MSVC, <1600) && defined(_CPPLIB_VER) && defined(BOOST_DINKUMWARE_STDLIB) && !(defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION))
   //
   // MSVC 8 will either emit warnings or else refuse to compile
   // code that makes perfectly legitimate use of std::copy, when
   // the OutputIterator type is a user-defined class (apparently all user 
   // defined iterators are "unsafe").  This code works around that:
   //
   template<class InputIterator, class OutputIterator>
   inline OutputIterator copy(
      InputIterator first, 
      InputIterator last, 
      OutputIterator dest
   )
   {
      return stdext::unchecked_copy(first, last, dest);
   }
   template<class InputIterator1, class InputIterator2>
   inline bool equal(
      InputIterator1 first, 
      InputIterator1 last, 
      InputIterator2 with
   )
   {
      return stdext::unchecked_equal(first, last, with);
   }
#elif BOOST_WORKAROUND(BOOST_MSVC, > 1500)
   //
   // MSVC 10 will either emit warnings or else refuse to compile
   // code that makes perfectly legitimate use of std::copy, when
   // the OutputIterator type is a user-defined class (apparently all user 
   // defined iterators are "unsafe").  What's more Microsoft have removed their
   // non-standard "unchecked" versions, even though their still in the MS
   // documentation!! Work around this as best we can: 
   //
   template<class InputIterator, class OutputIterator>
   inline OutputIterator copy(
      InputIterator first, 
      InputIterator last, 
      OutputIterator dest
   )
   {
      while(first != last)
         *dest++ = *first++;
      return dest;
   }
   template<class InputIterator1, class InputIterator2>
   inline bool equal(
      InputIterator1 first, 
      InputIterator1 last, 
      InputIterator2 with
   )
   {
      while(first != last)
         if(*first++ != *with++) return false;
      return true;
   }
#else 
   using std::copy; 
   using std::equal; 
#endif 
#if BOOST_WORKAROUND(BOOST_MSVC,>=1400) && defined(__STDC_WANT_SECURE_LIB__) && __STDC_WANT_SECURE_LIB__ 

   // use safe versions of strcpy etc:
   using ::strcpy_s;
   using ::strcat_s;
#else
   inline std::size_t strcpy_s(
      char *strDestination,
      std::size_t sizeInBytes,
      const char *strSource 
   )
   {
	  std::size_t lenSourceWithNull = std::strlen(strSource) + 1;
	  if (lenSourceWithNull > sizeInBytes)
         return 1;
	  std::memcpy(strDestination, strSource, lenSourceWithNull);
      return 0;
   }
   inline std::size_t strcat_s(
      char *strDestination,
      std::size_t sizeInBytes,
      const char *strSource 
   )
   {
	  std::size_t lenSourceWithNull = std::strlen(strSource) + 1;
	  std::size_t lenDestination = std::strlen(strDestination);
	  if (lenSourceWithNull + lenDestination > sizeInBytes)
         return 1;
	  std::memcpy(strDestination + lenDestination, strSource, lenSourceWithNull);
      return 0;
   }

#endif

   inline void overflow_error_if_not_zero(std::size_t i)
   {
      if(i)
      {
         std::overflow_error e("String buffer too small");
         boost::throw_exception(e);
      }
   }

}} // namespaces

#endif // __cplusplus

#endif // include guard

