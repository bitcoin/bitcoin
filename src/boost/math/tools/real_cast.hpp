//  Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_REAL_CAST_HPP
#define BOOST_MATH_TOOLS_REAL_CAST_HPP

#include <boost/math/tools/config.hpp>

#ifdef _MSC_VER
#pragma once
#endif

namespace boost{ namespace math
{
  namespace tools
  {
    template <class To, class T>
    inline constexpr To real_cast(T t) noexcept(BOOST_MATH_IS_FLOAT(T) && BOOST_MATH_IS_FLOAT(To))
    {
       return static_cast<To>(t);
    }
  } // namespace tools
} // namespace math
} // namespace boost

#endif // BOOST_MATH_TOOLS_REAL_CAST_HPP



