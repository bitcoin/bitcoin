//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_FUNCTIONAL_RELATIONAL_HPP
#define BOOST_COMPUTE_FUNCTIONAL_RELATIONAL_HPP

#include <boost/compute/functional/detail/macros.hpp>

namespace boost {
namespace compute {

BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(isequal, int (T, T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(isnotequal, int (T, T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(isgreater, int (T, T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(isgreaterequal, int (T, T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(isless, int (T, T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(islessequal, int (T, T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(islessgreater, int (T, T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(isfinite, int (T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(isinf, int (T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(isnan, int (T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(isnormal, int (T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(isordered, int (T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(isunordered, int (T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION_UNDERSCORE(signbit, int (T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION(any, int (T), class T)
BOOST_COMPUTE_DECLARE_BUILTIN_FUNCTION(all, int (T), class T)

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_FUNCTIONAL_RELATIONAL_HPP
