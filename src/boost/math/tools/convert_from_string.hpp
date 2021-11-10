//  Copyright John Maddock 2016.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_CONVERT_FROM_STRING_INCLUDED
#define BOOST_MATH_TOOLS_CONVERT_FROM_STRING_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#include <type_traits>
#include <boost/math/tools/lexical_cast.hpp>

namespace boost{ namespace math{ namespace tools{

   template <class T>
   struct convert_from_string_result
   {
      typedef typename std::conditional<std::is_constructible<T, const char*>::value, const char*, T>::type type;
   };

   template <class Real>
   Real convert_from_string(const char* p, const std::false_type&)
   {
#ifdef BOOST_MATH_NO_LEXICAL_CAST
      // This function should not compile, we don't have the necessary functionality to support it:
      static_assert(sizeof(Real) == 0, "boost.lexical_cast is not supported in standalone mode.");
      (void)p; // Supresses -Wunused-parameter
      return Real(0);
#else
      return boost::lexical_cast<Real>(p);
#endif
   }
   template <class Real>
   constexpr const char* convert_from_string(const char* p, const std::true_type&) noexcept
   {
      return p;
   }
   template <class Real>
   constexpr typename convert_from_string_result<Real>::type convert_from_string(const char* p) noexcept((std::is_constructible<Real, const char*>::value))
   {
      return convert_from_string<Real>(p, std::is_constructible<Real, const char*>());
   }

} // namespace tools
} // namespace math
} // namespace boost

#endif // BOOST_MATH_TOOLS_CONVERT_FROM_STRING_INCLUDED

