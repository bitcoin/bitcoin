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

#include <boost/regex/config.hpp>
#include <algorithm>
#include <stdexcept>
#include <cstring>

#ifndef BOOST_REGEX_STANDALONE
#include <boost/detail/workaround.hpp>
#include <boost/throw_exception.hpp>
#endif

#ifdef BOOST_REGEX_NO_BOOL
#  define BOOST_REGEX_MAKE_BOOL(x) static_cast<bool>((x) ? true : false)
#else
#  define BOOST_REGEX_MAKE_BOOL(x) static_cast<bool>(x)
#endif

/*****************************************************************************
 *
 *  helper functions pointer_construct/pointer_destroy:
 *
 ****************************************************************************/

#ifdef __cplusplus
namespace boost{ namespace BOOST_REGEX_DETAIL_NS{

#ifdef BOOST_REGEX_MSVC
#pragma warning (push)
#pragma warning (disable : 4100)
#endif

template <class T>
inline void pointer_destroy(T* p)
{ p->~T(); (void)p; }

#ifdef BOOST_REGEX_MSVC
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

#if defined(BOOST_WORKAROUND)
#if BOOST_WORKAROUND(BOOST_REGEX_MSVC, >= 1400) && defined(__STDC_WANT_SECURE_LIB__) && __STDC_WANT_SECURE_LIB__
#define BOOST_REGEX_HAS_STRCPY_S
#endif
#endif

#ifdef __cplusplus
namespace boost{ namespace BOOST_REGEX_DETAIL_NS{

#if defined(BOOST_REGEX_MSVC) && (BOOST_REGEX_MSVC < 1910)
   //
   // MSVC 10 will either emit warnings or else refuse to compile
   // code that makes perfectly legitimate use of std::copy, when
   // the OutputIterator type is a user-defined class (apparently all user 
   // defined iterators are "unsafe").  What's more Microsoft have removed their
   // non-standard "unchecked" versions, even though they are still in the MS
   // documentation!! Work around this as best we can: 
   //
   template<class InputIterator, class OutputIterator>
   inline OutputIterator copy(
      InputIterator first,
      InputIterator last,
      OutputIterator dest
   )
   {
      while (first != last)
         *dest++ = *first++;
      return dest;
   }
#else 
   using std::copy;
#endif 


#if defined(BOOST_REGEX_HAS_STRCPY_S)

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
#ifndef BOOST_REGEX_STANDALONE
         boost::throw_exception(e);
#else
         throw e;
#endif
      }
   }

}} // namespaces

#endif // __cplusplus

#endif // include guard

